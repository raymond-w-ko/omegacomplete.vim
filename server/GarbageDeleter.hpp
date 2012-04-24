#pragma once

class TrieNode;

typedef boost::unordered_set<std::string> StringSet;
class GarbageDeleter
{
public:
	GarbageDeleter();
	~GarbageDeleter();

	void QueueForDeletion(TrieNode* pointer);
	void QueueForDeletion(StringSet* pointer);

private:
	void deletionLoop();

	std::thread thread_;
	std::atomic<int> is_quitting_;

	std::mutex mutex_;
	std::vector<TrieNode*> trie_node_pointers_;
	std::vector<StringSet*> words_pointers_;
};
