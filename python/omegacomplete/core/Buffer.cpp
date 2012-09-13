#include "stdafx.hpp"

#include "Buffer.hpp"
#include "OmegaComplete.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"

using namespace std;

Buffer::Buffer()
:
parent_(NULL)
{
}

Buffer::~Buffer()
{
    if (parent_ == NULL) return;
    if (words_ == NULL) return;

    auto(i, words_->begin());
    for (; i != words_->end(); ++i)
    {
        parent_->WordSet.UpdateWord(i->first, -i->second);
    }
}

bool Buffer::operator==(const Buffer& other)
{
    return (this->buffer_id_ == other.buffer_id_);
}

bool Buffer::Init(OmegaComplete* parent, unsigned buffer_id)
{
    parent_ = parent;
    buffer_id_ = buffer_id;

    return true;
}

unsigned Buffer::GetBufferId() const
{
    return buffer_id_;
}

void Buffer::ReplaceContentsWith(StringPtr new_contents)
{
    // null pointer passed in
    if (!new_contents) return;

    // we already have parsed something before and the
    // new content we replace with is the exact samething
    if (contents_ && *contents_ == *new_contents) return;

    StringIntMapPtr new_words =
        boost::make_shared<StringIntMap>();
    Buffer::TokenizeContentsIntoKeywords(new_contents, new_words);

    if (!contents_) {
        // easy first case, just increment reference count for each word
        auto(i, new_words->begin());
        for (; i != new_words->end(); ++i)
        {
            parent_->WordSet.UpdateWord(i->first, +i->second);
        }
    } else {
        // otherwise we have to a slow calculation of words to add and words to
        // remove. since this happens in a background thread, it is okay
        StringIntMap to_be_removed;
        StringIntMap to_be_added;
        StringIntMap to_be_changed;

        // calculated words to be removed, by checking lack of existsnce in
        // new_words set
        auto(i, words_->begin());
        for (; i != words_->end(); ++i)
        {
            const std::string& word = i->first;
            const int ref_count = i->second;
            if (!Contains(*new_words, word))
                to_be_removed[word] += ref_count;
        }

        // calcule words to be added, by checking checking lack of existence in
        // the old set
        auto(j, new_words->begin());
        for (; j != new_words->end(); ++j)
        {
            const std::string& word = j->first;
            const int ref_count = j->second;
            if (!Contains(*words_, word))
                to_be_added[word] += ref_count;
        }

        auto(k, words_->begin());
        for (; k != words_->end(); ++k)
        {
            const std::string& word = k->first;
            const int ref_count = k->second;
            if (Contains(*new_words, word))
                to_be_changed[word] += (*new_words)[word] - ref_count;
        }

        for (StringIntMapConstIter iter = to_be_added.begin();
             iter != to_be_added.end();
             ++iter)
        {
            parent_->WordSet.UpdateWord(iter->first, +iter->second);
        }
        for (StringIntMapConstIter iter = to_be_removed.begin();
             iter != to_be_removed.end();
             ++iter)
        {
            parent_->WordSet.UpdateWord(iter->first, -iter->second);
        }
        for (StringIntMapConstIter iter = to_be_changed.begin();
             iter != to_be_changed.end();
             ++iter)
        {
            parent_->WordSet.UpdateWord(iter->first,  iter->second);
        }
    }

    // replace old contents and words with thew new ones
    contents_ = new_contents;
    words_ = new_words;
}

void Buffer::TokenizeContentsIntoKeywords(
    StringPtr contents,
    StringIntMapPtr words)
{
    const std::string& text = *contents;
    size_t contents_size = text.size();
    for (size_t ii = 0; ii < contents_size; ++ii)
    {
        // initial case, find character in the set of ([a-z][A-Z][0-9]_)
        // this will be what is considered a word
        // I guess we have unicode stuff we are screwed :
        char c = text[ii];
        if (!LookupTable::IsPartOfWord[c])
            continue;

        // we have found the beginning of the word, loop until
        // we reach the end or we find a non world character
        size_t jj = ii + 1;
        for (; jj < contents_size; ++jj)
        {
            c = text[jj - 1];

            if (LookupTable::IsPartOfWord[text[jj]])
                continue;
            break;
        }

        // construct word based off of pointer
        // don't want "words" that end in hyphen
        std::string word(
            text.begin() + ii,
            text.begin() + jj + (c == '-' ? -1 : 0));
        (*words)[word]++;

        // for loop will autoincrement
        ii = jj;
    }
}

void Buffer::CalculateCurrentWordOfCursor(
    const std::string& line,
    const FileLocation& pos)
{
    int end_bound = pos.Column;
    while (LookupTable::IsPartOfWord[line[end_bound]] &&
           end_bound < static_cast<int>(line.size()))
    {
        end_bound++;
    }

    int begin_bound = pos.Column - 1;
    while ( begin_bound >= 0 &&
            LookupTable::IsPartOfWord[line[begin_bound]] )
    {
        begin_bound--;
    }

    // increment by one to start on the actual word
    begin_bound++;

    current_cursor_word_ = std::string(
        line.begin() + begin_bound,
        line.begin() + end_bound);
}
