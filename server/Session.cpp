#include "stdafx.hpp"

#include "Session.hpp"

unsigned int Session::connection_ticket_ = 0;

Session::Session(io_service& io_service, Room& room)
:
socket_(io_service),
room_(room),
connection_number_(connection_ticket_++)
{
    ;
}

void Session::Start()
{
    std::cout << "session started, connection number: " << connection_number_ << "\n";
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
    if (error)
    {
        room_.Leave(shared_from_this());
        
        LogAsioError(error, "failed to handleReadRequest()");
        return;
    }

    std::ostringstream ss;
    ss << &request_;
    std::string request = ss.str();
    
    // find first space
    int index = -1;
    for (int ii = 0; ii < request.size(); ++ii)
    {
        if (request[ii] == ' ')
        {
            index = ii;
            break;
        }
    }
    
    if (index == -1) throw std::exception();
    
    std::string command(request.begin(), request.begin() + index);
    std::string argument(request.begin() + index + 1, request.end());
    
    boost::regex re("\\W+", boost::regex::normal | boost::regex::icase);
    boost::sregex_token_iterator ii(argument.begin(), argument.end(), re, -1);
    boost::sregex_token_iterator end;
    
    // process request
    std::cout << command << "\n";
    
    while (ii != end)
    {
        ii++;
    }

    // write response    
    std::string response = "ACK";
    response.resize(response.size() + 1, '\0');
    async_write(
        socket_,
        buffer(&response[0], response.size()),
        boost::bind(
            &Session::handleWriteResponse,
            this,
            placeholders::error));
            
    asyncReadUntilNullChar();
}

void Session::handleWriteResponse(const boost::system::error_code& error)
{
    if (error)
    {
        room_.Leave(shared_from_this());
        
        LogAsioError(error, "failed in handleWriteResponse()");
        return;
    }
}
