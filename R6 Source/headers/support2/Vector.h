/***************************************************************************
//
//	File:			support2/Vector.h
//
//	Description:	A simple array-like container class.  Implemented as
//					a general purpose abstract base-class BAbstractVector,
//					with the concrete class BVector layered on top and
//					templatized on the array type.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_VECTOR_H
#define _SUPPORT2_VECTOR_H

#include <support2/SupportDefs.h>
#include <support2/TypeFuncs.h>

namespace B {
namespace Support2 {

/*--------------------------------------------------------*/
/*----- BAbstractVector abstract base class --------------*/

class BAbstractVector
{
public:
							BAbstractVector(size_t element_size);
							BAbstractVector(const BAbstractVector& o);
							// WARNING: Your subclass must call MakeEmpty()
							// in its own destructor!
	virtual					~BAbstractVector();
	
			BAbstractVector&operator=(const BAbstractVector& o);
	
	/* Size stats */
	
			void			SetCapacity(size_t total_space);
			void			SetExtraCapacity(size_t extra_space);
			size_t			Capacity() const;
			
			size_t			ItemSize() const;
			
			size_t			CountItems() const;
			
	/* Data access */

			const void*		At(size_t index) const;
			void*			EditAt(size_t index);
	
	/* Array modification */
	
			ssize_t			Add(const void* newElement);
			ssize_t			AddAt(const void* newElement, size_t index);
			
			ssize_t			AddVector(const BAbstractVector& o);
			ssize_t			AddVectorAt(const BAbstractVector& o, size_t index = SSIZE_MAX);
			
			void			MakeEmpty();
			void			RemoveItemsAt(size_t index, size_t count);
			
			void			Swap(BAbstractVector& o);
			
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
void BMoveBefore(BAbstractVector* to, BAbstractVector* from, size_t count = 1);
void BMoveAfter(BAbstractVector* to, BAbstractVector* from, size_t count = 1);
void BSwap(BAbstractVector& v1, BAbstractVector& v2);

/*--------------------------------------------------------*/
/*----- BVector concrete class ---------------------------*/

template<class TYPE>
class BVector : private BAbstractVector
{
public:
							BVector();
							BVector(const BVector<TYPE>& o);
	virtual					~BVector();
	
			BVector<TYPE>&	operator=(const BVector<TYPE>& o);
	
	/* Size stats */
	
			void			SetCapacity(size_t total_space);
			void			SetExtraCapacity(size_t extra_space);
			size_t			Capacity() const;
			
			size_t			CountItems() const;
	
	/* Data access */

			const TYPE&		operator[](size_t i) const;
			const TYPE&		ItemAt(size_t i) const;
			TYPE&			EditItemAt(size_t i);
	
	/* Array modification */
	
			ssize_t			AddItem();
			ssize_t			AddItem(const TYPE& item);
			ssize_t			AddItemAt(size_t index);
			ssize_t			AddItemAt(const TYPE& item, size_t index);
			
			ssize_t			ReplaceItemAt(const TYPE& item, size_t index);
	
			ssize_t			AddVector(const BVector<TYPE>& o);
			ssize_t			AddVectorAt(const BVector<TYPE>& o, size_t index);
			
			void			MakeEmpty();
			void			RemoveItemsAt(size_t index, size_t count = 1);
	
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

inline void BSwap(BAbstractVector& v1, BAbstractVector& v2)
{
	v1.Swap(v2);
}

/*-------------------------------------------------------------*/

template<class TYPE> inline
BVector<TYPE>::BVector()
	:	BAbstractVector(sizeof(TYPE[2])/2)
{
}

template<class TYPE> inline
BVector<TYPE>::BVector(const BVector<TYPE>& o)
	:	BAbstractVector(o)
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
	BAbstractVector::operator=(o);
	return *this;
}

template<class TYPE> inline
void BVector<TYPE>::SetCapacity(size_t total_space)
{
	BAbstractVector::SetCapacity(total_space);
}

template<class TYPE> inline
void BVector<TYPE>::SetExtraCapacity(size_t extra_space)
{
	BAbstractVector::SetExtraCapacity(extra_space);
}

template<class TYPE> inline
size_t BVector<TYPE>::Capacity() const
{
	return BAbstractVector::Capacity();
}

template<class TYPE> inline
size_t BVector<TYPE>::CountItems() const
{
	return BAbstractVector::CountItems();
}

template<class TYPE> inline
const TYPE& BVector<TYPE>::operator[](size_t i) const
{
	return *static_cast<const TYPE*>(At(i));
}

template<class TYPE> inline
const TYPE& BVector<TYPE>::ItemAt(size_t i) const
{
	return *static_cast<const TYPE*>(At(i));
}

template<class TYPE> inline
TYPE& BVector<TYPE>::EditItemAt(size_t i)
{
	return *static_cast<TYPE*>(EditAt(i));
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddItem()
{
	return Add(NULL);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddItem(const TYPE &item)
{
	return Add(&item);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddItemAt(size_t index)
{
	return AddAt(NULL, index);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddItemAt(const TYPE &item, size_t index)
{
	return AddAt(&item, index);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::ReplaceItemAt(const TYPE& item, size_t index)
{
	TYPE* elem = static_cast<TYPE*>(EditAt(index));
	if (elem) {
		*elem = item;
		return index;
	}
	return B_NO_MEMORY;
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddVector(const BVector<TYPE>& o)
{
	return BAbstractVector::AddVector(o);
}

template<class TYPE> inline
ssize_t BVector<TYPE>::AddVectorAt(const BVector<TYPE>& o, size_t index)
{
	return BAbstractVector::AddVectorAt(o, index);
}

template<class TYPE> inline
void BVector<TYPE>::MakeEmpty()
{
	BAbstractVector::MakeEmpty();
}

template<class TYPE> inline
void BVector<TYPE>::RemoveItemsAt(size_t index, size_t count)
{
	BAbstractVector::RemoveItemsAt(index, count);
}

template<class TYPE> inline
void BVector<TYPE>::Swap(BVector<TYPE>& o)
{
	BAbstractVector::Swap(o);
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
	BMoveBefore(static_cast<BAbstractVector*>(to), static_cast<BAbstractVector*>(from), count);
}

template<class TYPE> inline
void BMoveAfter(BVector<TYPE>* to, BVector<TYPE>* from, size_t count)
{
	BMoveAfter(static_cast<BAbstractVector*>(to), static_cast<BAbstractVector*>(from), count);
}

template<class TYPE> inline
void BSwap(BVector<TYPE>& v1, BVector<TYPE>& v2)
{
	v1.Swap(v2);
}

} }	// namespace B::Support2

#endif
