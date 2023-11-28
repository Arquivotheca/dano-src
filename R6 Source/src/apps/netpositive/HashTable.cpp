// ===========================================================================
//	HashTable.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "HashTable.h"

#include <SupportDefs.h>
#include <malloc.h>
#include <stdio.h>
#include <Debug.h>
// ===========================================================================

const char* HashEntry::Key(int& length)
{
	length = 0;
	return NULL;
}

// ===========================================================================

HashTable::HashTable() : mCount(0),mValidIndex(false),mIndex(NULL)
{
}

HashTable::~HashTable()
{
	if (mIndex)
		free(mIndex);
}

//	Turn a string into hash

ulong HashTable::HashKey(const char* key, int length)
{
	ulong hash = 0;
// This hash algorithm sucks because only the first few characters of the
// string are significant (all others get shifted off the end of the hash).
//	while (length > 0)
//		hash = (hash << 1) + key[--length];

// This has is tuned to create good 10-bit hash keys, which matches our hash
// table size of 997 entries.  For each character in the string, left-rotate
// our hash value through 10 bits and XOR the current character.
	while (length > 0)
		hash = (((hash & 0x0200) >> 9) | ((hash & 0x01FF) << 1)) ^ (key[--length] & 0x00FF);
	return hash;
}

//	Lookup an entry in the hash table

HashEntry* HashTable::Lookup(const char* key, int length)
{
//	NP_ASSERT(length);
//	NP_ASSERT(key);
	ulong hash = HashKey(key,length) % kTableSize;
	
	HashEntry* h = (HashEntry*)mTable[hash].First();
	for (;h;h = (HashEntry*)h->Next()) {
		int l;
		const char* k = h->Key(l);
		if (l == length) {
			if (memcmp(k,key,length) == 0)
				break;
		}
	}
	return h;
}

//	Add a entry to the hash table

void HashTable::Add(HashEntry* entry)
{
//printf("HashTable::Add 0x%x\n", entry);
	const char* key;
	int length;
	
	key = entry->Key(length);
	ulong hash = HashKey(key,length) % kTableSize;
	
	HashEntry* h = (HashEntry*)mTable[hash].First();
	for (;h;h = (HashEntry*)h->Next())
		if (h == entry)
			return;
	
	mTable[hash].Add(entry);
	mValidIndex = false;
	mCount++;
}

//	Add a entry to the hash table

bool HashTable::Delete(HashEntry* entry)
{
	const char* key;
	int length;
	
	key = entry->Key(length);
	ulong hash = HashKey(key,length) % kTableSize;
	
	HashEntry* h = (HashEntry*)mTable[hash].First();
	for (;h;h = (HashEntry*)h->Next()) {
		if (h == entry) {
			mTable[hash].Delete(entry);
			mValidIndex = false;
			mCount--;
			return true;
		}
	}
	return false;
//	NP_ASSERT(false);
}

void HashTable::Remove(HashEntry* entry)
{
	const char* key;
	int length;
	
	key = entry->Key(length);
	ulong hash = HashKey(key,length) % kTableSize;
	
	HashEntry* h = (HashEntry*)mTable[hash].First();
	for (;h;h = (HashEntry*)h->Next()) {
		if (h == entry) {
			mTable[hash].Remove(entry);
			mValidIndex = false;
			mCount--;
			break;
		}
	}
//	NP_ASSERT(false);
}

// Build a flat index of all the entries in the table

void HashTable::BuildIndex()					
{
//printf("HashTable::BuildIndex.  Count is %ld\n", mCount);
	mIndex = (HashEntry**)malloc(mCount*4);
//	NP_ASSERT(mIndex);
	if (mIndex == NULL)
		return;
		
	int j = 0;
	for (int i = 0; i < kTableSize; i++) {
		HashEntry* h = (HashEntry*)mTable[i].First();
		for (;h;h = (HashEntry*)h->Next()) {
			mIndex[j++] = h;
		}
	}
	mValidIndex = true;
}

HashEntry* HashTable::Get(int i)
{
	if (i < 0 || i >= mCount)
		return NULL;
	if (mValidIndex == false)
		BuildIndex();
	if (mValidIndex == false)
		return NULL;
	return mIndex[i];
}



