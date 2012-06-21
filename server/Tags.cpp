#include "stdafx.hpp"

#include "Tags.hpp"
#include "GlobalWordSet.hpp"

Tags::Tags()
:
last_write_time_(0)
{
    ;
}

bool Tags::Init(const std::string& pathname)
{
    pathname_ = pathname;

    std::string temp(1, ' ');
    for (size_t index = 0; index <= 255; ++index)
    {
        is_part_of_word_[index] = IsPartOfWord(index) ? 1 : 0;
        temp.resize(1, ' ');
        temp[0] = (char)index;
        boost::algorithm::to_lower(temp);
        to_lower_[index] = temp[0];
    }

    if (calculateParentDirectory() == false)
    {
        std::cout << "couldn't calculate parent directory for tags file, not parsing"
                  << std::endl;
        std::cout << pathname_ << std::endl;
        return false;
    }

    thread_ = boost::thread(&Tags::Update, this);

    return true;
}

bool Tags::calculateParentDirectory()
{
    // normalize Windows pathnames to UNIX format
    boost::replace_all(pathname_, "\\", "/");

    size_t pos = pathname_.rfind('/');
    // error out because we can't find the last '/'
    if (pos == std::string::npos)
    {
        std::cout << "coulnd't find last directory separator" << std::endl;
        std::cout << pathname_ << std::endl;
        return false;
    }
    // there is nothing after the '/'
    if (pos >= (pathname_.size() - 1))
    {
        std::cout << "there is nothing after the last '/'" << std::endl;
        std::cout << pathname_ << std::endl;
        return false;
    }

    parent_directory_ = std::string(pathname_.begin(), pathname_.begin() + pos);

    return true;
}

Tags::Tags(const Tags& other)
{
    other.mutex_.lock();

    for (int ii = 0; ii < 256; ++ii)
    {
        is_part_of_word_[ii] = other.is_part_of_word_[ii];
        to_lower_[ii] = other.to_lower_[ii];
    }

    pathname_ = other.pathname_;
    last_write_time_ = other.last_write_time_;
    parent_directory_ = other.parent_directory_;

    tags_ = other.tags_;
    words_ = other.words_;
    title_cases_ = other.title_cases_;
    underscores_ = other.underscores_;
    title_case_cache_ = other.title_case_cache_;
    underscore_cache_ = other.underscore_cache_;

    other.mutex_.unlock();
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

    std::ifstream file(pathname_.c_str());
    unsigned line_num = 0;

    for (std::string line; std::getline(file, line).good(); ++line_num)
    {
        // the first 6 lines contain a description of the tags file, which we don't need
        if (line_num < 6) continue;

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of("\t"), boost::token_compress_off);

        if (tokens.size() < 3)
        {
            std::cout << "invalid tag line detected!" << std::endl;
            return;
        }

        TagInfo tag_info;
        tag_info.Tag = tokens[0];
        tag_info.Location = tokens[1];
        // normalize Windows pathnames to UNIX format
        boost::replace_all(tag_info.Location, "\\", "/");
        // if there is a dot slash in front, replace with location of tags
        if (tag_info.Location.length() > 1 && tag_info.Location[0] == '.')
        {
            boost::replace_first(tag_info.Location, ".", parent_directory_);
        }
        // if there is no slash at all then it is in the current directory of the tags
        if (tag_info.Location.find("/") == std::string::npos)
        {
            tag_info.Location = parent_directory_ + "/" + tag_info.Location;
        }

        size_t index = 2;
        std::string ex;
        for (; index < tokens.size(); ++index)
        {
            std::string token = tokens[index];

            if (token.empty()) token = "\t";

            ex += token;

            if (boost::ends_with(token, ";\"")) break;
        }
        if (boost::ends_with(ex, "\"") == false)
        {
            std::cout << "Ex didn't end with ;\"" << std::endl;
            return;
        }

        tag_info.Ex = ex;

        // increment to next token, which should be the more info fields
        index++;

        for (; index < tokens.size(); ++index)
        {
            std::string token = tokens[index];
            size_t colon = token.find(":");
            if (colon == std::string::npos)
            {
                std::cout << "expected key value pair, but could not find ':'" << std::endl;
                std::cout << line << std::endl;
                continue;
            }

            std::string key = std::string(token.begin(), token.begin() + colon);
            std::string value = std::string(token.begin() + colon + 1, token.end());
            tag_info.Info[key] = value;
        }

        // calculate prefix from Ex command
        // this can usually be used as the return type of the tag
        size_t header_index = tag_info.Ex.find("/^");
        size_t footer_index = tag_info.Ex.find(tag_info.Tag);
        if ((header_index != std::string::npos) &&
            (footer_index != std::string::npos)) {
            header_index += 2;
            std::string tag_prefix = tag_info.Ex.substr(
                header_index,
                footer_index - header_index);
            // search back until we find a space to handle complex
            // return types like
            // std::map<int, int> FunctionName
            // and
            // void Class::FunctionName
            footer_index = tag_prefix.rfind(" ");
            if (footer_index != std::string::npos) {
                tag_prefix = tag_prefix.substr(0, footer_index);
            }
            boost::trim(tag_prefix);
            tag_info.Info["prefix"] = tag_prefix;
        }

        tags_.insert( make_pair(tag_info.Tag, tag_info) );
        words_.insert(tag_info.Tag);
    }

    foreach (const std::string& word, words_)
    {
        const StringVector* title_cases = GlobalWordSet::ComputeTitleCase(
            word, title_case_cache_);
        const StringVector* underscores = GlobalWordSet::ComputeUnderscore(
            word, underscore_cache_);

        if (title_cases->size() > 0) {
            foreach (const std::string& title_case, *title_cases) {
                title_cases_.insert(std::make_pair(title_case, &word));
            }
        }
        if (underscores->size() > 0) {
            foreach (const std::string& underscore, *underscores) {
                underscores_.insert(std::make_pair(underscore, &word));
            }
        }
    }

    mutex_.unlock();
}

void Tags::VimTaglistFunction(
    const std::string& expr,
    const std::vector<std::string>& tags_list,
    std::stringstream& ss)
{
    mutex_.lock();
    foreach (const std::string& word, words_)
    {
        if (word != expr) continue;
        if (Contains(tags_, expr) == false) continue;

        auto(bounds, tags_.equal_range(expr));
        auto(ii, bounds.first);
        for (; ii != bounds.second; ++ii)
        {
            const TagInfo& ti = ii->second;
            ss << "{";
            ss << "},";
        }
    }
    mutex_.unlock();
}

void Tags::VimTaglistFunction(
    const std::string& word,
    std::stringstream& ss)
{
    auto(bounds, tags_.equal_range(word));
    auto(&iter, bounds.first);
    for (; iter != bounds.second; ++iter)
    {
        const TagInfo& tag_info = iter->second;
        ss << "{";

        ss << boost::str(boost::format(
            "'name':'%s','filename':'%s','cmd':'%s',")
            % tag_info.Tag
            % tag_info.Location
            % tag_info.Ex);
        foreach (const TagInfo::InfoIterator& pair, tag_info.Info)
        {
            ss << boost::str(boost::format(
                "'%s':'%s',")
                % pair.first % pair.second);
        }

        ss << "},";
    }
}

void Tags::GetAllWordsWithPrefix(
    const std::string& prefix,
    std::set<std::string>* results)
{
    mutex_.lock();
    foreach (const std::string& word, words_)
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
    auto(bounds1, title_cases_.equal_range(prefix));
    auto(ii, bounds1.first);
    for (ii = bounds1.first; ii != bounds1.second; ++ii)
    {
        results->insert(*ii->second);
    }

    auto(bounds2, underscores_.equal_range(prefix));
    for (ii = bounds2.first; ii != bounds2.second; ++ii)
    {
        results->insert(*ii->second);
    }
    mutex_.unlock();
}

void Tags::Update()
{
    bool reparse_needed = false;

#ifdef _WIN32
    HANDLE hFile = ::CreateFile(
        pathname_.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "could not open file to check last modified time" << std::endl;
        return;
    }

    FILETIME ft_last_write_time;
    ::GetFileTime(
        hFile,
        NULL,
        NULL,
        &ft_last_write_time);

    __int64 last_write_time = to_int64(ft_last_write_time);
    if (last_write_time > last_write_time_)
    {
        reparse_needed = true;

        last_write_time_ = last_write_time;
    }

    CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
#else
    // TODO(rko): implement for UNIX OSes
#endif

    if (reparse_needed)
    {
        reparse();
    }
}
