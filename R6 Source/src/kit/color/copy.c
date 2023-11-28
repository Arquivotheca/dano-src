/*
  File:         copy.c          @(#)copy.c	13.7 03/29/96

*//***************************************************************
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
 * functions to copy futs and their components.  Note that these are
 * physical (hard) copies.  If a virtual (soft) copy will suffice
 * for itbls, otbls, and gtbls, you can use fut_share_[iog]tbl().
 */
#include "fut.h"
#include "fut_util.h"		/* internal interface file */

/*
 * fut_copy copies an existing fut.  If the fut's itbls and otbls are
 * currently being shared, then the new fut will share these tables also.
 * Grid tables are always copied though.
 */
fut_ptr_t
fut_copy (fut_ptr_t fut)
{
	fut_ptr_t	new_fut;
	int	i;
	fut_handle		h;

	if ( ! IS_FUT(fut) )
		return (0);

				/* allocate basic fut_structure */
	new_fut = fut_alloc_fut ();
	if ( new_fut == NULL )
		return (NULL);

				/* save handle before copying over old fut */
	h = new_fut->handle;

				/* copy over all data (including pointers */
	*new_fut = *fut;

				/* now copy back handle */
	new_fut->handle = h;

				/* copy id string */
	new_fut->idstr = 0;
	(void) fut_set_idstr (new_fut, fut->idstr);

				/* copy input tables */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		new_fut->itbl[i] = (IS_SHARED (fut->itbl[i])) ?
					fut_share_itbl (fut->itbl[i]) :
					fut_copy_itbl (fut->itbl[i]);
		new_fut->itblHandle[i] = (IS_ITBL(new_fut->itbl[i])) ?
													new_fut->itbl[i]->handle : FUT_NULL_HANDLE;
	}

				/* copy output channels */
	for ( i=0; i<FUT_NOCHAN; i++ ) {
		new_fut->chan[i] = fut_copy_chan (fut->chan[i]);
		new_fut->chanHandle[i] = (IS_CHAN(new_fut->chan[i])) ?
													new_fut->chan[i]->handle : FUT_NULL_HANDLE;
	}

				/* now check that all copies were succesful */
	if ( new_fut->idstr == 0 && fut->idstr != 0 ) {
		fut_free (new_fut);
		return (NULL);
	}

	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( new_fut->itbl[i] == 0 && fut->itbl[i] != 0) {
			fut_free (new_fut);
			return (NULL);
		}
	}
	for ( i=0; i<FUT_NOCHAN; i++ ) {
		if ( new_fut->chan[i] == 0 && fut->chan[i] != 0) {
			fut_free (new_fut);
			return (NULL);
		}
	}

	return (new_fut);

} /* fut_copy */

/*
 * fut_copy_chan makes a copy of an existing fut_chan_t structure.
 * If input or output tables in chan are being shared, then the
 * new chan will share them also,  otherwise a new copy of the tables
 * will be made.  Grid tables are always copied rather than shared.
 */
fut_chan_ptr_t
fut_copy_chan (fut_chan_ptr_t chan)
{
	fut_chan_ptr_t	new_chan;
	int		i;
	fut_handle		h;

	if ( ! IS_CHAN(chan) )
		return (NULL);

	new_chan = fut_alloc_chan ();
	if ( new_chan == NULL )
		return (NULL);

				/* save handle before copying over old fut */
	h = new_chan->handle;

				/* copy over to new structure */
	*new_chan = *chan;

				/* move handle back to new structure */
	new_chan->handle = h;

					/* copy (or share) itbls */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		new_chan->itbl[i] = (IS_SHARED (chan->itbl[i])) ?
					fut_share_itbl (chan->itbl[i]) :
					fut_copy_itbl (chan->itbl[i]);
		new_chan->itblHandle[i] = (IS_ITBL(new_chan->itbl[i])) ?
													new_chan->itbl[i]->handle : FUT_NULL_HANDLE;
	}

					/* copy (or share) otbl */
	new_chan->otbl = (IS_SHARED(chan->otbl)) ?
				fut_share_otbl (chan->otbl) :
				fut_copy_otbl (chan->otbl);
	new_chan->otblHandle = (IS_OTBL(new_chan->otbl)) ?
													new_chan->otbl->handle : FUT_NULL_HANDLE;

					/* always copy gtbl */
	new_chan->gtbl = fut_copy_gtbl (chan->gtbl);
	new_chan->gtblHandle =  (IS_GTBL(new_chan->gtbl)) ?
													new_chan->gtbl->handle : FUT_NULL_HANDLE;

					/* check for successful copies */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( new_chan->itbl[i] == 0 && chan->itbl[i] != 0 ) {
			fut_free_chan (new_chan);
			return (NULL);
		}
	}
	if ( (new_chan->otbl == 0 && chan->otbl != 0) ||
	     (new_chan->gtbl == 0 && chan->gtbl != 0) ) {
		fut_free_chan (new_chan);
		return (NULL);
	}

	return (new_chan);

} /* fut_copy_chan */

/*
 * fut_copy_itbl makes an exact copy of an existing fut_itbl_t
 */
fut_itbl_ptr_t
fut_copy_itbl (fut_itbl_ptr_t itbl)
{
	fut_itbl_ptr_t	new_itbl;
	fut_handle		h;
	KpUInt32_t		mode;
	KpInt32_t		entrySize;

				/* check for valid itbl */
	if ( ! IS_ITBL(itbl) )
		return (FUT_NULL_ITBL);

				/* allocate the new itbl structure */
	new_itbl = fut_alloc_itbl ();
	if ( new_itbl == FUT_NULL_ITBL ) {
		DIAG("fut_copy_itbl: can't alloc input table struct.\n", 0);
		return (FUT_NULL_ITBL);
	}

				/* save handle before copying over old fut */
	h = new_itbl->handle;

				/* copy entire struct except reference count */
	*new_itbl = *itbl;
	new_itbl->ref = 0;
				/* copy back handle */
	new_itbl->handle = h;

	if (new_itbl->tblFlag == 0) {
		mode = ITBLMODE8BIT;
		entrySize = (FUT_INPTBL_ENT+1)*sizeof(fut_itbldat_t);
	}
	else {
		mode = ITBLMODE12BIT;
		entrySize = ((FUT_INPTBL_ENT+1) + (FUT_INPTBL_ENT2+1)) *
					sizeof(fut_itbldat_t);
	}
				/* allocate array of table entries */
	new_itbl->tbl = fut_alloc_itbldat (mode);
	if ( new_itbl->tbl == NULL ) {
		DIAG("fut_copy_itbl: can't alloc input table array.\n", 0);
		fut_free_itbl (new_itbl);
		return (FUT_NULL_ITBL);
	}
	new_itbl->tblHandle = getHandleFromPtr((fut_generic_ptr_t)new_itbl->tbl);

				/* copy the table entries */
	bcopy ((fut_generic_ptr_t)itbl->tbl, (fut_generic_ptr_t)new_itbl->tbl,
		      entrySize);

	return (new_itbl);

} /* fut_copy_itbl */

/*
 * fut_copy_otbl makes an exact copy of an existing fut_otbl_t
 */
fut_otbl_ptr_t
fut_copy_otbl (fut_otbl_ptr_t otbl)
{
	fut_otbl_ptr_t	new_otbl;
	fut_handle		h;

				/* check for valid otbl */
	if ( ! IS_OTBL(otbl) )
		return (FUT_NULL_OTBL);

				/* allocate the new otbl structure */
	new_otbl = fut_alloc_otbl ();
	if ( new_otbl == FUT_NULL_OTBL ) {
		DIAG("fut_copy_otbl: can't alloc output table struct.\n", 0);
		return (FUT_NULL_OTBL);
	}

				/* save handle before copying over old fut */
	h = new_otbl->handle;

				/* copy entire struct except reference count */
	*new_otbl = *otbl;
	new_otbl->ref = 0;
	new_otbl->handle = h;

				/* allocate array of table entries */
	new_otbl->tblHandle = FUT_NULL_HANDLE;
	new_otbl->tbl = fut_alloc_otbldat();
	if ( new_otbl->tbl == NULL ) {
		DIAG("fut_copy_otbl: can't alloc output table array.\n", 0);
		fut_free_otbl (new_otbl);
		return (FUT_NULL_OTBL);
	}
	new_otbl->tblHandle = getHandleFromPtr ((fut_generic_ptr_t)new_otbl->tbl);

				/* copy the table entries */
	bcopy ((fut_generic_ptr_t)otbl->tbl, (fut_generic_ptr_t)new_otbl->tbl,
		      (int32)(FUT_OUTTBL_ENT)*sizeof(fut_otbldat_t));

	return (new_otbl);

} /* fut_copy_otbl */

/*
 * fut_copy_gtbl makes an exact copy of an existing fut_gtbl_t
 */
fut_gtbl_ptr_t
fut_copy_gtbl (fut_gtbl_ptr_t gtbl)
{
	fut_gtbl_ptr_t	new_gtbl;
	int32		gsize;
	fut_handle		h;

				/* check for valid gtbl */
	if ( ! IS_GTBL(gtbl) )
		return (FUT_NULL_GTBL);

				/* allocate the new gtbl structure */
	new_gtbl = fut_alloc_gtbl ();
	if ( new_gtbl == FUT_NULL_GTBL ) {
		DIAG("fut_copy_gtbl: can't alloc grid table struct.\n", 0);
		return (FUT_NULL_GTBL);
	}

				/* save handle before copying over old fut */
	h = new_gtbl->handle;

				/* copy entire struct except reference count */
	*new_gtbl = *gtbl;
	new_gtbl->ref = 0;
	new_gtbl->handle = h;

				/* allocate array of table entries */
	gsize = gtbl->tbl_size / (int32)sizeof(fut_gtbldat_t);
	new_gtbl->tblHandle = FUT_NULL_HANDLE;
	new_gtbl->tbl = fut_alloc_gtbldat (gsize);
	if ( new_gtbl->tbl == NULL ) {
		DIAG("fut_copy_gtbl: can't alloc grid table array.\n", 0);
		fut_free_gtbl (new_gtbl);
		return (FUT_NULL_GTBL);
	}
	new_gtbl->tblHandle = getHandleFromPtr ((fut_generic_ptr_t)new_gtbl->tbl);

				/* copy the table entries */
	bcopy ((fut_generic_ptr_t)gtbl->tbl, (fut_generic_ptr_t)new_gtbl->tbl,
			(int32)gtbl->tbl_size);

	return (new_gtbl);

} /* fut_copy_gtbl */


