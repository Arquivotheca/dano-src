/*
	File: osi_mem.c		@(#)osi_mem.c	13.12 10/28/94
	
***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1994 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/

/* This file defines Operating System Independent memory
 * handling routines used internally by the fut library.
 * Currently, this covers:
 *		1. Sun with Shared memory (for table data)
 *
 *		2. All others
 *						********************
 * The fut library should NOT use any of the standard memory routines
 * listed in stdlib.h.	Instead it must use fut_malloc and fut_mfree
 * for *all* allocations.
 *
 * The following five functions must be written for each system:
 *
 *		fut_generic_ptr_t fut_malloc (int32 size, char* type [, fut_generic_ptr_t ptr]);
 *		void			fut_mfree (fut_generic_ptr_t ptr, char* type);
 *
 *				"m" = normal (like malloc)
 *				"c" = clear memory (like calloc)
 *				"r" = reallocate (like realloc) - 3rd argument is required!!!
 *				"h" = memory with handles (for most fut structures)
 *				"i" = input table data array (may be shared)
 *				"o" = output table data array (may be shared)
 *				"g" = grid table data array (may be shared and/or huge)
 *
 ************************************************************
 */

#include "fut.h"
#include "fut_util.h"			/* internal interface file */


/*	Note that the underlying WINDOWS code assumes that
 *	WINDOWS is not in REAL mode!!  */


#if !(defined (FUT_SHMEM) && defined (KPUNIX))

	/*********** Allocate Memory ***************/
fut_generic_ptr_t
	fut_malloc (int32 size, char_p type, ...)
{
KCMS_VA_ARG_PTR			ap;
fut_generic_ptr_t		ptr=(fut_generic_ptr_t)NULL, oldptr;
char_p	array;		/* required by KCMS_VA_xxx */
int32 oldsize;


	switch (*type) {
	case 'm':
	case 'c':
	case 'h':
	case 'i':
	case 'o':
	case 'g':
		ptr = (fut_generic_ptr_t) allocBufferPtr(size);
		break;

/*	Fake out reallocate since it is only used in the
 *	ID string stuff, which is not used for KCMS */
	case 'r':
		KCMS_VA_START(ap,array,type);
		oldptr = KCMS_VA_ARG (ap, array, fut_generic_ptr_t);
		KCMS_VA_END(ap,array);
		oldsize = getPtrSize(oldptr);
		ptr = (fut_generic_ptr_t) allocBufferPtr(size);
		if( ptr	!=	(fut_generic_ptr_t)NULL ) {
			bcopy(oldptr,ptr,oldsize);
			freeBufferPtr(oldptr);
		}
		break;

	}

	if ( ptr != (fut_generic_ptr_t)0
		&& ((type[0] == 'c') || (type[1] == 'c' ))) {
		bzero ((fut_generic_ptr_t)ptr, size);
	}
	return (ptr);
}


	/*********** Free Memory ***************/
void
	fut_mfree (fut_generic_ptr_t ptr, char_p type)
{
	if (type) {} 				/* avoid compiler warning */
	
	if ( ptr == (fut_generic_ptr_t)0 ) {
		return;
	} else {
		freeBufferPtr(ptr);
	}
	
	return;
}

#endif /* not Sun shared memory */



	/************** SYS V shared memory *********************/
#if defined (FUT_SHMEM) && defined (KPUNIX)

/* When SYS V shared memory is turned on, we use malloc and free
 * calls in the shmem library to implement fut_malloc() and fut_mfree().
 * for input, output, and grid table arrays.
 * Depending on whether shared memory is enabled in the kernel, resulting
 * allocation is sharable.
 * All other types use normal malloc and free.
 *
 * Note that the value of FUT_SHMEM is the mode for permissions.
 * If -DFUT_SHMEM is used in build instead of -DFUT_SHMEM=mode, value
 * of FUT_SHMEM will be 1 which causes 0664 to be used.
 */
#if FUT_SHMEM <= 1
#undef FUT_SHMEM
#define FUT_SHMEM 0664
#endif

#include "shmem.h"
#include <malloc.h>

fut_generic_ptr_t	fut_malloc (int32 size, char_p	type, ...)
{
	fut_generic_ptr_t ptr;

	switch (*type) {
	case 'c':
		ptr = (fut_generic_ptr_t) calloc (size, 1);
		break;

	case 'r': {
		KCMS_VA_ARG_PTR		ap;	/* arg pointer */
		char_p	vap;	/* for FUT_VARARGS */
		fut_generic_ptr_t	oldptr;

		KCMS_VA_START(ap, vap, type);
		oldptr = KCMS_VA_ARG(ap, vap, fut_generic_ptr_t);
		KCMS_VA_END(ap, vap);

		ptr = (fut_generic_ptr_t) realloc ((char *)oldptr, size);
		}
		break;

	case 'i':
	case 'o':
	case 'g':
		ptr =	(fut_generic_ptr_t) shmalloc (size, FUT_SHMEM);
		break;

	default:	/* 'm', 'h' */
		if ( type[1] == 'c' ) {
			ptr = (fut_generic_ptr_t) calloc (size, 1);
		} else {
			ptr = (fut_generic_ptr_t) malloc (size);
		}
		return (ptr);	/* don't bother clearing below */
	}

	if ( ptr != (fut_generic_ptr_t)0
		&& ((type[0] == 'c') || (type[1] == 'c' ))) {
		bzero ((fut_generic_ptr_t)ptr, size);
	}
	return (ptr);
}

void
fut_mfree (ptr, type)
fut_generic_ptr_t ptr;
char	* type;
{
	if ( ptr == (fut_generic_ptr_t)0 )
		return;

	switch (*type) {
		case 'i':
		case 'o':
		case 'g':
		shmfree ((char *)ptr);
		break;
		default:
		(void) free ((char *)ptr);
		break;
	}
	return;
}

#endif	/* defined (FUT_SHMEM) && defined (KPUNIX) */

