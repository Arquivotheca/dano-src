//	Leak checking code
//	(c) 1997, Be Incorporated

#include <malloc.h>
#include <typeinfo>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>

#include "LeakChecking.h"
#include "LeakChecker.h"

// other files that implement leak checking:
// /boot/develop/be/exp/src/kit/kernel/common/gen_malloc.c
// /boot/develop/be/exp/srcx/cpp_new_except/new.cpp


MallocLeakCheckTable *mallocTable = 0;
Benaphore mallocInitLock("mallocLock");

NewLeakCheckTable *newTable = 0;
Benaphore newInitLock("newInitLock");

long outstanding_new_count = 0;
long outstanding_malloc_count = 0;
long new_count = 0;
long malloc_count = 0;
bool newLeakCheckingOn = false;
bool mallocLeakCheckingOn = false;
int32 mallocDumpPeriod;
int32 newDumpPeriod;
int32 mallocDefaultCompareLevel;
int32 newDefaultCompareLevel;
bool mallocSortBySize = false;
bool newSortBySize = false;


template<class T>
LeakCheckTable<T>::LeakCheckTable(long size)
	:	tableSize(LeakCheckTable<T>::MatchSize(size)),
		numEntries(0),
		numDeleted(0),
		creationTime(system_time()),
		lock("leakCheckTableLock")
{
	table = (T *)__malloc(tableSize * sizeof(T));
	for (long i = 0; i < tableSize; i++)
		table[i].key = 0;
}

template<class T>
bool 
LeakCheckTable<T>::Grow(bool flushDeletedOnly)
{
//	__print(("growing %s\n", flushDeletedOnly ? "flushing only" : ""));

	ASSERT(IsLocked());
	int32 newSize = flushDeletedOnly ? tableSize : MatchSize(tableSize + 1);
	if (!newSize) {
		__print(("can't grow hash table any further\n"));
		return false;
	}

	T *newTable = (T *)__malloc(newSize * sizeof(T));

	if (!newTable)
		return false;

	for (long i = 0; i < newSize; i++)
		newTable[i].key = 0;
	
	T *oldTable = table;
	int32 oldSize = tableSize;

	table = newTable;
	tableSize = newSize;

	for (int32 index = 0; index < oldSize; index++) {
		if (oldTable[index].Free())
			continue;
		if (oldTable[index].Deleted()) 
			continue;

		void *key = oldTable[index].Key();
		long hash = Hash(key, true);
		ASSERT(hash >= 0);
		ASSERT(table[hash].Free());
		table[hash] = oldTable[index];
		ASSERT(!table[hash].Deleted() && !table[hash].Free());
	}

	numEntries -= numDeleted;
		// just got rid of all the deleted entries
	numDeleted = 0;

	__free(oldTable);

	return true;
}

template<class T>
LeakCheckTable<T>::~LeakCheckTable()
{
	__free(table);
}

template<class T>
void *
LeakCheckTable<T>::operator new(size_t size)
{
	return __malloc(size);
}

template<class T>
void 
LeakCheckTable<T>::operator delete(void *p)
{
	if (p) 
		__free(p);
}

template<class T>
void *
LeakCheckTable<T>::__malloc(size_t size)
{
	// point these to internal malloc/free calls so we don't
	// leakcheck the leakchecker
	return unchecked_malloc(size);
}

template<class T>
void 
LeakCheckTable<T>::__free(void *p)
{
	unchecked_free(p);
}

#define CHECK_UNACCOUNTED_ALLOCATIONS

template<class T>
LeakCheckTable<T>::ProbeResult
LeakCheckTable<T>::Probe(void *key, ulong index, bool inserting)
{
	if (inserting) {
		if (table[index].Free() || table[index].Deleted())
			return kFound;

		if (table[index].Match(key)){
#ifdef CHECK_UNACCOUNTED_ALLOCATIONS
			__print(("original call address:"));
			table[index].Dump();
			__print((" inserting %x - pointer already found - "
				"probably new freed by free or malloc freed by delete\n", key));
#endif
			return kFound;
		}
	} else {
		if (table[index].Match(key))
			return kFound;

		if (table[index].Free())
			return kNotFound;
	}
	
	return kKeepProbing;
}

template<class T>
long
LeakCheckTable<T>::Hash(void *key, bool inserting)
{
	ulong subkey = ((ulong)key >> 2);
		// strip of pointer insignificant junk
	ulong index = subkey % tableSize;

	ProbeResult result = Probe(key, index, inserting);
	if (result == kFound)
		return index;
	else if (result == kNotFound)
		return -1;

	// use double hashing for second and subsequent probes
	ulong secondHash = subkey % (tableSize - 2);

	if (inserting)
		for (;;) {
			index = (index + 1 + secondHash);
			if (index >= tableSize)	// faster than %
				index -= tableSize;
			
			result = Probe(key, index, true);
			if (result == kFound)
				return index;

			ASSERT(result != kNotFound);
		}
	else
		for (;;) {
			index = (index + 1 + secondHash);
			if (index >= tableSize)	// faster than %
				index -= tableSize;
			
			result = Probe(key, index, false);
			if (result == kFound)
				return index;
			else if (result == kNotFound)
				return -1;

		}

	return kNotFound;
}

template<class T>
long 
LeakCheckTable<T>::MatchSize(long equalOrMoreThan)
{
	// the bigger of the twins is used for table size
	equalOrMoreThan -= 2;
	for (int32 index = 0; ; index++)
		if (!primeTwins[index]
			|| primeTwins[index] >= equalOrMoreThan)
			return primeTwins[index] + 2;

	TRESPASS();
	return 0;
}

template<class T>
T *
LeakCheckTable<T>::AllocatingCommon(void *ptr)
{
	ASSERT(IsLocked());

	if (numEntries == tableSize) {
		TRESPASS();
		// table full, shouldn't be here
		return 0;
	}

	long hash = Hash(ptr, true);
	if (hash == -1) {
		TRESPASS();
		// table full, shouldn't be here
		return 0;
	}

	if (table[hash].Deleted())
		numDeleted--;
	else
		numEntries++;
	
	if (numEntries >= (tableSize - tableSize / 5)) {
		bool result;
		if (tableSize < 10000 || numDeleted < (tableSize / 3))
			result = Grow(false);
		else
			// make sure we have a good load factor, rehash if too many
			// deleted entries fill up the table
			result = Grow(true);			

		if (!result) {
			__print((" ran out of malloc leak check table space \n"));
			return 0;
		} else {
			// have to rehash, table just changed
			hash = Hash(ptr, true);
			ASSERT(hash >= 1);
		}
	}	
	return &table[hash];
}

template<class T>
void
LeakCheckTable<T>::Freeing(void *ptr, ulong pc)
{
	long hash = Hash(ptr, false);
	if (hash == -1) {
#ifdef CHECK_UNACCOUNTED_ALLOCATIONS
		__print(("freeing %x from 0x%x not found -"
			"probably new freed by free or malloc freed by delete\n", ptr, pc));
#endif
		return;
	}

	table[hash].SetDeleted();
	numDeleted++;
}

template<class T>
void
LeakCheckTable<T>::Dump(int32)
{
	TRESPASS();
}

template<class T>
void
LeakCheckTable<T>::Allocating(void *ptr, long size)
{
	T *result = AllocatingCommon(ptr);
	if (!result) 
		return;

	result->key = ptr;
	result->size = size;
	GetCallerAddress(-1, result->StackDepth(), result->MaxStackDepth(), result->pc);
}

MallocLeakCheckTable *
MallocLeakCheckTable::Table()
{
	mallocInitLock.Lock();
	
	if (!mallocTable) 
		mallocTable = new MallocLeakCheckTable(2000);

	mallocInitLock.Unlock();
	return mallocTable;
}

NewLeakCheckTable *
NewLeakCheckTable::Table()
{
	newInitLock.Lock();
	
	if (!newTable) 
		newTable = new NewLeakCheckTable(2000);

	newInitLock.Unlock();
	return newTable;
}

class DumpElement {
public:

	DumpElement(const TableEntry *entry)
		:	pcSize(entry->StackDepth()),
			count(1),
			totalSize(entry->size)
		{
			for (int32 index = 0; index < entry->MaxStackDepth(); index ++)
				pc[index] = entry->pc[index];
		}
		
	int Compare(const TableEntry *entry, int32 compareDepth) const
		{
			for (int32 index = 0; index < compareDepth; index ++)
				if (pc[index] < entry->pc[index])
					return -1;
				else if (pc[index] > entry->pc[index])
					return 1;

			return 0;
		}

	// override new/free so we don't leakcheck ourselves
	void *operator new(size_t size)
		{ return unchecked_malloc(size); }

	void operator delete(void *p)
		{
			if (p)
				unchecked_free(p);
		}

	ulong pc[kMaxStackDepth];
	long pcSize;
	long count;
	long totalSize;
};

int SortByCount(DumpElement *const *, DumpElement *const *);
int SortBySize(DumpElement *const *, DumpElement *const *);
const DumpElement *const DumpOne(DumpElement *const , void *);

template<class T> 
class DumpList : public TList<DumpElement *> {
public:
	DumpList()
		:	TList<DumpElement *>(100, true)
		{}

	void Insert(T *, int32 compareDepth);
	void Dump(long minCount, bool sortBySize);
	void SortItems(int (*)(DumpElement *const *, DumpElement *const *));

	// override new/free so we don't leakcheck ourselves
	void *operator new(size_t size)
		{ return unchecked_malloc(size); }

	void operator delete(void *p)
		{
			if (p)
				unchecked_free(p);
		}
};

template<class T> 
void
DumpList<T>::Insert(T *insertedElement, int32 compareDepth)
{
	ulong count = CountItems();
	ulong index = 0;

	// ToDo:
	// use binary sort here instead
	for (; index < count; index ++) {
		DumpElement *element = ItemAt(index);
		int result = element->Compare(insertedElement, compareDepth);
		if (result == 0) {
			element->count++;
			element->totalSize += insertedElement->size;
			return;
		}
		if (result > 0) {
			AddItem(new DumpElement(insertedElement), index);
			return;
		}
	}
	AddItem(new DumpElement(insertedElement), index);
}

int
SortByCount(DumpElement *const *elem1, DumpElement *const *elem2)
{
	if ((*elem1)->count > (*elem2)->count)
		return 1;
	else if ((*elem1)->count == (*elem2)->count)
		return 0;
	else
		return -1;
}

int
SortBySize(DumpElement *const *elem1, DumpElement *const *elem2)
{
	if ((*elem1)->totalSize > (*elem2)->totalSize)
		return 1;
	else if ((*elem1)->totalSize == (*elem2)->totalSize)
		return 0;
	else
		return -1;
}

template<class T> 
void
DumpList<T>::SortItems(int (*compare)(DumpElement *const *, DumpElement *const *))
{
	// this is the lamest sort ever
	ulong count = CountItems();
	
	for (ulong outerIndex = 0; outerIndex < count; outerIndex ++) {
		bool swapped = false;
		for (ulong index = 0; index < count - 1; index ++) {
			DumpElement *element1 = ItemAt(index);
			DumpElement *element2 = ItemAt(index + 1);
			if (compare(&element1, &element2) == -1) {
				DumpElement tmp = *element1;
				*ItemAt(index) = *element2;
				*ItemAt(index + 1) = tmp;
				swapped = true;
			}
		}
		if (!swapped)
			return;
	}
}

const DumpElement *const
DumpOne(DumpElement *const element, void *castToCount)
{
	long count = (long) castToCount;
	if (element && element->count >= count) {
	
		__print(("call: "));
		for (int32 index = 0; index < element->pcSize; index++) {
			if (element->pc[index] == 0)
				break;
			__print(("%x, ", element->pc[index]));
		}
		__print(("%d calls, size %d\n", element->count, element->totalSize));
	}
	return 0;
}

template<class T> 
void
DumpList<T>::Dump(long minCount, bool sortBySize)
{
	SortItems(sortBySize ? SortBySize : SortByCount);
	for (int32 index = 0; index < 20; index++)
		DumpOne(ItemAt(index), (void *)minCount);
}

void
NewLeakCheckTable::Dump(int32 compareDepth)
{
	Lock();
	long entries = Entries();
	long deletedEntries = DeletedEntries();
	int32 currentTableSize = tableSize;

	// nothing inside this critical section is allowed to call new/delete
	// otherwise we will cause a deadlock
	// everyone has to use overriden new/delete

	long totalSize = 0;
	DumpList<TableEntry> dumpList;
	for (long i = tableSize - 1; i >= 0; i--)
		if (!table[i].Deleted() && !table[i].Free()) {
			totalSize += table[i].size;
			dumpList.Insert(&table[i], compareDepth);
		}

	// we have a copy of all the stats in our dump list now, we can unlock
	Unlock();

	__print(("%d current new allocations, %d bytes, sorted by %s: ------------\n",
		outstanding_new_count, totalSize, newSortBySize ? "size" : "count"));
	dumpList.Dump(1, newSortBySize);
}

void
MallocLeakCheckTable::Dump(int32 compareDepth)
{
	Lock();
	int32 entries = Entries();
	int32 deletedEntries = DeletedEntries();
	int32 currentTableSize = tableSize;
	// nothing inside this critical section is allowed to call malloc/free
	// otherwise we will cause a deadlock

	long totalSize = 0;
	DumpList<TableEntry> dumpList;
	for (long i = tableSize - 1; i >= 0; i--)
		if (!table[i].Deleted() && !table[i].Free()) {
			totalSize += table[i].size;
			dumpList.Insert(&table[i], compareDepth);
		}

	// we have a copy of all the stats in our dump list now, we can unlock
	Unlock();

	__print(("%d current malloc allocations, %d bytes, sorted by %s: ============\n",
		(int32)outstanding_malloc_count, totalSize, mallocSortBySize ? "size" : "count"));
	dumpList.Dump(1, mallocSortBySize);
}


void 
record_new(void *p, size_t size)
{
	NewLeakCheckTable *table = NewLeakCheckTable::Table();
	ASSERT(table);
	table->Lock();
	new_count++;
	outstanding_new_count++;
	table->Allocating(p, size);
	table->Unlock();

	if (newDumpPeriod != 0 && (new_count % newDumpPeriod) == 0) 
		DumpNewLeakCheckTable(newDefaultCompareLevel);
}

void 
record_delete(void *p)
{
	NewLeakCheckTable *table = NewLeakCheckTable::Table();
	table->Lock();
	outstanding_new_count--;
	table->Freeing(p, GetCallerAddress(1));
	table->Unlock();
}

void 
record_malloc(void *p, size_t size)
{
	MallocLeakCheckTable *table = MallocLeakCheckTable::Table();
	ASSERT(table);
	table->Lock();
	malloc_count++;
	outstanding_malloc_count++;
	ASSERT(table->IsLocked());
	table->Allocating(p, size);

	table->Unlock();

	if (mallocDumpPeriod != 0 && (malloc_count % mallocDumpPeriod) == 0) 
		DumpMallocLeakCheckTable(mallocDefaultCompareLevel);
}

void 
record_realloc(void *result, void *original, size_t size)
{
	malloc_count++;
	
	MallocLeakCheckTable *table = MallocLeakCheckTable::Table();
	ASSERT(table);
	table->Lock();
	ASSERT(table->IsLocked());
	if (original)
		table->Freeing(original, GetCallerAddress(1));
	else
		// realloc can be called on a null original, in which case
		// we need to treat it like a malloc
		outstanding_malloc_count++;
		
	table->Allocating(result, size);

	table->Unlock();
}


void 
record_free(void *p)
{
	MallocLeakCheckTable *table = MallocLeakCheckTable::Table();
	table->Lock();

	outstanding_malloc_count--;
	ASSERT(table->IsLocked());
	table->Freeing(p, GetCallerAddress(1));
	table->Unlock();
}

bool 
NewLeakChecking()
{
	return newLeakCheckingOn;
}

void 
SetNewLeakChecking(bool on)
{
//	if (on)
//		__print(("turning new leak checking on\n"));
	newLeakCheckingOn = on;
}

void 
DumpNewLeakCheckTable(int32 compareLevel)
{
	if (compareLevel > kStackDepth)
		compareLevel = kStackDepth;

	NewLeakCheckTable *table = NewLeakCheckTable::Table();
	if (!table)
		return;
	table->Dump(compareLevel);
}

bool 
MallocLeakChecking()
{
	return mallocLeakCheckingOn;
}

void 
SetMallocLeakChecking(bool on)
{
//	if (on)
//		__print(("turning malloc leak checking on\n"));
	mallocLeakCheckingOn = on;
}

void 
SetDefaultNewLeakCheckDumpPeriod(int32 period)
{
	newDumpPeriod = period;
	if (period < 0)
		period = 0;
}

void 
SetDefaultMallocLeakCheckDumpPeriod(int32 period)
{
	mallocDumpPeriod = period;
	if (period < 0)
		period = 0;
}

void 
SetDefaultNewLeakCheckSortBySize(bool on)
{
	newSortBySize = on;
}

void 
SetDefaultMallocLeakCheckSortBySize(bool on)
{
	mallocSortBySize = on;
}


void 
DumpMallocLeakCheckTable(int32 compareLevel)
{
	if (compareLevel > kStackDepth)
		compareLevel = kStackDepth;

	MallocLeakCheckTable *table = MallocLeakCheckTable::Table();
	if (!table)
		return;
	table->Dump(compareLevel);
}

void
_init_leak_checker()
{
	outstanding_new_count = 0;
	outstanding_malloc_count = 0;
	new_count = 0;
	malloc_count = 0;
	newLeakCheckingOn = false;
	mallocLeakCheckingOn = false;
	mallocDumpPeriod = 2000;
	newDumpPeriod = 2000;
	mallocDefaultCompareLevel = 4;
	newDefaultCompareLevel = 4;
	mallocSortBySize = false;
	newSortBySize = false;

    bool mallocOn = getenv("MALLOC_LEAK_CHECK") != NULL;
	SetMallocLeakChecking(mallocOn);

    bool newOn = getenv("NEW_LEAK_CHECK") != NULL;
	SetNewLeakChecking(newOn);
	
	const char *tmp = getenv("MALLOC_LEAK_CHECK_DUMP_PERIOD");
	if (tmp) {
		mallocDumpPeriod = atoi(tmp);
		if (mallocDumpPeriod < 0)
			mallocDumpPeriod = 0;
	}	

	tmp = getenv("NEW_LEAK_CHECK_DUMP_PERIOD");
	if (tmp) {
		newDumpPeriod = atoi(tmp);
		if (newDumpPeriod < 0)
			newDumpPeriod = 0;
	}
	
	tmp = getenv("MALLOC_LEAK_CHECK_COMPARE_LEVEL");
	if (tmp) {
		mallocDefaultCompareLevel = atoi(tmp);
		if (mallocDefaultCompareLevel < 0)
			mallocDefaultCompareLevel = 4;
		if (mallocDefaultCompareLevel > kMaxStackDepth)
			mallocDefaultCompareLevel = 4;
	}	

	newDefaultCompareLevel = 4;
	tmp = getenv("NEW_LEAK_CHECK_COMPARE_LEVEL");
	if (tmp) {
		newDefaultCompareLevel = atoi(tmp);
		if (newDefaultCompareLevel < 0)
			newDefaultCompareLevel = 4;
		if (newDefaultCompareLevel > kMaxStackDepth)
			newDefaultCompareLevel = 4;
	}

	mallocSortBySize = getenv("MALLOC_LEAK_CHECK_SORT_BY_SIZE") != 0;
	newSortBySize = getenv("NEW_LEAK_CHECK_SORT_BY_SIZE") != 0;
}

void 
_cleanup_leak_checker()
{
	if (MallocLeakChecking() || NewLeakChecking())
		__print(("app exiting, final list: \n"));

	if (MallocLeakChecking())
		DumpMallocLeakCheckTable(mallocDefaultCompareLevel);
	if (NewLeakChecking())
		DumpNewLeakCheckTable(newDefaultCompareLevel);
}


#if __POWERPC__
__asm ulong * get_caller_frame();

__asm ulong *
get_caller_frame ()
{
	lwz     r3, 0 (r1)
	blr
}

#endif

ulong
GetCallerAddress(int level)
{
#if __INTEL__
	// skip past the leakchecking layers
	level += 3;
	
	ulong fp = 0, nfp, ret;
	fp = (ulong)get_stack_frame();
	nfp = *(ulong *)fp;
	while (nfp && --level > 0) {
		nfp = *(ulong *)fp;
		ret = *(ulong *)(fp + 4);
		if (ret < 0x80000000 || ret > 0xfc000000 || ret == 0)
			break;
		fp = nfp;
	}

	return ret;
#else
	ulong *cf = get_caller_frame();
	ulong result = 0;
	level += 3;
	
	for (; cf && --level > 0; ) {
		result = cf[2];
		if (result < 0x80000000 || result > 0xfc000000)
			break;
		cf = (ulong *)*cf;
	}

	return result;
#endif
}

void
GetCallerAddress(int level, int depth, int size, ulong *addresses)
{
	for (int i = 0; i < size; i++)
		addresses[i] = 0;

	// skip past the leakchecking layers
	level += 3;
#if __INTEL__
	ulong fp = (ulong)get_stack_frame();
	ulong nfp, result;

	for (int index = -level; index < depth; index++) {
		nfp = *(ulong *)fp;
		if (!nfp)
			return;
		result = *(ulong *)(fp + 4);
		if (result < 0x80000000 /*|| result > 0xfc000000 */|| result == 0)
			return;
		if (index >= 0)
			addresses[index] = result;
		fp = nfp;
	}

#else
	ulong *fp = get_caller_frame();
	ulong result = 0;
	
	for (int index = -level; index < depth; index++) {
		result = fp[2];
		if (result < 0x80000000 || result > 0xfc000000)
			break;

		if (index >= 0)
			addresses[index] = result;

		fp = (ulong *)*fp;
		if (!fp)
			break;
	}

#endif
}


_PointerList::_PointerList(const _PointerList &list)
	:	owning(list.owning)
{
	fItemCount = list.fItemCount;
	fBlockSize = list.fBlockSize;
	fPhysicalSize = list.fPhysicalSize;
	fObjectList = (void**)unchecked_malloc(fPhysicalSize);
	memcpy(fObjectList, list.fObjectList, fPhysicalSize);
}

_PointerList::_PointerList(long itemsPerBlock = 20, bool owningList)
	:	owning(owningList)
{
	fItemCount = 0;
	if (itemsPerBlock > 0)
		fBlockSize = itemsPerBlock;
	else
		fBlockSize = 20;
	fPhysicalSize = fBlockSize * sizeof(void*);
	fObjectList = (void**)unchecked_malloc(fPhysicalSize);
}

_PointerList &_PointerList::operator=(const _PointerList& list)
{
	if (this != &list) {
		owning = list.owning;
		unchecked_free(fObjectList);
		fItemCount = list.fItemCount;
		fBlockSize = list.fBlockSize;
		fPhysicalSize = list.fPhysicalSize;
		fObjectList = (void**)unchecked_malloc(fPhysicalSize);
		memcpy(fObjectList, list.fObjectList, fPhysicalSize);
	}
	return *this;
}

_PointerList::~_PointerList()
{
	unchecked_free(fObjectList);
}

bool _PointerList::AddItem(void* item, int32 index)
{
	if (index < 0 || index > fItemCount)
		return(FALSE);

	if (fPhysicalSize < (fItemCount + 1) * sizeof(void*))
		Resize(fBlockSize);

	// make room for new object
	if (index != fItemCount)
		memmove(fObjectList + index + 1,		// dest
	        	fObjectList + index,			// src
	        	(fItemCount - index) * sizeof(void*));	// count

	fObjectList[index] = item;
	fItemCount++;
	return(TRUE);
}

bool _PointerList::RemoveItem(void* item)
{
	return(RemoveItem(IndexOf(item)) != NULL);
}

void* _PointerList::RemoveItem(int32 index)
{
	void* item;

	if (index < 0 || index >= fItemCount)
		return(NULL);

	item = fObjectList[index];
	fItemCount--;

	// compact list to get rid of 'Removed' item
	memcpy(fObjectList + index,			// dest
	       fObjectList + index + 1,			// src
	       (fItemCount - index) * sizeof(void*));	// count

	// resize list if possible
	if ((fPhysicalSize - (fBlockSize * sizeof(void*))) > (fItemCount * sizeof(void*)))
		Resize(-fBlockSize);

	return(item);
}

void _PointerList::MakeEmpty()
{
	if (fPhysicalSize > (sizeof(void*) * fBlockSize)) {
		fPhysicalSize = sizeof(void*) * fBlockSize;
		unchecked_free(fObjectList);
		fObjectList = (void**)unchecked_malloc(fPhysicalSize);
	}

	fItemCount = 0;
}

int32 _PointerList::IndexOf(void* item) const
{
	int32	i;
	void	**ptr;

	i = fItemCount;
	ptr = fObjectList;
	while(i--) {
		if (*ptr++ == item)
			return(fItemCount - i - 1);
	}
	return(-1);			
}

void _PointerList::Resize(int32 by_count)
{
	if (by_count > 0)
		fPhysicalSize += (by_count * sizeof(void*));
	else
		fPhysicalSize -= (by_count * sizeof(void*));

	fObjectList = (void**)unchecked_realloc(fObjectList, fPhysicalSize);
}


void		*_PointerList::Items() const
			{ return(fObjectList); }

void		*_PointerList::ItemAt(int32 indx) const
			{	if (indx < 0 || indx >= fItemCount)
					return(NULL);
				else
					return(fObjectList[indx]);
			}

bool		_PointerList::AddItem(void* item)
			{ return(AddItem(item, fItemCount)); }

int32		_PointerList::CountItems() const
			{ return(fItemCount); }

void 		*_PointerList::FirstItem()	 const
			{
				if (fItemCount == 0)
					return(NULL);
				else
					return(fObjectList[0]);
			}

void 		*_PointerList::LastItem() 	 const
			{
				if (fItemCount == 0)
					return(NULL);
				else
					return(fObjectList[fItemCount - 1]);
			}

bool		_PointerList::IsEmpty() const
			{ return(fItemCount == 0); }

bool		_PointerList::HasItem(void* item) const
			{ return(IndexOf(item) != -1); }

