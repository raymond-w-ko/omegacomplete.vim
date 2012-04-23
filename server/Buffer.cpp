#include "stdafx.hpp"

#include "Buffer.hpp"
#include "Session.hpp"
#include "Stopwatch.hpp"

const unsigned int kTrieDepth = 2;
const unsigned int kNumThreads = 8;
const int kLevenshteinMaxCost = 2;
const size_t kMinLengthForLevenshteinCompletion = 4;

Buffer::Buffer()
:
cursor_pos_(0, 0)
{
    //word_split_regex_ = boost::xpressive::sregex::compile("\\W+");
	
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
			trie_.Clear();
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
	trie_.Clear();
	current_line_words_.clear();
	//watch.Stop(); watch.PrintResultMilliseconds();

	// around 375 ms, this is the bottleneck
	watch.Start();
    tokenizeKeywords();
	watch.Stop(); watch.PrintResultMilliseconds();
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
		words_.insert(std::move(word));
		
		// for loop will autoincrement
		ii = jj;
	}

	// since we have recreated the set of words, invalidate
	// the trie structure, next time we complete, it will be
	// regenerated again
	trie_.Clear();

	// build trie of all the unique words in the buffer
	//for (const std::string& word : words_)
	//{
		//trie_.Insert(word);
	//}
	
	
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
	std::set<std::string>* results)
{
	// with std::set
	//auto iter = words_.lower_bound(prefix);
	//while (iter != words_.end() && boost::starts_with(*iter, prefix))
	//{
		//results->emplace_back(*iter);
		//++iter;
	//}
	
	// with boost::unordered_set
	for (const std::string& word : words_)
	{
		if (boost::starts_with(word, prefix))
		{
			results->insert(word);
		}
	}
}

void Buffer::GetAllWordsWithPrefixFromCurrentLine(
	const std::string& prefix,
	std::set<std::string>* results)
{
	auto iter = current_line_words_.lower_bound(prefix);
	while (iter != current_line_words_.end() && boost::starts_with(*iter, prefix))
	{
		results->insert(*iter);
		++iter;
	}
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

	for (auto& child : trie_.Children)
	{
		char letter = child.first;
		TrieNode* next_node = child.second.get();
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
	for (int column = 1; column < columns; ++column)
	{
		int insert_cost = current_row[column - 1] + 1;
		int delete_cost = previous_row[column] + 1;

		int replace_cost;
		if (word[column - 1] != letter)
			replace_cost = previous_row[column - 1] + 1;
		else
			replace_cost = previous_row[column - 1];

		current_row.push_back( std::min(
			insert_cost,
			std::min(delete_cost, replace_cost)) );
	}

	// if the last entry in the row indicates the optimal cost is less than the
	// maximum cost, and there is a word in this trie node, then add it.
	size_t last_index = current_row.size() - 1; 
	if ( (current_row[last_index] <= max_cost) &&
	     (node->Word.length() > 0) )
	{
		results[ current_row[last_index] ].insert(node->Word);
	}

	// if any entries in the row are less than the maximum cost, then 
	// recursively search each branch of the trie
	if (*std::min_element(current_row.begin(), current_row.end()) <= max_cost)
	{
		for (auto& child : node->Children)
		{
			char letter = child.first;
			TrieNode* next_node = child.second.get();
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

void Buffer::GetLevenshteinCompletions(
	const std::string& prefix,
	LevenshteinSearchResults& results)
{
	if ((words_.size()) > 0 && trie_.Empty())
	{
		for (const std::string& word : words_)
		{
			trie_.Insert(word);
		}
	}

	// only try fancier matching if we have a sufficiently long enough
	// word to make it worthwhile
	if (prefix.length() >= kMinLengthForLevenshteinCompletion) return;

	levenshteinSearch(prefix, kLevenshteinMaxCost, results);
}
