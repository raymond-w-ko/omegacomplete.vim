#pragma once

#include "KeywordTrie.hpp"

class Buffer
{
public:
    Buffer();
    ~Buffer();
    bool Init(std::string buffer_id);
    
    bool operator==(const Buffer& other);
    std::string GetBufferId() const;
    
    void SetPathname(std::string pathname);
    void SetContents(std::string contents);
    
    void Parse();

private:
    void tokenizeKeywords();

    std::string buffer_id_;
    std::string pathname_;
    std::string contents_;
    
    KeywordTrie words_;
    std::unordered_set<std::string> already_processed_words_;
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
