#ifndef _ATOMIC_H
#define _ATOMIC_H

#include <BeBuild.h>
#include <OS.h>

#if __cplusplus
	extern "C" {
#endif

#if !_NO_INLINE_ASM && __INTEL__
	inline int32 compare_and_swap32(volatile int32 *location, int32 oldValue, int32 newValue)
	{
		int32 success;
		asm volatile("lock; cmpxchg %%ecx, (%%edx); sete %%al; andl $1, %%eax"
			: "=a" (success) : "a" (oldValue), "c" (newValue), "d" (location));
		return success;
	}
	
#else
	int32 compare_and_swap32(volatile int32 *location, int32 oldValue, int32 newValue);
#endif

#if 0
	inline int32 compare_and_swap64(volatile int64 *location, int64 oldValue, int64 newValue)
	{
		int32 success;
		int dummy;
		asm volatile("lock; cmpxchg8b (%%edi); sete %%al; andl $1, %%eax"
			: "=a" (success) : "a" ((unsigned) oldValue), "b" ((unsigned) newValue),
				"c" ((unsigned)(newValue >> 32)), "d" ((unsigned)(oldValue >> 32)),
				"D" (location));
		return success;
	}
#else
	int32 compare_and_swap64(volatile int64 *location, int64 oldValue, int64 newValue);
#endif

inline bool cmpxchg32(volatile int32 *atom, int32 *value, int32 newValue)
{
	int32 success = compare_and_swap32(atom, *value, newValue);
	if (!success)
		*value = *atom;

	return success;
};

inline bool cmpxchg64(volatile int64 *atom, int64 *value, int64 newValue)
{
	int32 success = compare_and_swap64(atom, *value, newValue);
	if (!success)
		*value = *atom;

	return success;
};

typedef struct atomic_list_head {
	void* first;
	int32 sequence;
} atomic_list_head;

inline void atomic_push(atomic_list_head* list, void* item)
{
	while (1) {
		atomic_add(&(list->sequence), 1);
		atomic_list_head top = *list;
		*(void**)item = top.first;
		atomic_list_head next;
		next.first = item;
		next.sequence = top.sequence;
		if (compare_and_swap64((int64*)list, *(int64*)&top, *(int64*)&next)) {
			return;
		}
	}
}

inline void* atomic_pop(atomic_list_head* list)
{
	atomic_add(&(list->sequence), 1);
	atomic_list_head top = *list;
	
	while (top.first) {
		atomic_list_head next;
		next.first = *(void**)(top.first);
		next.sequence = top.sequence;
		if (compare_and_swap64((int64*)list, *(int64*)&top, *(int64*)&next)) {
			return top.first;
		}
		atomic_add(&(list->sequence), 1);
		top = *list;
	}
	
	return NULL;
}

#if __cplusplus
	}
#endif


#endif
