#include "stdafx.hpp"

#include "OmegaComplete.hpp"
#include "TagsSet.hpp"
#include "Stopwatch.hpp"
#include "LookupTable.hpp"
#include "CompletionSet.hpp"
#include "Teleprompter.hpp"
#include "Algorithm.hpp"

std::vector<char> OmegaComplete::QuickMatchKey;
boost::unordered_map<char, unsigned> OmegaComplete::ReverseQuickMatch;
const std::string OmegaComplete::default_response_ = "ACK";
OmegaComplete* OmegaComplete::instance_ = NULL;

void OmegaComplete::InitGlobal()
{
    // dependencies in other classes that have to initialized first
    LookupTable::InitGlobal();
    TagsSet::InitGlobal();
    Algorithm::InitGlobal();
#if defined (_WIN32) && defined (TELEPROMPTER)
    Teleprompter::InitGlobal();
#endif

    // this class's own initialization
    QuickMatchKey.resize(CompletionSet::kMaxNumCompletions, ' '),

    QuickMatchKey[0] = '1';
    QuickMatchKey[1] = '2';
    QuickMatchKey[2] = '3';
    QuickMatchKey[3] = '4';
    QuickMatchKey[4] = '5';
    QuickMatchKey[5] = '6';
    QuickMatchKey[6] = '7';
    QuickMatchKey[7] = '8';
    QuickMatchKey[8] = '9';
    QuickMatchKey[9] = '0';

    for (unsigned ii = 0; ii < 10; ++ii)
    {
        ReverseQuickMatch[QuickMatchKey[ii]] = ii;
    }

    instance_ = new OmegaComplete;
}

OmegaComplete::OmegaComplete()
:
is_quitting_(0),
prev_input_(3),
is_corrections_only_(false)
{
    initCommandDispatcher();

    worker_thread_ = boost::thread(
        &OmegaComplete::workerThreadLoop,
        this);

#ifdef ENABLE_CLANG_COMPLETION
    clang_.Init();
#endif
}

void OmegaComplete::initCommandDispatcher()
{
    command_dispatcher_["current_buffer_id"] = boost::bind(
        &OmegaComplete::cmdCurrentBufferId, boost::ref(*this), _1);

    command_dispatcher_["current_buffer_absolute_path"] = boost::bind(
        &OmegaComplete::cmdCurrentBufferAbsolutePath, boost::ref(*this), _1);

    command_dispatcher_["current_line"] = boost::bind(
        &OmegaComplete::cmdCurrentLine, boost::ref(*this), _1);

    command_dispatcher_["cursor_position"] = boost::bind(
        &OmegaComplete::cmdCursorPosition, boost::ref(*this), _1);

    command_dispatcher_["buffer_contents"] = boost::bind(
        &OmegaComplete::cmdBufferContents, boost::ref(*this), _1);

    command_dispatcher_["complete"] = boost::bind(
        &OmegaComplete::cmdComplete, boost::ref(*this), _1);

    command_dispatcher_["free_buffer"] = boost::bind(
        &OmegaComplete::cmdFreeBuffer, boost::ref(*this), _1);

    command_dispatcher_["current_directory"] = boost::bind(
        &OmegaComplete::cmdCurrentDirectory, boost::ref(*this), _1);

    command_dispatcher_["current_tags"] = boost::bind(
        &OmegaComplete::cmdCurrentTags, boost::ref(*this), _1);

    command_dispatcher_["taglist_tags"] = boost::bind(
        &OmegaComplete::cmdTaglistTags, boost::ref(*this), _1);

    command_dispatcher_["vim_taglist_function"] = boost::bind(
        &OmegaComplete::cmdVimTaglistFunction, boost::ref(*this), _1);

    command_dispatcher_["prune"] = boost::bind(
        &OmegaComplete::cmdPrune, boost::ref(*this), _1);

    command_dispatcher_["hide_teleprompter"] = boost::bind(
        &OmegaComplete::cmdHideTeleprompter, boost::ref(*this), _1);

    command_dispatcher_["flush_caches"] = boost::bind(
        &OmegaComplete::cmdFlushCaches, boost::ref(*this), _1);

    command_dispatcher_["is_corrections_only"] = boost::bind(
        &OmegaComplete::cmdIsCorrectionsOnly, boost::ref(*this), _1);
}

OmegaComplete::~OmegaComplete()
{
    is_quitting_ = 1;
    worker_thread_.join();
}

const std::string OmegaComplete::Eval(const char* request, const int request_len)
{
    // find first space
    int index = -1;
    for (int ii = 0; ii < request_len; ++ii)
    {
        if (request[ii] == ' ')
        {
            index = ii;
            break;
        }
    }

    if (index == -1)
        throw std::exception();

    // break it up into a "request" string and a "argument"
    std::string command(request, request + index);
    StringPtr argument = boost::make_shared<std::string>(
        request + index + 1, request);

    auto(iter, command_dispatcher_.find(command));
    if (iter == command_dispatcher_.end())
    {
        std::cout << boost::str(boost::format(
            "unknown command %s %s") % command % *argument);

        return default_response_;
    }
    else
    {
        return iter->second(argument);
    }
}

std::string OmegaComplete::cmdCurrentBufferId(StringPtr argument)
{
    current_buffer_id_ = boost::lexical_cast<unsigned>(*argument);
    if (Contains(buffers_, current_buffer_id_) == false)
    {
        buffers_[current_buffer_id_].Init(this, current_buffer_id_);
    }

    return default_response_;
}

std::string OmegaComplete::cmdCurrentBufferAbsolutePath(StringPtr argument)
{
    current_buffer_absolute_path_ = *argument;

    return default_response_;
}

std::string OmegaComplete::cmdCurrentLine(StringPtr argument)
{
    current_line_ = *argument;

    return default_response_;
}

std::string OmegaComplete::cmdCursorPosition(StringPtr argument)
{
    std::vector<std::string> position;
    boost::split(
        position,
        *argument,
        boost::is_any_of(" "),
        boost::token_compress_on);
    unsigned x = boost::lexical_cast<unsigned>(position[0]);
    unsigned y = boost::lexical_cast<unsigned>(position[1]);
    cursor_pos_.Line = x; cursor_pos_.Column = y;

    buffers_[current_buffer_id_].CalculateCurrentWordOfCursor(
        current_line_,
        cursor_pos_);

    return default_response_;
}

std::string OmegaComplete::cmdBufferContents(StringPtr argument)
{
    current_contents_ = argument;

    ParseJob job(current_buffer_id_, current_contents_);
    queueParseJob(job);

#ifdef ENABLE_CLANG_COMPLETION
    clang_.CreateOrUpdate(current_buffer_absolute_path_, current_contents_);
#endif

    return default_response_;
}

std::string OmegaComplete::cmdComplete(StringPtr argument)
{
    //Stopwatch watch; watch.Start();
    std::string response;
    response.reserve(8192);
    calculateCompletionCandidates(*argument, response);
    //watch.Stop(); std::cout << "complete: "; watch.PrintResultMilliseconds();

    return response;
}

std::string OmegaComplete::cmdFreeBuffer(StringPtr argument)
{
    // make sure job queue is empty before we can delete buffer
    while (true) {
        job_queue_mutex_.lock();
        if (job_queue_.size() == 0) {
            break;
        }
        job_queue_mutex_.unlock();
    }

    buffers_mutex_.lock();
    buffers_.erase(boost::lexical_cast<unsigned>(*argument));
    buffers_mutex_.unlock();
    job_queue_mutex_.unlock();

    return default_response_;
}

std::string OmegaComplete::cmdCurrentDirectory(StringPtr argument)
{
    current_directory_ = *argument;

    return default_response_;
}

std::string OmegaComplete::cmdCurrentTags(StringPtr argument)
{
    current_tags_.clear();

    std::vector<std::string> tags_list;
    boost::split(tags_list, *argument,
                    boost::is_any_of(","), boost::token_compress_on);
    foreach (const std::string& tags, tags_list)
    {
        if (tags.size() == 0) continue;

        TagsSet::Instance()->CreateOrUpdate(tags, current_directory_);
        current_tags_.push_back(tags);
    }

    return default_response_;
}

std::string OmegaComplete::cmdTaglistTags(StringPtr argument)
{
    taglist_tags_.clear();

    std::vector<std::string> tags_list;
    boost::split(tags_list, *argument,
                    boost::is_any_of(","), boost::token_compress_on);
    foreach (const std::string& tags, tags_list)
    {
        if (tags.size() == 0) continue;

        TagsSet::Instance()->CreateOrUpdate(tags, current_directory_);
        taglist_tags_.push_back(tags);
    }

    return default_response_;
}

std::string OmegaComplete::cmdVimTaglistFunction(StringPtr argument)
{
    const std::string& response = TagsSet::Instance()->VimTaglistFunction(
        *argument, taglist_tags_, current_directory_);

    return response;
}

std::string OmegaComplete::cmdPrune(StringPtr argument)
{

    unsigned words_pruned = WordSet.Prune();
    //std::cout << count << " words pruned" << std::endl;

    return default_response_;
}

std::string OmegaComplete::cmdHideTeleprompter(StringPtr argument)
{
#ifdef TELEPROMPTER
        Teleprompter::Instance()->Show(false);
#endif
    return default_response_;
}

std::string OmegaComplete::cmdFlushCaches(StringPtr argument)
{
    TagsSet::Instance()->Clear();
    Algorithm::ClearGlobalCache();

    return default_response_;
}

std::string OmegaComplete::cmdIsCorrectionsOnly(StringPtr argument)
{
    const std::string& response = is_corrections_only_ ? "1" : "0";
    return response;
}

void OmegaComplete::queueParseJob(ParseJob job)
{
    job_queue_mutex_.lock();
    job_queue_.push_back(job);
    job_queue_mutex_.unlock();
}

void OmegaComplete::workerThreadLoop()
{
    bool did_prune = false;

    while (true)
    {
#ifdef _WIN32
        // this is in milliseconds
        ::Sleep(1);
#else
        ::usleep(1 * 1000);
#endif
        if (is_quitting_ == 1)
            break;

        // pop off the next job
try_get_next_job:
        job_queue_mutex_.lock();
        if (job_queue_.size() > 0)
        {
            ParseJob job(job_queue_.front());
            job_queue_.pop_front();
            job_queue_mutex_.unlock();

            // do the job
            buffers_mutex_.lock();
            if (Contains(buffers_, job.BufferNumber) == false)
            {
                std::cout << "buffer " << job.BufferNumber
                          << "no longer exists! "
                          << "skipping this job" << std::endl;
                continue;
            }
            buffers_[job.BufferNumber].ReplaceContentsWith(job.Contents);
            buffers_mutex_.unlock();

            did_prune = false;

            // try and see if there is something queued up if so, then
            // immediately process it, don't go to sleep
            goto try_get_next_job;
        }
        else
        {
            job_queue_mutex_.unlock();

            if (did_prune == false) {
                WordSet.Prune();
                did_prune = true;
            }

            continue;
        }
    }
}

void OmegaComplete::calculateCompletionCandidates(
    const std::string& line,
    std::string& result)
{
#ifdef ENABLE_CLANG_COMPLETION
    if (boost::ends_with(current_buffer_absolute_path_, ".cpp"))
    {
        clang_.DoCompletion(
            current_buffer_absolute_path_,
            line,
            cursor_pos_,
            current_contents_,
            result);
    }
#endif

    genericKeywordCompletion(line, result);
}

void OmegaComplete::genericKeywordCompletion(
    const std::string& line,
    std::string& result)
{
    std::string prefix_to_complete = getWordToComplete(line);
    if (prefix_to_complete.empty())
    {
#ifdef TELEPROMPTER
        Teleprompter::Instance()->Show(false);
#endif
        return;
    }

    // keep a trailing list of previous inputs
    prev_input_[2] = prev_input_[1];
    prev_input_[1] = prev_input_[0];
    prev_input_[0] = prefix_to_complete;

    bool terminus_mode = shouldEnableTerminusMode(prefix_to_complete);
    std::string terminus_prefix;

    bool disambiguate_mode = shouldEnableDisambiguateMode(prefix_to_complete);
    char disambiguate_letter = 0;
    unsigned disambiguate_index = UINT_MAX;
    String orig_disambiguate_prefix;
    if (terminus_mode)
    {
        terminus_prefix = prefix_to_complete;
        terminus_prefix.resize( prefix_to_complete.size() - 1 );
    }
    else if (disambiguate_mode)
    {
        orig_disambiguate_prefix = prefix_to_complete;

        disambiguate_letter = prefix_to_complete[ prefix_to_complete.size() - 1 ];

        prefix_to_complete.resize( prefix_to_complete.size() - 1 );

        if (Contains(ReverseQuickMatch, disambiguate_letter) == true)
            disambiguate_index = ReverseQuickMatch[disambiguate_letter];
    }

    CompletionSet main_completion_set;
    CompletionSet terminus_completion_set;

retry_completion:

    if (terminus_mode)
    {
        always_assert(disambiguate_mode == false);
        fillCompletionSet(terminus_prefix, terminus_completion_set);
    }
    std::vector<CompleteItem> banned_words;
    if (disambiguate_mode)
    {
        banned_words.push_back(orig_disambiguate_prefix);
    }
    fillCompletionSet(prefix_to_complete, main_completion_set, &banned_words);

    // encountered an invalid disambiguation, abort and retry normally
    // as though it was not intended
    //
    // for example, should2 would falsely trigger 'disambiguate_mode'
    // but we probably want to try it as a regular prefix completion
    if (disambiguate_mode &&
        disambiguate_index >= 0 &&
        disambiguate_index >= main_completion_set.GetNumCompletions())
    {
        disambiguate_mode = false;
        disambiguate_index = UINT_MAX;

        main_completion_set.Clear();

        prefix_to_complete += boost::lexical_cast<std::string>(disambiguate_letter);

        goto retry_completion;
    }

    // only if we have no completions do we try to Levenshtein distance completion
    LevenshteinSearchResults levenshtein_completions;
    if (main_completion_set.GetNumCompletions() == 0)
    {
        WordSet.GetLevenshteinCompletions(
            prefix_to_complete,
            levenshtein_completions);

        is_corrections_only_ = true;
    }
    else
    {
        is_corrections_only_ = false;
    }

    std::vector<CompleteItem> result_list;
    std::vector<CompleteItem> terminus_result_list;

    if (terminus_mode)
    {
        terminus_completion_set.FillResults(terminus_result_list);
    }

    // this is to prevent completions from being considered
    // that are basically the word from before you press Backspace.
    if (prev_input_[1].size() == (prefix_to_complete.size() + 1) &&
        boost::starts_with(prev_input_[1], prefix_to_complete))
    {
        main_completion_set.AddBannedWord(prev_input_[1]);
        if (terminus_mode)
        {
            terminus_completion_set.AddBannedWord(prev_input_[1]);
        }
    }

    if (terminus_mode)
    {
        foreach (const CompleteItem& completion, terminus_result_list)
        {
            const std::string& word = completion.Word;
            if (word[word.size() - 1] == '_')
                main_completion_set.AddBannedWord(word);
        }
    }

    main_completion_set.FillResults(result_list);

    // convert to format that VIM expects, basically a list of dictionaries
#ifdef TELEPROMPTER
    Teleprompter::Instance()->Show(true);
    Teleprompter::Instance()->Clear();
    Teleprompter::Instance()->SetCurrentWord(prefix_to_complete);
#endif

    result += "[";
    if (disambiguate_mode == false)
    {
        if (terminus_mode)
        {
            foreach (const CompleteItem& completion, terminus_result_list)
            {
                const std::string& word = completion.Word;
                if (word[ word.size() - 1 ] != '_')
                    continue;
                if (word == prefix_to_complete)
                    continue;

                result += completion.SerializeToVimDict();
            }

#ifdef TELEPROMPTER
            Teleprompter::Instance()->AppendText(terminus_result_list);
#endif
        }

        foreach (const CompleteItem& completion, result_list)
        {
            result += completion.SerializeToVimDict();
        }
            
#ifdef TELEPROMPTER
        Teleprompter::Instance()->AppendText(result_list);
#endif

        main_completion_set.FillLevenshteinResults(
            levenshtein_completions,
            prefix_to_complete,
            result);
    }
    else
    {
        // make sure we actually have a result, or we crash
        if (result_list.size() > disambiguate_index)
        {
            CompleteItem single_result = result_list[disambiguate_index];
            single_result.Abbr = single_result.Word + " <==";
            result += single_result.SerializeToVimDict();

#ifdef TELEPROMPTER
            Teleprompter::Instance()->AppendText(
                single_result + "    <==");
#endif
        }
    }
    result += "]";

#ifdef TELEPROMPTER
    Teleprompter::Instance()->Redraw();
#endif
}


std::string OmegaComplete::getWordToComplete(const std::string& line)
{
    if (line.length() == 0) return "";

    int partial_end = line.length();
    int partial_begin = partial_end - 1;
    for (; partial_begin >= 0; --partial_begin)
    {
        char c = line[partial_begin];
        if (LookupTable::IsPartOfWord[c])
            continue;

        break;
    }

    if ((partial_begin + 1) == partial_end) return "";

    std::string partial( &line[partial_begin + 1], &line[partial_end] );
    return partial;
}

bool OmegaComplete::shouldEnableDisambiguateMode(const std::string& word)
{
    if (word.size() < 2) return false;
    if ( !LookupTable::IsNumber[ word[word.size() - 1] ] )
        return false;

    return true;
}

bool OmegaComplete::shouldEnableTerminusMode(const std::string& word)
{
    if (word.size() < 2) return false;

    if (word[ word.size() - 1 ] == '_')
        return true;

    return false;
}

void OmegaComplete::fillCompletionSet(
    const std::string& prefix_to_complete,
    CompletionSet& completion_set,
    const std::vector<CompleteItem>* banned_words)
{
    this->WordSet.GetAbbrCompletions(
        prefix_to_complete,
        &completion_set.AbbrCompletions);

    TagsSet::Instance()->GetAbbrCompletions(
        prefix_to_complete, current_tags_, current_directory_,
        &completion_set.TagsAbbrCompletions);

    this->WordSet.GetPrefixCompletions(
        prefix_to_complete,
        &completion_set.PrefixCompletions);

    TagsSet::Instance()->GetAllWordsWithPrefix(
        prefix_to_complete, current_tags_, current_directory_,
        &completion_set.TagsPrefixCompletions);

    CompleteItem repeat(prefix_to_complete);
    completion_set.AbbrCompletions.erase(repeat);
    completion_set.TagsAbbrCompletions.erase(repeat);
    completion_set.PrefixCompletions.erase(repeat);
    completion_set.TagsPrefixCompletions.erase(repeat);

    if (banned_words == NULL)
        return;

    foreach (const CompleteItem& completion, *banned_words)
    {
        completion_set.AbbrCompletions.erase(completion);
        completion_set.TagsAbbrCompletions.erase(completion);
        completion_set.PrefixCompletions.erase(completion);
        completion_set.TagsPrefixCompletions.erase(completion);
    }
}
