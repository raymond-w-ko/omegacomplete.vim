#pragma once

#include "CompletionPriorities.hpp"
#include "Omegacomplete.hpp"

class Algorithm {
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

  static void ProcessWords(
      Omegacomplete::Completions& completions,
      boost::shared_ptr<boost::mutex> mutex,
      const boost::unordered_map<int, String>& word_list,
      int begin,
      int end,
      const std::string& input,
      bool terminus_mode);
  static float GetWordScore(
      const std::string& word, const std::string& input,
      bool terminus_mode);

  static float TitleCaseMatchScore(const std::string& word, const std::string& input);
  static float SeparatorMatchScore(
      const std::string& word, const std::string& input, const uchar separator);
  static float UnderScoreMatchScore(const std::string& word, const std::string& input);
  static float HyphenMatchScore(const std::string& word, const std::string& input);

  static bool IsSubsequence(const std::string& word, const std::string& input);
};
