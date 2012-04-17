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

Session::~Session()
{
    std::cout << boost::str(boost::format(
        "session %u destroyed\n") % connection_number_);
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
    
    std::string response = "ACK";
    
    if (false) { }
    else if (command == "open_file")
    {
        std::cout << boost::str(boost::format("%s: %s\n") % command % argument);
    }
    else if (command == "current_buffer")
    {
        std::cout << boost::str(boost::format("%s: %s\n") % command % argument);
        
        current_buffer_ = argument;

        // create and initialize buffer object if it doesn't exist
        if (buffers_.find(current_buffer_) == buffers_.end())
        {
            buffers_[current_buffer_].Init(this, argument);
        }
    }
    else if (command == "current_pathname")
    {
        std::cout << boost::str(boost::format("%s: %s\n") % command % argument);

        buffers_[current_buffer_].SetPathname(argument);
    }
    else if (command == "buffer_contents")
    {
        buffers_[current_buffer_].SetContents(argument);
        buffers_[current_buffer_].Parse();

        std::cout << boost::str(boost::format(
            "%s: length = %u\n") % command % argument.length());
    }
    else
    {
        std::cout << boost::str(boost::format(
            "unknown command %s %s") % command % argument);
    }
    
    // write response    
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
