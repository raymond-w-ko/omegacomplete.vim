#pragma once

struct WordInfo {
    WordInfo()
        : ReferenceCount(0),
          WordListIndex(-1) {
    }

    ~WordInfo() { }

    int ReferenceCount;
    int WordListIndex;
};
