#define BOOST_THREAD_USE_LIB

#ifdef WIN32
    #define _WIN32_WINNT 0x0501
    #include <winsock2.h>
    #include <windows.h>

    #ifndef _DEBUG
        #define _SECURE_SCL 0
    #endif
#endif

#include <cstdio>
#include <cstdlib>

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
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/container/flat_map.hpp>

#define auto BOOST_AUTO
#define foreach BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

using namespace boost::asio;

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

inline bool IsPartOfWord(char c)
{
    if ( (('a' <= c) && (c <= 'z')) ||
         (('A' <= c) && (c <= 'Z')) ||
         (c == '_') ||
         (('0' <= c) && (c <= '9')) )
    {
        return true;
    }

    return false;
}

inline bool IsUpper(char c)
{
    if (('A' <= c) && (c <= 'Z')) return true;
    return false;
}

template <typename Container, typename Item>
bool Contains(const Container& container, const Item& item)
{
    return container.find(item) != container.end();
}

inline
__int64 to_int64(const FILETIME& ft)
{
    return static_cast<__int64>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
}

inline void always_assert(bool condition)
{
    if (condition == false)
        throw std::exception();

    return;
}

typedef boost::shared_ptr<std::string> StringPtr;
typedef boost::unordered_set<std::string> UnorderedStringSet;
typedef boost::shared_ptr<UnorderedStringSet> UnorderedStringSetPtr;
typedef std::vector<std::string> StringVector;
typedef
    boost::unordered_map<std::string, StringVector>
    StringToStringVectorUnorderedMap;
