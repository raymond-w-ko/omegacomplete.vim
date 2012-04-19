#pragma once

class KeywordTrie
{
public:
    KeywordTrie();
    ~KeywordTrie();
    
    void Clear();
    void AddKeyword(
        const char* word_part, const std::string& whole_word,
        const unsigned int depth_remaining);
    
    void GetAllWordsWithPrefix(
        const char* prefix,
		const std::string& whole_prefix,
        std::vector<std::string>* matching_words,
		const unsigned int depth_remaining);

private:
    boost::unordered_map<char, boost::shared_ptr<KeywordTrie>> letters_;
    // this is case sensitive
    boost::unordered_set<std::string> words_;
};
