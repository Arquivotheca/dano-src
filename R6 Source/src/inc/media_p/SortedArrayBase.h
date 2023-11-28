#ifndef __SORTED_ARRAY_BASE__
#define __SORTED_ARRAY_BASE__

// Copyright 1998, Be Incorporated
//

#if __MWERKS__
#include <mcompile.h>
#else
#include <map>
#endif

#include <algobase.h>

#if __MWERKS__
#include <functional.h>
#endif

#include <new>
#include "locked_allocator.h"
#include <Debug.h>

#include <assert.h>

// would make _DoubleLinkedElement a local class of _ListElementVector and that
// a local class of SortedArray but gcc barfs on that

#define IGNORE_ELEMENT_DELETION

template <class Element>
class _DoubleLinkedElement {
public:
	int prev;
	int next;
	Element e;
};

template <class Element, int Size>
class _ListElementVector {

	// pool of double linked elements, empty elements are
	// in a single-linked list starting at nextFree
private:
	typedef _DoubleLinkedElement<Element> _Element;

public:
	_ListElementVector()
		:	nextFree(-1)
		{
			// link up all free elements
			int index = 0;
			for (; index < Size - 1; index++)
				array[index].next = index + 1;

			array[index].next = -1;
			nextFree = 0;
		}

	int insert(const Element &e, int prev, int next)
		{
			int index = nextFree;
			if (index >= 0) {
				_Element &elem = array[index];
				nextFree = elem.next;
				elem.e = e;
				elem.prev = prev;
				elem.next = next;
			}
			return index;
		}

	void remove(int index)
		{
#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to have a placement delete here
			// if non-trivial destructor is needed
			// array[index].e.destroy();
#endif
			array[index].next = nextFree;
			nextFree = index;
		}

	const _Element &operator[](int index) const
		{ return array[index]; }

	_Element &operator[](int index)
		{ return array[index]; }

	void clear()
		{
#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to have a placement delete here
			// if non-trivial destructor is needed
			// array[index].e.destroy();
#endif
			int index = 0;
			for (; index < Size - 1; index++)
				array[index].next = index + 1;

			array[index].next = -1;
			nextFree = 0;
		}

private:
	int nextFree;
	_Element array[Size];
};


template <size_t Size>
class _Buckets {
public:
	int _b[Size];
};

template <class Key, class Element, class KeyOfValue,
	class SizeTraits, class ArrayTraits, class Compare>
class SortedArrayBase {

private:
	typedef _DoubleLinkedElement<Element> _Element;
	typedef _ListElementVector<Element, SizeTraits::size> _ElementVector;

	class iterator;
	friend class iterator;
	class const_iterator;
	friend class const_iterator;

public:
	Compare key_compare;
	typedef Key key_type;
	typedef Element value_type;
	typedef Element &reference;
	typedef const Element &const_reference;
	typedef size_t size_type;

	class iterator {

		friend class SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>;
        friend class const_iterator;

	protected:
		iterator(SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare> *array,
			int linkIndex, int bucket)
			:	array(array),
				linkIndex(linkIndex),
				bucket(bucket)
			{}

	public:
		iterator() {}
		reference operator*() const
			{ return array->_ItemVec()[linkIndex].e; }

		bool operator==(const iterator &y) const
			{ return linkIndex == y.linkIndex; }
		bool operator!=(const iterator &y) const
			{ return linkIndex != y.linkIndex; }
		bool operator==(const const_iterator &y) const
			{ return linkIndex == y.linkIndex; }
		bool operator!=(const const_iterator &y) const
			{ return linkIndex != y.linkIndex; }

		iterator &operator++()
			{
				if (!ArrayTraits::can_increment_decrement) 
					TRESPASS();

				if (array->_count && linkIndex >= 0) {
					linkIndex = array->_ItemVec()[linkIndex].next;
					while (linkIndex < 0 && bucket < (int)SizeTraits::numBuckets - 1)
						// end of bucket, go to next nonempty bucket
						linkIndex = array->_BucketAt(++bucket);
				}						
				return *this;
			}
			
		iterator &operator--()
			{
				if (!ArrayTraits::can_increment_decrement) 
					TRESPASS();

				if (array->_count) {
					if (linkIndex < 0) {
						if (bucket == 0) 
							bucket = SizeTraits::numBuckets;
					} else
						linkIndex = array->_ItemVec()[linkIndex].prev;

					while (linkIndex < 0 && bucket > 0) {
						// end of bucket, go to previous
						linkIndex = array->_BucketAt(--bucket);
						
						// skip to the end of the list in this bucket
						int newLinkIndex = linkIndex;
						while (newLinkIndex >= 0) {
							newLinkIndex = array->_ItemVec()[linkIndex].next;
							if (newLinkIndex >= 0)
								linkIndex = newLinkIndex;
						}
					}
				}						
				return *this;
			}

		iterator operator++(int)
			{
				iterator result = *this;
				++*this;
				return result;
			}
			
		iterator operator--(int)
			{
				iterator result = *this;
				--*this;
				return result;
			}

		void dump()
			{ printf("it: %lx, %d, %d\n", array, linkIndex, bucket); }

	private:
		SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare> *array;
		int linkIndex;
		int bucket;
	};

	class const_iterator {

		friend class SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>;
		friend class iterator;

	protected:
		const_iterator(const SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare> *array,
			int linkIndex, int bucket)
			:	array(array),
				linkIndex(linkIndex),
				bucket(bucket)
			{}

	public:
		const_iterator() {}
		const_iterator(const iterator &it)
			{
				array = it.array;
				linkIndex = it.linkIndex;
				bucket = it.bucket;
			}

		const_reference operator*() const
			{ return array->_ItemVec()[linkIndex].e; }

		bool operator==(const iterator &y) const
			{ return linkIndex == y.linkIndex; }
		bool operator!=(const iterator &y) const
			{ return linkIndex != y.linkIndex; }
		bool operator==(const const_iterator &y) const
			{ return linkIndex == y.linkIndex; }
		bool operator!=(const const_iterator &y) const
			{ return linkIndex != y.linkIndex; }

		const_iterator &operator++()
			{
				if (!ArrayTraits::can_increment_decrement) 
					TRESPASS();

				if (array->_count && linkIndex >= 0) {
					linkIndex = array->_ItemVec()[linkIndex].next;
					while (linkIndex < 0 && bucket < (int)SizeTraits::numBuckets - 1)
						// end of bucket, go to next nonempty bucket
						linkIndex = array->_BucketAt(++bucket);
				}						
				return *this;
			}
			
		const_iterator &operator--()
			{
				if (!ArrayTraits::can_increment_decrement) 
					TRESPASS();

				if (array->_count) {
					if (linkIndex < 0) {
						if (bucket == 0) 
							bucket = SizeTraits::numBuckets;
					} else
						linkIndex = array->_ItemVec()[linkIndex].prev;

					while (linkIndex < 0 && bucket > 0) {
						// end of bucket, go to previous
						linkIndex = array->_BucketAt(--bucket);
						
						// skip to the end of the list in this bucket
						int newLinkIndex = linkIndex;
						while (newLinkIndex >= 0) {
							newLinkIndex = array->_ItemVec()[linkIndex].next;
							if (newLinkIndex >= 0)
								linkIndex = newLinkIndex;
						}
					}
				}						
				return *this;
			}

		const_iterator operator++(int)
			{
				const_iterator result = *this;
				++*this;
				return result;
			}
			
		const_iterator operator--(int)
			{
				const_iterator result = *this;
				--*this;
				return result;
			}

		void dump()
			{ printf("cit: %lx, %d, %d\n", array, linkIndex, bucket); }
	private:
		const SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare> *array;
		int linkIndex;
		int bucket;
	};

private:

	static const size_t _bucketsSize = sizeof(_Buckets<SizeTraits::numBuckets>);
	static const size_t _itemsSize = sizeof(_ElementVector);

	typedef locked_allocator<_bucketsSize> bucketsAlloc;
	typedef locked_allocator<_itemsSize> itemsAlloc;
	
public:

	SortedArrayBase()
		:	_count(0)
		{
			void *rawBuckets = bucketsAlloc::allocate(_bucketsSize);
			if (!rawBuckets)
				throw std::bad_alloc();
			_buckets = new(rawBuckets) _Buckets<SizeTraits::numBuckets>;
			
			
			void *rawItems = itemsAlloc::allocate(_itemsSize);
			if (!rawItems)
				throw std::bad_alloc();
			_items = new(rawItems) _ElementVector;

			for (size_t i = 0; i < SizeTraits::numBuckets; i++)
				_BucketAt(i) = -1;
		}
	
	SortedArrayBase(const SortedArrayBase &cloneThis)
		{
			void *rawBuckets = bucketsAlloc::allocate(_bucketsSize);
			if (!rawBuckets)
				throw std::bad_alloc();
			_buckets = new(rawBuckets)
				_Buckets<SizeTraits::numBuckets>(*cloneThis._buckets);

			void *rawItems = itemsAlloc::allocate(_itemsSize);
			if (!rawItems)
				throw std::bad_alloc();
			_items = new(rawItems)
				_ElementVector(*cloneThis._items);
		}

	~SortedArrayBase()
		{
#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to call placement delete here on _items
#endif
			bucketsAlloc::deallocate(_buckets, _bucketsSize);
			itemsAlloc::deallocate(_items, _itemsSize);
		}
		
	SortedArrayBase &operator=(const SortedArrayBase &cloneThis)
		{
			if (&cloneThis != this) {
				// check self assingment

#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to call placement delete here on _subarray
#endif
				bucketsAlloc::deallocate(_buckets, _bucketsSize);
				itemsAlloc::deallocate(_items, _itemsSize);

				void *rawBuckets = bucketsAlloc::allocate(_bucketsSize);
				if (!rawBuckets)
					throw std::bad_alloc();
				_buckets = new(rawBuckets)
					_Buckets<SizeTraits::numBuckets>(*cloneThis._buckets);

				void *rawItems = itemsAlloc::allocate(_itemsSize);
				if (!rawItems)
					throw std::bad_alloc();
				_items = new(rawItems)
					_ElementVector(*cloneThis._items);
				
			}
			return *this;
		}
		

	iterator insert(const Element &);
	std::pair<iterator, bool> insert_unique(const Element &);
	size_type erase(const key_type &);
	void erase(iterator);

	iterator find(const key_type &);
	const_iterator find(const key_type &) const;

	iterator lower_bound(const key_type &);
	const_iterator lower_bound(const key_type &) const;
	iterator upper_bound(const key_type &);
	const_iterator upper_bound(const key_type &) const;

	iterator begin()
		{
			if (!ArrayTraits::has_begin) 
				TRESPASS();

			size_t bucket = 0;
			int index = -1;
			for (; bucket < SizeTraits::numBuckets; bucket++) {
				index = _BucketAt(bucket);
				if (index >= 0)
					return iterator(this, index, bucket);
			}
			// empty array, return end??
			return iterator(this, -1, 0);
		}
	iterator end()
		{ return iterator(this, -1, 0); }
	const_iterator begin() const
		{
			if (!ArrayTraits::has_begin) 
				TRESPASS();

			size_t bucket = 0;
			int index = -1;
			for (; bucket < SizeTraits::numBuckets; bucket++) {
				index = _BucketAt(bucket);
				if (index >= 0)
					return const_iterator(this, index, bucket);
			}
			// empty array, return end??
			return const_iterator(this, -1, 0);
		}
	const_iterator end() const
		{ return const_iterator(this, -1, 0); }

	size_type size() const
		{ return _count; }
	size_type max_size() const
		{ return SizeTraits::size; }
	bool empty() const
		{ return _count == 0; }
	
	void clear();

	void dump()
		{ TRESPASS(); }

protected:

	_ElementVector &_ItemVec()
		{ return *_items; }

	const _ElementVector &_ItemVec() const
		{ return *_items; }
	
	int &_BucketAt(int _index)
		{ return (_buckets->_b)[_index]; }

	int _BucketAt(int _index) const
		{ return (_buckets->_b)[_index]; }

	_Buckets<SizeTraits::numBuckets> *_buckets;
	_ElementVector *_items;
	size_t _count;
};

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::insert(const Element &e)
{
	assert(_buckets != 0);
	assert(_items != 0);

	if (size() == SizeTraits::size) 
		// no more room
		throw std::bad_alloc();

	int bucket = ArrayTraits::_bucket(KeyOfValue()(e));
	if (_BucketAt(bucket) == -1) {
		int index = _ItemVec().insert(e, -1, -1);
		_BucketAt(bucket) = index;
		++_count;
		return iterator(this, index, bucket);
	}
	for (int index = _BucketAt(bucket); index >= 0;) {
	
		_Element &elem = _ItemVec()[index];
		if (key_compare(KeyOfValue()(e), KeyOfValue()(elem.e))) {
			// hit larger element, insert before
			int newIndex = _ItemVec().insert(e, elem.prev, index);
			if (elem.prev >= 0)
				// relink old previous item to new element
				_ItemVec()[elem.prev].next = newIndex;
			else
				// no previous item, point bucket new element
				_BucketAt(bucket) = newIndex;
				
			elem.prev = newIndex;
			++_count;
			return iterator(this, newIndex, bucket);
		} else if (elem.next < 0) {
			// end of list for this bucket, add after last element
			int newIndex = _ItemVec().insert(e, index, -1);
			elem.next = newIndex;
			++_count;
			return iterator(this, newIndex, bucket);
		}		
		index = elem.next;
	}
	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
std::pair<SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::iterator , bool>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::insert_unique(const Element &e)
{
	assert(_buckets != 0);
	assert(_items != 0);

	if (size() == SizeTraits::size) 
		// no more room
		throw std::bad_alloc();

	int bucket = ArrayTraits::_bucket(KeyOfValue()(e));
	if (_BucketAt(bucket) == -1) {
		// empty bucket, add first item
		int index = _ItemVec().insert(e, -1, -1);
		_BucketAt(bucket) = index;
		++_count;
		return std::pair<iterator, bool>(iterator(this, index, bucket), true);
	}
	for (int index = _BucketAt(bucket); index >= 0;) {
		_Element &elem = _ItemVec()[index];
		if (KeyOfValue()(e) == KeyOfValue()(elem.e)) {
			// already got it, bail
			return std::pair<iterator, bool>(iterator(this, index, bucket), false);
		} else if (key_compare(KeyOfValue()(e), KeyOfValue()(elem.e))) {
			// hit larger element, insert before
			int newIndex = _ItemVec().insert(e, elem.prev, index);
			if (elem.prev >= 0)
				// relink old previous item to new element
				_ItemVec()[elem.prev].next = newIndex;
			else
				// no previous item, point bucket new element
				_BucketAt(bucket) = newIndex;
				
			elem.prev = newIndex;
			++_count;
			return std::pair<iterator, bool>(iterator(this, newIndex, bucket), true);
		} else if (elem.next < 0) {
			// end of list for this bucket, add after last element
			int newIndex = _ItemVec().insert(e, index, -1);
			elem.next = newIndex;
			++_count;
			return std::pair<iterator, bool>(iterator(this, newIndex, bucket), true);
		}		
		index = elem.next;
	}
	return std::pair<iterator, bool>(end(), false);
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::size_type
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::erase(const key_type &e)
{
	assert(_buckets != 0);
	assert(_items != 0);

	if (!size())
		return 0;

	int bucket = ArrayTraits::_bucket(e);
	int index = _BucketAt(bucket);
	
	for ( ;index >= 0; ) {
		_Element &elem = _ItemVec()[index];
		if (KeyOfValue()(elem.e) == e) {
			// found it, unlink and remove
			
			if (elem.prev < 0)
				_BucketAt(bucket) = elem.next;
			else
				_ItemVec()[elem.prev].next = elem.next;
			
			if (elem.next >= 0)
				_ItemVec()[elem.next].prev = elem.prev;
			
			_ItemVec().remove(index);
			--_count;
			return 1;
		} else if (key_compare(e, KeyOfValue()(elem.e)))
			break;

		index = elem.next;
	}
	return 0;
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
void
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::erase(iterator i)
{
	int index = i.linkIndex;
	_Element &elem = _ItemVec()[index];

	if (elem.prev < 0)
		_BucketAt(i.bucket) = elem.next;
	else
		_ItemVec()[elem.prev].next = elem.next;
	
	if (elem.next >= 0)
		_ItemVec()[elem.next].prev = elem.prev;
	
	_ItemVec().remove(index);
	--_count;
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::find(const key_type &e)
{
	if (size()) {
		int bucket = ArrayTraits::_bucket(e);
		for (int index = _BucketAt(bucket); index >= 0; ) {
			const _Element &elem = _ItemVec()[index];
			if (KeyOfValue()(_ItemVec()[index].e) == e) 
				return iterator(this, index, bucket);
			index = elem.next;
		}
	}

	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::const_iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::find(const key_type &e) const
{
	if (size()) {
		int bucket = ArrayTraits::_bucket(e);
		for (int index = _BucketAt(bucket); index >= 0; ) {
			const _Element &elem = _ItemVec()[index];
			if (KeyOfValue()(_ItemVec()[index].e) == e) 
				return const_iterator(this, index, bucket);
			index = elem.next;
		}
	}

	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::lower_bound(const key_type &e)
{
	if (!ArrayTraits::has_lower_upper_bound) 
		TRESPASS();

	if (size()) 
		for(size_t bucket = ArrayTraits::_bucket(e); bucket < SizeTraits::numBuckets; bucket++) 
			for (int index = _BucketAt(bucket); index >= 0; ) {
				const _Element &elem = _ItemVec()[index];
				if (!key_compare(KeyOfValue()(_ItemVec()[index].e), e))
					return iterator(this, index, bucket);
				index = elem.next;
			}

	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::const_iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::lower_bound(const key_type &e) const
{
	if (!ArrayTraits::has_lower_upper_bound) 
		TRESPASS();

	if (size()) 
		for(size_t bucket = ArrayTraits::_bucket(e); bucket < SizeTraits::numBuckets; bucket++) 
			for (int index = _BucketAt(bucket); index >= 0; ) {
				const _Element &elem = _ItemVec()[index];
				if (!key_compare(KeyOfValue()(_ItemVec()[index].e), e))
					return const_iterator(this, index, bucket);
				index = elem.next;
			}

	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::upper_bound(const key_type &e)
{
	if (!ArrayTraits::has_lower_upper_bound) 
		TRESPASS();

	if (size()) 
		for(size_t bucket = ArrayTraits::_bucket(e); bucket < SizeTraits::numBuckets; bucket++) 
			for (int index = _BucketAt(bucket); index >= 0; ) {
				const _Element &elem = _ItemVec()[index];
				if (key_compare(e, KeyOfValue()(_ItemVec()[index].e)))
					return iterator(this, index, bucket);
				index = elem.next;
			}

	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::const_iterator
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::upper_bound(const key_type &e) const
{
	if (!ArrayTraits::has_lower_upper_bound) 
		TRESPASS();

	if (size()) 
		for(size_t bucket = ArrayTraits::_bucket(e); bucket < SizeTraits::numBuckets; bucket++) 
			for (int index = _BucketAt(bucket); index >= 0; ) {
				const _Element &elem = _ItemVec()[index];
				if (key_compare(e, KeyOfValue()(_ItemVec()[index].e)))
					return const_iterator(this, index, bucket);
				index = elem.next;
			}
	return end();
}

template <class Key, class Element, class KeyOfValue, class SizeTraits, class ArrayTraits, class Compare>
void
SortedArrayBase<Key, Element, KeyOfValue, SizeTraits, ArrayTraits, Compare>::clear()
{
	_count = 0;
	for (size_t i = 0; i < SizeTraits::numBuckets; i++)
		_BucketAt(i) = -1;
	_ItemVec().clear();
}

#endif

