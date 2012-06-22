#pragma once

class LookupTable
{
public:
    static void GlobalInit();

    // used to find if a letter is part of a "word"
    static char IsPartOfWord[256];
    // the lowercase equivalent of a given letter
    static char ToLower[256];

private:
    LookupTable();
    LookupTable(const LookupTable&);
    ~LookupTable();

    static bool isPartOfWord(char c);
};
