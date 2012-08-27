#pragma once

class LookupTable : public boost::noncopyable
{
public:
    static void InitStatic();

    /// maximum number of completions to return
    static const unsigned kMaxNumCompletions;
    /// maximum number of quick match keys
    static const unsigned kMaxNumQuickMatch;

    /// used to find if a given char is part of a "word"
    static char IsPartOfWord[256];
    /// used to find if a given char is an uppercase letter
    static char IsUpper[256];
    /// the lowercase equivalent of a given char
    static char ToLower[256];
    /// used to find if a given char is a number
    static char IsNumber[256];

    /// Quick Match Keys
    /// basically a mapping from result number to keyboard key to press
    /// first result  -> '1'
    /// second result -> '2'
    /// third result  -> '3'
    /// and etc.
    static std::vector<char> QuickMatchKey;
    /// the reverse mapping of the above
    static boost::unordered_map<char, unsigned> ReverseQuickMatch;

private:
    /// this is a purely static class
    LookupTable();

    static bool isPartOfWord(char c);
    static bool isUpper(char c);
    static bool isNumber(char c);
};
