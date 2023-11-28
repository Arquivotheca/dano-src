
#ifndef SMARTARRAY_H
#define SMARTARRAY_H

#include <new>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <SupportDefs.h>

template<class TYPE>
class SmartArray
{
	public:
		inline				SmartArray(int32 size=0);
		inline				SmartArray(const SmartArray<TYPE>& o);
		inline				~SmartArray();
		inline	SmartArray<TYPE>&	operator=(const SmartArray<TYPE>& o);
		inline	const TYPE&	operator[](int i) const;
		inline	TYPE &		operator[](int i);
		inline	TYPE &		ItemAt(int i) const;
		inline	int32		AddItem();
		inline	int32		AddItem(const TYPE &item);
		inline	TYPE &		InsertItem(const TYPE &item, int32 index);
		inline	TYPE &		InsertItem(int32 index);
		inline	void		AssertSize(int size);
		inline	int32		CountItems() const;
		inline	int32		Count() const;
		inline	void		RemoveItems(int32 index, int32 len);
		inline	void		RemoveItem(int32 index);
		inline	void		MakeEmpty();

	private:
				bool		operator==(const SmartArray<TYPE>& o);
		
		inline	void		Resize(int size);
				int32		fSize;
				int32		fNumItems;
				TYPE *		fArray;
};

template<class TYPE>
TYPE & SmartArray<TYPE>::ItemAt(int i) const
{
	return fArray[i];
}

template<class TYPE>
const TYPE & SmartArray<TYPE>::operator[](int i) const
{
	return fArray[i];
}

template<class TYPE>
TYPE & SmartArray<TYPE>::operator[](int i)
{
	return fArray[i];
}

template<class TYPE>
int32 SmartArray<TYPE>::Count() const
{
	return fNumItems;
}

template<class TYPE>
int32 SmartArray<TYPE>::CountItems() const
{
	return Count();
}

template<class TYPE>
void SmartArray<TYPE>::RemoveItems(int32 index, int32 len)
{
	for (int i = index; i < index+len; i++) fArray[i].~TYPE();
	memmove(fArray+index,fArray+index+len,sizeof(TYPE)*(fNumItems-index-len));
	fNumItems-=len;
};

template<class TYPE>
void SmartArray<TYPE>::RemoveItem(int32 index)
{
	RemoveItems(index,1);
};

template<class TYPE>
TYPE & SmartArray<TYPE>::InsertItem(int32 index)
{
	int32 newSize = fNumItems+1;
	if (newSize > fSize) {
		fArray = (TYPE*)realloc(fArray, newSize * sizeof(TYPE));
		fSize = newSize;
	}
	fNumItems = newSize;

	memmove(fArray+index+1,fArray+index,(fNumItems-index-1)*sizeof(TYPE));
	new ((void*)&fArray[index]) TYPE;
	return fArray[index];
}

template<class TYPE>
TYPE & SmartArray<TYPE>::InsertItem(const TYPE &item, int32 index)
{
	InsertItem(index);
	fArray[index] = item;
	return fArray[index];
}

template<class TYPE>
SmartArray<TYPE>::SmartArray(int32 /*size*/)
{
	fSize = fNumItems = 0;
	fArray = NULL;
//	if (size) Resize(size);
}

template<class TYPE>
SmartArray<TYPE>::SmartArray(const SmartArray<TYPE>& o)
{
	fSize = fNumItems = 0;
	fArray = NULL;
	if (o.fNumItems > 0) {
		Resize(o.fNumItems);
		for (int32 i=0; i<fNumItems; i++)
			new ((void*)&fArray[i]) TYPE(o.fArray[i]);
	}
}

template<class TYPE>
SmartArray<TYPE>::~SmartArray()
{
	for (int i = 0; i < fNumItems; i++) fArray[i].~TYPE();
	if (fArray) free(fArray);
}

template<class TYPE>
SmartArray<TYPE>& SmartArray<TYPE>::operator=(const SmartArray<TYPE>& o)
{
	int32 i;
	for (i = 0; i < fNumItems; i++) {
		fArray[i].~TYPE();
	}
	if (fNumItems != o.fNumItems) {
		fNumItems = o.fNumItems;
		fSize = o.fNumItems;
		if (fNumItems <= 0) {
			free(fArray);
			fArray = NULL;
		} else {
			fArray = (TYPE*)realloc(fArray, fNumItems*sizeof(TYPE));
			if (!fArray)
				fNumItems = fSize = 0;
		}
	}
	for (i=0; i<fNumItems; i++) {
		new ((void*)&fArray[i]) TYPE(o.fArray[i]);
	}
	return *this;
}

template<class TYPE>
void SmartArray<TYPE>::Resize(int size)
{
	assert(size);
	if (size < fNumItems) {
		for (int i = size; i < fNumItems; i++)
			fArray[i].~TYPE();
		fArray = (TYPE*)realloc(fArray, size * sizeof(TYPE));
	} else if (size > fNumItems) {
		fArray = (TYPE*)realloc(fArray, size * sizeof(TYPE));
		for (int i = fNumItems; i < size; i++)
			new ((void*)&fArray[i]) TYPE;
	}
	
	fSize = size;
	fNumItems = size;
}

template<class TYPE>
void SmartArray<TYPE>::AssertSize(int size)
{
	if (fNumItems < size) Resize(size);
}

template<class TYPE>
int32 SmartArray<TYPE>::AddItem()
{
	Resize(fNumItems+1);
	return fNumItems-1;
}

template<class TYPE>
int32 SmartArray<TYPE>::AddItem(const TYPE &item)
{
	int32 i = AddItem();
	ItemAt(i) = item;
	return i;
}

template<class TYPE>
void SmartArray<TYPE>::MakeEmpty()
{
	RemoveItems(0,Count());
}

#endif
