#include "stdafx.hpp"
#include "CompletionSet.hpp"
#include "Session.hpp"

const unsigned CompletionSet::kMaxNumCompletions = 32;

CompletionSet::CompletionSet()
:
num_completions_added_(0)
{
    ;
}

unsigned int CompletionSet::GetNumCompletions() const
{
    return AbbrCompletions.size() +
           TagsAbbrCompletions.size() +
           PrefixCompletions.size() +
           TagsPrefixCompletions.size();
}

void CompletionSet::Clear()
{
    AbbrCompletions.clear();
    TagsAbbrCompletions.clear();
    PrefixCompletions.clear();
    TagsPrefixCompletions.clear();
}

void CompletionSet::FillResults(std::vector<StringPair>& result_list)
{
    addWordsToResults(AbbrCompletions, result_list);
    addWordsToResults(TagsAbbrCompletions, result_list);
    addWordsToResults(PrefixCompletions, result_list);
    addWordsToResults(TagsPrefixCompletions, result_list);
}

void CompletionSet::addWordsToResults(
    const std::set<std::string>& words,
    std::vector<StringPair>& result_list)
{
    foreach (const std::string& word, words)
    {
        if (num_completions_added_ >= kMaxNumCompletions) break;
        if (Contains(added_words_, word) == true) continue;

        result_list.push_back(std::make_pair(
            word,
            boost::lexical_cast<std::string>(
                Session::QuickMatchKey[num_completions_added_++]) ));

        added_words_.insert(word);
    }
}

void CompletionSet::AddBannedWord(const std::string& word)
{
    added_words_.insert(word);
}

void CompletionSet::FillLevenshteinResults(
    const LevenshteinSearchResults& levenshtein_completions,
    const std::string& prefix_to_complete,
    std::string& result)
{
    auto (iter, levenshtein_completions.begin());
    for (; iter != levenshtein_completions.end(); ++iter) {
        if (num_completions_added_ >= kMaxNumCompletions) break;

        int score = iter->first;
        foreach (const std::string& word, iter->second) {
            if (num_completions_added_ >= kMaxNumCompletions) break;
            if (Contains(added_words_, word) == true) continue;
            if (word == prefix_to_complete) continue;
            if (boost::starts_with(prefix_to_complete, word)) continue;

            result += boost::str(boost::format(
                "{'abbr':'*** %s','word':'%s'},")
                % word % word);

            num_completions_added_++;

            added_words_.insert(word);
        }
    }
}
