#include "stdafx.hpp"

#include "Buffer.hpp"

typedef std::map< int, std::set<std::string> > LevenshteinSearchResults;

    void GetLevenshteinCompletions(
        const std::string& prefix,
        LevenshteinSearchResults& results);

    void levenshteinSearch(
        const std::string& word,
        int max_cost,
        LevenshteinSearchResults& results);

    void levenshteinSearchRecursive(
        TrieNode* node,
        char letter,
        const std::string& word,
        const std::vector<int>& previous_row,
        LevenshteinSearchResults& results,
        int max_cost);

void Buffer::GetLevenshteinCompletions(
    const std::string& prefix,
    LevenshteinSearchResults& results)
{
    if ((words_->size()) > 0 && trie_->Empty())
    {
        //Stopwatch watch; watch.Start();
        foreach (WordsIterator& word_count_pair, *words_)
        {
            trie_->Insert(&word_count_pair.first);
        }
        //watch.Stop();
        //std::cout << "trie_ creation time: "; watch.PrintResultMilliseconds();
    }

    // only try fancier matching if we have a sufficiently long enough
    // word to make it worthwhile
    if (prefix.length() < kMinLengthForLevenshteinCompletion) return;

    levenshteinSearch(prefix, kLevenshteinMaxCost, results);
}


// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Buffer::levenshteinSearch(
    const std::string& word,
    int max_cost,
    LevenshteinSearchResults& results)
{
    // generate sequence from [0, len(word)]
    size_t current_row_end = word.length() + 1;
    std::vector<int> current_row;
    for (size_t ii = 0; ii < current_row_end; ++ii) current_row.push_back(ii);

    foreach (TrieNode::ChildrenIterator& child, trie_->Children)
    {
        char letter = child.first;
        TrieNode* next_node = child.second;
        levenshteinSearchRecursive(
            next_node,
            letter,
            word,
            current_row,
            results,
            max_cost);
    }
}

// translated from Python code provided by
// http://stevehanov.ca/blog/index.php?id=114
void Buffer::levenshteinSearchRecursive(
    TrieNode* node,
    char letter,
    const std::string& word,
    const std::vector<int>& previous_row,
    LevenshteinSearchResults& results,
    int max_cost)
{
    size_t columns = word.length() + 1;
    std::vector<int> current_row;
    current_row.push_back( previous_row[0] + 1 );

    // Build one row for the letter, with a column for each letter in the target
    // word, plus one for the empty string at column 0
    for (unsigned column = 1; column < columns; ++column)
    {
        int insert_cost = current_row[column - 1] + 1;
        int delete_cost = previous_row[column] + 1;

        int replace_cost;
        if (word[column - 1] != letter)
            replace_cost = previous_row[column - 1] + 1;
        else
            replace_cost = previous_row[column - 1];

        current_row.push_back( (std::min)(
            insert_cost,
            (std::min)(delete_cost, replace_cost)) );
    }

    // if the last entry in the row indicates the optimal cost is less than the
    // maximum cost, and there is a word in this trie node, then add it.
    size_t last_index = current_row.size() - 1;
    if ( (current_row[last_index] <= max_cost) &&
         (node->Word != NULL) &&
         (node->Word->length() > 0) )
    {
        results[ current_row[last_index] ].insert(*node->Word);
    }

    // if any entries in the row are less than the maximum cost, then
    // recursively search each branch of the trie
    if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost)
    {
        foreach (TrieNode::ChildrenIterator& child, node->Children)
        {
            char letter = child.first;
            TrieNode* next_node = child.second;
            levenshteinSearchRecursive(
                next_node,
                letter,
                word,
                current_row,
                results,
                max_cost);
        }
    }
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

