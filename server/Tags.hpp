#pragma once

#include "CompleteItem.hpp"

struct TagInfo
{
    std::string Location;
    std::string Ex;
    typedef
        std::map<String, String>::iterator
        InfoIterator;
    std::map<String, String> Info;
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
        std::set<CompleteItem>* results);

    void GetAbbrCompletions(
        const std::string& prefix,
        std::set<CompleteItem>* results);

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

    std::string pathname_;
    int64_t last_write_time_;
    std::string parent_directory_;

    typedef
        std::multimap<String, String>::iterator
        tags_iterator;
    std::multimap<String, String> tags_;
    std::multimap<String, const String*> abbreviations_;
};
