#include "DumbVector.h"

#include <cstring>

namespace B {
namespace Private {

BDumbVector::BDumbVector() :
	BAbstractVector(sizeof(void*))
{
}

BDumbVector::BDumbVector(const BDumbVector &clone) :
	BAbstractVector(clone)
{
}

BDumbVector::~BDumbVector()
{
	MakeEmpty();
}

BDumbVector &
BDumbVector::operator=(const BDumbVector &clone)
{
	BAbstractVector::operator=(clone);
	return *this;
}

void 
BDumbVector::SetCapacity(size_t total_space)
{
	BAbstractVector::SetCapacity(total_space);
}

void 
BDumbVector::SetExtraCapacity(size_t extra_space)
{
	BAbstractVector::SetExtraCapacity(extra_space);
}

size_t 
BDumbVector::Capacity() const
{
	return BAbstractVector::Capacity();
}

size_t 
BDumbVector::CountItems() const
{
	return BAbstractVector::CountItems();
}

const void *
BDumbVector::ItemAt(size_t i) const
{
	return *static_cast<void * const *>(At(i));
}

void *
BDumbVector::EditItemAt(size_t i)
{
	return *static_cast<void**>(EditAt(i));
}


ssize_t 
BDumbVector::AddItem()
{
	return BAbstractVector::Add(0);
}

ssize_t 
BDumbVector::AddItem(void *item)
{
	return BAbstractVector::Add(&item);
}

ssize_t 
BDumbVector::AddItemAt(size_t index)
{
	return BAbstractVector::AddAt(0, index);
}

ssize_t 
BDumbVector::AddItemAt(void *item, size_t index)
{
	return BAbstractVector::AddAt(&item, index);
}

ssize_t 
BDumbVector::ReplaceItemAt(void *item, size_t index)
{
	void ** elem = static_cast<void**>(BAbstractVector::EditAt(index));
	if (elem)
	{
		*elem = item;
		return index;
	}
	return B_NO_MEMORY;
}

ssize_t 
BDumbVector::AddVector(const BDumbVector &o)
{
	return BAbstractVector::AddVector(o);
}

ssize_t 
BDumbVector::AddVectorAt(const BDumbVector &o, size_t index)
{
	return BAbstractVector::AddVectorAt(o, index);
}

void 
BDumbVector::MakeEmpty()
{
	BAbstractVector::MakeEmpty();
}

void 
BDumbVector::RemoveItemsAt(size_t index, size_t count)
{
	BAbstractVector::RemoveItemsAt(index, count);
}

void 
BDumbVector::Swap(BDumbVector &o)
{
	BAbstractVector::Swap(o);
}

void 
BDumbVector::PerformConstruct(void *, size_t) const
{
}

void 
BDumbVector::PerformCopy(void *to, const void *from, size_t count) const
{
	memcpy(to, from, count * sizeof(void*));
}

void 
BDumbVector::PerformDestroy(void *, size_t) const
{
}

void 
BDumbVector::PerformMoveBefore(void *to, void *from, size_t count) const
{
	memcpy(to, from, count * sizeof(void*));
}

void 
BDumbVector::PerformMoveAfter(void *to, void *from, size_t count) const
{
	memmove(to, from, count * sizeof(void*));
}

void 
BDumbVector::PerformAssign(void *to, const void *from, size_t count) const
{
	memcpy(to, from, count * sizeof(void*));
}

} } // B::Private
