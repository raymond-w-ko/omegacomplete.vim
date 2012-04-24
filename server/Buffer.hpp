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

private:
    void tokenizeKeywords();
    void tokenizeKeywordsOfLine(const std::string& line);
	void tokenizeKeywordsUsingRegex();

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
    //boost::xpressive::sregex word_split_regex_;

    std::string buffer_id_;
    std::string pathname_;
    std::string contents_;
	
	std::string prev_cur_line_;
	std::pair<int, int> cursor_pos_;
	
	char is_part_of_word_[256];
	boost::unordered_set<std::string>* words_;
	std::set<std::string> current_line_words_;
	TrieNode* trie_;
	boost::unordered_map<std::string, std::string> title_cases_;
	boost::unordered_map<std::string, std::string> underscores_;
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
