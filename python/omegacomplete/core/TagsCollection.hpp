#pragma once

#include "CompleteItem.hpp"
#include "Tags.hpp"
#include "WordCollection.hpp"

class TagsCollection : public boost::noncopyable {
 public:
  static bool InitStatic();
  static void GlobalShutdown();
  static std::string ResolveFullPathname(const std::string& tags_pathname,
                                         const std::string& current_directory);

  TagsCollection() : Words(false) {}
  ~TagsCollection() {}

  bool CreateOrUpdate(std::string tags_pathname,
                      const std::string& current_directory);
  void Clear();

  std::string VimTaglistFunction(const std::string& word,
                                 const std::vector<std::string>& tags_list,
                                 const std::string& current_directory);

  WordCollection Words;

 private:
  std::map<String, TagsPtr> tags_list_;
};
