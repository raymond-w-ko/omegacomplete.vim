#pragma once

#include "TrieNode.hpp"

class Session;

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
		std::vector<std::string>* results);
	void GetAllWordsWithPrefix(
		const std::string& prefix,
		std::vector<std::string>* results);

	void GetLevenshteinCompletions(
		const std::string& prefix,
		std::vector<std::string>* results);

private:
    void tokenizeKeywords();
    void tokenizeKeywordsOfLine(const std::string& line);
	void tokenizeKeywordsUsingRegex();

	typedef std::vector< std::pair<std::string, int> > LevenshteinSearchResults;
	LevenshteinSearchResults levenshteinSearch(
		const std::string& word,
		int max_cost);

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
	
	TrieNode trie_;
	boost::unordered_set<std::string> words_;
	std::set<std::string> current_line_words_;
	
	char is_part_of_word_[256];
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
