#pragma once

#include "TrieNode.hpp"
#include "CompleteItem.hpp"
#include "WordInfo.hpp"
#include "AbbreviationInfo.hpp"

typedef std::map< size_t, std::set<std::string> > LevenshteinSearchResults;

class WordCollection : public boost::noncopyable {
 public:
  WordCollection(bool enable_trie);
  ~WordCollection() {}

  void UpdateWords(const UnorderedStringIntMap* word_ref_count_deltas);
  size_t Prune();

  void GetLevenshteinCompletions(const std::string& prefix, LevenshteinSearchResults& results);

  void Lock() { mutex_.lock(); }
  void Unlock() { mutex_.unlock(); }
  const std::vector<String>& GetWordList() const { return word_list_; }

 private:
  // NOT multithread safe!
  void updateWord(const std::string& word, int reference_count_delta);

  boost::mutex mutex_;

  boost::unordered_map<String, WordInfo> words_;
  std::vector<String> word_list_;
  boost::unordered_set<int> empty_indices_;

  bool trie_enabled_;
  TrieNode* trie_;

  boost::random::mt19937 rng_;
};
