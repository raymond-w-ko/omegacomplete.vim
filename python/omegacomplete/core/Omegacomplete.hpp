#pragma once

#include "Buffer.hpp"
#include "WordCollection.hpp"
#include "CompleteItem.hpp"
#include "Stopwatch.hpp"
#include "TagsCollection.hpp"
#include "TestCases.hpp"

class Omegacomplete : public boost::noncopyable {
 public:
  struct Completions {
    boost::mutex Mutex;
    CompleteItemVectorPtr Items;
  };

  struct DoneStatus {
    DoneStatus() : Count(0) {}
    boost::mutex Mutex;
    boost::condition_variable ConditionVariable;
    int Count;
  };

  static void InitStatic();
  static unsigned GetNumThreadsToUse();

  static unsigned kNumThreads;

 public:
  Omegacomplete();
  ~Omegacomplete();
  const std::string Eval(const char* request, const int request_len);
  const std::string Eval(const std::string& request);

  WordCollection Words;
  TagsCollection Tags;

 private:
  struct ParseJob {
    ParseJob() : BufferNumber(0) {}

    ParseJob(unsigned buffer_number, const StringPtr& contents)
        : BufferNumber(buffer_number), Contents(contents) {}

    ParseJob(const ParseJob& other)
        : BufferNumber(other.BufferNumber), Contents(other.Contents) {}

    unsigned BufferNumber;
    StringPtr Contents;
  };

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
  std::string cmdSetLogFile(StringPtr argument);
  std::string cmdStartStopwatch(StringPtr argument);
  std::string cmdStopStopwatch(StringPtr argument);
  std::string cmdDoTests(StringPtr argument);
  std::string cmdCurrentIskeyword(StringPtr argument);
  std::string cmdGetWordBeginIndex(StringPtr argument);

  ////////////////////////////////////////////////////////////////////////////
  // Omegacomplete Core
  ////////////////////////////////////////////////////////////////////////////
  void queueParseJob(ParseJob job);
  void workerThreadLoop();

  void calculateCompletionCandidates(const std::string& line,
                                     std::string& result);
  std::string getWordToComplete(const std::string& line);
  void genericKeywordCompletion(const std::string& line, std::string& result);

  bool shouldEnableDisambiguateMode(const std::string& word, unsigned& index);
  void addLevenshteinCorrections(const std::string& input,
                                 CompleteItemVectorPtr& completions);

  ////////////////////////////////////////////////////////////////////////////
  // Omegacomplete Core
  ////////////////////////////////////////////////////////////////////////////
  // primary thread pool
  boost::asio::io_service io_service_;
  boost::asio::io_service::work io_service_work_;
  boost::thread_group threads_;

  // single thread for sequential jobs
  boost::thread worker_thread_;
  volatile int is_quitting_;

  std::map<String, boost::function<std::string(StringPtr)> >
      command_dispatcher_;

  boost::mutex buffers_mutex_;
  boost::unordered_map<unsigned, Buffer> buffers_;

  boost::mutex job_queue_mutex_;
  boost::condition_variable job_queue_condition_variable_;
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

  Stopwatch stopwatch_;
  std::ofstream log_file_;

  TestCases test_cases_;
};
