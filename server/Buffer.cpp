#include "stdafx.hpp"

#include "Buffer.hpp"

const unsigned int kTrieDepth = 3;
const unsigned int kNumThreads = 8;

Buffer::Buffer()
{
    ;
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
    
    typedef std::vector<std::string> StringVector;
    typedef boost::packaged_task<StringVector> StringVectorTask;
    typedef boost::unique_future<StringVector> StringVectorFuture;

    std::vector<StringVectorFuture> results;
    for (size_t ii = 0; ii < kNumThreads; ++ii)
    {
        const size_t begin = (ii * results.size()) / kNumThreads;
        const size_t end = ((ii + 1) * results.size()) / kNumThreads;
        
        StringVectorTask pt([&] {
            auto iter_begin = words.begin() + begin;
            auto iter_end = words.begin() + end;
            std::vector<std::string> matches;

            while (iter_begin != iter_end)
            {
                matches.push_back(*iter_begin);
                iter_begin++;
            }
            
            return matches;
        });

        StringVectorFuture fi = pt.get_future();
        //results.push_back(fi);
        boost::thread task(boost::move(pt));
    }
    
    for (auto& future : results)
    {
        future.wait();
        auto chunk = future.get();
        std::copy(chunk.begin(), chunk.end(), std::back_inserter(words));
    }

    std::cout << "\n";
}

bool Buffer::Init(std::string buffer_id)
{
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
    boost::regex re("\\W+", boost::regex::normal | boost::regex::icase);
    boost::sregex_token_iterator token(contents_.begin(), contents_.end(), re, -1);
    boost::sregex_token_iterator token_end;
    unsigned int counter = 0;
    while (token != token_end)
    {
        const std::string& word = *token;
        if (already_processed_words_.find(word) == already_processed_words_.end())
        {
            words_.AddKeyword(&word[0], word, kTrieDepth);
            already_processed_words_.insert(word);
        }
        
        token++;
        counter++;
    }

    std::cout << boost::str(boost::format(
        "stored %u words\n") % already_processed_words_.size());
}
