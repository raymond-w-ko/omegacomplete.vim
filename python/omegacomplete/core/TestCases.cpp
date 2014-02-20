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

static void always_assert(bool condition) {
    if (condition == false) {
#ifdef _WIN32
        DebugBreak();
#endif
        throw std::exception();
    }

    return;
}

void TestCases::TrieNodeTest(std::stringstream& results) {
#ifdef _WIN32
  TrieNode* root_node = new TrieNode(NULL, ' ');

  std::ifstream input_file("wordlist.txt");
  std::string line;

  std::vector<std::string> lines;

  // creation time
  while (std::getline(input_file, line))
    lines.push_back(line);

  DWORD start_time = ::GetTickCount();
  for (size_t i = 0; i < lines.size(); ++i)
    root_node->Insert(lines[i]);
  DWORD end_time = ::GetTickCount();

  unsigned delta = (end_time - start_time);
  results << "wordlist insertion time: " << delta << " ms" << "\n";

  // search time
  unsigned int num_searches = 256;
  LevenshteinSearchResults search_results;
  results << "randomly selecting " << num_searches << " words from from a pool of "
          << lines.size() << "\n";

  boost::random::mt19937 gen;
  gen.seed(static_cast<unsigned int>(0));
  boost::random::uniform_int_distribution<> dist(0, static_cast<int>(lines.size() - 1));
  start_time = ::GetTickCount();
  for (size_t i = 0; i < num_searches; ++i) {
    const std::string& word = lines.at(dist(gen));
    Algorithm::LevenshteinSearch(word, 2, *root_node, search_results);
  }
  end_time = ::GetTickCount();
  delta = end_time - start_time;
  results << "trie search time: " << delta  << " ms" << "\n";

  // deletion time
  start_time = ::GetTickCount();
  for (size_t i = 0; i < lines.size(); ++i)
    root_node->Erase(lines[i]);
  end_time = ::GetTickCount();
  delta = end_time - start_time;
  results << "wordlist removal time: " << delta << " ms" << "\n";

  always_assert(root_node->NumChildren == 0);
  always_assert(!root_node->IsWord);
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
