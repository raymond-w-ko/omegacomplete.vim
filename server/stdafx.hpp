#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <set>
#include <vector>
#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <future>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/utility/result_of.hpp>

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
