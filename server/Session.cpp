#include "stdafx.hpp"

#include "Session.hpp"

Session::Session(io_service& io_service, Room& room)
:
socket_(io_service),
room_(room)
{
    ;
}

void Session::Start()
{
    room_.Join(shared_from_this());
    
    asyncReadUntilNullChar();
}

void Session::asyncReadUntilNullChar()
{
    std::string null_delimiter(1, '\0');

    async_read_until(
        socket_,
        request_,
        null_delimiter,
        boost::bind(
            &Session::handleReadRequest,
            this,
            placeholders::error));
}

void Session::handleReadRequest(const boost::system::error_code& error)
{
    std::istream is(&request_);
    std::string request;
    is >> request;
    
    // process request

    // write response    
    std::string response = "ACK";
    async_write(
        socket_,
        buffer(&response[0], response.size()),
        boost::bind(
            &Session::handleWriteResponse,
            this,
            placeholders::error));
}

void Session::handleWriteResponse(const boost::system::error_code& error)
{
    ;
}
