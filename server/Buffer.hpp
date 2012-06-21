#pragma once

#include "TrieNode.hpp"

class Session;

class Buffer
{
public:
    static void GlobalInit();
    static void TokenizeContentsIntoKeywords(
        StringPtr contents,
        UnorderedStringSetPtr words);

    Buffer();
    ~Buffer();
    bool Init(Session* parent, unsigned buffer_id);

    bool operator==(const Buffer& other);
    unsigned GetBufferId() const;

    void ReplaceContents(StringPtr new_contents);

    void CalculateCurrentWordOfCursor(
        const std::string& line,
        const std::pair<unsigned, unsigned> pos);

private:
    static char is_part_of_word_[256];
    static char to_lower_[256];

    Session* parent_;
    unsigned buffer_id_;

    // the buffer's contents
    StringPtr contents_;
    // set of all unique words generated from this buffer's contents
    UnorderedStringSetPtr words_;
    // what word the the cursor in this buffer is currently in
    std::string current_cursor_word_;
};

namespace boost
{
template<> struct hash<Buffer>
{
    size_t operator()(const Buffer& buffer) const
    {
        return boost::hash<unsigned>()(buffer.GetBufferId());
    }
};
}
