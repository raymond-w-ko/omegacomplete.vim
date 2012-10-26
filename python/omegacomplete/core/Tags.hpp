#pragma once

#include "CompleteItem.hpp"
#include "AbbreviationInfo.hpp"

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

    void GetPrefixCompletions(
        const std::string& input,
        CompleteItemVectorPtr& completions, std::set<std::string>& added_words,
        bool terminus_mode);

    void GetAbbrCompletions(
        const std::string& input,
        CompleteItemVectorPtr& completions, std::set<std::string>& added_words,
        bool terminus_mode);

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

    bool win32_CheckIfModified();

    std::string pathname_;
    int64_t last_write_time_;
    double last_tick_count_;
    std::string parent_directory_;

    typedef
        std::multimap<String, String>::iterator
        tags_iterator;
    std::multimap<String, String> tags_;
    std::map<String, std::set<AbbreviationInfo> > abbreviations_;
};
