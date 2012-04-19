#pragma once

#include "Room.hpp"
#include "Buffer.hpp"

class Session
:
public Participant,
public boost::enable_shared_from_this<Session>
{
public:
    Session(io_service& io_service, Room& room);
    ~Session();
    
    ip::tcp::socket& Socket()
    {
        return socket_;
    }
    
    void Start();
    
private:
    void handleReadRequest(const boost::system::error_code& error);
    void handleWriteResponse(const boost::system::error_code& error);
    void asyncReadUntilNullChar();
	
	std::string calculateCompletionCandidates(const std::string& line);
	std::string getWordToComplete(const std::string& line);

    // connection related variables
    static unsigned int connection_ticket_;

    ip::tcp::socket socket_;
    Room& room_;
    unsigned int connection_number_;
    streambuf request_;
    
    // actually useful variables
    boost::unordered_map<std::string, Buffer> buffers_;
    std::string current_buffer_;
	
	std::string current_line_;
	std::pair<int, int> cursor_pos_;
};

typedef boost::shared_ptr<Session> SessionPtr;
