/*

   File:       util.c	@(#)util.c	13.14 10/22/97

   Contains:	Utility routines for fut library.
			 	This module defines global data arrays and structures.

   Written by: Drivin' Team

   Copyright:  (c) 1991-1997 by Eastman Kodak Company, all rights reserved.

   Change History (most recent first):

   To Do:
*/
/***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1997 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/

#define	KCM_COPYRIGHT 15	/* (string) Copyright information */
 
#include <string.h>

#define FUT_ALLOCEXTERN
#include "fut.h"		/* external interface file */
#include "fut_util.h"		/* internal interface file */
#include "interp.h"		/* interface file for interpolators */
#undef FUT_ALLOCEXTERN

#if defined(KPWIN16)
#define strlen(s)			_fstrlen(s)
#define strcpy(s1,s2)	_fstrcpy(s1,s2)
#define strcat(s1,s2)	_fstrcat(s1,s2)
#endif

/*
 * fut_iomask_check checks a fut_eval iomask for consistency with
 * a given fut, returning a (possibly) modified iomask or 0 if some
 * inconsistency was found.  Consistency checks are as follows:
 *
 *   1. channels specified in the "pass-through" mask are 'or'ed
 *	with the input mask.
 *
 *   2. the fut must provide outputs specified in the omask but
 *	not appearing the pmask.  Any missing fut output that appears
 *	in the pmask will be copied through from the input.
 *
 *   3. the user must supply all inputs required by the fut to produce
 *	the outputs specified by the user.
 */
int32
fut_iomask_check (fut_ptr_t fut, int32 iomask)
{
	int	pmask = (int)FUT_PMASK(iomask);
	int	omask = (int)FUT_OMASK(iomask);
	int	imask = (int)FUT_IMASK(iomask);
	int	rmask;

	imask |= pmask;			/* user supplied inputs */

	/*
	 * required fut inputs are those inputs required for each channel
	 * specified in omask.  If any required inputs are missing,
	 * return ERROR.
	 */
	rmask = (int)fut_required_inputs (fut, omask);
	if ( rmask & ~imask )
		return (0);

	/*
	 * required fut outputs are those in omask which are not in
	 * pmask. If any required fut outputs are missing, return ERROR.
	 */
	rmask = omask & ~pmask;		/* required fut outputs */
	if ( rmask & ~fut->iomask.out )
		return (0);

	/*
	 * return modified iomask.
	 */
	return (iomask | FUT_IN(imask));
}

/*
 * fut_required_inputs returns an input mask describing the required
 * inputs for the fut channels specified in omask.
 */
int32
fut_required_inputs (fut_ptr_t fut, int32 omask)
{
	int	i;
	int	imask = 0;

	if ( fut == FUT_NULL )
		return (0);

	if ( ! IS_FUT (fut) )
		return (-1);

	if ( omask == 0 )
		omask = fut->iomask.out;

	for ( i=0; i<FUT_NOCHAN; i++ )
		if (omask & FUT_BIT(i))
			imask |= (int)FUT_CHAN_IMASK(fut->chan[i]);

	return (imask);
}

/*
 * fut_exchange transfers all the tables and channel structures
 * from one fut to another.  This is used primarily inside the library
 * for implementing "in place" operations.  In this case, the user already
 * has a handle on (i.e. pointer to) an existing fut which may need to
 * have completely new channel structures and tables.  We can't simply
 * return a pointer to a new fut, we must replace all the existing
 * data in the existing fut structure.  This function is usually followed
 * by a call to fut_free() on one of the futs.
 *
 * Note: interpolation order and idstring are NOT exchanged!
 */
int
fut_exchange (fut_ptr_t fut1, fut_ptr_t fut0)
{
	fut_t			temp_fut;
	char fut_far	* temp_idstr;
	int		temp_order;

				/* exchange all data in entire structure */
	temp_fut = *fut1;
	*fut1 = *fut0;
	*fut0 = temp_fut;

				/* preserve idstr */
	temp_idstr = fut1->idstr;
	fut1->idstr = fut0->idstr;
	fut0->idstr = temp_idstr;

				/* preserve interpolation order */
	temp_order = fut1->iomask.order;
	fut1->iomask.order = fut0->iomask.order;
	fut0->iomask.order = temp_order;

	return (1);
}

/*
 * fut_first_chan returns the first channel number defined in a
 * channel mask or -1, if no channel is defined.
 */
int
fut_first_chan (int mask)
{
	int	i;

	if ( mask <= 0 )
		return (-1);

	for ( i=0; (mask & 1) == 0; mask >>= 1, i++ ) ;

	return (i);
}

/*
 * fut_last_chan returns the last channel number defined in a
 * channel mask or -1, if no channel is defined.
 */
int
fut_last_chan (int mask)
{
	int	i;

	if ( mask <= 0 )
		return (-1);

	for ( i=0; mask != 0; mask >>= 1, i++ ) ;

	return (--i);
}

/*
 * fut_count_chan returns the number of channels defined in a
 * channel mask. (counts number of bits set).
 */
int
fut_count_chan (int mask)
{
	int	i;

	for ( i=0; mask != 0; mask &= (mask-1), i++ ) ;

	return (i);
}

/*
 * fut_unique_id returns a unique id number for this currently running
 * application. Id numbers start at 1.  An id of zero is considered
 * non-unique elsewhere in the fut library.  Negative ids are reserved
 * to describe ramp tables.
 */
int
fut_unique_id ()
{
	static int	unique_id = 1;
	return (unique_id++);
}

/*
 * fut_gtbl_imask computes the input mask for a given grid table
 * based on its dimensions.
 */
int
fut_gtbl_imask (fut_gtbl_ptr_t gtbl)
{
	int	i, imask = 0;

	if ( gtbl != 0 )
		for ( i=0; i<FUT_NICHAN; i++ )
			if ( gtbl->size[i] > 1 ) imask |= FUT_BIT(i);

	return (imask);
}

/*
 * fut_reset_iomask recomputes the input and output mask portions
 * of a futs iomask, according to the existing input and grid tables.
 * it also performs a consistency check on the input and grid table
 * sizes, returning 0 (FALSE) if any problems, 1 (TRUE) if ok.
 */
int
fut_reset_iomask (fut_ptr_t fut)
{
	int		i, j;
					/* clear existing masks */
	fut->iomask.in = 0;
	fut->iomask.out = 0;

	for ( i=0; i<FUT_NOCHAN; i++ ) {
		fut_chan_ptr_t	chan = fut->chan[i];

		if ( chan == 0 ) continue;
					/* recompute masks */
		chan->imask = fut_gtbl_imask (chan->gtbl);
		fut->iomask.out |= FUT_BIT(i);
		fut->iomask.in |= chan->imask;

					/* consistency check */
		for ( j=0; j<FUT_NICHAN; j++ ) {
			if ( (chan->imask & FUT_BIT(j)) ) {
				if ( ! IS_ITBL(chan->itbl[j]) ||
				     chan->itbl[j]->size != chan->gtbl->size[j] )
					return (0);
			}
		}
	}

	return (1);
}

/*
 * fut_is_separable returns 1 (TRUE) if each output channel of the fut
 * is a function only of the corresponding input, 0 (FALSE) otherwise.
 * (i.e. f(x,y,z) = (fx(x),fy(y),fz(z)) ).
 */
int
fut_is_separable (fut_ptr_t fut)
{
	int		i;
	fut_chan_ptr_t	chan;

	for ( i=0; i<FUT_NOCHAN; i++ ) {
		chan = fut->chan[i];
		if ( chan != 0 && chan->imask != (int32)FUT_BIT(i) )
			return (0);
	}
	return (1);
}

 /*
 * fut_make_copyright generates the copyright string in the buffer pointed to
 *	by buffer.  It returns 1 (TRUE) if successful, 0 (FALSE) otherwise.
 */
 int
 fut_make_copyright (char fut_far *buffer)
 {
 
 	int			year;
 	char  		yearStr[5], tagStr[20];
 	struct kpTm   currentTime;
 												/* Get the current year							*/
												
	KpGetLocalTime(&currentTime);
	
												/* the year field is the number of 
													years since 1900 so add 1900 to that
													field to get the actual year				*/
													
	year = currentTime.year + 1900;													
	KpItoa(year, yearStr);
	
												/* now build the copyright string.  		*/
	
	KpItoa (KCM_COPYRIGHT, tagStr);
	strcpy (buffer, tagStr);
	strcat (buffer, "=");
	strcat (buffer, FUT_COPYRIGHT_PREFIX);
	strcat (buffer, yearStr);
	strcat (buffer, FUT_COPYRIGHT_SUFFIX);

	return (1);
	
} /* fut_make_copyright */
	
	
	
		
												
													
ÿ