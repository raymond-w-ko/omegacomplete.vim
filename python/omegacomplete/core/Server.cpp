#include "stdafx.hpp"

#include "Server.hpp"

Server::~Server()
{
    ;
}

Server::Server(
    boost::asio::io_service& io_service,
    const boost::asio::ip::tcp::endpoint& endpoint)
:
io_service_(io_service),
acceptor_(io_service, endpoint)
{
    startAccept();
}

void Server::startAccept()
{
    SessionPtr new_session(new Session(io_service_, room_));
    acceptor_.async_accept(
        new_session->Socket(),
        boost::bind(
            &Server::handleAccept,
            this,
            new_session,
            boost::asio::placeholders::error));
}

void Server::handleAccept(
    SessionPtr session,
    const boost::system::error_code& error)
{
    if (!error)
    {
        session->Start();
    }
    
    startAccept();
}
