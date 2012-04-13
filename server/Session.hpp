#pragma once

#include "Room.hpp"

class Session
:
public Participant,
public boost::enable_shared_from_this<Session>
{
public:
    Session(io_service& io_service, Room& room);
    
    ip::tcp::socket& Socket()
    {
        return socket_;
    }
    
    void Start();

private:
    void handleReadRequest(const boost::system::error_code& error);
    void handleWriteResponse(const boost::system::error_code& error);

    void asyncReadUntilNullChar();

    ip::tcp::socket socket_;
    Room& room_;
    
    streambuf request_;
};

typedef boost::shared_ptr<Session> SessionPtr;
