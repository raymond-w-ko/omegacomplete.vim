#include "stdafx.hpp"

#include "LookupTable.hpp"

const unsigned LookupTable::kMaxNumCompletions = 16;
const unsigned LookupTable::kMaxNumQuickMatch = 10;

char LookupTable::IsPartOfWord[256];
char LookupTable::IsUpper[256];
char LookupTable::ToLower[256];
char LookupTable::IsNumber[256];
std::vector<char> LookupTable::QuickMatchKey;
boost::unordered_map<char, unsigned> LookupTable::ReverseQuickMatch;

void LookupTable::InitStatic() {
  // generate lookup tables
  for (size_t index = 0; index <= 255; ++index) {
    char ch = static_cast<char>(index);
    IsPartOfWord[index] = isPartOfWord(ch) ? 1 : 0;
    IsUpper[index] = isUpper(ch) ? 1 : 0;
    IsNumber[index] = isNumber(ch) ? 1 : 0;

    std::string temp(1, ch);
    boost::algorithm::to_lower(temp);
    LookupTable::ToLower[index] = temp[0];
  }

  QuickMatchKey.resize(kMaxNumCompletions, ' '),

  QuickMatchKey[0] = '1';
  QuickMatchKey[1] = '2';
  QuickMatchKey[2] = '3';
  QuickMatchKey[3] = '4';
  QuickMatchKey[4] = '5';
  QuickMatchKey[5] = '6';
  QuickMatchKey[6] = '7';
  QuickMatchKey[7] = '8';
  QuickMatchKey[8] = '9';
  QuickMatchKey[9] = '0';

  for (unsigned i = 0; i < 10; ++i) {
    ReverseQuickMatch[QuickMatchKey[i]] = i;
  }
}

bool LookupTable::isPartOfWord(char c) {
  if ((('a' <= c) && (c <= 'z')) ||
      (('A' <= c) && (c <= 'Z')) ||
      (c == '-') ||
      (c == '_') ||
      (('0' <= c) && (c <= '9'))) {
    return true;
  }

  return false;
}

bool LookupTable::isUpper(char c) {
  if (('A' <= c) && (c <= 'Z')) {
    return true;
  } else {
    return false;
  }
}

bool LookupTable::isNumber(char c) {
  if (('0' <= c) && (c <= '9')) {
    return true;
  } else {
    return false;
  }
}
