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
		if (node->Children.find(letter) == node->Children.end())
		{
			node->Children.insert( make_pair(
				letter,
				make_unique<TrieNode>() ));
		}

		node = node->Children[letter].get();
	}

	node->Word = word;
}

void TrieNode::Clear()
{
	Word = "";
	Children.clear();
}

bool TrieNode::Empty()
{
	return Children.empty();
}
