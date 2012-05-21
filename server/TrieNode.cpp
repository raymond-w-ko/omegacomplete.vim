#include "stdafx.hpp"

#include "TrieNode.hpp"

TrieNode::TrieNode()
:
Word(NULL)
{
    ;
}

TrieNode::~TrieNode()
{
    foreach (ChildrenIterator& iter, Children) {
        delete iter.second;
    }
}

void TrieNode::Insert(const std::string* word)
{
    TrieNode* node = this;
    foreach (char letter, *word)
    {
        auto(&children, node->Children);
        if (Contains(children, letter) == false)
        {
            TrieNode* new_node = new TrieNode;
            children.insert(std::make_pair(letter, new_node));
        }

        node = children[letter];
    }

    node->Word = word;
}

void TrieNode::Clear()
{
    Word = NULL;
    Children.clear();
}

bool TrieNode::Empty()
{
    return Children.empty();
}
