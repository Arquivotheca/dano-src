#include <support2/OrderedVector.h>

#include <kernel/OS.h>
#include <support2/Debug.h>

#include <stdlib.h>

namespace B {
namespace Support2 {

BAbstractOrderedVector::BAbstractOrderedVector(size_t element_size)
	:	BAbstractVector(element_size)
{
}

BAbstractOrderedVector::BAbstractOrderedVector(const BAbstractVector& o)
	:	BAbstractVector(o)
{
}

BAbstractOrderedVector::~BAbstractOrderedVector()
{
}

BAbstractOrderedVector&	BAbstractOrderedVector::operator=(const BAbstractOrderedVector& o)
{
	BAbstractVector::operator=(o);
	return *this;
}

ssize_t BAbstractOrderedVector::AddOrdered(const void* newElement, bool* added)
{
	size_t pos;
	if (!GetOrderOf(newElement, &pos)) {
		pos = AddAt(newElement, pos);
		if (added) *added = (static_cast<ssize_t>(pos) >= B_OK ? true : false);
	} else {
		void* elem = EditAt(pos);
		if (elem)
			PerformAssign(elem, newElement, 1);
		if (added) *added = false;
	}
	return static_cast<ssize_t>(pos);
}

ssize_t BAbstractOrderedVector::OrderOf(const void* newElement) const
{
	size_t pos;
	return GetOrderOf(newElement, &pos) ? pos : B_NAME_NOT_FOUND;
}

ssize_t BAbstractOrderedVector::RemoveOrdered(const void* element)
{
	size_t pos;
	if (GetOrderOf(element, &pos)) {
		RemoveItemsAt(pos, 1);
		return pos;
	}
	return B_NAME_NOT_FOUND;
}

bool BAbstractOrderedVector::GetOrderOf(const void* element, size_t* pos) const
{
	ssize_t mid, low = 0, high = CountItems()-1;
	while (low <= high) {
		mid = (low + high)/2;
		const int32 cmp = PerformCompare(element, At(mid));
		if (cmp < 0) {
			high = mid-1;
		} else if (cmp > 0) {
			low = mid+1;
		} else {
			*pos = mid;
			return true;
		}
	}
	
	*pos = low;
	return false;
}

void BMoveBefore(BAbstractOrderedVector* to, BAbstractOrderedVector* from, size_t count)
{
	memcpy(to, from, sizeof(BAbstractOrderedVector)*count);
}

void BMoveAfter(BAbstractOrderedVector* to, BAbstractOrderedVector* from, size_t count)
{
	memmove(to, from, sizeof(BAbstractOrderedVector)*count);
}

status_t BAbstractOrderedVector::_ReservedUntypedOrderedVector1() { return B_UNSUPPORTED; }
status_t BAbstractOrderedVector::_ReservedUntypedOrderedVector2() { return B_UNSUPPORTED; }
status_t BAbstractOrderedVector::_ReservedUntypedOrderedVector3() { return B_UNSUPPORTED; }
status_t BAbstractOrderedVector::_ReservedUntypedOrderedVector4() { return B_UNSUPPORTED; }
status_t BAbstractOrderedVector::_ReservedUntypedOrderedVector5() { return B_UNSUPPORTED; }
status_t BAbstractOrderedVector::_ReservedUntypedOrderedVector6() { return B_UNSUPPORTED; }

} }	// namespace B::Support2
