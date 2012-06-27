#pragma once

#include "GlobalWordSet.hpp"

struct CompletionSet
{
public:
    CompletionSet();

    std::set<std::string> AbbrCompletions;
    std::set<std::string> TagsAbbrCompletions;
    std::set<std::string> PrefixCompletions;
    std::set<std::string> TagsPrefixCompletions;

    unsigned int GetNumCompletions() const;

    void Clear();

    void FillResults(std::vector<StringPair>& result_list);
    void FillLevenshteinResults(
        const LevenshteinSearchResults& levenshtein_completions,
        const std::string& prefix_to_complete,
        std::string& result);

    void AddBannedWord(const std::string& word);

    static const unsigned kMaxNumCompletions;

private:
    void addWordsToResults(
        const std::set<std::string>& words,
        std::vector<StringPair>& result_list);

    unsigned num_completions_added_;
    boost::unordered_set<std::string> added_words_;
};

