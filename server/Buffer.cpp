#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"
#include "Stopwatch.hpp"

using namespace std;

const unsigned int kTrieDepth = 2;
const unsigned int kNumThreads = 8;
const int kLevenshteinMaxCost = 2;
const size_t kMinLengthForLevenshteinCompletion = 4;

char Buffer::is_part_of_word_[256];
char Buffer::to_lower_[256];

void Buffer::GlobalInit()
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

Buffer::Buffer()
{
}

Buffer::~Buffer()
{
    foreach (const std::string& word, *words_) {
        parent_->WordSet.UpdateWord(word, -1);
    }
}

bool Buffer::operator==(const Buffer& other)
{
    return (this->buffer_id_ == other.buffer_id_);
}

bool Buffer::Init(Session* parent, unsigned buffer_id)
{
    parent_ = parent;
    buffer_id_ = buffer_id;

    return true;
}

unsigned Buffer::GetBufferId() const
{
    return buffer_id_;
}

void Buffer::ReplaceContents(StringPtr new_contents)
{
    // null pointer passed in
    if (!new_contents) return;

    // we already have parsed something before and the
    // new content we replace with is the exact samething
    if (contents_ && *contents_ == *new_contents) return;

    UnorderedStringSetPtr new_words = boost::make_shared<UnorderedStringSet>();
    Buffer::TokenizeContentsIntoKeywords(new_contents, new_words);

    if (!contents_) {
        // easy first case, just increment reference count for each word
        foreach (const std::string& word, *new_words) {
            parent_->WordSet.UpdateWord(word, 1);
        }
    } else {
        // otherwise we have to a slow calculation of words to add and words
        // to remove
        // since this happens in a background thread, it is okay
        std::vector<std::string> to_be_removed;
        std::vector<std::string> to_be_added;

        // calculated words to be removed, by checking lack of existsnce
        // in new_words set
        foreach (const std::string& word, *words_) {
            if (Contains(*new_words, word) == false) {
                to_be_removed.push_back(word);
            }
        }

        // calcule words to be added, by checking checking lack of existence
        // in the old set
        foreach (const std::string& word, *new_words) {
            if (Contains(*words_, word) == false) {
                to_be_added.push_back(word);
            }
        }

        foreach (const std::string& word, to_be_added) {
            parent_->WordSet.UpdateWord(word, 1);
        }
        foreach (const std::string& word, to_be_removed) {
            parent_->WordSet.UpdateWord(word, -1);
        }
    }

    // replace old contents and words with thew new ones
    contents_ = new_contents;
    words_ = new_words;
}

//void Buffer::ParseInsertMode(
    //const std::string& new_contents,
    //const std::string& cur_line,
    //std::pair<unsigned, unsigned> cursor_pos)
//{
    // if our cursor row has changed, then capture the contents of the original
    // current line before any changes have occurred
    //if (cursor_pos.first != cursor_pos_.first)
    //{
        //initial_current_line_ = cur_line;
        //tokenizeKeywordsOfOriginalCurrentLine(initial_current_line_);

        //// by changing lines in insert mode, we have to reparse buffer
        //// to keep it current
        //if (contents_ != new_contents)
        //{
            //contents_ = new_contents;
            //tokenizeKeywords();
        //}
    //}

    // calling the function implies that the current line has changed,
    // so parse the current line
    //tokenizeKeywordsOfCurrentLine(cur_line);
    //calculateCurrentWordOfCursor(cur_line, cursor_pos);

    //prev_cur_line_ = cur_line;
    //cursor_pos_ = cursor_pos;
//}

void Buffer::TokenizeContentsIntoKeywords(
    StringPtr contents,
    UnorderedStringSetPtr words)
{
    const std::string& text = *contents;
    size_t contents_size = text.size();
    for (size_t ii = 0; ii < contents_size; ++ii)
    {
        // initial case, find character in the set of ([a-z][A-Z][0-9]_)
        // this will be what is considered a word
        // I guess we have unicode stuff we are screwed :
        char c = text[ii];
        if (!is_part_of_word_[c]) continue;

        // we have found the beginning of the word, loop until
        // we reach the end or we find a non world character
        size_t jj = ii + 1;
        for (; jj < contents_size; ++jj)
        {
            if (is_part_of_word_[text[jj]])
                continue;
            break;
        }

        // construct word based off of pointer
        std::string word(
            text.begin() + ii,
            text.begin() + jj);
        (*words).insert(word);

        // for loop will autoincrement
        ii = jj;
    }
}

void Buffer::CalculateCurrentWordOfCursor(
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
