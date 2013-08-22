#include "stdafx.hpp"

#include "TagsCollection.hpp"

TagsCollection* TagsCollection::instance_ = NULL;
#ifdef _WIN32
static std::string kWin32SystemDrive;
#endif

bool TagsCollection::InitStatic() {
  instance_ = new TagsCollection;

#ifdef _WIN32
  char* sys_drive = NULL;
  size_t sys_drive_len = 0;
  errno_t err = ::_dupenv_s(&sys_drive, &sys_drive_len, "SystemDrive");
  if (err)
    return false;
  // apparently this means "C:\0", so 3 bytes
  if (sys_drive_len != 3)
    return false;

  kWin32SystemDrive.resize(2);
  kWin32SystemDrive[0] = sys_drive[0];
  kWin32SystemDrive[1] = ':';

  free(sys_drive);
  sys_drive = NULL;
#endif

  return true;
}

void TagsCollection::GlobalShutdown() {
  delete instance_;
  instance_ = NULL;
}

bool TagsCollection::CreateOrUpdate(std::string tags_pathname,
                                    const std::string& current_directory) {
  // in case the user has set 'tags' to something like:
  // './tags,./TAGS,tags,TAGS'
  // we have to resolve ambiguity
  tags_pathname = ResolveFullPathname(tags_pathname, current_directory);

  if (!Contains(tags_list_, tags_pathname))
    tags_list_[tags_pathname].Init(tags_pathname);
  else
    tags_list_[tags_pathname].Update();

  return true;
}

std::string TagsCollection::VimTaglistFunction(
    const std::string& word,
    const std::vector<std::string>& tags_list,
    const std::string& current_directory)
{
  std::stringstream ss;

  ss << "[";

  foreach (std::string tags, tags_list) {
    tags = ResolveFullPathname(tags, current_directory);
    if (Contains(tags_list_, tags) == false) continue;

    tags_list_[tags].VimTaglistFunction(word, ss);
  }

  ss << "]";

  return ss.str();
}

void TagsCollection::GetPrefixCompletions(
    const std::string& prefix,
    const std::vector<std::string>& tags_list,
    const std::string& current_directory,
    CompleteItemVectorPtr& completions, std::set<std::string> added_words,
    bool terminus_mode)
{
  foreach (std::string tags, tags_list) {
    tags = ResolveFullPathname(tags, current_directory);
    if (Contains(tags_list_, tags) == false)
      continue;

    tags_list_[tags].GetPrefixCompletions(
        prefix,
        completions, added_words,
        terminus_mode);
  }
}

void TagsCollection::GetAbbrCompletions(
    const std::string& prefix,
    const std::vector<std::string>& tags_list,
    const std::string& current_directory,
    CompleteItemVectorPtr& completions, std::set<std::string> added_words,
    bool terminus_mode)
{
  foreach (std::string tags, tags_list) {
    tags = ResolveFullPathname(tags, current_directory);
    if (Contains(tags_list_, tags) == false)
      continue;

    tags_list_[tags].GetAbbrCompletions(
        prefix,
        completions, added_words,
        terminus_mode);
  }
}

std::string TagsCollection::ResolveFullPathname(
    const std::string& tags_pathname,
    const std::string& current_directory)
{
  std::string full_tags_pathname = tags_pathname;

  // TODO(rko): support UNC if someone uses it
#ifdef _WIN32
  NormalizeToWindowsPathSeparators(full_tags_pathname);

  // we have an absolute pathname of the form "C:\X"
  if (full_tags_pathname.size() >= 4 &&
      ::isalpha(static_cast<unsigned char>(full_tags_pathname[0])) &&
      full_tags_pathname[1] == ':' &&
      full_tags_pathname[2] == '\\')
  {
    return full_tags_pathname;
  }

  // we have a UNIX style pathname that assumes someone will
  // prepend the system drive in front of the tags pathname
  if (full_tags_pathname.size() >= 2 &&
      full_tags_pathname[0] == '\\')
  {
    full_tags_pathname = kWin32SystemDrive + full_tags_pathname;
    return full_tags_pathname;
  }

  // we have a relative pathname, prepend current_directory
  full_tags_pathname = current_directory + "\\" + full_tags_pathname;
#else
  NormalizeToUnixPathSeparators(full_tags_pathname);

  // given pathname is absolute
  if (full_tags_pathname[0] == '/')
  {
    return full_tags_pathname;
  }

  // we have a relative pathname, prepend current_directory
  full_tags_pathname = current_directory + "/" + full_tags_pathname;
#endif

  return full_tags_pathname;
}

void TagsCollection::Clear()
{
  tags_list_.clear();
}
