#pragma once

class ClangCompleter
{
public:
    ClangCompleter();
    ~ClangCompleter();

    void Init();

    void CreateOrUpdate(std::string absolute_path, StringPtr contents);

private:
    struct ParseJob
    {
        ParseJob(std::string absolute_path, const StringPtr& contents)
        :
        AbsolutePath(absolute_path),
        Contents(contents)
        { }

        ParseJob(const ParseJob& other)
        :
        AbsolutePath(other.AbsolutePath),
        Contents(other.Contents)
        { }

        std::string AbsolutePath;
        StringPtr Contents;
    };

    void createOrUpdate(std::string absolute_path, StringPtr contents);
    const StringVectorPtr getProjectOptions(std::string absolute_path);
    void workerThreadLoop();

    boost::thread worker_thread_;
    volatile int is_quitting_;

    boost::mutex job_queue_mutex_;
    std::deque<ParseJob> job_queue_;


    CXIndex index_;
    boost::mutex translation_units_mutex_;
    std::map<std::string, CXTranslationUnit> translation_units_;
    std::map<std::string, StringVectorPtr> options_cache_;
};
