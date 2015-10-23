#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Omegacomplete.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"

using namespace std;

void Buffer::TokenizeContentsIntoKeywords(StringPtr contents,
                                          UnorderedStringIntMapPtr words) {
  const std::string& text = *contents;
  size_t len = text.size();
  for (size_t i = 0; i < len; ++i) {
    // initial case, find continuous sequences in the set of [a-zA-Z0-9_\-]
    // This will be what is considered a "word".  I guess we have Unicode stuff
    // then we are screwed :-(
    uchar c = static_cast<uchar>(text[i]);
    if (!mIskeyword[c]) continue;

    // we have found the beginning of the word, loop until we reach the end or
    // we find a non-"word" character.
    size_t j = i + 1;
    for (; j < len; ++j) {
      if (mIskeyword[static_cast<uchar>(text[j])]) continue;
      break;
    }

    // construct word based off of pointer
    // don't want "words" that end in hyphen
    if (j > i) {
      std::string word(text.begin() + i, text.begin() + j);
      (*words)[word]++;
    }

    // for loop will autoincrement
    i = j;
  }
}

Buffer::~Buffer() {
  if (parent_ == NULL) return;
  if (words_ == NULL) return;

  AUTO(i, words_->begin());
  for (; i != words_->end(); ++i) {
    i->second = -i->second;
    ;
  }
  parent_->Words.UpdateWords(words_.get());
}

void Buffer::Init(Omegacomplete* parent, unsigned buffer_id) {
  parent_ = parent;
  buffer_id_ = buffer_id;
}

void Buffer::ReplaceContentsWith(StringPtr new_contents) {
  if (!new_contents) return;

  // we have already parsed the exact samething, no need to reparse again
  if (contents_ && *contents_ == *new_contents) return;

  UnorderedStringIntMapPtr new_words;
  if (words_) {
    // prevent unnecessary rehashing on resize by allocating at least the
    // number of buckets of the previous hash table.
    new_words =
        boost::make_shared<UnorderedStringIntMap>(words_->bucket_count());
  } else {
    new_words = boost::make_shared<UnorderedStringIntMap>();
  }
  Buffer::TokenizeContentsIntoKeywords(new_contents, new_words);

  if (!contents_) {
    // easy first case, just increment reference count for each word
    parent_->Words.UpdateWords(new_words.get());
  } else {
    // otherwise we have to a slow calculation of words to add and words to
    // remove. since this happens in a background thread, it is okay to be
    // slow.
    UnorderedStringIntMap to_be_removed;
    UnorderedStringIntMap to_be_added;
    UnorderedStringIntMap to_be_changed;

    // calculated words to be removed, by checking lack of existence in
    // new_words set
    AUTO(i, words_->begin());
    for (; i != words_->end(); ++i) {
      const std::string& word = i->first;
      const int ref_count = i->second;
      if (!Contains(*new_words, word)) to_be_removed[word] -= ref_count;
    }

    // calculate words to be added, by checking checking lack of existence
    // in the old set
    AUTO(j, new_words->begin());
    for (; j != new_words->end(); ++j) {
      const std::string& word = j->first;
      const int ref_count = j->second;
      if (!Contains(*words_, word)) to_be_added[word] += ref_count;
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

    parent_->Words.UpdateWords(&to_be_added);
    parent_->Words.UpdateWords(&to_be_removed);
    parent_->Words.UpdateWords(&to_be_changed);
  }

  // replace old contents and words with thew new ones
  contents_ = new_contents;
  words_ = new_words;
}

static int read_char(const std::string& str, int i, uchar* outch) {
  if (!LookupTable::IsNumber[str[i]]) {
    *outch = str[i];
    return i + 1;
  } else {
    const int n = str.size();
    int digit = 0;
    while (i < n && LookupTable::IsNumber[str[i]]) {
      digit = 10 * digit + (str[i] - '0');
      i++;
    }

    *outch = (uchar)digit;
    return i;
  }
}

void Buffer::SetIskeyword(std::string iskeyword) {
  if (mIskeyword == iskeyword) return;
  mIskeyword = iskeyword;

  for (int i = 0; i < 256; ++i) {
    mIsWord[i] = 0;
  }

  enum Mode : int {
    kAcceptFirstChar,
    kMaybeAcceptSecondChar,
    kAcceptSecondChar,
  };

  const int n = iskeyword.size();
  int i = 0;
  uchar ch = 0;
  uchar begin_char = 0;
  uchar end_char = 0;
  int mode = 0;
  bool process = false;
  bool exclude = false;
  while (i <= n) {
    if (process || i == n) {
      for (char c = begin_char; c <= end_char; ++c) {
        mIsWord[c] = !exclude;
      }

      begin_char = 0;
      end_char = 0;
      process = false;
      exclude = false;

      if (i == n)
        break;
      else
        ++i;
    }

    ch = (uchar)iskeyword[i];

    if (mode == kAcceptFirstChar) {
      i = read_char(iskeyword, i, &begin_char);
      end_char = begin_char;
      mode++;
    }
    else if (mode == kMaybeAcceptSecondChar) {
      if (begin_char == '^') {
        exclude = true;
        begin_char = ch;
        i++;
      } else if (ch == '-') {
        mode++;
        i++;
      } else if (ch == ',') {
        process = true;
        i++;
        mode++;
        continue;
      } else {
        assert(false);
      }
    }
    else if (mode == kAcceptSecondChar) {
      i = read_char(iskeyword, i, &end_char);
    }
  }
}
