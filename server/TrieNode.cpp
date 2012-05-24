#include "stdafx.hpp"

#include "TrieNode.hpp"

TrieNode::TrieNode()
:
Parent(NULL)
{
}

TrieNode::~TrieNode()
{
    foreach (const ChildrenIterator& iter, Children) {
        delete iter.second;
    }
}

void TrieNode::Insert(const std::string& word)
{
    TrieNode* prev_node = NULL;
    TrieNode* node = this;
    foreach (char letter, word) {
        auto(&children, node->Children);
        if (Contains(children, letter) == false) {
            TrieNode* new_node = new TrieNode;
            children.insert(std::make_pair(letter, new_node));
            new_node->Parent = node;
        }

        prev_node = node;
        node = children[letter];
    }

    node->Word = word;
}

void TrieNode::Erase(const std::string& word)
{
    TrieNode* prev_node = NULL;
    TrieNode* node = this;
    foreach (char letter, word)
    {
        auto(&children, node->Children);
        if (Contains(children, letter) == false) {
            return;
        }

        prev_node = node;
        node = children[letter];
    }

    node->Word.clear();

    if (node->Children.size() == 0) {
        char index = word[word.size() - 1];
        prev_node->Children.erase(index);
    }
}

void TrieNode::Clear()
{
    Word = "";
    Children.clear();
}

