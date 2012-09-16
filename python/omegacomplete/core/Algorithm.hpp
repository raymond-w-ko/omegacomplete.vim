#pragma once

#include "CompletionPriorities.hpp"

static const unsigned kWordSizeCutoffPointForDepthLists = 5;
static const unsigned kMaxDepthPerIndex = 3;

class Algorithm
{
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

    static const StringVector* ComputeUnderscoreCached(
        const std::string& word);
    static const StringVector* ComputeTitleCaseCached(
        const std::string& word);
    static void ClearGlobalCache();

    static UnsignedStringPairVectorPtr ComputeUnderscore(
        const std::string& word);
    static UnsignedStringPairVectorPtr ComputeHyphens(
        const std::string& word);
    static UnsignedStringPairVectorPtr ComputeTitleCase(
        const std::string& word);

    static bool StartsWith(const std::string& input, char c)
    {
        return input[0] == c;
    }
    static bool EndsWith(const std::string& input, char c)
    {
        return input[input.size() - 1] == c;
    }

private:
    template <char C, bool AllowedOnHeadTail>
    static UnsignedStringPairVectorPtr computeSeparator(
        const std::string& word)
    {
        UnsignedStringPairVectorPtr results =
            boost::make_shared<UnsignedStringPairVector>();

        const size_t word_size = word.size();

        // calculate indices of all the 'underscore' points,
        // which means the first letter, and the letters following an underscore
        std::vector<size_t> indices;
        for (size_t ii = 0; ii < word_size; ++ii) {
            if (ii == 0 && word[ii] != C) {
                indices.push_back(ii);
                continue;
            }

            if (word[ii] == C) {
                if ((ii < (word_size - 1)) && (word[ii + 1] != C))
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
                abbr += LookupTable::ToLower[word[index]];
            if (AllowedOnHeadTail ||
                (!AllowedOnHeadTail && (!StartsWith(abbr, C) ||
                                        !EndsWith(abbr, C))))
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
                        if (c == C)
                            break;
                        abbr += LookupTable::ToLower[c];
                    }
                }
                unsigned priority = abbr.size() <= indices.size() ?
                    kPrioritySinglesAbbreviation :
                    kPrioritySubsequenceAbbreviation;
                if (AllowedOnHeadTail ||
                    (!AllowedOnHeadTail && (!StartsWith(abbr, C) ||
                                            !EndsWith(abbr, C))))
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

    static boost::unordered_map<String, StringVector> underscore_cache_;
    static boost::mutex underscore_mutex_;

    static boost::unordered_map<String, StringVector> title_case_cache_;
    static boost::mutex title_case_mutex_;
};
