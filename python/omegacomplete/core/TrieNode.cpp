#include "stdafx.hpp"

#include "TrieNode.hpp"

TrieNode::TrieNode() {
}

TrieNode::~TrieNode() {
  for (ChildrenIterator iter = Children.begin();
       iter != Children.end();
       ++iter) {
    delete iter->second;
  }
}

void TrieNode::Insert(const String& word) {
  TrieNode* node = this;
  size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    uchar letter = static_cast<uchar>(word[i]);
    AUTO(&children, node->Children);
    if (!Contains(children, letter)) {
      TrieNode* new_node = new TrieNode;
      children[letter] = new_node;
      node = new_node;
    } else {
      node = children[letter];
    }
  }
  node->Word = word;
}

void TrieNode::Erase(const String& word) {
  std::vector<TrieNode*> node_list(word.size() + 1);
  TrieNode* node = this;
  node_list[0] = node;
  size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    uchar letter = static_cast<uchar>(word[i]);
    AUTO(&children, node->Children);
    if (!Contains(children, letter)) {
      return;
    }

    node = children[letter];
    node_list[i + 1] = node;
  }

  node->Word.clear();

  assert(node_list.size() == (word.size() + 1));

  for (int ii = static_cast<int>(word_size) - 1; ii >= 0; --ii) {
    if (node_list[ii + 1]->Children.size() == 0 &&
        node_list[ii + 1]->Word.empty()) {
      AUTO(&children, node_list[ii]->Children);
      uchar letter_key = word[ii];

      node = children[letter_key];
      delete node;

      children.erase(letter_key);
    } else {
      break;
    }
  }
}

void TrieNode::Clear() {
  Word.clear();
  Children.clear();
}
