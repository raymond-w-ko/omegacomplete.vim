#pragma once

#include "Tags.hpp"

class TagsSet
{
public:
    static bool GlobalInit();
    static TagsSet* Instance();
    static void GlobalShutdown();
    ~TagsSet();

    bool CreateOrUpdate(const std::string tags_path);

    void GetAllWordsWithPrefix(
        const std::string& prefix,
        const std::vector<std::string>& tags_list,
        std::set<std::string>* results);

    void GetAbbrCompletions(
        const std::string& prefix,
        const std::vector<std::string>& tags_list,
        std::set<std::string>* results);

    std::string VimTaglistFunction(
        const std::string& word,
        const std::vector<std::string>& tags_list);

private:
    TagsSet();
    static TagsSet* instance_;

    boost::unordered_map<std::string, Tags> tags_list_;
};
