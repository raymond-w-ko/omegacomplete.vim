#include "stdafx.hpp"

#include "KeywordTrie.hpp"

KeywordTrie::KeywordTrie()
{
    ;
}

KeywordTrie::~KeywordTrie()
{
    ;
}

void KeywordTrie::Clear()
{
    letters_.clear();
}

void KeywordTrie::AddKeyword(
    const char* word_part, const std::string& whole_word,
    const unsigned int depth_remaining)
{
    // if we hit the NUL char or reached our target depth, add word
    if (depth_remaining == 0 || (*word_part) == '\0')
    {
        words_.insert(whole_word);
    }
    else
    {
        auto& next_letter = letters_[*word_part];
        if (next_letter == NULL) {
            next_letter = boost::make_shared<KeywordTrie>();
        }

        next_letter->AddKeyword(
            ++word_part,
            whole_word,
            depth_remaining - 1);
    }
}

void KeywordTrie::GetAllWordsWithPrefix(
    const char* prefix,
    std::vector<std::string>* matching_words)
{
    if (prefix == nullptr || (*prefix) == '\0')
    {
        for (const std::string& word : words_)
        {
            matching_words->push_back(word);
        }
        
        // store all words in children also
        for (auto& child : letters_)
        {
            child.second->GetAllWordsWithPrefix(nullptr, matching_words);
        }
    }
    else
    {
        auto& next_letter = letters_[*prefix];
        if (next_letter == NULL) return;

        letters_[*prefix]->GetAllWordsWithPrefix(prefix + 1, matching_words);
    }
}
