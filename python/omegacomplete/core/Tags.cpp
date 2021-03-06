#include "stdafx.hpp"

#include "Tags.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "CompletionPriorities.hpp"

#ifndef _WIN32
static inline void UnixGetClockTime(struct timespec& ts) {
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
}
#endif

Tags::Tags(TagsCollection* parent, const std::string& pathname)
    : last_write_time_(0), last_tick_count_(-1.0) {
  parent_ = parent;
  pathname_ = pathname;

  if (!calculateParentDirectory()) {
    // std::cout << "couldn't calculate parent directory for tags file, not
    // parsing"
    //<< std::endl;
    // std::cout << pathname_ << std::endl;
    return;
  }
  this->Update();
}

bool Tags::calculateParentDirectory() {
  std::string directory_separator;

#ifdef _WIN32
  directory_separator = "\\";
#else
  directory_separator = "/";
#endif

  size_t pos = pathname_.rfind(directory_separator);
  // error out because we can't find the last directory separator
  if (pos == std::string::npos) {
    // std::cout << "coulnd't find last directory separator" << std::endl;
    // std::cout << pathname_ << std::endl;
    return false;
  }
  // there is nothing after the last directory separator
  if (pos >= (pathname_.size() - 1)) {
    // std::cout << "there is nothing after the last directory separator" <<
    // std::endl;
    // std::cout << pathname_ << std::endl;
    return false;
  }

  parent_directory_ = std::string(pathname_.begin(), pathname_.begin() + pos);

  return true;
}

void Tags::updateWordRefCount(const std::multimap<String, String>& tags,
                              int sign) {
  UnorderedStringIntMap deltas;

  std::string prev_word;
  AUTO(iter, tags.begin());
  for (; iter != tags.end(); ++iter) {
    const String& word = iter->first;
    if (word == prev_word) continue;

    deltas[word] += sign * 1;

    prev_word = word;
  }

  parent_->Words.UpdateWords(&deltas);
}

void Tags::reparse() {
  if (parent_directory_.empty()) return;

  updateWordRefCount(tags_, -1);
  tags_.clear();

  std::ifstream file(pathname_.c_str());
  unsigned line_num = 0;

  for (std::string line; std::getline(file, line).good(); ++line_num) {
    // the first 6 lines contain a description of the tags file, which we don't
    // need
    if (line_num < 6) continue;

    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of("\t"),
                 boost::token_compress_off);
    if (tokens.size() < 3) {
      // std::cout << "invalid tag line detected!" << std::endl;
      return;
    }

    const std::string tag_name = tokens[0];
    tags_.insert(make_pair(tag_name, line));
  }

  // re-add all unique tags in this tags file
  updateWordRefCount(tags_, 1);
}

void Tags::VimTaglistFunction(const std::string& word, std::stringstream& ss) {
  AUTO(bounds, tags_.equal_range(word));
  AUTO(&iter, bounds.first);
  for (; iter != bounds.second; ++iter) {
    const std::string& tag_name = iter->first;
    const std::string& line = iter->second;
    std::string dummy;
    TagInfo tag_info;
    if (!calculateTagInfo(line, dummy, tag_info)) continue;

    ss << "{";

    ss << boost::str(boost::format("'name':'%s','filename':'%s','cmd':'%s',") %
                     tag_name % tag_info.Location % tag_info.Ex);
    for (TagInfo::InfoIterator pair = tag_info.Info.begin();
         pair != tag_info.Info.end(); ++pair) {
      ss << boost::str(boost::format("'%s':'%s',") % pair->first %
                       pair->second);
    }

    ss << "},";
  }
}

void Tags::Update() {
  const double check_time = 60.0 * 1000.0;
  bool reparse_needed = false;

#ifdef _WIN32
  if (last_tick_count_ == -1) {
    reparse_needed = true;
    last_tick_count_ = static_cast<double>(::GetTickCount64());
  } else {
    double new_count = static_cast<double>(::GetTickCount64());
    if (::fabs(new_count - last_tick_count_) > check_time) {
      reparse_needed = win32_CheckIfModified();
      last_tick_count_ = new_count;
    } else {
      reparse_needed = false;
    }
  }
#else
  timespec ts;
  UnixGetClockTime(ts);
  if (last_tick_count_ == -1) {
    reparse_needed = true;
    last_tick_count_ = ts.tv_sec * 1000.0;
  } else {
    double new_count = ts.tv_sec * 1000.0;
    if (::fabs(new_count - last_tick_count_) > check_time) {
      reparse_needed = unix_CheckIfModified();
      last_tick_count_ = new_count;
    } else {
      reparse_needed = false;
    }
  }
#endif

  if (reparse_needed) reparse();
}

bool Tags::calculateTagInfo(const std::string& line, std::string& tag_name,
                            TagInfo& tag_info) {
  std::vector<std::string> tokens;
  boost::split(tokens, line, boost::is_any_of("\t"), boost::token_compress_off);

  if (tokens.size() < 3) {
    // std::cout << "invalid tag line detected!" << std::endl;
    return false;
  }

  tag_name = tokens[0];
  tag_info.Location = tokens[1];
  // normalize Windows pathnames to UNIX format
  boost::replace_all(tag_info.Location, "\\", "/");
  // if there is a dot slash in front, replace with location of tags
  if (tag_info.Location.length() > 1 && tag_info.Location[0] == '.')
    boost::replace_first(tag_info.Location, ".", parent_directory_);
  // if there is no slash at all then it is in the current directory of the tags
  if (tag_info.Location.find("/") == std::string::npos)
    tag_info.Location = parent_directory_ + "/" + tag_info.Location;
#ifdef _WIN32
  NormalizeToWindowsPathSeparators(tag_info.Location);
#else
  NormalizeToUnixPathSeparators(tag_info.Location);
#endif

  size_t index = 2;
  std::string ex;
  for (; index < tokens.size(); ++index) {
    std::string token = tokens[index];
    if (token.empty()) token = "\t";

    ex += token;

    if (boost::ends_with(token, ";\"")) break;
  }
  if (!boost::ends_with(ex, "\"")) {
    // std::cout << "Ex didn't end with ;\"" << std::endl;
    return false;
  }

  tag_info.Ex = ex;

  // increment to next token, which should be the more info fields
  index++;

  for (; index < tokens.size(); ++index) {
    std::string token = tokens[index];
    size_t colon = token.find(":");
    if (colon == std::string::npos) {
      // std::cout << "expected key value pair, but could not find ':'"
      //<< std::endl;
      // std::cout << line << std::endl;
      continue;
    }

    std::string key = std::string(token.begin(), token.begin() + colon);
    std::string value = std::string(token.begin() + colon + 1, token.end());
    tag_info.Info[key] = value;
  }

  // calculate prefix from Ex command
  // this can usually be used as the return type of the tag
  size_t header_index = tag_info.Ex.find("/^");
  size_t footer_index = tag_info.Ex.find(tag_name);
  if ((header_index != std::string::npos) &&
      (footer_index != std::string::npos)) {
    header_index += 2;
    std::string tag_prefix =
        tag_info.Ex.substr(header_index, footer_index - header_index);
    // search back until we find a space to handle complex
    // return types like
    // std::map<int, int> FunctionName
    // and
    // void Class::FunctionName
    footer_index = tag_prefix.rfind(" ");
    if (footer_index != std::string::npos) {
      tag_prefix = tag_prefix.substr(0, footer_index);
    }
    boost::trim(tag_prefix);
    tag_info.Info["prefix"] = tag_prefix;
  }

  return true;
}

bool Tags::win32_CheckIfModified() {
#ifdef _WIN32
  bool reparse_needed = false;

  HANDLE hFile =
      ::CreateFile(pathname_.c_str(), GENERIC_READ,
                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) return false;

  FILETIME ft_last_write_time;
  ::GetFileTime(hFile, NULL, NULL, &ft_last_write_time);

  __int64 last_write_time = to_int64(ft_last_write_time);
  if (last_write_time > last_write_time_) {
    reparse_needed = true;
    last_write_time_ = last_write_time;
  }

  CloseHandle(hFile);
  hFile = INVALID_HANDLE_VALUE;

  return reparse_needed;
#else
  return false;
#endif
}

bool Tags::unix_CheckIfModified() {
#ifdef _WIN32
  return false;
#else
  bool reparse_needed = false;
  struct stat statbuf;
  if (::stat(pathname_.c_str(), &statbuf) == -1) {
    return false;
  }

  int64_t last_write_time = statbuf.st_mtime;
  if (last_write_time > last_write_time_) {
    reparse_needed = true;
    last_write_time_ = last_write_time;
  }

  return reparse_needed;
#endif
}
