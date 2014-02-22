#include "../stdafx.hpp"
#include "../TrieNode.hpp"
#include "../Algorithm.hpp"

static void always_assert(bool condition) {
    if (condition == false) {
#ifdef _WIN32
        DebugBreak();
#endif
        throw std::exception();
    }
}

static void loadLines(std::vector<std::string>& lines) {
  std::ifstream input_file("english-words.95");
  std::string line;

  while (std::getline(input_file, line))
    lines.push_back(line);
}

int main() {
  typedef boost::chrono::high_resolution_clock clock;
  boost::chrono::nanoseconds ns;
  double ms;

  TrieNode::InitStatic();

  TrieNode* root_node = new TrieNode(NULL, 'Q');

  std::vector<std::string> lines;
  loadLines(lines);

  // creation time
  clock::time_point start_time = clock::now();
  for (size_t i = 0; i < lines.size(); ++i) {
    root_node->Insert(lines[i]);
  }
  clock::time_point end_time = clock::now();

  ns = end_time - start_time;
  ms = ns.count() / 1e6;
  std::cout << "wordlist insertion time: " << ms << " ms" << "\n";

  // search time
  unsigned int num_searches = 256;
  LevenshteinSearchResults search_results;
  std::cout
      << "randomly selecting " << num_searches << " words from from a pool of "
      << lines.size() << "\n";

  boost::random::mt19937 gen;
  gen.seed(static_cast<unsigned int>(0));
  boost::random::uniform_int_distribution<> dist(0, static_cast<int>(lines.size() - 1));
  start_time = clock::now();
  for (size_t i = 0; i < num_searches; ++i) {
    const std::string& word = lines.at(dist(gen));
    Algorithm::LevenshteinSearch(word, 2, *root_node, search_results);
  }
  end_time = clock::now();
  ns = end_time - start_time;
  ms = ns.count() / 1e6;
  std::cout << "trie search time: " << ms  << " ms" << "\n";

  // deletion time
  start_time = clock::now();
  for (size_t i = 0; i < lines.size(); ++i)
    root_node->Erase(lines[i]);
  end_time = clock::now();
  ns = end_time - start_time;
  ms = ns.count() / 1e6;
  std::cout << "wordlist removal time: " << ms << " ms" << "\n";

  always_assert(root_node->NumChildren == 0);
  always_assert(!root_node->IsWord);

  return 0;
}
