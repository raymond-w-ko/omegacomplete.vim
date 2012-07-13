#include "stdafx.hpp"
#include "GlobalWordSet.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"

static const int kLevenshteinMaxCost = 2;
static const size_t kMinLengthForLevenshteinCompletion = 4;

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

    const std::map<String, WordInfo>& const_words = words_;
    if (Contains(const_words, word) == false) {
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

    StringVectorPtr title_cases = Algorithm::ComputeTitleCase(word);
    StringVectorPtr underscores = Algorithm::ComputeUnderscore(word);

    // generate and store abbreviations
    mutex_.lock();
    foreach (const String& title_case, *title_cases) {
        abbreviations_.insert(make_pair(title_case, word));
    }
    foreach (const String& underscore, *underscores) {
        abbreviations_.insert(make_pair(underscore, word));
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

    auto(bounds, abbreviations_.equal_range(prefix));
    auto(iter, bounds.first);
    for (; iter != bounds.second; ++iter) {
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
        // this actually can't be reliably used since it can cause
        // the removal of abbreviations that are still valid.
        // e.g.: if we have if we have 'foo_bar' and 'fizz_buzz', they
        // both map to 'fb' but if all traces of foo_bar disappear,
        // then it will remove 'fb' even though 'fizz_buzz' -> 'fb'
        // is still valid.
        // The proper way to fix this is to add reference counts to
        // these also, but having some dangling strings in memory should
        // still be okay
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

    for (TrieNode::ChildrenConstIterator iter = trie.Children.begin();
        iter != trie.Children.end();
        ++iter)
    {
        char letter = iter->first;
        TrieNode* next_node = iter->second;

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
        for (TrieNode::ChildrenConstIterator iter = node->Children.begin();
            iter != node->Children.end();
            ++iter)
        {
            char letter = iter->first;
            TrieNode* next_node = iter->second;

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
