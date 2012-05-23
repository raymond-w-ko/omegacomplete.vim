#include "stdafx.hpp"
#include "GlobalWordSet.hpp"

char GlobalWordSet::is_part_of_word_[256];
char GlobalWordSet::to_lower_[256];
boost::unordered_map<std::string, std::string> GlobalWordSet::title_case_cache_;
boost::unordered_map<std::string, std::string> GlobalWordSet::underscore_cache_;

void GlobalWordSet::GlobalInit()
{
    // generate lookup tables
    std::string temp(1, ' ');
    for (size_t index = 0; index <= 255; ++index)
    {
        is_part_of_word_[index] = IsPartOfWord(index) ? 1 : 0;
        temp.resize(1, ' ');
        temp[0] = (char)index;
        boost::algorithm::to_lower(temp);
        to_lower_[index] = temp[0];
    }
}

const std::string& GlobalWordSet::ComputeUnderscore(const std::string& word)
{
    if (Contains(underscore_cache_, word) == true) {
        return underscore_cache_[word];
    }

    std::string underscore;

    const size_t word_length = word.length();
    for (size_t ii = 0; ii < word_length; ++ii) {
        char c = word[ii];

        if (ii == 0) {
            underscore += to_lower_[c];
            continue;
        }

        if (word[ii] == '_')
        {
            if (ii < (word_length - 1) && word[ii + 1] != '_')
                underscore += to_lower_[word[ii + 1]];
        }
    }

    if (underscore.length() < 2) underscore = "";

    // store calculation in cache
    underscore_cache_[word] = underscore;

    return underscore_cache_[word];
}

const std::string& GlobalWordSet::ComputeTitleCase(const std::string& word)
{
    if (Contains(underscore_cache_, word) == true) {
        return title_case_cache_[word];
    }

    std::string title_case;

    const size_t word_length = word.length();
    for (size_t ii = 0; ii < word_length; ++ii)
    {
        char c = word[ii];

        if (ii == 0) {
            title_case += to_lower_[c];
            continue;
        }

        if (IsUpper(c)) {
            title_case += to_lower_[c];
        }
    }

    if (title_case.length() < 2) title_case = "";

    // store calculation in cache
    title_case_cache_[word] = title_case;

    return title_case_cache_[word];
}

WordInfo::WordInfo()
:
ReferenceCount(0),
GeneratedAbbreviations(false)
{
    ;
}

WordInfo::~WordInfo()
{
    ;
}

GlobalWordSet::GlobalWordSet()
:
const_words_(words_)
{
    ;
}

GlobalWordSet::~GlobalWordSet()
{
    ;
}

void GlobalWordSet::UpdateWord(const std::string& word, int reference_count_delta)
{
    // the following below, while not being totally safe and correct, this done
    // this minimize any potential latency.
    bool lock_required;

    // if we can find the WordInfo struct that means we don't need a lock since
    // we are doing a read only operation on words_ to find the location of the
    // target WordInfo. If we can't find it, accessing the reference to it means
    // creation, which requires a lock since the data structure internally
    // will change.
    if (const_words_.find(word) != const_words_.end()) {
        lock_required = true;
    } else {
        lock_required = false;
    }

    if (lock_required) mutex_.lock();
    WordInfo& wi = words_[word];
    if (lock_required) mutex_.unlock();

    // this is not locked, which means that reads can be out of date, but
    // I don't care since I want performance.
    wi.ReferenceCount += reference_count_delta;

    if (wi.ReferenceCount <= 0) return;
    if (wi.GeneratedAbbreviations) return;

    const std::string& title_case = ComputeTitleCase(word);
    const std::string& underscore = ComputeUnderscore(word);

    mutex_.lock();
    if (title_case.length() > 0) {
        title_cases_.insert(make_pair(title_case, word));
    }
    if (underscore.length() > 0) {
        underscores_.insert(make_pair(underscore, word));
    }
    mutex_.unlock();

    wi.GeneratedAbbreviations = true;
}

void GlobalWordSet::GetPrefixCompletions(
    const std::string& prefix,
    std::set<std::string>* completions)
{
    mutex_.lock();

    auto(iter, words_.lower_bound(prefix));
    for (; iter != words_.end(); ++iter) {
        const std::string& candidate = iter->first;
        const WordInfo& wi = iter->second;

        if (boost::starts_with(candidate, prefix) == false) break;
        if (wi.ReferenceCount == 0) continue;

        completions->insert(candidate);
    }

    mutex_.unlock();
}

void GlobalWordSet::GetAbbrCompletions(
    const std::string& prefix,
    std::set<std::string>* completions)
{
    mutex_.lock();

    auto(bounds1, title_cases_.equal_range(prefix));
    auto(iter, bounds1.first);
    for (iter = bounds1.first; iter != bounds1.second; ++iter) {
        const std::string& candidate = iter->second;
        if (words_[candidate].ReferenceCount == 0) continue;
        completions->insert(candidate);
    }

    auto(bounds2, underscores_.equal_range(prefix));
    for (iter = bounds2.first; iter != bounds2.second; ++iter) {
        const std::string& candidate = iter->second;
        if (words_[candidate].ReferenceCount == 0) continue;
        completions->insert(candidate);
    }

    mutex_.unlock();
}
