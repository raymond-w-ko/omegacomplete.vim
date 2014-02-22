#include "stdafx.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "WordCollection.hpp"
#include "CompletionPriorities.hpp"

static const int kLevenshteinMaxCost = 2;
static const size_t kMinLengthForLevenshteinCompletion = 4;

WordCollection::WordCollection(bool enable_trie) 
    : trie_enabled_(enable_trie) {
  trie_ = new TrieNode(NULL, 'Q');
}

void WordCollection::UpdateWord(const std::string& word,
                                int reference_count_delta) {
  boost::mutex::scoped_lock lock(mutex_);

  WordInfo& wi = words_[word];
  wi.ReferenceCount += reference_count_delta;
  if (wi.ReferenceCount > 0) {
    // < 0 implies it doesn't exist in word_list_ yet
    if (wi.WordListIndex < 0) {
      int index;
      if (empty_indices_.size() > 0) {
        index = *empty_indices_.begin();
        empty_indices_.erase(index);
      } else {
        index = static_cast<int>(word_list_.size());
      }
      wi.WordListIndex = index;
      word_list_[index] = word;
    }

    if (trie_enabled_) {
      boost::mutex::scoped_lock trie_lock(trie_mutex_);
      trie_->Insert(word);
    }

  } else {
    // >= 0 implies that it exist in the word list and we need to free it
    if (wi.WordListIndex >= 0) {
      int index = wi.WordListIndex;
      wi.WordListIndex = -1;
      word_list_[index].clear();
      empty_indices_.insert(index);
    }

    if (trie_enabled_) {
      boost::mutex::scoped_lock trie_lock(trie_mutex_);
      trie_->Erase(word);
    }
  }
}

size_t WordCollection::Prune() {
  boost::mutex::scoped_lock lock(mutex_);

  // simply just looping through words_ and removing things might cause
  // iterator invalidation, so be safe and built a set of things to remove
  std::vector<std::string> to_be_pruned;
  AUTO(i, words_.begin());
  for (; i != words_.end(); ++i) {
    if (i->second.ReferenceCount > 0)
      continue;
    to_be_pruned.push_back(i->first);
  }

  foreach (const std::string& word, to_be_pruned) {
    words_.erase(word);

    if (trie_enabled_) {
      boost::mutex::scoped_lock trie_lock(trie_mutex_);
      trie_->Erase(word);
    }
  }

  // rebuild word_list_ to eliminate any holes
  empty_indices_.clear();
  word_list_.clear();
  AUTO(iter, words_.begin());
  int index = 0;
  for (; iter != words_.end(); ++iter) {
    const std::string& word = iter->first;
    WordInfo& wi = iter->second;

    word_list_[index] = word;
    wi.WordListIndex = index;
    index++;
  }

  // Fisher-Yates shuffle the word_list_ to evenly spread out words so
  // computationally intensive ones don't clump together in one core.

  int num_words = (int)words_.size();
  for (int i = num_words - 1; i > 0; --i) {
    boost::random::uniform_int_distribution<> dist(0, i);
    int j = dist(rng_);

    String word1 = word_list_[i];
    WordInfo& word_info1 = words_[word1];

    String word2 = word_list_[j];
    WordInfo& word_info2 = words_[word2];

    word_list_[i] = word2;
    word_info2.WordListIndex = i;
    word_list_[j] = word1;
    word_info1.WordListIndex = j;
  }

  return to_be_pruned.size();
}

void WordCollection::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results) {
  // only try correction if we have a sufficiently long enough
  // word to make it worthwhile
  if (prefix.length() < kMinLengthForLevenshteinCompletion)
    return;

  if (trie_enabled_) {
    boost::mutex::scoped_lock lock(trie_mutex_);
    Algorithm::LevenshteinSearch(
        prefix, kLevenshteinMaxCost, *trie_, results);
  }
}
