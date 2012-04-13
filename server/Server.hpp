#pragma once

#include "Session.hpp"

class Server
{
public:
    Server(io_service& io_service, const ip::tcp::endpoint& endpoint);
    ~Server();
    
private:
    void startAccept();
    void handleAccept(
        SessionPtr session,
        const boost::system::error_code& error);

    io_service& io_service_;
    ip::tcp::acceptor acceptor_;
    Room room_;
};

typedef boost::shared_ptr<Server> ServerPtr;
