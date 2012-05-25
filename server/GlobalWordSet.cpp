#include "stdafx.hpp"
#include "GlobalWordSet.hpp"

const int kLevenshteinMaxCost = 2;
const size_t kMinLengthForLevenshteinCompletion = 4;

char GlobalWordSet::is_part_of_word_[256];
char GlobalWordSet::to_lower_[256];
boost::unordered_map<std::string, std::string> GlobalWordSet::title_case_cache_;
boost::unordered_map<std::string, std::string> GlobalWordSet::underscore_cache_;

void GlobalWordSet::GlobalInit()
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
}

const std::string& GlobalWordSet::ComputeUnderscore(const std::string& word)
{
    if (Contains(underscore_cache_, word) == true) {
        return underscore_cache_[word];
    }

    std::string underscore;

    const size_t word_length = word.length();
    for (size_t ii = 0; ii < word_length; ++ii) {
        char c = word[ii];

        if (ii == 0) {
            underscore += to_lower_[c];
            continue;
        }

        if (word[ii] == '_')
        {
            if (ii < (word_length - 1) && word[ii + 1] != '_')
                underscore += to_lower_[word[ii + 1]];
        }
    }

    if (underscore.length() < 2) underscore = "";

    // store calculation in cache
    underscore_cache_[word] = underscore;

    return underscore_cache_[word];
}

const std::string& GlobalWordSet::ComputeTitleCase(const std::string& word)
{
    if (Contains(title_case_cache_, word) == true) {
        return title_case_cache_[word];
    }

    std::string title_case;

    const size_t word_length = word.length();
    for (size_t ii = 0; ii < word_length; ++ii)
    {
        char c = word[ii];

        if (ii == 0) {
            title_case += to_lower_[c];
            continue;
        }

        if (IsUpper(c)) {
            title_case += to_lower_[c];
        }
    }

    if (title_case.length() < 2) title_case = "";

    // store calculation in cache
    title_case_cache_[word] = title_case;

    return title_case_cache_[word];
}

WordInfo::WordInfo()
:
ReferenceCount(0),
GeneratedAbbreviations(false)
{
    ;
}

WordInfo::~WordInfo()
{
    ;
}

GlobalWordSet::GlobalWordSet()
:
const_words_(words_)
{
    ;
}

GlobalWordSet::~GlobalWordSet()
{
    ;
}

void GlobalWordSet::UpdateWord(const std::string& word, int reference_count_delta)
{
    // the following below, while not being totally safe and correct, this done
    // this minimize any potential latency.
    bool lock_required;

    // if we can find the WordInfo struct that means we don't need a lock since
    // we are doing a read only operation on words_ to find the location of the
    // target WordInfo. If we can't find it, accessing the reference to it means
    // creation, which requires a lock since the data structure internally
    // will change.
    if (const_words_.find(word) != const_words_.end()) {
        lock_required = true;
    } else {
        lock_required = false;
    }

    if (lock_required) mutex_.lock();
    WordInfo& wi = words_[word];
    if (lock_required) mutex_.unlock();

    // this is not locked, which means that reads can be out of date, but
    // I don't care since I want performance.
    wi.ReferenceCount += reference_count_delta;

    if (wi.ReferenceCount <= 0) return;

    trie_mutex_.lock();
    trie_.Insert(word);
    trie_mutex_.unlock();

    if (wi.GeneratedAbbreviations) return;

    const std::string& title_case = ComputeTitleCase(word);
    const std::string& underscore = ComputeUnderscore(word);

    // generate and store abbreviations
    mutex_.lock();
    if (title_case.length() > 0) {
        title_cases_.insert(make_pair(title_case, word));
    }
    if (underscore.length() > 0) {
        underscores_.insert(make_pair(underscore, word));
    }
    mutex_.unlock();

    wi.GeneratedAbbreviations = true;
}

void GlobalWordSet::GetPrefixCompletions(
    const std::string& prefix,
    set<std::string>* completions)
{
    mutex_.lock();

    auto(iter, words_.lower_bound(prefix));
    for (; iter != words_.end(); ++iter) {
        const std::string& candidate = iter->first;
        const WordInfo& wi = iter->second;

        if (boost::starts_with(candidate, prefix) == false) break;
        if (wi.ReferenceCount == 0) continue;

        completions->insert(candidate);
    }

    mutex_.unlock();
}

void GlobalWordSet::GetAbbrCompletions(
    const std::string& prefix,
    set<std::string>* completions)
{
    mutex_.lock();

    auto(bounds1, title_cases_.equal_range(prefix));
    auto(iter, bounds1.first);
    for (iter = bounds1.first; iter != bounds1.second; ++iter) {
        const std::string& candidate = iter->second;
        if (words_[candidate].ReferenceCount == 0) continue;
        completions->insert(candidate);
    }

    auto(bounds2, underscores_.equal_range(prefix));
    for (iter = bounds2.first; iter != bounds2.second; ++iter) {
        const std::string& candidate = iter->second;
        if (words_[candidate].ReferenceCount == 0) continue;
        completions->insert(candidate);
    }

    mutex_.unlock();
}

unsigned GlobalWordSet::Prune()
{
    mutex_.lock();

    std::vector<std::string> to_be_pruned;
    auto(iter, words_.begin());
    for (; iter != words_.end(); ++iter) {
        if (iter->second.ReferenceCount != 0) continue;

        to_be_pruned.push_back(iter->first);
    }

    mutex_.unlock();

    foreach (const std::string& word, to_be_pruned) {
        mutex_.lock();
        words_.erase(word);
        title_cases_.erase(word);
        underscores_.erase(word);
        mutex_.unlock();

        trie_mutex_.lock();
        trie_.Erase(word);
        trie_mutex_.unlock();
    }

    return to_be_pruned.size();
}

void GlobalWordSet::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results)
{
    // only try correction if we have a sufficiently long enough
    // word to make it worthwhile
    if (prefix.length() < kMinLengthForLevenshteinCompletion) return;

    trie_mutex_.lock();
    levenshteinSearch(prefix, kLevenshteinMaxCost, results);
    trie_mutex_.unlock();
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void GlobalWordSet::levenshteinSearch(
    const std::string& word,
    int max_cost,
    LevenshteinSearchResults& results)
{
    // generate sequence from [0, len(word)]
    size_t current_row_end = word.length() + 1;
    std::vector<int> current_row;
    for (size_t ii = 0; ii < current_row_end; ++ii) current_row.push_back(ii);

    foreach (const TrieNode::ChildrenIterator& iter, trie_.Children) {
        char letter = iter.first;
        TrieNode* next_node = iter.second;

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
void GlobalWordSet::levenshteinSearchRecursive(
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
         (node->Word.length() > 0) )
    {
        results[ current_row[last_index] ].insert(node->Word);
    }

    // if any entries in the row are less than the maximum cost, then
    // recursively search each branch of the trie
    if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost)
    {
        foreach (const TrieNode::ChildrenIterator& iter, node->Children) {
            char letter = iter.first;
            TrieNode* next_node = iter.second;
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


