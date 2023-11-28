/*
  File:         idstr0.c          @(#)idstr0.c	13.8 06/08/95
 *
 * This file provides just the minimal interface to the fut idstrings:
 *
 *	fut_set_idstr - set (or clear) the idstring of a fut
 *	fut_idstr - return a pointer to the idstring of a fut
 *	fut_alloc_idstr - allocate double even sized memory for idstring
 *	fut_free_idstr - free an idstring
 *
 * These functions are the only ones used internally  by the fut library.
 * Most of the other idstring functions may be found in fut_idstr.c.  The
 * separation is done so that if the idstring is not used extensively by an
 * application, a lot of unnecessary code is not pulled in from the library.
 */

/***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1995 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/

#include <string.h>
#include "fut.h"
#include "fut_util.h"

#if defined(KPWIN16)
#define strlen(s)	_fstrlen(s)
#endif


/*
 * fut_alloc_idstr allocates a new idstring of at least nbytes bytes.
 * The resulting allocation is always double even in size and is
 * null terminated.  If any extra padding is added, it is filled with
 * newlines.  (The remainder of the memory is left uninitialized).
 *
 * This function is used only internally by the library.
 */
char fut_far*
fut_alloc_idstr (int32 nbytes)
{
int32		size, last;
char fut_far	*p;

	size = FUT_DOUBLE_EVEN(nbytes);
	p = (char fut_far*)fut_malloc (size, "m");

	if ( p == NULL ) {
		return (NULL);
	}

				/* fill padding with newlines */
	for (last = nbytes-1; last < size-1; last++) {
		p[last] = KP_NEWLINE;
	}

				/* null terminate */
	p[last] = '\0';

	return (p);
}


void
fut_free_idstr (char fut_far *idstr)
{
	fut_mfree (idstr, "h");
}


/*
 * fut_new_idstr sets the id string of a fut, allocating a new string
 * and performing a copy.  Any existing string is freed first.
 */
int
fut_new_idstr (fut_ptr_t fut, char fut_far * s)
{
int	len, i;

	if( !IS_FUT(fut) )
		return(0);

	if ( fut->idstr != NULL ) {
		fut_free_idstr (fut->idstr);
		fut->idstr = NULL;
	}

	if ( s != NULL ) {
		len = (int)strlen(s);		/* get the length of the id string */
		for  (i = 0; i < len; i++) {
			if (s[i] == KP_NEWLINE) {
				return (0);
			}
		}
		
		/* allocate a new string */
		/* +1 for KP_NEWLINE */
		/* +1 for null terminator */
		fut->idstr = fut_alloc_idstr (len + 2);
		if ( fut->idstr == NULL ) {
			return (0);
		}
		
	/* copy text making sure not to cover the KP_NEWLINE */
		bcopy (  (fut_generic_ptr_t)s,
			(fut_generic_ptr_t)fut->idstr,
			(int32)len );

	/* insert the '\n' */
		fut->idstr[len] = KP_NEWLINE;
		
	/* insert the null terminator */
		fut->idstr[len+1] = '\0';
	}

	return (1);

} /* fut_set_idstr */


/*
 * fut_set_idstr sets the id string of a fut, allocating a new string
 * and performing a copy.  Any existing string is freed first.
 */
int
fut_set_idstr (fut_ptr_t fut, char fut_far * s)
{
int	len;

	if( !IS_FUT(fut) )
		return(0);

	if ( fut->idstr != NULL ) {
		fut_free_idstr (fut->idstr);
		fut->idstr = NULL;
	}

	if ( s != NULL ) {
				/* allocate a new string */
		len = (int)strlen(s) +1;		/* +1 for null terminator */

		fut->idstr = fut_alloc_idstr (len);
		if ( fut->idstr == NULL ) {
			return (0);
		}
			/* copy text to it */
		bcopy (  (fut_generic_ptr_t)s,
			(fut_generic_ptr_t)fut->idstr,
			(int32)len );
	}

	return (1);

} /* fut_set_idstr */


/*
 * fut_idstr() rturns a pointer to the id string of a a fut,
 * or NULL if none exists.
 */
char fut_far*
fut_idstr (fut_ptr_t fut)
{
	if( !IS_FUT(fut) )
		return((char fut_far*)NULL);

	return ( fut->idstr );

} /* fut_idstr */

