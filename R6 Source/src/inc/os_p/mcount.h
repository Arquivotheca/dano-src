#ifndef _MCOUNT_H
#define _MCOUNT_H

#include <OS.h>

/*
 * Possible states of profiling.
 */
#define	PROF_ON		0
#define	PROF_BUSY	1
#define	PROF_ERROR	2
#define	PROF_OFF	3

#define HASHBITS	2
#define HASHADDRESS(addr)	(((uint32) addr) >> HASHBITS)
#define UNHASHADDRESS(addr)	(((uint32) addr) << HASHBITS)

#define PROFILE_STACK_SIZE	1250

// If we change what we put on the stack, we must change kFrameSize
#define FRAMESIZE (sizeof(int32)*6)
 
struct func_table {				/* hash table */
	struct func_table *next;
	uint32		addr;
	uint32		count;
	uint32		lock;			/* only need to lock head of list */
	bigtime_t 	time;
	bigtime_t	total_elapsed;
};

typedef struct func_table func_table;

struct image_list {
	struct image_list *next;
	struct image_list *prev;
	char name[MAXPATHLEN]; 
	uint32 lowpc;
	uint32 highpc;
	uint32 textsize;
	struct func_table *func_table;
	uint32 *func_offsets;
	uint32 offsets_size;
	uint32 func_table_size;
};

struct profile_state {
	int32 state;
	struct image_list *images;
};


#endif /* _MCOUNT_H */
