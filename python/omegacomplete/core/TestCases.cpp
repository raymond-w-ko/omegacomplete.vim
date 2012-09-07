#include "stdafx.hpp"

#ifdef HAVE_RECENT_BOOST_LIBRARY

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#endif

#include "TestCases.hpp"
#include "TrieNode.hpp"
#include "GlobalWordSet.hpp"
#include "TagsSet.hpp"
#include "Algorithm.hpp"

TestCases::TestCases()
{
    std::cout << "running test cases" << std::endl;
    std::cout << std::endl;

    ustringTest();
    std::cout << std::endl;

    TrieNodeTest();
    std::cout << std::endl;

    TagsTest();
    std::cout << std::endl;

    ClangTest();
    std::cout << std::endl;
}

TestCases::~TestCases()
{
    ;
}

void TestCases::ustringTest()
{
    std::cout << "checking ustring<8>" << std::endl;
    ustring<8> dummy("lelouch");
}

void TestCases::TrieNodeTest()
{
#ifdef _WIN32
    TrieNode root_node;

    std::ifstream input_file("wordlists/brit-a-z.txt");
    std::string line;

    // creation time
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

    // search time
    unsigned int num_searches = 32;
    start_time = ::GetTickCount();
    LevenshteinSearchResults results;
    std::vector<std::string> words;
    while ( std::getline(input_file, line) )
    {
        words.push_back(line);
    }
    std::cout << "randomly selecting " << num_searches
              << " words from from a pool of "
              << words.size() << std::endl;

    boost::random::mt19937 gen;
    gen.seed( static_cast<unsigned int>(0) );
    boost::random::uniform_int_distribution<> dist(0, words.size() - 1);
    for (size_t ii = 0; ii < num_searches; ++ii)
    {
        const std::string& word = words.at(dist(gen));
        GlobalWordSet::LevenshteinSearch(word, 2, root_node, results);
    }
    end_time = ::GetTickCount();
    delta = (end_time - start_time);
    std::cout << "trie search time: " << delta  << " ms" << std::endl;

    input_file.clear();
    input_file.seekg(0, std::ios::beg);

    // deletion time
    start_time = ::GetTickCount();
    while ( std::getline(input_file, line) )
    {
        root_node.Erase(line);
    }
    end_time = ::GetTickCount();
    delta = (end_time - start_time);
    std::cout << "wordlist removal time: " << delta << " ms" << std::endl;

    always_assert( root_node.Children.size() == 0 );
    always_assert( root_node.Word.empty() );
#endif
}

void TestCases::TagsTest()
{
    std::cout << "preloading tags files:" << std::endl;
    std::string current_directory;
    std::string tags;

    current_directory = "C:\\";

    tags = "C:\\SVN\\Syandus_ALIVE4\\Platform\\ThirdParty\\OGRE\\Include\\tags";
    std::cout << tags << std::endl;
    TagsSet::Instance()->CreateOrUpdate(tags, current_directory);

    //TagsSet::Instance()->Clear();
    //Algorithm::ClearGlobalCache();
}

void TestCases::ClangTest()
{
#ifdef ENABLE_CLANG_COMPLETION
    std::cout << "checking to see if libclang/libclang.dll works" << std::endl;

    int argc = 1;
    char* argv[] = {"--version"};

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index, 0, argv, argc, 0, 0, CXTranslationUnit_None);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
#endif
}
