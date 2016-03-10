#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"

class DiskMultiMap
{
public:

	class Iterator
	{
	public:
		Iterator();
		Iterator(DiskMultiMap* dmm_ptr, BinaryFile::Offset off);
		// You may add additional constructors
		/*
		The Iterator class must have a public destructor, copy constructor and assignment 
		operator, either declared and implemented by you or left unmentioned so that the 
		compiler will generate them for you.
		*/
		bool isValid() const;
		Iterator& operator++();
		MultiMapTuple operator*();
	private:
		BinaryFile::Offset m_it;
		DiskMultiMap* m_dmm;
	};

	DiskMultiMap();
	~DiskMultiMap();
	bool createNew(const std::string& filename, unsigned int numBuckets);
	bool openExisting(const std::string& filename);
	void close();
	bool insert(const std::string& key, const std::string& value, const std::string& context);
	Iterator search(const std::string& key);
	int erase(const std::string& key, const std::string& value, const std::string& context);

private:
	BinaryFile::Offset hash(const std::string& key) const;
	struct Data {
		char m_key[121];
		char m_value[121];
		char m_context[121];
		BinaryFile::Offset next;
	};
	struct HashTableInfo {
		unsigned int m_numBuckets;
		BinaryFile::Offset newNodeLoc;
		BinaryFile::Offset deletedNodeLoc;
	};
	BinaryFile m_bf;
	HashTableInfo m_hashin;
};

#endif // DISKMULTIMAP_H_#pragma once
