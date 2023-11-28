#ifndef _LOCKED_ALLOCATOR_H
#define _LOCKED_ALLOCATOR_H

#include <OS.h>
#include <rt_alloc.h>
#include <stdio.h>

class realtime_blocks_32k {
public:
	enum { NUM_BLOCKS = 32*1024 };
};

extern rtm_pool * _rtm_pool;

template <size_t ElementSize>
class locked_allocator {
public:
	static void *allocate(size_t size)
		{
			void * ret = rtm_alloc(_rtm_pool, size);
			if (ret == NULL) {
				fprintf(stderr, "locked_allocator: alloc(%ld) fails\n", size);
			}
			return ret;
		}

	static void deallocate(void *ptr, size_t /*size*/)
		{
			(void)rtm_free(ptr);
		}

};


#endif
