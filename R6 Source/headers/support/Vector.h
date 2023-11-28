/***************************************************************************
//
//	File:			support/Vector.h
//
//	Description:	A simple array-like container class.  Implemented as
//					a general purpose abstract base-class BUntypedVector,
//					with the concrete class BVector layered on top and
//					templatized on the array type.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT_VECTOR_H
#define _SUPPORT_VECTOR_H

#include <support/SupportDefs.h>
#include <support/TypeFuncs.h>

namespace B {
namespace Support {

/*--------------------------------------------------------*/
/*----- BUntypedVector abstract base class ---------------*/

class BUntypedVector
{
public:
						BUntypedVector(size_t element_size);
						BUntypedVector(const BUntypedVector& o);
						// WARNING: Your subclass must call MakeEmpty()
						// in its own destructor!
virtual					~BUntypedVector();

		BUntypedVector&	operator=(const BUntypedVector& o);

		size_t			ElementSize() const;
		
		size_t			CountElements() const;
		size_t			CountAvail() const;
		const void*		ElementAt(size_t index) const;
		
		ssize_t			AddElement(const void* newElement);
		ssize_t			InsertElement(const void* newElement, size_t index);
		void*			EditElementAt(size_t index);
		
		ssize_t			AddUntypedVector(const BUntypedVector& o);
		ssize_t			InsertUntypedVector(const BUntypedVector& o, size_t index);
		
		void			MakeEmpty();
		void			RemoveElements(size_t index, size_t count);
		
		void			Reserve(size_t total_space);
		void			ReserveExtra(size_t extra_space);
		
		void			Swap(BUntypedVector& o);
		
protected:
virtual	void			PerformConstruct(void* base, size_t count) const = 0;
virtual	void			PerformCopy(void* to, const void* from, size_t count) const = 0;
virtual	void			PerformDestroy(void* base, size_t count) const = 0;

virtual	void			PerformMoveBefore(void* to, void* from, size_t count) const = 0;
virtual	void			PerformMoveAfter(void* to, void* from, size_t count) const = 0;

virtual	void			PerformAssign(void* to, const void* from, size_t count) const = 0;

private:

virtual	status_t		_ReservedUntypedVector1();
virtual	status_t		_ReservedUntypedVector2();
virtual	status_t		_ReservedUntypedVector3();
virtual	status_t		_ReservedUntypedVector4();
virtual	status_t		_ReservedUntypedVector5();
virtual	status_t		_ReservedUntypedVector6();
virtual	status_t		_ReservedUntypedVector7();
virtual	status_t		_ReservedUntypedVector8();
virtual	status_t		_ReservedUntypedVector9();
virtual	status_t		_ReservedUntypedVector10();

		uint8*			grow(size_t amount, size_t factor=3, size_t pos=0xFFFFFFFF);
		uint8*			shrink(size_t amount, size_t factor=4, size_t pos=0xFFFFFFFF);
		const uint8*	data() const;
		uint8*			edit_data();
		
		const size_t	m_elementSize;
		const size_t	m_localSpace;
		size_t			m_avail;
		size_t			m_size;
		
		union {
			uint8*		heap;
			uint8		local[8];
		} m_data;
		
		int32			_reserved[2];
};

// Type optimizations.
void BMoveBefore(BUntypedVector* to, BUntypedVector* from, size_t count = 1);
void BMoveAfter(BUntypedVector* to, BUntypedVector* from, size_t count = 1);
void BSwap(BUntypedVector& v1, BUntypedVector& v2);

/*--------------------------------------------------------*/
/*----- BVector concrete class ---------------------------*/

template<class TYPE>
class BVector : private BUntypedVector
{
public:
						BVector();
						BVector(const BVector<TYPE>& o);
virtual					~BVector();

		BVector<TYPE>&	operator=(const BVector<TYPE>& o);

		size_t			CountItems() const;
		size_t			CountAvail() const;

		const TYPE&		operator[](size_t i) const;
		const TYPE&		ItemAt(size_t i) const;

		ssize_t			AddItem();
		ssize_t			AddItem(const TYPE& item);
		ssize_t			InsertItem(size_t index);
		ssize_t			InsertItem(const TYPE& item, size_t index);
		ssize_t			ReplaceItem(size_t index, const TYPE& item);
		TYPE&			EditItemAt(size_t i);

		ssize_t			AddVector(const BVector<TYPE>& o);
		ssize_t			InsertVector(const BVector<TYPE>& o, size_t index);
		
		void			MakeEmpty();
		void			RemoveItems(size_t index, size_t count);
		void			RemoveItem(size_t index);

		void			Reserve(size_t total_space);
		void			ReserveExtra(size_t extra_space);
		
		void			Swap(BVector<TYPE>& o);
		
protected:
virtual	void			PerformConstruct(void* base, size_t count) const;
virtual	void			PerformCopy(void* to, const void* from, size_t count) const;
virtual	void			PerformDestroy(void* base, size_t count) const;

virtual	void			PerformMoveBefore(void* to, void* from, size_t count) const;
virtual	void			PerformMoveAfter(void* to, void* from, size_t count) const;

virtual	void			PerformAssign(void* to, const void* from, size_t count) const;
};

// Type optimizations.
template<class TYPE>
void BMoveBefore(BVector<TYPE>* to, BVector<TYPE>* from, size_t count = 1);
template<class TYPE>
void BMoveAfter(BVector<TYPE>* to, BVector<TYPE>* from, size_t count = 1);
template<class TYPE>
void BSwap(BVector<TYPE>& v1, BVector<TYPE>& v2);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline void BSwap(BUntypedVector& v1, BUntypedVector& v2)
{
	v1.Swap(v2);
}

/*-------------------------------------------------------------*/

template<class TYPE> inline
BVector<TYPE>::BVector()
	:	BUntypedVector(sizeof(TYPE[2])/2)
{
}

template<class TYPE> inline
BVector<TYPE>::BVector(const BVector<TYPE>& o)
	:	BUntypedVector(o)
{
}

template<class TYPE> inline
BVector<TYPE>::~BVector()
{
	MakeEmpty();
}

template<class TYPE> inline
BVector<TYPE>& BVector<TYPE>::operator=(const BVector<TYPE>& o)
{
	BUntypedVector::operator=(o);
	return *this;
}

template<class TYPE> inline
size_t BVector<TYPE>::CountItems() const
{
	return CountElements();
}

template<class TYPE> inline
size_t BVector<TYPE>::CountAvail() const
{
	return BUntypedVector::CountAvail();
}

template<class TYPE> inline
const TYPE& BVector<TYPE>::operator[](size_t i) const
{
	return *static_cast<const TYPE*>(ElementAt(i));
}

template<class TYPE> inline
const TYPE& BVector<TYPE>::ItemAt(size_t i) const
{
	return *static_cast<const TYPE*>(ElementAt(i));
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddItem()
{
	return AddElement(NULL);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddItem(const TYPE &item)
{
	return AddElement(&item);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::InsertItem(size_t index)
{
	return InsertElement(NULL, index);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::InsertItem(const TYPE &item, size_t index)
{
	return InsertElement(&item, index);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::ReplaceItem(size_t index, const TYPE& item)
{
	TYPE* elem = static_cast<TYPE*>(EditElementAt(index));
	if (elem) {
		*elem = item;
		return index;
	}
	return B_NO_MEMORY;
}

template<class TYPE> inline
TYPE& BVector<TYPE>::EditItemAt(size_t i)
{
	return *static_cast<TYPE*>(EditElementAt(i));
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddVector(const BVector<TYPE>& o)
{
	return AddUntypedVector(o);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::InsertVector(const BVector<TYPE>& o, size_t index)
{
	return InsertUntypedVector(o, index);
}

template<class TYPE> inline
void BVector<TYPE>::MakeEmpty()
{
	BUntypedVector::MakeEmpty();
}

template<class TYPE> inline
void BVector<TYPE>::RemoveItems(size_t index, size_t count)
{
	RemoveElements(index, count);
}

template<class TYPE> inline
void BVector<TYPE>::RemoveItem(size_t index)
{
	RemoveElements(index, 1);
}

template<class TYPE> inline
void BVector<TYPE>::Reserve(size_t total_space)
{
	BUntypedVector::Reserve(total_space);
}

template<class TYPE> inline
void BVector<TYPE>::ReserveExtra(size_t extra_space)
{
	BUntypedVector::ReserveExtra(extra_space);
}

template<class TYPE> inline
void BVector<TYPE>::Swap(BVector<TYPE>& o)
{
	BUntypedVector::Swap(o);
}

template<class TYPE>
void BVector<TYPE>::PerformConstruct(void* base, size_t count) const
{
	BConstruct(static_cast<TYPE*>(base), count);
}

template<class TYPE>
void BVector<TYPE>::PerformCopy(void* to, const void* from, size_t count) const
{
	BCopy(static_cast<TYPE*>(to), static_cast<const TYPE*>(from), count);
}

template<class TYPE>
void BVector<TYPE>::PerformDestroy(void* base, size_t count) const
{
	BDestroy(static_cast<TYPE*>(base), count);
}

template<class TYPE>
void BVector<TYPE>::PerformMoveBefore(	void* to, void* from, size_t count) const
{
	BMoveBefore(static_cast<TYPE*>(to), static_cast<TYPE*>(from), count);
}

template<class TYPE>
void BVector<TYPE>::PerformMoveAfter(	void* to, void* from, size_t count) const
{
	BMoveAfter(static_cast<TYPE*>(to), static_cast<TYPE*>(from), count);
}

template<class TYPE>
void BVector<TYPE>::PerformAssign(	void* to, const void* from, size_t count) const
{
	BAssign(static_cast<TYPE*>(to), static_cast<const TYPE*>(from), count);
}

/*-------------------------------------------------------------*/

template<class TYPE> inline
void BMoveBefore(BVector<TYPE>* to, BVector<TYPE>* from, size_t count)
{
	BMoveBefore(static_cast<BUntypedVector*>(to), static_cast<BUntypedVector*>(from), count);
}

template<class TYPE> inline
void BMoveAfter(BVector<TYPE>* to, BVector<TYPE>* from, size_t count)
{
	BMoveAfter(static_cast<BUntypedVector*>(to), static_cast<BUntypedVector*>(from), count);
}

template<class TYPE> inline
void BSwap(BVector<TYPE>& v1, BVector<TYPE>& v2)
{
	v1.Swap(v2);
}

} }	// namespace B::Support

#endif
