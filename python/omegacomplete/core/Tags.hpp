#pragma once

#include "CompleteItem.hpp"
#include "AbbreviationInfo.hpp"

class TagsCollection;

struct TagInfo {
  std::string Location;
  std::string Ex;
  typedef std::map<String, String>::iterator InfoIterator;
  std::map<String, String> Info;
};

class Tags {
 public:
  Tags(TagsCollection* parent, const std::string& pathname);
  ~Tags() {}

  void Update();

  void VimTaglistFunction(const std::string& word, std::stringstream& ss);

 private:
  void updateWordRefCount(const std::multimap<String, String>& tags, int sign);

  bool calculateParentDirectory();
  void reparse();
  bool calculateTagInfo(const std::string& line, std::string& tag_name,
                        TagInfo& tag_info);

  bool win32_CheckIfModified();
  bool unix_CheckIfModified();

  TagsCollection* parent_;
  std::string pathname_;
  int64_t last_write_time_;
  double last_tick_count_;
  std::string parent_directory_;

  std::multimap<String, String> tags_;
};
typedef boost::shared_ptr<Tags> TagsPtr;
