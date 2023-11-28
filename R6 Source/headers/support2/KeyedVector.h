/***************************************************************************
//
//	File:			support2/KeyedVector.h
//
//	Description:	A templatized key/value mapping.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_ASSOCIATIVEVECTOR_H
#define _SUPPORT2_ASSOCIATIVEVECTOR_H

#include <support2/Vector.h>
#include <support2/OrderedVector.h>

namespace B {
namespace Support2 {

/*--------------------------------------------------------*/
/*----- BKeyedVector class -------------------------------*/

template<class KEY, class VALUE>
class BKeyedVector
{
public:
							BKeyedVector(const VALUE& undefined = VALUE());
							BKeyedVector(const BKeyedVector<KEY,VALUE>& o);
							BKeyedVector(	const BOrderedVector<KEY>& keys,
											const BVector<VALUE>& values,
											const VALUE& undefined = VALUE());
	virtual					~BKeyedVector();
	
			BKeyedVector<KEY,VALUE>&	operator=(const BKeyedVector<KEY,VALUE>& o);
	
	/* Size stats */
	
			void			SetCapacity(size_t total_space);
			void			SetExtraCapacity(size_t extra_space);
			size_t			Capacity() const;
			
			size_t			CountItems() const;
		
	/* Value by Key */

			const VALUE&	ValueFor(const KEY& key, bool* found = NULL) const;
			VALUE&			EditValueFor(const KEY& key, bool* found = NULL);
			
	/* Value/Key by index */

			const KEY&		KeyAt(size_t i) const;
			const VALUE&	ValueAt(size_t i) const;
			VALUE&			EditValueAt(size_t i);
			
			const BOrderedVector<KEY>&	KeyVector() const;
			const BVector<VALUE>&		ValueVector() const;
			BVector<VALUE>&				ValueVector();
			
	/* List manipulation */

			ssize_t			IndexOf(const KEY& key) const;
			bool			GetIndexOf(const KEY& key, size_t* index) const;
			
			ssize_t			AddItem(const KEY& key, const VALUE& value);
			
			void			RemoveItemsAt(size_t index, size_t count = 1);
			ssize_t			RemoveItemFor(const KEY& key);
			
			void			MakeEmpty();
			
			void			Swap(BKeyedVector& o);
			
private:
			BOrderedVector<KEY>	m_keys;
			BVector<VALUE>		m_values;
			VALUE				m_undefined;
			size_t				_reserved_data1;
			size_t				_reserved_data2;
};

// Type optimizations.
template<class KEY, class VALUE>
void BSwap(BKeyedVector<KEY, VALUE>& v1, BKeyedVector<KEY, VALUE>& v2);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

template<class KEY, class VALUE> inline
BKeyedVector<KEY,VALUE>::BKeyedVector(const VALUE& undefined)
	:	m_undefined(undefined),
		_reserved_data1(0), _reserved_data2(0)
{
}

template<class KEY, class VALUE> inline
BKeyedVector<KEY,VALUE>::BKeyedVector(const BKeyedVector<KEY,VALUE>& o)
	:	m_keys(o.m_keys), m_values(o.m_values), m_undefined(o.m_undefined),
		_reserved_data1(0), _reserved_data2(0)
{
}

template<class KEY, class VALUE> inline
BKeyedVector<KEY,VALUE>::BKeyedVector(const BOrderedVector<KEY>& keys,
			const BVector<VALUE>& values, const VALUE& undefined)
	:	m_keys(keys), m_values(values), m_undefined(undefined),
		_reserved_data1(0), _reserved_data2(0)
{
}

template<class KEY, class VALUE> inline
BKeyedVector<KEY,VALUE>::~BKeyedVector()
{
}

template<class KEY, class VALUE> inline
BKeyedVector<KEY,VALUE>& BKeyedVector<KEY,VALUE>::operator=(const BKeyedVector<KEY,VALUE>& o)
{
	m_keys = o.m_keys; m_values = o.m_values; m_undefined = o.m_undefined;
	return *this;
}

template<class KEY, class VALUE> inline
void BKeyedVector<KEY,VALUE>::SetCapacity(size_t total_space)
{
	m_keys.SetCapacity(total_space); m_values.SetCapacity(total_space);
}

template<class KEY, class VALUE> inline
void BKeyedVector<KEY,VALUE>::SetExtraCapacity(size_t extra_space)
{
	m_keys.SetExtraCapacity(total_space); m_values.SetExtraCapacity(total_space);
}

template<class KEY, class VALUE> inline
size_t BKeyedVector<KEY,VALUE>::Capacity() const
{
	return m_keys.Capacity();
}

template<class KEY, class VALUE> inline
size_t BKeyedVector<KEY,VALUE>::CountItems() const
{
	return m_keys.CountItems();
}

template<class KEY, class VALUE> inline
const VALUE& BKeyedVector<KEY,VALUE>::ValueFor(const KEY& key, bool* found) const
{
	ssize_t pos = m_keys.IndexOf(key);
	if (pos >= static_cast<ssize_t>(m_values.CountItems())) pos = B_ERROR;
	if (pos >= B_OK) {
		if (found) *found = true;
		return m_values.ItemAt(pos);
	} else {
		if (found) *found = false;
		return m_undefined;
	}
}

template<class KEY, class VALUE> inline
VALUE& BKeyedVector<KEY,VALUE>::EditValueFor(const KEY& key, bool* found)
{
	ssize_t pos = m_keys.IndexOf(key);
	if (pos >= static_cast<ssize_t>(m_values.CountItems())) pos = B_ERROR;
	if (pos >= B_OK) {
		if (found) *found = true;
		return m_values.EditItemAt(pos);
	} else {
		if (found) *found = false;
		return m_undefined;
	}
}

template<class KEY, class VALUE> inline
const KEY& BKeyedVector<KEY,VALUE>::KeyAt(size_t i) const
{
	return m_keys.ItemAt(i);
}

template<class KEY, class VALUE> inline
const VALUE& BKeyedVector<KEY,VALUE>::ValueAt(size_t i) const
{
	return m_values.ItemAt(i);
}

template<class KEY, class VALUE> inline
VALUE& BKeyedVector<KEY,VALUE>::EditValueAt(size_t i)
{
	return m_values.EditItemAt(i);
}

template<class KEY, class VALUE> inline
const BOrderedVector<KEY>& BKeyedVector<KEY,VALUE>::KeyVector() const
{
	return m_keys;
}

template<class KEY, class VALUE> inline
const BVector<VALUE>& BKeyedVector<KEY,VALUE>::ValueVector() const
{
	return m_values;
}

template<class KEY, class VALUE> inline
BVector<VALUE>& BKeyedVector<KEY,VALUE>::ValueVector()
{
	return m_values;
}

template<class KEY, class VALUE> inline
ssize_t BKeyedVector<KEY,VALUE>::IndexOf(const KEY& key) const
{
	return m_keys.IndexOf(key);
}

template<class KEY, class VALUE> inline
bool BKeyedVector<KEY,VALUE>::GetIndexOf(const KEY& key, size_t* index) const
{
	return m_keys.GetIndexOf(key, index);
}

template<class KEY, class VALUE> inline
ssize_t BKeyedVector<KEY,VALUE>::AddItem(const KEY& key, const VALUE& value)
{
	bool added;
	const ssize_t pos = m_keys.AddItem(key, &added);
	if (added) {
		const ssize_t vpos = m_values.AddItemAt(value, pos);
		if (vpos < B_OK) m_keys.RemoveItemsAt(pos);
		return vpos;
	} else {
		m_values.ReplaceItemAt(value, pos);
		return pos;
	}
}

template<class KEY, class VALUE> inline
void BKeyedVector<KEY,VALUE>::RemoveItemsAt(size_t index, size_t count)
{
	m_keys.RemoveItemsAt(index, count); m_values.RemoveItemsAt(index, count);
}

template<class KEY, class VALUE> inline
ssize_t BKeyedVector<KEY,VALUE>::RemoveItemFor(const KEY& key)
{
	const ssize_t pos = m_keys.RemoveItemFor(key);
	if (pos >= B_OK) m_values.RemoveItemsAt(pos);
	return pos;
}

template<class KEY, class VALUE> inline
void BKeyedVector<KEY,VALUE>::MakeEmpty()
{
	m_keys.MakeEmpty(); m_values.MakeEmpty();
}

template<class KEY, class VALUE> inline
void BKeyedVector<KEY, VALUE>::Swap(BKeyedVector<KEY, VALUE>& o)
{
	BSwap(m_keys, o.m_keys);
	BSwap(m_values, o.m_values);
	BSwap(m_undefined, o.m_undefined);
}

template<class KEY, class VALUE> inline
void BSwap(BKeyedVector<KEY, VALUE>& v1, BKeyedVector<KEY, VALUE>& v2)
{
	v1.Swap(v2);
}

} }	// namespace B::Support2

#endif
