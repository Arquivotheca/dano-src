#ifndef __DEBUGGING_ALLOCATOR__
#define __DEBUGGING_ALLOCATOR__

// public API for the leakchecker calls

#pragma export on

#ifdef __cplusplus
extern "C" {
#endif


void _init_leak_checker();
void _cleanup_leak_checker();


bool NewLeakChecking();
void SetNewLeakChecking(bool);
void DumpNewLeakCheckTable(int32 compareLevel);
	// compareLevel is the number of staccrawl frames the leakchecker
	// uses to match up two distinct allocations
	// If you use 1, you will see say all calls to new BCheckBox in a single
	// table entry; If you use say 8 (10 is max), you will get a lot of different
	// table entries for new BCheckBox calls, each table entry for a different
	// path that leads to the allocation. Use the right value to zero in on
	// the depth you need
	// 4 is the default value used by the periodic dump call

bool MallocLeakChecking();
void SetMallocLeakChecking(bool);
void DumpMallocLeakCheckTable(int32 compareLevel);

void SetDefaultNewLeakCheckDumpPeriod(int32 period);
void SetDefaultMallocLeakCheckDumpPeriod(int32 period);
void SetDefaultNewLeakCheckSortBySize(bool on);
void SetDefaultMallocLeakCheckSortBySize(bool on);

struct malloc_state;

void record_new(void *, size_t);
void record_delete(void *);

void record_malloc(void *, size_t);
void record_free(void *);
void record_realloc(void *, void *, size_t);

void *unchecked_malloc(size_t);
void *unchecked_realloc(void *, size_t);
void unchecked_free(void *);

#ifdef __cplusplus
}
#endif

#pragma export reset


#endif
