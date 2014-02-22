#include "stdafx.hpp"

#ifdef HAVE_RECENT_BOOST_LIBRARY

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#endif

#include "TestCases.hpp"
#include "TrieNode.hpp"
#include "WordCollection.hpp"
#include "TagsCollection.hpp"
#include "Algorithm.hpp"

void TestCases::TagsTest()
{
    std::cout << "preloading tags files:" << std::endl;
    std::string current_directory;
    std::string tags;

    current_directory = "C:\\";

    tags = "C:\\SVN\\Syandus_ALIVE4\\Platform\\ThirdParty\\OGRE\\Include\\tags";
    std::cout << tags << std::endl;
    //TagsCollection::Instance()->CreateOrUpdate(tags, current_directory);

    //TagsCollection::Instance()->Clear();
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
