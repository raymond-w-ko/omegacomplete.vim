#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"
#include "Stopwatch.hpp"

const unsigned int kTrieDepth = 4;
const unsigned int kNumThreads = 8;

Buffer::Buffer()
:
cursor_pos_(0, 0)
{
    word_split_regex_ = boost::xpressive::sregex::compile("\\W+");
}

Buffer::~Buffer()
{
    ;
}

bool Buffer::operator==(const Buffer& other)
{
    return (this->buffer_id_ == other.buffer_id_);
}

static void dummy()
{
	//std::vector<std::string> words;
	//words_.GetAllWordsWithPrefix("mo", &words);
	//for (const std::string& word : words)
	//{
		//std::cout << word << " ";
	//}
    
	//words.clear();
    
	//typedef std::future<std::vector<std::string>> StringVectorFuture;
	//std::vector<StringVectorFuture> futures;
	
	//std::future<int> the_answer = std::async([] { return 42; });
	//int foobar = the_answer.get();

    //for (size_t ii = 0; ii < kNumThreads; ++ii)
    //{
        //const size_t begin = (ii * already_processed_words_.size()) / kNumThreads;
        //const size_t end = ((ii + 1) * already_processed_words_.size()) / kNumThreads;
        
        //std::vector<std::string>::iterator iter_begin = words.begin() + begin;
        //std::vector<std::string>::iterator iter_end = words.begin() + end;

        //StringVectorFuture svf = parent_->SubmitJob( boost::bind(
            //&Buffer::fuzzyMatch,
            //nullptr,
            //nullptr));

        ////futures.push_back(svf);
    //}
    
    //for (auto& future : futures)
    //{
        //auto chunk = future.get();
        //std::copy(chunk.begin(), chunk.end(), std::back_inserter(words));
    //}

    //std::cout << "\n";
}

void Buffer::ParseInsertMode(
	const std::string& new_contents,
	const std::string& cur_line,
	std::pair<int, int> cursor_pos)
{
	bool need_total_reparse = false;
	//std::cout << "cur_line: " << cur_line << "#" <<std::endl;
	//std::cout << "prev_cur_line_: " << prev_cur_line_ << "#" << std::endl;
	if (boost::starts_with(cur_line, prev_cur_line_))
	{
		// since the old current line is a prefix of the new current
		// line (the common case), we can only have new words to add
		// just parse the current line and re-add all the words,
		// since we are usinga  set dupicates can't happen
		need_total_reparse = false;

		current_line_words_.Clear();
		tokenizeKeywordsOfLine(cur_line);
		//std::cout << "no need to do total reparse, just re-adding line\n";
	}
	else
	{
		// if the old current_line is no longer the prefix of
		// current line, then something is deleted, or has changed
		// we have to trigger a reparse since we don't maintain original
		// unchanged words on the line
		need_total_reparse = true;
	}
	// NUL byte at the end
	prev_cur_line_ = std::string(cur_line.begin(), cur_line.end() - 1);

	// if we have somehow changed rows in insert mode
	// (are you really using arrow keys?) then we should reparse just to be sure
    //std::cout << boost::str(boost::format("%d %d ? %d %d\n")
		//% cursor_pos_.first % cursor_pos_.second
		//% cursor_pos.first % cursor_pos.second);
	if (need_total_reparse ||
	    (cursor_pos_.first != cursor_pos.first) &&
		(cursor_pos_.first != 0 && cursor_pos_.second != 0)
		)
	{
		if (contents_ != new_contents)
		{
			//std::cout << "doing total reparse\n";
			contents_ = new_contents;

			already_processed_words_.clear();
			words_.Clear();

			tokenizeKeywords();
		}

	}
	cursor_pos_ = cursor_pos;
}

void Buffer::ParseNormalMode(
	const std::string& new_contents)
{
	if (contents_ == new_contents) return;
	
	contents_ = new_contents;

    already_processed_words_.clear();
    words_.Clear();

	//Stopwatch watch;
	//watch.Start();
    tokenizeKeywords();
	//watch.Stop();
	//watch.PrintResultMilliseconds();
}

bool Buffer::Init(Session* parent, std::string buffer_id)
{
    parent_ = parent;

    if (buffer_id.size() == 0) throw std::exception();
    buffer_id_ = buffer_id;
}

std::string Buffer::GetBufferId() const
{
    return buffer_id_;
}

void Buffer::SetPathname(std::string pathname)
{
    pathname_ = pathname;
}

void Buffer::tokenizeKeywords()
{
	size_t contents_size = contents_.size();

	for (size_t ii = 0; ii < contents_size; ++ii)
	{
		// initial case, find character in the set of ([a-z][A-Z][0-9]_)
		// this will be what is considered a word
		// I guess we have unicode stuff we are screwed :(
		char c = contents_[ii];
		if (IsPartOfWord(c) == false) continue;
		
		// we have found the beginning of the word, loop until
		// we reach the end or we find a non world character
		size_t jj = ii + 1;
		for (; jj < contents_size; ++jj)
		{
			if (IsPartOfWord(contents_[jj]) == true)
				continue;
			break;
		}
		if (jj == contents_size) break;
		
		// construct word based off of pointer
		std::string word(&contents_[ii], &contents_[jj]);

        if (already_processed_words_.find(word) == already_processed_words_.end())
        {
			words_.AddKeyword(&word[0], word, kTrieDepth);
            already_processed_words_.insert(word);
        }
		
		// for loop will autoincrement
		ii = jj;
	}
	
    //std::cout << boost::str(boost::format(
        //"stored %u words\n") % already_processed_words_.size());
}

void Buffer::tokenizeKeywordsOfLine(const std::string& line)
{
	size_t contents_size = line.size();

	for (size_t ii = 0; ii < contents_size; ++ii)
	{
		// initial case, find character in the set of ([a-z][A-Z][0-9]_)
		// this will be what is considered a word
		// I guess we have unicode stuff we are screwed :(
		char c = line[ii];
		if (IsPartOfWord(c) == false) continue;
		
		// we have found the beginning of the word, loop until
		// we reach the end or we find a non world character
		size_t jj = ii + 1;
		for (; jj < contents_size; ++jj)
		{
			if (IsPartOfWord(line[jj]) == true)
				continue;
			break;
		}
		if (jj == contents_size) break;
		
		// construct word based off of pointer
		std::string word(&line[ii], &line[jj]);

		current_line_words_.AddKeyword(&word[0], word, kTrieDepth);
		
		// for loop will autoincrement
		ii = jj;
	}
	
    //std::cout << boost::str(boost::format(
        //"stored %u words\n") % already_processed_words_.size());
}

void Buffer::tokenizeKeywordsUsingRegex()
{
	// too slow, takes around 750 ms on a 500KB file (Hub/Window.cpp)
    using namespace boost::xpressive;
    sregex_token_iterator token_cur(contents_.begin(), contents_.end(), word_split_regex_, -1);
    sregex_token_iterator token_end;
    unsigned int counter = 0;
    for (; token_cur != token_end; ++token_cur)
    {
        const std::string& word = *token_cur;
		words_.AddKeyword(word.c_str(), word, kTrieDepth);
        
        counter++;
    }
}

void Buffer::GetAllWordsWithPrefix(
	const std::string& prefix,
	std::vector<std::string>* results)
{
	words_.GetAllWordsWithPrefix(prefix.c_str(), results, kTrieDepth);
}

void Buffer::GetAllWordsWithPrefixFromCurrentLine(
	const std::string& prefix,
	std::vector<std::string>* results)
{
	current_line_words_.GetAllWordsWithPrefix(prefix.c_str(), results, kTrieDepth);
}
