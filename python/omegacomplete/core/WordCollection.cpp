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
  if (wi.ReferenceCount <= 0)
    return;

  {
    boost::mutex::scoped_lock trie_lock(trie_mutex_);
    trie_.Insert(word);
  }

  if (wi.GeneratedAbbreviations)
    return;

  UnsignedStringPairVectorPtr title_cases = Algorithm::ComputeTitleCase(word);
  UnsignedStringPairVectorPtr underscores = Algorithm::ComputeUnderscore(word);
  UnsignedStringPairVectorPtr hyphens = Algorithm::ComputeHyphens(word);

  // generate and store abbreviations
  foreach (const UnsignedStringPair& title_case, *title_cases) {
    AbbreviationInfo ai(title_case.first + kPriorityTitleCase, word);
    abbreviations_[title_case.second].insert(ai);
  }
  foreach (const UnsignedStringPair& underscore, *underscores) {
    AbbreviationInfo ai(underscore.first + kPriorityUnderscore, word);
    abbreviations_[underscore.second].insert(ai);
  }
  foreach (const UnsignedStringPair& hyphen, *hyphens) {
    AbbreviationInfo ai(hyphen.first + kPriorityHyphen, word);
    abbreviations_[hyphen.second].insert(ai);
  }

  wi.GeneratedAbbreviations = true;
}

void WordCollection::GetPrefixCompletions(
    const std::string& input,
    CompleteItemVectorPtr& completions, std::set<std::string>& added_words,
    bool terminus_mode) {
  boost::mutex::scoped_lock lock(mutex_);

  AUTO(iter, words_.lower_bound(input));
  for (; iter != words_.end(); ++iter) {
    const std::string& candidate = iter->first;
    const WordInfo& wi = iter->second;

    if (completions->size() == LookupTable::kMaxNumCompletions)
      break;
    if (boost::starts_with(candidate, input) == false)
      break;

    if (wi.ReferenceCount == 0)
      continue;
    if (terminus_mode && !boost::ends_with(candidate, "_"))
      continue;
    if (boost::ends_with(candidate, "-"))
      continue;
    if (Contains(added_words, candidate))
      continue;

    CompleteItem completion(candidate);
    completion.Menu = boost::str(boost::format("        [%d Counts]")
                                 % wi.ReferenceCount);
    completions->push_back(completion);

    added_words.insert(candidate);
  }
}

void WordCollection::GetAbbrCompletions(
    const std::string& input,
    CompleteItemVectorPtr& completions, std::set<std::string>& added_words,
    bool terminus_mode) {
  boost::mutex::scoped_lock lock(mutex_);

  AUTO(const &set, abbreviations_[input]);
  AUTO(iter, set.begin());
  for (; iter != set.end(); ++iter) {
    const AbbreviationInfo& candidate = *iter;
    const WordInfo& wi = words_[candidate.Word];

    if (completions->size() == LookupTable::kMaxNumCompletions)
      break;

    if (wi.ReferenceCount == 0)
      continue;
    if (terminus_mode && !boost::ends_with(candidate.Word, "_"))
      continue;
    if (Contains(added_words, candidate.Word))
      continue;

    CompleteItem completion(candidate.Word, candidate.Weight);
    completion.Menu = boost::str(boost::format("        [%d Counts]")
                                 % wi.ReferenceCount);
    completions->push_back(completion);

    added_words.insert(candidate.Word);
  }
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
    std::vector<UnsignedStringPairVectorPtr> collections;

    collections.push_back(Algorithm::ComputeTitleCase(word));
    collections.push_back(Algorithm::ComputeUnderscore(word));
    collections.push_back(Algorithm::ComputeHyphens(word));

    foreach (UnsignedStringPairVectorPtr collection, collections) {
      foreach (UnsignedStringPair w, *collection) {
        std::set<AbbreviationInfo>& set = abbreviations_[w.second];

        AUTO (j, set.begin());
        while (j != set.end()) {
          if (j->Word == word)
            set.erase(j++);
          else
            ++j;
        }

        if (abbreviations_[w.second].size() == 0)
          abbreviations_.erase(w.second);
      }
    }

    words_.erase(word);

    {
      boost::mutex::scoped_lock trie_lock(trie_mutex_);
      trie_.Erase(word);
    }
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
