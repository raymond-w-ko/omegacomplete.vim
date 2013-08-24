#include "stdafx.hpp"

#include "LookupTable.hpp"
#include "CompletionPriorities.hpp"
#include "Algorithm.hpp"

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Algorithm::LevenshteinSearch(
    const std::string& word,
    const size_t max_cost,
    const TrieNode& trie,
    LevenshteinSearchResults& results) {
  // generate sequence from [0, len(word)]
  size_t row_size = word.size() + 1;
  std::vector<size_t> current_row(row_size);
  for (size_t i = 0; i < row_size; ++i)
    current_row[i] = i;

  for (TrieNode::ChildrenConstIterator iter = trie.Children.begin();
       iter != trie.Children.end();
       ++iter) {
    char letter = iter->first;
    TrieNode* next_node = iter->second;

    Algorithm::LevenshteinSearchRecursive(
        *next_node,
        letter,
        word,
        current_row,
        results,
        max_cost);
  }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Algorithm::LevenshteinSearchRecursive(
    const TrieNode& node,
    char letter,
    const std::string& word,
    const std::vector<size_t>& previous_row,
    LevenshteinSearchResults& results,
    size_t max_cost) {
  size_t columns = word.length() + 1;
  std::vector<size_t> current_row(columns);
  current_row[0] = previous_row[0] + 1;

  // Build one row for the letter, with a column for each letter in the target
  // word, plus one for the empty string at column 0
  for (unsigned column = 1; column < columns; ++column) {
    size_t insert_cost = current_row[column - 1] + 1;
    size_t delete_cost = previous_row[column] + 1;

    size_t replace_cost;
    if (word[column - 1] != letter)
      replace_cost = previous_row[column - 1] + 1;
    else
      replace_cost = previous_row[column - 1];

    current_row[column] = (std::min)(insert_cost,
                                     (std::min)(delete_cost, replace_cost));
  }

  // if the last entry in the row indicates the optimal cost is less than the
  // maximum cost, and there is a word in this trie node, then add it.
  size_t last_index = current_row.size() - 1;
  if (current_row[last_index] <= max_cost && !node.Word.empty()) {
    results[current_row[last_index]].insert(node.Word);
  }

  // if any entries in the row are less than the maximum cost, then
  // recursively search each branch of the trie
  if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost) {
    for (TrieNode::ChildrenConstIterator iter = node.Children.cbegin();
         iter != node.Children.cend();
         ++iter) {
      char next_letter = iter->first;
      TrieNode* next_node = iter->second;

      LevenshteinSearchRecursive(
          *next_node,
          next_letter,
          word,
          current_row,
          results,
          max_cost);
    }
  }
}

void Algorithm::ProcessWords(
    Omegacomplete::Completions& completions,
    boost::shared_ptr<boost::mutex> mutex,
    const boost::unordered_map<int, String>& word_list,
    int begin,
    int end,
    const std::string& input,
    bool terminus_mode) {
  CompleteItem item;
  for (int i = begin; i < end; ++i) {
    AUTO(const & iter, word_list.find(i));
    if (iter == word_list.end())
      continue;
    const std::string& word = iter->second;
    if (word.empty())
      continue;
    if (word == input)
      continue;
    if (input.size() > word.size())
      continue;

    item.Score = Algorithm::GetWordScore(word, input, terminus_mode);
    if (terminus_mode)
      item.Score *= 100.0f;

    if (item.Score > 0) {
      item.Word = word;
      completions.Mutex.lock();
      completions.Items->push_back(item);
      completions.Mutex.unlock();
    }
  }

  if (input[input.size() - 1] == '_') {
    int trimmed_end = static_cast<int>(input.size()) - 1;
    while (input[trimmed_end] == '_')
      trimmed_end--;
    trimmed_end++;
    std::string trimmed_input(input.begin(), input.begin() + trimmed_end);
    Algorithm::ProcessWords(
        completions,
        mutex,
        word_list,
        begin, end,
        trimmed_input,
        true);
  } else {
    mutex->unlock();
  }
}

float Algorithm::GetWordScore(const std::string& word, const std::string& input,
                              bool terminus_mode) {
  if (terminus_mode && word[word.size() - 1] != '_')
    return 0;

  float score = 0;

  score = 100 * TitleCaseMatchScore(word, input);
  if (score > 0.0f)
    return score;

  score = 90 * UnderScoreMatchScore(word, input);
  if (score > 0.0f)
    return score;

  score = 80 * HyphenMatchScore(word, input);
  if (score > 0.0f)
    return score;

  if (boost::starts_with(word, input))
    return 40;

  return 0;
}

float Algorithm::TitleCaseMatchScore(const std::string& word,
                                 const std::string& input) {
  if (word.size() < 4 || input.size() < 2)
    return 0.0f;

  std::string abbrev;
  abbrev += LookupTable::ToLower[word[0]];
  size_t len = word.size();
  uchar ch;
  for (size_t i = 1; i < len; ++i) {
    ch = word[i];
    if (LookupTable::IsUpper[ch]) {
      abbrev += LookupTable::ToLower[ch];
    }
  }

  if (input == abbrev)
    return 1.0f;
  if (abbrev.size() <= 1)
    return 0.0f;
  if (abbrev.size() > input.size())
    return 0.0f;

  if (IsSubsequence(word, input) && IsSubsequence(input, abbrev))
    return 0.5f;

  return 0.0f;
}

float Algorithm::SeparatorMatchScore(const std::string& word,
                                 const std::string& input,
                                 const uchar separator) {
  if (word.size() < 3 || input.size() < 2)
    return 0.0f;;

  std::string abbrev;
  abbrev += LookupTable::ToLower[word[0]];

  size_t len = word.size();
  for (size_t i = 2; i < len; ++i) {
    if (word[i - 1] == separator) {
      abbrev += LookupTable::ToLower[word[i]];
    }
  }

  if (input == abbrev)
    return 1.0f;
  if (abbrev.size() <= 1)
    return 0.0f;
  if (abbrev.size() > input.size())
    return 0.0f;

  if (IsSubsequence(word, input) && IsSubsequence(input, abbrev))
    return 0.5f;

  return 0.0f;
}

float Algorithm::UnderScoreMatchScore(const std::string& word, const std::string& input) {
  return SeparatorMatchScore(word, input, '_');
}

float Algorithm::HyphenMatchScore(const std::string& word, const std::string& input) {
  return SeparatorMatchScore(word, input, '-');
}

bool Algorithm::IsSubsequence(const std::string& word, const std::string& input) {
  size_t word_size = word.size();
  size_t input_size = input.size();

  if (input_size > word_size)
    return false;

  size_t j = 0;
  for (size_t i = 0; i < word_size; ++i) {
    char word_ch = word[i];
    if (j >= input_size)
      break;
    char input_ch = input[j];
    if (LookupTable::ToLower[input_ch] == LookupTable::ToLower[word_ch])
      j++;
  }

  if (j == input_size)
    return true;
  else
    return false;
}
