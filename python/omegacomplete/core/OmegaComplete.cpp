#include "stdafx.hpp"

#include "OmegaComplete.hpp"
#include "TagsSet.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"
#include "Teleprompter.hpp"
#include "Algorithm.hpp"

OmegaComplete* OmegaComplete::instance_ = NULL;
const std::string OmegaComplete::default_response_ = "ACK";

void OmegaComplete::InitStatic()
{
    // dependencies in other classes that have to initialized first
    LookupTable::InitStatic();
    TagsSet::InitStatic();
    Algorithm::InitStatic();
#if defined (_WIN32) && defined (TELEPROMPTER)
    Teleprompter::InitStatic();
#endif

    instance_ = new OmegaComplete;
}

OmegaComplete::OmegaComplete()
:
is_quitting_(0),
buffer_contents_follow_(false),
prev_input_(3),
is_corrections_only_(false)
{
    initCommandDispatcher();

    worker_thread_ = boost::thread(
        &OmegaComplete::workerThreadLoop,
        this);

#ifdef ENABLE_CLANG_COMPLETION
    clang_.Init();
#endif
}

void OmegaComplete::initCommandDispatcher()
{
    command_dispatcher_["current_buffer_id"] = boost::bind(
        &OmegaComplete::cmdCurrentBufferId, boost::ref(*this), _1);

    command_dispatcher_["current_buffer_absolute_path"] = boost::bind(
        &OmegaComplete::cmdCurrentBufferAbsolutePath, boost::ref(*this), _1);

    command_dispatcher_["current_line"] = boost::bind(
        &OmegaComplete::cmdCurrentLine, boost::ref(*this), _1);

    command_dispatcher_["cursor_position"] = boost::bind(
        &OmegaComplete::cmdCursorPosition, boost::ref(*this), _1);

    command_dispatcher_["buffer_contents_follow"] = boost::bind(
        &OmegaComplete::cmdBufferContentsFollow, boost::ref(*this), _1);

    command_dispatcher_["complete"] = boost::bind(
        &OmegaComplete::cmdComplete, boost::ref(*this), _1);

    command_dispatcher_["free_buffer"] = boost::bind(
        &OmegaComplete::cmdFreeBuffer, boost::ref(*this), _1);

    command_dispatcher_["current_directory"] = boost::bind(
        &OmegaComplete::cmdCurrentDirectory, boost::ref(*this), _1);

    command_dispatcher_["current_tags"] = boost::bind(
        &OmegaComplete::cmdCurrentTags, boost::ref(*this), _1);

    command_dispatcher_["taglist_tags"] = boost::bind(
        &OmegaComplete::cmdTaglistTags, boost::ref(*this), _1);

    command_dispatcher_["vim_taglist_function"] = boost::bind(
        &OmegaComplete::cmdVimTaglistFunction, boost::ref(*this), _1);

    command_dispatcher_["prune"] = boost::bind(
        &OmegaComplete::cmdPrune, boost::ref(*this), _1);

    command_dispatcher_["hide_teleprompter"] = boost::bind(
        &OmegaComplete::cmdHideTeleprompter, boost::ref(*this), _1);

    command_dispatcher_["flush_caches"] = boost::bind(
        &OmegaComplete::cmdFlushCaches, boost::ref(*this), _1);

    command_dispatcher_["is_corrections_only"] = boost::bind(
        &OmegaComplete::cmdIsCorrectionsOnly, boost::ref(*this), _1);

    command_dispatcher_["prune_buffers"] = boost::bind(
        &OmegaComplete::cmdPruneBuffers, boost::ref(*this), _1);
}

OmegaComplete::~OmegaComplete()
{
    is_quitting_ = 1;
    worker_thread_.join();
}

const std::string OmegaComplete::Eval(const char* request, const int request_len)
{
    if (buffer_contents_follow_)
    {
        StringPtr argument = boost::make_shared<std::string>(
            request, static_cast<size_t>(request_len));

        return queueBufferContents(argument);
    }
    else
    {
        // find first space
        int index = -1;
        for (int ii = 0; ii < request_len; ++ii)
        {
            if (request[ii] == ' ')
            {
                index = ii;
                break;
            }
        }

        if (index == -1)
            throw std::exception();

        // break it up into a "request" string and a "argument"
        std::string command(request, static_cast<size_t>(index));
        StringPtr argument = boost::make_shared<std::string>(
            request + index + 1,
            static_cast<size_t>(request_len - command.size() - 1));

        auto(iter, command_dispatcher_.find(command));
        if (iter == command_dispatcher_.end())
        {
            std::cout << boost::str(boost::format(
                "unknown command %s %s") % command % *argument);

            return default_response_;
        }
        else
        {
            return iter->second(argument);
        }
    }
}

std::string OmegaComplete::cmdCurrentBufferId(StringPtr argument)
{
    current_buffer_id_ = boost::lexical_cast<unsigned>(*argument);
    if (Contains(buffers_, current_buffer_id_) == false)
    {
        buffers_[current_buffer_id_].Init(this, current_buffer_id_);
    }

    return default_response_;
}

std::string OmegaComplete::cmdCurrentBufferAbsolutePath(StringPtr argument)
{
    current_buffer_absolute_path_ = *argument;

    return default_response_;
}

std::string OmegaComplete::cmdCurrentLine(StringPtr argument)
{
    current_line_ = *argument;

    return default_response_;
}

std::string OmegaComplete::cmdCursorPosition(StringPtr argument)
{
    std::vector<std::string> position;
    boost::split(
        position,
        *argument,
        boost::is_any_of(" "),
        boost::token_compress_on);
    unsigned x = boost::lexical_cast<unsigned>(position[0]);
    unsigned y = boost::lexical_cast<unsigned>(position[1]);
    cursor_pos_.Line = x; cursor_pos_.Column = y;

    buffers_[current_buffer_id_].CalculateCurrentWordOfCursor(
        current_line_,
        cursor_pos_);

    return default_response_;
}

std::string OmegaComplete::cmdBufferContentsFollow(StringPtr argument)
{
    buffer_contents_follow_ = true;

    return default_response_;
}

std::string OmegaComplete::queueBufferContents(StringPtr argument)
{
    buffer_contents_follow_ = false;

    current_contents_ = argument;

    ParseJob job(current_buffer_id_, current_contents_);
    queueParseJob(job);

#ifdef ENABLE_CLANG_COMPLETION
    clang_.CreateOrUpdate(current_buffer_absolute_path_, current_contents_);
#endif

    return default_response_;
}

std::string OmegaComplete::cmdComplete(StringPtr argument)
{
    //Stopwatch watch; watch.Start();
    std::string response;
    response.reserve(8192);
    calculateCompletionCandidates(*argument, response);
    //watch.Stop(); std::cout << "complete: "; watch.PrintResultMilliseconds();

    return response;
}

std::string OmegaComplete::cmdFreeBuffer(StringPtr argument)
{
    // make sure job queue is empty before we can delete buffer
    while (true) {
        job_queue_mutex_.lock();
        if (job_queue_.size() == 0) {
            break;
        }
        job_queue_mutex_.unlock();
    }

    buffers_mutex_.lock();
    buffers_.erase(boost::lexical_cast<unsigned>(*argument));
    buffers_mutex_.unlock();
    job_queue_mutex_.unlock();

    return default_response_;
}

std::string OmegaComplete::cmdCurrentDirectory(StringPtr argument)
{
    current_directory_ = *argument;

    return default_response_;
}

std::string OmegaComplete::cmdCurrentTags(StringPtr argument)
{
    current_tags_.clear();

    std::vector<std::string> tags_list;
    boost::split(tags_list, *argument,
                    boost::is_any_of(","), boost::token_compress_on);
    foreach (const std::string& tags, tags_list)
    {
        if (tags.size() == 0) continue;

        TagsSet::Instance()->CreateOrUpdate(tags, current_directory_);
        current_tags_.push_back(tags);
    }

    return default_response_;
}

std::string OmegaComplete::cmdTaglistTags(StringPtr argument)
{
    taglist_tags_.clear();

    std::vector<std::string> tags_list;
    boost::split(tags_list, *argument,
                    boost::is_any_of(","), boost::token_compress_on);
    foreach (const std::string& tags, tags_list)
    {
        if (tags.size() == 0) continue;

        TagsSet::Instance()->CreateOrUpdate(tags, current_directory_);
        taglist_tags_.push_back(tags);
    }

    return default_response_;
}

std::string OmegaComplete::cmdVimTaglistFunction(StringPtr argument)
{
    const std::string& response = TagsSet::Instance()->VimTaglistFunction(
        *argument, taglist_tags_, current_directory_);

    return response;
}

std::string OmegaComplete::cmdPrune(StringPtr argument)
{

    WordSet.Prune();
    //std::cout << count << " words pruned" << std::endl;

    return default_response_;
}

std::string OmegaComplete::cmdHideTeleprompter(StringPtr argument)
{
#ifdef TELEPROMPTER
        Teleprompter::Instance()->Show(false);
#endif
    return default_response_;
}

std::string OmegaComplete::cmdFlushCaches(StringPtr argument)
{
    TagsSet::Instance()->Clear();
    Algorithm::ClearGlobalCache();

    return default_response_;
}

std::string OmegaComplete::cmdIsCorrectionsOnly(StringPtr argument)
{
    const std::string& response = is_corrections_only_ ? "1" : "0";
    return response;
}

std::string OmegaComplete::cmdPruneBuffers(StringPtr argument)
{
    std::vector<std::string> numbers;
    boost::split(
        numbers, *argument,
        boost::is_any_of(","), boost::token_compress_on);
    std::set<unsigned> valid_buffers;
    foreach (const std::string& number, numbers) {
        valid_buffers.insert(boost::lexical_cast<unsigned>(number));
    }

    buffers_mutex_.lock();

    std::vector<unsigned> to_be_erased;
    auto (iter, buffers_.begin());
    for (; iter != buffers_.end(); ++iter) {
        unsigned num = iter->second.GetBufferId();
        if (!Contains(valid_buffers, num))
            to_be_erased.push_back(num);
    }

    foreach (unsigned num, to_be_erased) {
        buffers_.erase(num);
    }

    buffers_mutex_.unlock();

    return default_response_;
}

void OmegaComplete::queueParseJob(ParseJob job)
{
    boost::mutex::scoped_lock lock(job_queue_mutex_);
    job_queue_.push(job);
    lock.unlock();
    job_queue_conditional_variable_.notify_all();
}

void OmegaComplete::workerThreadLoop()
{
    while (!is_quitting_) {
        ParseJob job;
        {
            boost::mutex::scoped_lock lock(job_queue_mutex_);
            while (job_queue_.empty()) {
                job_queue_conditional_variable_.wait(lock);
            }
            job = job_queue_.front();
            job_queue_.pop();
        }

        // do the job
        boost::mutex::scoped_lock lock(buffers_mutex_);
        if (!Contains(buffers_, job.BufferNumber))
            continue;
        buffers_[job.BufferNumber].ReplaceContentsWith(job.Contents);

        this->WordSet.Prune();
    }
}

void OmegaComplete::calculateCompletionCandidates(
    const std::string& line,
    std::string& result)
{
#ifdef ENABLE_CLANG_COMPLETION
    if (boost::ends_with(current_buffer_absolute_path_, ".cpp"))
    {
        clang_.DoCompletion(
            current_buffer_absolute_path_,
            line,
            cursor_pos_,
            current_contents_,
            result);
    }
#endif

    genericKeywordCompletion(line, result);
}

void OmegaComplete::genericKeywordCompletion(
    const std::string& line,
    std::string& result)
{
    std::string input = getWordToComplete(line);
    if (input.empty())
        return;

    // keep a trailing list of previous inputs
    prev_input_[2] = prev_input_[1];
    prev_input_[1] = prev_input_[0];
    prev_input_[0] = input;

    unsigned disambiguate_index = UINT_MAX;
    bool disambiguate_mode = shouldEnableDisambiguateMode(input, disambiguate_index);
    if (disambiguate_mode &&
        prev_completions_ &&
        disambiguate_index < prev_completions_->size())
    {
        const CompleteItem& completion = (*prev_completions_)[disambiguate_index];
        result += "[";
        result += completion.SerializeToVimDict();
        result += "]";
        return;
    }

    CompleteItemVectorPtr completions = boost::make_shared<CompleteItemVector>();
    std::set<std::string> added_words;
    added_words.insert(input);
    // this is to prevent completions from being considered
    // that are basically the word from before you press Backspace.
    if (prev_input_[1].size() == (input.size() + 1) &&
        boost::starts_with(prev_input_[1], input))
    {
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
    WordSet.GetAbbrCompletions(
        input,
        completions, added_words,
        terminus_mode);

    TagsSet::Instance()->GetAbbrCompletions(
        input,
        current_tags_, current_directory_,
        completions, added_words,
        terminus_mode);

    WordSet.GetPrefixCompletions(
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
        item.Menu =
            boost::lexical_cast<std::string>(LookupTable::QuickMatchKey[i]) +
            " " + item.Menu;
    }

    addLevenshteinCorrections(input, completions, added_words);

    result += "[";
    foreach (const CompleteItem& completion, *completions) {
        result += completion.SerializeToVimDict();
    }
    result += "]";

    prev_completions_ = completions;
}

std::string OmegaComplete::getWordToComplete(const std::string& line)
{
    if (line.length() == 0)
        return "";

    int partial_end = line.length();
    int partial_begin = partial_end - 1;
    for (; partial_begin >= 0; --partial_begin) {
        char c = line[partial_begin];
        if (LookupTable::IsPartOfWord[c])
            continue;

        break;
    }

    if ((partial_begin + 1) == partial_end)
        return "";

    std::string partial( &line[partial_begin + 1], &line[partial_end] );
    return partial;
}

bool OmegaComplete::shouldEnableDisambiguateMode(
    const std::string& word, unsigned& index)
{
    if (word.size() < 2)
        return false;

    char last = word[word.size() - 1];
    if (LookupTable::IsNumber[last]) {
        if (Contains(LookupTable::ReverseQuickMatch, last))
            index = LookupTable::ReverseQuickMatch[last];
        return true;
    }

    return false;
}

bool OmegaComplete::shouldEnableTerminusMode(
    const std::string& word, std::string& prefix)
{
    if (word.size() < 2)
        return false;

    if (word[word.size() - 1] == '_') {
        prefix = word;
        prefix.resize(word.size() - 1);
        return true;
    }

    return false;
}

void OmegaComplete::addLevenshteinCorrections(
    const std::string& input,
    CompleteItemVectorPtr& completions,
    std::set<std::string>& added_words)
{
    if (completions->size() == 0) {
        is_corrections_only_ = true;
    } else {
        is_corrections_only_ = false;
        return;
    }

    LevenshteinSearchResults levenshtein_completions;
    WordSet.GetLevenshteinCompletions(input, levenshtein_completions);

    auto (iter, levenshtein_completions.begin());
    for (; iter != levenshtein_completions.end(); ++iter) {
        foreach (const std::string& word, iter->second) {
            if (completions->size() >= LookupTable::kMaxNumCompletions)
                return;

            if (word == input)
                continue;
            if (boost::starts_with(input, word))
                continue;
            if (Contains(added_words, word))
                continue;

            CompleteItem completion(word);
            completions->push_back(completion);
        }
    }
}
