#pragma once

#include "TrieNode.hpp"

class Session;

class Buffer
{
public:
    static void TokenizeContentsIntoKeywords(
        StringPtr contents,
        StringIntMapPtr words);

    Buffer();
    ~Buffer();
    bool Init(Session* parent, unsigned buffer_id);

    bool operator==(const Buffer& other);
    unsigned GetBufferId() const;

    void ReplaceContentsWith(StringPtr new_contents);

    void CalculateCurrentWordOfCursor(
        const std::string& line,
        const FileLocation& pos);

private:
    Session* parent_;
    unsigned buffer_id_;

    // the buffer's contents
    StringPtr contents_;
    // set of all unique words generated from this buffer's contents
    StringIntMapPtr words_;
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
