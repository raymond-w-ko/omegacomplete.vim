#pragma once

class TestCases : public boost::noncopyable
{
public:
    TestCases();
    ~TestCases();

private:
    void TrieNodeTest();
    void TagsTest();
    void ClangTest();
};
