#pragma once

#include "TrieNode.hpp"
#include "CompleteItem.hpp"
#include "WordInfo.hpp"
#include "AbbreviationInfo.hpp"

typedef std::map<size_t, std::set<std::string> > LevenshteinSearchResults;

class WordCollection : public boost::noncopyable {
 public:
  WordCollection(bool enable_trie);
  ~WordCollection() {}

  void UpdateWords(const UnorderedStringIntMap* word_ref_count_deltas);
  size_t Prune();

  void GetLevenshteinCompletions(const std::string& prefix,
                                 LevenshteinSearchResults& results);

  void Lock() { mutex_.lock(); }
  void Unlock() { mutex_.unlock(); }
  const std::vector<String>& GetWordList() const { return word_list_; }

 private:
  // NOT multithread safe!
  void updateWord(const std::string& word, int reference_count_delta);

  boost::mutex mutex_;

  boost::unordered_map<String, WordInfo> words_;
  std::vector<String> word_list_;

  bool trie_enabled_;
  TrieNode* trie_;

  static int rand_seed_;
  static inline int fastrand() {
    rand_seed_ = 214013 * rand_seed_ + 2531011;
    return (rand_seed_ >> 16) & 0x7FFF;
  }
};
