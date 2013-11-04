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
    Omegacomplete::Completions* completions,
    Omegacomplete::DoneStatus* done_status,
    const boost::unordered_map<int, String>* word_list,
    int begin,
    int end,
    const std::string& input,
    bool terminus_mode) {
  CompleteItem item;
  for (int i = begin; i < end; ++i) {
    AUTO(const & iter, word_list->find(i));
    if (iter == word_list->end())
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
      item.Score *= 2.0f;

    if (item.Score > 0) {
      item.Word = word;
      completions->Mutex.lock();
      completions->Items->push_back(item);
      completions->Mutex.unlock();
    }
  }

  if (input.size() >= 2 && input[input.size() - 1] == '_') {
    std::string trimmed_input(input.begin(), input.end() - 1);
    Algorithm::ProcessWords(
        completions,
        done_status,
        word_list,
        begin, end,
        trimmed_input,
        true);
  } else {
    boost::mutex::scoped_lock lock(done_status->Mutex);
    done_status->Count += 1;
    done_status->ConditionVariable.notify_one();
  }
}

float Algorithm::GetWordScore(const std::string& word, const std::string& input,
                              bool terminus_mode) {
  if (terminus_mode && word[word.size() - 1] != '_')
    return 0;

  const float word_size = static_cast<float>(word.size());
  float score1 = 0;
  float score2 = 0;

  std::string boundaries = Algorithm::GetWordBoundaries(word);

  if (boundaries.size() > 0 &&
      IsSubsequence(input, boundaries) &&
      IsSubsequence(word, input)) {
    score1 = input.size() / word_size;
  }

  if (input.size() > 4 && boost::starts_with(word, input)) {
    score2 = input.size() / word_size;
  }

  return std::max(score1, score2);
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

std::string Algorithm::GetWordBoundaries(const std::string& word) {
  std::string boundaries;

  if (word.size() < 3)
    return boundaries;

  boundaries += word[0];
  const size_t len = word.size() - 1;
  for (size_t i = 1; i < len; ++i) {
    char c = word[i];
    char c2 = word[i + 1];
    if (LookupTable::IsUpper[c] && !LookupTable::IsUpper[c2]) {
      boundaries.push_back(c);
    } else if (c == '_' || c == '-') {
      boundaries.push_back(c2);
    }
  }

  if (boundaries.size() > 1) {
    return boundaries;
  } else {
    return std::string();
  }
}
