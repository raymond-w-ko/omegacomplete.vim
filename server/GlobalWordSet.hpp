#pragma once

struct WordInfo
{
    WordInfo();
    ~WordInfo();

    int ReferenceCount;
    bool GeneratedAbbreviations;
};

class GlobalWordSet
{
public:
    static void GlobalInit();

    static char is_part_of_word_[256];
    static char to_lower_[256];
    static const std::string& ComputeUnderscore(const std::string& word);
    static const std::string& ComputeTitleCase(const std::string& word);

    GlobalWordSet();
    ~GlobalWordSet();

    void UpdateWord(const std::string& word, int reference_count_delta);

    void GetPrefixCompletions(
        const std::string& prefix,
        std::set<std::string>* completions);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<std::string>* completions);

private:
    static boost::unordered_map<std::string, std::string> title_case_cache_;
    static boost::unordered_map<std::string, std::string> underscore_cache_;

    boost::mutex mutex_;

    std::map<std::string, WordInfo> words_;
    std::map<std::string, WordInfo>& const_words_;
    boost::unordered_multimap<std::string, std::string> title_cases_;
    boost::unordered_multimap<std::string, std::string> underscores_;
};
