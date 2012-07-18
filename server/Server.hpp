#pragma once

#include "Session.hpp"

class Server
{
public:
    Server( 
        boost::asio::io_service& io_service,
        const boost::asio::ip::tcp::endpoint& endpoint);
    ~Server();
    
private:
    void startAccept();
    void handleAccept(
        SessionPtr session,
        const boost::system::error_code& error);

    boost::asio::io_service& io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    Room room_;
};

typedef boost::shared_ptr<Server> ServerPtr;
