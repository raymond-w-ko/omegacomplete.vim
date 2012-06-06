#pragma once

#include "TrieNode.hpp"

typedef std::map< int, std::set<std::string> > LevenshteinSearchResults;

struct WordInfo
{
    WordInfo();
    ~WordInfo();

    int ReferenceCount;
    bool GeneratedAbbreviations;
};

class GlobalWordSet
{
public:

    static void GlobalInit();

    static char is_part_of_word_[256];
    static char to_lower_[256];
    // digit_upper_bound is of the form [1, digit_upper_bound]
    // no side effects
    static void ResolveCarries(
        std::vector<size_t>& indices,
        size_t digit_upper_bound);
    // no side effects
    static void GenerateDepths(
        const unsigned num_indices,
        const unsigned depth,
        std::vector<std::vector<size_t> >& depth_list);

    static const StringVector* ComputeUnderscore(
        const std::string& word,
        StringToStringVectorUnorderedMap& underscore_cache);
    static const StringVector* ComputeTitleCase(
        const std::string& word,
        StringToStringVectorUnorderedMap& title_case_cache);

    GlobalWordSet();
    ~GlobalWordSet();

    void UpdateWord(const std::string& word, int reference_count_delta);
    unsigned Prune();

    void GetPrefixCompletions(
        const std::string& prefix,
        std::set<std::string>* completions);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<std::string>* completions);

    void GetLevenshteinCompletions(
        const std::string& prefix,
        LevenshteinSearchResults& results);

private:
    void levenshteinSearch(
        const std::string& word,
        int max_cost,
        LevenshteinSearchResults& results);

    void levenshteinSearchRecursive(
        TrieNode* node,
        char letter,
        const std::string& word,
        const std::vector<int>& previous_row,
        LevenshteinSearchResults& results,
        int max_cost);

    static StringToStringVectorUnorderedMap title_case_cache_;
    static StringToStringVectorUnorderedMap underscore_cache_;

    static
        boost::unordered_map<size_t, std::vector<std::vector<size_t> > >
        depth_list_cache_;

    boost::mutex mutex_;

    std::map<std::string, WordInfo> words_;
    std::map<std::string, WordInfo>& const_words_;
    boost::unordered_multimap<std::string, std::string> title_cases_;
    boost::unordered_multimap<std::string, std::string> underscores_;

    boost::mutex trie_mutex_;
    TrieNode trie_;
};
