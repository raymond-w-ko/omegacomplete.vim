#include "stdafx.hpp"
#include "GlobalWordSet.hpp"

const int kLevenshteinMaxCost = 2;
const size_t kMinLengthForLevenshteinCompletion = 4;
const size_t kWordSizeCutoffPointForDepthLists = 5;
const size_t kMaxDepthPerIndex = 3;

char GlobalWordSet::is_part_of_word_[256];
char GlobalWordSet::to_lower_[256];
StringToStringVectorUnorderedMap GlobalWordSet::title_case_cache_;
StringToStringVectorUnorderedMap GlobalWordSet::underscore_cache_;
boost::unordered_map<size_t, std::vector<std::vector<size_t> > >
    GlobalWordSet::depth_list_cache_;

void GlobalWordSet::GlobalInit()
{
    // generate lookup tables
    // for determining what is part of of a 'word'
    // for determining what the lowercase equivalent of a letter is
    std::string temp(1, ' ');
    for (size_t index = 0; index <= 255; ++index)
    {
        is_part_of_word_[index] = IsPartOfWord(index) ? 1 : 0;
        temp.resize(1, ' ');
        temp[0] = static_cast<char>(index);
        boost::algorithm::to_lower(temp);
        to_lower_[index] = temp[0];
    }

    // generate depth lists
    for (size_t ii = 1; ii < kWordSizeCutoffPointForDepthLists; ++ii) {
        GlobalWordSet::GenerateDepths(ii, kMaxDepthPerIndex, depth_list_cache_[ii]);

        //foreach (std::vector<size_t> line, depth_list_cache_[ii]) {
            //std::cout << ii << ": ";
            //reverse_foreach (size_t index, line)
                //std::cout << index;
            //std::cout << "\n";
        //}
        //std::cout << "\n";
    }
}

void GlobalWordSet::ResolveCarries(
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

void GlobalWordSet::GenerateDepths(
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
        GlobalWordSet::ResolveCarries(indices, max_depth);
    }
}

const StringVector* GlobalWordSet::ComputeUnderscore(
    const std::string& word,
    StringToStringVectorUnorderedMap& underscore_cache)
{
    // we have already computed this before
    if (Contains(underscore_cache, word) == true) {
        return &underscore_cache[word];
    }

    const size_t word_size = word.size();

    // calculate indices of all the 'underscore' points,
    // which means the first letter, and the letters following an underscore
    std::vector<size_t> indices;
    for (size_t ii = 0; ii < word_size; ++ii) {
        if (ii == 0 && word[ii] != '_') {
            indices.push_back(ii);
            continue;
        }

        if (word[ii] == '_') {
            if ((ii < (word_size - 1)) && (word[ii + 1] != '_'))
                indices.push_back(ii + 1);
        }
    }
    const size_t indices_size = indices.size();

    // if we only have 1 index, then we don't have enought for a underscore
    // abbreviation, create and return empty vector reference
    if (indices_size <= 1)
        return &underscore_cache[word];

    // if the number of indices is >= the cutoff point, then just grab the
    // letter at each index
    if (indices_size >= kWordSizeCutoffPointForDepthLists) {
        std::string abbr;
        abbr.reserve(kWordSizeCutoffPointForDepthLists);
        foreach (size_t index, indices)
            abbr += to_lower_[word[index]];
        underscore_cache[word].push_back(abbr);
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
                    if (c == '_')
                        break;
                    abbr += to_lower_[c];
                }
            }
            underscore_cache[word].push_back(abbr);
        }
    }

    return &underscore_cache[word];
}

const StringVector* GlobalWordSet::ComputeTitleCase(
    const std::string& word,
    StringToStringVectorUnorderedMap& title_case_cache)
{
    // we have already computed this before
    if (Contains(title_case_cache, word) == true) {
        return &title_case_cache[word];
    }

    const size_t word_size = word.size();

    // calculate indices of all the 'TitleCase' points
    std::vector<size_t> indices;
    for (size_t ii = 0; ii < word_size; ++ii) {
        char c = word[ii];

        if (ii == 0) {
            indices.push_back(ii);
            continue;
        }

        if (IsUpper(c)) {
            indices.push_back(ii);
        }
    }
    const size_t indices_size = indices.size();

    // if we only have 1 index, then we don't have enought for a TitleCase
    // abbreviation
    if (indices_size <= 1)
        return &title_case_cache[word];

    for (size_t depth = 1; depth < (2 + 1); ++depth) {
        std::string abbr;
        for (size_t ii = 0; ii < indices.size(); ++ii) {
            size_t index = indices[ii];
            size_t next_index =
                (ii + 1) < indices_size ? indices[ii + 1] : word.size();
            for (size_t cur_depth = 0; cur_depth < depth; ++cur_depth) {
                if ((index + cur_depth) >= next_index)
                    break;
                abbr += to_lower_[word[index + cur_depth]];
            }
        }
        title_case_cache[word].push_back(abbr);
    }

    // if the number of indices is >= the cutoff point, then just grab the
    // letter at each index
    if (indices_size >= kWordSizeCutoffPointForDepthLists) {
        std::string abbr;
        abbr.reserve(kWordSizeCutoffPointForDepthLists);
        foreach (size_t index, indices)
            abbr += to_lower_[word[index]];
        title_case_cache_[word].push_back(abbr);
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
                    abbr += to_lower_[c];
                }
            }
            title_case_cache_[word].push_back(abbr);
        }
    }

    return &title_case_cache[word];
}

WordInfo::WordInfo()
:
ReferenceCount(0),
GeneratedAbbreviations(false)
{
    ;
}

WordInfo::~WordInfo()
{
    ;
}

GlobalWordSet::GlobalWordSet()
:
const_words_(words_)
{
    ;
}

GlobalWordSet::~GlobalWordSet()
{
    ;
}

void GlobalWordSet::UpdateWord(const std::string& word, int reference_count_delta)
{
    // the following below, while not being totally safe and correct, this done
    // this minimize any potential latency.
    bool lock_required;

    // if we can find the WordInfo struct that means we don't need a lock since
    // we are doing a read only operation on words_ to find the location of the
    // target WordInfo. If we can't find it, accessing the reference to it means
    // creation, which requires a lock since the data structure internally
    // will change.
    if (const_words_.find(word) != const_words_.end()) {
        lock_required = true;
    } else {
        lock_required = false;
    }

    if (lock_required) mutex_.lock();
    WordInfo& wi = words_[word];
    if (lock_required) mutex_.unlock();

    // this is not locked, which means that reads can be out of date, but
    // I don't care since I want performance.
    wi.ReferenceCount += reference_count_delta;

    if (wi.ReferenceCount <= 0) return;

    trie_mutex_.lock();
    trie_.Insert(word);
    trie_mutex_.unlock();

    if (wi.GeneratedAbbreviations) return;

    const StringVector* title_cases = ComputeTitleCase(word, title_case_cache_);
    const StringVector* underscores = ComputeUnderscore(word, underscore_cache_);

    // generate and store abbreviations
    mutex_.lock();
    if (title_cases->size() > 0) {
        foreach (const std::string& title_case, *title_cases) {
            title_cases_.insert(make_pair(title_case, word));
        }
    }
    if (underscores->size() > 0) {
        foreach (const std::string& underscore, *underscores) {
            underscores_.insert(make_pair(underscore, word));
        }
    }
    mutex_.unlock();

    wi.GeneratedAbbreviations = true;
}

void GlobalWordSet::GetPrefixCompletions(
    const std::string& prefix,
    std::set<std::string>* completions)
{
    mutex_.lock();

    auto(iter, words_.lower_bound(prefix));
    for (; iter != words_.end(); ++iter) {
        const std::string& candidate = iter->first;
        const WordInfo& wi = iter->second;

        if (boost::starts_with(candidate, prefix) == false) break;
        if (wi.ReferenceCount == 0) continue;

        completions->insert(candidate);
    }

    mutex_.unlock();
}

void GlobalWordSet::GetAbbrCompletions(
    const std::string& prefix,
    std::set<std::string>* completions)
{
    mutex_.lock();

    auto(bounds1, title_cases_.equal_range(prefix));
    auto(iter, bounds1.first);
    for (iter = bounds1.first; iter != bounds1.second; ++iter) {
        const std::string& candidate = iter->second;
        if (words_[candidate].ReferenceCount == 0) continue;
        completions->insert(candidate);
    }

    auto(bounds2, underscores_.equal_range(prefix));
    for (iter = bounds2.first; iter != bounds2.second; ++iter) {
        const std::string& candidate = iter->second;
        if (words_[candidate].ReferenceCount == 0) continue;
        completions->insert(candidate);
    }

    mutex_.unlock();
}

unsigned GlobalWordSet::Prune()
{
    mutex_.lock();

    std::vector<std::string> to_be_pruned;
    auto(iter, words_.begin());
    for (; iter != words_.end(); ++iter) {
        if (iter->second.ReferenceCount > 0) continue;

        to_be_pruned.push_back(iter->first);
    }

    mutex_.unlock();

    foreach (const std::string& word, to_be_pruned) {
        mutex_.lock();
        words_.erase(word);
        // this actually can't be reliably used since it can caused
        // the removal of abbreviations that are still valid.
        // e.g.: if we have if we have 'foo_bar' and 'fizz_buzz', they
        // both map to 'fb' but if all traces of foo_bar disappear,
        // then it will remove 'fb' even though 'fizz_buzz' -> 'fb'
        // is still valid.
        // The proper way to fix this is to add reference counts to
        // these also, but it might not be necessary.
        /*
        foreach (const std::string& w, *ComputeTitleCase(word)) {
            title_cases_.erase(w);
        }
        foreach (const std::string& w, *ComputeUnderscore(word)) {
            underscores_.erase(w);
        }
        */
        mutex_.unlock();

        trie_mutex_.lock();
        trie_.Erase(word);
        trie_mutex_.unlock();
    }

    return to_be_pruned.size();
}

void GlobalWordSet::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results)
{
    // only try correction if we have a sufficiently long enough
    // word to make it worthwhile
    if (prefix.length() < kMinLengthForLevenshteinCompletion) return;

    trie_mutex_.lock();
    LevenshteinSearch(prefix, kLevenshteinMaxCost, trie_, results);
    trie_mutex_.unlock();
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void GlobalWordSet::LevenshteinSearch(
    const std::string& word,
    const int max_cost,
    const TrieNode& trie,
    LevenshteinSearchResults& results)
{
    // generate sequence from [0, len(word)]
    size_t current_row_end = word.length() + 1;
    std::vector<int> current_row;
    for (size_t ii = 0; ii < current_row_end; ++ii) current_row.push_back(ii);

    foreach (const TrieNode::ChildrenIterator& iter, trie.Children) {
        char letter = iter.first;
        TrieNode* next_node = iter.second;

        GlobalWordSet::LevenshteinSearchRecursive(
            next_node,
            letter,
            word,
            current_row,
            results,
            max_cost);
    }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void GlobalWordSet::LevenshteinSearchRecursive(
    TrieNode* node,
    char letter,
    const std::string& word,
    const std::vector<int>& previous_row,
    LevenshteinSearchResults& results,
    int max_cost)
{
    size_t columns = word.length() + 1;
    std::vector<int> current_row;
    current_row.push_back( previous_row[0] + 1 );

    // Build one row for the letter, with a column for each letter in the target
    // word, plus one for the empty string at column 0
    for (unsigned column = 1; column < columns; ++column)
    {
        int insert_cost = current_row[column - 1] + 1;
        int delete_cost = previous_row[column] + 1;

        int replace_cost;
        if (word[column - 1] != letter)
            replace_cost = previous_row[column - 1] + 1;
        else
            replace_cost = previous_row[column - 1];

        current_row.push_back( (std::min)(
            insert_cost,
            (std::min)(delete_cost, replace_cost)) );
    }

    // if the last entry in the row indicates the optimal cost is less than the
    // maximum cost, and there is a word in this trie node, then add it.
    size_t last_index = current_row.size() - 1;
    if ( (current_row[last_index] <= max_cost) &&
         (node->Word.length() > 0) )
    {
        results[ current_row[last_index] ].insert(node->Word);
    }

    // if any entries in the row are less than the maximum cost, then
    // recursively search each branch of the trie
    if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost)
    {
        foreach (const TrieNode::ChildrenIterator& iter, node->Children) {
            char letter = iter.first;
            TrieNode* next_node = iter.second;
            LevenshteinSearchRecursive(
                next_node,
                letter,
                word,
                current_row,
                results,
                max_cost);
        }
    }
}
