#include "stdafx.hpp"

#include "LookupTable.hpp"
#include "CompletionPriorities.hpp"
#include "Algorithm.hpp"

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Algorithm::LevenshteinSearch(const std::string& word,
                                  const size_t max_cost, const TrieNode& trie,
                                  LevenshteinSearchResults& results) {
  // generate sequence from [0, len(word)]
  size_t row_size = word.size() + 1;
  std::vector<size_t> current_row(row_size);
  for (size_t i = 0; i < row_size; ++i) current_row[i] = i;

  for (int i = 0; i < TrieNode::kNumChars; ++i) {
    TrieNode* next_node = trie.Children[i];
    if (!next_node) continue;
    char letter = trie.Letter;

    Algorithm::LevenshteinSearchRecursive(*next_node, letter, word, current_row,
                                          results, max_cost);
  }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Algorithm::LevenshteinSearchRecursive(
    const TrieNode& node, char letter, const std::string& word,
    const std::vector<size_t>& previous_row, LevenshteinSearchResults& results,
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

    current_row[column] =
        (std::min)(insert_cost, (std::min)(delete_cost, replace_cost));
  }

  // if the last entry in the row indicates the optimal cost is less than the
  // maximum cost, and there is a word in this trie node, then add it.
  size_t last_index = current_row.size() - 1;
  if (current_row[last_index] <= max_cost && node.IsWord) {
    results[current_row[last_index]].insert(node.GetWord());
  }

  // if any entries in the row are less than the maximum cost, then
  // recursively search each branch of the trie
  if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost) {
    for (int i = 0; i < TrieNode::kNumChars; ++i) {
      TrieNode* next_node = node.Children[i];
      if (!next_node) continue;
      char next_letter = next_node->Letter;

      LevenshteinSearchRecursive(*next_node, next_letter, word, current_row,
                                 results, max_cost);
    }
  }
}

void Algorithm::ProcessWords(Omegacomplete::Completions* completions,
                             Omegacomplete::DoneStatus* done_status,
                             const std::vector<String>* word_list,
                             const int begin, const int end,
                             const std::string& input,
                             const bool terminus_mode) {
  CompleteItem item;
  for (int i = begin; i < end; ++i) {
    const String& word = (*word_list)[i];
    if (word.empty()) continue;
    if (word == input) continue;
    if (input.size() > word.size()) continue;

    item.Score = Algorithm::GetWordScore(word, input, terminus_mode);

    if (item.Score > 0) {
      item.Word = word;
      completions->Mutex.lock();
      completions->Items->push_back(item);
      completions->Mutex.unlock();
    }
  }

  if (input.size() >= 2 && input[input.size() - 1] == '_') {
    std::string trimmed_input(input.begin(), input.end() - 1);
    Algorithm::ProcessWords(completions, done_status, word_list, begin, end,
                            trimmed_input, true);
  } else {
    boost::mutex::scoped_lock lock(done_status->Mutex);
    done_status->Count += 1;
    done_status->ConditionVariable.notify_one();
  }
}

float Algorithm::GetWordScore(const std::string& word, const std::string& input,
                              const bool terminus_mode) {
  if (terminus_mode && word[word.size() - 1] != '_') return 0;

  const float word_size = static_cast<float>(word.size());
  float score1 = 0;
  float score2 = 0;

  std::string boundaries;
  Algorithm::GetWordBoundaries(word, boundaries);

  if (boundaries.size() > 0) {
    if (IsSubsequence(input, boundaries) && IsSubsequence(word, input)) {
      score1 = input.size() / word_size;
    }
  }

  if (input.size() > 3 && boost::starts_with(word, input)) {
    score2 = input.size() / word_size;
  }

  float score3 = Algorithm::GetClumpingScore(word, input);

  score1 = std::max(std::max(score1, score2), score3);
  if (terminus_mode) {
    score1 *= 2.0;
  }
  return score1;
}

bool Algorithm::IsSubsequence(const std::string& haystack,
                              const std::string& needle) {
  const size_t haystack_size = haystack.size();
  const size_t needle_size = needle.size();

  if (needle_size > haystack_size) return false;

  size_t j = 0;
  for (size_t i = 0; i < haystack_size; ++i) {
    char haystack_ch = haystack[i];
    if (j >= needle_size) break;
    char needle_ch = needle[j];
    if (LookupTable::ToLower[needle_ch] == LookupTable::ToLower[haystack_ch])
      j++;
  }

  if (j == needle_size)
    return true;
  else
    return false;
}

void Algorithm::GetWordBoundaries(const std::string& word,
                                  std::string& boundaries) {
  if (word.size() < 3) {
    return;
  }

  boundaries += word[0];
  const size_t len = word.size() - 1;
  for (size_t i = 1; i < len; ++i) {
    char c = word[i];
    char c2 = word[i + 1];

    if (c == '_' || c == '-') {
      boundaries.push_back(c2);
      // skip pushed char
      ++i;
    } else if (!LookupTable::IsUpper[c] && LookupTable::IsUpper[c2]) {
      boundaries.push_back(c2);
      ++i;
    }
  }

  if (boundaries.size() == 1) {
    boundaries.clear();
  }
}

float Algorithm::GetClumpingScore(const std::string& word,
                                  const std::string& input) {
  if (word.size() < 8 || input.size() < 4) {
    return 0.0f;
  }

  const float letter_score = 1.0f / word.size();

  enum ClumpStatus {
    kNoClumping,
    kMatch1,
    kMatch2,
    kMatch3OrMore,
  };
  size_t i = 0;
  size_t j = 0;
  ClumpStatus clumping = kNoClumping;
  float score = 0.0f;

  while (i < word.size() && j < input.size()) {
    if (LookupTable::ToLower[word[i]] == LookupTable::ToLower[input[j]]) {
      if (clumping == kNoClumping) {
        // some clumping must happen, otherwise we get really crazy matches
        // in fact, we penalize the initial match to prevent the case of
        // initial clumping followed by sporadic unclumped match afterwards.
        score += 0.0f;
        clumping = kMatch1;
      } else if (clumping == kMatch1) {
        score += 1.5f * letter_score;
        clumping = kMatch2;
      } else if (clumping == kMatch2) {
        score += 1.5f * letter_score;
        clumping = kMatch3OrMore;
      } else if (clumping == kMatch3OrMore) {
        score += letter_score;
      }
      j++;
    } else {
      clumping = kNoClumping;
    }
    i++;
  }

  if (j != input.size())
    return 0.0f;
  else
    return std::max(0.0f, score);
}
