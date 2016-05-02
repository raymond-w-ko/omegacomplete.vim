#pragma once
#include <stdlib.h>
#include <stdint.h>

#ifdef __CYGWIN__
#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP IPV6_ADD_MEMBERSHIP
#endif
#ifndef IPV6_LEAVE_GROUP
#define IPV6_LEAVE_GROUP IPV6_DROP_MEMBERSHIP
#endif
#endif

// must be included before <windows.h> or an error occurs
#ifdef _WIN32
// at least 0x600 needed for GetTickCount64()
#define _WIN32_WINNT 0x0600
#endif
#include <boost/asio.hpp>

#ifdef _WIN32
#ifndef _DEBUG
// disable checked iterators for performance
#define _SECURE_SCL 0
#endif

#include <windows.h>

inline int64_t to_int64(const FILETIME& ft) {
  return static_cast<int64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
}
#else
#include <sys/types.h>
#include <sys/stat.h>
// Mac OS X specific includes
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#endif

#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <fstream>

#include <boost/exception/all.hpp>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/atomic.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

#define AUTO BOOST_AUTO
#define foreach BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

template <typename Container, typename Item>
bool Contains(const Container& container, const Item& item) {
  return container.find(item) != container.end();
}

inline void NormalizeToUnixPathSeparators(std::string& pathname) {
  boost::replace_all(pathname, "\\", "/");
}

inline void NormalizeToWindowsPathSeparators(std::string& pathname) {
  boost::replace_all(pathname, "/", "\\");
}

struct FileLocation {
  unsigned Line;
  unsigned Column;
};

typedef unsigned char uchar;

typedef std::string String;
typedef boost::shared_ptr<String> StringPtr;

typedef std::set<String> StringSet;
typedef boost::shared_ptr<StringSet> StringSetPtr;

typedef boost::unordered_set<String> UnorderedStringSet;
typedef boost::shared_ptr<UnorderedStringSet> UnorderedStringSetPtr;

typedef boost::unordered_map<String, int> UnorderedStringIntMap;
typedef boost::shared_ptr<UnorderedStringIntMap> UnorderedStringIntMapPtr;
typedef std::map<String, int> StringIntMap;
typedef std::map<String, int>::iterator StringIntMapIter;
typedef std::map<String, int>::const_iterator StringIntMapConstIter;
typedef boost::shared_ptr<StringIntMap> StringIntMapPtr;

typedef std::vector<String> StringVector;
typedef boost::shared_ptr<StringVector> StringVectorPtr;
typedef std::pair<String, String> StringPair;

typedef std::pair<unsigned, String> UnsignedStringPair;
typedef std::vector<UnsignedStringPair> UnsignedStringPairVector;
typedef boost::shared_ptr<UnsignedStringPairVector> UnsignedStringPairVectorPtr;
