#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Omegacomplete.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"

using namespace std;

void Buffer::TokenizeContentsIntoKeywords(
    StringPtr contents,
    UnorderedStringIntMapPtr words) {
  const std::string& text = *contents;
  size_t len = text.size();
  for (size_t i = 0; i < len; ++i) {
    // initial case, find continuous sequences in the set of [a-zA-Z0-9_\-]
    // This will be what is considered a "word".  I guess we have Unicode stuff
    // then we are screwed :-(
    uchar c = static_cast<uchar>(text[i]);
    if (!LookupTable::IsPartOfWord[c])
      continue;

    // we have found the beginning of the word, loop until we reach the end or
    // we find a non-"word" character.
    size_t j = i + 1;
    for (; j < len; ++j) {
      if (LookupTable::IsPartOfWord[static_cast<uchar>(text[j])])
        continue;
      break;
    }

    // strip leading hyphens (e.g. in the case of --number)
    while (i < j && i < len && text[i] == '-')
      ++i;

    size_t next_index = j;

    // strip trailing hyphens (e.g. in the case of number-)
    while (j > 0 && text[j - 1] == '-')
      --j;

    // construct word based off of pointer
    // don't want "words" that end in hyphen
    if (j > i) {
      std::string word(text.begin() + i,
                       text.begin() + j);
      (*words)[word]++;
    }

    // for loop will autoincrement
    i = next_index;
  }
}

Buffer::~Buffer() {
    if (parent_ == NULL)
        return;
    if (words_ == NULL)
        return;

    AUTO(i, words_->begin());
    for (; i != words_->end(); ++i) {
        parent_->Words.UpdateWord(i->first, -i->second);
    }
}

void Buffer::Init(Omegacomplete* parent, unsigned buffer_id) {
  parent_ = parent;
  buffer_id_ = buffer_id;
}

void Buffer::ReplaceContentsWith(StringPtr new_contents) {
  if (!new_contents)
    return;

  // we have already parsed the exact samething, no need to reparse again
  if (contents_ && *contents_ == *new_contents)
    return;

  UnorderedStringIntMapPtr new_words;
  if (words_) {
    // prevent unnecessary rehashing by allocating at least the number of
    // buckets of the previous hash table.
    new_words = boost::make_shared<UnorderedStringIntMap>(
        words_->bucket_count());
  } else {
    new_words = boost::make_shared<UnorderedStringIntMap>();
  }
  Buffer::TokenizeContentsIntoKeywords(new_contents, new_words);

  if (!contents_) {
    // easy first case, just increment reference count for each word
    AUTO(i, new_words->begin());
    for (; i != new_words->end(); ++i) {
      parent_->Words.UpdateWord(i->first, +i->second);
    }
  } else {
    // otherwise we have to a slow calculation of words to add and words to
    // remove. since this happens in a background thread, it is okay to be
    // slow.
    StringIntMap to_be_removed;
    StringIntMap to_be_added;
    StringIntMap to_be_changed;

    // calculated words to be removed, by checking lack of existence in
    // new_words set
    AUTO(i, words_->begin());
    for (; i != words_->end(); ++i) {
      const std::string& word = i->first;
      const int ref_count = i->second;
      if (!Contains(*new_words, word))
        to_be_removed[word] += ref_count;
    }

    // calculate words to be added, by checking checking lack of existence
    // in the old set
    AUTO(j, new_words->begin());
    for (; j != new_words->end(); ++j) {
      const std::string& word = j->first;
      const int ref_count = j->second;
      if (!Contains(*words_, word))
        to_be_added[word] += ref_count;
    }

    // update reference count for words that both exist in new_words and
    // words_
    AUTO(k, words_->begin());
    for (; k != words_->end(); ++k) {
      const std::string& word = k->first;
      const int ref_count = k->second;
      if (Contains(*new_words, word))
        to_be_changed[word] += (*new_words)[word] - ref_count;
    }

    for (StringIntMapConstIter iter = to_be_added.begin();
         iter != to_be_added.end();
         ++iter) {
      parent_->Words.UpdateWord(iter->first, +iter->second);
    }
    for (StringIntMapConstIter iter = to_be_removed.begin();
         iter != to_be_removed.end();
         ++iter) {
      parent_->Words.UpdateWord(iter->first, -iter->second);
    }
    for (StringIntMapConstIter iter = to_be_changed.begin();
         iter != to_be_changed.end();
         ++iter) {
      parent_->Words.UpdateWord(iter->first, iter->second);
    }
  }

  // replace old contents and words with thew new ones
  contents_ = new_contents;
  words_ = new_words;
}
