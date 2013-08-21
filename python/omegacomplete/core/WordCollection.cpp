#include "stdafx.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "WordCollection.hpp"
#include "CompletionPriorities.hpp"

static const int kLevenshteinMaxCost = 2;
static const size_t kMinLengthForLevenshteinCompletion = 4;

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void WordCollection::LevenshteinSearch(
    const std::string& word,
    const size_t max_cost,
    const TrieNode& trie,
    LevenshteinSearchResults& results) {
  // generate sequence from [0, len(word)]
  size_t current_row_end = word.length() + 1;
  std::vector<size_t> current_row;
  for (size_t ii = 0; ii < current_row_end; ++ii)
    current_row.push_back(ii);

  for (TrieNode::ChildrenConstIterator iter = trie.Children.begin();
       iter != trie.Children.end();
       ++iter) {
    char letter = iter->first;
    TrieNode* next_node = iter->second;

    WordCollection::LevenshteinSearchRecursive(
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
void WordCollection::LevenshteinSearchRecursive(
    const TrieNode& node,
    char letter,
    const std::string& word,
    const std::vector<size_t>& previous_row,
    LevenshteinSearchResults& results,
    size_t max_cost) {
  size_t columns = word.length() + 1;
  std::vector<size_t> current_row;
  current_row.push_back( previous_row[0] + 1 );

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

    current_row.push_back( (std::min)(
            insert_cost,
            (std::min)(delete_cost, replace_cost)) );
  }

  // if the last entry in the row indicates the optimal cost is less than the
  // maximum cost, and there is a word in this trie node, then add it.
  size_t last_index = current_row.size() - 1;
  if ((current_row[last_index] <= max_cost) &&
      (node.Word.length() > 0) ) {
    results[ current_row[last_index] ].insert(node.Word);
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

void WordCollection::UpdateWord(const std::string& word, int reference_count_delta) {
  boost::mutex::scoped_lock lock(mutex_);

  WordInfo& wi = words_[word];
  wi.ReferenceCount += reference_count_delta;
  if (wi.ReferenceCount > 0) {
    // < 0 implies it doesn't exist in word_list_ yet
    if (wi.WordListIndex < 0) {
      int index;
      if (empty_indices_.size() > 0) {
        index = *empty_indices_.begin();
        empty_indices_.erase(index);
      } else {
        index = static_cast<int>(word_list_.size());
      }
      wi.WordListIndex = index;
      word_list_[index] = word;
    }
  } else {
    // >= 0 implies that it exist in the world list and we need to free it
    if (wi.WordListIndex >= 0) {
      int index = wi.WordListIndex;
      wi.WordListIndex = -1;
      word_list_[index].clear();
      empty_indices_.insert(index);
    }
    return;
  }

  {
    boost::mutex::scoped_lock trie_lock(trie_mutex_);
    trie_.Insert(word);
  }

  return;
}

size_t WordCollection::Prune() {
  boost::mutex::scoped_lock lock(mutex_);

  // simply just looping through words_ and removing things might cause
  // iterator invalidation, so be safe and built a set of things to remove
  std::vector<std::string> to_be_pruned;
  AUTO(i, words_.begin());
  for (; i != words_.end(); ++i) {
    if (i->second.ReferenceCount > 0)
      continue;
    to_be_pruned.push_back(i->first);
  }

  foreach (const std::string& word, to_be_pruned) {
    words_.erase(word);

    {
      boost::mutex::scoped_lock trie_lock(trie_mutex_);
      trie_.Erase(word);
    }
  }

  // rebuild word_list_ to eliminate any holes
  empty_indices_.clear();
  word_list_.clear();
  AUTO(iter, words_.begin());
  int index = 0;
  for (; iter != words_.end(); ++iter) {
    const std::string& word = iter->first;
    WordInfo& wi = iter->second;

    word_list_[index] = word;
    wi.WordListIndex = index;
    index++;
  }

  return to_be_pruned.size();
}

void WordCollection::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results) {
  // only try correction if we have a sufficiently long enough
  // word to make it worthwhile
  if (prefix.length() < kMinLengthForLevenshteinCompletion)
    return;

  boost::mutex::scoped_lock lock(trie_mutex_);
  WordCollection::LevenshteinSearch(prefix, kLevenshteinMaxCost, trie_, results);
}
