#include "stdafx.hpp"

#include "Session.hpp"

unsigned int Session::connection_ticket_ = 0;

Session::Session(io_service& io_service, Room& room)
:
socket_(io_service),
room_(room),
connection_number_(connection_ticket_++)
{
    ;
}

Session::~Session()
{
    std::cout << boost::str(boost::format(
        "session %u destroyed\n") % connection_number_);
}

void Session::Start()
{
    std::cout << "session started, connection number: " << connection_number_ << "\n";
    room_.Join(shared_from_this());
    
    asyncReadUntilNullChar();
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

void Session::handleReadRequest(const boost::system::error_code& error)
{
    if (error)
    {
        room_.Leave(shared_from_this());
        
        LogAsioError(error, "failed to handleReadRequest()");
        return;
    }

	// convert to a string
    std::ostringstream ss;
    ss << &request_;
    std::string request = ss.str();
    
    // find first space
    int index = -1;
    for (int ii = 0; ii < request.size(); ++ii)
    {
        if (request[ii] == ' ')
        {
            index = ii;
            break;
        }
    }
    
    if (index == -1) throw std::exception();
    
	// break it up into a "request" string and a "argument"
    std::string command(request.begin(), request.begin() + index);
    std::string argument(request.begin() + index + 1, request.end());
    
    std::string response = "ACK";
    
    if (false) { }
    //else if (command == "open_file")
    //{
		//std::cout << boost::str(boost::format("%s: %s\n") % command % argument);
    //}
    else if (command == "current_buffer")
    {
		//std::cout << boost::str(boost::format("%s: %s\n") % command % argument);
        
        current_buffer_ = argument;

        // create and initialize buffer object if it doesn't exist
        if (buffers_.find(current_buffer_) == buffers_.end())
        {
            buffers_[current_buffer_].Init(this, argument);
        }
    }
    else if (command == "current_pathname")
    {
		//std::cout << boost::str(boost::format("%s: %s\n") % command % argument);

        buffers_[current_buffer_].SetPathname(argument);
    }
    else if (command == "current_line")
    {
		//std::cout << boost::str(boost::format("%s: %s\n") % command % argument);

		current_line_ = argument;
    }
    else if (command == "buffer_contents_insert_mode")
    {
		auto& buffer = buffers_[current_buffer_];
        buffer.ParseInsertMode(argument, current_line_, cursor_pos_);

		//std::cout << boost::str(boost::format(
			//"%s: length = %u\n") % command % argument.length());
    }
    else if (command == "buffer_contents")
    {
		auto& buffer = buffers_[current_buffer_];
        buffer.ParseNormalMode(argument);

		//std::cout << boost::str(boost::format(
			//"%s: length = %u\n") % command % argument.length());
    }
	else if (command == "cursor_position")
	{
		std::vector<std::string> position;
		std::string trimmed_argument = std::string(argument.begin(), argument.end() - 1);
		boost::split(
			position,
			trimmed_argument,
			boost::is_any_of(" "),
			boost::token_compress_on);
		int x = boost::lexical_cast<int>(position[0]);
		int y = boost::lexical_cast<int>(position[1]);
		cursor_pos_.first = x; cursor_pos_.second = y;

		//std::cout << boost::str(boost::format("%s: %d %d\n") % command % x % y);
	}
	else if (command == "complete")
	{
		response = calculateCompletionCandidates(std::string(argument.begin(), argument.end() - 1));
	}
    else
    {
        std::cout << boost::str(boost::format(
            "unknown command %s %s") % command % argument);
    }
	
    // write response    
    response.resize(response.size() + 1, '\0');
    async_write(
        socket_,
        buffer(&response[0], response.size()),
        boost::bind(
            &Session::handleWriteResponse,
            this,
            placeholders::error));
            
    asyncReadUntilNullChar();
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

std::string Session::calculateCompletionCandidates(const std::string& line)
{
	std::string word_to_complete = getWordToComplete(line);
	if (word_to_complete.empty()) return "";

	//std::cout << "--- BEGIN COMPLETIONS ---\n";
	
	std::vector<std::string> completions;
	// consider completions from the current buffer first
	// because of spatial locality
	buffers_[current_buffer_].GetAllWordsWithPrefixFromCurrentLine(word_to_complete, &completions);
	buffers_[current_buffer_].GetAllWordsWithPrefix(word_to_complete, &completions);
	
	// then look at other buffers
	for (auto& buffer : buffers_)
	{
		if (completions.size() >= 32) break;
		if (buffer.first == current_buffer_) continue;
		
		buffer.second.GetAllWordsWithPrefix(word_to_complete, &completions);
	}

	std::stringstream results;
	results << "[";
	for (size_t ii = 0; ii < completions.size(); ++ii)
	{
		const std::string& word = completions[ii];
		if (word == word_to_complete) continue;
		if (boost::starts_with(word, word_to_complete) == false) continue;

		//std::cout << word << "\n";
		
		results << "\"" << word << "\"";
		if (ii != (completions.size() - 1))
		{
			results << ", ";
		}
	}
	results << "]";
	
	//std::cout << "--- END COMPLETIONS ---\n";
	
	return results.str();
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
