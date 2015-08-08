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
      Omegacomplete::Completions* completions,
      Omegacomplete::DoneStatus* done_status,
      const std::vector<String>* word_list,
      const int begin,
      const int end,
      const std::string& input,
      const bool terminus_mode);
  static float GetWordScore(
      const std::string& word, const std::string& input,
      const bool terminus_mode);

  // assumes boundaries is already an empty string
  static void GetWordBoundaries(const std::string& word,
                                std::string& boundaries);
  static bool IsSubsequence(const std::string& haystack,
                            const std::string& needle);
  static float GetClumpingScore(const std::string& word,
                                const std::string& input);
};
