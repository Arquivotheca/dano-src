/***************************************************************************
//
//	File:			support/OrderedVector.h
//
//	Description:	A BVector whose types have an intrinsic ordering.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _SUPPORT_ORDEREDVECTOR_H
#define _SUPPORT_ORDEREDVECTOR_H

#include <support/Vector.h>

namespace B {
namespace Support {

/*--------------------------------------------------------*/
/*----- BUntypedOrderedVector abstract base class --------*/

class BUntypedOrderedVector : public BUntypedVector
{
public:
						BUntypedOrderedVector(size_t element_size);
						BUntypedOrderedVector(const BUntypedVector& o);
						// WARNING: Your subclass must call MakeEmpty()
						// in its own destructor!
virtual					~BUntypedOrderedVector();

		// Note that we inherit the assignment operator, so you can
		// assign a plain BUntypedVector to an instance of this class.

		ssize_t			InsertOrderedElement(	const void* newElement,
												bool* added = NULL);
		ssize_t			FindOrderedElement(const void* element) const;
		
		bool			GetOrderedElementIndexOf(	const void* element,
													size_t* index) const;
		
		ssize_t			RemoveOrderedElement(const void* element);
		
		void			Swap(BUntypedOrderedVector& o);
		
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
void BMoveBefore(BUntypedOrderedVector* to, BUntypedOrderedVector* from, size_t count = 1);
void BMoveAfter(BUntypedOrderedVector* to, BUntypedOrderedVector* from, size_t count = 1);
void BSwap(BUntypedOrderedVector& v1, BUntypedOrderedVector& v2);

/*--------------------------------------------------------*/
/*----- BOrderedVector concrete class --------------------*/

template<class TYPE>
class BOrderedVector : private BUntypedOrderedVector
{
public:
						BOrderedVector();
						BOrderedVector(const BOrderedVector<TYPE>& o);
virtual					~BOrderedVector();

		BOrderedVector<TYPE>&	operator=(const BOrderedVector<TYPE>& o);

		size_t			CountItems() const;
		size_t			CountAvail() const;

		const TYPE&		operator[](size_t i) const;
		const TYPE&		ItemAt(size_t i) const;

		ssize_t			InsertValue(const TYPE& item, bool* added = NULL);
		ssize_t			FindValue(const TYPE& item) const;

		bool			GetIndexOf(const TYPE& item, size_t* index) const;
		
		void			MakeEmpty();
		void			RemoveItems(size_t index, size_t count);
		void			RemoveItem(size_t index);

		ssize_t			RemoveValue(const TYPE& item);
		
		void			Reserve(size_t total_space);
		void			ReserveExtra(size_t extra_space);
		
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

inline void BSwap(BUntypedOrderedVector& v1, BUntypedOrderedVector& v2)
{
	v1.Swap(v2);
}

/*-------------------------------------------------------------*/

template<class TYPE> inline
BOrderedVector<TYPE>::BOrderedVector()
	:	BUntypedOrderedVector(sizeof(TYPE[2])/2)
{
}

template<class TYPE> inline
BOrderedVector<TYPE>::BOrderedVector(const BOrderedVector<TYPE>& o)
	:	BUntypedOrderedVector(o)
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
	BUntypedOrderedVector::operator=(o);
	return *this;
}

template<class TYPE> inline
size_t BOrderedVector<TYPE>::CountItems() const
{
	return CountElements();
}

template<class TYPE> inline
size_t BOrderedVector<TYPE>::CountAvail() const
{
	return BUntypedOrderedVector::CountAvail();
}

template<class TYPE> inline
const TYPE& BOrderedVector<TYPE>::operator[](size_t i) const
{
	return *static_cast<const TYPE*>(ElementAt(i));
}

template<class TYPE> inline
const TYPE& BOrderedVector<TYPE>::ItemAt(size_t i) const
{
	return *static_cast<const TYPE*>(ElementAt(i));
}

template<class TYPE> inline
ssize_t BOrderedVector<TYPE>::InsertValue(const TYPE& item, bool* added)
{
	return InsertOrderedElement(&item, added);
}

template<class TYPE> inline
ssize_t BOrderedVector<TYPE>::FindValue(const TYPE& item) const
{
	return FindOrderedElement(&item);
}

template<class TYPE> inline
bool BOrderedVector<TYPE>::GetIndexOf(const TYPE& item, size_t* index) const
{
	return GetOrderedElementIndexOf(&item, index);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::MakeEmpty()
{
	BUntypedOrderedVector::MakeEmpty();
}

template<class TYPE> inline
void BOrderedVector<TYPE>::RemoveItems(size_t index, size_t count)
{
	RemoveElements(index, count);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::RemoveItem(size_t index)
{
	RemoveElements(index, 1);
}

template<class TYPE> inline
ssize_t BOrderedVector<TYPE>::RemoveValue(const TYPE& item)
{
	return RemoveOrderedElement(&item);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::Reserve(size_t total_space)
{
	BUntypedOrderedVector::Reserve(total_space);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::ReserveExtra(size_t extra_space)
{
	BUntypedOrderedVector::ReserveExtra(extra_space);
}

template<class TYPE> inline
void BOrderedVector<TYPE>::Swap(BOrderedVector<TYPE>& o)
{
	BUntypedOrderedVector::Swap(o);
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
	BMoveBefore(static_cast<BUntypedOrderedVector*>(to), static_cast<BUntypedOrderedVector*>(from), count);
}

template<class TYPE> inline
void BMoveAfter(BOrderedVector<TYPE>* to, BOrderedVector<TYPE>* from, size_t count)
{
	BMoveAfter(static_cast<BUntypedOrderedVector*>(to), static_cast<BUntypedOrderedVector*>(from), count);
}

template<class TYPE> inline
void BSwap(BOrderedVector<TYPE>& v1, BOrderedVector<TYPE>& v2)
{
	v1.Swap(v2);
}

} }	// namespace B::Support

#endif
