/*
	File:		fut_malloc.c	@(#)malloc.c	13.11 07/18/97

	Contains:	Allocation routines for FuT library

	Written by:	Drivin' Team

	Change History (most recent first):

 <2>	  8/9/91	gbp		correct chan locking/unlocking
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

/*
 * These functions allocate and free memory used for input,
 *	output, and grid tables.
 ***** These form the major interface for fut memory allocation
 * fut_t 				*fut_alloc_fut ();
 * fut_chan_t 			*fut_alloc_chan ();
 * fut_itbl_t 			*fut_alloc_itbl ();
 * fut_otbl_t 			*fut_alloc_otbl ();
 * fut-gtbl_t 			*fut_alloc_gtbl ();
 * fut_itbldat_t 		*fut_alloc_itbldat ();
 * fut_otbldat_t 		*fut_alloc_otbldat ();
 * fut-gtbldat_t 		*fut_alloc_gtbldat (int32);
 *
 ***** These form the fut unlock and lock interface *****
 * fut_handle			fut_unlock_fut(fut);
 * fut_handle			fut_unlock_itbl(itbl);
 * fut_handle			fut_unlock_otbl(otbl);
 * fut_handle			fut_unlock_gtbl(gtbl);
 * fut_handle			fut_unlock_chan(chan);
 *
 * fut_t*				fut_lock_fut(handle);
 * fut_itbl_t*			fut_lock_itbl(handle);
 * fut_otbl_t*			fut_lock_otbl(handle);
 * fut_gtbl_t*			fut_lock_gtbl(handle);
 * fut_chan_t*			fut_lock_chan(handle);
 */

#include "fut.h"
#include "fut_util.h"

/*
 * convenient allocators of fut, and table structures:
 */
fut_ptr_t
fut_alloc_fut(void)
{
	fut_ptr_t	fut;

			/* allocate a zeroed block of memory */
	fut = (fut_ptr_t )fut_malloc((int32)sizeof(fut_t), "hc");
 	if (fut==NULL)
		return (NULL);

	fut->magic = FUT_MAGIC;			/* set magic number */
	fut->refNum = fut_unique_id ();	/* and unique reference number */

			/* get handle and store */
	fut->handle = getHandleFromPtr ((fut_generic_ptr_t)fut);
	return(fut);
}

fut_chan_ptr_t
fut_alloc_chan(void)
{
	fut_chan_ptr_t chan;

			/* allocate a zeroed block of memory */
	chan = (fut_chan_ptr_t)fut_malloc((int32)sizeof(fut_chan_t), "hc");
 	if (chan==NULL)
		return(NULL);

			/* set magic number */
	chan->magic = FUT_CMAGIC;

			/* get handle and store */
	chan->handle = getHandleFromPtr ((fut_generic_ptr_t)chan);
	return(chan);
}

fut_itbl_ptr_t
fut_alloc_itbl(void)
{
	fut_itbl_ptr_t itbl;

			/* allocate a zeroed block of memory */
	itbl = (fut_itbl_ptr_t )fut_malloc((int32)sizeof(fut_itbl_t), "hc");
 	if ( itbl == NULL )
		return (NULL);

			/* set magic number */
	itbl->magic = FUT_IMAGIC;

			/* get handle and store */
	itbl->handle = getHandleFromPtr ((fut_generic_ptr_t)itbl);
	return (itbl);
}

fut_otbl_ptr_t
fut_alloc_otbl(void)
{
	fut_otbl_ptr_t  otbl;

			/* allocate a zeroed block of memory */
	otbl = (fut_otbl_ptr_t )fut_malloc((int32)sizeof(fut_otbl_t), "hc");
 	if ( otbl == NULL )
		return (NULL);

			/* set magic number */
	otbl->magic = FUT_OMAGIC;

			/* get handle and store */
	otbl->handle = getHandleFromPtr ((fut_generic_ptr_t)otbl);
	return (otbl);
}

fut_gtbl_ptr_t
fut_alloc_gtbl(void)
{
	fut_gtbl_ptr_t  gtbl;

			/* allocate a zeroed block of memory */
	gtbl = (fut_gtbl_ptr_t )fut_malloc((int32)sizeof(fut_gtbl_t), "hc");
 	if ( gtbl == NULL ) {
		return (NULL);
	}

			/* set magic number */
	gtbl->magic = FUT_GMAGIC;

			/* get handle and store */
	gtbl->handle = getHandleFromPtr ((fut_generic_ptr_t)gtbl);
	return (gtbl);
}

fut_itbldat_ptr_t
fut_alloc_itbldat(KpUInt32_t mode)
{
	int32	size;

	/* determine the mode */
	switch (mode)
	{
		case ITBLMODE8BIT:
			size = (FUT_INPTBL_ENT+1) * sizeof(fut_itbldat_t);
			break;

		case ITBLMODE12BIT:
			size = ((FUT_INPTBL_ENT+1) + (FUT_INPTBL_ENT2+1)) *
					sizeof(fut_itbldat_t);
			break;
		case ITBLMODE16BIT:
			size = ((FUT_INPTBL_ENT+1) + (FUT_INPTBL_ENT3+1)) *
					sizeof(fut_itbldat_t);
			break;
		default:
			return (NULL);
	}

	return ((fut_itbldat_ptr_t )fut_malloc(size, "i"));
}

fut_otbldat_ptr_t
fut_alloc_otbldat(void)
{
	int32	size;

	size = (FUT_OUTTBL_ENT) * sizeof(fut_otbldat_t);

	return ((fut_otbldat_ptr_t )fut_malloc(size, "o"));
}

fut_gtbldat_ptr_t
fut_alloc_gtbldat(int32 size)
{
	size *= (int32) sizeof(fut_gtbldat_t);

	return ((fut_gtbldat_ptr_t)fut_malloc(size, "g"));
}

/* The fut lock and unlock functions are performed by these basic routines */

fut_handle
fut_unlock_fut(fut_ptr_t fut)
{
	int	i;
	fut_handle	handle;

	if (fut == FUT_NULL) {
		return ((fut_handle)FUT_NULL);
	}

	handle = fut->handle;

	for (i=0; i<FUT_NICHAN; i++) {
		fut->itblHandle[i] = fut_unlock_itbl(fut->itbl[i]);
		fut->itbl[i] = FUT_NULL_ITBL;
	}

	for (i=0; i<FUT_NOCHAN; i++) {
		fut->chanHandle[i] = fut_unlock_chan(fut->chan[i]);
		fut->chan[i] = FUT_NULL_CHAN;
	}

	(void)unlockBuffer (handle);

	return(handle);
}


fut_handle
fut_unlock_itbl(fut_itbl_ptr_t itbl)
{
	fut_handle	handle;

	if (itbl == FUT_NULL_ITBL) {
		return ((fut_handle)FUT_NULL_ITBL);
	}

	handle = itbl->handle;

	(void)unlockBuffer (itbl->tblHandle);
	itbl->tbl = FUT_NULL_ITBLDAT;
	itbl->tbl2 = FUT_NULL_ITBLDAT;

	(void)unlockBuffer (handle);

	return(handle);
}

fut_handle
fut_unlock_otbl(fut_otbl_ptr_t otbl)
{
	fut_handle	handle;

	if (otbl == FUT_NULL_OTBL) {
		return ((fut_handle)FUT_NULL_OTBL);
	}

	handle = otbl->handle;

	(void)unlockBuffer (otbl->tblHandle);
	otbl->tbl = FUT_NULL_OTBLDAT;

	(void)unlockBuffer (handle);

	return(handle);
}


fut_handle
fut_unlock_gtbl(fut_gtbl_ptr_t gtbl)
{
	fut_handle	handle;

	handle = gtbl->handle;

	(void)unlockBuffer (gtbl->tblHandle);
	gtbl->tbl = FUT_NULL_GTBLDAT;

	(void)unlockBuffer (handle);

	return(handle);
}

fut_handle
fut_unlock_chan(fut_chan_ptr_t chan)
{
	int		i;
	fut_handle	handle;

	if (chan == FUT_NULL_CHAN) {
		return ((fut_handle)FUT_NULL_CHAN);
	}

	handle = chan->handle;

	chan->gtblHandle = fut_unlock_gtbl(chan->gtbl);
	chan->gtbl = FUT_NULL_GTBL;

	chan->otblHandle = fut_unlock_otbl(chan->otbl);
	chan->otbl = FUT_NULL_OTBL;

	for ( i=0; i<FUT_NICHAN; i++) {
		chan->itblHandle[i] = fut_unlock_itbl(chan->itbl[i]);
		chan->itbl[i] = FUT_NULL_ITBL;
	}

	(void)unlockBuffer (handle);

	return(handle);
}


fut_ptr_t
fut_lock_fut(fut_handle handle)
{
	int	i;
	fut_ptr_t fut;

	if (handle == NULL) {
		return (FUT_NULL);
	}

	fut = (fut_ptr_t )lockBuffer (handle);

	for (i=0; i<FUT_NICHAN; i++) {
		fut->itbl[i] = fut_lock_itbl(fut->itblHandle[i]);
	}

	for (i=0; i<FUT_NOCHAN; i++) {
		fut->chan[i] = fut_lock_chan(fut->chanHandle[i]);
	}

	return(fut);
}

fut_itbl_ptr_t
fut_lock_itbl(fut_handle handle)
{
	fut_itbl_ptr_t itbl;

	if (handle == NULL) {
		return (FUT_NULL_ITBL);
	}

	itbl = (fut_itbl_ptr_t )lockBuffer (handle);

	itbl->tbl = (fut_itbldat_ptr_t )lockBuffer (itbl->tblHandle);

	if (itbl->tblFlag > 0) {
		itbl->tbl2 = itbl->tbl + (FUT_INPTBL_ENT+1);
	}

	return(itbl);
}

fut_otbl_ptr_t
fut_lock_otbl(fut_handle handle)
{
	fut_otbl_ptr_t otbl;

	if (handle == NULL) {
		return (FUT_NULL_OTBL);
	}

	otbl = (fut_otbl_ptr_t )lockBuffer (handle);

	otbl->tbl = (fut_otbldat_ptr_t )lockBuffer (otbl->tblHandle);

	return(otbl);
}

fut_gtbl_ptr_t
fut_lock_gtbl(fut_handle handle)
{
	fut_gtbl_ptr_t gtbl;

	if (handle == NULL) {
		return (FUT_NULL_GTBL);
	}

	gtbl = (fut_gtbl_ptr_t )lockBuffer (handle);

	gtbl->tbl = (fut_gtbldat_ptr_t)lockBuffer (gtbl->tblHandle);

	return(gtbl);
}

fut_chan_ptr_t
fut_lock_chan(fut_handle handle)
{
	int		i;
	fut_chan_ptr_t	chan;

	if (handle == NULL) {
		return (FUT_NULL_CHAN);
	}

	chan = (fut_chan_ptr_t)lockBuffer (handle);

	chan->gtbl = fut_lock_gtbl(chan->gtblHandle);

	chan->otbl = fut_lock_otbl(chan->otblHandle);

	for ( i=0; i<FUT_NICHAN; i++ ) {
		chan->itbl[i] = fut_lock_itbl(chan->itblHandle[i]);
	}

	return(chan);
}


/**** End of lock routines ****************/


/************** End of MALLOC.C ***************/

