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
    
    void Parse(const std::string& new_contents, bool force);
	void GetAllWordsWithPrefix(
		const std::string& prefix,
		std::vector<std::string>* results);

private:
    void tokenizeKeywords();
	void tokenizeKeywordsUsingRegex();
    
    static std::vector<std::string> fuzzyMatch( 
        std::string* iter_begin,
        std::string* iter_end);
    
    Session* parent_;
    boost::xpressive::sregex word_split_regex_;

    std::string buffer_id_;
    std::string pathname_;
    std::string contents_;
    
    KeywordTrie words_;
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
