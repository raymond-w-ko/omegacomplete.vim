#include "stdafx.hpp"

#include "TrieNode.hpp"

uchar TrieNode::msCharIndex[256];
bool TrieNode::msValidChar[256];

void TrieNode::InitStatic() {
  assert(sizeof(uchar) == 1);
  assert(sizeof(bool) == 1);

  for (int i = 0; i < 256; ++i) {
    msCharIndex[(uchar)i] = 255;
    msValidChar[(uchar)i] = false;
  }

  uchar n = 0;
  for (uchar c = 'A'; c <= 'Z'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }
  for (uchar c = 'a'; c <= 'z'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }
  for (uchar c = '0'; c <= '9'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }

  msCharIndex['_'] = n++;
  msValidChar['_'] = true;

  msCharIndex['-'] = n++;
  msValidChar['-'] = true;
}

TrieNode::TrieNode(TrieNode* parent, uchar letter)
    : Parent(parent), Letter(letter), IsWord(false), NumChildren(0) {
  for (int i = 0; i < kNumChars; ++i) {
    Children[i] = NULL;
  }

  if (Parent) {
    Parent->Children[msCharIndex[Letter]] = this;
    Parent->NumChildren++;
  }
}

TrieNode::~TrieNode() {
  if (Parent) {
    Parent->Children[msCharIndex[Letter]] = NULL;
    Parent->NumChildren--;
  }

  for (int i = 0; i < kNumChars; ++i) {
    delete Children[i];
  }
}

inline bool TrieNode::IsValidWord(const String& word) const {
  size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    uchar ch = (uchar)word[i];
    if (!msValidChar[ch]) {
      return false;
    }
  }
  return true;
}

void TrieNode::Insert(const String& word) {
  if (!IsValidWord(word)) return;

  TrieNode* node = this;
  const size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    uchar ch = word[i];
    uchar index = msCharIndex[ch];
    if (!node->Children[index]) {
      node = new TrieNode(node, ch);
    } else {
      node = node->Children[index];
    }
  }
  node->IsWord = true;
}

void TrieNode::Erase(const String& word) {
  if (!IsValidWord(word)) return;

  TrieNode* node = this;
  const size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    if (!node) {
      return;
    }
    uchar ch = (uchar)word[i];
    uchar index = msCharIndex[ch];
    node = node->Children[index];
  }
  if (!node) {
    return;
  }

  node->IsWord = false;

  while (node) {
    TrieNode* prev_node = node->Parent;
    if (node->NumChildren == 0 && node->Parent) {
      delete node;
    }
    node = prev_node;
  }
}

std::string TrieNode::GetWord() const {
  std::string word;
  const TrieNode* node = this;
  while (node) {
    if (node->Parent) {
      word = std::string(1, node->Letter) + word;
    }
    node = node->Parent;
  }
  return word;
}
