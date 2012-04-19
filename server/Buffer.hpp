#pragma once

#include "KeywordTrie.hpp"

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

private:
    void tokenizeKeywords();
    void tokenizeKeywordsOfLine(const std::string& line);
	void tokenizeKeywordsUsingRegex();
    
    Session* parent_;
    boost::xpressive::sregex word_split_regex_;

    std::string buffer_id_;
    std::string pathname_;
    std::string contents_;
	
	std::string prev_cur_line_;
	std::pair<int, int> cursor_pos_;
	
    KeywordTrie words_;
	KeywordTrie current_line_words_;
    boost::unordered_set<std::string> already_processed_words_;
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
