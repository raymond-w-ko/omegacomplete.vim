#include "stdafx.hpp"

#include "Buffer.hpp"

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

void Buffer::tokenizeKeywordsUsingRegex()
{
    // too slow, takes around 750 ms on a 500KB file (Hub/Window.cpp)
    using namespace boost::xpressive;
    sregex_token_iterator token_cur(
        contents_.begin(),
        contents_.end(),
        word_split_regex_, -1);
    sregex_token_iterator token_end;
    unsigned int counter = 0;
    for (; token_cur != token_end; ++token_cur)
    {
        const std::string& word = *token_cur;
        words_.insert(word);

        counter++;
    }
}

void Buffer::dummy2()
{
    size_t contents_size = contents_.size();

    const unsigned int kNumTokenizeThreads = 1;
    std::vector<std::pair<size_t, size_t>> bounds;
    // create initial bounds
    for (size_t ii = 0; ii < kNumTokenizeThreads; ++ii)
    {
        // in the form of [begin, end)
        bounds.push_back(std::make_pair(
            // lower bound
            (ii * contents_size) / kNumTokenizeThreads,
            // upper bound
            ((ii + 1) * contents_size) / kNumTokenizeThreads ));
    }

    // "push" bounds foward until we get to a non word char
    for (size_t ii = 0; ii < (kNumTokenizeThreads - 1); ++ii)
    {
        // sanity check
        //if (bounds[ii].second != bounds[ii + 1].first)
            //throw std::exception();

        size_t new_bounds = bounds[ii].second;

        while ( IsPartOfWord(contents_[new_bounds]) )
            new_bounds++;

        bounds[ii].second = bounds[ii + 1].first = new_bounds;
    }

    //std::cout << "contents_size: " << contents_size << "\n";
    //for (size_t ii = 0; ii < kNumTokenizeThreads; ++ii)
    //{
        //std::cout << boost::str(boost::format("%d: %u %u\n") % ii % bounds[ii].first % bounds[ii].second);
    //}

    // parallelize tokenization
    std::vector< std::future<int> > futures;
    for (size_t ii = 0; ii < kNumTokenizeThreads; ++ii)
    {
        std::pair<size_t, size_t> bound = bounds[ii];

        futures.push_back(std::async(
        [&]
        {
            for (size_t jj = bound.first; jj < bound.second; ++jj)
            {
                // initial case, find character in the set of ([a-z][A-Z][0-9]_)
                // this will be what is considered a word
                // I guess we have unicode stuff we are screwed :
                char c = contents_[jj];
                if (IsPartOfWord(c) == false) continue;

                // we have found the beginning of the word, loop until
                // we reach the end or we find a non world character
                size_t kk = jj + 1;
                for (; kk < contents_size; ++kk)
                {
                    if (IsPartOfWord(contents_[kk]) == true)
                        continue;
                    break;
                }
                if (kk == bound.second) break;

                // construct word based off of pointer
                std::string word(&contents_[jj], &contents_[kk]);

                //lock_.lock();
                //if (already_processed_words_.find(word) == already_processed_words_.end())
                //{
                    //words_.AddKeyword(&word[0], word, kTrieDepth);
                    //already_processed_words_.insert(word);
                //}
                //lock_.unlock();

                // for loop will autoincrement
                jj = kk;
            }

            return 0;
        }
        ));
    }

    for (auto& future : futures)
    {
        int ret = future.get();
    }

    return;

}

