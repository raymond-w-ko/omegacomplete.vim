#include "stdafx.hpp"

#include "LookupTable.hpp"
#include "CompletionPriorities.hpp"
#include "Algorithm.hpp"

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Algorithm::LevenshteinSearch(
    const std::string& word,
    const size_t max_cost,
    const TrieNode& trie,
    LevenshteinSearchResults& results) {
  // generate sequence from [0, len(word)]
  size_t row_size = word.size() + 1;
  std::vector<size_t> current_row(row_size);
  for (size_t i = 0; i < row_size; ++i)
    current_row[i] = i;

  for (int i = 0; i < TrieNode::kNumChars; ++i) {
    TrieNode* next_node = trie.Children[i];
    if (!next_node)
      continue;
    char letter = trie.Letter;

    Algorithm::LevenshteinSearchRecursive(
        *next_node,
        letter,
        word,
        current_row,
        results,
        max_cost);
  }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Algorithm::LevenshteinSearchRecursive(
    const TrieNode& node,
    char letter,
    const std::string& word,
    const std::vector<size_t>& previous_row,
    LevenshteinSearchResults& results,
    size_t max_cost) {
  size_t columns = word.length() + 1;
  std::vector<size_t> current_row(columns);
  current_row[0] = previous_row[0] + 1;

  // Build one row for the letter, with a column for each letter in the target
  // word, plus one for the empty string at column 0
  for (unsigned column = 1; column < columns; ++column) {
    size_t insert_cost = current_row[column - 1] + 1;
    size_t delete_cost = previous_row[column] + 1;

    size_t replace_cost;
    if (word[column - 1] != letter)
      replace_cost = previous_row[column - 1] + 1;
    else
      replace_cost = previous_row[column - 1];

    current_row[column] =
        (std::min)(insert_cost, (std::min)(delete_cost, replace_cost));
  }

  // if the last entry in the row indicates the optimal cost is less than the
  // maximum cost, and there is a word in this trie node, then add it.
  size_t last_index = current_row.size() - 1;
  if (current_row[last_index] <= max_cost && node.IsWord) {
    results[current_row[last_index]].insert(node.GetWord());
  }

  // if any entries in the row are less than the maximum cost, then
  // recursively search each branch of the trie
  if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost) {
    for (int i = 0; i < TrieNode::kNumChars; ++i) {
      TrieNode* next_node = node.Children[i];
      if (!next_node)
        continue;
      char next_letter = next_node->Letter;

      LevenshteinSearchRecursive(
          *next_node,
          next_letter,
          word,
          current_row,
          results,
          max_cost);
    }
  }
}

void Algorithm::ProcessWords(
    Omegacomplete::Completions* completions,
    Omegacomplete::DoneStatus* done_status,
    const std::vector<String>* word_list,
    int begin,
    int end,
    const std::string& input,
    bool terminus_mode) {
  CompleteItem item;
  for (int i = begin; i < end; ++i) {
    const String& word = (*word_list)[i];
    if (word.empty())
      continue;
    if (word == input)
      continue;
    if (input.size() > word.size())
      continue;

    item.Score = Algorithm::GetWordScore(word, input, terminus_mode);

    if (item.Score > 0) {
      item.Word = word;
      completions->Mutex.lock();
      completions->Items->push_back(item);
      completions->Mutex.unlock();
    }
  }

  if (input.size() >= 2 && input[input.size() - 1] == '_') {
    std::string trimmed_input(input.begin(), input.end() - 1);
    Algorithm::ProcessWords(
        completions,
        done_status,
        word_list,
        begin, end,
        trimmed_input,
        true);
  } else {
    boost::mutex::scoped_lock lock(done_status->Mutex);
    done_status->Count += 1;
    done_status->ConditionVariable.notify_one();
  }
}

float Algorithm::GetWordScore(const std::string& word, const std::string& input,
                              bool terminus_mode) {
  if (terminus_mode && word[word.size() - 1] != '_')
    return 0;

  const float word_size = static_cast<float>(word.size());
  float score1 = 0;
  float score2 = 0;

  std::string boundaries;
  Algorithm::GetWordBoundaries(word, boundaries);

  if (boundaries.size() > 0 &&
      IsSubsequence(input, boundaries) &&
      IsSubsequence(word, input)) {
    score1 = input.size() / word_size;
  }

  if (input.size() > 3 && boost::starts_with(word, input)) {
    score2 = input.size() / word_size;
  }

  score1 = std::max(score1, score2);
  if (terminus_mode) {
    score1 *= 2.0;
  }
  return score1;
}

bool Algorithm::IsSubsequence(const std::string& haystack, const std::string& needle) {
  const size_t haystack_size = haystack.size();
  const size_t needle_size = needle.size();

  if (needle_size > haystack_size)
    return false;

  size_t j = 0;
  for (size_t i = 0; i < haystack_size; ++i) {
    char haystack_ch = haystack[i];
    if (j >= needle_size)
      break;
    char needle_ch = needle[j];
    if (LookupTable::ToLower[needle_ch] == LookupTable::ToLower[haystack_ch])
      j++;
  }

  if (j == needle_size)
    return true;
  else
    return false;
}

void Algorithm::GetWordBoundaries(const std::string& word, std::string& boundaries) {
  if (word.size() < 3) {
    return;
  }

  boundaries += word[0];
  const size_t len = word.size() - 1;
  for (size_t i = 1; i < len; ++i) {
    char c = word[i];
    char c2 = word[i + 1];

    if (c == '_' || c == '-') {
      boundaries.push_back(c2);
      // skip pushed char
      ++i;
    } else if (!LookupTable::IsUpper[c] && LookupTable::IsUpper[c2]) {
      boundaries.push_back(c2);
      ++i;
    }
  }

  if (boundaries.size() == 1) {
    boundaries.clear();
  }
}
#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Omegacomplete.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"

using namespace std;

void Buffer::TokenizeContentsIntoKeywords(
    StringPtr contents,
    UnorderedStringIntMapPtr words) {
  const std::string& text = *contents;
  size_t len = text.size();
  for (size_t i = 0; i < len; ++i) {
    // initial case, find continuous sequences in the set of [a-zA-Z0-9_\-]
    // This will be what is considered a "word".  I guess we have Unicode stuff
    // then we are screwed :-(
    uchar c = static_cast<uchar>(text[i]);
    if (!LookupTable::IsPartOfWord[c])
      continue;

    // we have found the beginning of the word, loop until we reach the end or
    // we find a non-"word" character.
    size_t j = i + 1;
    for (; j < len; ++j) {
      if (LookupTable::IsPartOfWord[static_cast<uchar>(text[j])])
        continue;
      break;
    }

    // strip leading hyphens (e.g. in the case of --number)
    while (i < j && i < len && text[i] == '-')
      ++i;

    size_t next_index = j;

    // strip trailing hyphens (e.g. in the case of number-)
    while (j > 0 && text[j - 1] == '-')
      --j;

    // construct word based off of pointer
    // don't want "words" that end in hyphen
    if (j > i) {
      std::string word(text.begin() + i,
                       text.begin() + j);
      (*words)[word]++;
    }

    // for loop will autoincrement
    i = next_index;
  }
}

Buffer::~Buffer() {
    if (parent_ == NULL)
        return;
    if (words_ == NULL)
        return;

    AUTO(i, words_->begin());
    for (; i != words_->end(); ++i) {
        i->second = -i->second;;
    }
    parent_->Words.UpdateWords(words_.get());
}

void Buffer::Init(Omegacomplete* parent, unsigned buffer_id) {
  parent_ = parent;
  buffer_id_ = buffer_id;
}

void Buffer::ReplaceContentsWith(StringPtr new_contents) {
  if (!new_contents)
    return;

  // we have already parsed the exact samething, no need to reparse again
  if (contents_ && *contents_ == *new_contents)
    return;

  UnorderedStringIntMapPtr new_words;
  if (words_) {
    // prevent unnecessary rehashing on resize by allocating at least the
    // number of buckets of the previous hash table.
    new_words = boost::make_shared<UnorderedStringIntMap>(words_->bucket_count());
  } else {
    new_words = boost::make_shared<UnorderedStringIntMap>();
  }
  Buffer::TokenizeContentsIntoKeywords(new_contents, new_words);

  if (!contents_) {
    // easy first case, just increment reference count for each word
    parent_->Words.UpdateWords(new_words.get());
  } else {
    // otherwise we have to a slow calculation of words to add and words to
    // remove. since this happens in a background thread, it is okay to be
    // slow.
    UnorderedStringIntMap to_be_removed;
    UnorderedStringIntMap to_be_added;
    UnorderedStringIntMap to_be_changed;

    // calculated words to be removed, by checking lack of existence in
    // new_words set
    AUTO(i, words_->begin());
    for (; i != words_->end(); ++i) {
      const std::string& word = i->first;
      const int ref_count = i->second;
      if (!Contains(*new_words, word))
        to_be_removed[word] -= ref_count;
    }

    // calculate words to be added, by checking checking lack of existence
    // in the old set
    AUTO(j, new_words->begin());
    for (; j != new_words->end(); ++j) {
      const std::string& word = j->first;
      const int ref_count = j->second;
      if (!Contains(*words_, word))
        to_be_added[word] += ref_count;
    }

    // update reference count for words that both exist in new_words and
    // words_
    AUTO(k, words_->begin());
    for (; k != words_->end(); ++k) {
      const std::string& word = k->first;
      const int ref_count = k->second;
      if (Contains(*new_words, word))
        to_be_changed[word] += (*new_words)[word] - ref_count;
    }

    parent_->Words.UpdateWords(&to_be_added);
    parent_->Words.UpdateWords(&to_be_removed);
    parent_->Words.UpdateWords(&to_be_changed);
  }

  // replace old contents and words with thew new ones
  contents_ = new_contents;
  words_ = new_words;
}
// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.hpp"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved) {
  (void)hModule;
  (void)lpReserved;
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
#endif
#include "stdafx.hpp"

#include "LookupTable.hpp"

const unsigned LookupTable::kMaxNumCompletions = 32;
const unsigned LookupTable::kMaxNumQuickMatch = 10;

char LookupTable::IsPartOfWord[256];
char LookupTable::IsUpper[256];
char LookupTable::ToLower[256];
char LookupTable::IsNumber[256];
std::vector<char> LookupTable::QuickMatchKey;
boost::unordered_map<char, unsigned> LookupTable::ReverseQuickMatch;

void LookupTable::InitStatic() {
  // generate lookup tables
  for (size_t index = 0; index <= 255; ++index) {
    char ch = static_cast<char>(index);
    IsPartOfWord[index] = isPartOfWord(ch) ? 1 : 0;
    IsUpper[index] = isUpper(ch) ? 1 : 0;
    IsNumber[index] = isNumber(ch) ? 1 : 0;

    std::string temp(1, ch);
    boost::algorithm::to_lower(temp);
    LookupTable::ToLower[index] = temp[0];
  }

  QuickMatchKey.resize(kMaxNumCompletions, ' '),

  QuickMatchKey[0] = '1';
  QuickMatchKey[1] = '2';
  QuickMatchKey[2] = '3';
  QuickMatchKey[3] = '4';
  QuickMatchKey[4] = '5';
  QuickMatchKey[5] = '6';
  QuickMatchKey[6] = '7';
  QuickMatchKey[7] = '8';
  QuickMatchKey[8] = '9';
  QuickMatchKey[9] = '0';

  for (unsigned i = 0; i < 10; ++i) {
    ReverseQuickMatch[QuickMatchKey[i]] = i;
  }
}

bool LookupTable::isPartOfWord(char c) {
  if ((('a' <= c) && (c <= 'z')) ||
      (('A' <= c) && (c <= 'Z')) ||
      (c == '-') ||
      (c == '_') ||
      (('0' <= c) && (c <= '9'))) {
    return true;
  }

  return false;
}

bool LookupTable::isUpper(char c) {
  if (('A' <= c) && (c <= 'Z')) {
    return true;
  } else {
    return false;
  }
}

bool LookupTable::isNumber(char c) {
  if (('0' <= c) && (c <= '9')) {
    return true;
  } else {
    return false;
  }
}
#include "stdafx.hpp"

#include "Omegacomplete.hpp"
#include "TagsCollection.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "TrieNode.hpp"

using namespace std;
using namespace boost;

static const std::string kDefaultResponse = "ACK";
static const std::string kErrorResponse = "ERROR";
static const std::string kUnknownCommandResponse = "UNKNOWN_COMMAND";

unsigned Omegacomplete::kNumThreads = 1;

void Omegacomplete::InitStatic() {
  // dependencies in other classes that have to initialized first
  LookupTable::InitStatic();
  TrieNode::InitStatic();
  TagsCollection::InitStatic();

  unsigned num_hardware_threads = boost::thread::hardware_concurrency();
  // hardware_concurrency() can return 0, so make at least 1 threads
  kNumThreads = max(num_hardware_threads, (unsigned)1);
}

unsigned Omegacomplete::GetNumThreadsToUse() {
  return kNumThreads;
}

Omegacomplete::Omegacomplete()
    : Words(true),
      io_service_work_(io_service_),
      is_quitting_(0),
      buffer_contents_follow_(false),
      prev_input_(3),
      is_corrections_only_(false),
      should_autocomplete_(false),
      suffix0_(false),
      suffix1_(false) {
  initCommandDispatcher();

  for (unsigned i = 0; i < Omegacomplete::GetNumThreadsToUse(); ++i) {
    threads_.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service_));
  }

  worker_thread_ = boost::thread(&Omegacomplete::workerThreadLoop, this);
}

void Omegacomplete::initCommandDispatcher() {
  using boost::bind;
  using boost::ref;

#define CONNECT(cmd, func) \
  command_dispatcher_[cmd] = boost::bind(&Omegacomplete::func, ref(*this), _1);

  CONNECT("current_buffer_id", cmdCurrentBufferId)
  CONNECT("current_buffer_absolute_path", cmdCurrentBufferAbsolutePath)
  CONNECT("current_line", cmdCurrentLine)
  CONNECT("cursor_position", cmdCursorPosition)
  CONNECT("buffer_contents_follow", cmdBufferContentsFollow)
  CONNECT("complete", cmdComplete)
  CONNECT("free_buffer", cmdFreeBuffer)
  CONNECT("current_directory", cmdCurrentDirectory)
  CONNECT("current_tags", cmdCurrentTags)
  CONNECT("taglist_tags", cmdTaglistTags)
  CONNECT("vim_taglist_function", cmdVimTaglistFunction)
  CONNECT("prune", cmdPrune)
  CONNECT("flush_caches", cmdFlushCaches)
  CONNECT("is_corrections_only", cmdIsCorrectionsOnly)
  CONNECT("prune_buffers", cmdPruneBuffers)
  CONNECT("should_autocomplete", cmdShouldAutocomplete)
  CONNECT("config", cmdConfig)
  CONNECT("get_autocomplete", cmdGetAutocomplete)
  CONNECT("set_log_file", cmdSetLogFile)
  CONNECT("start_stopwatch", cmdStartStopwatch)
  CONNECT("stop_stopwatch", cmdStopStopwatch)
  CONNECT("do_tests", cmdDoTests)

#undef CONNECT
}

Omegacomplete::~Omegacomplete() {
  is_quitting_ = 1;
  worker_thread_.join();
  io_service_.stop();
  threads_.join_all();
}

const std::string Omegacomplete::Eval(const std::string& request) {
  return this->Eval(request.c_str(), (int)request.size());
}

const std::string Omegacomplete::Eval(const char* request,
                                      const int request_len) {
  if (buffer_contents_follow_) {
    StringPtr argument = boost::make_shared<string>(request, static_cast<size_t>(request_len));

    return queueBufferContents(argument);
  } else {
    // find first space
    int index = -1;
    for (int i = 0; i < request_len; ++i) {
      if (request[i] == ' ') {
        index = i;
        break;
      }
    }
    if (index == -1) {
      return kErrorResponse;
    }

    // break it up into a "request" string and an "argument" string
    std::string command(request, static_cast<size_t>(index));
    StringPtr argument = boost::make_shared<std::string>(
        request + index + 1,
        static_cast<size_t>(request_len - command.size() - 1));

    AUTO(iter, command_dispatcher_.find(command));
    if (iter == command_dispatcher_.end()) {
      return kUnknownCommandResponse;
    } else {
      return iter->second(argument);
    }
  }
}

std::string Omegacomplete::cmdCurrentBufferId(StringPtr argument) {
  current_buffer_id_ = lexical_cast<unsigned>(*argument);
  if (!Contains(buffers_, current_buffer_id_)) {
    buffers_[current_buffer_id_].Init(this, current_buffer_id_);
  }
  return kDefaultResponse;
}

std::string Omegacomplete::cmdCurrentBufferAbsolutePath(StringPtr argument) {
  current_buffer_absolute_path_ = *argument;
  return kDefaultResponse;
}

std::string Omegacomplete::cmdCurrentLine(StringPtr argument) {
  current_line_ = *argument;
  return kDefaultResponse;
}

std::string Omegacomplete::cmdCursorPosition(StringPtr argument) {
  std::vector<std::string> position;
  split(position,
        *argument,
        boost::is_any_of(" "),
        boost::token_compress_on);
  unsigned x = lexical_cast<unsigned>(position[0]);
  unsigned y = lexical_cast<unsigned>(position[1]);
  cursor_pos_.Line = x; cursor_pos_.Column = y;

  return kDefaultResponse;
}

std::string Omegacomplete::cmdBufferContentsFollow(StringPtr argument) {
  buffer_contents_follow_ = true;
  return kDefaultResponse;
}

std::string Omegacomplete::queueBufferContents(StringPtr argument) {
  buffer_contents_follow_ = false;
  current_contents_ = argument;

  ParseJob job(current_buffer_id_, current_contents_);
  queueParseJob(job);

  return kDefaultResponse;
}

std::string Omegacomplete::cmdComplete(StringPtr argument) {
  std::string response;
  calculateCompletionCandidates(*argument, response);

  return response;
}

std::string Omegacomplete::cmdFreeBuffer(StringPtr argument) {
  // make sure job queue is empty before we can delete buffer
  for (;;) {
    job_queue_mutex_.lock();
    if (job_queue_.size() == 0)
      break;
    job_queue_mutex_.unlock();

    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  }

  buffers_mutex_.lock();
  buffers_.erase(boost::lexical_cast<unsigned>(*argument));
  buffers_mutex_.unlock();
  job_queue_mutex_.unlock();

  return kDefaultResponse;
}

std::string Omegacomplete::cmdCurrentDirectory(StringPtr argument) {
  current_directory_ = *argument;
  return kDefaultResponse;
}

std::string Omegacomplete::cmdCurrentTags(StringPtr argument) {
  current_tags_.clear();

  std::vector<std::string> tags_list;
  boost::split(tags_list,
               *argument,
               boost::is_any_of(","),
               boost::token_compress_on);
  foreach (const std::string& tags, tags_list) {
    if (tags.size() == 0)
      continue;

    Tags.CreateOrUpdate(tags, current_directory_);
    current_tags_.push_back(tags);
  }

  return kDefaultResponse;
}

std::string Omegacomplete::cmdTaglistTags(StringPtr argument) {
  taglist_tags_.clear();

  std::vector<std::string> tags_list;
  boost::split(tags_list, *argument,
               boost::is_any_of(","), boost::token_compress_on);
  foreach (const std::string& tags, tags_list)
  {
    if (tags.size() == 0)
      continue;

    Tags.CreateOrUpdate(tags, current_directory_);
    taglist_tags_.push_back(tags);
  }

  return kDefaultResponse;
}

std::string Omegacomplete::cmdVimTaglistFunction(StringPtr argument) {
  const std::string& response = Tags.VimTaglistFunction(
      *argument, taglist_tags_, current_directory_);

  return response;
}

std::string Omegacomplete::cmdPrune(StringPtr argument) {
  Words.Prune();

  return kDefaultResponse;
}

std::string Omegacomplete::cmdFlushCaches(StringPtr argument) {
  Tags.Clear();

  return kDefaultResponse;
}

std::string Omegacomplete::cmdIsCorrectionsOnly(StringPtr argument) {
  const std::string& response = is_corrections_only_ ? "1" : "0";
  return response;
}

std::string Omegacomplete::cmdPruneBuffers(StringPtr argument) {
  if (argument->size() == 0)
    return kDefaultResponse;

  std::vector<std::string> numbers;
  boost::split(
      numbers,
      *argument,
      boost::is_any_of(","),
      boost::token_compress_on);
  std::set<unsigned> valid_buffers;
  foreach (const std::string& number, numbers) {
    valid_buffers.insert(boost::lexical_cast<unsigned>(number));
  }

  buffers_mutex_.lock();

  std::vector<unsigned> to_be_erased;
  AUTO (iter, buffers_.begin());
  for (; iter != buffers_.end(); ++iter) {
    unsigned num = iter->second.GetNumber();
    if (!Contains(valid_buffers, num))
      to_be_erased.push_back(num);
  }

  foreach (unsigned num, to_be_erased) {
    buffers_.erase(num);
  }

  buffers_mutex_.unlock();

  return kDefaultResponse;
}

std::string Omegacomplete::cmdShouldAutocomplete(StringPtr argument) {
  const std::string& response = should_autocomplete_ ? "1" : "0";
  return response;
}

std::string Omegacomplete::cmdConfig(StringPtr argument) {
  std::vector<std::string> tokens;
  boost::split(tokens, *argument, boost::is_any_of(" "), boost::token_compress_on);

  if (tokens.size() != 2)
    return kDefaultResponse;
  config_[tokens[0]] = tokens[1];

  return kDefaultResponse;
}

std::string Omegacomplete::cmdGetAutocomplete(StringPtr argument) {
  if (!suffix0_ || !suffix1_)
    return "";

  if (!autocomplete_completions_ || autocomplete_completions_->size() == 0) {
    suffix0_ = suffix1_ = false;
    return "";
  }

  // create autocompletion string
  std::string result;
  for (size_t i = 0; i < autocomplete_input_trigger_.size(); ++i) {
    result += "\x08";
  }
  result += (*autocomplete_completions_)[0].Word;

  // clear so it can't be accidentally reused
  suffix0_ = suffix1_ = false;
  autocomplete_completions_.reset();

  return result;
}

std::string Omegacomplete::cmdSetLogFile(StringPtr argument) {
  log_file_.close();
  log_file_.open(argument->c_str(), std::ios::app) ;
  return kDefaultResponse;
}

std::string Omegacomplete::cmdStartStopwatch(StringPtr argument) {
  stopwatch_.Start();
  return kDefaultResponse;
}

std::string Omegacomplete::cmdStopStopwatch(StringPtr argument) {
  stopwatch_.Stop();
  const std::string& ns = stopwatch_.ResultNanosecondsToString();
  log_file_ << ns << std::endl;
  return kDefaultResponse;
}

std::string Omegacomplete::cmdDoTests(StringPtr argument) {
  std::stringstream results;
  log_file_ << results.str();
  return kDefaultResponse;
}

void Omegacomplete::queueParseJob(ParseJob job) {
  boost::mutex::scoped_lock lock(job_queue_mutex_);
  job_queue_.push(job);
  lock.unlock();
  job_queue_condition_variable_.notify_one();
}

#ifdef __CYGWIN__
#  include <windows.h>
#endif

void Omegacomplete::workerThreadLoop() {
#if defined(_WIN32) || defined(__CYGWIN__)
  {
    BOOL success = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    if (!success)
      ExitProcess(42);
  }
#else
  {
    int failed;

    pthread_t this_thread = pthread_self();
    struct sched_param params;
    params.sched_priority = 0;
#ifndef SCHED_IDLE
    // need to figure out the equivalent on Mac OS X
    failed = 0;
#else
    failed = pthread_setschedparam(this_thread, SCHED_IDLE, &params);
#endif
    if (failed)
      exit(42);

    int policy = 0;
    failed = pthread_getschedparam(this_thread, &policy, &params);
    if (failed)
      exit(42);
#ifdef SCHED_IDLE
    if (policy != SCHED_IDLE)
      exit(42);
#endif
  }
#endif

  ParseJob job;
  while (!is_quitting_) {
    {
      boost::mutex::scoped_lock lock(job_queue_mutex_);
      while (job_queue_.empty()) {
        if (is_quitting_)
          return;

        const boost::system_time timeout =
            boost::get_system_time() +
            boost::posix_time::milliseconds(1 * 1000);
        job_queue_condition_variable_.timed_wait(lock, timeout);

        if (is_quitting_)
          return;
      }
      job = job_queue_.front();
      job_queue_.pop();
    }

    // do the job
    boost::mutex::scoped_lock lock(buffers_mutex_);
    if (!Contains(buffers_, job.BufferNumber))
      continue;
    buffers_[job.BufferNumber].ReplaceContentsWith(job.Contents);
  }
}

void Omegacomplete::calculateCompletionCandidates(
    const std::string& line,
    std::string& result) {
  genericKeywordCompletion(line, result);
}

void Omegacomplete::genericKeywordCompletion(
    const std::string& line,
    std::string& result) {
  std::string input = getWordToComplete(line);
  if (input.empty())
    return;

  // check to see if autocomplete is tripped
  if (suffix0_ &&
      input.size() >= 3 &&
      input.substr(input.size() - 2) == config_["autocomplete_suffix"]) {
    suffix1_ = true;
    autocomplete_input_trigger_ = input;
    should_autocomplete_ = true;
    return;
  } else if (!suffix0_ &&
             input[input.size() - 1] == config_["autocomplete_suffix"][0]) {
    suffix0_ = true;
    autocomplete_completions_ = prev_completions_;
  } else {
    suffix0_ = suffix1_ = false;
  }

  // keep a trailing list of previous inputs
  prev_input_[2] = prev_input_[1];
  prev_input_[1] = prev_input_[0];
  prev_input_[0] = input;

  // server side disambiguation
  unsigned disambiguate_index = UINT_MAX;
  bool disambiguate_mode =
      config_["server_side_disambiguate"] != "0" &&
      shouldEnableDisambiguateMode(input, disambiguate_index);
  if (disambiguate_mode &&
      prev_completions_ &&
      disambiguate_index < prev_completions_->size()) {
    const CompleteItem& completion = (*prev_completions_)[disambiguate_index];
    result += "[";
    result += completion.SerializeToVimDict();
    result += "]";
    return;
  }

  const int num_threads = Omegacomplete::GetNumThreadsToUse();
  Completions completions;
  completions.Items = boost::make_shared<CompleteItemVector>();
  Completions tags_completions;
  tags_completions.Items = boost::make_shared<CompleteItemVector>();

  Words.Lock();
  AUTO(const & word_list, Words.GetWordList());
  Tags.Words.Lock();
  AUTO(const & tags_word_list, Tags.Words.GetWordList());

  DoneStatus done_status;

  // queue jobs for main (buffer) words
  for (int i = 0; i < num_threads; ++i) {
    const int begin = ((i + 0) * (unsigned)word_list.size()) / num_threads;
    const int end = ((i + 1) * (unsigned)word_list.size()) / num_threads;
    io_service_.post(boost::bind(
            Algorithm::ProcessWords,
            &completions,
            &done_status,
            &word_list, begin, end, input, false));
  }
  // queue jobs for tags words
  for (int i = 0; i < num_threads; ++i) {
    const int begin = ((i + 0) * (unsigned)tags_word_list.size()) / num_threads;
    const int end = ((i + 1) * (unsigned)tags_word_list.size()) / num_threads;
    io_service_.post(boost::bind(
            Algorithm::ProcessWords,
            &tags_completions,
            &done_status,
            &tags_word_list, begin, end, input, false));
  }

  // unlock all the things
  {
    boost::mutex::scoped_lock lock(done_status.Mutex);
    while (done_status.Count < (num_threads * 2)) {
      done_status.ConditionVariable.wait(lock);
    }
  }
  Tags.Words.Unlock();
  Words.Unlock();

  std::sort(completions.Items->begin(), completions.Items->end());
  std::sort(tags_completions.Items->begin(), tags_completions.Items->end());

  // filter out duplicates
  boost::unordered_set<std::string> added_words;
  added_words.insert(input);
  // prevent completions from being considered that are basically the word from
  // before you press Backspace, as often it doesn't get removed fast enough
  // from the global word collection
  if (prev_input_[1].size() == (input.size() + 1) &&
      boost::starts_with(prev_input_[1], input)) {
    added_words.insert(prev_input_[1]);
  }

  // build final list
  CompleteItemVectorPtr final_items = boost::make_shared<CompleteItemVector>();
  CompleteItemVectorPtr main_items = completions.Items;
  CompleteItemVectorPtr tags_items = tags_completions.Items;
  for (size_t i = 0; i < main_items->size(); ++i) {
    const std::string& word = (*main_items)[i].Word;
    if (!Contains(added_words, word)) {
      added_words.insert(word);
      final_items->push_back((*main_items)[i]);
    }
  }
  for (size_t i = 0; i < tags_items->size(); ++i) {
    const std::string& word = (*tags_items)[i].Word;
    if (!Contains(added_words, word)) {
      added_words.insert(word);
      final_items->push_back((*tags_items)[i]);
    }
  }

  // try to spell check if we have no candidates
  addLevenshteinCorrections(input, final_items);

  // assign quick match number of entries
  for (size_t i = 0; i < LookupTable::kMaxNumQuickMatch; ++i) {
    if (i >= final_items->size())
      break;

    CompleteItem& item = (*final_items)[i];
    item.Menu = lexical_cast<string>(LookupTable::QuickMatchKey[i]) +
        " " + item.Menu;
  }

  result += "[";
  for (size_t i = 0; i < final_items->size(); ++i) {
    if (i > LookupTable::kMaxNumCompletions) {
      break;
    }
    result += (*final_items)[i].SerializeToVimDict();
  }
  result += "]";

  prev_completions_ = final_items;

  if (final_items->size() > 0)
    suffix0_ = false;
}

std::string Omegacomplete::getWordToComplete(const std::string& line) {
  if (line.length() == 0)
    return "";

  // we can potentially have an empty line
  int partial_end = static_cast<int>(line.length());
  int partial_begin = partial_end - 1;
  for (; partial_begin >= 0; --partial_begin) {
    uchar c = static_cast<uchar>(line[partial_begin]);
    if (LookupTable::IsPartOfWord[c])
      continue;

    break;
  }

  if ((partial_begin + 1) == partial_end)
    return "";

  std::string partial(&line[partial_begin + 1], &line[partial_end]);
  return partial;
}

bool Omegacomplete::shouldEnableDisambiguateMode(
    const std::string& word, unsigned& index) {
  if (word.size() < 2)
    return false;

  uchar last = static_cast<uchar>(word[word.size() - 1]);
  if (LookupTable::IsNumber[last]) {
    if (Contains(LookupTable::ReverseQuickMatch, last))
      index = LookupTable::ReverseQuickMatch[last];
    return true;
  }

  return false;
}

void Omegacomplete::addLevenshteinCorrections(
    const std::string& input,
    CompleteItemVectorPtr& completions) {
  if (completions->size() == 0) {
    is_corrections_only_ = true;
  } else {
    is_corrections_only_ = false;
    return;
  }

  LevenshteinSearchResults levenshtein_completions;
  Words.GetLevenshteinCompletions(input, levenshtein_completions);

  AUTO (iter, levenshtein_completions.begin());
  for (; iter != levenshtein_completions.end(); ++iter) {
    foreach (const std::string& word, iter->second) {
      if (completions->size() >= LookupTable::kMaxNumCompletions)
        return;

      if (word == input)
        continue;
      if (boost::starts_with(input, word))
        continue;
      // it take some time for unique words to clear WordCollection,
      // so counteract this
      if ((input.size() < word.size()) &&
          (word.size() - input.size() <= 2) &&
          boost::starts_with(word, input))
        continue;
      if (boost::ends_with(word, "-"))
        continue;

      CompleteItem completion(word);
      completions->push_back(completion);
    }
  }
}
#include "stdafx.hpp"
#include "Omegacomplete.hpp"

#ifdef _WIN32
#define HAVE_ROUND
#endif
#include <Python.h>

static Omegacomplete* omegacomplete = NULL;

static PyObject* eval(PyObject* self, PyObject* args) {
  (void)self;

  const char* input;
  int len;
  if (!PyArg_ParseTuple(args, "s#", &input, &len)) {
    return NULL;
  }

  const std::string& result = omegacomplete->Eval(input, len);
  return Py_BuildValue("s", result.c_str());
}

static PyMethodDef CoreMethods[] = {
  {
    "eval", eval,
    METH_VARARGS,
    "Send a message to the completion core and possibly receive a response"
  },
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initcore(void) {
  (void) Py_InitModule("core", CoreMethods);

  Omegacomplete::InitStatic();
  omegacomplete = new Omegacomplete;
}
#include "stdafx.hpp"
#include "stdafx.hpp"

#include "Tags.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "CompletionPriorities.hpp"

#ifndef _WIN32
static inline void UnixGetClockTime(struct timespec& ts) {
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
}
#endif

Tags::Tags(TagsCollection* parent, const std::string& pathname)
    : last_write_time_(0),
      last_tick_count_(-1.0) {
  parent_ = parent;
  pathname_ = pathname;

  if (!calculateParentDirectory()) {
    //std::cout << "couldn't calculate parent directory for tags file, not parsing"
        //<< std::endl;
    //std::cout << pathname_ << std::endl;
    return;
  }
  this->Update();
}

bool Tags::calculateParentDirectory() {
  std::string directory_separator;

#ifdef _WIN32
  directory_separator = "\\";
#else
  directory_separator = "/";
#endif

  size_t pos = pathname_.rfind(directory_separator);
  // error out because we can't find the last directory separator
  if (pos == std::string::npos) {
    //std::cout << "coulnd't find last directory separator" << std::endl;
    //std::cout << pathname_ << std::endl;
    return false;
  }
  // there is nothing after the last directory separator
  if (pos >= (pathname_.size() - 1)) {
    //std::cout << "there is nothing after the last directory separator" << std::endl;
    //std::cout << pathname_ << std::endl;
    return false;
  }

  parent_directory_ = std::string(pathname_.begin(), pathname_.begin() + pos);

  return true;
}

void Tags::updateWordRefCount(const std::multimap<String, String>& tags, int sign) {
  UnorderedStringIntMap deltas;

  std::string prev_word;
  AUTO(iter, tags.begin());
  for (; iter != tags.end(); ++iter) {
    const String& word = iter->first;
    if (word == prev_word)
      continue;

    deltas[word] += sign * 1;

    prev_word = word;
  }

  parent_->Words.UpdateWords(&deltas);
}

void Tags::reparse() {
  if (parent_directory_.empty())
    return;

  updateWordRefCount(tags_, -1);
  tags_.clear();

  std::ifstream file(pathname_.c_str());
  unsigned line_num = 0;

  for (std::string line; std::getline(file, line).good(); ++line_num) {
    // the first 6 lines contain a description of the tags file, which we don't
    // need
    if (line_num < 6)
      continue;

    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of("\t"), boost::token_compress_off);
    if (tokens.size() < 3) {
      //std::cout << "invalid tag line detected!" << std::endl;
      return;
    }

    const std::string tag_name = tokens[0];
    tags_.insert(make_pair(tag_name, line));
  }

  // re-add all unique tags in this tags file
  updateWordRefCount(tags_, 1);
}

void Tags::VimTaglistFunction(const std::string& word, std::stringstream& ss) {
  AUTO(bounds, tags_.equal_range(word));
  AUTO(&iter, bounds.first);
  for (; iter != bounds.second; ++iter) {
    const std::string& tag_name = iter->first;
    const std::string& line = iter->second;
    std::string dummy;
    TagInfo tag_info;
    if (!calculateTagInfo(line, dummy, tag_info))
      continue;

    ss << "{";

    ss << boost::str(boost::format(
            "'name':'%s','filename':'%s','cmd':'%s',")
        % tag_name
        % tag_info.Location
        % tag_info.Ex);
    for (TagInfo::InfoIterator pair = tag_info.Info.begin();
         pair != tag_info.Info.end();
         ++pair) {
      ss << boost::str(boost::format(
              "'%s':'%s',")
          % pair->first % pair->second);
    }

    ss << "},";
  }
}

void Tags::Update() {
  const double check_time = 60.0 * 1000.0;
  bool reparse_needed = false;

#ifdef _WIN32
  if (last_tick_count_ == -1) {
    reparse_needed = true;
    last_tick_count_ = static_cast<double>(::GetTickCount());
  } else {
    double new_count = static_cast<double>(::GetTickCount());
    if (::fabs(new_count - last_tick_count_) > check_time) {
      reparse_needed = win32_CheckIfModified();
      last_tick_count_ = new_count;
    } else {
      reparse_needed = false;
    }
  }
#else
  timespec ts;
  UnixGetClockTime(ts);
  if (last_tick_count_ == -1) {
    reparse_needed = true;
    last_tick_count_ = ts.tv_sec * 1000.0;
  } else {
    double new_count = ts.tv_sec * 1000.0;
    if (::fabs(new_count - last_tick_count_) > check_time) {
      reparse_needed = unix_CheckIfModified();
      last_tick_count_ = new_count;
    } else {
      reparse_needed = false;
    }
  }
#endif

  if (reparse_needed)
    reparse();
}


bool Tags::calculateTagInfo(
    const std::string& line,
    std::string& tag_name,
    TagInfo& tag_info) {
  std::vector<std::string> tokens;
  boost::split(tokens, line, boost::is_any_of("\t"), boost::token_compress_off);

  if (tokens.size() < 3) {
    //std::cout << "invalid tag line detected!" << std::endl;
    return false;
  }

  tag_name = tokens[0];
  tag_info.Location = tokens[1];
  // normalize Windows pathnames to UNIX format
  boost::replace_all(tag_info.Location, "\\", "/");
  // if there is a dot slash in front, replace with location of tags
  if (tag_info.Location.length() > 1 && tag_info.Location[0] == '.')
    boost::replace_first(tag_info.Location, ".", parent_directory_);
  // if there is no slash at all then it is in the current directory of the tags
  if (tag_info.Location.find("/") == std::string::npos)
    tag_info.Location = parent_directory_ + "/" + tag_info.Location;
#ifdef _WIN32
  NormalizeToWindowsPathSeparators(tag_info.Location);
#else
  NormalizeToUnixPathSeparators(tag_info.Location);
#endif

  size_t index = 2;
  std::string ex;
  for (; index < tokens.size(); ++index) {
    std::string token = tokens[index];
    if (token.empty())
      token = "\t";

    ex += token;

    if (boost::ends_with(token, ";\""))
      break;
  }
  if (!boost::ends_with(ex, "\"")) {
    //std::cout << "Ex didn't end with ;\"" << std::endl;
    return false;
  }

  tag_info.Ex = ex;

  // increment to next token, which should be the more info fields
  index++;

  for (; index < tokens.size(); ++index) {
    std::string token = tokens[index];
    size_t colon = token.find(":");
    if (colon == std::string::npos) {
      //std::cout << "expected key value pair, but could not find ':'"
      //<< std::endl;
      //std::cout << line << std::endl;
      continue;
    }

    std::string key = std::string(token.begin(), token.begin() + colon);
    std::string value = std::string(token.begin() + colon + 1, token.end());
    tag_info.Info[key] = value;
  }

  // calculate prefix from Ex command
  // this can usually be used as the return type of the tag
  size_t header_index = tag_info.Ex.find("/^");
  size_t footer_index = tag_info.Ex.find(tag_name);
  if ((header_index != std::string::npos) &&
      (footer_index != std::string::npos)) {
    header_index += 2;
    std::string tag_prefix = tag_info.Ex.substr(
        header_index,
        footer_index - header_index);
    // search back until we find a space to handle complex
    // return types like
    // std::map<int, int> FunctionName
    // and
    // void Class::FunctionName
    footer_index = tag_prefix.rfind(" ");
    if (footer_index != std::string::npos) {
      tag_prefix = tag_prefix.substr(0, footer_index);
    }
    boost::trim(tag_prefix);
    tag_info.Info["prefix"] = tag_prefix;
  }

  return true;
}

bool Tags::win32_CheckIfModified() {
#ifdef _WIN32
  bool reparse_needed = false;

  HANDLE hFile = ::CreateFile(
      pathname_.c_str(),
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return false;

  FILETIME ft_last_write_time;
  ::GetFileTime(
      hFile,
      NULL,
      NULL,
      &ft_last_write_time);

  __int64 last_write_time = to_int64(ft_last_write_time);
  if (last_write_time > last_write_time_) {
    reparse_needed = true;
    last_write_time_ = last_write_time;
  }

  CloseHandle(hFile);
  hFile = INVALID_HANDLE_VALUE;

  return reparse_needed;
#else
  return false;
#endif
}

bool Tags::unix_CheckIfModified() {
#ifdef _WIN32
  return false;
#else
  bool reparse_needed = false;
  struct stat statbuf;
  if (::stat(pathname_.c_str(), &statbuf) == -1) {
    return false;
  }

  int64_t last_write_time = statbuf.st_mtime;
  if (last_write_time > last_write_time_) {
    reparse_needed = true;
    last_write_time_ = last_write_time;
  }

  return reparse_needed;
#endif
}
#include "stdafx.hpp"

#include "TagsCollection.hpp"

#ifdef _WIN32
static std::string kWin32SystemDrive;
#endif

std::string TagsCollection::ResolveFullPathname(
    const std::string& tags_pathname,
    const std::string& current_directory) {
  std::string full_tags_pathname = tags_pathname;

  // TODO(rko): support UNC if someone uses it
#ifdef _WIN32
  NormalizeToWindowsPathSeparators(full_tags_pathname);

  // we have an absolute pathname of the form "C:\X"
  if (full_tags_pathname.size() >= 4 &&
      ::isalpha(static_cast<unsigned char>(full_tags_pathname[0])) &&
      full_tags_pathname[1] == ':' &&
      full_tags_pathname[2] == '\\') {
    return full_tags_pathname;
  }

  // we have a UNIX style pathname that assumes someone will
  // prepend the system drive in front of the tags pathname
  if (full_tags_pathname.size() >= 2 &&
      full_tags_pathname[0] == '\\') {
    full_tags_pathname = kWin32SystemDrive + full_tags_pathname;
    return full_tags_pathname;
  }

  // we have a relative pathname, prepend current_directory
  full_tags_pathname = current_directory + "\\" + full_tags_pathname;
#else
  NormalizeToUnixPathSeparators(full_tags_pathname);

  // given pathname is absolute
  if (full_tags_pathname[0] == '/')
    return full_tags_pathname;

  // we have a relative pathname, prepend current_directory
  full_tags_pathname = current_directory + "/" + full_tags_pathname;
#endif

  return full_tags_pathname;
}

bool TagsCollection::InitStatic() {
#ifdef _WIN32
  char* sys_drive = NULL;
  size_t sys_drive_len = 0;
  errno_t err = ::_dupenv_s(&sys_drive, &sys_drive_len, "SystemDrive");
  if (err)
    return false;
  // apparently this means "C:\0", so 3 bytes
  if (sys_drive_len != 3)
    return false;

  kWin32SystemDrive.resize(2);
  kWin32SystemDrive[0] = sys_drive[0];
  kWin32SystemDrive[1] = ':';

  free(sys_drive);
  sys_drive = NULL;
#endif

  return true;
}

bool TagsCollection::CreateOrUpdate(std::string tags_pathname,
                                    const std::string& current_directory) {
  // in case the user has set 'tags' to something like:
  // './tags,./TAGS,tags,TAGS'
  // we have to resolve ambiguity
  tags_pathname = ResolveFullPathname(tags_pathname, current_directory);

  if (!Contains(tags_list_, tags_pathname))
    tags_list_[tags_pathname] = boost::make_shared<Tags>(this, tags_pathname);
  else
    tags_list_[tags_pathname]->Update();

  return true;
}

void TagsCollection::Clear() {
  tags_list_.clear();
}

std::string TagsCollection::VimTaglistFunction(
    const std::string& word,
    const std::vector<std::string>& tags_list,
    const std::string& current_directory) {
  std::stringstream ss;

  ss << "[";
  foreach (std::string tags, tags_list) {
    tags = ResolveFullPathname(tags, current_directory);
    if (Contains(tags_list_, tags) == false)
      continue;

    tags_list_[tags]->VimTaglistFunction(word, ss);
  }
  ss << "]";

  return ss.str();
}
#include "stdafx.hpp"

#ifdef HAVE_RECENT_BOOST_LIBRARY

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#endif

#include "TestCases.hpp"
#include "TrieNode.hpp"
#include "WordCollection.hpp"
#include "TagsCollection.hpp"
#include "Algorithm.hpp"

void TestCases::TagsTest()
{
    std::cout << "preloading tags files:" << std::endl;
    std::string current_directory;
    std::string tags;

    current_directory = "C:\\";

    tags = "C:\\SVN\\Syandus_ALIVE4\\Platform\\ThirdParty\\OGRE\\Include\\tags";
    std::cout << tags << std::endl;
    //TagsCollection::Instance()->CreateOrUpdate(tags, current_directory);

    //TagsCollection::Instance()->Clear();
}

void TestCases::ClangTest()
{
#ifdef ENABLE_CLANG_COMPLETION
    std::cout << "checking to see if libclang/libclang.dll works" << std::endl;

    int argc = 1;
    char* argv[] = {"--version"};

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index, 0, argv, argc, 0, 0, CXTranslationUnit_None);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
#endif
}
#include "stdafx.hpp"

#include "TrieNode.hpp"

uchar TrieNode::msCharIndex[256];
bool TrieNode::msValidChar[256];

void TrieNode::InitStatic() {
  assert(sizeof(uchar) == 1);
  assert(sizeof(bool) == 1);

  for (int i = 0; i < 256; ++i) {
    msCharIndex[(uchar)i] = 255;
    msValidChar[(uchar)i] = false;
  }

  uchar n = 0;
  for (uchar c = 'A'; c <= 'Z'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }
  for (uchar c = 'a'; c <= 'z'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }
  for (uchar c = '0'; c <= '9'; ++c) {
    msCharIndex[c] = n++;
    msValidChar[c] = true;
  }

  msCharIndex['_'] = n++;
  msValidChar['_'] = true;

  msCharIndex['-'] = n++;
  msValidChar['-'] = true;
}

TrieNode::TrieNode(TrieNode* parent, uchar letter)
    : Parent(parent),
      Letter(letter),
      IsWord(false),
      NumChildren(0) {
  for (int i = 0; i < kNumChars; ++i) {
    Children[i] = NULL;
  }

  if (Parent) {
    Parent->Children[msCharIndex[Letter]] = this;
    Parent->NumChildren++;
  }
}

TrieNode::~TrieNode() {
  if (Parent) {
    Parent->Children[msCharIndex[Letter]] = NULL;
    Parent->NumChildren--;
  }

  for (int i = 0; i < kNumChars; ++i) {
    delete Children[i];
  }
}

inline bool TrieNode::IsValidWord(const String& word) const {
  size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    uchar ch = (uchar)word[i];
    if (!msValidChar[ch]) {
      return false;
    }
  }
  return true;
}

void TrieNode::Insert(const String& word) {
  if (!IsValidWord(word))
    return;

  TrieNode* node = this;
  const size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    uchar ch = word[i];
    uchar index = msCharIndex[ch];
    if (!node->Children[index]) {
      node = new TrieNode(node, ch);
    } else {
      node = node->Children[index];
    }
  }
  node->IsWord = true;
}

void TrieNode::Erase(const String& word) {
  if (!IsValidWord(word))
    return;

  TrieNode* node = this;
  const size_t word_size = word.size();
  for (size_t i = 0; i < word_size; ++i) {
    if (!node) {
      return;
    }
    uchar ch = (uchar)word[i];
    uchar index = msCharIndex[ch];
    node = node->Children[index];
  }
  if (!node) {
    return;
  }

  node->IsWord = false;

  while (node) {
    TrieNode* prev_node = node->Parent;
    if (node->NumChildren == 0 && node->Parent) {
      delete node;
    }
    node = prev_node;
  }
}

std::string TrieNode::GetWord() const {
  std::string word;
  const TrieNode* node = this;
  while (node) {
    if (node->Parent) {
      word = std::string(1, node->Letter) + word;
    }
    node = node->Parent;
  }
  return word;
}
#include "stdafx.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"
#include "WordCollection.hpp"
#include "CompletionPriorities.hpp"

static const int kLevenshteinMaxCost = 2;
static const size_t kMinLengthForLevenshteinCompletion = 4;

int WordCollection::rand_seed_ = 42;

WordCollection::WordCollection(bool enable_trie) : trie_enabled_(enable_trie) {
  if (trie_enabled_) {
    trie_ = new TrieNode(NULL, 'Q');
  } else {
    trie_ = NULL;
  }
}

void WordCollection::UpdateWords(const UnorderedStringIntMap* word_ref_count_deltas) {
  boost::mutex::scoped_lock lock(mutex_);

  AUTO(end, word_ref_count_deltas->end());
  for (AUTO(iter, word_ref_count_deltas->begin()); iter != end; ++iter) {
    updateWord(iter->first, iter->second);
  }
}

void WordCollection::updateWord(const std::string& word, int reference_count_delta) {
  WordInfo& wi = words_[word];
  wi.ReferenceCount += reference_count_delta;
  if (wi.ReferenceCount > 0) {
    // < 0 implies it doesn't exist in word_list_ yet
    if (wi.WordListIndex < 0) {
      int index;
      if (empty_indices_.size() > 0) {
        index = *empty_indices_.begin();
        empty_indices_.erase(index);
        word_list_[index] = word;
      } else {
        index = static_cast<int>(word_list_.size());
        word_list_.push_back(word);
      }
      wi.WordListIndex = index;
    }

    if (trie_enabled_) {
      trie_->Insert(word);
    }

  } else {
    // >= 0 implies that it exist in the word list and we need to free it
    if (wi.WordListIndex >= 0) {
      int index = wi.WordListIndex;
      wi.WordListIndex = -1;
      word_list_[index].clear();
      empty_indices_.insert(index);
    }

    if (trie_enabled_) {
      trie_->Erase(word);
    }
  }
}

size_t WordCollection::Prune() {
  boost::mutex::scoped_lock lock(mutex_);

  // simply just looping through words_ and removing things might cause
  // iterator invalidation, so be safe and built a set of things to remove
  std::vector<std::string> to_be_pruned;
  AUTO(i, words_.begin());
  for (; i != words_.end(); ++i) {
    if (i->second.ReferenceCount > 0)
      continue;
    to_be_pruned.push_back(i->first);
  }

  foreach (const std::string& word, to_be_pruned) {
    words_.erase(word);

    if (trie_enabled_) {
      trie_->Erase(word);
    }
  }

  // rebuild word_list_ to eliminate any holes, basically a compaction step
  empty_indices_.clear();
  word_list_.clear();
  AUTO(iter, words_.begin());
  int index = 0;
  for (; iter != words_.end(); ++iter) {
    const std::string& word = iter->first;
    WordInfo& wi = iter->second;

    word_list_.push_back(word);
    wi.WordListIndex = index;
    index++;
  }

  // Fisher-Yates shuffle the word_list_ to evenly spread out words so
  // computationally intensive ones don't clump together in one core.
  int num_words = (int)words_.size();
  for (int i = num_words - 1; i > 0; --i) {
    int j = fastrand() % (i + 1);

    String word1 = word_list_[i];
    WordInfo& word_info1 = words_[word1];

    String word2 = word_list_[j];
    WordInfo& word_info2 = words_[word2];

    word_list_[i] = word2;
    word_info2.WordListIndex = i;
    word_list_[j] = word1;
    word_info1.WordListIndex = j;
  }

  return to_be_pruned.size();
}

void WordCollection::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results) {
  boost::mutex::scoped_lock lock(mutex_);

  // only try correction if we have a sufficiently long enough
  // word to make it worthwhile
  if (prefix.length() < kMinLengthForLevenshteinCompletion)
    return;

  if (trie_enabled_) {
    Algorithm::LevenshteinSearch(prefix, kLevenshteinMaxCost, *trie_, results);
  }
}
