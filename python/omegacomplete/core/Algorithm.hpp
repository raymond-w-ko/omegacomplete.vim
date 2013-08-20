#pragma once

#include "CompletionPriorities.hpp"
#include "Omegacomplete.hpp"

static const unsigned kWordSizeCutoffPointForDepthLists = 5;
static const unsigned kMaxDepthPerIndex = 3;

class Algorithm {
 public:
  static void InitStatic();

  // no side effects
  static void GenerateDepths(
      const unsigned num_indices,
      const unsigned depth,
      std::vector<std::vector<size_t> >& depth_list);
  // digit_upper_bound is of the form [1, digit_upper_bound]
  // no side effects
  static void ResolveCarries(
      std::vector<size_t>& indices,
      size_t digit_upper_bound);

  static UnsignedStringPairVectorPtr ComputeUnderscore(
      const std::string& word);
  static UnsignedStringPairVectorPtr ComputeHyphens(
      const std::string& word);
  static UnsignedStringPairVectorPtr ComputeTitleCase(
      const std::string& word);

  static bool StartsWith(const std::string& input, char c) {
    return input[0] == c;
  }
  static bool EndsWith(const std::string& input, char c) {
    return input[input.size() - 1] == c;
  }

  static void ProcessWords(
      Omegacomplete::Completions& completions,
      boost::shared_ptr<boost::mutex> mutex,
      const boost::unordered_map<int, String>& word_list,
      int begin,
      int end,
      const std::string& input);
  static int GetWordScore(const std::string& word, const std::string& input);
  static bool IsTitleCaseMatch(const std::string& word, const std::string& input);
  static bool IsSeparatorMatch(
      const std::string& word, const std::string& input, const uchar separator);
  static bool IsUnderScoreMatch(const std::string& word, const std::string& input);
  static bool IsHyphenMatch(const std::string& word, const std::string& input);

 private:
  template <char Separator, bool AllowedOnHeadOrTail>
  static UnsignedStringPairVectorPtr computeSeparator(
      const std::string& word) {
    UnsignedStringPairVectorPtr results =
        boost::make_shared<UnsignedStringPairVector>();

    const size_t word_size = word.size();

    // calculate indices of all the 'underscore' points, which means the
    // first letter, and the letters following an underscore
    std::vector<size_t> indices;
    for (size_t ii = 0; ii < word_size; ++ii) {
      if (ii == 0 && word[ii] != Separator) {
        indices.push_back(ii);
        continue;
      }

      if (word[ii] == Separator) {
        if ((ii < (word_size - 1)) && (word[ii + 1] != Separator))
          indices.push_back(ii + 1);
      }
    }
    const size_t indices_size = indices.size();

    // if we only have 1 index, then we don't have enough for a underscore
    // abbreviation, return an empty vector
    if (indices_size <= 1)
      return results;

    // if the number of indices is >= the cutoff point, then just grab the
    // letter at each index
    if (indices_size >= kWordSizeCutoffPointForDepthLists) {
      std::string abbr;
      abbr.reserve(kWordSizeCutoffPointForDepthLists);
      foreach (size_t index, indices)
          abbr += LookupTable::ToLower[static_cast<uchar>(word[index])];
      if (AllowedOnHeadOrTail ||
          (!AllowedOnHeadOrTail && (!StartsWith(abbr, Separator) ||
                                    !EndsWith(abbr, Separator))))
      {
        results->push_back(std::make_pair(kPrioritySinglesAbbreviation,
                                          abbr));
      }
    }
    // generate various abbreviations based on depth permutations
    else {
      const std::vector<std::vector<size_t> >& depth_list =
          depth_list_cache_[indices_size];
      foreach (const std::vector<size_t>& depths, depth_list) {
        std::string abbr;
        for (size_t ii = 0; ii < indices_size; ++ii) {
          const size_t index = indices[ii];
          const size_t next_index =
              (ii + 1) < indices_size ? indices[ii + 1] : word.size();
          const size_t depth = depths[ii];
          for (size_t cur_depth = 0; cur_depth < depth; ++cur_depth) {
            if ((index + cur_depth) >= next_index)
              break;
            char c = word[index + cur_depth];
            if (c == Separator)
              break;
            abbr += LookupTable::ToLower[static_cast<uchar>(c)];
          }
        }
        unsigned priority = abbr.size() <= indices.size() ?
            kPrioritySinglesAbbreviation :
            kPrioritySubsequenceAbbreviation;
        if (AllowedOnHeadOrTail ||
            (!AllowedOnHeadOrTail && (!StartsWith(abbr, Separator) ||
                                      !EndsWith(abbr, Separator))))
        {
          results->push_back(std::make_pair(priority, abbr));
        }
      }
    }

    return results;
  }

  static
      boost::unordered_map<size_t, std::vector<std::vector<size_t> > >
      depth_list_cache_;
};
