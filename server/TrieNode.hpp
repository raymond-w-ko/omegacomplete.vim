#pragma once

struct TrieNode
{
    TrieNode();
    ~TrieNode();

    void Insert(const std::string* word);
    void Clear();
    bool Empty();

    const std::string* Word;
    std::map<char, std::unique_ptr<TrieNode>> Children;
};
