#include "stdafx.hpp"

#include "Server.hpp"
#include "Tags.hpp"
#include "TagsSet.hpp"

const unsigned short PORT = 31337;

int main(int argc, char* argv[])
{
#ifdef WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

    // intialize the tags container
    if (TagsSet::GlobalInit() == false) return 1;

    io_service io_service;
    std::vector<ServerPtr> servers;
    ip::tcp::endpoint endpoint(ip::tcp::v4(), PORT);
    ServerPtr server(new Server(io_service, endpoint));
    servers.push_back(server);

    io_service.run();

    return 0;
}
