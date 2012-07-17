#pragma once

#include "CompleteItem.hpp"
#include "Tags.hpp"

class TagsSet : public boost::noncopyable
{
public:
    static bool GlobalInit();
    static TagsSet* Instance();
    static void GlobalShutdown();
    std::string ResolveFullPathname(
        const std::string& tags_pathname,
        const std::string& current_directory);

    ~TagsSet();

    bool CreateOrUpdate(
        std::string tags_pathname,
        const std::string& current_directory);

    void GetAllWordsWithPrefix(
        const std::string& prefix,
        const std::vector<std::string>& tags_list,
        const std::string& current_directory,
        std::set<CompleteItem>* results);

    void GetAbbrCompletions(
        const std::string& prefix,
        const std::vector<std::string>& tags_list,
        const std::string& current_directory,
        std::set<CompleteItem>* results);

    std::string VimTaglistFunction(
        const std::string& word,
        const std::vector<std::string>& tags_list,
        const std::string& current_directory);

    void Clear();

private:
    TagsSet();
    static TagsSet* instance_;
#ifdef _WIN32
    static std::string win32_system_drive_;
#endif

    std::map<String, Tags> tags_list_;
};
