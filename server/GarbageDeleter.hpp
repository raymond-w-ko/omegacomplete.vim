#pragma once

struct TrieNode;

typedef boost::unordered_set<std::string> StringSet;
typedef boost::unordered_map<std::string, unsigned> StringUnsignedMap;
typedef boost::unordered_multimap<std::string, std::string> StringStringMultiMap;
typedef boost::unordered_multimap<std::string, const std::string*>
        StringConstStringPointerMultiMap;
class GarbageDeleter
{
public:
    GarbageDeleter();
    ~GarbageDeleter();

    void QueueForDeletion(TrieNode* pointer);
    void QueueForDeletion(StringSet* pointer);
    void QueueForDeletion(StringUnsignedMap* pointer);
    void QueueForDeletion(StringStringMultiMap* pointer);
    void QueueForDeletion(StringConstStringPointerMultiMap* pointer);

private:
    void deletionLoop();

    boost::thread thread_;

    // check your platform to make sure volatile does what the VC++ compiler
    // does when you declare a volatile variable. C++03 and/or Boost does
    // not have an atomic library to do this portably yet.
    // http://msdn.microsoft.com/en-us/library/12a04hfd%28v=vs.90%29.aspx
    //std::atomic<int> is_quitting_;
    volatile int is_quitting_;

    boost::mutex mutex_;
    std::vector<TrieNode*> trie_node_vector_pointers_;
    std::vector<StringSet*> string_set_pointers_;
    std::vector<StringUnsignedMap*> string_unsigned_map_pointers_;
    std::vector<StringStringMultiMap*> multimap_pointers_;
    std::vector<StringConstStringPointerMultiMap*> ssp_multimap_pointers_;
};
