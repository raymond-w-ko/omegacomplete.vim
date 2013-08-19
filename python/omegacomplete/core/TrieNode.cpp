#include "stdafx.hpp"

#include "TrieNode.hpp"

TrieNode::TrieNode()
{
}

TrieNode::~TrieNode()
{
    for (ChildrenIterator iter = Children.begin();
        iter != Children.end();
        ++iter)
    {
        delete iter->second;
    }
}

void TrieNode::Insert(const std::string& word)
{
    TrieNode* node = this;
    foreach (uchar letter, word) {
        AUTO(&children, node->Children);
        if (Contains(children, letter) == false) {
            TrieNode* new_node = new TrieNode;
            children.insert(std::make_pair(letter, new_node));
        }

        node = children[letter];
    }

    node->Word = word;
}

void TrieNode::Erase(const std::string& word)
{
    std::vector<TrieNode*> node_list;
    node_list.reserve(word.size() + 1);

    TrieNode* node = this;
    node_list.push_back(node);
    foreach (uchar letter, word)
    {
        AUTO(&children, node->Children);
        if (Contains(children, letter) == false) {
            return;
        }

        node = children[letter];
        node_list.push_back(node);
    }

    node->Word.clear();

    assert ( node_list.size() == (word.size() + 1) );

    for (int ii = static_cast<int>(word.size()) - 1; ii >= 0; --ii)
    {
        if (node_list[ii + 1]->Children.size() == 0 &&
            node_list[ii + 1]->Word.empty())
        {
            AUTO (&children, node_list[ii]->Children);
            uchar letter_key = word[ii];

            node = children[letter_key];
            delete node;

            children.erase(letter_key);
        }
        else
        {
            break;
        }
    }
}

void TrieNode::Clear()
{
    Word.clear();
    Children.clear();
}

