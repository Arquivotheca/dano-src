//
#include <Debug.h>
#include "TypedList.h"

void *
PointerList::EachElement(GenericEachFunction func, void *passThru)
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

PointerList::PointerList(const PointerList &list)
	:	BList(list),
		owning(list.owning)
{
}

PointerList::PointerList(int32 itemsPerBlock = 20, bool owningList)
	:	BList(itemsPerBlock),
		owning(owningList)
{
}

PointerList::~PointerList()
{
}

bool
PointerList::Owning() const
{
	return owning;
}

bool 
PointerList::AddUnique(void *newItem)
{
	if (IndexOf(newItem) >= 0)
		return false;

	return AddItem(newItem);
}

struct OneMatchParams {
	void *matchThis;
	PointerList::GenericCompareFunction matchFunction;
};


static void *
MatchOne(void *item, void *castToParams)
{
	OneMatchParams *params = (OneMatchParams *)castToParams;
	if (params->matchFunction(item, params->matchThis) == 0)
		// got a match, terminate search
		return item;

	return 0;
}

bool 
PointerList::AddUnique(void *newItem, GenericCompareFunction function)
{
	OneMatchParams params;
	params.matchThis = newItem;
	params.matchFunction = function;

	if (EachElement(MatchOne, &params))
		// already in list
		return false;
	
	return AddItem(newItem);
}

// qsort from Metrowerks Standard Library  Version 1.6
//		Copyright © 1995-1996 Metrowerks, Inc.
//		All rights reserved.

#define table_ptr(i)	(((char *) table_base) + (member_size * ((i) - 1)))

#if !__POWERPC__

#define swap(dst, src, cnt)                                             \
do {                                                                    \
  char *  p;                                                            \
  char *  q;                                                            \
  size_t  n = cnt;                                                      \
                                                                        \
  unsigned char tmp;                                                    \
                                                                        \
  for (p = src, q = dst, n++; --n;)                                     \
  {                                                                     \
    tmp = *q;                                                           \
    *q++ = *p;                                                          \
    *p++ = tmp;                                                         \
  }                                                                     \
} while (0)
	
#else

#define swap(dst, src, cnt)                                             \
do {                                                                    \
  char *  p;                                                            \
  char *  q;                                                            \
  size_t  n = cnt;                                                      \
                                                                        \
  unsigned long tmp;                                                    \
                                                                        \
  for (p = (char *) src - 1, q = (char *) dst - 1, n++; --n;)     			\
  {                                                                     \
    tmp = *++q;                                                         \
    *q = *++p;                                                          \
    *p = tmp;                                                           \
  }                                                                     \
} while (0)

#endif

void
PointerList::SortItems(int (*cmp)(const void *, const void *, const void *),
	void *params)
{
	void *table_base = Items();
	size_t num_members = CountItems();
	size_t member_size = sizeof(void *);

	size_t	l, r, j;
	char *lp;
	char *rp;
	char *ip;
	char *jp;
	char *kp;
	
	if (num_members < 2)
		return;
	
	r = num_members;
	l = (r / 2) + 1;
	
	lp = table_ptr(l);
	rp = table_ptr(r);
	
	for (;;)
	{
		if (l > 1)
		{
			l--;
			lp -= member_size;
		}
		else
		{
			swap(lp,rp,member_size);
			
			if (--r == 1)
				return;
				
			rp -= member_size;
		}
			
		j = l;
		
		jp = table_ptr(j);
		
		while (j*2 <= r)
		{
			j *= 2;
			
			ip = jp;
			jp = table_ptr(j);
			
			if (j < r)
			{
				kp = jp + member_size;
				
				if (cmp(jp, kp, params) < 0)
				{
					j++;
					jp = kp;
				}
			}
			
			if (cmp(ip,jp, params) < 0)
				swap(ip,jp,member_size);	
			else
				break;
		}
	}
}
