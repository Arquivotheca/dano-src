#ifndef __SORTED_ARRAY__
#define __SORTED_ARRAY__

// Copyright 1998, Be Incorporated
//
// bucket array with linked lists of elements
// near O log(N) insertion, erasing and finding
// Non-growable, indended for a fixed 
//
// SortedArray performs sub-optimally if values get clustered into
// a few isolated buckets. Should work reasonably well for uniformly
// distributed keys such as object pointers.
//
// iterator::operator--() could be enhanced by keeping a tail pointer in
// each bucket in addition to the existing head pointer. Currently the decrement
// operator has to traverse the entire lined list when traversing into a lower
// bucket. This is probably not a problem though and offsets the overhead of having
// to maintain the tail pointer for insertions and deletions

#include "SortedArrayBase.h"

template <class Key, class SizeTraits>
class SortedArrayTraits {

public:

	static int _bucket(const Key &key)
		{
			int result = (((int)key - (int)SizeTraits::lowValue) * SizeTraits::numBuckets)
				/ ((int)SizeTraits::highValue - (int)SizeTraits::lowValue);
			
			if (result < 0)
				result = 0;
			if (result >= (int)SizeTraits::numBuckets)
				result = (int)SizeTraits::numBuckets - 1;

			return result;
		}


	static const bool can_increment_decrement = true;
	static const bool has_begin = true;
	static const bool has_lower_upper_bound = true;
};

template <class Key, class Element, class KeyOfValue,
	class SizeTraits, class Compare = std::less<Key> >
class SortedArray : public SortedArrayBase<Key, Element, KeyOfValue,
	SizeTraits, SortedArrayTraits<Key, SizeTraits>, Compare > {

public:

	SortedArray()
		:	SortedArrayBase<Key, Element, KeyOfValue,
				SizeTraits, SortedArrayTraits<Key, SizeTraits>, Compare>()
		{
		}
	
	SortedArray(const SortedArray &cloneThis)
		:	SortedArrayBase<Key, Element, KeyOfValue,
				SizeTraits, SortedArrayTraits<Key, SizeTraits>, Compare>(cloneThis)
		{
		}
	
	void dump()
		{
			// not yet implemented
			TRESPASS();
		}

};

template <int Size, int NumBuckets, unsigned long Low, unsigned long High>
struct _ulong_SortedArray_traits {
	static const size_t size = Size;
	static const size_t numBuckets = NumBuckets;
	static const unsigned long lowValue = Low; 
	static const unsigned long highValue = High; 
};

struct _sorted_array_64 : public _ulong_SortedArray_traits<64, 16, 0, 128*1024L> {
};
struct _sorted_array_1024 : public _ulong_SortedArray_traits<1024, 64, 0, 128*1024L> {
};
struct _sorted_array_4096 : public _ulong_SortedArray_traits<4096, 128, 0, 128*1024L> {
};


#endif

