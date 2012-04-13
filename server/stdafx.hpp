#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <set>
#include <vector>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#define foreach BOOST_FOREACH
#define auto BOOST_AUTO

using namespace boost::asio;
