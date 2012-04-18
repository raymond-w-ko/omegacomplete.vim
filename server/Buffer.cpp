#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"

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

void Buffer::Parse()
{
    already_processed_words_.clear();
    words_.Clear();

    tokenizeKeywords();
    
    std::vector<std::string> words;
    words_.GetAllWordsWithPrefix("mo", &words);
    for (const std::string& word : words)
    {
        std::cout << word << " ";
    }
    
    words.clear();
    
<<<<<<< Updated upstream
    typedef std::future<std::vector<std::string>> StringVectorFuture;
    std::vector<StringVectorFuture> futures;
    for (size_t ii = 0; ii < kNumThreads; ++ii)
=======
    //typedef std::future<std::vector<std::string>> StringVectorFuture;
    //std::vector<StringVectorFuture> futures;
    //for (size_t ii = 0; ii < kNumThreads; ++ii)
>>>>>>> Stashed changes
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
    
<<<<<<< Updated upstream
    for (auto& future : futures)
    {
        auto chunk = future.get();
        std::copy(chunk.begin(), chunk.end(), std::back_inserter(words));
    }
=======
    //for (auto& future : futures)
    //{
        //auto chunk = future.get();
        //std::copy(chunk.begin(), chunk.end(), std::back_inserter(words));
    //}
>>>>>>> Stashed changes

    std::cout << "\n";
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

void Buffer::SetContents(std::string contents)
{
    contents_ = contents;
}

void Buffer::tokenizeKeywords()
{
<<<<<<< Updated upstream
    boost::regex re("\\W+", boost::regex::normal | boost::regex::icase);
    boost::sregex_token_iterator token(contents_.begin(), contents_.end(), re, -1);
    boost::sregex_token_iterator token_end;
    unsigned int counter = 0;
    while (token != token_end)
    {
        const std::string& word = *token;
=======
    using namespace boost::xpressive;
    sregex_token_iterator token_cur(contents_.begin(), contents_.end(), word_split_regex_, -1);
    sregex_token_iterator token_end;
    unsigned int counter = 0;
    for (; token_cur != token_end; ++token_cur)
    {
        const std::string& word = *token_cur;
        // add word only if we haven't processed it yet
>>>>>>> Stashed changes
        if (already_processed_words_.find(word) == already_processed_words_.end())
        {
            words_.AddKeyword(&word[0], word, kTrieDepth);
            already_processed_words_.insert(word);
        }
        
<<<<<<< Updated upstream
        token++;
=======
>>>>>>> Stashed changes
        counter++;
    }

    std::cout << boost::str(boost::format(
        "stored %u words\n") % already_processed_words_.size());
}
