/***************************************************************************
//
//	File:			support/AssociativeVector.h
//
//	Description:	A templatized key/value mapping.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT_ASSOCIATIVEVECTOR_H
#define _SUPPORT_ASSOCIATIVEVECTOR_H

#include <support/Vector.h>
#include <support/OrderedVector.h>

namespace B {
namespace Support {

/*--------------------------------------------------------*/
/*----- BAssociativeVector class -------------------------*/

template<class KEY, class VALUE>
class BAssociativeVector
{
public:
						BAssociativeVector(const VALUE& undefined = VALUE());
						BAssociativeVector(const BAssociativeVector<KEY,VALUE>& o);
virtual					~BAssociativeVector();

		BAssociativeVector<KEY,VALUE>&	operator=(const BAssociativeVector<KEY,VALUE>& o);

		ssize_t			InsertValue(const KEY& key, const VALUE& value);
		const VALUE&	LookupValue(const KEY& key, bool* found = NULL) const;
		VALUE&			EditValue(const KEY& key, bool* found = NULL);
		
		size_t			CountItems() const;

		const KEY&		KeyAt(size_t i) const;
		const VALUE&	ValueAt(size_t i) const;
		VALUE&			EditValueAt(size_t i);
		
		ssize_t			FindKey(const KEY& key) const;
		bool			GetIndexOf(const KEY& key, size_t* index) const;
		
		void			MakeEmpty();
		void			RemoveItems(size_t index, size_t count);
		void			RemoveItem(size_t index);

		ssize_t			RemoveKey(const KEY& key);
		
		void			Reserve(size_t total_space);
		void			ReserveExtra(size_t extra_space);

		void			Swap(BAssociativeVector& o);
		
private:
		BOrderedVector<KEY>	m_keys;
		BVector<VALUE>		m_values;
		VALUE				m_undefined;
};

// Type optimizations.
template<class KEY, class VALUE>
void BSwap(BAssociativeVector<KEY, VALUE>& v1, BAssociativeVector<KEY, VALUE>& v2);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

template<class KEY, class VALUE> inline
BAssociativeVector<KEY,VALUE>::BAssociativeVector(const VALUE& undefined)
	:	m_undefined(undefined)
{
}

template<class KEY, class VALUE> inline
BAssociativeVector<KEY,VALUE>::BAssociativeVector(const BAssociativeVector<KEY,VALUE>& o)
	:	m_keys(o.m_keys), m_values(o.m_values), m_undefined(o.m_undefined)
{
}

template<class KEY, class VALUE> inline
BAssociativeVector<KEY,VALUE>::~BAssociativeVector()
{
}

template<class KEY, class VALUE> inline
BAssociativeVector<KEY,VALUE>& BAssociativeVector<KEY,VALUE>::operator=(const BAssociativeVector<KEY,VALUE>& o)
{
	m_keys = o.m_keys; m_values = o.m_values; m_undefined = o.m_undefined;
	return *this;
}

template<class KEY, class VALUE> inline
ssize_t BAssociativeVector<KEY,VALUE>::InsertValue(const KEY& key, const VALUE& value)
{
	bool added;
	const ssize_t pos = m_keys.InsertValue(key, &added);
	if (added) {
		const ssize_t vpos = m_values.InsertItem(value, pos);
		if (vpos < B_OK) m_keys.RemoveValue(key);
		return vpos;
	} else {
		m_values.ReplaceItem(pos, value);
		return pos;
	}
}

template<class KEY, class VALUE> inline
const VALUE& BAssociativeVector<KEY,VALUE>::LookupValue(const KEY& key, bool* found) const
{
	ssize_t pos = m_keys.FindValue(key);
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
VALUE& BAssociativeVector<KEY,VALUE>::EditValue(const KEY& key, bool* found)
{
	ssize_t pos = m_keys.FindValue(key);
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
size_t BAssociativeVector<KEY,VALUE>::CountItems() const
{
	return m_keys.CountItems();
}

template<class KEY, class VALUE> inline
const KEY& BAssociativeVector<KEY,VALUE>::KeyAt(size_t i) const
{
	return m_keys.ItemAt(i);
}

template<class KEY, class VALUE> inline
const VALUE& BAssociativeVector<KEY,VALUE>::ValueAt(size_t i) const
{
	return m_values.ItemAt(i);
}

template<class KEY, class VALUE> inline
VALUE& BAssociativeVector<KEY,VALUE>::EditValueAt(size_t i)
{
	return m_values.EditItemAt(i);
}

template<class KEY, class VALUE> inline
ssize_t BAssociativeVector<KEY,VALUE>::FindKey(const KEY& key) const
{
	return m_keys.FindValue(key);
}

template<class KEY, class VALUE> inline
bool BAssociativeVector<KEY,VALUE>::GetIndexOf(const KEY& key, size_t* index) const
{
	return m_keys.GetIndexOf(key, index);
}

template<class KEY, class VALUE> inline
void BAssociativeVector<KEY,VALUE>::MakeEmpty()
{
	m_keys.MakeEmpty(); m_values.MakeEmpty();
}

template<class KEY, class VALUE> inline
void BAssociativeVector<KEY,VALUE>::RemoveItems(size_t index, size_t count)
{
	m_keys.RemoveItems(index, count); m_values.RemoveItems(index, count);
}

template<class KEY, class VALUE> inline
void BAssociativeVector<KEY,VALUE>::RemoveItem(size_t index)
{
	m_keys.RemoveItem(index); m_values.RemoveItem(index);
}

template<class KEY, class VALUE> inline
ssize_t BAssociativeVector<KEY,VALUE>::RemoveKey(const KEY& key)
{
	const ssize_t pos = m_keys.RemoveValue(key);
	if (pos >= B_OK) m_values.RemoveItem(pos);
	return pos;
}

template<class KEY, class VALUE> inline
void BAssociativeVector<KEY,VALUE>::Reserve(size_t total_space)
{
	m_keys.Reserve(total_space); m_values.Reserve(total_space);
}

template<class KEY, class VALUE> inline
void BAssociativeVector<KEY,VALUE>::ReserveExtra(size_t extra_space)
{
	m_keys.ReserveExtra(total_space); m_values.ReserveExtra(total_space);
}

template<class KEY, class VALUE> inline
void BAssociativeVector<KEY, VALUE>::Swap(BAssociativeVector<KEY, VALUE>& o)
{
	BSwap(m_keys, o.m_keys);
	BSwap(m_values, o.m_values);
	BSwap(m_undefined, o.m_undefined);
}

template<class KEY, class VALUE> inline
void BSwap(BAssociativeVector<KEY, VALUE>& v1, BAssociativeVector<KEY, VALUE>& v2)
{
	v1.Swap(v2);
}

} }	// namespace B::Support

#endif
