// ===========================================================================
//	HashTable.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1997 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __HASHTAB__
#define __HASHTAB__

#include "Utils.h"

// ===========================================================================

class HashEntry : public CLinkable {
public:

virtual	const char*	Key(int& length);

};

// ===========================================================================

class HashTable /*: public NPObject*/ {
public:		
					enum {
						kTableSize = 997		// nice prime number
					} EHashTable;
		
					HashTable();
virtual				~HashTable();

static	unsigned long		HashKey(const char* key, int length);

		void		Add(HashEntry* entry);
		bool		Delete(HashEntry* entry);
		void		Remove(HashEntry* entry);
		HashEntry*	Lookup(const char* key, int length);
	
virtual	void		BuildIndex();				// Build an index of the contents
		HashEntry*	Get(int i);					// Get an indexed entry

protected:
		CLinkedList	mTable[kTableSize];
		int			mCount;
		bool		mValidIndex;
		HashEntry**	mIndex;

};


#endif
