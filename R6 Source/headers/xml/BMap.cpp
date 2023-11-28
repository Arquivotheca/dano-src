
#include <xml/BStringMap.h>
#include <ctype.h>


namespace B {
namespace XML {

// =====================================================================
template <class K, class T>
BMap<K,T>::BMap()
	:_names(3),
	 _values(3)
{
	
}


// =====================================================================
template <class K, class T>
BMap<K,T>::BMap(const BMap<K,T> & copy)
	:_names(copy._names.CountItems()),
	 _values(copy._values.CountItems())
{
	K * name;
	T * value;
	int32 count = copy._names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		name = new K(*(K *) copy._names.ItemAt(i));
		value = new T(*(T *) copy._names.ItemAt(i));
		_names.AddItem(name);
		_values.AddItem(value);
	}
}


// =====================================================================
template <class K, class T>
BMap<K,T> &
BMap<K,T>::operator=(const BMap<K,T> & copy)
{
	if (&copy == this)
		return *this;
	
	MakeEmpty();
	K * name;
	T * value;
	int32 count = copy._names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		name = new K(*(K *) copy._names.ItemAt(i));
		value = new T(*(T *) copy._names.ItemAt(i));
		_names.AddItem(name);
		_values.AddItem(value);
	}
	return *this;
}


// =====================================================================
template <class K, class T>
BMap<K,T>::~BMap()
{
	MakeEmpty();
}


// =====================================================================
template <class K, class T>
void
BMap<K,T>::Add(const K & key, const T & val)
{
	int32 index = FindIndex(key);
	if (index == -1)
	{
		// Add it fresh
		K * name = new K(key);
		T * value = new T(val);
		_names.AddItem(name);
		_values.AddItem(value);
	}
	else
	{
		// Replace
		T * v = (T *) _values.ItemAt(index);
		*v = val;
	}
}


// =====================================================================
template <class K, class T>
int32
BMap<K,T>::FindIndex(const K & key) const
{
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
		if (*((K *) _names.ItemAt(i)) == key)
			return i;
	return -1;
}


// =====================================================================
template <class K, class T>
status_t
BMap<K,T>::PairAt(int32 index, K & key, T & val) const
{
	if (index < 0 || index >= _names.CountItems())
		return B_BAD_INDEX;
	K * name = (K *) _names.ItemAt(index);
	T * value = (T *) _values.ItemAt(index);
	key = *name;
	val = *value;
	return B_OK;
}


// =====================================================================
template <class K, class T>
status_t
BMap<K,T>::PairAt(int32 index, K ** key, T ** val)
{
	if (index < 0 || index >= _names.CountItems())
		return B_BAD_INDEX;
	*key = (const K *) _names.ItemAt(index);
	*val = (const T *) _values.ItemAt(index);
	return B_OK;
}


// =====================================================================
template <class K, class T>
status_t
BMap<K,T>::PairAt(int32 index, const K ** key, const T ** val) const
{
	if (index < 0 || index >= _names.CountItems())
		return B_BAD_INDEX;
	*key = (const K *) _names.ItemAt(index);
	*val = (const T *) _values.ItemAt(index);
	return B_OK;
}


// =====================================================================
template <class K, class T>
status_t
BMap<K,T>::Find(const K & key, T & val) const
{
	K * str;
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		str = (K *) _names.ItemAt(i);
		if (*(str) == key)
		{
			val = *(T *) _values.ItemAt(i);
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}


// =====================================================================
template <class K, class T>
T *
BMap<K,T>::Find(const K & key) const
{
	K * str;
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		str = (K *) _names.ItemAt(i);
		if (*(str) == key)
		{
			return (T *) _values.ItemAt(i);
		}
	}
	return NULL;
}


// =====================================================================
template <class K, class T>
void
BMap<K,T>::MakeEmpty()
{
	int32 count = _names.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		delete (K *) _names.ItemAt(i);
		delete (T *) _values.ItemAt(i);
	}
	_names.MakeEmpty();
	_values.MakeEmpty();
}


// =====================================================================
template <class K, class T>
int32
BMap<K,T>::CountItems() const
{
	return _names.CountItems();
}


}; // namespace XML
}; // namespace B

