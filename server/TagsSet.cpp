#include "stdafx.hpp"

#include "TagsSet.hpp"

TagsSet* TagsSet::instance_ = NULL;

bool TagsSet::GlobalInit()
{
    instance_ = new TagsSet;

    if (instance_ == NULL)
        return false;
    else
        return true;
}

TagsSet* TagsSet::Instance()
{
    return instance_;
}

void TagsSet::GlobalShutdown()
{
    delete instance_; instance_ = NULL;
}

bool TagsSet::CreateOrUpdate(const std::string tags_path)
{
    if (Contains(tags_list_, tags_path) == false)
    {
        tags_list_[tags_path].Init(tags_path);
    }
    else
    {
        tags_list_[tags_path].Update();
    }
    return true;
}

TagsSet::TagsSet()
{
    ;
}

TagsSet::~TagsSet()
{
    ;
}

std::string TagsSet::VimTaglistFunction(
    const std::string& word,
    const std::vector<std::string>& tags_list)
{
    std::stringstream ss;

    ss << "[";

    foreach (const std::string& requested_tag, tags_list)
    {
        if (Contains(tags_list_, requested_tag) == false) continue;

        Tags& tags = tags_list_[requested_tag];

        tags.VimTaglistFunction(word, ss);
    }

    ss << "]";

    return ss.str();
}

void TagsSet::GetAllWordsWithPrefix(
    const std::string& prefix,
    const std::vector<std::string>& tags_list,
    std::set<std::string>* results)
{
    foreach (const std::string& tags, tags_list)
    {
        if (Contains(tags_list_, tags) == false) continue;
        tags_list_[tags].GetAllWordsWithPrefix(prefix, results);
    }
}

void TagsSet::GetAbbrCompletions(
    const std::string& prefix,
    const std::vector<std::string>& tags_list,
    std::set<std::string>* results)
{
    foreach (const std::string& tags, tags_list)
    {
        if (Contains(tags_list_, tags) == false) continue;
        tags_list_[tags].GetAbbrCompletions(prefix, results);
    }
}
