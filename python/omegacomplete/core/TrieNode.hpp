#pragma once

struct TrieNode : public boost::noncopyable
{
    TrieNode();
    ~TrieNode();

    void Insert(const std::string& word);
    void Erase(const std::string& word);
    void Clear();

    String Word;
    typedef
        boost::unordered_map<char, TrieNode*>::iterator
        ChildrenIterator;
    typedef
        boost::unordered_map<char, TrieNode*>::const_iterator
        ChildrenConstIterator;
    boost::unordered_map<char, TrieNode*> Children;
};