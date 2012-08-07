#include "stdafx.hpp"

#include "Session.hpp"
#include "TagsSet.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"
#include "CompletionSet.hpp"
#include "Teleprompter.hpp"
#include "Algorithm.hpp"

unsigned int Session::connection_ticket_ = 0;
std::vector<char> Session::QuickMatchKey;
boost::unordered_map<char, unsigned> Session::ReverseQuickMatch;

void Session::GlobalInit()
{
    QuickMatchKey.resize(CompletionSet::kMaxNumCompletions, ' '),

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

    for (unsigned ii = 0; ii < 10; ++ii)
    {
        ReverseQuickMatch[QuickMatchKey[ii]] = ii;
    }
}

Session::Session(boost::asio::io_service& io_service, Room& room)
:
socket_(io_service),
room_(room),
connection_number_(connection_ticket_++),
is_quitting_(0),
prev_input_(3),
is_corrections_only_(false)
{
    command_dispatcher_["current_buffer_id"] = boost::bind(
        &Session::cmdCurrentBufferId, boost::ref(*this), _1);

    command_dispatcher_["current_buffer_absolute_path"] = boost::bind(
        &Session::cmdCurrentBufferAbsolutePath, boost::ref(*this), _1);

    command_dispatcher_["current_line"] = boost::bind(
        &Session::cmdCurrentLine, boost::ref(*this), _1);

    command_dispatcher_["cursor_position"] = boost::bind(
        &Session::cmdCursorPosition, boost::ref(*this), _1);

    command_dispatcher_["buffer_contents"] = boost::bind(
        &Session::cmdBufferContents, boost::ref(*this), _1);

    command_dispatcher_["complete"] = boost::bind(
        &Session::cmdComplete, boost::ref(*this), _1);

    command_dispatcher_["free_buffer"] = boost::bind(
        &Session::cmdFreeBuffer, boost::ref(*this), _1);

    command_dispatcher_["current_directory"] = boost::bind(
        &Session::cmdCurrentDirectory, boost::ref(*this), _1);

    command_dispatcher_["current_tags"] = boost::bind(
        &Session::cmdCurrentTags, boost::ref(*this), _1);

    command_dispatcher_["taglist_tags"] = boost::bind(
        &Session::cmdTaglistTags, boost::ref(*this), _1);

    command_dispatcher_["vim_taglist_function"] = boost::bind(
        &Session::cmdVimTaglistFunction, boost::ref(*this), _1);

    command_dispatcher_["prune"] = boost::bind(
        &Session::cmdPrune, boost::ref(*this), _1);

    command_dispatcher_["hide_teleprompter"] = boost::bind(
        &Session::cmdHideTeleprompter, boost::ref(*this), _1);

    command_dispatcher_["flush_caches"] = boost::bind(
        &Session::cmdFlushCaches, boost::ref(*this), _1);

    command_dispatcher_["is_corrections_only"] = boost::bind(
        &Session::cmdIsCorrectionsOnly, boost::ref(*this), _1);
}

Session::~Session()
{
    is_quitting_ = 1;
    worker_thread_.join();

    std::cout << boost::str(boost::format(
        "session %u destroyed\n") % connection_number_);
}

void Session::Start()
{
    worker_thread_ = boost::thread(
        &Session::workerThreadLoop,
        this);

    clang_.Init();

    std::cout << "session started, connection number: " << connection_number_ << "\n";
    socket_.set_option(boost::asio::ip::tcp::no_delay(true));
    room_.Join(shared_from_this());

    asyncReadHeader();
}

void Session::asyncReadHeader()
{
    async_read(
        socket_,
        boost::asio::buffer(request_header_, 4),
        boost::bind(
            &Session::handleReadHeader,
            this,
            boost::asio::placeholders::error));
}

void Session::handleReadHeader(const boost::system::error_code& error)
{
    if (error)
    {
        room_.Leave(shared_from_this());

        LogAsioError(error, "failed to handleReadHeader()");
        return;
    }

    // this probably isn't totally portable, but whatever
    unsigned body_size = *( reinterpret_cast<unsigned*>(request_header_) );
    request_body_.resize(body_size, '\0');

    async_read(
        socket_,
        boost::asio::buffer(&request_body_[0], body_size),
        boost::bind(
            &Session::handleReadRequest,
            this,
            boost::asio::placeholders::error));
}

void Session::handleReadRequest(const boost::system::error_code& error)
{
    if (error)
    {
        room_.Leave(shared_from_this());

        LogAsioError(error, "failed to handleReadRequest()");
        return;
    }

    processClientMessage();
}

void Session::processClientMessage()
{
    const std::string& request = request_body_;

    // find first space
    int index = -1;
    if (request.size() > INT_MAX) throw std::exception();
    for (size_t ii = 0; ii < request.size(); ++ii)
    {
        if (request[ii] == ' ')
        {
            index = static_cast<int>(ii);
            break;
        }
    }

    if (index == -1) throw std::exception();

    // break it up into a "request" string and a "argument"
    std::string command(request.begin(), request.begin() + index);
    StringPtr argument = boost::make_shared<std::string>(
        request.begin() + index + 1, request.end());

    //std::cout << command << " " << *argument << "$" << std::endl;
    
    auto(iter, command_dispatcher_.find(command));
    if (iter == command_dispatcher_.end())
    {
        std::cout << boost::str(boost::format(
            "unknown command %s %s") % command % *argument);

        writeResponse("ACK");
    }
    else
    {
        iter->second(argument);
    }
}

void Session::cmdCurrentBufferId(StringPtr argument)
{
    writeResponse("ACK");

    current_buffer_id_ = boost::lexical_cast<unsigned>(*argument);
    if (Contains(buffers_, current_buffer_id_) == false)
    {
        buffers_[current_buffer_id_].Init(this, current_buffer_id_);
    }
}

void Session::cmdCurrentBufferAbsolutePath(StringPtr argument)
{
    writeResponse("ACK");

    current_buffer_absolute_path_ = *argument;
}

void Session::cmdCurrentLine(StringPtr argument)
{
    writeResponse("ACK");

    current_line_ = *argument;
}

void Session::cmdCursorPosition(StringPtr argument)
{
    writeResponse("ACK");

    std::vector<std::string> position;
    boost::split(
        position,
        *argument,
        boost::is_any_of(" "),
        boost::token_compress_on);
    unsigned x = boost::lexical_cast<unsigned>(position[0]);
    unsigned y = boost::lexical_cast<unsigned>(position[1]);
    cursor_pos_.first = x; cursor_pos_.second = y;

    buffers_[current_buffer_id_].CalculateCurrentWordOfCursor(
        current_line_,
        cursor_pos_);
}

void Session::cmdBufferContents(StringPtr argument)
{
    writeResponse("ACK");

    ParseJob job(current_buffer_id_, argument);
    queueParseJob(job);

    clang_.CreateOrUpdate(current_buffer_absolute_path_, argument);
}

void Session::cmdComplete(StringPtr argument)
{
    //Stopwatch watch; watch.Start();

    std::string response;
    response.reserve(8192);
    calculateCompletionCandidates(*argument, response);

    writeResponse(response);

    //watch.Stop(); std::cout << "complete: "; watch.PrintResultMilliseconds();
}

void Session::cmdFreeBuffer(StringPtr argument)
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

    // block until buffer is deleted
    writeResponse("ACK");
}

void Session::cmdCurrentDirectory(StringPtr argument)
{
    writeResponse("ACK");

    current_directory_ = *argument;
}

void Session::cmdCurrentTags(StringPtr argument)
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

    // block server ACK until above is done
    writeResponse("ACK");
}

void Session::cmdTaglistTags(StringPtr argument)
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

    // block server ACK until above is done
    writeResponse("ACK");
}

void Session::cmdVimTaglistFunction(StringPtr argument)
{
    std::string response = TagsSet::Instance()->VimTaglistFunction(
        *argument, taglist_tags_, current_directory_);

    writeResponse(response);
}

void Session::cmdPrune(StringPtr argument)
{
    writeResponse("ACK");

    unsigned words_pruned = WordSet.Prune();
    //std::cout << count << " words pruned" << std::endl;
}

void Session::cmdHideTeleprompter(StringPtr argument)
{
    writeResponse("ACK");

#ifdef TELEPROMPTER
        Teleprompter::Instance()->Show(false);
#endif
}

void Session::cmdFlushCaches(StringPtr argument)
{
    TagsSet::Instance()->Clear();
    Algorithm::ClearGlobalCache();

    writeResponse("ACK");
}

void Session::cmdIsCorrectionsOnly(StringPtr argument)
{
    const std::string& response = is_corrections_only_ ? "1" : "0";
    writeResponse(response);
}

void Session::writeResponse(const std::string& response)
{
    // note that we MUST perform synchronous writes. if async_write() is used
    // instead it is possible that writing large responses (e.g. > 65536 bytes)
    // takes multiple background async_write()s and the NULL character gets
    // written before we are done writing the actual response!

    boost::asio::write(
        socket_,
        boost::asio::buffer(&response[0], response.size()));

    std::string null_byte(1, '\0');
    boost::asio::write(
        socket_,
        boost::asio::buffer(&null_byte[0], null_byte.size()));

    asyncReadHeader();
}

void Session::handleWriteResponse(const boost::system::error_code& error)
{
    if (error)
    {
        room_.Leave(shared_from_this());

        LogAsioError(error, "failed in handleWriteResponse()");
        return;
    }
}

void Session::queueParseJob(ParseJob job)
{
    job_queue_mutex_.lock();
    job_queue_.push_back(job);
    job_queue_mutex_.unlock();
}

void Session::workerThreadLoop()
{
#ifdef _WIN32
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
#endif

    bool did_prune = false;

    while (true)
    {
#ifdef _WIN32
        // this is in milliseconds
        ::Sleep(1);
#else
        ::usleep(1 * 1000);
#endif
        if (is_quitting_ == 1)
            break;

        // pop off the next job
try_get_next_job:
        job_queue_mutex_.lock();
        if (job_queue_.size() > 0)
        {
            ParseJob job(job_queue_.front());
            job_queue_.pop_front();
            job_queue_mutex_.unlock();

            // do the job
            buffers_mutex_.lock();
            if (Contains(buffers_, job.BufferNumber) == false)
            {
                std::cout << "buffer " << job.BufferNumber
                          << "no longer exists! "
                          << "skipping this job" << std::endl;
                continue;
            }
            buffers_[job.BufferNumber].ReplaceContentsWith(job.Contents);
            buffers_mutex_.unlock();

            did_prune = false;

            // try and see if there is something queued up if so, then
            // immediately process it, don't go to sleep
            goto try_get_next_job;
        }
        else
        {
            job_queue_mutex_.unlock();

            if (did_prune == false) {
                WordSet.Prune();
                did_prune = true;
            }

            continue;
        }
    }
}

void Session::calculateCompletionCandidates(
    const std::string& line,
    std::string& result)
{
    std::string prefix_to_complete = getWordToComplete(line);
    if (prefix_to_complete.empty())
    {
#ifdef TELEPROMPTER
        Teleprompter::Instance()->Show(false);
#endif
        return;
    }

    // keep a trailing list of previous inputs
    prev_input_[2] = prev_input_[1];
    prev_input_[1] = prev_input_[0];
    prev_input_[0] = prefix_to_complete;

    bool terminus_mode = shouldEnableTerminusMode(prefix_to_complete);
    std::string terminus_prefix;

    bool disambiguate_mode = shouldEnableDisambiguateMode(prefix_to_complete);
    char disambiguate_letter = 0;
    unsigned disambiguate_index = UINT_MAX;
    String orig_disambiguate_prefix;
    if (terminus_mode)
    {
        terminus_prefix = prefix_to_complete;
        terminus_prefix.resize( prefix_to_complete.size() - 1 );
    }
    else if (disambiguate_mode)
    {
        orig_disambiguate_prefix = prefix_to_complete;

        disambiguate_letter = prefix_to_complete[ prefix_to_complete.size() - 1 ];

        prefix_to_complete.resize( prefix_to_complete.size() - 1 );

        if (Contains(ReverseQuickMatch, disambiguate_letter) == true)
            disambiguate_index = ReverseQuickMatch[disambiguate_letter];
    }

    CompletionSet main_completion_set;
    CompletionSet terminus_completion_set;

retry_completion:

    if (terminus_mode)
    {
        always_assert(disambiguate_mode == false);
        fillCompletionSet(terminus_prefix, terminus_completion_set);
    }
    std::vector<CompleteItem> banned_words;
    if (disambiguate_mode)
    {
        banned_words.push_back(orig_disambiguate_prefix);
    }
    fillCompletionSet(prefix_to_complete, main_completion_set, &banned_words);

    // encountered an invalid disambiguation, abort and retry normally
    // as though it was not intended
    //
    // for example, should2 would falsely trigger 'disambiguate_mode'
    // but we probably want to try it as a regular prefix completion
    if (disambiguate_mode &&
        disambiguate_index >= 0 &&
        disambiguate_index >= main_completion_set.GetNumCompletions())
    {
        disambiguate_mode = false;
        disambiguate_index = UINT_MAX;

        main_completion_set.Clear();

        prefix_to_complete += boost::lexical_cast<std::string>(disambiguate_letter);

        goto retry_completion;
    }

    // only if we have no completions do we try to Levenshtein distance completion
    LevenshteinSearchResults levenshtein_completions;
    if (main_completion_set.GetNumCompletions() == 0)
    {
        WordSet.GetLevenshteinCompletions(
            prefix_to_complete,
            levenshtein_completions);

        is_corrections_only_ = true;
    }
    else
    {
        is_corrections_only_ = false;
    }

    std::vector<CompleteItem> result_list;
    std::vector<CompleteItem> terminus_result_list;

    if (terminus_mode)
    {
        terminus_completion_set.FillResults(terminus_result_list);
    }

    // this is to prevent completions from being considered
    // that are basically the word from before you press Backspace.
    if (prev_input_[1].size() == (prefix_to_complete.size() + 1) &&
        boost::starts_with(prev_input_[1], prefix_to_complete))
    {
        main_completion_set.AddBannedWord(prev_input_[1]);
        if (terminus_mode)
        {
            terminus_completion_set.AddBannedWord(prev_input_[1]);
        }
    }

    if (terminus_mode)
    {
        foreach (const CompleteItem& completion, terminus_result_list)
        {
            const std::string& word = completion.Word;
            if (word[word.size() - 1] == '_')
                main_completion_set.AddBannedWord(word);
        }
    }

    main_completion_set.FillResults(result_list);

    // convert to format that VIM expects, basically a list of dictionaries
#ifdef TELEPROMPTER
    Teleprompter::Instance()->Show(true);
    Teleprompter::Instance()->Clear();
    Teleprompter::Instance()->SetCurrentWord(prefix_to_complete);
#endif

    result += "[";
    if (disambiguate_mode == false)
    {
        if (terminus_mode)
        {
            foreach (const CompleteItem& completion, terminus_result_list)
            {
                const std::string& word = completion.Word;
                if (word[ word.size() - 1 ] != '_')
                    continue;
                if (word == prefix_to_complete)
                    continue;

                result += completion.SerializeToVimDict();
            }

#ifdef TELEPROMPTER
            Teleprompter::Instance()->AppendText(terminus_result_list);
#endif
        }

        foreach (const CompleteItem& completion, result_list)
        {
            result += completion.SerializeToVimDict();
        }
            
#ifdef TELEPROMPTER
        Teleprompter::Instance()->AppendText(result_list);
#endif

        main_completion_set.FillLevenshteinResults(
            levenshtein_completions,
            prefix_to_complete,
            result);
    }
    else
    {
        // make sure we actually have a result, or we crash
        if (result_list.size() > disambiguate_index)
        {
            CompleteItem single_result = result_list[disambiguate_index];
            single_result.Abbr = single_result.Word + " <==";
            result += single_result.SerializeToVimDict();

#ifdef TELEPROMPTER
            Teleprompter::Instance()->AppendText(
                single_result + "    <==");
#endif
        }
    }
    result += "]";

#ifdef TELEPROMPTER
    Teleprompter::Instance()->Redraw();
#endif
}


std::string Session::getWordToComplete(const std::string& line)
{
    if (line.length() == 0) return "";

    int partial_end = line.length();
    int partial_begin = partial_end - 1;
    for (; partial_begin >= 0; --partial_begin)
    {
        char c = line[partial_begin];
        if (LookupTable::IsPartOfWord[c])
            continue;

        break;
    }

    if ((partial_begin + 1) == partial_end) return "";

    std::string partial( &line[partial_begin + 1], &line[partial_end] );
    return partial;
}

bool Session::shouldEnableDisambiguateMode(const std::string& word)
{
    if (word.size() < 2) return false;
    if ( !LookupTable::IsNumber[ word[word.size() - 1] ] )
        return false;

    return true;
}

bool Session::shouldEnableTerminusMode(const std::string& word)
{
    if (word.size() < 2) return false;

    if (word[ word.size() - 1 ] == '_')
        return true;

    return false;
}

void Session::fillCompletionSet(
    const std::string& prefix_to_complete,
    CompletionSet& completion_set,
    const std::vector<CompleteItem>* banned_words)
{
    this->WordSet.GetAbbrCompletions(
        prefix_to_complete,
        &completion_set.AbbrCompletions);

    TagsSet::Instance()->GetAbbrCompletions(
        prefix_to_complete, current_tags_, current_directory_,
        &completion_set.TagsAbbrCompletions);

    this->WordSet.GetPrefixCompletions(
        prefix_to_complete,
        &completion_set.PrefixCompletions);

    TagsSet::Instance()->GetAllWordsWithPrefix(
        prefix_to_complete, current_tags_, current_directory_,
        &completion_set.TagsPrefixCompletions);

    CompleteItem repeat(prefix_to_complete);
    completion_set.AbbrCompletions.erase(repeat);
    completion_set.TagsAbbrCompletions.erase(repeat);
    completion_set.PrefixCompletions.erase(repeat);
    completion_set.TagsPrefixCompletions.erase(repeat);

    if (banned_words == NULL)
        return;

    foreach (const CompleteItem& completion, *banned_words)
    {
        completion_set.AbbrCompletions.erase(completion);
        completion_set.TagsAbbrCompletions.erase(completion);
        completion_set.PrefixCompletions.erase(completion);
        completion_set.TagsPrefixCompletions.erase(completion);
    }
}
