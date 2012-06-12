#include "stdafx.hpp"

#include "TestCases.hpp"
#include "TrieNode.hpp"

TestCases::TestCases()
{
    std::cout << "running test cases" << std::endl;

    TrieNodeTest();
}

TestCases::~TestCases()
{
    ;
}

void TestCases::TrieNodeTest()
{
    TrieNode root_node;

    std::ifstream input_file("wordlists/brit-a-z.txt");
    std::string line;
    DWORD start_time = ::GetTickCount();
    while ( std::getline(input_file, line) )
    {
        root_node.Insert(line);
    }
    DWORD end_time = ::GetTickCount();

    unsigned delta = (end_time - start_time);
    std::cout << "wordlist insertion time: " << delta  << " ms" << std::endl;

    input_file.clear();
    input_file.seekg(0, std::ios::beg);

    start_time = ::GetTickCount();
    while ( std::getline(input_file, line) )
    {
        root_node.Erase(line);
    }
    end_time = ::GetTickCount();
    delta = (end_time - start_time);
    std::cout << "wordlist removal time: " << delta  << " ms" << std::endl;

    assert( root_node.Children.size() == 0 );
    assert( root_node.Word.empty() );
}
