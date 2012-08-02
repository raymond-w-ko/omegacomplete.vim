#pragma once

#include "TrieNode.hpp"
#include "CompleteItem.hpp"

typedef std::map< int, std::set<std::string> > LevenshteinSearchResults;

struct WordInfo
{
    WordInfo();
    ~WordInfo();

    int ReferenceCount;
    bool GeneratedAbbreviations;
};

class GlobalWordSet : public boost::noncopyable
{
public:
    static void LevenshteinSearch(
        const std::string& word,
        int max_cost,
        const TrieNode& trie,
        LevenshteinSearchResults& results);

    static void LevenshteinSearchRecursive(
        const TrieNode& node,
        char letter,
        const std::string& word,
        const std::vector<int>& previous_row,
        LevenshteinSearchResults& results,
        int max_cost);

    GlobalWordSet();
    ~GlobalWordSet();

    void UpdateWord(const std::string& word, int reference_count_delta);
    unsigned Prune();

    void GetPrefixCompletions(
        const std::string& prefix,
        std::set<CompleteItem>* completions);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<CompleteItem>* completions);

    void GetLevenshteinCompletions(
        const std::string& prefix,
        LevenshteinSearchResults& results);

private:
    boost::mutex mutex_;

    std::map<String, WordInfo> words_;
    std::multimap<String, String> abbreviations_;

    boost::mutex trie_mutex_;
    TrieNode trie_;
};
