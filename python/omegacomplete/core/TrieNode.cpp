#include "stdafx.hpp"

#include "TrieNode.hpp"

char TrieNode::msCharIndex[256];
bool TrieNode::msValidChar[256];

void TrieNode::InitStatic() {
  assert(sizeof(char) == 1);
  assert(sizeof(bool) == 1);

  for (int i = 0; i < 256; ++i) {
    msCharIndex[(char)i] = 255;
    msValidChar[(char)i] = false;
  }

  char n = 0;
  for (char c = 'A'; c <= 'Z'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }
  for (char c = 'a'; c <= 'z'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }
  for (char c = '0'; c <= '9'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }

  msCharIndex['_'] = n++;
  msValidChar['_'] = true;

  msCharIndex['-'] = n++;
  msValidChar['-'] = true;
}

TrieNode::TrieNode(TrieNode* parent, char letter)
    : Parent(parent),
      Letter(letter),
      IsWord(false),
      NumChildren(0) {
  for (int i = 0; i < kNumChars; ++i) {
    Children[i] = NULL;
  }

  if (parent) {
    parent->Children[Letter] = this;
    parent->NumChildren++;
  }
}

TrieNode::~TrieNode() {
  if (Parent) {
    Parent->Children[Letter] = NULL;
    Parent->NumChildren--;
  }

  for (int i = 0; i < kNumChars; ++i) {
    delete Children[i];
  }
}

void TrieNode::Insert(const String& word) {
  const size_t word_size = word.size();

  for (size_t i = 0; i < word_size; ++i) {
    if (!msValidChar[word[i]]) {
      return;
    }
  }

  TrieNode* node = this;
  for (size_t i = 0; i < word_size; ++i) {
    char ch = word[i];
    char index = msCharIndex[ch];
    if (!Children[index]) {
      Children[index] = new TrieNode(node, ch);
    }
    node = Children[index];
  }
  node->IsWord = true;
}

void TrieNode::Erase(const String& word) {
  const size_t word_size = word.size();

  for (size_t i = 0; i < word_size; ++i) {
    if (!msValidChar[word[i]]) {
      return;
    }
  }

  TrieNode* node = this;
  for (size_t i = 0; i < word_size; ++i) {
    if (!node) {
      return;
    }
    char ch = word[i];
    char index = msCharIndex[ch];
    node = Children[index];
  }
  if (!node) {
    return;
  }

  node->IsWord = false;

  while (node) {
    TrieNode* prev_node = node->Parent;
    if (node->NumChildren == 0) {
      delete node;
    }
    node = prev_node;
  }
}

std::string TrieNode::GetWord() const {
  std::string word;
  const TrieNode* node = this;
  while (node) {
    word = std::string(1, node->Letter) + word;
    node = node->Parent;
  }
  return word;
}
