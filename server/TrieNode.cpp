#include "stdafx.hpp"

#include "TrieNode.hpp"

TrieNode::TrieNode()
:
Word(nullptr)
{
    ;
}

TrieNode::~TrieNode()
{
    ;
}

void TrieNode::Insert(const std::string* word)
{
    TrieNode* node = this;
    for (char letter : *word)
    {
        auto& children = node->Children;
        if (children.find(letter) == children.end())
        {
            children.insert( make_pair(
                letter,
                make_unique<TrieNode>() ));
        }

        node = children[letter].get();
    }

    node->Word = word;
}

void TrieNode::Clear()
{
    Word = nullptr;
    Children.clear();
}

bool TrieNode::Empty()
{
    return Children.empty();
}
