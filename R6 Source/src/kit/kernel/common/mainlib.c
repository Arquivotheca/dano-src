/* ++++++++++
	Copyright (C) 1991 Be Labs, Inc.  All Rights Reserved

	Mainlib.  Main portion of Be Inc. runtime startup glue.

	Modification History (most recent first):
	09 jan 95	elr	brkval initvalue changed to come from area_for
	30 mar 94	elr	Added _exit() for malloc cleanup.
	16 mar 94	elr 	moved crap from crt0.s to here
	03 nov 93	elr	removed init_glue
	23 oct 92	elr	converted to mainlib
	16 dec 91	rwh	removed most glue - not needed now.I
	?? ??? 91	e?r	new today.
+++++ */

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include <be_rw_lock.h>
#include <priv_runtime.h>
#include <priv_syscalls.h>
#include <priv_kernel_export_data.h>
#include <OS.h>

char	**argv_save;
kernel_export_data_t	*kernel_export_data;

/*
 * First is the all-new, Cyril inspired library implementation of 
 * sbrk. 
 */

char		*brkval = NULL;

void		init_sbrk(void);
void		*_sbrk();
void		*(*sbrk_hook)() = &_sbrk;

static area_id		data_aid = -1;
static char			*heap_start;

void
__fork_sbrk_(void)
{
	/* POSIX Fork support -- the address layout for the new
	 * team is exactly the same as the parent, however, the ids
	 * change.. simply lookup the new data segment ID 
	 */

	data_aid = area_for(heap_start);
}

void
_init_sbrk_(void)
{
	heap_start = (char *) 0x80001000;
	data_aid = create_area("heap", (void **) &heap_start, B_BASE_ADDRESS, 2*B_PAGE_SIZE,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (data_aid < 0)
		exit(1);
	brkval = heap_start;
}

#if __GLIBC__
void *
__sbrk(ptrdiff_t incr)
{
	return (*sbrk_hook)(incr);
}
#pragma weak sbrk = __sbrk
#else /* __GLIBC__ */

void *
sbrk(long incr)
{
	return (*sbrk_hook)(incr);
}

#endif /* __GLIBC__ */ 

void *
_sbrk(int incr)
{
	char	*oldbrk, *curpgend, *newpgend;
	int		newsize;

	oldbrk = brkval;		/* save the original */
	if (incr == 0)			/* 0 incr means to */
		return(oldbrk);		/* return current break */

	brkval += incr;
	newpgend = (char *)(((int)brkval + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1));
	curpgend = (char *)(((int)oldbrk + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1));

	if (newpgend == curpgend)
		return(oldbrk);

	/* we need to add/remove pages... */
	/* first compute newsize */

	newsize = newpgend - heap_start;

	/* now try to resize the area */

	if (resize_area(data_aid, newsize) < B_NO_ERROR) {
		brkval -= incr;
		return ((char *)(-1));
	}

	return(oldbrk);
}

void
_init_kernel_export_data(void)
{
	if(_kmap_kernel_export_data_(&kernel_export_data) < B_OK) {
		debugger("_init_kernel_export_data failed\n");
		return;
	}
}

/* ----------
	_init_system_time_ sets up the time base to microsecond conversion factor
	for the library version of system_time.
---- */

void
_init_system_time_ (void)
{
	extern int32	*cv_factor_ptr;
	extern int64	*system_time_base_ptr;
	extern int64	*system_real_time_base_ptr;

	cv_factor_ptr = &kernel_export_data->cv_factor;
	system_time_base_ptr = &kernel_export_data->system_time_base;
	system_real_time_base_ptr = &kernel_export_data->system_real_time_base;
}

/*
 * #### THOSE FUNCTIONS ARE NOT LEGITIMATE AT USER LEVEL!!!
 */

void
acquire_spinlock (volatile long *addr)
{
	while (1) {
		while (*addr)
			;
		if (!atomic_or(addr, 1)) {
			break;
		}
	}
}

void
release_spinlock (volatile long *addr)
{
	*addr = 0;
}
