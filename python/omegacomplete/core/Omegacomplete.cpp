#include "stdafx.hpp"

#include "Omegacomplete.hpp"
#include "TagsSet.hpp"
#include "LookupTable.hpp"
#include "Algorithm.hpp"

using namespace std;
using namespace boost;

Omegacomplete* Omegacomplete::instance_ = NULL;
static const std::string kDefaultResponse = "ACK";
static const std::string kUnknownCommandResponse = "UNKNOWN_COMMAND";

void Omegacomplete::InitStatic() {
  // dependencies in other classes that have to initialized first
  LookupTable::InitStatic();
  TagsSet::InitStatic();

  instance_ = new Omegacomplete;
}

int Omegacomplete::NumThreads()
{
  unsigned num_hardware_threads = boost::thread::hardware_concurrency();
  unsigned num_threads = max(num_hardware_threads, (unsigned)2);
  return static_cast<int>(num_threads);
}

Omegacomplete::Omegacomplete()
    : io_service_work_(io_service_),
      is_quitting_(0),
      buffer_contents_follow_(false),
      prev_input_(3),
      is_corrections_only_(false),
      should_autocomplete_(false),
      suffix0_(false),
      suffix1_(false) {
  initCommandDispatcher();

  // hardware_concurrency() can return 0
  for (int i = 0; i < Omegacomplete::NumThreads(); ++i) {
    threads_.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service_));
    mutexes_.push_back(boost::make_shared<boost::mutex>());
  }

  worker_thread_ = boost::thread(&Omegacomplete::workerThreadLoop, this);
}

void Omegacomplete::initCommandDispatcher() {
  using boost::bind;
  using boost::ref;

  AUTO(&m, command_dispatcher_);

  m["current_buffer_id"] = boost::bind(
      &Omegacomplete::cmdCurrentBufferId, ref(*this), _1);

  m["current_buffer_absolute_path"] = boost::bind(
      &Omegacomplete::cmdCurrentBufferAbsolutePath, ref(*this), _1);

  m["current_line"] = boost::bind(
      &Omegacomplete::cmdCurrentLine, ref(*this), _1);

  m["cursor_position"] = boost::bind(
      &Omegacomplete::cmdCursorPosition, ref(*this), _1);

  m["buffer_contents_follow"] = boost::bind(
      &Omegacomplete::cmdBufferContentsFollow, ref(*this), _1);

  m["complete"] = boost::bind(
      &Omegacomplete::cmdComplete, ref(*this), _1);

  m["free_buffer"] = boost::bind(
      &Omegacomplete::cmdFreeBuffer, ref(*this), _1);

  m["current_directory"] = boost::bind(
      &Omegacomplete::cmdCurrentDirectory, ref(*this), _1);

  m["current_tags"] = boost::bind(
      &Omegacomplete::cmdCurrentTags, ref(*this), _1);

  m["taglist_tags"] = boost::bind(
      &Omegacomplete::cmdTaglistTags, ref(*this), _1);

  m["vim_taglist_function"] = boost::bind(
      &Omegacomplete::cmdVimTaglistFunction, ref(*this), _1);

  m["prune"] = boost::bind(
      &Omegacomplete::cmdPrune, ref(*this), _1);

  m["flush_caches"] = boost::bind(
      &Omegacomplete::cmdFlushCaches, ref(*this), _1);

  m["is_corrections_only"] = boost::bind(
      &Omegacomplete::cmdIsCorrectionsOnly, ref(*this), _1);

  m["prune_buffers"] = boost::bind(
      &Omegacomplete::cmdPruneBuffers, ref(*this), _1);

  m["should_autocomplete"] = boost::bind(
      &Omegacomplete::cmdShouldAutocomplete, ref(*this), _1);

  m["config"] = boost::bind(
      &Omegacomplete::cmdConfig, ref(*this), _1);

  m["get_autocomplete"] = boost::bind(
      &Omegacomplete::cmdGetAutocomplete, ref(*this), _1);

  m["set_log_file"] = boost::bind(
      &Omegacomplete::cmdSetLogFile, ref(*this), _1);

  m["start_stopwatch"] = boost::bind(
      &Omegacomplete::cmdStartStopwatch, ref(*this), _1);

  m["stop_stopwatch"] = boost::bind(
      &Omegacomplete::cmdStopStopwatch, ref(*this), _1);
}

Omegacomplete::~Omegacomplete() {
  is_quitting_ = 1;
  worker_thread_.join();
  io_service_.stop();
  threads_.join_all();
}

const std::string Omegacomplete::Eval(const char* request, const int request_len) {
  if (buffer_contents_follow_) {
    StringPtr argument = boost::make_shared<std::string>(
        request, static_cast<size_t>(request_len));

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
    if (index == -1)
      throw std::exception();

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
  response.reserve(8192);
  calculateCompletionCandidates(*argument, response);

  return response;
}

std::string Omegacomplete::cmdFreeBuffer(StringPtr argument) {
  // make sure job queue is empty before we can delete buffer
  while (true) {
    job_queue_mutex_.lock();
    if (job_queue_.size() == 0)
      break;
    job_queue_mutex_.unlock();
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
  boost::split(tags_list, *argument,
               boost::is_any_of(","), boost::token_compress_on);
  foreach (const std::string& tags, tags_list) {
    if (tags.size() == 0)
      continue;

    TagsSet::Instance()->CreateOrUpdate(tags, current_directory_);
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

    TagsSet::Instance()->CreateOrUpdate(tags, current_directory_);
    taglist_tags_.push_back(tags);
  }

  return kDefaultResponse;
}

std::string Omegacomplete::cmdVimTaglistFunction(StringPtr argument) {
  const std::string& response = TagsSet::Instance()->VimTaglistFunction(
      *argument, taglist_tags_, current_directory_);

  return response;
}

std::string Omegacomplete::cmdPrune(StringPtr argument) {
  Words.Prune();

  return kDefaultResponse;
}

std::string Omegacomplete::cmdFlushCaches(StringPtr argument) {
  TagsSet::Instance()->Clear();

  return kDefaultResponse;
}

std::string Omegacomplete::cmdIsCorrectionsOnly(StringPtr argument) {
  const std::string& response = is_corrections_only_ ? "1" : "0";
  return response;
}

std::string Omegacomplete::cmdPruneBuffers(StringPtr argument) {
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

  // create autcompletion string
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
  log_file_.open(*argument, std::ios::app) ;
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

void Omegacomplete::queueParseJob(ParseJob job) {
  boost::mutex::scoped_lock lock(job_queue_mutex_);
  job_queue_.push(job);
  lock.unlock();
  job_queue_conditional_variable_.notify_one();
}

void Omegacomplete::workerThreadLoop() {
  ParseJob job;
  while (!is_quitting_) {
    {
      boost::mutex::scoped_lock lock(job_queue_mutex_);
      while (job_queue_.empty()) {
        const boost::system_time timeout =
            boost::get_system_time() +
            boost::posix_time::milliseconds(5 * 1000);
        job_queue_conditional_variable_.timed_wait(lock, timeout);
      }
      job = job_queue_.front();
      job_queue_.pop();
    }

    // do the job
    boost::mutex::scoped_lock lock(buffers_mutex_);
    if (!Contains(buffers_, job.BufferNumber))
      continue;
    buffers_[job.BufferNumber].ReplaceContentsWith(job.Contents);

    this->Words.Prune();
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
      input.substr(input.size() - 2) == config_["autcomplete_suffix"]) {
    suffix1_ = true;
    autocomplete_input_trigger_ = input;
    should_autocomplete_ = true;
    return;
  } else if (!suffix0_ &&
             input[input.size() - 1] == config_["autcomplete_suffix"][0]) {
    suffix0_ = true;
    autocomplete_completions_ = prev_completions_;
  } else {
    suffix0_ = suffix1_ = false;
  }

  // keep a trailing list of previous inputs
  prev_input_[2] = prev_input_[1];
  prev_input_[1] = prev_input_[0];
  prev_input_[0] = input;

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

  int num_threads = Omegacomplete::NumThreads();
  Completions completions;

  completions.Items = boost::make_shared<CompleteItemVector>();
  Words.Lock();

  AUTO(const & word_list, Words.GetWordList());

  for (int i = 0; i < num_threads; ++i) {
    mutexes_[i]->lock();
    const int begin = ((i + 0) * (unsigned)word_list.size()) / num_threads;
    const int end = ((i + 1) * (unsigned)word_list.size()) / num_threads;
    io_service_.post(boost::bind(
            Algorithm::ProcessWords,
            boost::ref(completions),
            mutexes_[i],
            boost::cref(word_list), begin, end, input, false));
  }

  for (int i = 0; i < num_threads; ++i) {
    mutexes_[i]->lock();
    mutexes_[i]->unlock();
  }
  Words.Unlock();

  /*
  std::set<std::string> added_words;
  added_words.insert(input);
  // this is to prevent completions from being considered
  // that are basically the word from before you press Backspace.
  if (prev_input_[1].size() == (input.size() + 1) &&
      boost::starts_with(prev_input_[1], input)) {
    added_words.insert(prev_input_[1]);
  }


  // terminus_prefix is only filled in if terminus_mode would be set to true
  bool terminus_mode;
  std::string terminus_prefix;
  terminus_mode = shouldEnableTerminusMode(input, terminus_prefix);
  if (terminus_mode) {
    // don't want to get the word that is currently displayed on screen
    added_words.insert(input);
    input = terminus_prefix;
  }

retry_completion:
  Words.GetAbbrCompletions(
      input,
      completions, added_words,
      terminus_mode);

  TagsSet::Instance()->GetAbbrCompletions(
      input,
      current_tags_, current_directory_,
      completions, added_words,
      terminus_mode);

  Words.GetPrefixCompletions(
      input,
      completions, added_words,
      terminus_mode);

  TagsSet::Instance()->GetPrefixCompletions(
      input,
      current_tags_, current_directory_,
      completions, added_words,
      terminus_mode);

  if (terminus_mode && completions->size() == 0) {
    terminus_mode = false;
    input = input + "_";
    goto retry_completion;
  }

  // assign quick match number of entries
  for (size_t i = 0; i < LookupTable::kMaxNumQuickMatch; ++i) {
    if (i >= completions->size())
      break;

    CompleteItem& item = (*completions)[i];
    item.Menu = lexical_cast<string>(LookupTable::QuickMatchKey[i]) +
        " " + item.Menu;
  }

  */

  addLevenshteinCorrections(input, completions.Items);

  std::sort(completions.Items->begin(), completions.Items->end());

  result += "[";
  foreach (const CompleteItem& completion, *completions.Items) {
    result += completion.SerializeToVimDict();
  }
  result += "]";

  prev_completions_ = completions.Items;

  if (completions.Items->size() > 0)
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

bool Omegacomplete::shouldEnableTerminusMode(
    const std::string& word, std::string& prefix) {
  if (word.size() < 2)
    return false;

  if (word[word.size() - 1] == '_') {
    prefix = word;
    prefix.resize(word.size() - 1);
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
      if (boost::ends_with(word, "-"))
        continue;

      CompleteItem completion(word);
      completions->push_back(completion);
    }
  }
}
