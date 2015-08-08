#pragma once
struct TrieNode : public boost::noncopyable {
  // only supports 26 lowercase letters + 26 uppercase letters + 10 numbers +
  // underscore and hyphen and a 26 + 26 + 10 + 2 == 64
  enum Constants {
    kNumChars = 64,
  };

  static void InitStatic();

  static uchar msCharIndex[256];
  static bool msValidChar[256];

  TrieNode(TrieNode* parent, uchar letter);
  ~TrieNode();

  bool IsValidWord(const String& word) const;
  void Insert(const String& word);
  void Erase(const String& word);

  std::string GetWord() const;

  TrieNode* const Parent;
  const uchar Letter;
  bool IsWord;
  uchar NumChildren;
  TrieNode* Children[kNumChars];
};
