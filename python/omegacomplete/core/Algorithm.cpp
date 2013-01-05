#include "stdafx.hpp"

#include "LookupTable.hpp"
#include "CompletionPriorities.hpp"
#include "Algorithm.hpp"

boost::unordered_map<size_t, std::vector<std::vector<size_t> > >
    Algorithm::depth_list_cache_;

void Algorithm::InitStatic()
{
    // generate depth lists
    for (unsigned ii = 1; ii < kWordSizeCutoffPointForDepthLists; ++ii) {
        Algorithm::GenerateDepths(ii, kMaxDepthPerIndex, depth_list_cache_[ii]);

        //foreach (std::vector<size_t> line, depth_list_cache_[ii]) {
            //std::cout << ii << ": ";
            //reverse_foreach (size_t index, line)
                //std::cout << index;
            //std::cout << "\n";
        //}
        //std::cout << "\n";
    }
}

void Algorithm::GenerateDepths(
    const unsigned num_indices,
    const unsigned max_depth,
    std::vector<std::vector<size_t> >& depth_list)
{
    const unsigned num_possible = static_cast<unsigned>(::pow(
        (double)max_depth, (int)num_indices));

    std::vector<size_t> indices;
    for (size_t ii = 0; ii < num_indices; ++ii)
        indices.push_back(1);

    for (size_t ii = 0; ii < num_possible; ++ii) {
        //reverse_foreach (size_t index, indices) std::cout << index;
        //std::cout << "\n";

        depth_list.push_back(indices);

        indices[0]++;
        Algorithm::ResolveCarries(indices, max_depth);
    }
}

void Algorithm::ResolveCarries(
    std::vector<size_t>& indices,
    size_t digit_upper_bound)
{
    digit_upper_bound++;

    for (size_t ii = 0; ii < (indices.size() - 1); ++ii) {
        size_t potential_carry = indices[ii] / digit_upper_bound;
        if (potential_carry == 0)
            break;
        indices[ii + 1] += potential_carry;
        indices[ii] = (indices[ii] % digit_upper_bound) + 1;
    }
}

UnsignedStringPairVectorPtr Algorithm::ComputeUnderscore(
    const std::string& word)
{
    return computeSeparator<'_', true>(word);
}

UnsignedStringPairVectorPtr Algorithm::ComputeHyphens(
    const std::string& word)
{
    return computeSeparator<'-', false>(word);
}

UnsignedStringPairVectorPtr Algorithm::ComputeTitleCase(
    const std::string& word)
{
    UnsignedStringPairVectorPtr results =
        boost::make_shared<UnsignedStringPairVector>();

    const size_t word_size = word.size();

    // calculate indices of all the 'TitleCase' points
    std::vector<size_t> indices;
    for (size_t ii = 0; ii < word_size; ++ii) {
        uchar c = static_cast<uchar>(word[ii]);

        if (ii == 0) {
            indices.push_back(ii);
            continue;
        }

        if (LookupTable::IsUpper[c]) {
            indices.push_back(ii);
        }
    }
    const size_t indices_size = indices.size();

    // if we only have 1 index, then we don't have enough for a TitleCase
    // abbreviation
    if (indices_size <= 1)
        return results;

    // if the number of indices is >= the cutoff point, then just grab the
    // letter at each index
    if (indices_size >= kWordSizeCutoffPointForDepthLists) {
        std::string abbr;
        abbr.reserve(kWordSizeCutoffPointForDepthLists);
        foreach (size_t index, indices)
            abbr += LookupTable::ToLower[static_cast<uchar>(word[index])];
        results->push_back(std::make_pair(kPrioritySinglesAbbreviation, abbr));
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
                    uchar c = static_cast<uchar>(word[index + cur_depth]);
                    abbr += LookupTable::ToLower[c];
                }
            }
            unsigned priority = abbr.size() <= indices.size() ?
                kPrioritySinglesAbbreviation : kPrioritySubsequenceAbbreviation;
            results->push_back(std::make_pair(priority, abbr));
        }
    }

    return results;
}
