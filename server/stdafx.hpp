#include <stdint.h>

#ifdef _WIN32
    // Debug Mode
    #ifdef _DEBUG
        #define _CRTDBG_MAP_ALLOC
        #include <stdlib.h>
        #include <crtdbg.h>
    // Release Mode
    #else
        // disable checked iterators for performance reasons
        #define _SECURE_SCL 0
    #endif

    #include <winsock2.h>
    #include <windows.h>

    inline
    int64_t to_int64(const FILETIME& ft)
    {
        return static_cast<int64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
    }
#else
        #include <stdlib.h>
        #include <crtdbg.h>
#endif

#include <stdio.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <utility>
#include <fstream>
#include <vector>
#include <deque>
#include <set>
#include <map>

#define BOOST_THREAD_USE_LIB

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread/locks.hpp>
#include <boost/filesystem.hpp>

#define auto BOOST_AUTO
#define foreach BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

#include <clang-c/Index.h>

#include "ustring.hpp"

inline void LogAsioError(
    const boost::system::error_code& error,
    std::string message)
{
    std::string msg = boost::str(boost::format(
        "code: %d, code_str: %s, %s")
        % error.value()
        % error.category().message(error.value())
        % message.c_str());
    std::cerr << msg << std::endl;
}

template <typename Container, typename Item>
bool Contains(const Container& container, const Item& item)
{
    return container.find(item) != container.end();
}

inline void always_assert(bool condition)
{
    if (condition == false)
        throw std::exception();

    return;
}

inline void NormalizeToUnixPathSeparators(std::string& pathname)
{
    boost::replace_all(pathname, "\\", "/");
}

inline void NormalizeToWindowsPathSeparators(std::string& pathname)
{
    boost::replace_all(pathname, "/", "\\");
}

typedef std::string String;
//typedef ustring String;

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

#ifdef _WIN32
    #ifdef _DEBUG
        #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
        #define new DEBUG_NEW
    #endif
#endif
