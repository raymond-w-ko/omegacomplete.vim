#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"
#include "Stopwatch.hpp"

const unsigned int kTrieDepth = 2;
const unsigned int kNumThreads = 8;

Buffer::Buffer()
:
cursor_pos_(0, 0)
{
    word_split_regex_ = boost::xpressive::sregex::compile("\\W+");
	
	for (size_t index = 0; index <= 255; ++index)
	{
		is_part_of_word_[index] = IsPartOfWord(index) ? 1 : 0;
	}
}

Buffer::~Buffer()
{
    ;
}

bool Buffer::operator==(const Buffer& other)
{
    return (this->buffer_id_ == other.buffer_id_);
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
		// since we are using a  set dupicates can't happen
		need_total_reparse = false;

		current_line_words_.clear();
		tokenizeKeywordsOfLine(cur_line);
		//std::cout << "no need to do total reparse, just re-adding line\n";
	}
	else
	{
		// if the old current_line is no longer the prefix of
		// current line, then something is deleted, or has changed
		// we have to trigger a reparse since we don't maintain original
		// unchanged words on the line
		//std::cout << "need to do total reparse\n"; need_total_reparse = true;
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
			std::cout << "doing total reparse\n";
			contents_ = new_contents;

			words_.clear();
			current_line_words_.clear();

			tokenizeKeywords();
		}

	}
	cursor_pos_ = cursor_pos;
}

void Buffer::ParseNormalMode(
	const std::string& new_contents)
{
	if (contents_ == new_contents) return;
	
	Stopwatch watch;
	
	// around 0.02 ms, not a problem
	//watch.Start();
	contents_ = new_contents;
	//watch.Stop(); watch.PrintResultMilliseconds();

	// around 60 ms, could be a problem
	// using std::unique_ptr<T> this drops to around 45 ms
	//watch.Start();
    //already_processed_words_.clear();
    words_.clear();
	current_line_words_.clear();
	//watch.Stop(); watch.PrintResultMilliseconds();

	// around 375 ms, this is the bottleneck
	//watch.Start();
    tokenizeKeywords();
	//watch.Stop(); watch.PrintResultMilliseconds();
	//std::cout << "\n";
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
		// I guess we have unicode stuff we are screwed :
		char c = contents_[ii];
		if (!is_part_of_word_[c]) continue;
		
		// we have found the beginning of the word, loop until
		// we reach the end or we find a non world character
		size_t jj = ii + 1;
		for (; jj < contents_size; ++jj)
		{
			if (is_part_of_word_[contents_[jj]])
				continue;
			break;
		}
		if (jj == contents_size) break;
		
		// construct word based off of pointer
		std::string word(&contents_[ii], &contents_[jj]);
		words_.insert(word);
		
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
		if (!is_part_of_word_[c]) continue;
		
		// we have found the beginning of the word, loop until
		// we reach the end or we find a non world character
		size_t jj = ii + 1;
		for (; jj < contents_size; ++jj)
		{
			if (is_part_of_word_[line[jj]])
				continue;
			break;
		}
		if (jj == contents_size) break;
		
		// construct word based off of pointer
		std::string word(&line[ii], &line[jj]);
		current_line_words_.insert(word);
		
		// for loop will autoincrement
		ii = jj;
	}
	
    //std::cout << boost::str(boost::format(
        //"stored %u words\n") % already_processed_words_.size());
}

void Buffer::GetAllWordsWithPrefix(
	const std::string& prefix,
	std::vector<std::string>* results)
{
	auto iter = words_.lower_bound(prefix);
	while (iter != words_.end() && boost::starts_with(*iter, prefix))
	{
		results->emplace_back(*iter);
		++iter;
	}
}

void Buffer::GetAllWordsWithPrefixFromCurrentLine(
	const std::string& prefix,
	std::vector<std::string>* results)
{
	auto iter = current_line_words_.lower_bound(prefix);
	while (iter != current_line_words_.end() && boost::starts_with(*iter, prefix))
	{
		results->emplace_back(*iter);
		++iter;
	}
}
