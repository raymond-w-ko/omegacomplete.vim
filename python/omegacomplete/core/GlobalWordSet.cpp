#include "stdafx.hpp"
#include "GlobalWordSet.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "CompletionPriorities.hpp"

static const int kLevenshteinMaxCost = 2;
static const size_t kMinLengthForLevenshteinCompletion = 4;

GlobalWordSet::GlobalWordSet()
{
    ;
}

GlobalWordSet::~GlobalWordSet()
{
    ;
}

void GlobalWordSet::UpdateWord(const std::string& word, int reference_count_delta)
{
    boost::unique_lock<boost::mutex> lock(mutex_);

    WordInfo& wi = words_[word];
    wi.ReferenceCount += reference_count_delta;
    if (wi.ReferenceCount <= 0)
        return;

    trie_mutex_.lock();
    trie_.Insert(word);
    trie_mutex_.unlock();

    if (wi.GeneratedAbbreviations)
        return;

    UnsignedStringPairVectorPtr title_cases = Algorithm::ComputeTitleCase(word);
    UnsignedStringPairVectorPtr underscores = Algorithm::ComputeUnderscore(word);

    // generate and store abbreviations
    foreach (const UnsignedStringPair& title_case, *title_cases) {
        AbbreviationInfo ai(title_case.first, word);
        abbreviations_[title_case.second].insert(ai);
    }
    foreach (const UnsignedStringPair& underscore, *underscores) {
        AbbreviationInfo ai(underscore.first, word);
        abbreviations_[underscore.second].insert(ai);
    }

    wi.GeneratedAbbreviations = true;
}

void GlobalWordSet::GetPrefixCompletions(
    const std::string& input,
    CompleteItemVectorPtr& completions, std::set<std::string> added_words,
    bool terminus_mode)
{
    boost::unique_lock<boost::mutex> lock(mutex_);

    auto(iter, words_.lower_bound(input));
    for (; iter != words_.end(); ++iter) {
        const std::string& candidate = iter->first;
        const WordInfo& wi = iter->second;

        if (completions->size() == LookupTable::kMaxNumCompletions)
            break;
        if (boost::starts_with(candidate, input) == false)
            break;

        if (wi.ReferenceCount == 0)
            continue;
        if (terminus_mode && !boost::ends_with(candidate, "_"))
            continue;
        if (Contains(added_words, candidate))
            continue;

        CompleteItem completion(candidate);
        completion.Menu = boost::str(boost::format("        [%d Counts]")
            % wi.ReferenceCount);
        completions->push_back(completion);

        added_words.insert(candidate);
    }
}

void GlobalWordSet::GetAbbrCompletions(
    const std::string& input,
    CompleteItemVectorPtr& completions, std::set<std::string> added_words,
    bool terminus_mode)
{
    boost::unique_lock<boost::mutex> lock(mutex_);

    auto(const &set, abbreviations_[input]);
    auto(iter, set.begin());
    for (; iter != set.end(); ++iter) {
        const AbbreviationInfo& candidate = *iter;
        const WordInfo& wi = words_[candidate.Word];

        if (completions->size() == LookupTable::kMaxNumCompletions)
            break;

        if (wi.ReferenceCount == 0)
            continue;
        if (terminus_mode && !boost::ends_with(candidate.Word, "_"))
            continue;
        if (Contains(added_words, candidate.Word))
            continue;

        CompleteItem completion(candidate.Word, candidate.Weight);
        completion.Menu = boost::str(boost::format("        [%d Counts]")
            % wi.ReferenceCount);
        completions->push_back(completion);

        added_words.insert(candidate.Word);
    }
}

unsigned GlobalWordSet::Prune()
{
    boost::unique_lock<boost::mutex> lock(mutex_);

    // simply just looping through words_ and removing things might cause
    // iterator invalidation, so be safe and built a set of things to remove
    std::vector<std::string> to_be_pruned;
    auto(iter, words_.begin());
    for (; iter != words_.end(); ++iter)
    {
        if (iter->second.ReferenceCount > 0)
            continue;

        to_be_pruned.push_back(iter->first);
    }

    foreach (const std::string& word, to_be_pruned) {
        UnsignedStringPairVectorPtr collection;

        collection = Algorithm::ComputeTitleCase(word);
        foreach (UnsignedStringPair w, *collection) {
            std::set<AbbreviationInfo>& set = abbreviations_[w.second];

            auto (iter, set.begin());
            while (iter != set.end()) {
                if (iter->Word == word)
                    set.erase(iter++);
                else
                    ++iter;
            }

            if (abbreviations_[w.second].size() == 0)
                abbreviations_.erase(w.second);
        }

        collection = Algorithm::ComputeUnderscore(word);
        foreach (UnsignedStringPair w, *collection) {
            std::set<AbbreviationInfo>& set = abbreviations_[w.second];

            auto (iter, set.begin());
            while (iter != set.end()) {
                if (iter->Word == word)
                    set.erase(iter++);
                else
                    ++iter;
            }

            if (abbreviations_[w.second].size() == 0)
                abbreviations_.erase(w.second);
        }
        
        // we must erase the things that contains a const& to the word before
        // removing the actual word for safety reasons
        words_.erase(word);

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

    for (TrieNode::ChildrenConstIterator iter = trie.Children.begin();
        iter != trie.Children.end();
        ++iter)
    {
        char letter = iter->first;
        TrieNode* next_node = iter->second;

        GlobalWordSet::LevenshteinSearchRecursive(
            *next_node,
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
    const TrieNode& node,
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
         (node.Word.length() > 0) )
    {
        results[ current_row[last_index] ].insert(node.Word);
    }

    // if any entries in the row are less than the maximum cost, then
    // recursively search each branch of the trie
    if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost)
    {
        for (TrieNode::ChildrenConstIterator iter = node.Children.cbegin();
            iter != node.Children.cend();
            ++iter)
        {
            char letter = iter->first;
            TrieNode* next_node = iter->second;

            LevenshteinSearchRecursive(
                *next_node,
                letter,
                word,
                current_row,
                results,
                max_cost);
        }
    }
}
