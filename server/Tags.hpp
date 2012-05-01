#pragma once

struct TagInfo
{
    std::string Tag;
    std::string Location;
    std::string Ex;
    std::map<std::string, std::string> Info;
};

class Tags
{
public:
    Tags();
    bool Init(const std::string& pathname);
    ~Tags();

    void Update();

    void GetAllWordsWithPrefix(
        const std::string& prefix,
        std::set<std::string>* results);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<std::string>* results);

private:
    void reparse();

    char is_part_of_word_[256];
    char to_lower_[256];

    std::string pathname_;
    std::thread thread_;
    std::mutex mutex_;

    std::multimap<std::string, TagInfo> tags_;
    boost::unordered_set<std::string> words_;
    boost::unordered_multimap<std::string, const std::string*> title_cases_;
    boost::unordered_multimap<std::string, const std::string*> underscores_;
};
