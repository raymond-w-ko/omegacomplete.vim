#pragma once
struct TrieNode : public boost::noncopyable {
  TrieNode();
  ~TrieNode();

  void Insert(const String& word);
  void Erase(const String& word);
  void Clear();

  String Word;
  typedef
      boost::unordered_map<uchar, TrieNode*>::iterator
      ChildrenIterator;
  typedef
      boost::unordered_map<uchar, TrieNode*>::const_iterator
      ChildrenConstIterator;
  boost::unordered_map<uchar, TrieNode*> Children;
};
