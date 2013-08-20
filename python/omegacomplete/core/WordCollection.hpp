#pragma once

#include "TrieNode.hpp"
#include "CompleteItem.hpp"
#include "WordInfo.hpp"
#include "AbbreviationInfo.hpp"

typedef std::map< size_t, std::set<std::string> > LevenshteinSearchResults;

class WordCollection : public boost::noncopyable {
 public:
  static void LevenshteinSearch(
      const std::string& word,
      size_t max_cost,
      const TrieNode& trie,
      LevenshteinSearchResults& results);

  static void LevenshteinSearchRecursive(
      const TrieNode& node,
      char letter,
      const std::string& word,
      const std::vector<size_t>& previous_row,
      LevenshteinSearchResults& results,
      size_t max_cost);

  WordCollection() {}
  ~WordCollection() {}

  void UpdateWord(const std::string& word, int reference_count_delta);
  size_t Prune();

  void GetPrefixCompletions(
      const std::string& input,
      CompleteItemVectorPtr& completions, std::set<std::string>& added_words,
      bool terminus_mode);

  void GetAbbrCompletions(
      const std::string& input,
      CompleteItemVectorPtr& completions, std::set<std::string>& added_words,
      bool terminus_mode);

  void GetLevenshteinCompletions(
      const std::string& prefix,
      LevenshteinSearchResults& results);

  void Lock() { mutex_.lock(); }
  void Unlock() { mutex_.unlock(); }

  const boost::unordered_map<int, String>& GetWordList() const {
    return word_list_;
  }

 private:
  boost::mutex mutex_;

  std::map<String, WordInfo> words_;
  std::map<String, std::set<AbbreviationInfo> > abbreviations_;
  boost::unordered_map<int, String> word_list_;
  boost::unordered_set<int> empty_indices_;

  boost::mutex trie_mutex_;
  TrieNode trie_;
};
