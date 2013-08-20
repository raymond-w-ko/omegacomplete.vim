#pragma once

struct WordInfo {
    WordInfo()
        : ReferenceCount(0),
          WordListIndex(-1),
          GeneratedAbbreviations(false) {
    }

    ~WordInfo() { }

    int ReferenceCount;
    int WordListIndex;
    bool GeneratedAbbreviations;
};
