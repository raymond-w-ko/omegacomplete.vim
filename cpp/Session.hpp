#pragma once

#include "Room.hpp"
#include "Buffer.hpp"
#include "GlobalWordSet.hpp"
#include "CompletionSet.hpp"
#include "CompleteItem.hpp"
#include "ClangCompleter.hpp"

class Session
:
public boost::noncopyable,
public Participant,
public boost::enable_shared_from_this<Session>
{
public:
    static void GlobalInit();

    ////////////////////////////////////////////////////////////////////////////
    // boost::asio
    ////////////////////////////////////////////////////////////////////////////
    // when creating a session, this determine what the connection number is
    static unsigned int connection_ticket_;
    // quick match keys
    // basically a mapping from result number to keyboard key to press
    // first result  -> 'a'
    // second result -> 's'
    // third result  -> 'd'
    // and etc.
    static std::vector<char> QuickMatchKey;
    static boost::unordered_map<char, unsigned> ReverseQuickMatch;

    Session(boost::asio::io_service& io_service, Room& room);
    ~Session();

    boost::asio::ip::tcp::socket& Socket()
    {
        return socket_;
    }

    void Start();

    GlobalWordSet WordSet;

private:
    struct ParseJob
    {
        ParseJob(unsigned buffer_number, const StringPtr& contents)
        :
        BufferNumber(buffer_number),
        Contents(contents)
        { }

        ParseJob(const ParseJob& other)
        :
        BufferNumber(other.BufferNumber),
        Contents(other.Contents)
        { }

        unsigned BufferNumber;
        StringPtr Contents;
    };

    ////////////////////////////////////////////////////////////////////////////
    // boost::asio
    ////////////////////////////////////////////////////////////////////////////
    void handleReadRequest(const boost::system::error_code& error);
    void handleWriteResponse(const boost::system::error_code& error);

    void asyncReadHeader();
    void handleReadHeader(const boost::system::error_code& error);
    void writeResponse(const std::string& response);

    void cmdCurrentBufferId(StringPtr argument);
    void cmdCurrentBufferAbsolutePath(StringPtr argument);
    void cmdCurrentLine(StringPtr argument);
    void cmdCursorPosition(StringPtr argument);
    void cmdBufferContents(StringPtr argument);
    void cmdComplete(StringPtr argument);
    void cmdFreeBuffer(StringPtr argument);
    void cmdCurrentDirectory(StringPtr argument);
    void cmdCurrentTags(StringPtr argument);
    void cmdTaglistTags(StringPtr argument);
    void cmdVimTaglistFunction(StringPtr argument);
    void cmdPrune(StringPtr argument);
    void cmdHideTeleprompter(StringPtr argument);
    void cmdFlushCaches(StringPtr argument);
    void cmdIsCorrectionsOnly(StringPtr argument);

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
    void genericKeywordCompletion(
        const std::string& line,
        std::string& result);

    void addWordsToResults(
        const std::set<std::string>& words,
        std::vector<StringPair>& result,
        unsigned& num_completions_added,
        boost::unordered_set<std::string>& added_words);

    bool shouldEnableDisambiguateMode(const std::string& word);
    bool shouldEnableTerminusMode(const std::string& word);
    void fillCompletionSet(
        const std::string& prefix_to_complete,
        CompletionSet& completion_set,
        const std::vector<CompleteItem>* banned_words = NULL);

    boost::asio::ip::tcp::socket socket_;
    Room& room_;
    unsigned int connection_number_;
    // used when reading up to a NULL char as a delimiter
    boost::asio::streambuf request_;
    // read based on a header length
    unsigned char request_header_[4];
    std::string request_body_;
    std::map<String, boost::function<void (StringPtr)> > command_dispatcher_;

    ////////////////////////////////////////////////////////////////////////////
    // OmegaComplete Core
    ////////////////////////////////////////////////////////////////////////////
    boost::thread worker_thread_;
    volatile int is_quitting_;

    boost::mutex buffers_mutex_;
    boost::unordered_map<unsigned, Buffer> buffers_;

    boost::mutex job_queue_mutex_;
    std::deque<ParseJob> job_queue_;

    unsigned current_buffer_id_;
    std::string current_buffer_absolute_path_;
    std::string current_line_;
    FileLocation cursor_pos_;
    StringPtr current_contents_;

    std::string current_directory_;
    std::vector<std::string> current_tags_;
    std::vector<std::string> taglist_tags_;

    std::vector<std::string> prev_input_;

    bool is_corrections_only_;

#ifdef ENABLE_CLANG_COMPLETION
    ClangCompleter clang_;
#endif
};
typedef boost::shared_ptr<Session> SessionPtr;