#pragma once

struct WordInfo {
    WordInfo()
        : ReferenceCount(0),
          GeneratedAbbreviations(false) {
    }

    ~WordInfo() { }

    int ReferenceCount;
    bool GeneratedAbbreviations;
};
