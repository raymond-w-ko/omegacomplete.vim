#include "stdafx.hpp"

#include "Session.hpp"
#include "TagsSet.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"
#include "CompletionSet.hpp"

unsigned int Session::connection_ticket_ = 0;
std::vector<char> Session::QuickMatchKey;
boost::unordered_map<char, unsigned> Session::ReverseQuickMatch;

void Session::GlobalInit()
{
    QuickMatchKey.resize(CompletionSet::kMaxNumCompletions, ' '),

    QuickMatchKey[0] = 'A';
    QuickMatchKey[1] = 'S';
    QuickMatchKey[2] = 'D';
    QuickMatchKey[3] = 'F';
    QuickMatchKey[4] = 'G';
    QuickMatchKey[5] = 'H';
    QuickMatchKey[6] = 'J';
    QuickMatchKey[7] = 'K';
    QuickMatchKey[8] = 'L';
    QuickMatchKey[9] = ';';

    QuickMatchKey[10] = 'Q';
    QuickMatchKey[11] = 'W';
    QuickMatchKey[12] = 'E';
    QuickMatchKey[13] = 'R';
    QuickMatchKey[14] = 'T';
    QuickMatchKey[15] = 'Y';
    QuickMatchKey[16] = 'U';
    QuickMatchKey[17] = 'I';
    QuickMatchKey[18] = 'O';
    QuickMatchKey[19] = 'P';

    for (unsigned ii = 0; ii < 20; ++ii)
    {
        ReverseQuickMatch[QuickMatchKey[ii]] = ii;
    }
}

Session::Session(io_service& io_service, Room& room)
:
socket_(io_service),
room_(room),
connection_number_(connection_ticket_++),
is_quitting_(0),
prev_input_(3)
{
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

    std::cout << "session started, connection number: " << connection_number_ << "\n";
    socket_.set_option(ip::tcp::no_delay(true));
    room_.Join(shared_from_this());

    asyncReadHeader();
}

void Session::asyncReadHeader()
{
    async_read(
        socket_,
        buffer(request_header_, 4),
        boost::bind(
            &Session::handleReadHeader,
            this,
            placeholders::error));
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
        buffer(&request_body_[0], body_size),
        boost::bind(
            &Session::handleReadRequest,
            this,
            placeholders::error));
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

    std::string response = "ACK";

    if (false) { }
    else if (command == "current_buffer")
    {
        writeResponse(response);

        current_buffer_ = boost::lexical_cast<unsigned>(*argument);
        if (Contains(buffers_, current_buffer_) == false)
        {
            buffers_[current_buffer_].Init(this, current_buffer_);
        }
    }
    else if (command == "current_line")
    {
        writeResponse(response);

        current_line_ = *argument;
    }
    // assumes that above is called immediately before
    else if (command == "cursor_position")
    {
        writeResponse(response);

        std::vector<std::string> position;
        boost::split(
            position,
            *argument,
            boost::is_any_of(" "),
            boost::token_compress_on);
        unsigned x = boost::lexical_cast<unsigned>(position[0]);
        unsigned y = boost::lexical_cast<unsigned>(position[1]);
        cursor_pos_.first = x; cursor_pos_.second = y;

        buffers_[current_buffer_].CalculateCurrentWordOfCursor(
            current_line_,
            cursor_pos_);
    }
    else if (command == "buffer_contents")
    {
        writeResponse(response);

        ParseJob job;
        job.BufferNumber = current_buffer_;
        job.Contents = argument;
        queueParseJob(job);
    }
    else if (command == "complete")
    {
        //Stopwatch watch; watch.Start();

        response.clear();
        response.reserve(1024);
        calculateCompletionCandidates(*argument, response);
        //std::cout << response << std::endl;
        writeResponse(response);

        //watch.Stop(); std::cout << "complete: "; watch.PrintResultMilliseconds();
    }
    else if (command == "free_buffer")
    {
        writeResponse(response);

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
    }
    else if (command == "current_tags")
    {
        current_tags_.clear();

        std::vector<std::string> tags_list;
        boost::split(tags_list, *argument,
                     boost::is_any_of(","), boost::token_compress_on);
        foreach (const std::string& tags, tags_list)
        {
            if (tags.size() == 0) continue;

            TagsSet::Instance()->CreateOrUpdate(tags);
            current_tags_.push_back(tags);
        }

        writeResponse(response);
    }
    else if (command == "taglist_tags")
    {
        taglist_tags_.clear();

        std::vector<std::string> tags_list;
        boost::split(tags_list, *argument,
                     boost::is_any_of(","), boost::token_compress_on);
        foreach (const std::string& tags, tags_list)
        {
            if (tags.size() == 0) continue;

            TagsSet::Instance()->CreateOrUpdate(tags);
            taglist_tags_.push_back(tags);
        }

        writeResponse(response);
    }
    else if (command == "vim_taglist_function")
    {
        response = TagsSet::Instance()->VimTaglistFunction(
            *argument,
            taglist_tags_);
        writeResponse(response);
    }
    else if (command == "prune") {
        writeResponse(response);

        unsigned count = WordSet.Prune();
        //std::cout << count << " words pruned" << std::endl;
    }
    else
    {
        std::cout << boost::str(boost::format(
            "unknown command %s %s") % command % *argument);

        writeResponse(response);
    }
}

void Session::writeResponse(std::string& response)
{
    // write response
    response.resize(response.size() + 1, '\0');
    async_write(
        socket_,
        buffer(&response[0], response.size()),
        boost::bind(
            &Session::handleWriteResponse,
            this,
            placeholders::error));

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
    bool did_prune = false;
    ParseJob job;

    while (true)
    {
#ifdef _WIN32
        ::Sleep(1);
#else
        ::usleep(1 * 1000);
#endif
        if (is_quitting_ == 1) break;

        // pop off the next job
try_get_next_job:
        job_queue_mutex_.lock();
        if (job_queue_.size() > 0) {
            job = job_queue_.front();
            job_queue_.pop_front();
            job_queue_mutex_.unlock();

            // do the job
            buffers_mutex_.lock();
            if (Contains(buffers_, job.BufferNumber) == false) {
                std::cout << "buffer " << job.BufferNumber
                          << "no longer exists! "
                          << "skipping this job" << std::endl;
                continue;
            }
            buffers_[job.BufferNumber].ReplaceContents(job.Contents);
            buffers_mutex_.unlock();

            did_prune = false;

            // try and see if there is something queued up
            // if so, then immediately process it,
            // don't go to sleep
            goto try_get_next_job;
        } else {
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
        return;

    // keep a trailing list of previous inputs
    prev_input_[2] = prev_input_[1];
    prev_input_[1] = prev_input_[0];
    prev_input_[0] = prefix_to_complete;

    bool terminus_mode = shouldEnableTerminusMode(prefix_to_complete);
    std::string terminus_prefix;

    bool disambiguate_mode = shouldEnableDisambiguateMode(prefix_to_complete);
    char disambiguate_letter = 0;
    unsigned disambiguate_index = UINT_MAX;
    if (terminus_mode)
    {
        terminus_prefix = prefix_to_complete;
        terminus_prefix.resize( prefix_to_complete.size() - 1 );
    }
    else if (disambiguate_mode)
    {
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
    fillCompletionSet(prefix_to_complete, main_completion_set);

    // encountered an invalid disambiguation, abort and retry normally
    // as though it was not intended
    //
    // for example, shouldE would falsely trigger 'disambiguate_mode'
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
    if (main_completion_set.GetNumCompletions() == 0) {
        WordSet.GetLevenshteinCompletions(
            prefix_to_complete,
            levenshtein_completions);
    }

    std::vector<StringPair> result_list;
    std::vector<StringPair> terminus_result_list;

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
        foreach (const StringPair& pair, terminus_result_list)
        {
            const std::string& word = pair.first;
            if (word[word.size() - 1] == '_')
                main_completion_set.AddBannedWord(pair.first);
        }
    }

    main_completion_set.FillResults(result_list);

    // convert to format that VIM expects, basically a list of dictionaries
    result += "[";
    if (disambiguate_mode == false)
    {
        if (terminus_mode)
        {
            foreach (const StringPair& pair, terminus_result_list)
            {
                const std::string& word = pair.first;
                if (word[ word.size() - 1 ] != '_')
                    continue;
                if (word == prefix_to_complete)
                    continue;

                result += boost::str(boost::format(
                    "{'word':'%s','menu':'[%s]'},")
                    % pair.first % pair.second );
            }
        }

        foreach (const StringPair& pair, result_list)
        {
            result += boost::str(boost::format(
                "{'word':'%s','menu':'[%s]'},")
                % pair.first % pair.second );
        }

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
            const std::string& single_result =
                result_list[disambiguate_index].first;
            result += boost::str(boost::format(
                "{'abbr':'%s','word':'%s'},")
                % (single_result + " <--") % single_result );
        }
    }
    result += "]";
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
    if ( !LookupTable::IsUpper[ word[word.size() - 1] ] )
        return false;

    for (size_t ii = 0; ii < (word.size() - 1); ++ii)
    {
        if ( LookupTable::IsUpper[ word[ii] ] )
            return false;
    }

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
    CompletionSet& completion_set)
{
    this->WordSet.GetAbbrCompletions(
        prefix_to_complete,
        &completion_set.AbbrCompletions);
    completion_set.AbbrCompletions.erase(prefix_to_complete);

    TagsSet::Instance()->GetAbbrCompletions(
        prefix_to_complete, current_tags_,
        &completion_set.TagsAbbrCompletions);
    completion_set.TagsAbbrCompletions.erase(prefix_to_complete);

    this->WordSet.GetPrefixCompletions(
        prefix_to_complete,
        &completion_set.PrefixCompletions);
    completion_set.PrefixCompletions.erase(prefix_to_complete);

    TagsSet::Instance()->GetAllWordsWithPrefix(
        prefix_to_complete, current_tags_,
        &completion_set.TagsPrefixCompletions);
    completion_set.TagsPrefixCompletions.erase(prefix_to_complete);
}
