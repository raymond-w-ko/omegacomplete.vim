#pragma once

struct TrieNode
{
    TrieNode();
    ~TrieNode();
	
	void Insert(const std::string& word);
	void Clear();
	bool Empty();

	std::string Word;
    boost::unordered_map<char, std::unique_ptr<TrieNode>> Children;
};
