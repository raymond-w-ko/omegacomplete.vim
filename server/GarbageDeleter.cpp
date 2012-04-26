#include "stdafx.hpp"

#include "GarbageDeleter.hpp"
#include "TrieNode.hpp"

GarbageDeleter::GarbageDeleter()
:
is_quitting_(0)
{
    thread_ = std::thread(
        &GarbageDeleter::deletionLoop,
        this);
}

GarbageDeleter::~GarbageDeleter()
{
    is_quitting_ = 1;
    thread_.join();
}

void GarbageDeleter::QueueForDeletion(TrieNode* pointer)
{
    mutex_.lock();
    trie_node_vector_pointers_.push_back(pointer);
    mutex_.unlock();
}

void GarbageDeleter::QueueForDeletion(StringSet* pointer)
{
    mutex_.lock();
    string_set_pointers_.push_back(pointer);
    mutex_.unlock();
}

void GarbageDeleter::QueueForDeletion(StringUnsignedMap* pointer)
{
    mutex_.lock();
    string_unsigned_map_pointers_.push_back(pointer);
    mutex_.unlock();
}

void GarbageDeleter::QueueForDeletion(StringStringMultiMap* pointer)
{
    mutex_.lock();
    multimap_pointers_.push_back(pointer);
    mutex_.unlock();
}

void GarbageDeleter::QueueForDeletion(StringConstStringPointerMultiMap* pointer)
{
    mutex_.lock();
    ssp_multimap_pointers_.push_back(pointer);
    mutex_.unlock();
}

void GarbageDeleter::deletionLoop()
{
    while (true)
    {
        if (is_quitting_ == 1) break;

        std::vector<TrieNode*> trie_nodes;
        mutex_.lock();
        trie_nodes = trie_node_vector_pointers_;
        trie_node_vector_pointers_.clear();
        mutex_.unlock();
        for (TrieNode* node : trie_nodes) delete node;

        std::vector<StringSet*> wordsets;
        mutex_.lock();
        wordsets = string_set_pointers_;
        string_set_pointers_.clear();
        mutex_.unlock();
        for (StringSet* wordset : wordsets) delete wordset;

        std::vector<StringUnsignedMap*> sums;
        mutex_.lock();
        sums = string_unsigned_map_pointers_;
        string_unsigned_map_pointers_.clear();
        mutex_.unlock();
        for (StringUnsignedMap* sum : sums) delete sum;

        std::vector<StringStringMultiMap*> multimaps;
        mutex_.lock();
        multimaps = multimap_pointers_;
        multimap_pointers_.clear();
        mutex_.unlock();
        for (StringStringMultiMap* multimap : multimaps) delete multimap;

        std::vector<StringConstStringPointerMultiMap*> scspmultimaps;
        mutex_.lock();
        scspmultimaps = ssp_multimap_pointers_;
        ssp_multimap_pointers_.clear();
        mutex_.unlock();
        for (StringConstStringPointerMultiMap* scspmultimap : scspmultimaps)
            delete scspmultimap;

#ifdef WIN32
        Sleep(100);
#else
        sleep(100);
#endif
    }
}
