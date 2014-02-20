#pragma once
struct TrieNode : public boost::noncopyable {
  // only supports 26 lowercase letters + 26 uppercase letters + 10 numbers +
  // underscore and hyphen and a 26 + 26 + 10 + 2 == 64
  enum Constants {
    kNumChars = 64,
  };

  static void InitStatic();

  static char msCharIndex[256];
  static bool msValidChar[256];

  TrieNode(TrieNode* parent, char letter);
  ~TrieNode();

  void Insert(const String& word);
  void Erase(const String& word);

  std::string GetWord() const;

  typedef
      boost::unordered_map<uchar, TrieNode*>::iterator
      ChildrenIterator;
  typedef
      boost::unordered_map<uchar, TrieNode*>::const_iterator
      ChildrenConstIterator;

  TrieNode* const Parent;
  const char Letter;
  bool IsWord;
  TrieNode* Children[kNumChars];
  unsigned NumChildren;
};
