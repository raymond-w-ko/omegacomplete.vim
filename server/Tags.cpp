#include "stdafx.hpp"

#include "Tags.hpp"

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
        std::cout << "couldn't calculate parent directory for tags file, not parsing" << std::endl;
        std::cout << pathname_ << std::endl;
        return false;
    }

    thread_ = std::thread(&Tags::Update, this);

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

        int index = 2;
        std::string ex;
        for (; index < tokens.size(); ++index)
        {
            std::string token = tokens[index];

            if (token.empty()) token = "\t";

            ex += token;

            if (boost::ends_with(token, "\"")) break;
        }
        if (boost::ends_with(ex, "\"") == false)
        {
            std::cout << "Ex didn't end with \"" << std::endl;
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
                return;
            }

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

void Tags::VimTaglistFunction(
    const std::string& expr,
    const std::vector<std::string>& tags_list,
    std::stringstream& ss)
{
    mutex_.lock();
    for (const std::string& word : words_)
    {
        if (word != expr) continue;
        if (Contains(tags_, expr) == false) continue;

        auto bounds = tags_.equal_range(expr);
        for (auto ii = bounds.first; ii != bounds.second; ++ii)
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
    auto bounds = tags_.equal_range(word);

    for (auto& iter = bounds.first; iter != bounds.second; ++iter)
    {
        const TagInfo& tag_info = iter->second;
        ss << "{";

        ss << boost::str(boost::format(
            "'name':'%s','filename':'%s','cmd':'%s',")
            % tag_info.Tag
            % tag_info.Location
            % tag_info.Ex);
        for (const auto& pair : tag_info.Info)
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

void Tags::Update()
{
    bool reparse_needed = false;

#ifdef WIN32
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
#endif

    if (reparse_needed)
    {
        reparse();
    }
}
