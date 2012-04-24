#pragma once

class TrieNode;

typedef boost::unordered_set<std::string> StringSet;
typedef boost::unordered_multimap<std::string, std::string> StringStringMultiMap;
class GarbageDeleter
{
public:
	GarbageDeleter();
	~GarbageDeleter();

	void QueueForDeletion(TrieNode* pointer);
	void QueueForDeletion(StringSet* pointer);
	void QueueForDeletion(StringStringMultiMap* pointer);

private:
	void deletionLoop();

	std::thread thread_;
	std::atomic<int> is_quitting_;

	std::mutex mutex_;
	std::vector<TrieNode*> trie_node_pointers_;
	std::vector<StringSet*> words_pointers_;
	std::vector<StringStringMultiMap*> multimap_pointers_;
};
