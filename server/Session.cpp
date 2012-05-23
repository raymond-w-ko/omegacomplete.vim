#include "stdafx.hpp"

#include "Session.hpp"
#include "Stopwatch.hpp"
#include "TagsSet.hpp"

unsigned int Session::connection_ticket_ = 0;

Session::Session(io_service& io_service, Room& room)
:
socket_(io_service),
room_(room),
connection_number_(connection_ticket_++),
quick_match_key_(1024, ' '),
is_quitting_(0)
{
    quick_match_key_[0] = 'a';
    quick_match_key_[1] = 's';
    quick_match_key_[2] = 'd';
    quick_match_key_[3] = 'f';
    quick_match_key_[4] = 'g';
    quick_match_key_[5] = 'h';
    quick_match_key_[6] = 'j';
    quick_match_key_[7] = 'k';
    quick_match_key_[8] = 'l';
    quick_match_key_[9] = ';';

    quick_match_key_[10] = 'q';
    quick_match_key_[11] = 'w';
    quick_match_key_[12] = 'e';
    quick_match_key_[13] = 'r';
    quick_match_key_[14] = 't';
    quick_match_key_[15] = 'y';
    quick_match_key_[16] = 'u';
    quick_match_key_[17] = 'i';
    quick_match_key_[18] = 'o';
    quick_match_key_[19] = 'p';

    worker_thread_ = boost::thread(
        &Session::workerThreadLoop,
        this);
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
    std::cout << "session started, connection number: " << connection_number_ << "\n";
    room_.Join(shared_from_this());

    //asyncReadUntilNullChar();
    asyncReadHeader();
}

void Session::asyncReadUntilNullChar()
{
    std::string null_delimiter(1, '\0');

    async_read_until(
        socket_,
        request_,
        null_delimiter,
        boost::bind(
            &Session::handleReadRequest,
            this,
            placeholders::error));
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
    // convert to a string
    //std::ostringstream ss;
    //ss << &request_;
    //const std::string& request = ss.str();
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
    //std::string argument(request.begin() + index + 1, request.end() - 1);

    StringPtr argument = boost::make_shared<std::string>(
        request.begin() + index + 1, request.end());

    std::string response = "ACK";

    if (false) { }
    else if (command == "current_buffer")
    {
        writeResponse(response);

        current_buffer_ = boost::lexical_cast<unsigned>(*argument);

        // create and initialize buffer object if it doesn't exist
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
    else if (command == "buffer_contents_insert_mode")
    {
        writeResponse(response);

        //Stopwatch watch; watch.Start();

        auto(&buffer, buffers_[current_buffer_]);
        buffer.ParseInsertMode(*argument, current_line_, cursor_pos_);

        //watch.Stop();
        //std::cout << "insert mode parse: ";watch.PrintResultMilliseconds();
    }
    else if (command == "buffer_contents")
    {
        writeResponse(response);

        //Stopwatch watch; watch.Start();

        auto(&buffer, buffers_[current_buffer_]);
        buffer.ParseNormalMode(*argument);

        //watch.Stop();
        //std::cout << "normal mode parse: "; watch.PrintResultMilliseconds();
    }
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
    }
    else if (command == "complete")
    {
        //Stopwatch watch; watch.Start();

        calculateCompletionCandidates(*argument, response);
        writeResponse(response);

        //watch.Stop();
        //std::cout << "complete: "; watch.PrintResultMilliseconds();
    }
    else if (command == "free_buffer")
    {
        writeResponse(response);

        buffers_.erase(boost::lexical_cast<unsigned>(argument));
    }
    else if (command == "current_tags")
    {
        writeResponse(response);

        current_tags_.clear();

        std::vector<std::string> tags_list;
        boost::split(tags_list, *argument, boost::is_any_of(","), boost::token_compress_on);
        foreach (const std::string& tags, tags_list)
        {
            if (tags.size() == 0) continue;

            TagsSet::Instance()->CreateOrUpdate(tags);
            current_tags_.push_back(tags);
        }
    }
    else if (command == "taglist_tags")
    {
        writeResponse(response);

        taglist_tags_.clear();

        std::vector<std::string> tags_list;
        boost::split(tags_list, *argument, boost::is_any_of(","), boost::token_compress_on);
        foreach (const std::string& tags, tags_list)
        {
            if (tags.size() == 0) continue;

            TagsSet::Instance()->CreateOrUpdate(tags);
            taglist_tags_.push_back(tags);
        }
    }
    else if (command == "vim_taglist_function")
    {
        response = TagsSet::Instance()->VimTaglistFunction(*argument, taglist_tags_);
        writeResponse(response);
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

    //asyncReadUntilNullChar();
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

void Session::workerThreadLoop()
{
    while (true)
    {
        const unsigned int sleep_time_ms = 1;
#ifdef WIN32
        Sleep(sleep_time_ms);
#else
        sleep(sleep_time_ms);
#endif

        if (is_quitting_ == 1) break;

        // pop off the next job
        ParseJob job;
        job_queue_mutex_.lock();
        if (job_queue_.size() > 0) {
            job = job_queue_.front();
            job_queue_.pop_front();
        }
        job_queue_mutex_.unlock();

        // the job queue is empty
        if (job.Contents == NULL) continue;
    }
}

void Session::calculateCompletionCandidates(
    const std::string& line,
    std::string& result)
{
    std::string word_to_complete = getWordToComplete(line);
    if (word_to_complete.empty()) {
        result.clear();
        return;
    }

    std::set<std::string> abbr_completions;
    calculateAbbrCompletions(word_to_complete, &abbr_completions);
    abbr_completions.erase(word_to_complete);

    std::set<std::string> tags_abbr_completions;
    TagsSet::Instance()->GetAbbrCompletions(
        word_to_complete, current_tags_,
        &tags_abbr_completions);
    tags_abbr_completions.erase(word_to_complete);

    std::set<std::string> prefix_completions;
    calculatePrefixCompletions(word_to_complete, &prefix_completions);
    prefix_completions.erase(word_to_complete);

    std::set<std::string> tags_prefix_completions;
    TagsSet::Instance()->GetAllWordsWithPrefix(
        word_to_complete, current_tags_,
        &tags_prefix_completions);
    tags_prefix_completions.erase(word_to_complete);


    //LevenshteinSearchResults levenshtein_completions;
    // I type so fast and value the popup not appearing if it doesn't
    // recognize the word, so I have disabled this for now
    // only if we have no completions do we try to Levenshtein distance completion
    //if ((abbr_completions.size() + prefix_completions.size()) == 0)
    //{
        //calculateLevenshteinCompletions(
            //word_to_complete,
            //levenshtein_completions);
    //}

    // compile results and send
    unsigned int num_completions_added = 0;
    boost::unordered_set<std::string> added_words;

    std::stringstream results;
    results << "[";
    // append abbreviations first
    foreach (const std::string& word, abbr_completions)
    {
        results << boost::str(boost::format(
            "{'word':'%s','menu':'[%c]'},")
            % word % quick_match_key_[num_completions_added++]);

        added_words.insert(word);
    }
    // append tags abbreviations, make sure it's not part of above
    foreach (const std::string& word, tags_abbr_completions)
    {
        if (Contains(added_words, word) == true) continue;

        results << boost::str(boost::format(
            "{'word':'%s','menu':'[%c]'},")
            % word % quick_match_key_[num_completions_added++]);

        added_words.insert(word);
    }

    // append prefix completions
    foreach (const std::string& word, prefix_completions)
    {
        if (Contains(added_words, word) == true) continue;

        results << boost::str(boost::format(
            "{'word':'%s','menu':'[%c]'},")
            % word % quick_match_key_[num_completions_added++]);

        added_words.insert(word);

        if (num_completions_added >= 32) break;
    }
    foreach (const std::string& word, tags_prefix_completions)
    {
        if (Contains(added_words, word) == true) continue;

        results << boost::str(boost::format(
            "{'word':'%s','menu':'[%c]'},")
            % word % quick_match_key_[num_completions_added++]);

        added_words.insert(word);

        if (num_completions_added >= 32) break;
    }

    // append levenshtein completions
    //bool done = false;
    //for (auto& completion : levenshtein_completions)
    //{
        //auto& cost = completion.first;
        //auto& word_set = completion.second;
        //for (const std::string& word : word_set)
        //{
            //if (word == word_to_complete) continue;

            //results << boost::str(boost::format(
                //"{'word':'%s','menu':'[%d]'},")
                //% word % cost);
        //}

        //if (done) break;
    //}

    results << "]";

    result = results.str();
}

std::string Session::getWordToComplete(const std::string& line)
{
    if (line.length() == 0) return "";

    int partial_end = line.length();
    int partial_begin = partial_end - 1;
    for (; partial_begin >= 0; --partial_begin)
    {
        char c = line[partial_begin];
        if (IsPartOfWord(c) == true)
        {
            continue;
        }

        break;
    }

    if ((partial_begin + 1) == partial_end) return "";

    std::string partial( &line[partial_begin + 1], &line[partial_end] );
    //std::cout << "complete word: " << partial << std::endl;
    return partial;
}

void Session::calculatePrefixCompletions(
    const std::string& word_to_complete,
    std::set<std::string>* completions)
{
    buffers_[current_buffer_].GetAllWordsWithPrefixFromCurrentLine(
        word_to_complete,
        completions);

    foreach (BuffersIterator& buffer, buffers_)
    {
        buffer.second.GetAllWordsWithPrefix(word_to_complete, completions);
    }
}

void Session::calculateLevenshteinCompletions(
    const std::string& word_to_complete,
    LevenshteinSearchResults& completions)
{
    foreach (BuffersIterator& buffer, buffers_)
    {
        buffer.second.GetLevenshteinCompletions(word_to_complete, completions);
    }
}

void Session::calculateAbbrCompletions(
    const std::string& word_to_complete,
    std::set<std::string>* completions)
{
    foreach (BuffersIterator& buffer, buffers_)
    {
        buffer.second.GetAbbrCompletions(word_to_complete, completions);
    }
}
