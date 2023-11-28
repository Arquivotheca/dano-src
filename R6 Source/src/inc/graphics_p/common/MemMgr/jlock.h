#ifndef __JLOCK_H__
#define __JLOCK_H__

#include <OS.h>

typedef uint32 jlock;

/*
	Here is the magic jlock which will not require a kernel call but is still
	safe.  The jlock can has only two state locked and unlocked.  The internal
	value is 0 for unlocked and id (assigned by the kernel driver) when locked.
	This allows the kernel driver to clean these up by unlocking any locks which
	contain a value other than those known to the driver.  
*/

static inline void jlock_lock( jlock *lock, uint32 id )
{
	uint32 ret;
	__asm__ __volatile__ (
		"xorl %%eax, %%eax \n"
		"cmpxchgl %%ecx, (%%edx) \n"
		: "=a"(ret) : "d"(lock), "c"(id) );
	while( ret )
	{
		snooze(1);
		__asm__ __volatile__ (
			"xorl %%eax, %%eax \n"
			"cmpxchgl %%ecx, (%%edx) \n"
			: "=a"(ret) : "d"(lock), "c"(id) );
	}
}

static inline void jlock_unlock( jlock *lock )
{
	lock[0] = 0;
}

static inline void jlock_init( jlock *lock )
{
	lock[0] = 0;
}

static inline uint32 jlock_getID( jlock *lock )
{
	return lock[0];
}





#endif
