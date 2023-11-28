#include <Debug.h>
#include "ObjectList.h"

_PointerList_::_PointerList_(const _PointerList_ &list)
	:	BList(list),
		owning(list.owning)
{
}

_PointerList_::_PointerList_(int32 itemsPerBlock = 20, bool owningList)
	:	BList(itemsPerBlock),
		owning(owningList)
{
}

_PointerList_::~_PointerList_()
{
}

void *
_PointerList_::EachElement(GenericEachFunction func, void *passThru)
{
	// iterates through all elements, calling func on each
	// if each function returns a nonzero value, terminates early
	void *result = NULL; 
	int32 numElements = CountItems();
	
	for (int32 index = 0; index < numElements; index++)
		if ((result = func(ItemAtFast(index), passThru)) != NULL)
			break;

	return result;
}

bool 
_PointerList_::ReplaceItem(int32 index, void *item)
{
	if (index < 0 || index >= CountItems())
		return false;

	(((void **)Items())[index]) = item;
	return true;
}


void
_PointerList_::SortItems(GenericCompareFunction cmp)
{
	size_t count = CountItems();
	if (count < 2)
		return;
	
	size_t r = count;
	size_t l = (r / 2) + 1;
	
	void **base = (void **)Items();
	void **lp = base + (l - 1);
	void **rp = base + (r - 1);
	
	for (;;) {
		if (l > 1) {
			l--;
			lp--;
		} else {
			void *tmp = *lp;
			*lp = *rp;
			*rp = tmp;

			if (--r == 1)
				return;
				
			rp--;
		}
			
		size_t j = l;
		
		void **jp = base + (j - 1);
		
		while (j*2 <= r) {
			j *= 2;
			
			void **ip = jp;
			jp = base + (j - 1);
			
			if (j < r) {
				void **tmp = jp + 1;
				if (cmp(*jp, *tmp) < 0) {
					j++;
					jp = tmp;
				}
			}
			
			if (cmp(*ip, *jp) >= 0) 
				break;

			void *tmp = *ip;
			*ip = *jp;
			*jp = tmp;
		}
	}
}

void
_PointerList_::SortItems(GenericCompareFunctionWithState cmp,
	void *params)
{
	size_t count = CountItems();
	if (count < 2)
		return;
	
	size_t r = count;
	size_t l = (r / 2) + 1;
	
	void **base = (void **)Items();
	void **lp = base + (l - 1);
	void **rp = base + (r - 1);
	
	for (;;) {
		if (l > 1) {
			l--;
			lp--;
		} else {
			void *tmp = *lp;
			*lp = *rp;
			*rp = tmp;

			if (--r == 1)
				return;
				
			rp--;
		}
			
		size_t j = l;
		
		void **jp = base + (j - 1);
		
		while (j*2 <= r) {
			j *= 2;
			
			void **ip = jp;
			jp = base + (j - 1);
			
			if (j < r) {
				void **tmp = jp + 1;
				if (cmp(*jp, *tmp, params) < 0) {
					j++;
					jp = tmp;
				}
			}
			
			if (cmp(*ip, *jp, params) >= 0) 
				break;

			void *tmp = *ip;
			*ip = *jp;
			*jp = tmp;
		}
	}
}

static void
heapup(void **base, int32 count, int32 i, _PointerList_::GenericCompareFunction cmp)
{
	for (;;) {
		int32 j = 2*i + 1;
		if (j >= count)
			break;

		int32 k = 2*i + 2;
		if (k < count && (cmp)(base[j], base[k]) < 0)
			j = k;
		if ((cmp)(base[i], base[j]) >= 0)
			break;
	
		void *tmp = base[i];
		base[i] = base[j];
		base[j] = tmp;
		i = j;
	}
}

void
_PointerList_::HSortItems(GenericCompareFunction cmp)
{
	ASSERT(!"broken, dont use");
	int32 count = CountItems();
	if (count < 2)
		return;

	void **base = (void **)Items();	

	for (int32 index = (count - 2) / 2; index ;--index) 
		heapup(base, count, index, cmp);

	while (--count) {
		void *tmp = base[count];
		base[count] = base[0];
		base[0] = tmp;

		heapup(base, count, 0, cmp);
	}
}

static void
heapup(void **base, int32 count, int32 index, 
	_PointerList_::GenericCompareFunctionWithState cmp, void *state)
{
	for (;;) {
		int32 j = 2 * index + 1;
		if (j >= count)
			break;

		int32 k = 2 * index + 2;
		if (k < count && (cmp)(base[j], base[k], state) < 0)
			j = k;
		if ((cmp)(base[index], base[j], state) >= 0)
			break;
	
		void *tmp = base[index];
		base[index] = base[j];
		base[j] = tmp;
		index = j;
	}
}

void
_PointerList_::HSortItems(GenericCompareFunctionWithState cmp, void *state)
{
	ASSERT(!"broken, dont use");
	int32 count = CountItems();
	if (count < 2)
		return;

	void **base = (void **)Items();	

	for (int32 index = (count - 2) / 2; index ;--index) 
		heapup(base, count, index, cmp, state);

	while (--count) {
		void *tmp = base[count];
		base[count] = base[0];
		base[0] = tmp;

		heapup(base, count, 0, cmp, state);
	}
}

void *
_PointerList_::BinarySearch(const void *key, GenericCompareFunction func) const
{
	int32 r = CountItems();
	void *result = 0;

	for (int32 l = 1; l <= r; ) {
		int32 m = (l + r) / 2;

		result = ItemAt(m - 1);
		int32 compareResult = (func)(result, key);
		if (compareResult == 0)
			return result;
		else if (compareResult < 0)
			l = m + 1;
		else
			r = m - 1;
	}
		
	return 0;
}

void *
_PointerList_::BinarySearch(const void *key, GenericCompareFunctionWithState func, void *state) const
{
	int32 r = CountItems();
	void *result = 0;

	for (int32 l = 1; l <= r; ) {
		int32 m = (l + r) / 2;

		result = ItemAt(m - 1);
		int32 compareResult = (func)(result, key, state);
		if (compareResult == 0)
			return result;
		else if (compareResult < 0)
			l = m + 1;
		else
			r = m - 1;
	}
			
	return 0;
}

int32 
_PointerList_::BinarySearchIndex(const void *key, GenericCompareFunction func) const
{
	int32 r = CountItems() - 1;
	void *item = 0;
	int32 result = 0;
	int32 compareResult = 0;
	
	for (int32 l = 0; l <= r; ) {
		result = (l + r) / 2;

		item = ItemAt(result);

		compareResult = (func)(item, key);
		
		if (compareResult > 0)
			r = result - 1;
		else if (compareResult < 0)
			l = result + 1;
		else
			return result;
	}

	if (compareResult < 0)
		result ++;	
		
	return -(result + 1);
}

int32 
_PointerList_::BinarySearchIndex(const void *key, GenericCompareFunctionWithState func,
	void *state) const
{
	int32 r = CountItems() - 1;
	void *item = 0;
	int32 result = 0;
	int32 compareResult = 0;
	
	for (int32 l = 0; l <= r; ) {
		result = (l + r) / 2;

		item = ItemAt(result);

		compareResult = (func)(item, key, state);
		
		if (compareResult > 0)
			r = result - 1;
		else if (compareResult < 0)
			l = result + 1;
		else
			return result;
	}

	if (compareResult < 0)
		result++;	
		
	return -(result + 1);
}

int32 
_PointerList_::BinarySearchIndexByPredicate(const void *state,
	UnaryPredicateGlue func) const
{
	int32 r = CountItems() - 1;
	void *item = 0;
	int32 result = 0;
	int32 compareResult = 0;
	
	for (int32 l = 0; l <= r; ) {
		result = (l + r) / 2;

		item = ItemAt(result);

		compareResult = (func)(item, (void *)state);
			// ToDo: fix the above cast in the header file instead
		
		if (compareResult > 0)
			r = result - 1;
		else if (compareResult < 0)
			l = result + 1;
		else
			return result;
	}

	if (compareResult < 0)
		result++;	
		
	return -(result + 1);
}

