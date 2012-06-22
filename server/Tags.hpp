#pragma once

struct TagInfo
{
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
    Tags(const Tags& other);
    ~Tags();
    bool Init(const std::string& pathname);

    void Update();

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
    bool calculateTagInfo(
        const std::string& line,
        std::string& tag_name,
        TagInfo& tag_info);


    static boost::unordered_map<std::string, StringVector> title_case_cache_;
    static boost::unordered_map<std::string, StringVector> underscore_cache_;

    std::string pathname_;
    int64_t last_write_time_;
    std::string parent_directory_;

    typedef
        boost::unordered_multimap<std::string, std::string>::iterator
        tags_iterator;
    boost::unordered_multimap<std::string, std::string> tags_;
    boost::unordered_multimap<std::string, const std::string*> title_cases_;
    boost::unordered_multimap<std::string, const std::string*> underscores_;
};
