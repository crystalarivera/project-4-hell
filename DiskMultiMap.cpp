#define _CRT_SECURE_NO_WARNINGS //VC++ thing
#include "DiskMultiMap.h"
#include <iostream>
#include <cstring>

DiskMultiMap::DiskMultiMap() {

}

DiskMultiMap::~DiskMultiMap() {
	close();
}

bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets) {
	if (m_bf.isOpen())
		m_bf.close();
	bool success = m_bf.createNew(filename); // create a new file
	if (!success)
		cout << "Error! Unable to create file\n";

	m_hashin.m_numBuckets = numBuckets;
	m_hashin.newNodeLoc = sizeof(HashTableInfo);
	m_hashin.deletedNodeLoc = 0;

	int danku = numBuckets;
	BinaryFile::Offset temp = 0;
	for (int i = 0; i < danku; i++) {
		success = m_bf.write(temp, m_hashin.newNodeLoc);//write each bucket on disk as an offset = 0 @ newNodeLoc
		m_hashin.newNodeLoc += sizeof(BinaryFile::Offset); //increment new Node loc by size of offset
	} //m_hashin.newNodeLoc holds valid new node loc

	success = m_bf.write(m_hashin, 0);	//write struct to disk

	return success;
}

bool DiskMultiMap::openExisting(const std::string& filename) {
	if (m_bf.isOpen())
		m_bf.close();
	return m_bf.openExisting(filename);
}

void DiskMultiMap::close() {
	if(m_bf.isOpen())
		m_bf.close();
}

//have an outside hash function so you can use it for search too.
//the hash returns the exact location of the offset of the bucket uwu
BinaryFile::Offset DiskMultiMap::hash(const std::string& key) const {
	std::hash<string> str_hash;
	unsigned int bucketNum = str_hash(key) % m_hashin.m_numBuckets;
	BinaryFile::Offset bucketLoc = sizeof(HashTableInfo) + (sizeof(BinaryFile::Offset) * bucketNum);
	return bucketLoc;
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context) {
	if (key.size() > 120 || value.size() > 120 || context.size() > 120)
		return false;

	//create data node
	Data toAdd;
	strcpy(toAdd.m_key, key.c_str());
	strcpy(toAdd.m_value, value.c_str());
	strcpy(toAdd.m_context, context.c_str());

	//hash it here
	BinaryFile::Offset bucket = hash(key);

	//set new data's next to the offset of bucket since this works as pushing front
	BinaryFile::Offset offsetOfFirstNode;
	m_bf.read(offsetOfFirstNode, bucket);
	toAdd.next = offsetOfFirstNode;

	//from warmup: implementation for reusing dead space
	if (m_hashin.deletedNodeLoc != 0) { //there are deleted node spaces to reuse
		offsetOfFirstNode = m_hashin.deletedNodeLoc;
		Data deletedNode;
		m_bf.read(deletedNode, offsetOfFirstNode);
		m_hashin.deletedNodeLoc = deletedNode.next;
		m_bf.write(toAdd, offsetOfFirstNode);
	}
	else { //we gotta make a new node. g o t t a 
		//write new data node to next available slot on disk	
		m_bf.write(toAdd, m_hashin.newNodeLoc);

		//bucket's next points to new node
		BinaryFile::Offset bucketNext = m_hashin.newNodeLoc;
		//write bucket at hashed offset
		m_bf.write(bucketNext, bucket);

		//update new insertion position
		m_hashin.newNodeLoc += sizeof(Data);
	}
	m_bf.write(m_hashin, 0);
	return true;
}

DiskMultiMap::Iterator::Iterator() {
	m_it = 0;
}

DiskMultiMap::Iterator::Iterator(DiskMultiMap* dmm_ptr, BinaryFile::Offset off) {
	m_it = off;
	m_dmm = dmm_ptr;
}

bool DiskMultiMap::Iterator::isValid() const {
	return (m_it != 0);
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++() {
	if (!isValid())
		return *this;

	//read the key it is pointing to and set temp to it
	Data originalData;
	Data compData;
	m_dmm->m_bf.read(originalData, m_it);
	m_it = originalData.next;

	while (isValid()) {
		//compare currnode with temp node's value
		m_dmm->m_bf.read(compData, m_it);
		if (strcmp(originalData.m_key, compData.m_key) == 0) {
			return *this; //if they match, return iterator 
		}
		m_it = compData.next;
	}
	return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*() {
	MultiMapTuple mmt;
	mmt.key = "";
	mmt.value = "";
	mmt.context = "";

	if (!isValid())
		return mmt;

	Data copyMe;
	m_dmm->m_bf.read(copyMe, m_it);
	mmt.key = copyMe.m_key;
	mmt.value = copyMe.m_value;
	mmt.context = copyMe.m_context;

	return mmt;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key) {
	BinaryFile::Offset bucket = hash(key);
	BinaryFile::Offset pre_it;
	m_bf.read(pre_it, bucket); //pre_it contains ptr to first data node
	//follow the bucket until the key matches the data's key or until you hit the end of the bucket
	Data DataNode;
	while (pre_it != 0) {
		m_bf.read(DataNode, pre_it);
		if (strcmp(DataNode.m_key, key.c_str()) == 0) {
			Iterator theONE(this, pre_it);
			return theONE;
		}
		pre_it = DataNode.next;
	}
	Iterator invalid;
	return invalid;
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context) {
	int deleted = 0;
	
	BinaryFile::Offset first = hash(key);
	BinaryFile::Offset cur;
	m_bf.read(cur, first); //now pointing at first node in list
	if (cur == 0) //empty list
		return deleted;

	BinaryFile::Offset prev = cur;
	Data curNode;
	Data toDelete;
	strcpy(toDelete.m_key, key.c_str());
	strcpy(toDelete.m_value, value.c_str());
	strcpy(toDelete.m_context, context.c_str());

	while (cur != 0) {
		m_bf.read(curNode, cur);
		if (strcmp(curNode.m_key, toDelete.m_key) == 0 && strcmp(curNode.m_value, toDelete.m_value) == 0 && strcmp(curNode.m_context, toDelete.m_context) == 0) {
			//store curNode in deleted list
			Data toBeRemoved;
			m_bf.read(toBeRemoved, cur);  
			toBeRemoved.next = m_hashin.deletedNodeLoc;
			m_hashin.deletedNodeLoc = cur;
			m_bf.write(toBeRemoved, cur);
			deleted++;

			if (cur == prev) { //delete head node
				BinaryFile::Offset offsetOfFirstNode = curNode.next;
				m_bf.write(offsetOfFirstNode, first); //rewriting bucket ptr
				cur = offsetOfFirstNode;
				prev = offsetOfFirstNode;
			}
			else { //non head node
				BinaryFile::Offset tempNext = curNode.next;
				m_bf.read(curNode, prev);
				curNode.next = tempNext;
				m_bf.write(curNode, prev);
				//prev is alredy prev
				cur = curNode.next;
			}
			//update hashtable deleted info with write
			m_bf.write(m_hashin, 0);
			deleted = true;
		}
		else {
			prev = cur;
			cur = curNode.next;
		}
	}
	return deleted;
}
