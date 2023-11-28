#ifndef __HASHED_ARRAY__
#define __HASHED_ARRAY__

// Copyright 1998, Be Incorporated
//
// Bucket hash table with sorted linked lists for collision resolution

#include "SortedArrayBase.h"

template <class Key, class SizeTraits>
class HashedArrayTraits {

public:

	static int _bucket(const Key &key)
		{
			// hash function optimized for pointers, should
			// specialize for different key types and pass in
			// as a trait
			
			int result = (int)key >> 4;
//			int result = (int)key;
				// strip of pointer insignificant junk
			result = result % SizeTraits::numBuckets;

			return result;
		}

	static const bool can_increment_decrement = false;
	static const bool has_begin = false;
	static const bool has_lower_upper_bound = false;
};


template <class Key, class Element, class KeyOfValue,
	class SizeTraits, class Compare = less<Key> >
class HashedArray : public SortedArrayBase<Key, Element, KeyOfValue,
	SizeTraits, HashedArrayTraits<Key, SizeTraits>, Compare> {

public:

	HashedArray()
		:	SortedArrayBase<Key, Element, KeyOfValue,
				SizeTraits,
				HashedArrayTraits<Key, SizeTraits>, Compare>()
		{
		}
	
	HashedArray(const HashedArray &cloneThis)
		:	SortedArrayBase<Key, Element, KeyOfValue,
				SizeTraits,
				HashedArrayTraits<Key, SizeTraits>, Compare>(cloneThis)
		{
		}
	
	void dump() // hacked up, assumes keys are ints
		{
			printf("hashTable: size %ld", _count);
			for (size_t bucket = 0; bucket < SizeTraits::numBuckets; bucket++) {
				printf("\n[%ld] ->", bucket);
				for (int index = _BucketAt(bucket); index >= 0;) {
					_Element &elem = _ItemVec()[index];
					printf("%d ->", (int)KeyOfValue()(elem.e));
					index = elem.next;
				}
			}
			printf("\n--------------\n");
		}

};


template <int Size, int NumBuckets>
struct _HashedArray_traits {
	static const size_t size = Size;
	static const size_t numBuckets = NumBuckets;
};

// handy primes close to 2^n for optimal bucket array memory utilization
//const uint32 kPrimes [] = {
//	31, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071, 262139,
//	524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393, 67108859,
//	134217689, 268435399, 536870909, 1073741789, 2147483647, 0
//};

struct _hash_table_31_64 : public _HashedArray_traits<64, 31> {
};

struct _hash_table_127_1024 : public _HashedArray_traits<1024, 127> {
};

#endif
