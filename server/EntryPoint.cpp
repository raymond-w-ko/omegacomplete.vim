#include "stdafx.hpp"

#include "EntryPoint.hpp"
#include "Server.hpp"
#include "Tags.hpp"
#include "TagsSet.hpp"
#include "GlobalWordSet.hpp"
#include "TestCases.hpp"
#include "LookupTable.hpp"
#include "Teleprompter.hpp"

static const char* ADDRESS = "127.0.0.1";
static const unsigned short PORT = 31337;

int main(int argc, char* argv[])
{
#ifndef _DEBUG
    TestCases tc;
#endif

    // static intializers
    LookupTable::GlobalInit();
    if (TagsSet::GlobalInit() == false)
        return 1;
    GlobalWordSet::GlobalInit();
    Session::GlobalInit();

#ifdef _WIN32
#ifdef TELEPROMPTER
    Teleprompter::GlobalInit();
#endif
#endif

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
