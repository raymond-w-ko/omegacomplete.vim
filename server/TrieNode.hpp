#pragma once

struct TrieNode
{
    TrieNode();
    ~TrieNode();

    void Insert(const std::string& word);
    void Erase(const std::string& word);
    void Clear();

    std::string Word;
    typedef
        boost::container::flat_map<char, TrieNode*>::value_type
        ChildrenIterator;
    boost::container::flat_map<char, TrieNode*> Children;
};
