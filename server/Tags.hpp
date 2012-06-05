#pragma once

struct TagInfo
{
    std::string Tag;
    std::string Location;
    std::string Ex;
    typedef
        boost::unordered_map<std::string, std::string>::value_type
        InfoIterator;
    boost::unordered_map<std::string, std::string> Info;
};

class Tags
{
public:
    Tags();
    bool Init(const std::string& pathname);
    ~Tags();

    void Update();

    void VimTaglistFunction(
        const std::string& expr,
        const std::vector<std::string>& tags_list,
        std::stringstream& ss);

    void GetAllWordsWithPrefix(
        const std::string& prefix,
        std::set<std::string>* results);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<std::string>* results);

    void VimTaglistFunction(
        const std::string& word,
        std::stringstream& ss);

private:
    bool calculateParentDirectory();
    void reparse();

    char is_part_of_word_[256];
    char to_lower_[256];

    std::string pathname_;
    __int64 last_write_time_;
    std::string parent_directory_;
    boost::thread thread_;
    boost::mutex mutex_;

    boost::unordered_multimap<std::string, TagInfo> tags_;
    boost::unordered_set<std::string> words_;
    boost::unordered_multimap<std::string, const std::string*> title_cases_;
    boost::unordered_multimap<std::string, const std::string*> underscores_;
    StringToStringVectorUnorderedMap title_case_cache_;
    StringToStringVectorUnorderedMap underscore_cache_;
};
