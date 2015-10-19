#pragma once

struct AbbreviationInfo {
  AbbreviationInfo(unsigned weight, const std::string word)
      : Weight(weight), Word(word) {}

  AbbreviationInfo(const AbbreviationInfo& other)
      : Weight(other.Weight), Word(other.Word) {}

  ~AbbreviationInfo() {}

  unsigned Weight;
  const std::string Word;

  bool operator<(const AbbreviationInfo& other) const {
    if (Weight < other.Weight)
      return true;
    else if (Weight > other.Weight)
      return false;

    return Word < other.Word;
  }

 private:
  AbbreviationInfo() {}

  AbbreviationInfo& operator=(const AbbreviationInfo&);
};
