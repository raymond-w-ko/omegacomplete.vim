#pragma once

#include "GlobalWordSet.hpp"
#include "CompleteItem.hpp"

struct CompletionSet
{
public:
    CompletionSet();

    std::set<CompleteItem> AbbrCompletions;
    std::set<CompleteItem> TagsAbbrCompletions;
    std::set<CompleteItem> PrefixCompletions;
    std::set<CompleteItem> TagsPrefixCompletions;

    unsigned int GetNumCompletions() const;

    void Clear();

    void FillResults(std::vector<CompleteItem>& result_list);
    void FillLevenshteinResults(
        const LevenshteinSearchResults& levenshtein_completions,
        const std::string& prefix_to_complete,
        std::string& result);

    void AddBannedWord(const std::string& word);

    static const unsigned kMaxNumCompletions;

private:
    void addCompletionsToResults(
        const std::set<CompleteItem>& completions,
        std::vector<CompleteItem>& result_list);

    unsigned num_completions_added_;
    boost::unordered_set<std::string> added_words_;
};

