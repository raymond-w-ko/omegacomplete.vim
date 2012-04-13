#include "stdafx.hpp"

#include "Server.hpp"

const unsigned short PORT = 31337;

int main(int argc, char* argv[])
{
    try
    {
        io_service io_service;
        
        std::vector<ServerPtr> servers;

        ip::tcp::endpoint endpoint(ip::tcp::v4(), PORT);
        ServerPtr server(new Server(io_service, endpoint));
        servers.push_back(server);
        
        io_service.run();
    }
    catch(std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
