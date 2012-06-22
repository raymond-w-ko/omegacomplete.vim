#include "stdafx.hpp"

#include "LookupTable.hpp"

char LookupTable::IsPartOfWord[256];
char LookupTable::IsUpper[256];
char LookupTable::ToLower[256];

void LookupTable::GlobalInit()
{
    // generate lookup tables
    for (size_t index = 0; index <= 255; ++index)
    {
        LookupTable::IsPartOfWord[index] =
            LookupTable::isPartOfWord(index) ? 1 : 0;

        LookupTable::IsUpper[index] =
            LookupTable::isUpper(index) ? 1 : 0;

        std::string temp(1, static_cast<char>(index));
        boost::algorithm::to_lower(temp);
        LookupTable::ToLower[index] = temp[0];
    }
}

bool LookupTable::isPartOfWord(char c)
{
    if ( (('a' <= c) && (c <= 'z')) ||
         (('A' <= c) && (c <= 'Z')) ||
         (c == '_') ||
         (('0' <= c) && (c <= '9')) )
    {
        return true;
    }

    return false;
}

bool LookupTable::isUpper(char c)
{
    if (('A' <= c) && (c <= 'Z')) return true;
    return false;
}
