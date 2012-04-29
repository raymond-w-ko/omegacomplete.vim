#define _WIN32_WINNT 0x0501
#define BOOST_THREAD_USE_LIB

#include <winsock2.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <set>
#include <vector>
#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <memory>
#include <future>
#include <utility>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

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

using namespace boost::asio;

inline void LogAsioError(const boost::system::error_code& error, std::string message)
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

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename Container, typename Item>
bool Contains(const Container& container, const Item& item)
{
    return container.find(item) != container.end();
}

inline void CalculateTitlecaseAndUnderscore(
    const std::string& word,
    const char* to_lower,
    std::string& title_case, std::string& underscore)
{
    const size_t word_length = word.length();
    for (size_t ii = 0; ii < word_length; ++ii)
    {
        char c = word[ii];

        if (ii == 0)
        {
            title_case += to_lower[c];
            underscore += to_lower[c];

            continue;
        }

        if (IsUpper(c))
        {
            title_case += to_lower[c];
        }

        if (word[ii] == '_')
        {
            if (ii < (word_length - 1) && word[ii + 1] != '_')
                underscore += to_lower[word[ii + 1]];
        }
    }
}
