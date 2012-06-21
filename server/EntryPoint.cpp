#include "stdafx.hpp"

#include "EntryPoint.hpp"
#include "Server.hpp"
#include "Tags.hpp"
#include "TagsSet.hpp"
#include "GlobalWordSet.hpp"
#include "TestCases.hpp"

static const char* ADDRESS = "127.0.0.1";
static const unsigned short PORT = 31337;

int main(int argc, char* argv[])
{
    TestCases tc;

#ifdef _WIN32
    //SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#else
    // TODO(rko): set priority on UNIX OSes
#endif

    // static intializers
    if (TagsSet::GlobalInit() == false) return 1;
    GlobalWordSet::GlobalInit();
    Buffer::GlobalInit();

    // where everything starts
    io_service io_service;
    std::vector<ServerPtr> servers;
    ip::tcp::endpoint endpoint(
        ip::address::from_string(ADDRESS),
        PORT);
    std::cout << boost::str(boost::format(
        "listening on %s:%hu\n")
        % ADDRESS % PORT);
    ServerPtr server(new Server(io_service, endpoint));
    servers.push_back(server);

    io_service.run();

    return 0;
}
