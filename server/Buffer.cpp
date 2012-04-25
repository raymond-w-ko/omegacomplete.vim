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
words_(nullptr),
trie_(nullptr),
title_cases_(nullptr),
underscores_(nullptr),
abbreviations_dirty_(true),
cursor_pos_(0, 0)
{
	
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
	//std::cout << "prev: " << cursor_pos_.first << "\n";
	//std::cout << "cur: " << cursor_pos.first << "\n";
	// if our cursor row has changed, then capture the contents of the original
	// current line before any changes have occurred
	if (cursor_pos.first != cursor_pos_.first)
	{
		//std::cout << "reparse all\n";
		initial_current_line_ = cur_line;
		tokenizeKeywordsOfOriginalCurrentLine(initial_current_line_);

		// by changing lines in insert mode, we have to reparse buffer
		// to keep it current
		if (contents_ != new_contents)
		{
			contents_ = new_contents;
			tokenizeKeywords();
		}
	}

	// calling the function implies that the current line has changed,
	// so parse the current line
	tokenizeKeywordsOfCurrentLine(cur_line);

	// end() - 1 because of extra byte at the end?
	prev_cur_line_ = std::string(cur_line.begin(), cur_line.end() - 1);
	cursor_pos_ = cursor_pos;
}

void Buffer::ParseNormalMode(
	const std::string& new_contents)
{
	if (contents_ == new_contents) return;
	
	contents_ = new_contents;

	Stopwatch watch;
	watch.Start();
    tokenizeKeywords();
	watch.Stop();
	std::cout << "tokenizeKeywords(): "; watch.PrintResultMilliseconds();
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
	// re-tokenizing means clearing existing wordlist
	//Stopwatch watch;
	//watch.Start();
	current_line_words_.clear();
	parent_->GD.QueueForDeletion(words_);
	words_ = new boost::unordered_map<std::string, unsigned>;
	//watch.Stop(); watch.PrintResultMilliseconds();

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
		(*words_)[std::move(word)]++;
		
		// for loop will autoincrement
		ii = jj;
	}

	// since we have recreated the set of words, invalidate
	// the trie structure, next time we complete, it will be
	// regenerated again
	// same thing with TitleCase structure and userscores structure

	//watch.Start();
	parent_->GD.QueueForDeletion(trie_);
	trie_ = new TrieNode();
	//watch.Stop(); watch.PrintResultMilliseconds();

	//watch.Start();
	//title_cases_.clear();
	//watch.Stop(); watch.PrintResultMilliseconds();

	//watch.Start();
	//underscores_.clear();
	//watch.Stop(); watch.PrintResultMilliseconds();
	
	//generateTitleCasesAndUnderscores();
	// defer abbrevation generation until we are in insert mode
	abbreviations_dirty_ = true;
}

void Buffer::tokenizeKeywordsHelper(
	const std::string& content, 
	boost::unordered_set<std::string>& container)
{
	size_t contents_size = content.size();

	for (size_t ii = 0; ii < contents_size; ++ii)
	{
		// initial case, find character in the set of ([a-z][A-Z][0-9]_)
		// this will be what is considered a word
		// I guess we have unicode stuff we are screwed :(
		char c = content[ii];
		if (!is_part_of_word_[c]) continue;
		
		// we have found the beginning of the word, loop until
		// we reach the end or we find a non world character
		size_t jj = ii + 1;
		for (; jj < contents_size; ++jj)
		{
			if (is_part_of_word_[content[jj]])
				continue;
			break;
		}
		if (jj == contents_size) break;
		
		// construct word based off of pointer
		std::string word(&content[ii], &content[jj]);
		container.insert(word);
		
		// for loop will autoincrement
		ii = jj;
	}
}

void Buffer::tokenizeKeywordsOfCurrentLine(const std::string& line)
{
	current_line_words_.clear();
	tokenizeKeywordsHelper(line, current_line_words_);
}

void Buffer::tokenizeKeywordsOfOriginalCurrentLine(const std::string& line)
{
	orig_cur_line_words_.clear();
	tokenizeKeywordsHelper(line, orig_cur_line_words_);
}

void Buffer::GetAllWordsWithPrefix(
	const std::string& prefix,
	std::set<std::string>* results)
{
	for (auto& word_count_pair : *words_)
	{
		const std::string& word = word_count_pair.first;
		if (boost::starts_with(word, prefix))
		{
			// check to make sure really exists
			unsigned count = word_count_pair.second;
			if (count == 1 &&
				// there is only one instance in the buffer and this line
				// originally has it
				orig_cur_line_words_.find(word) != orig_cur_line_words_.end() &&
				// but the current line doesn't have it
				current_line_words_.find(word) == current_line_words_.end())
			{
				// then the word doesn't anymore, it's just that the buffer
				// hasn't been reparsed (most likely triggered by backspace)
				continue;
			}

			results->insert(word);
		}
	}
}

void Buffer::GetAllWordsWithPrefixFromCurrentLine(
	const std::string& prefix,
	std::set<std::string>* results)
{
	for (const std::string& word : current_line_words_)
	{
		if (boost::starts_with(word, prefix))
		{
			results->insert(word);
		}
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

	for (auto& child : trie_->Children)
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
	     (node->Word != nullptr) &&
	     (node->Word->length() > 0) )
	{
		results[ current_row[last_index] ].insert(*node->Word);
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
	if ((words_->size()) > 0 && trie_->Empty())
	{
		for (auto& word_count_pair : *words_)
		{
			trie_->Insert(&word_count_pair.first);
		}
	}

	// only try fancier matching if we have a sufficiently long enough
	// word to make it worthwhile
	if (prefix.length() < kMinLengthForLevenshteinCompletion) return;

	levenshteinSearch(prefix, kLevenshteinMaxCost, results);
}

void Buffer::generateTitleCasesAndUnderscores()
{
	parent_->GD.QueueForDeletion(title_cases_);
	title_cases_ = new boost::unordered_multimap<std::string, std::string>;

	parent_->GD.QueueForDeletion(underscores_);
	underscores_ = new boost::unordered_multimap<std::string, std::string>;

	for (auto& word_count_pair : *words_)
	{
		const std::string& word = word_count_pair.first;
		if (word.length() <= 2) continue;

		std::string title_case;
		std::string underscore;

		for (size_t ii = 0; ii < word.length(); ++ii)
		{
			char c = word[ii];
			
			if (ii == 0)
			{
				title_case += c;
				underscore += c;

				continue;
			}

			if (IsUpper(c))
			{
				title_case += c;
			}

			if (word[ii] == '_')
			{
				if (ii < (word.length() - 1) && word[ii + 1] != '_')
					underscore += word[ii + 1];
			}
		}

		if (title_case.length() >= 2)
		{
			boost::algorithm::to_lower(title_case);
			title_cases_->insert(std::make_pair(title_case, word));
		}

		if (underscore.length() >= 2)
		{
			underscores_->insert(std::make_pair(underscore, word));
		}
	}

	abbreviations_dirty_ = false;
}

void Buffer::GetAbbrCompletions(
	const std::string& prefix,
	std::set<std::string>* results)
{
	// generate abbreviations if they already haven't
	if (abbreviations_dirty_)
	{
		generateTitleCasesAndUnderscores();	
	}
	
	if (prefix.length() < 2) return;

	auto bounds1 = title_cases_->equal_range(prefix);
	for (auto& ii = bounds1.first; ii != bounds1.second; ++ii)
	{
		results->insert(ii->second);
	}

	auto bounds2 = underscores_->equal_range(prefix);
	for (auto& ii = bounds2.first; ii != bounds2.second; ++ii)
	{
		results->insert(ii->second);
	}
}
