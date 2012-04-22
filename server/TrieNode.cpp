#include "stdafx.hpp"

#include "TrieNode.hpp"

TrieNode::TrieNode()
{
    ;
}

TrieNode::~TrieNode()
{
    ;
}

void TrieNode::Insert(const std::string& word)
{
	TrieNode* node = this;
	for (char letter : word)
	{
		if (Children.find(letter) == Children.end())
		{
			node->Children.emplace(
				letter,
				std::unique_ptr<TrieNode>(new TrieNode()));
		}

		node = node->Children[letter].get();
	}
	
	node->Word = word;
}
