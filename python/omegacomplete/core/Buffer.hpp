#pragma once

class Omegacomplete;

class Buffer {
 public:
  void TokenizeContentsIntoKeywords(StringPtr contents,
                                    UnorderedStringIntMapPtr words);

 public:
  Buffer() : parent_(NULL) {}
  ~Buffer();
  void Init(Omegacomplete* parent, unsigned buffer_id);

  bool operator==(const Buffer& other) {
    return buffer_id_ == other.buffer_id_;
  }
  unsigned GetNumber() const { return buffer_id_; }
  void ReplaceContentsWith(StringPtr new_contents);
  void SetIskeyword(std::string iskeyword);
  int GetWordBeginIndex(const std::string& line, FileLocation loc);

 private:
  Omegacomplete* parent_;
  unsigned buffer_id_;

  // the buffer's contents
  StringPtr contents_;
  // set of all unique words generated from this buffer's contents
  UnorderedStringIntMapPtr words_;

  std::string mIskeyword;
  bool mIsWord[256];
};

namespace boost {
template <>
struct hash<Buffer> {
  size_t operator()(const Buffer& buffer) const {
    return boost::hash<int>()(buffer.GetNumber());
  }
};
}
