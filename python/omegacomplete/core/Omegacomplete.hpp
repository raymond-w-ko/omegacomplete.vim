#pragma once

#include "Buffer.hpp"
#include "GlobalWordSet.hpp"
#include "CompleteItem.hpp"

class Omegacomplete : public boost::noncopyable {
public:
    static void InitStatic();
    static Omegacomplete* GetInstance() { return instance_; }
    ~Omegacomplete();

    const std::string Eval(const char* request, const int request_len);

    static const std::string default_response_;

    GlobalWordSet WordSet;

private:
    struct ParseJob
    {
        ParseJob()
        :
        BufferNumber(0)
        {}

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

    Omegacomplete();
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
    std::string cmdFlushCaches(StringPtr argument);
    std::string cmdIsCorrectionsOnly(StringPtr argument);
    std::string cmdPruneBuffers(StringPtr argument);
    std::string cmdShouldAutocomplete(StringPtr argument);
    std::string cmdConfig(StringPtr argument);
    std::string cmdGetAutocomplete(StringPtr argument);

    ////////////////////////////////////////////////////////////////////////////
    // Omegacomplete Core
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

    bool shouldEnableDisambiguateMode(const std::string& word, unsigned& index);
    bool shouldEnableTerminusMode(
        const std::string& word, std::string& prefix);
    void addLevenshteinCorrections(
        const std::string& input,
        CompleteItemVectorPtr& completions,
        std::set<std::string>& added_words);

    ////////////////////////////////////////////////////////////////////////////
    // Omegacomplete Core
    ////////////////////////////////////////////////////////////////////////////
    static Omegacomplete* instance_;

    boost::thread worker_thread_;
    volatile int is_quitting_;

    std::map<String, boost::function<std::string (StringPtr)> >
        command_dispatcher_;

    boost::mutex buffers_mutex_;
    boost::unordered_map<unsigned, Buffer> buffers_;

    boost::mutex job_queue_mutex_;
    boost::condition_variable job_queue_conditional_variable_;
    std::queue<ParseJob> job_queue_;

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
    CompleteItemVectorPtr prev_completions_;

    bool is_corrections_only_;
    bool should_autocomplete_;

    // if both these are true then autocomplete is triggered
    bool suffix0_;
    bool suffix1_;
    CompleteItemVectorPtr autocomplete_completions_;
    std::string autocomplete_input_trigger_;

    std::map<std::string, std::string> config_;
};
