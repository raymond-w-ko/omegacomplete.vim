#include "stdafx.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "WordCollection.hpp"
#include "CompletionPriorities.hpp"

static const int kLevenshteinMaxCost = 2;
static const size_t kMinLengthForLevenshteinCompletion = 4;

int WordCollection::rand_seed_ = 42;

WordCollection::WordCollection(bool enable_trie) : trie_enabled_(enable_trie) {
  if (trie_enabled_) {
    trie_ = new TrieNode(NULL, 'Q');
  } else {
    trie_ = NULL;
  }
}

void WordCollection::UpdateWords(const UnorderedStringIntMap* word_ref_count_deltas) {
  boost::mutex::scoped_lock lock(mutex_);

  AUTO(end, word_ref_count_deltas->end());
  for (AUTO(iter, word_ref_count_deltas->begin()); iter != end; ++iter) {
    updateWord(iter->first, iter->second);
  }
}

void WordCollection::updateWord(const std::string& word, int reference_count_delta) {
  WordInfo& wi = words_[word];
  wi.ReferenceCount += reference_count_delta;
  if (wi.ReferenceCount > 0) {
    // < 0 implies it doesn't exist in word_list_ yet
    if (wi.WordListIndex < 0) {
      int index = static_cast<int>(word_list_.size());
      word_list_.push_back(word);
      wi.WordListIndex = index;

      if (trie_enabled_) {
        trie_->Insert(word);
      }
    }
  } else {
    // >= 0 implies that it exist in the word list and we need to free it
    if (wi.WordListIndex >= 0) {
      size_t n = word_list_.size();

      int index = wi.WordListIndex;
      wi.WordListIndex = -1;
      if (n == 1) {
        word_list_.clear();
      } else {
        String last_word = word_list_[n - 1];
        std::swap(word_list_[index], word_list_[n - 1]);
        word_list_.resize(n - 1);
        words_[last_word].WordListIndex = index;
      }
      words_.erase(word);

      if (trie_enabled_) {
        trie_->Erase(word);
      }
    }
  }
}

size_t WordCollection::Prune() {
  boost::mutex::scoped_lock lock(mutex_);

  // Fisher-Yates shuffle the word_list_ to evenly spread out words so
  // computationally intensive ones don't clump together in one core.
  // Only shuffle 10% of the words though for speed purposes
  int num_words = (int)words_.size();
  for (int i = num_words / 10; i > 0; --i) {
    int j = fastrand() % (i + 1);

    String word1 = word_list_[i];
    WordInfo& word_info1 = words_[word1];

    String word2 = word_list_[j];
    WordInfo& word_info2 = words_[word2];

    word_list_[i] = word2;
    word_info2.WordListIndex = i;
    word_list_[j] = word1;
    word_info1.WordListIndex = j;
  }

  return 0;
}

void WordCollection::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results) {
  boost::mutex::scoped_lock lock(mutex_);

  // only try correction if we have a sufficiently long enough
  // word to make it worthwhile
  if (prefix.length() < kMinLengthForLevenshteinCompletion)
    return;

  if (trie_enabled_) {
    Algorithm::LevenshteinSearch(prefix, kLevenshteinMaxCost, *trie_, results);
  }
}
