#pragma once

class TestCases : public boost::noncopyable {
public:
 TestCases() {}
 ~TestCases() {}

 void TrieNodeTest(std::stringstream& results);
 void TagsTest();
 void ClangTest();
};
