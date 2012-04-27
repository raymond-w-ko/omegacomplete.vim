#pragma once

#include "TrieNode.hpp"

class Session;

typedef std::map< int, std::set<std::string> > LevenshteinSearchResults;

class Buffer
{
public:

    Buffer();
    ~Buffer();
    bool Init(Session* parent, std::string buffer_id);

    bool operator==(const Buffer& other);
    std::string GetBufferId() const;

    void SetPathname(std::string pathname);

    void ParseNormalMode(
        const std::string& new_contents);
    void ParseInsertMode(
        const std::string& new_contents,
        const std::string& cur_line,
        std::pair<int, int> cursor_pos);
    void GetAllWordsWithPrefixFromCurrentLine(
        const std::string& prefix,
        std::set<std::string>* results);
    void GetAllWordsWithPrefix(
        const std::string& prefix,
        std::set<std::string>* results);

    void GetLevenshteinCompletions(
        const std::string& prefix,
        LevenshteinSearchResults& results);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<std::string>* results);

private:
    void tokenizeKeywords();

    void tokenizeKeywordsHelper(
        const std::string& content,
        boost::unordered_set<std::string>& container);
    void tokenizeKeywordsOfCurrentLine(const std::string& line);
    void tokenizeKeywordsOfOriginalCurrentLine(const std::string& line);
    void calculateCurrentWordOfCursor(
        const std::string& line,
        const std::pair<int, int> pos);

    void generateTitleCasesAndUnderscores();

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

    Session* parent_;

    std::string buffer_id_;
    std::string pathname_;
    std::string contents_;

    std::pair<int, int> cursor_pos_;
    std::string initial_current_line_;
    std::string prev_cur_line_;
    std::string current_cursor_word_;

    char is_part_of_word_[256];
    char to_lower_[256];
    //boost::unordered_set<std::string>* words_;
    boost::unordered_map<std::string, unsigned>* words_;
    boost::unordered_set<std::string> current_line_words_;
    boost::unordered_set<std::string> orig_cur_line_words_;
    TrieNode* trie_;

    bool abbreviations_dirty_;
    boost::unordered_multimap<std::string, const std::string*>* title_cases_;
    boost::unordered_multimap<std::string, const std::string*>* underscores_;
    boost::unordered_map<std::string, std::string> title_case_cache_;
    boost::unordered_map<std::string, std::string> underscore_cache_;
};

namespace std
{
template<> struct hash<Buffer>
{
    size_t operator()(const Buffer& buffer) const
    {
        return std::hash<std::string>()(buffer.GetBufferId());
    }
};
}
