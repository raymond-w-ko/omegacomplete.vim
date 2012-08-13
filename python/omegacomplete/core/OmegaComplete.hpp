#pragma once

#include "Room.hpp"
#include "Buffer.hpp"
#include "GlobalWordSet.hpp"
#include "CompletionSet.hpp"
#include "CompleteItem.hpp"
#include "ClangCompleter.hpp"

class OmegaComplete
:
public boost::noncopyable
{
public:
    static void InitGlobal();
    static OmegaComplete* GetInstance() { return instance_; }
    ~OmegaComplete();

    const std::string Eval(const char* request, const int request_len);

    // Quick Match Keys
    // basically a mapping from result number to keyboard key to press
    // first result  -> 'a'
    // second result -> 's'
    // third result  -> 'd'
    // and etc.
    static std::vector<char> QuickMatchKey;
    static boost::unordered_map<char, unsigned> ReverseQuickMatch;
    static const std::string default_response_;

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

    OmegaComplete();
    void initCommandDispatcher();

    ////////////////////////////////////////////////////////////////////////////
    // Commands
    ////////////////////////////////////////////////////////////////////////////
    std::string cmdCurrentBufferId(StringPtr argument);
    std::string cmdCurrentBufferAbsolutePath(StringPtr argument);
    std::string cmdCurrentLine(StringPtr argument);
    std::string cmdCursorPosition(StringPtr argument);
    std::string cmdBufferContentsFollow(StringPtr argument);
    std::string queueBufferContents(StringPtr argument);
    std::string cmdComplete(StringPtr argument);
    std::string cmdFreeBuffer(StringPtr argument);
    std::string cmdCurrentDirectory(StringPtr argument);
    std::string cmdCurrentTags(StringPtr argument);
    std::string cmdTaglistTags(StringPtr argument);
    std::string cmdVimTaglistFunction(StringPtr argument);
    std::string cmdPrune(StringPtr argument);
    std::string cmdHideTeleprompter(StringPtr argument);
    std::string cmdFlushCaches(StringPtr argument);
    std::string cmdIsCorrectionsOnly(StringPtr argument);

    ////////////////////////////////////////////////////////////////////////////
    // OmegaComplete Core
    ////////////////////////////////////////////////////////////////////////////

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

    ////////////////////////////////////////////////////////////////////////////
    // OmegaComplete Core
    ////////////////////////////////////////////////////////////////////////////
    static OmegaComplete* instance_;

    boost::thread worker_thread_;
    volatile int is_quitting_;

    std::map<String, boost::function<std::string (StringPtr)> >
        command_dispatcher_;

    boost::mutex buffers_mutex_;
    boost::unordered_map<unsigned, Buffer> buffers_;

    boost::mutex job_queue_mutex_;
    std::deque<ParseJob> job_queue_;

    unsigned current_buffer_id_;
    std::string current_buffer_absolute_path_;
    std::string current_line_;
    FileLocation cursor_pos_;
    bool buffer_contents_follow_;
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
typedef boost::shared_ptr<OmegaComplete> SessionPtr;
