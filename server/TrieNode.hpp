#pragma once

struct TrieNode : public boost::noncopyable
{
    TrieNode();
    ~TrieNode();

    void Insert(const std::string& word);
    void Erase(const std::string& word);
    void Clear();

    std::string Word;
    typedef
        boost::unordered_map<char, TrieNode*>::value_type
        ChildrenIterator;
    boost::unordered_map<char, TrieNode*> Children;
};
