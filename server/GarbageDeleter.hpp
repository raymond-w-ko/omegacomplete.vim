#pragma once

class TrieNode;

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

	std::thread thread_;
	std::atomic<int> is_quitting_;

	std::mutex mutex_;
	std::vector<TrieNode*> trie_node_vector_pointers_;
	std::vector<StringSet*> string_set_pointers_;
	std::vector<StringUnsignedMap*> string_unsigned_map_pointers_;
	std::vector<StringStringMultiMap*> multimap_pointers_;
	std::vector<StringConstStringPointerMultiMap*> ssp_multimap_pointers_;
};
