#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"
#include "Stopwatch.hpp"

using namespace std;

const unsigned int kTrieDepth = 2;
const unsigned int kNumThreads = 8;
const int kLevenshteinMaxCost = 2;
const size_t kMinLengthForLevenshteinCompletion = 4;

Buffer::Buffer()
:
words_(NULL),
trie_(NULL),
title_cases_(NULL),
underscores_(NULL),
abbreviations_dirty_(true),
cursor_pos_(0, 0)
{
    // generate lookup tables
    std::string temp(1, ' ');
    for (size_t index = 0; index <= 255; ++index)
    {
        is_part_of_word_[index] = IsPartOfWord(index) ? 1 : 0;
        temp.resize(1, ' ');
        temp[0] = (char)index;
        boost::algorithm::to_lower(temp);
        to_lower_[index] = temp[0];
    }

    // initialize words_ so it doesn't crash
    tokenizeKeywords();
}

Buffer::~Buffer()
{
    ;
}

bool Buffer::operator==(const Buffer& other)
{
    return (this->buffer_id_ == other.buffer_id_);
}

void Buffer::ParseInsertMode(
    const std::string& new_contents,
    const std::string& cur_line,
    std::pair<unsigned, unsigned> cursor_pos)
{
    // if our cursor row has changed, then capture the contents of the original
    // current line before any changes have occurred
    if (cursor_pos.first != cursor_pos_.first)
    {
        initial_current_line_ = cur_line;
        tokenizeKeywordsOfOriginalCurrentLine(initial_current_line_);

        // by changing lines in insert mode, we have to reparse buffer
        // to keep it current
        if (contents_ != new_contents)
        {
            contents_ = new_contents;
            tokenizeKeywords();
        }
    }

    // calling the function implies that the current line has changed,
    // so parse the current line
    tokenizeKeywordsOfCurrentLine(cur_line);
    calculateCurrentWordOfCursor(cur_line, cursor_pos);

    prev_cur_line_ = cur_line;
    cursor_pos_ = cursor_pos;
}

void Buffer::ParseNormalMode(
    const std::string& new_contents)
{
    if (contents_ == new_contents) return;

    contents_ = new_contents;

    tokenizeKeywords();
}

bool Buffer::Init(Session* parent, std::string buffer_id)
{
    parent_ = parent;

    if (buffer_id.size() == 0) throw std::exception();
    buffer_id_ = buffer_id;

    return true;
}

std::string Buffer::GetBufferId() const
{
    return buffer_id_;
}

void Buffer::tokenizeKeywords()
{
    // re-tokenizing means clearing existing wordlist
    //Stopwatch watch;
    //watch.Start();
    current_line_words_.clear();
    parent_->GD.QueueForDeletion(words_);
    words_ = new boost::unordered_map<std::string, unsigned>;
    //watch.Stop(); watch.PrintResultMilliseconds();

    size_t contents_size = contents_.size();

    for (size_t ii = 0; ii < contents_size; ++ii)
    {
        // initial case, find character in the set of ([a-z][A-Z][0-9]_)
        // this will be what is considered a word
        // I guess we have unicode stuff we are screwed :
        char c = contents_[ii];
        if (!is_part_of_word_[c]) continue;

        // we have found the beginning of the word, loop until
        // we reach the end or we find a non world character
        size_t jj = ii + 1;
        for (; jj < contents_size; ++jj)
        {
            if (is_part_of_word_[contents_[jj]])
                continue;
            break;
        }
        if (jj == contents_size) break;

        // construct word based off of pointer
        std::string word(&contents_[ii], &contents_[jj]);
        (*words_)[word]++;

        // for loop will autoincrement
        ii = jj;
    }

    // since we have recreated the set of words, invalidate
    // the trie structure, next time we complete, it will be
    // regenerated again
    // same thing with TitleCase structure and userscores structure

    //watch.Start();
    parent_->GD.QueueForDeletion(trie_);
    trie_ = new TrieNode();
    //watch.Stop(); watch.PrintResultMilliseconds();

    //watch.Start();
    //title_cases_.clear();
    //watch.Stop(); watch.PrintResultMilliseconds();

    //watch.Start();
    //underscores_.clear();
    //watch.Stop(); watch.PrintResultMilliseconds();

    //generateTitleCasesAndUnderscores();
    // defer abbrevation generation until we are in insert mode
    abbreviations_dirty_ = true;
}

void Buffer::tokenizeKeywordsHelper(
    const std::string& content,
    boost::unordered_set<std::string>& container)
{
    size_t contents_size = content.size();

    for (size_t ii = 0; ii < contents_size; ++ii)
    {
        // initial case, find character in the set of ([a-z][A-Z][0-9]_)
        // this will be what is considered a word
        // I guess we have unicode stuff we are screwed :(
        char c = content[ii];
        if (!is_part_of_word_[c]) continue;

        // we have found the beginning of the word, loop until
        // we reach the end or we find a non world character
        size_t jj = ii + 1;
        for (; jj < contents_size; ++jj)
        {
            if (is_part_of_word_[content[jj]])
                continue;
            break;
        }
        if (jj == contents_size) break;

        // construct word based off of pointer
        std::string word(&content[ii], &content[jj]);
        container.insert(word);

        // for loop will autoincrement
        ii = jj;
    }
}

void Buffer::tokenizeKeywordsOfCurrentLine(const std::string& line)
{
    current_line_words_.clear();
    tokenizeKeywordsHelper(line, current_line_words_);
}

void Buffer::tokenizeKeywordsOfOriginalCurrentLine(const std::string& line)
{
    orig_cur_line_words_.clear();
    tokenizeKeywordsHelper(line, orig_cur_line_words_);
}

void Buffer::calculateCurrentWordOfCursor(
    const std::string& line,
    const std::pair<unsigned, unsigned> pos)
{
    int end_bound = pos.second;
    while ( is_part_of_word_[line[end_bound]] &&
            end_bound < static_cast<int>(line.size()) )
    {
        end_bound++;
    }

    int begin_bound = pos.second - 1;
    while ( begin_bound >= 0 &&
            is_part_of_word_[line[begin_bound]] )
    {
        begin_bound--;
    }

    // increment by one to start on the actual word
    begin_bound++;

    current_cursor_word_ = std::string(
        line.begin() + begin_bound,
        line.begin() + end_bound);
}

void Buffer::GetAllWordsWithPrefix(
    const std::string& prefix,
    std::set<std::string>* results)
{
    foreach (WordsIterator& word_count_pair, *words_)
    {
        const std::string& word = word_count_pair.first;
        if (boost::starts_with(word, prefix))
        {
            // check to make sure really exists
            unsigned count = word_count_pair.second;
            if (count == 1 &&
                // there is only one instance in the buffer
                // and this line originally has it
                Contains(orig_cur_line_words_, word) == true &&
                // but the current line doesn't have it
                Contains(current_line_words_, word) == false)
            {
                // then the word doesn't anymore, it's just that the buffer
                // hasn't been reparsed (most likely triggered by backspace)
                continue;
            }

            results->insert(word);
        }
    }
}

void Buffer::GetAllWordsWithPrefixFromCurrentLine(
    const std::string& prefix,
    std::set<std::string>* results)
{
    foreach (const std::string& word, current_line_words_)
    {
        if (boost::starts_with(word, prefix) &&
            word != current_cursor_word_)
        {
            results->insert(word);
        }
    }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Buffer::levenshteinSearch(
    const std::string& word,
    int max_cost,
    LevenshteinSearchResults& results)
{
    // generate sequence from [0, len(word)]
    size_t current_row_end = word.length() + 1;
    std::vector<int> current_row;
    for (size_t ii = 0; ii < current_row_end; ++ii) current_row.push_back(ii);

    foreach (TrieNode::ChildrenIterator& child, trie_->Children)
    {
        char letter = child.first;
        TrieNode* next_node = child.second;
        levenshteinSearchRecursive(
            next_node,
            letter,
            word,
            current_row,
            results,
            max_cost);
    }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Buffer::levenshteinSearchRecursive(
    TrieNode* node,
    char letter,
    const std::string& word,
    const std::vector<int>& previous_row,
    LevenshteinSearchResults& results,
    int max_cost)
{
    size_t columns = word.length() + 1;
    std::vector<int> current_row;
    current_row.push_back( previous_row[0] + 1 );

    // Build one row for the letter, with a column for each letter in the target
    // word, plus one for the empty string at column 0
    for (unsigned column = 1; column < columns; ++column)
    {
        int insert_cost = current_row[column - 1] + 1;
        int delete_cost = previous_row[column] + 1;

        int replace_cost;
        if (word[column - 1] != letter)
            replace_cost = previous_row[column - 1] + 1;
        else
            replace_cost = previous_row[column - 1];

        current_row.push_back( (std::min)(
            insert_cost,
            (std::min)(delete_cost, replace_cost)) );
    }

    // if the last entry in the row indicates the optimal cost is less than the
    // maximum cost, and there is a word in this trie node, then add it.
    size_t last_index = current_row.size() - 1;
    if ( (current_row[last_index] <= max_cost) &&
         (node->Word != NULL) &&
         (node->Word->length() > 0) )
    {
        results[ current_row[last_index] ].insert(*node->Word);
    }

    // if any entries in the row are less than the maximum cost, then
    // recursively search each branch of the trie
    if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost)
    {
        foreach (TrieNode::ChildrenIterator& child, node->Children)
        {
            char letter = child.first;
            TrieNode* next_node = child.second;
            levenshteinSearchRecursive(
                next_node,
                letter,
                word,
                current_row,
                results,
                max_cost);
        }
    }
}

void Buffer::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results)
{
    if ((words_->size()) > 0 && trie_->Empty())
    {
        //Stopwatch watch; watch.Start();
        foreach (WordsIterator& word_count_pair, *words_)
        {
            trie_->Insert(&word_count_pair.first);
        }
        //watch.Stop();
        //std::cout << "trie_ creation time: "; watch.PrintResultMilliseconds();
    }

    // only try fancier matching if we have a sufficiently long enough
    // word to make it worthwhile
    if (prefix.length() < kMinLengthForLevenshteinCompletion) return;

    levenshteinSearch(prefix, kLevenshteinMaxCost, results);
}

void Buffer::generateTitleCasesAndUnderscores()
{
    parent_->GD.QueueForDeletion(title_cases_);
    title_cases_ = new boost::unordered_multimap<std::string, const std::string*>;

    parent_->GD.QueueForDeletion(underscores_);
    underscores_ = new boost::unordered_multimap<std::string, const std::string*>;

    foreach (WordsIterator& word_count_pair, *words_)
    {
        const std::string& word = word_count_pair.first;
        if (word.length() <= 2) continue;

        // check if we have cached the result of the word
        // finding the word in title_case_cache_ implies that we will also
        // find the same word in underscore_cache_ because of how it is
        // done below
        if (Contains(title_case_cache_, word))
        {
            if (title_case_cache_[word].empty() == false)
            {
                title_cases_->insert(make_pair(title_case_cache_[word], &word));
            }
            if (underscore_cache_[word].empty() == false)
            {
                underscores_->insert(make_pair(underscore_cache_[word], &word));
            }
            continue;
        }

        std::string title_case, underscore;
        CalculateTitlecaseAndUnderscore(word, to_lower_, title_case, underscore);

        if (title_case.length() >= 2)
        {
            title_cases_->insert(std::make_pair(title_case, &word));
            // cache abbreviation calculation
            title_case_cache_[word] = title_case;
        }
        else
        {
            // mark as not valid calculation
            title_case_cache_[word] = "";
        }

        if (underscore.length() >= 2)
        {
            underscores_->insert(std::make_pair(underscore, &word));
            // cache abbreviation calculation
            underscore_cache_[word] = underscore;
        }
        else
        {
            // mark as not valid calculation
            underscore_cache_[word] = "";
        }
    }

    abbreviations_dirty_ = false;
}

void Buffer::GetAbbrCompletions(
    const std::string& prefix,
    std::set<std::string>* results)
{
    // generate abbreviations if they already haven't
    if (abbreviations_dirty_)
    {
        //Stopwatch watch; watch.Start();
        generateTitleCasesAndUnderscores();
        //watch.Stop();
        //std::cout << "generate abbr: "; watch.PrintResultMilliseconds();
    }

    if (prefix.length() < 2) return;

    auto(bounds1, title_cases_->equal_range(prefix));
    auto(&ii, bounds1.first);
    for (ii = bounds1.first; ii != bounds1.second; ++ii)
    {
        results->insert(*ii->second);
    }

    auto(bounds2, underscores_->equal_range(prefix));
    for (ii = bounds2.first; ii != bounds2.second; ++ii)
    {
        results->insert(*ii->second);
    }
}
