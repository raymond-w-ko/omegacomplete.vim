#pragma once

struct TrieNode
{
    TrieNode();
    ~TrieNode();

    void Insert(const std::string& word);
    void Erase(const std::string& word);
    void Clear();

    std::string Word;
    //TrieNode* Parent;
    typedef std::map<char, TrieNode*>::value_type ChildrenIterator;
    std::map<char, TrieNode*> Children;
};
