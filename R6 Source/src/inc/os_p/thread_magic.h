/*	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved. */
#ifndef _THREAD_MAGIC_H
#define _THREAD_MAGIC_H
/*
 * Thread magic to implement "globals" like errno
 */

/* 
 * Size of the data per-thread, containing things like errno.
 * This is a fixed size, and you could run out of space. 
 * The current usage is around 100 bytes.
 */
#define THREAD_DATA_SIZE 1024

/*
 * Magic stuff put at the top of every thread stack,
 * for sanity checking and other useful information.
 */
static const struct {
	unsigned long magic;
	unsigned short total_size;
	unsigned short next_offset;
} magic_template = {
	'!TD!',
	THREAD_DATA_SIZE,
	0
};

#endif /* _THREAD_MAGIC_H */

