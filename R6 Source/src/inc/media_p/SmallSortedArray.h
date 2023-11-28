
#ifndef __SIMPLE_SORTED_ARRAY__
#define __SIMPLE_SORTED_ARRAY__

// Copyright 1998, Be Incorporated
//
// simple sorted array with linear lookups, used for small data sets
// elements retain their location throughtout their lifetime by using an indirection
// on overflow array does not grow
// linear searches are used for element lookups, insertions and removals
// should be used for sets/maps/multimaps with a small number of elements (~32)

// O(N) insertion, erasing and finding

#include <new>
#include <locked_allocator.h>

#define IGNORE_ELEMENT_DELETION


template <class Element, int Size>
class _SmallVector {

	// _SmallVector used to actually store the elements
	// keeps track of empty slots by using a header with an empty bit

private:
	struct _Element {
		Element e;
		bool empty;
	};

public:
	_SmallVector()
		{
			for (int i = 0; i < Size; i++)
				array[i].empty = true;
		}

#if 0
	_SmallVector(const _SmallVector &cloneThis)
		{
			memcpy(this, cloneThis, sizeof(*this));
		}
#endif
		
	int insert(const Element &e)
		{
			for (int i = 0; i < Size; i++)
				if (array[i].empty) {
					array[i].e = e;
					array[i].empty = false;
					return i;
				}
			return -1;
		}

	void remove(int index)
		{
#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to have a placement delete here
			// if non-trivial destructor is needed
			// array[index].e.destroy();
#endif
			array[index].empty = true;
		}

	const Element &operator[](int index) const
		{
			ASSERT(!array[index].empty);
			return array[index].e;
		}

	Element &operator[](int index)
		{
			ASSERT(!array[index].empty);
			return array[index].e;
		}

	void clear()
		{
#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to have a placement delete here
			// if non-trivial destructor is needed
			// array[index].e.destroy();
#endif
			for (int i = 0; i < Size; i++)
				array[i].empty = true;
		}

private:
	_Element array[Size];
};

template <int Size>
class _Links {
public:
	int _l[Size];
};

template <class Key, class Element, class KeyOfValue, int Size, class Compare = less<Key> >
class SmallSortedArray {
	
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

		friend class SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>;
        friend class const_iterator;

	protected:
		iterator(SmallSortedArray<Key, Element, KeyOfValue, Size, Compare> *array,
			int linkIndex)
			:	array(array),
				linkIndex(linkIndex)
			{}

	public:
		iterator() {}
		reference operator*() const
			{
				int index = array->_LinkAt(linkIndex);
				reference result = array->_SubArray()[index];
				return result;
			}

		bool operator==(const iterator &y) const
			{ return array->_LinkAt(linkIndex) == y.array->_LinkAt(y.linkIndex); }
		bool operator!=(const iterator &y) const
			{ return array->_LinkAt(linkIndex) != y.array->_LinkAt(y.linkIndex); }
		bool operator==(const const_iterator &y) const
			{ return array->_LinkAt(linkIndex) == y.array->_LinkAt(y.linkIndex); }
		bool operator!=(const const_iterator &y) const
			{ return array->_LinkAt(linkIndex) != y.array->_LinkAt(y.linkIndex); }

		iterator &operator++()
			{
				if (linkIndex < array->_count)
					linkIndex++;
				return *this;
			}
			
		iterator &operator--()
			{
				if (linkIndex <= 0)
					linkIndex = array->_count;
				else
					linkIndex--;

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

	private:
		SmallSortedArray<Key, Element, KeyOfValue, Size, Compare> *array;
		int linkIndex;
	};

	class const_iterator {

		friend class SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>;
		friend class iterator;

	protected:
		const_iterator(const SmallSortedArray<Key, Element, KeyOfValue, Size,
			Compare> *array, int linkIndex)
			:	array(array),
				linkIndex(linkIndex)
			{}

	public:
		const_iterator() {}
		const_iterator(const iterator &it)
			{
				array = it.array;
				linkIndex = it.linkIndex;
			}

		const_reference operator*() const
			{ return array->_SubArray()[array->_LinkAt(linkIndex)]; }

		bool operator==(const iterator &y) const
			{ return array->_LinkAt(linkIndex) == y.array->_LinkAt(y.linkIndex); }
		bool operator!=(const iterator &y) const
			{ return array->_LinkAt(linkIndex) != y.array->_LinkAt(y.linkIndex); }
		bool operator==(const const_iterator &y) const
			{ return array->_LinkAt(linkIndex) == y.array->_LinkAt(y.linkIndex); }
		bool operator!=(const const_iterator &y) const
			{ return array->_LinkAt(linkIndex) != y.array->_LinkAt(y.linkIndex); }

		const_iterator &operator++()
			{
				if ((size_t)linkIndex < array->_count)
					linkIndex++;
				return *this;
			}
			
		const_iterator &operator--()
			{	
				if (linkIndex <= 0)
					linkIndex = (int)array->_count;
				else
					linkIndex--;

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

	private:
		const SmallSortedArray<Key, Element, KeyOfValue, Size, Compare> *array;
		int linkIndex;
	};

private:

	static const size_t _linksSize = sizeof(_Links<Size + 1>);
	static const size_t _subarraySize = sizeof(_SmallVector<Element, Size>);

	typedef locked_allocator<_linksSize> linkAlloc;
	typedef locked_allocator<_subarraySize> subarrayAlloc;
	
public:
	SmallSortedArray()
		:	_count(0)
		{
			void *linksRaw = linkAlloc::allocate(_linksSize);
			if (!linksRaw)
				throw bad_allow();
			_links = new (linksRaw)_Links<Size + 1>;
			
			void *subarrayRaw = subarrayAlloc::allocate(_subarraySize);
			if (!subarrayRaw)
				throw bad_allow();
			_subarray = new (subarrayRaw) _SmallVector<Element, Size>;
			
			for (int i = 0; i < Size + 1; i++)
				// last link used as a run-off for end
				_LinkAt(i) = -1;
		}

	SmallSortedArray(const SmallSortedArray &cloneThis)
		:	_count(0)
		{
			void *linksRaw = linkAlloc::allocate(_linksSize);
			if (!linksRaw)
				throw bad_allow();
			_links = new (linksRaw)
				_Links<Size + 1>(*cloneThis._links);
			void *subarrayRaw = subarrayAlloc::allocate(_subarraySize);
			if (!subarrayRaw)
				throw bad_allow();
			_subarray = new (subarrayRaw)
				_SmallVector<Element, Size>(*cloneThis._subarray);
		}

	~SmallSortedArray()
		{

#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to call placement delete here on _subarray
#endif
			linkAlloc::deallocate(_links, _linksSize);
			subarrayAlloc::deallocate(_subarray, _subarraySize);
		}
	
	SmallSortedArray &operator=(const SmallSortedArray &cloneThis)
		{
			if (&cloneThis != this) {
				// check self assingment

#ifndef IGNORE_ELEMENT_DELETION
 #error notImplemented
			// need to call placement delete here on _subarray
#endif
				linkAlloc::deallocate(_links, _linksSize);
				subarrayAlloc::deallocate(_subarray, _subarraySize);

				void *linksRaw = linkAlloc::allocate(_linksSize);
				if (!linksRaw)
					throw bad_allow();
				_links = new (linksRaw)
					_Links<Size + 1>(*cloneThis._links);

				void *subarrayRaw = subarrayAlloc::allocate(_subarraySize);
				if (!subarrayRaw)
					throw bad_allow();
				_subarray = new (subarrayRaw)
					_SmallVector<Element, Size>(*cloneThis._subarray);
				
			}
			return *this;
		}
		
	iterator insert(const Element &);
	pair<iterator, bool> insert_unique(const Element &);
	size_type erase(const key_type &);
	void erase(iterator);
		
	iterator find(const key_type &);
	const_iterator find(const key_type &) const;

	iterator lower_bound(const key_type &);
	const_iterator lower_bound(const key_type &) const;
	iterator upper_bound(const key_type &);
	const_iterator upper_bound(const key_type &) const;

	iterator begin()
		{ return iterator(this, 0); }
	iterator end()
		{ return iterator(this, _count); }
	const_iterator begin() const
		{ return const_iterator(this, 0); }
	const_iterator end() const
		{ return const_iterator(this, _count); }

	size_type size() const
		{ return _count; }
	size_type max_size() const
		{ return Size; }
	bool empty() const
		{ return _count == 0; }
	
	void clear()
		{	
			_count = 0;
			for (int i = 0; i < Size + 1; i++)
				// last link used as a run-off for end
				_LinkAt(i) = -1;
			_SubArray().clear();
		}
	
private:
	
	_SmallVector<Element, Size> &_SubArray()
		{ return *_subarray; }

	const _SmallVector<Element, Size> &_SubArray() const
		{ return *_subarray; }


	int &_LinkAt(int index)
		{ return (_links->_l)[index]; }

	int _LinkAt(int index) const
		{ return (_links->_l)[index]; }

	size_t _count;
	_Links<Size + 1> *_links;	// one more for simple implementation of end()
	_SmallVector<Element, Size> *_subarray;
};

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::insert(const Element &e)
{
	if (size() == Size) 
		// no more room
		throw bad_alloc();

	for (size_t i = 0; i < Size; i++) {
		if (_LinkAt(i) != -1) {
			// if key already in container, insert new element last
			if (!key_compare(KeyOfValue()(_SubArray()[_LinkAt(i)]), KeyOfValue()(e))
				|| KeyOfValue()(_SubArray()[_LinkAt(i)]) == KeyOfValue()(e)) {
				continue;
			}

				// spread out the _links array for a new link
			for (int j = (int)_count - 1; j >= (int)i; j--) {
				_LinkAt(j + 1) = _LinkAt(j);
			}
		}
		
		// insert element into _SubArray()
		_LinkAt(i) = _SubArray().insert(e);

		_count++;
		return iterator(this, i);
	}

	TRESPASS();
	return end();
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
pair<SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::iterator, bool>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::insert_unique(const Element &e)
{
	if (size() == Size) 
		// no more room
		throw bad_alloc();

	for (size_t i = 0; i < Size; i++) {
		if (_LinkAt(i) != -1) {
			if (key_compare(KeyOfValue()(_SubArray()[_LinkAt(i)]), KeyOfValue()(e))) 
				continue;

			if (KeyOfValue()(_SubArray()[_LinkAt(i)]) == KeyOfValue()(e)) 
				return pair<iterator, bool>(iterator(this, i), false);

				// spread out the _links array for a new link
			for (int j = _count - 1; j >= (int)i; j--) 
				_LinkAt(j + 1) = _LinkAt(j);
		}
		
		// insert element into _SubArray()
		_LinkAt(i) = _SubArray().insert(e);

		_count++;
		return pair<iterator, bool>(iterator(this, i), true);
	}
	
	TRESPASS();
	return pair<iterator, bool>(end(), false);
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::size_type
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::erase(const key_type &e)
{
	if (!size())
		return 0;
	
	for (size_t i = 0; i < Size; i++) {
		if (KeyOfValue()(_SubArray()[_LinkAt(i)]) == e) {
			// remove element from subarray
			_SubArray().remove(_LinkAt(i));
			for (size_t j = i; j < _count; j++) 
				_LinkAt(j) = _LinkAt(j + 1);

			_count--;
			return 1;
		} else if (key_compare(e, KeyOfValue()(_SubArray()[_LinkAt(i)])))
			// gone past, not found
			break;
	}
	return 0;
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
void
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::erase(iterator i)
{
	ASSERT(i != end());
	// remove element from subarray
	_SubArray().remove(_LinkAt(i.linkIndex));
	for (size_t j = i.linkIndex; j < _count; j++) 
		_LinkAt(j) = _LinkAt(j + 1);

	_count--;
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::find(const key_type &e)
{
	if (size())
		for (size_t i = 0; i < _count; i++) 
			if (KeyOfValue()(_SubArray()[_LinkAt(i)]) == e) 
				return iterator(this, i);

	return end();
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::const_iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::find(const key_type &e) const
{
	if (size())
		for (size_t i = 0; i < (int)_count; i++) 
			if (KeyOfValue()(_SubArray()[_LinkAt(i)]) == e) 
				return const_iterator(this, i);

	return end();
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::lower_bound(const key_type &e)
{
	if (size())
		for (size_t i = 0; i < _count; i++) 
			if (!key_compare(KeyOfValue()(_SubArray()[_LinkAt(i)]), e))
				return iterator(this, i);

	return end();
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::const_iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::lower_bound(const key_type &e) const
{
	if (size())
		for (size_t i = 0; i < _count; i++) 
			if (!key_compare(KeyOfValue()(_SubArray()[_LinkAt(i)]), e))
				return const_iterator(this, i);

	return end();
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::upper_bound(const key_type &e)
{
	if (size())
		for (size_t i = 0; i < _count; i++) 
			if (key_compare(e, KeyOfValue()(_SubArray()[_LinkAt(i)])))
				return iterator(this, i);

	return end();
}

template <class Key, class Element, class KeyOfValue, int Size, class Compare>
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::const_iterator
SmallSortedArray<Key, Element, KeyOfValue, Size, Compare>::upper_bound(const key_type &e) const
{
	if (size())
		for (size_t i = 0; i < _count; i++) 
			if (key_compare(e, KeyOfValue()(_SubArray()[_LinkAt(i)])))
				return const_iterator(this, i);

	return end();
}

#endif
