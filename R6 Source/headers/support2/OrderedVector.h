/***************************************************************************
//
//	File:			support2/OrderedVector.h
//
//	Description:	A BVector whose types have an intrinsic ordering.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT2_ORDEREDVECTOR_H
#define _SUPPORT2_ORDEREDVECTOR_H

#include <support2/Vector.h>

namespace B {
namespace Support2 {

/*--------------------------------------------------------*/
/*----- BAbstractOrderedVector abstract base class -------*/

class BAbstractOrderedVector : public BAbstractVector
{
public:
							BAbstractOrderedVector(size_t element_size);
							BAbstractOrderedVector(const BAbstractVector& o);
							// WARNING: Your subclass must call MakeEmpty()
							// in its own destructor!
	virtual					~BAbstractOrderedVector();
	
			BAbstractOrderedVector& operator=(const BAbstractOrderedVector& o);
			
			// Note that we inherit the assignment operator, so you can
			// assign a plain BAbstractVector to an instance of this class.
	
			ssize_t			AddOrdered(const void* newElement, bool* added = NULL);
			
			ssize_t			OrderOf(const void* element) const;
			bool			GetOrderOf(const void* element, size_t* index) const;
			
			ssize_t			RemoveOrdered(const void* element);
			
			void			Swap(BAbstractOrderedVector& o);
			
protected:
	virtual	int32			PerformCompare(const void* d1, const void* d2) const = 0;
	virtual	bool			PerformLessThan(const void* d1, const void* d2) const = 0;
	
private:
	virtual	status_t		_ReservedUntypedOrderedVector1();
	virtual	status_t		_ReservedUntypedOrderedVector2();
	virtual	status_t		_ReservedUntypedOrderedVector3();
	virtual	status_t		_ReservedUntypedOrderedVector4();
	virtual	status_t		_ReservedUntypedOrderedVector5();
	virtual	status_t		_ReservedUntypedOrderedVector6();
	
			int32			_reserved[2];
};

// Type optimizations.
void BMoveBefore(BAbstractOrderedVector* to, BAbstractOrderedVector* from, size_t count = 1);
void BMoveAfter(BAbstractOrderedVector* to, BAbstractOrderedVector* from, size_t count = 1);
void BSwap(BAbstractOrderedVector& v1, BAbstractOrderedVector& v2);

/*--------------------------------------------------------*/
/*----- BOrderedVector concrete class --------------------*/

template<class TYPE>
class BOrderedVector : private BAbstractOrderedVector
{
public:
							BOrderedVector();
							BOrderedVector(const BOrderedVector<TYPE>& o);
	virtual					~BOrderedVector();
	
			BOrderedVector<TYPE>&	operator=(const BOrderedVector<TYPE>& o);
	
	/* Size stats */
	
			void			SetCapacity(size_t total_space);
			void			SetExtraCapacity(size_t extra_space);
			size_t			Capacity() const;
			
			size_t			CountItems() const;
	
	/* Data access */

			const TYPE&		operator[](size_t i) const;
			const TYPE&		ItemAt(size_t i) const;
	
			ssize_t			IndexOf(const TYPE& item) const;
			bool			GetIndexOf(const TYPE& item, size_t* index) const;
			
	/* Array modification */
	
			ssize_t			AddItem(const TYPE& item, bool* added = NULL);
			
			void			MakeEmpty();
			void			RemoveItemsAt(size_t index, size_t count = 1);
	
			ssize_t			RemoveItemFor(const TYPE& item);
			
			void			Swap(BOrderedVector<TYPE>& o);
			
protected:
	virtual	void			PerformConstruct(void* base, size_t count) const;
	virtual	void			PerformCopy(void* to, const void* from, size_t count) const;
	virtual	void			PerformDestroy(void* base, size_t count) const;
	
	virtual	void			PerformMoveBefore(void* to, void* from, size_t count) const;
	virtual	void			PerformMoveAfter(void* to, void* from, size_t count) const;
	
	virtual	void			PerformAssign(void* to, const void* from, size_t count) const;
	
	virtual	int32			PerformCompare(const void* d1, const void* d2) const;
	virtual	bool			PerformLessThan(const void* d1, const void* d2) const;
};

// Type optimizations.
template<class TYPE>
void BMoveBefore(BOrderedVector<TYPE>* to, BOrderedVector<TYPE>* from, size_t count = 1);
template<class TYPE>
void BMoveAfter(BOrderedVector<TYPE>* to, BOrderedVector<TYPE>* from, size_t count = 1);
template<class TYPE>
void BSwap(BOrderedVector<TYPE>& v1, BOrderedVector<TYPE>& v2);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline void BSwap(BAbstractOrderedVector& v1, BAbstractOrderedVector& v2)
{
	v1.Swap(v2);
}

/*-------------------------------------------------------------*/

template<class TYPE> inline
BOrderedVector<TYPE>::BOrderedVector()
	:	BAbstractOrderedVector(sizeof(TYPE[2])/2)
{
}

template<class TYPE> inline
BOrderedVector<TYPE>::BOrderedVector(const BOrderedVector<TYPE>& o)
	:	BAbstractOrderedVector(o)
{
}

template<class TYPE> inline
BOrderedVector<TYPE>::~BOrderedVector()
{
	MakeEmpty();
}

template<class TYPE> inline
BOrderedVector<TYPE>& BOrderedVector<TYPE>::operator=(const BOrderedVector<TYPE>& o)
{
	BAbstractOrderedVector::operator=(o);
	return *this;
}

template<class TYPE> inline
void BOrderedVector<TYPE>::SetCapacity(size_t total_space)
{
	BAbstractVector::SetCapacity(total_space);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::SetExtraCapacity(size_t extra_space)
{
	BAbstractVector::SetExtraCapacity(extra_space);
}

template<class TYPE> inline
size_t BOrderedVector<TYPE>::Capacity() const
{
	return BAbstractVector::Capacity();
}

template<class TYPE> inline
size_t BOrderedVector<TYPE>::CountItems() const
{
	return BAbstractVector::CountItems();
}

template<class TYPE> inline
const TYPE& BOrderedVector<TYPE>::operator[](size_t i) const
{
	return *static_cast<const TYPE*>(At(i));
}

template<class TYPE> inline
const TYPE& BOrderedVector<TYPE>::ItemAt(size_t i) const
{
	return *static_cast<const TYPE*>(At(i));
}

template<class TYPE> inline
ssize_t BOrderedVector<TYPE>::AddItem(const TYPE& item, bool* added)
{
	return AddOrdered(&item, added);
}

template<class TYPE> inline
ssize_t BOrderedVector<TYPE>::IndexOf(const TYPE& item) const
{
	return OrderOf(&item);
}

template<class TYPE> inline
bool BOrderedVector<TYPE>::GetIndexOf(const TYPE& item, size_t* index) const
{
	return GetOrderOf(&item, index);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::MakeEmpty()
{
	BAbstractOrderedVector::MakeEmpty();
}

template<class TYPE> inline
void BOrderedVector<TYPE>::RemoveItemsAt(size_t index, size_t count = 1)
{
	BAbstractOrderedVector::RemoveItemsAt(index, count);
}

template<class TYPE> inline
ssize_t BOrderedVector<TYPE>::RemoveItemFor(const TYPE& item)
{
	return RemoveOrdered(&item);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::Swap(BOrderedVector<TYPE>& o)
{
	BAbstractOrderedVector::Swap(o);
}

template<class TYPE>
void BOrderedVector<TYPE>::PerformConstruct(void* base, size_t count) const
{
	BConstruct(static_cast<TYPE*>(base), count);
}

template<class TYPE>
void BOrderedVector<TYPE>::PerformCopy(void* to, const void* from, size_t count) const
{
	BCopy(static_cast<TYPE*>(to), static_cast<const TYPE*>(from), count);
}

template<class TYPE>
void BOrderedVector<TYPE>::PerformDestroy(void* base, size_t count) const
{
	BDestroy(static_cast<TYPE*>(base), count);
}

template<class TYPE>
void BOrderedVector<TYPE>::PerformMoveBefore(void* to, void* from, size_t count) const
{
	BMoveBefore(static_cast<TYPE*>(to), static_cast<TYPE*>(from), count);
}

template<class TYPE>
void BOrderedVector<TYPE>::PerformMoveAfter(void* to, void* from, size_t count) const
{
	BMoveAfter(static_cast<TYPE*>(to), static_cast<TYPE*>(from), count);
}

template<class TYPE>
void BOrderedVector<TYPE>::PerformAssign(void* to, const void* from, size_t count) const
{
	BAssign(static_cast<TYPE*>(to), static_cast<const TYPE*>(from), count);
}

template<class TYPE>
int32 BOrderedVector<TYPE>::PerformCompare(const void* d1, const void* d2) const
{
	return BCompare(*static_cast<const TYPE*>(d1), *static_cast<const TYPE*>(d2));
}

template<class TYPE>
bool BOrderedVector<TYPE>::PerformLessThan(const void* d1, const void* d2) const
{
	return BLessThan(*static_cast<const TYPE*>(d1), *static_cast<const TYPE*>(d2));
}

/*-------------------------------------------------------------*/

template<class TYPE> inline
void BMoveBefore(BOrderedVector<TYPE>* to, BOrderedVector<TYPE>* from, size_t count)
{
	BMoveBefore(static_cast<BAbstractOrderedVector*>(to), static_cast<BAbstractOrderedVector*>(from), count);
}

template<class TYPE> inline
void BMoveAfter(BOrderedVector<TYPE>* to, BOrderedVector<TYPE>* from, size_t count)
{
	BMoveAfter(static_cast<BAbstractOrderedVector*>(to), static_cast<BAbstractOrderedVector*>(from), count);
}

template<class TYPE> inline
void BSwap(BOrderedVector<TYPE>& v1, BOrderedVector<TYPE>& v2)
{
	v1.Swap(v2);
}

} }	// namespace B::Support2

#endif
