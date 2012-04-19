#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"
#include "Stopwatch.hpp"

const unsigned int kTrieDepth = 3;
const unsigned int kNumThreads = 8;

Buffer::Buffer()
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

void Buffer::Parse(const std::string& new_contents, bool force)
{
	Stopwatch watch;

	if (force)
	{
		contents_ = new_contents;
	}
	else
	{
		if (contents_ == new_contents)
		{
			//std::cout << "no need to Parse() contents" << std::endl;
			return;
		}
		
		contents_ = new_contents;
	}

    already_processed_words_.clear();
    words_.Clear();

	//watch.Start();
    tokenizeKeywords();
	//watch.Stop();
	//watch.PrintResultMilliseconds();
    
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


std::vector<std::string> Buffer::fuzzyMatch(
    std::string* iter_begin,
    std::string* iter_end)
{
    std::vector<std::string> matches;

    while (iter_begin != iter_end)
    {
        matches.push_back(*iter_begin);
        iter_begin++;
    }
            
    return matches;
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
        // add word only if we haven't processed it yet
        if (already_processed_words_.find(word) == already_processed_words_.end())
        {
            words_.AddKeyword(&word[0], word, kTrieDepth);
            already_processed_words_.insert(word);
        }
        
        counter++;
    }
}
