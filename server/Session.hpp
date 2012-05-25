#pragma once

#include "Room.hpp"
#include "Buffer.hpp"
#include "GlobalWordSet.hpp"

struct ParseJob
{
    unsigned BufferNumber;
    StringPtr Contents;
};

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

    GlobalWordSet WordSet;

private:
    ////////////////////////////////////////////////////////////////////////////
    // boost::asio
    ////////////////////////////////////////////////////////////////////////////
    void handleReadRequest(const boost::system::error_code& error);
    void handleWriteResponse(const boost::system::error_code& error);

    void asyncReadHeader();
    void handleReadHeader(const boost::system::error_code& error);
    void writeResponse(std::string& response);

    ////////////////////////////////////////////////////////////////////////////
    // OmegaComplete Core
    ////////////////////////////////////////////////////////////////////////////
    void processClientMessage();

    void queueParseJob(ParseJob job);
    void workerThreadLoop();

    void calculateCompletionCandidates(
        const std::string& line,
        std::string& result);
    std::string getWordToComplete(const std::string& line);

    ////////////////////////////////////////////////////////////////////////////
    // boost::asio
    ////////////////////////////////////////////////////////////////////////////
    // when creating a session, this determine what the connection number is
    static unsigned int connection_ticket_;

    ip::tcp::socket socket_;
    Room& room_;
    unsigned int connection_number_;
    // used when reading up to a NULL char as a delimiter
    streambuf request_;
    // read based on a header length
    unsigned char request_header_[4];
    std::string request_body_;

    ////////////////////////////////////////////////////////////////////////////
    // OmegaComplete Core
    ////////////////////////////////////////////////////////////////////////////
    boost::thread worker_thread_;
    volatile int is_quitting_;

    boost::mutex buffers_mutex_;
    typedef boost::unordered_map<unsigned, Buffer>::value_type BuffersIterator;
    boost::unordered_map<unsigned, Buffer> buffers_;

    boost::mutex job_queue_mutex_;
    deque<ParseJob> job_queue_;

    unsigned current_buffer_;
    std::string current_line_;
    std::pair<unsigned, unsigned> cursor_pos_;
    vector<std::string> current_tags_;
    vector<std::string> taglist_tags_;

    // quick match keys
    // basically a mapping from result number to keyboard key to press
    // first result  -> 'a'
    // second result -> 's'
    // third result  -> 'd'
    // and etc.
    vector<char> quick_match_key_;
};

typedef boost::shared_ptr<Session> SessionPtr;
