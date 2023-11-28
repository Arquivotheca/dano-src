#include <experimental/Order.h>

#include <Autolock.h>

#include <stdlib.h>

TOrderBase::TOrderBase(size_t size)
	: fSize(size), fOrder(0)
{
}
TOrderBase::~TOrderBase()
{
	delete fOrder;
}

size_t TOrderBase::operator[](int32 i) const
{
	return Order()[i];
}

int TOrderBase::compare_elements(const void* e1, const void* e2)
{
	return fOrderObj->Compare(*(const size_t*)e1, *(const size_t*)e2);
}

size_t* TOrderBase::Order() const
{
	if( fOrder ) return fOrder;
	
	TOrderBase* This = const_cast<TOrderBase*>(this);
	This->fOrder = new size_t[fSize];
	for( size_t i=0; i<fSize; i++ ) This->fOrder[i] = i;
	BAutolock l(fGlobalLock);
	fOrderObj = This;
	qsort(This->fOrder, fSize, sizeof(size_t), compare_elements);
	fOrderObj = 0;
	
	return fOrder;
}

BLocker TOrderBase::fGlobalLock;
TOrderBase* TOrderBase::fOrderObj = 0;
