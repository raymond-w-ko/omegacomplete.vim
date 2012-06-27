#include "stdafx.hpp"

#include "Session.hpp"
#include "TagsSet.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"

static const unsigned kMaxNumCompletions = 32;

unsigned int Session::connection_ticket_ = 0;
std::vector<char> Session::quick_match_key_;
boost::unordered_map<char, unsigned> Session::reverse_quick_match_;

void Session::GlobalInit()
{
    quick_match_key_.resize(kMaxNumCompletions, ' '),

    quick_match_key_[0] = 'A';
    quick_match_key_[1] = 'S';
    quick_match_key_[2] = 'D';
    quick_match_key_[3] = 'F';
    quick_match_key_[4] = 'G';
    quick_match_key_[5] = 'H';
    quick_match_key_[6] = 'J';
    quick_match_key_[7] = 'K';
    quick_match_key_[8] = 'L';
    quick_match_key_[9] = ';';

    quick_match_key_[10] = 'Q';
    quick_match_key_[11] = 'W';
    quick_match_key_[12] = 'E';
    quick_match_key_[13] = 'R';
    quick_match_key_[14] = 'T';
    quick_match_key_[15] = 'Y';
    quick_match_key_[16] = 'U';
    quick_match_key_[17] = 'I';
    quick_match_key_[18] = 'O';
    quick_match_key_[19] = 'P';

    for (unsigned ii = 0; ii < 20; ++ii)
    {
        reverse_quick_match_[quick_match_key_[ii]] = ii;
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

    bool disambiguate_mode = shouldEnableDisambiguateMode(prefix_to_complete);
    char disambiguate_letter = 0;
    int disambiguate_index = -1;
    if (disambiguate_mode)
    {
        disambiguate_letter = prefix_to_complete[ prefix_to_complete.size() - 1 ];

        prefix_to_complete.resize( prefix_to_complete.size() - 1 );

        if (Contains(reverse_quick_match_, disambiguate_letter) == true)
            disambiguate_index = reverse_quick_match_[disambiguate_letter];
    }

    std::set<std::string> abbr_completions;
    std::set<std::string> tags_abbr_completions;
    std::set<std::string> prefix_completions;
    std::set<std::string> tags_prefix_completions;

retry_completion:

    WordSet.GetAbbrCompletions(prefix_to_complete, &abbr_completions);
    abbr_completions.erase(prefix_to_complete);

    TagsSet::Instance()->GetAbbrCompletions(
        prefix_to_complete, current_tags_,
        &tags_abbr_completions);
    tags_abbr_completions.erase(prefix_to_complete);

    WordSet.GetPrefixCompletions(prefix_to_complete, &prefix_completions);
    prefix_completions.erase(prefix_to_complete);

    TagsSet::Instance()->GetAllWordsWithPrefix(
        prefix_to_complete, current_tags_,
        &tags_prefix_completions);
    tags_prefix_completions.erase(prefix_to_complete);

    size_t num_current_completions =
        abbr_completions.size() + tags_abbr_completions.size() +
        prefix_completions.size() + tags_prefix_completions.size();

    // encountered an invalid disambiguation, abort and retry normally
    // as though it was not intended
    //
    // for example, shouldE would falsely trigger 'disambiguate_mode'
    // but we probably want to try it as a regular prefix completion
    if (disambiguate_mode &&
        disambiguate_index >= 0 &&
        disambiguate_index >= num_current_completions)
    {
        disambiguate_mode = false;
        disambiguate_index = -1;

        abbr_completions.clear();
        tags_abbr_completions.clear();
        prefix_completions.clear();
        tags_prefix_completions.clear();

        prefix_to_complete += boost::lexical_cast<std::string>(disambiguate_letter);

        goto retry_completion;
    }

    // only if we have no completions do we try to Levenshtein distance completion
    LevenshteinSearchResults levenshtein_completions;
    if (num_current_completions == 0) {
        WordSet.GetLevenshteinCompletions(
            prefix_to_complete,
            levenshtein_completions);
    }

    unsigned num_completions_added = 0;
    std::vector<StringPair> result_list;
    boost::unordered_set<std::string> added_words;

    // this is to prevent completions from being considered
    // that are basically the word from before you press Backspace.
    if (prev_input_[1].size() == (prefix_to_complete.size() + 1) &&
        boost::starts_with(prev_input_[1], prefix_to_complete))
    {
        added_words.insert(prev_input_[1]);
    }

    addWordsToResults(
        abbr_completions,
        result_list, num_completions_added, added_words);

    addWordsToResults(
        tags_abbr_completions,
        result_list, num_completions_added, added_words);

    addWordsToResults(
        prefix_completions,
        result_list, num_completions_added, added_words);

    addWordsToResults(
        tags_prefix_completions,
        result_list, num_completions_added, added_words);

    // convert to format that VIM expects, basically a list of dictionaries
    result += "[";
    if (disambiguate_mode == false)
    {
        foreach (const StringPair& pair, result_list)
        {
            result += boost::str(boost::format(
                "{'word':'%s','menu':'[%s]'},")
                % pair.first % pair.second );
        }

        // append Levenshtein completions
        auto (iter, levenshtein_completions.begin());
        for (; iter != levenshtein_completions.end(); ++iter) {
            if (num_completions_added >= kMaxNumCompletions) break;

            int score = iter->first;
            foreach (const std::string& word, iter->second) {
                if (num_completions_added >= kMaxNumCompletions) break;
                if (Contains(added_words, word) == true) continue;
                if (word == prefix_to_complete) continue;
                if (boost::starts_with(prefix_to_complete, word)) continue;

                result += boost::str(boost::format(
                    "{'abbr':'*** %s','word':'%s'},")
                    % word % word);

                num_completions_added++;

                added_words.insert(word);
            }
        }
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

void Session::addWordsToResults(
    const std::set<std::string>& words,
    std::vector<StringPair>& result_list,
    unsigned& num_completions_added,
    boost::unordered_set<std::string>& added_words)
{
    foreach (const std::string& word, words)
    {
        if (num_completions_added >= kMaxNumCompletions) break;
        if (Contains(added_words, word) == true) continue;

        result_list.push_back(std::make_pair(
            word,
            boost::lexical_cast<std::string>(
                quick_match_key_[num_completions_added++]) ));

        added_words.insert(word);
    }
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
