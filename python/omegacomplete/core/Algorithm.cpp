#include "stdafx.hpp"

#include "LookupTable.hpp"
#include "CompletionPriorities.hpp"
#include "Algorithm.hpp"

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
  if (word == input)
    return 0;

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
