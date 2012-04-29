#include "stdafx.hpp"

#include "Tags.hpp"

Tags::Tags(const std::string pathname)
:
pathname_(pathname)
{
    std::string temp(1, ' ');

    for (size_t index = 0; index <= 255; ++index)
    {
        is_part_of_word_[index] = IsPartOfWord(index) ? 1 : 0;
        temp.resize(1, ' ');
        temp[0] = (char)index;
        boost::algorithm::to_lower(temp);
        to_lower_[index] = temp[0];
    }

    reparse();
}

Tags::~Tags()
{
    ;
}

void Tags::reparse()
{
    mutex_.lock();

    tags_.clear();
    words_.clear();
    title_cases_.clear();
    underscores_.clear();

    std::ifstream file(pathname_);
    unsigned line_num = 0;

    for (std::string line; std::getline(file, line).good(); ++line_num)
    {
        // the first 6 lines contain a description of the tags file, which we don't need
        if (line_num < 6) continue;

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of("\t"), boost::token_compress_off);

        if (tokens.size() < 3) throw "invalid tag line detected!";

        TagInfo tag_info;
        tag_info.Tag = tokens[0];
        tag_info.Location = tokens[1];

        int index = 2;
        std::string ex;
        for (; index < tokens.size(); ++index)
        {
            std::string token = tokens[index];

            if (token.empty()) token = "\t";

            ex += token;

            if (boost::ends_with(token, "\"")) break;
        }
        if (boost::ends_with(ex, "\"") == false) throw "Ex didn't end with \"";

        tag_info.Ex = ex;

        // increment to next token, which should be the more info fields
        index++;

        for (; index < tokens.size(); ++index)
        {
            std::string token = tokens[index];
            size_t colon = token.find(":");

            std::string key = std::string(token.begin(), token.begin() + colon);
            std::string value = std::string(token.begin() + colon + 1, token.end());
            tag_info.Info[key] = value;
        }

        tags_.insert( make_pair(tag_info.Tag, tag_info) );
        words_.insert(tag_info.Tag);
    }

    for (const std::string& word : words_)
    {
        std::string title_case, underscore;
        CalculateTitlecaseAndUnderscore(word, to_lower_, title_case, underscore);

        if (title_case.length() >= 2)
        {
            title_cases_.insert(std::make_pair(title_case, &word));
        }
        if (underscore.length() >= 2)
        {
            underscores_.insert(std::make_pair(underscore, &word));
        }
    }

    mutex_.unlock();
}

void Tags::GetAllWordsWithPrefix(
    const std::string& prefix,
    std::set<std::string>* results)
{
    mutex_.lock();
    for (const std::string& word : words_)
    {
        if (boost::starts_with(word, prefix))
        {
            results->insert(word);
        }
    }
    mutex_.unlock();
}

void Tags::GetAbbrCompletions(
    const std::string& prefix,
    std::set<std::string>* results)
{
    if (prefix.length() < 2) return;

    mutex_.lock();
    auto bounds1 = title_cases_.equal_range(prefix);
    for (auto& ii = bounds1.first; ii != bounds1.second; ++ii)
    {
        results->insert(*ii->second);
    }

    auto bounds2 = underscores_.equal_range(prefix);
    for (auto& ii = bounds2.first; ii != bounds2.second; ++ii)
    {
        results->insert(*ii->second);
    }
    mutex_.unlock();
}
