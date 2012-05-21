#pragma once

struct TrieNode
{
    TrieNode();
    ~TrieNode();

    void Insert(const std::string* word);
    void Clear();
    bool Empty();

    const std::string* Word;
    typedef std::map<char, TrieNode*>::value_type ChildrenIterator;
    std::map<char, TrieNode*> Children;
};
