#include <support/OrderedVector.h>

#include <kernel/OS.h>
#include <support/Debug.h>

#include <stdlib.h>

namespace B {
namespace Support {

BUntypedOrderedVector::BUntypedOrderedVector(size_t element_size)
	:	BUntypedVector(element_size)
{
}

BUntypedOrderedVector::BUntypedOrderedVector(const BUntypedVector& o)
	:	BUntypedVector(o)
{
}

BUntypedOrderedVector::~BUntypedOrderedVector()
{
}

ssize_t BUntypedOrderedVector::InsertOrderedElement(const void* newElement, bool* added)
{
	size_t pos;
	if (!GetOrderedElementIndexOf(newElement, &pos)) {
		pos = InsertElement(newElement, pos);
		if (added) *added = (static_cast<ssize_t>(pos) >= B_OK ? true : false);
	} else {
		void* elem = EditElementAt(pos);
		if (elem)
			PerformAssign(elem, newElement, 1);
		if (added) *added = false;
	}
	return static_cast<ssize_t>(pos);
}

ssize_t BUntypedOrderedVector::FindOrderedElement(const void* newElement) const
{
	size_t pos;
	return GetOrderedElementIndexOf(newElement, &pos) ? pos : B_NAME_NOT_FOUND;
}

ssize_t BUntypedOrderedVector::RemoveOrderedElement(const void* element)
{
	size_t pos;
	if (GetOrderedElementIndexOf(element, &pos)) {
		RemoveElements(pos, 1);
		return pos;
	}
	return B_NAME_NOT_FOUND;
}

bool BUntypedOrderedVector::GetOrderedElementIndexOf(	const void* element,
														size_t* pos) const
{
	ssize_t mid, low = 0, high = CountElements()-1;
	while (low <= high) {
		mid = (low + high)/2;
		const int32 cmp = PerformCompare(element, ElementAt(mid));
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

void BMoveBefore(BUntypedOrderedVector* to, BUntypedOrderedVector* from, size_t count)
{
	memcpy(to, from, sizeof(BUntypedOrderedVector)*count);
}

void BMoveAfter(BUntypedOrderedVector* to, BUntypedOrderedVector* from, size_t count)
{
	memmove(to, from, sizeof(BUntypedOrderedVector)*count);
}

status_t BUntypedOrderedVector::_ReservedUntypedOrderedVector1() { return B_UNSUPPORTED; }
status_t BUntypedOrderedVector::_ReservedUntypedOrderedVector2() { return B_UNSUPPORTED; }
status_t BUntypedOrderedVector::_ReservedUntypedOrderedVector3() { return B_UNSUPPORTED; }
status_t BUntypedOrderedVector::_ReservedUntypedOrderedVector4() { return B_UNSUPPORTED; }
status_t BUntypedOrderedVector::_ReservedUntypedOrderedVector5() { return B_UNSUPPORTED; }
status_t BUntypedOrderedVector::_ReservedUntypedOrderedVector6() { return B_UNSUPPORTED; }

} }	// namespace B::Support
