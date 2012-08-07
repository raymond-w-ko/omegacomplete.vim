#include "stdafx.hpp"

#include "Stopwatch.hpp"
#include "ClangCompleter.hpp"

ClangCompleter::ClangCompleter()
:
is_quitting_(0)
{
}

void ClangCompleter::Init()
{
    std::cout << "initializing a Clang index" << std::endl;

    int exclude_declarations_from_pch = 0;
    int display_diagnostics = 1;
    index_ = clang_createIndex(exclude_declarations_from_pch, display_diagnostics);

    clang_CXIndex_setGlobalOptions(
        index_,
        CXGlobalOpt_ThreadBackgroundPriorityForAll);

    worker_thread_ = boost::thread(
        &ClangCompleter::workerThreadLoop,
        this);
}

ClangCompleter::~ClangCompleter()
{
    is_quitting_ = 1;
    worker_thread_.join();

    auto(iter, translation_units_.begin());
    for (; iter != translation_units_.end(); ++iter)
    {
        std::cout << "disposing of translation unit: " << iter->first << std::endl;
        clang_disposeTranslationUnit(iter->second);
    }

    std::cout << "destroying a Clang index" << std::endl;
    clang_disposeIndex(index_);
}

void ClangCompleter::workerThreadLoop()
{
#ifdef _WIN32
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_LOWEST);
#else
#endif

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
            translation_units_mutex_.lock();
            createOrUpdate(job.AbsolutePath, job.Contents);
            translation_units_mutex_.unlock();

            // try and see if there is something queued up if so, then
            // immediately process it, don't go to sleep
            goto try_get_next_job;
        }
        else
        {
            job_queue_mutex_.unlock();

            // chance to do something after queue is empty but before queue is
            // going to sleep

            continue;
        }
    }
}

void ClangCompleter::CreateOrUpdate(
    const std::string& absolute_path, StringPtr contents)
{
    ParseJob job(absolute_path, contents);    
    job_queue_mutex_.lock();
    job_queue_.push_back(job);
    job_queue_mutex_.unlock();
}

void ClangCompleter::createOrUpdate(
    const std::string& absolute_path, StringPtr contents)
{
    if (!boost::ends_with(absolute_path, ".cpp"))
        return;

    const StringVectorPtr options_list = getProjectOptions(absolute_path);
    int num_options = options_list->size();
    if (num_options > 64)
    {
        std::cout << "too many command line options!" << std::endl;
        return;
    }
    const char* options[64];
    for (int i = 0; i < num_options; ++i)
    {
        options[i] = (*options_list)[i].c_str();
    }

    CXUnsavedFile files[1];
    files[0].Filename = absolute_path.c_str();
    files[0].Contents = &((*contents)[0]);
    files[0].Length = contents->size();

    auto (iter, translation_units_.find(absolute_path));
    if (iter == translation_units_.end())
    {
        std::cout << "attempting to create a translation unit for: "
                  << absolute_path << "..." << std::endl;

        unsigned flags = clang_defaultEditingTranslationUnitOptions();

        CXTranslationUnit tu = clang_parseTranslationUnit(
            index_,
            absolute_path.c_str(),
            options, num_options,
            files, 1,
            flags);

        if (tu)
        {
            translation_units_[absolute_path] = tu;
            std::cout << "created a translation unit for: "
                      << absolute_path << std::endl;
        }
        else
        {
            std::cout << "could not create a translation unit for: "
                      << absolute_path << std::endl;
        }

        // according to Rip-Rip/clang_complete:
        // Reparse to initialize the PCH cache even for auto completion This
        // should be done by index.parse(), however it is not.  So we need to
        // reparse ourselves.
        
        // this is the only flag
        std::cout << "attempting inital reparse..." << std::endl;

        flags = clang_defaultReparseOptions(tu);
        int ret = clang_reparseTranslationUnit(
            tu,
            1, files,
            flags);
        if (!ret)
        {
            std::cout << "initial reparse successful" << std::endl;
        }
        else
        {
            std::cout << "initial reparse failed" << std::endl;
        }
    }
    else
    {
        return;

        CXTranslationUnit tu = iter->second;
        unsigned flags = clang_defaultReparseOptions(tu);

        int ret = clang_reparseTranslationUnit(
            iter->second,
            1, files,
            flags);
        if (!ret)
        {
            std::cout << "reparsed file: " << absolute_path << std::endl;
        }
        else
        {
            std::cout << "deleting translation unit because failed to reparse file: "
                      << absolute_path << std::endl;

            clang_disposeTranslationUnit(iter->second);
            translation_units_.erase(absolute_path);
        }
    }
}

const StringVectorPtr ClangCompleter::getProjectOptions(std::string absolute_path)
{
    if (Contains(options_cache_, absolute_path))
        return options_cache_[absolute_path];

    using namespace boost::filesystem;
    StringVectorPtr results = boost::make_shared<StringVector>();

    path p(absolute_path);
    if (!exists(p))
    {
        std::cout << "original given path doesn't exist: "
                  << absolute_path << std::endl;
        return results;
    }

    std::string options_file;

    path dir = p.branch_path();
    while (dir != dir.branch_path())
    {
        path candidate = dir;
        candidate += "/.clang_complete";
        if (exists(candidate))
        {
            options_file =  candidate.generic_string();
            break;
        }

        dir = dir.branch_path();
    }

    if (options_file.empty())
        return results;

    std::ifstream in(options_file);

    while (in.good())
    {
        std::string line;
        std::getline(in, line);

        results->push_back(line);
    }

    options_cache_[absolute_path] = results;

    return results;
}

bool ClangCompleter::DoCompletion(
    const std::string& absolute_path,
    const std::string& current_line,
    const FileLocation& location,
    StringPtr contents,
    std::string& result)
{
    if (!AtCompletionPoint(current_line, location))
        return false;

    std::cout << "trying clang completion" << std::endl;

retry:
    job_queue_mutex_.lock();
    if (job_queue_.size() > 0)
    {
        job_queue_mutex_.unlock();
#ifdef _WIN32
        // this is in milliseconds
        ::Sleep(1);
#else
        ::usleep(1 * 1000);
#endif
        goto retry;
    }

    using namespace boost::gregorian;
    using namespace boost::posix_time;
    ptime begin = microsec_clock::universal_time();

    always_assert(job_queue_.size() == 0);

    boost::unique_lock<boost::mutex> lock(translation_units_mutex_);

    auto(iter, translation_units_.find(absolute_path));
    always_assert(iter != translation_units_.end());
    CXTranslationUnit tu = iter->second;

    job_queue_mutex_.unlock();

    CXUnsavedFile files[1];
    files[0].Filename = absolute_path.c_str();
    files[0].Contents = &((*contents)[0]);
    files[0].Length = contents->size();

    CXCodeCompleteResults* clang_results = clang_codeCompleteAt(
        tu,
        absolute_path.c_str(),
        location.Line, location.Column + 1,
        files, 1,
        clang_defaultCodeCompleteOptions());

    if (!clang_results)
    {
        return false;
    }

    std::vector<std::string> completions;
    for (unsigned i = 0; i < clang_results->NumResults; ++i)
    {
        CXCompletionResult completion = clang_results->Results[i];
        CXCursorKind cursor = completion.CursorKind;
        CXCompletionString completion_string = completion.CompletionString;

        unsigned num_chunks = clang_getNumCompletionChunks(completion_string);

        for (unsigned j = 0; j < num_chunks; ++j)
        {
            CXCompletionChunkKind kind = clang_getCompletionChunkKind(
                completion_string, j);

            if (kind != CXCompletionChunk_TypedText)
                continue;

            CXString string = clang_getCompletionChunkText(
                completion_string,
                j);
            const char* text = clang_getCString(string);

            std::string strText(text);
            completions.push_back(strText);
            
            clang_disposeString(string);
        }
    }

    clang_disposeCodeCompleteResults(clang_results);

    ptime end = microsec_clock::universal_time();
    time_duration delta = end - begin;
    std::cout << "clang: "
              << delta.total_seconds() << "." << delta.fractional_seconds()
              << std::endl;

    return true;
}

bool ClangCompleter::AtCompletionPoint(
    const std::string& line,
    const FileLocation& location)
{
    int i = location.Column - 1;
    if (i <= 0)
        return false;

    if (line[i] == '.')
        return true;

    if (i >= 2 && line[i - 1] == '-' && line [i] == '>')
        return true;

    if (i >= 2 && line[i - 1] == ':' && line [i] == ':')
        return true;

    return false;
}
