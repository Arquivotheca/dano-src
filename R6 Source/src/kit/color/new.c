/*
  File:		new.c		@(#)new.c	13.18 03/14/97
  Author:	Kit Enscoe, George Pawle

  functions to create and destroy futs and their components.

  Note that when a fut, chan, or [iog]tbl is freed, its magic number
  is zeroed.  Since all fut functions test for the magic number (and
  return an error if invalid), this prevents the structures from being
  used in the event that a freed pointer is accidentally referenced.
  Also, because of this checking, there is no need to zero the memory
  handles! (or anything else).

  PROPRIETARY NOTICE: The  software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on their
  designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1989-1994 Eastman Kodak Company.
  As  an  unpublished  work pursuant to Title 17 of the United
  States Code.  All rights reserved.
*/

#include "fut.h"
#include "fut_util.h"		/* internal interface file */


/* local prototypes */
static void fut_free_chan_list_p (fut_chan_ptr_t fut_far * chan_list,
                                  fut_handle fut_far * chanHdl_list);
static void fut_free_itbl_list_p (fut_itbl_ptr_t fut_far * itbl_list,
                                  fut_handle fut_far * itblHdl_list);
static void fut_free_otbl_p (fut_otbl_ptr_t otblPtr,
                                  fut_handle otblHdl);
static void fut_free_gtbl_p (fut_gtbl_ptr_t gtblPtr,
                                  fut_handle gtblHdl);


fut_ptr_t
fut_free (fut_ptr_t	fut)
{
	if ( ! IS_FUT(fut) )	/* check if defined */
		return (fut);

				/* free id string if exists */
	fut_free_idstr (fut->idstr);

				/* free shared input tables */
	fut_free_itbl_list (fut->itbl);

				/* free channels */
	fut_free_chan_list (fut->chan);

				/* free fut_t structure itself */
	fut->magic = 0;
	fut_mfree ((fut_generic_ptr_t)fut, "h");


	return ((fut_ptr_t)NULL);

} /* fut_free */

void
fut_free_chan_list (fut_chan_ptr_t	fut_far * chan_list)
{
	int	i;

	if ( chan_list == 0 )
		return;

	for ( i=0; i<FUT_NOCHAN; i++ ) {
		fut_free_chan (chan_list[i]);
		chan_list[i] = FUT_NULL_CHAN;
	}

} /* fut_free_chan_list */

void
fut_free_chan (fut_chan_ptr_t chan)
{
	if ( ! IS_CHAN(chan) )	/* check if defined */
		return;

	fut_free_itbl_list (chan->itbl);	/* free input tables */

	fut_qfree_otbl (chan->otbl);		/* free output table */

	fut_qfree_gtbl (chan->gtbl);		/* free grid table */

				/* free fut_chan_t structure itself */
	chan->magic = 0;
	fut_mfree ((fut_generic_ptr_t)chan, "h");

} /* fut_free_chan */

void
fut_free_itbl_list (fut_itbl_ptr_t	fut_far * itbl_list)
{
	int	i;

	if ( itbl_list == 0 )
		return;

	for ( i=0; i<FUT_NICHAN; i++ ) {
		fut_qfree_itbl (itbl_list[i]);
		itbl_list[i] = 0;
	}

} /* fut_free_itbl_list */

void
fut_free_itbl (fut_itbl_ptr_t itbl)
{
	if ( ! IS_ITBL(itbl) )	/* check if defined */
		return;
	else if ( itbl->ref < 0 )	/* defined externally */
		return;
	else if ( itbl->ref > 0 )	/* others still reference */
		itbl->ref--;
	else  {				/* last reference */
		fut_mfree ((fut_generic_ptr_t)itbl->tbl, "i");
		itbl->magic = 0;
		fut_mfree ((fut_generic_ptr_t)itbl, "h");
	}
} /* fut_free_itbl */

void
fut_free_otbl (fut_otbl_ptr_t otbl)
{
	if ( ! IS_OTBL(otbl) )	/* check if defined */
		return;
	else if ( otbl->ref < 0 )	/* defined externally */
		return;
	else if ( otbl->ref > 0 )	/* others still reference */
		otbl->ref--;
	else  {				/* last reference */
		fut_mfree ((fut_generic_ptr_t)otbl->tbl, "o");
		otbl->magic = 0;
		fut_mfree ((fut_generic_ptr_t)otbl, "h");
	}
} /* fut_free_otbl */

void
fut_free_gtbl (fut_gtbl_ptr_t gtbl)
{
	if ( ! IS_GTBL(gtbl) )	/* check if defined */
		return;
	else if ( gtbl->ref < 0 )	/* defined externally */
		return;
	else if ( gtbl->ref > 0 )	/* others still reference */
		gtbl->ref--;
	else {				/* last reference */
		fut_mfree ((fut_generic_ptr_t)gtbl->tbl, "g");
		gtbl->magic = 0;
		fut_mfree ((fut_generic_ptr_t)gtbl, "h");
	}
} /* fut_free_gtbl */

/*
 * fut_free_tbl frees any table regardless of type by checking the magic
 * number in the header.  It will also free a fut_t or a fut_chan_t.
 *
 * fut_free_tbls will free a null terminated list of any type of table,
 * useful for disposing of a set of tables which were used for constructing
 * a fut (which the fut has now absorbed and made shared copies of).
 */
void
fut_free_tbl (int32 fut_far* tbl)
{
	/*
	 *   Make sure that we do not have a
	 *   NULL pointer
	 */
	if( tbl == (int32 fut_far*)NULL )
		return;

	switch (*tbl) {
	    case FUT_MAGIC:
		fut_free ((fut_ptr_t) tbl);
		break;
	    case FUT_CMAGIC:
		fut_free_chan ((fut_chan_ptr_t) tbl);
		break;
	    case FUT_IMAGIC:
		fut_free_itbl ((fut_itbl_ptr_t) tbl);
		break;
	    case FUT_OMAGIC:
		fut_free_otbl ((fut_otbl_ptr_t) tbl);
		break;
	    case FUT_GMAGIC:
		fut_free_gtbl ((fut_gtbl_ptr_t) tbl);
		break;
	}

} /* fut_free_tbl */

void
fut_free_tbls (int32 fut_far	*tbl0, ...)
{
	char_p	array;	/* required by KCMS_VA_xxx */
	KCMS_VA_ARG_PTR	ap;	/* arg pointer */
	int32_p	tbl;

	tbl = tbl0;
	KCMS_VA_START(ap,array,tbl0);
	do   {
		fut_free_tbl(tbl);
		tbl = KCMS_VA_ARG(ap,array,int32 fut_far*);
		}   while ( tbl != (int32 fut_far*)NULL );

	KCMS_VA_END(ap,array);

} /* fut_free_tbls */

/*** Functions to free a fut but preserve the locked/unlocked
     state pf any element which is not actually freed.  Items
     will not be freed if they are shared with another fut.
***/

fut_ptr_t
fut_free_futH (fut_handle futHandle)
{
	fut_ptr_t fut;
	
	fut = (fut_ptr_t) lockBuffer(futHandle);

	if ( ! IS_FUT(fut) )	/* check if defined */
		return (fut);

				/* free id string if exists */
	fut_free_idstr (fut->idstr);

				/* free shared input tables */
	fut_free_itbl_list_p (fut->itbl, fut->itblHandle);

				/* free channels */
	fut_free_chan_list_p (fut->chan, fut->chanHandle);

				/* free fut_t structure itself */
	fut->magic = 0;
	fut_mfree ((fut_generic_ptr_t)fut, "h");


	return ((fut_ptr_t)NULL);

} /* fut_free */

/* fut_free_chan_list_p
      For each channel, This functions frees all input tables, 
      the output table and the grid table.  It then frees the
      actual fut_chan_t memory.
*/
static void
fut_free_chan_list_p (fut_chan_ptr_t fut_far * chan_list,
                      fut_handle fut_far * chanHdl_list)
{
int              i;
fut_chan_ptr_t   chan;

	if ( (chan_list == 0 ) || ( chanHdl_list == 0 )  )
		return;


	for ( i=0; i<FUT_NOCHAN; i++ ) {
		chan = chan_list[i];
		if (chan == 0 ) {		/* chan is unlocked on entry */
			chan = lockBuffer(chanHdl_list[i]);
		}
		
		if (chan != 0) {
			if ( IS_CHAN(chan) ) {	/* check if defined */

				fut_free_itbl_list_p (chan->itbl, chan->itblHandle);	/* free input tables */

				fut_free_otbl_p (chan->otbl, chan->otblHandle);		/* free output table */

				fut_free_gtbl_p (chan->gtbl, chan->gtblHandle);		/* free grid table */

				/* free fut_chan_t structure itself */
				chan->magic = 0;
				fut_mfree ((fut_generic_ptr_t)chan, "h");
				chan_list[i] = FUT_NULL_CHAN;
			}
		}
	}

} /* fut_free_chan_list_p */


/* fut_free_itbl_list_p
      This function is passed a list of fut_itbl_t pointers
      and handles.  If the ref count is zero, the table and
      the fut_itbl_t is freed.  Otherwise the ref count is 
      decremented and the lock state of the fut_itbl_t
      is returned to it's state on entry.
*/
static void
fut_free_itbl_list_p (fut_itbl_ptr_t	fut_far * itbl_list,
                      fut_handle	    fut_far * itblHdl_list)
{
int              i;
fut_itbl_ptr_t   itbl;

	if ( (itbl_list == 0 ) || ( itblHdl_list == 0 )  )
		return;

	for ( i=0; i<FUT_NICHAN; i++ ) {
		itbl = itbl_list[i];
		if (itbl == 0 ) {
			itbl = lockBuffer(itblHdl_list[i]);
		}
		if (itbl != 0) {
			if ( itbl->ref == 0 ) {
				/* last reference being freed */
				freeBuffer(itbl->tblHandle);
				itbl->magic = 0;
				fut_mfree ((fut_generic_ptr_t)itbl, "h");
		        itbl_list[i] = 0;
		        itblHdl_list[i] = 0;
		    }
			else if ( itbl->ref > 0 ) {
				/* still other references       */
				/* leave in original lock state */
				itbl->ref--;
				if (itbl_list[i] == 0)
					unlockBuffer(itblHdl_list[i]);
			}
		}
	}

} /* fut_free_itbl_list_p */



/* fut_free_otbl_p
      This function is passed a fut_otbl_t pointer
      and handle.  If the ref count is zero, the table and
      the fut_otbl_t is freed.  Otherwise the ref count is 
      decremented and the lock state of the fut_otbl_t
      is returned to it's state on entry.
*/
static void
fut_free_otbl_p (fut_otbl_ptr_t	otblPtr,
                 fut_handle     otblHdl)
{
fut_otbl_ptr_t   otbl;

	if ( otblHdl == 0 )
		return;

	otbl = otblPtr;
	if (otbl == 0 ) {		/* otbl is unlocked on entry */
		otbl = lockBuffer(otblHdl);
	}
	if (otbl != 0) {
		if ( otbl->ref == 0 ) {
			/* last reference being freed */
			freeBuffer(otbl->tblHandle);
			otbl->magic = 0;
			fut_mfree ((fut_generic_ptr_t)otbl, "h");
		}
		else if ( otbl->ref > 0 ) {
			/* still other references       */
			otbl->ref--;
			/* leave in original lock state */
			if (otblPtr == 0)
				unlockBuffer(otblHdl);
		}
	}

} /* fut_free_otbl_p */


/* fut_free_gtbl_p
      This function is passed a fut_gtbl_t pointer
      and handle.  If the ref count is zero, the table and
      the fut_gtbl_t is freed.  Otherwise the ref count is 
      decremented and the lock state of the fut_gtbl_t
      is returned to it's state on entry.
*/
static void
fut_free_gtbl_p (fut_gtbl_ptr_t	gtblPtr,
                 fut_handle     gtblHdl)
{
fut_gtbl_ptr_t   gtbl;

	if ( gtblHdl == 0 )
		return;

	gtbl = gtblPtr;
	if (gtbl == 0 ) {		/* gtbl is unlocked on entry */
		gtbl = lockBuffer(gtblHdl);
	}
	
	if (gtbl != 0) {
		if ( gtbl->ref == 0 ) {
			/* last reference being freed */
			freeBuffer(gtbl->tblHandle);
			gtbl->magic = 0;
			fut_mfree ((fut_generic_ptr_t)gtbl, "h");
		}
		else if ( gtbl->ref > 0 ) {
			/* still other references       */
			gtbl->ref--;
			/* leave in original lock state */
			if (gtblPtr == 0)
				unlockBuffer(gtblHdl);
		}
	}

} /* fut_free_gtbl_p */



/*
 * fut_new allocates and initializes a new fut_t data structure.
 * iomask specifies which (common) input tables and which output channels
 * are being defined.  Additional channels may be added later using
 * fut_defchan.
 *
 * NOTES:
 *   1. If the FUT_VARARGS form is used, all the tables must be packed
 *	into a single array.
 *
 *   2. If a needed input table is not supplied (as determined from the
 * 	grid table) or if a supplied input table is NULL, then a ramp
 *	input table will be automatically generated and inserted into
 *	the common itbl list.  The grid sizes are inferred from the
 *	supplied grid tables.
 */
fut_ptr_t fut_new (int32 iomask, ...)
{
	KCMS_VA_ARG_PTR		ap;	/* arg pointer */
	char_p	vap;	/* for FUT_VARARGS */
	fut_itbl_ptr_t			itbl[FUT_NICHAN];
	fut_otbl_ptr_t			otbl[FUT_NOCHAN];
	fut_gtbl_ptr_t			gtbl[FUT_NOCHAN];
	fut_ptr_t		fut;
	int			imask;
	int			omask;
	int			i;
	char			copyrightStr[FUT_COPYRIGHT_MAX_LEN];

					/* get input and output masks */
	imask = (int)FUT_IMASK(iomask);
	omask = (int)FUT_OMASK(iomask);
	if ( imask > FUT_ALLIN || omask > FUT_ALLOUT ) {
		DIAG("fut_new: too many input or output channels.\n", 0);
		return (NULL);
	}

					/* get args specified by iomask */
	KCMS_VA_START(ap,vap,iomask);

	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));

	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( (imask & FUT_BIT(i)) == 0 ) {
			itbl[i] = FUT_NULL_ITBL;
		} else {
			itbl[i] = KCMS_VA_ARG(ap,vap,fut_itbl_ptr_t);
		}
	}
	for ( i=0; i<FUT_NOCHAN; i++ ) {
		if ( (omask & FUT_BIT(i)) == 0 ) {
			gtbl[i] = FUT_NULL_GTBL;
			otbl[i] = FUT_NULL_OTBL;
		} else {
			gtbl[i] = KCMS_VA_ARG(ap,vap,fut_gtbl_ptr_t);
			otbl[i] = KCMS_VA_ARG(ap,vap,fut_otbl_ptr_t);
		}
	}

	KCMS_VA_END(ap,vap);

				/* allocate and clear the fut_t structure */
	fut = fut_alloc_fut ();
	if ( fut == NULL )
		return (NULL);

				/* set the interpolation order */
	fut->iomask.order = (int)FUT_ORDMASK(iomask);

				/* set copyright in idstring */
				
	if ( ! fut_make_copyright (copyrightStr)) {
		fut_free (fut);
		return (NULL);
	} 
	if ( ! fut_new_idstr (fut, copyrightStr) ) {
		fut_free (fut);
		return (NULL);
	}

				/* insert the specified input tables */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( itbl[i] == 0 ) continue;
		if ( ! IS_ITBL (itbl[i]) ) {
			fut_free (fut);
			return (NULL);
		}
		fut->iomask.in |= FUT_BIT(i);
		fut->itbl[i] = fut_share_itbl(itbl[i]);
		fut->itblHandle[i] = fut->itbl[i]->handle;
	}

				/* define the specified output channels */
	for ( i=0; i<FUT_NOCHAN; i++ ) {
		if ( gtbl[i] == 0 ) continue;
		if ( ! fut_defchan(fut,FUT_OUT(FUT_BIT(i)),gtbl[i],otbl[i]) ) {
			fut_free (fut);
			return (NULL);
		}
	}

	return (fut);

} /* fut_new */

/*
 * fut_new_chan allocates and initializes a fut_chan_t data structure.
 * If a required input table is missing, a ramp of the proper grid size
 * will be created.  If a supplied itbl is not required, it will not be
 * inserted into the channel's private itbl list.  All tables which are
 * actually used are "shared" and so the caller is responsible for
 * freeing the passed tables if necessary.
 *
 * If VARARGS is used, the list of input tables may be relaced by a
 * single array of fut_itbl_t pointers.  This array must then be followed
 * by a fut_gtbl_ptr_t and a fut_otbl_ptr_t.
 */
fut_chan_ptr_t fut_new_chan (int32 iomask, ...)
{
	KCMS_VA_ARG_PTR		ap;	/* arg pointer */
	char_p	vap;	/* for FUT_VARARGS */
	fut_itbl_ptr_t			itbl[FUT_NCHAN];
	fut_otbl_ptr_t			otbl;
	fut_gtbl_ptr_t			gtbl;
	fut_chan_ptr_t		chan;
	int			imask;
	int			i;

					/* get input mask */
	imask = (int)FUT_IMASK(iomask);

					/* get args specified by imask */
	KCMS_VA_START(ap,vap,iomask);

	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));
	for ( i=0; i<FUT_NCHAN; i++ ) {
		itbl[i] = (imask & FUT_BIT(i)) ? KCMS_VA_ARG(ap,vap,fut_itbl_ptr_t) : 0;
	}

					/* always get the next two args directly
					   from the variable arg list */
	KCMS_VA_LIST (ap, vap);

	gtbl = KCMS_VA_ARG(ap, vap, fut_gtbl_ptr_t);
	otbl = KCMS_VA_ARG(ap, vap, fut_otbl_ptr_t);

	KCMS_VA_END(ap,vap);

				/* allocate and clear the fut_chan_t structure */
	chan = fut_alloc_chan ();
	if ( chan == NULL )
		return (NULL);

				/* check for valid grid and output tables */
	if ( ! IS_GTBL(gtbl) || (otbl != 0 && ! IS_OTBL(otbl)) ) {
		DIAG("fut_new_chan: invalid grid or output table.\n", 0);
		fut_free_chan (chan);
		return (NULL);
	}
				/* get required input channels from gtbl */
	chan->imask = fut_gtbl_imask(gtbl);

				/* insert the required input tables */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( (chan->imask & FUT_BIT(i)) == 0 ) continue;
		if ( itbl[i] == FUT_NULL_ITBL ) {
			chan->itbl[i] = fut_new_itbl (gtbl->size[i], fut_iramp);
			if ( chan->itbl[i] == 0 ) {
				DIAG("fut_new_chan: can't create itbl.\n",0);
				fut_free_chan (chan);
				return (NULL);
			}
			chan->itblHandle[i] = chan->itbl[i]->handle;
		} else if ( ! IS_ITBL (itbl[i]) ) {
			DIAG("fut_new_chan: invalid input table.\n", 0);
			fut_free_chan (chan);
			return (NULL);
		} else if ( itbl[i]->size != gtbl->size[i] ) {
			DIAG("fut_new_chan: gtbl-itbl size mismatch.\n", 0);
			fut_free_chan (chan);
			return (NULL);
		} else {
			chan->itbl[i] = fut_share_itbl(itbl[i]);
			chan->itblHandle[i] = chan->itbl[i]->handle;
		}
	}

					/* insert grid and output tables */
	chan->gtbl = fut_share_gtbl (gtbl);
	chan->gtblHandle =  (IS_GTBL(chan->gtbl)) ?
													chan->gtbl->handle : FUT_NULL_HANDLE;
	chan->otbl = fut_share_otbl (otbl);
	chan->otblHandle = (IS_OTBL(chan->otbl)) ?
													chan->otbl->handle : FUT_NULL_HANDLE;

	return (chan);

} /* fut_new_chan */

/*
 * fut_new_itbl creates a new input table for one dimension of a grid table
 * of size 'size'.  Ifun must be a pointer to a function accepting a double
 * and returning a double, both in the range (0.0,1.0).  A pointer to the
 * newly allocated table is returned.  (If ifun is NULL, table is not
 * initialized).
 */
fut_itbl_ptr_t
fut_new_itbl(int size, double (*ifun) ARGS((double)))
{
	fut_itbl_ptr_t	itbl;

	if ( size <= 1 || size > FUT_GRD_MAXDIM ) {
		DIAG("fut_new_itbl: bad grid size (%d).\n", size);
		return (FUT_NULL_ITBL);
	}
					/* allocate input table structure */
	itbl = fut_alloc_itbl ();
	if ( itbl == FUT_NULL_ITBL ) {
		DIAG("fut_new_itbl: can't alloc input table struct.\n", 0);
		return (FUT_NULL_ITBL);
	}

	itbl->size = size;

					/* allocate the table */
	itbl->tbl = fut_alloc_itbldat(ITBLMODE8BIT);
	if ( itbl->tbl == NULL ) {
		DIAG("fut_new_itbl: can't alloc input table array.\n", 0);
		fut_free_itbl (itbl);
		return (FUT_NULL_ITBL);
	}
	itbl->tblHandle = getHandleFromPtr ((fut_generic_ptr_t)itbl->tbl);

	/* compute the input table entries */
	if ( ! fut_calc_itbl (itbl, ifun) ) {
		/* Note: fut_calc_itbl prints message on error */
		fut_free_itbl (itbl);
		return (FUT_NULL_ITBL);
	}

	return (itbl);

} /* fut_new_itbl */

/*
 * fut_new_otbl creates a new output table for one channel of a fut.
 * Ofun must be a pointer to a function accepting a fut_gtbldat_t in the
 * range (0,FUT_GRD_MAXVAL) and returning a fut_otbldat_t in the same
 * interval.  A pointer to the newly allocated table is returned.
 * (If ofun is NULL, table is not intialized!).
 */
fut_otbl_ptr_t
fut_new_otbl (fut_otbldat_t	(*ofun) ARGS((fut_gtbldat_t)))
{
	fut_otbl_ptr_t	otbl;

					/* allocate output table structure */
	otbl = fut_alloc_otbl();
	if ( otbl == FUT_NULL_OTBL ) {
		DIAG("fut_new_otbl: can't alloc output table struct.\n", 0);
		return (FUT_NULL_OTBL);
	}

					/* allocate the table */
	otbl->tbl = fut_alloc_otbldat();
	if ( otbl->tbl == NULL ) {
		DIAG("fut_new_otbl: can't alloc output table array.\n", 0);
		fut_free_otbl (otbl);
		return (FUT_NULL_OTBL);
	}
	otbl->tblHandle = getHandleFromPtr ((fut_generic_ptr_t)otbl->tbl);

					/* compute the output table entries */
	if ( ! fut_calc_otbl (otbl, ofun) ) {
		/* Note: fut_calc_otbl prints message on error */
		fut_free_otbl (otbl);
		return (FUT_NULL_OTBL);
	}

	return (otbl);

} /* fut_new_otbl */

/*
 * fut_new_gtbl creates a new grid table and optionally intializes it.
 * The input channels defined for the grid are specified in the input
 * channel mask portion of iomask.  Each input defined must have a size
 * specified in the variable arglist: sx, sy, sz, ... .
 * Gfun must be a pointer to a function accepting from zero to three
 * doubles (depending on values of sx, sy, and sz) in the range (0.0,1.0)
 * and returning a fut_gtbldat_t in the range (0,FUT_GRD_MAXVAL).
 * A pointer to the newly allocated table is returned if there were no
 * errors.  (If gfun is NULL, the table is not initialized).
 *
 * If FUT_VARARGS is used, the size list may be specified by a single  * (int) array.
 */
fut_gtbl_ptr_t
	fut_new_gtblA (int32 iomask, fut_gtbldat_t (*gfun)(double FAR*), int32 FAR* dimList)
{
fut_gtbl_ptr_t	gtbl;
int32			imask, i, dim_size, grid_size;

					/* get input mask */
	imask = (int)FUT_IMASK(iomask);

					/* allocate grid table structure */
	gtbl = fut_alloc_gtbl ();
	if ( gtbl == FUT_NULL_GTBL ) {
		DIAG("fut_new_gtblA: can't alloc grid table struct.\n", 0);
		return (FUT_NULL_GTBL);
	}
	gtbl->handle = getHandleFromPtr ((fut_generic_ptr_t)gtbl);

	/* get sizes from dimList */
	grid_size = 1;
	for ( i=0; i<FUT_NCHAN; i++ ) {
		dim_size = (imask & FUT_BIT(i)) ? dimList[i] : 1;
		if ( dim_size <= 0 ) {
			dim_size = 1;		/* make sure > 0 */
		}
		gtbl->size[i] = (int16)dim_size;
		grid_size *= (int32)dim_size;
	}

					/* check for valid grid size */
	if ( grid_size <= 0 || grid_size > FUT_GRD_MAX_ENT ) {
		DIAG("fut_new_gtblA: bad grid table size (%d).\n", grid_size);
		fut_free_gtbl(gtbl);
		return (FUT_NULL_GTBL);
	}
	gtbl->tbl_size = (int32)grid_size * (int32)sizeof(fut_gtbldat_t);

					/* allocate grid table */
	gtbl->tbl = fut_alloc_gtbldat((int32)grid_size);
	if ( gtbl->tbl == NULL ) {
		DIAG("fut_new_gtblA: can't alloc grid table array.\n", 0);
		fut_free_gtbl(gtbl);
		return (FUT_NULL_GTBL);
	}

	gtbl->tblHandle = getHandleFromPtr ((fut_generic_ptr_t)gtbl->tbl);

					/* compute the grid table entries */
	if ( ! fut_calc_gtblA (gtbl, gfun) ) {
		fut_free_gtbl(gtbl);
		return (FUT_NULL_GTBL);
	}

	return (gtbl);

} /* fut_new_gtblA */

/* create a new fut which has shared input tables and calculates the identity function */

fut_ptr_t
	fut_new_empty (	int32	ndim,
					int32_p	dim,
					int32	nchan)
{
fut_ptr_t		fut = FUT_NULL;
fut_generic_ptr_t tbls[FUT_NICHAN+FUT_NOCHAN+FUT_NOCHAN];
int32 			iomask = 0;
int32 			i1, i2;
int32 			dimTbl[FUT_NICHAN];
fut_itbl_ptr_t	itblP;
fut_gtbl_ptr_t	gtblP;
fut_otbl_ptr_t	otblP;

	if ((ndim > FUT_NICHAN) || (nchan > FUT_NOCHAN)) {
		return FUT_NULL;
	}
	
	for (i1 = 0; i1 < (FUT_NICHAN+FUT_NOCHAN+FUT_NOCHAN); i1++) {
		tbls[i1] = NULL;
	}
	
	for (i1 = 0, i2 = 0; i1 < ndim; i1++) {
		iomask |= FUT_IN(FUT_BIT(i1));
		itblP = fut_new_itbl((int)dim[i1], fut_iramp);	/* make new input table */
		if (itblP == FUT_NULL_ITBL) {
			goto GetOut;
		}
		itblP->id = fut_unique_id();				/* these usually do not stay ramps */
		tbls[i2++] = (fut_generic_ptr_t)itblP;		/* collect input tables */
	}

/* Move the table into an int array for typedef compatibility reasons. */
	for (i1 = 0; i1 < ndim; i1++) {
		dimTbl[i1] = (int)dim[i1];
	}

	for (i1 = 0; i1 < nchan; i1++) {
		iomask |= FUT_OUT(FUT_BIT(i1));

		gtblP = fut_new_gtblA (FUT_IMASK(iomask), FUT_NULL_GFUN, dimTbl);
		if (gtblP == FUT_NULL_GTBL) {
			goto GetOut;
		}
		tbls[i2++] = (fut_generic_ptr_t)gtblP;		/* collect grid tables */

		otblP = fut_new_otbl(fut_oramp);			/* make new output table */
		if (otblP == FUT_NULL_OTBL) {
			goto GetOut;
		}
		otblP->id = fut_unique_id();				/* these usually do not stay ramps */
		tbls[i2++] = (fut_generic_ptr_t)otblP;		/* collect output tables */
	}

	fut = fut_new (iomask | FUT_VARARGS, tbls);

												/* because the each of the tables was
													shared when it was included in the fut
													the reference count for each table is
													one greater than it should be.  Free
													each of the tables to adjust the 
													reference counts.					*/
													
GetOut:
	for (i1 = 0, i2 = 0; i1 < ndim; i1++) {
		if (tbls[i2] != NULL) {
			fut_free_itbl ((fut_itbl_ptr_t)tbls[i2++]);
		}
	}

	for (i1 = 0; i1 < nchan; i1++) {
		if (tbls[i2] != NULL) {
			fut_free_gtbl ((fut_gtbl_ptr_t)tbls[i2++]);
		}
		if (tbls[i2] != NULL) {
			fut_free_otbl ((fut_otbl_ptr_t)tbls[i2++]);
		}
	}

	return (fut);
}

/*
 * fut_defchan defines an output channel for a fut.  Returns FALSE(0) if
 * the output channel is already defined (or fut is NULL), TRUE(1)
 * otherwise.  The size of the grid table (if non-zero) must match those
 * of the corresponding input table.  If they do not, the channel remains
 * undefined and FALSE is returned.
 *
 * If a required input table is missing, the table will be shared
 * with the corresponding one from the list of common itbls.  If there
 * is no such table in the common list, a ramp table is created and
 * inserted into the common itbl list.
 *
 * Since fut_defchan is intended to be used for constructing futs with
 * shared input tables,  if an input table is supplied that conflicts with
 * a table in the common list, an error occurs.
 *
 * If FUT_VARARGS is used, the list of input tables may be specified in
 * a single array of fut_itbl_t pointers.
 */
int
fut_defchan(fut_ptr_t	fut,int32 iomask,...)
{
	KCMS_VA_ARG_PTR		ap;	/* arg pointer */
	char_p	vap;	/* for FUT_VARARGS */
	fut_itbl_ptr_t			itbl[FUT_NICHAN];
	fut_otbl_ptr_t			otbl;
	fut_gtbl_ptr_t			gtbl;
	fut_chan_ptr_t		chan;
	int			imask;
	int			i;

					/* check for valid fut */
	if ( ! IS_FUT(fut) )
		return (0);
					/* get input mask */
	imask = (int)FUT_IMASK(iomask);

					/* get args specified by imask */
	KCMS_VA_START(ap,vap,iomask);

	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));
	for ( i=0; i < FUT_NICHAN; i++ ) {
		if ( (imask & FUT_BIT(i)) != 0 )
				/* if itbl is in arglist, use it */
			itbl[i] = KCMS_VA_ARG(ap, vap, fut_itbl_ptr_t);
		else
				/* if in shared itbl list, use that */
			itbl[i] = fut->itbl[i];
	}
					/* always get the next two args directly
					   from the variable arg list */
	KCMS_VA_LIST (ap, vap);

	gtbl = KCMS_VA_ARG(ap, vap, fut_gtbl_ptr_t);	/* gtbl must be present */
	otbl = KCMS_VA_ARG(ap, vap, fut_otbl_ptr_t);	/* otbl must be present */

	KCMS_VA_END(ap,vap);

	chan = fut_new_chan (	(int32)(FUT_IN (FUT_ALLIN) | FUT_VARARGS),
				(fut_itbl_ptr_t fut_far*)itbl, gtbl, otbl);
	if ( chan == NULL )
		return (0);

	/*
	 * If fut_new_chan created a new itbl (ramp), add it to the
	 * common list.  However, if an itbl in the chan differs from
	 * one in the common list, return an error.
	 */
	for ( i=0; i < FUT_NICHAN; i++ ) {
		if ( chan->itbl[i] == 0 ) {
			continue;
		} else if ( fut->itbl[i] == 0 ) {
			fut->itbl[i] = fut_share_itbl (chan->itbl[i]);
			fut->itblHandle[i] = chan->itblHandle[i];
		} else if ( fut->itbl[i] != chan->itbl[i] ) {
			DIAG("fut_defchan: conflicting itbls.\n", 0);
			fut_free_chan (chan);
			return (0);
		}
	}

					/* insert channel into fut */
	if ( ! fut_add_chan (fut, iomask, chan) ) {
		fut_free_chan (chan);
		return (0);
	}

	return (1);

} /* fut_defchan */

/*
 * fut_add_chan inserts a new output channel into a fut.
 * Unlike itbls, otbls, and gtbls, the channel structure is not sharable
 * and so the caller must not free the chan after this call.  (If the
 * passed channel structure needs to be saved, use fut_share_chan).
 * The iomask in this case simply tells which output channel is being
 * added, and if this channel already exists, an error (0) is returned.
 *
 * fut_add_chan is intended to be used in conjunction with fut_new_chan
 * to construct futs with independent input tables.  It does not update
 * the list of common input tables as does fut_new and fut_defchan and
 * should not be mixed with calls to fut_defchan.
 */
int
fut_add_chan (fut_ptr_t	fut, int32 iomask, fut_chan_ptr_t chan)
{
	int		ochan;

	if ( ! IS_FUT(fut) || (chan != FUT_NULL_CHAN && ! IS_CHAN(chan)) )
		return (0);

					/* get output channel no. */
	ochan = FUT_CHAN ((int)FUT_OMASK(iomask));

					/* prohibit redefinition of channel */
	if ( ochan >= FUT_NOCHAN || fut->chan[ochan] != 0 )
		return (0);
					/* insert channel into fut */
	fut->chan[ochan] = chan;
	fut->chanHandle[ochan] = (IS_CHAN(fut->chan[ochan])) ?
													fut->chan[ochan]->handle : FUT_NULL_HANDLE;

					/* update iomasks */
	if ( IS_CHAN(chan) ) {
		fut->iomask.out |= FUT_BIT(ochan);
		fut->iomask.in |= chan->imask;
	}

	return (1);

} /* fut_add_chan */

/*
 * fut_replace_chan replaces an existing output channel with a new one.
 * The old channel is freed and the new one is inserted.
 * Unlike itbls, otbls, and gtbls, the channel structure is not sharable
 * and so the caller must not free the chan after this call.  If the caller
 * needs to retain control of the channel he should use fut_share_chan
 * or fut_copy_chan.
 * The iomask in this case simply tells which output channel is being
 * added, and if this channel already exists, an error (0) is returned.
 *
 * fut_replace_chan currently does not try to share input or output
 * tables.  This may be added in the future.
 */
int
fut_replace_chan (fut_ptr_t fut, int32 iomask, fut_chan_ptr_t chan)
{
	int		i;
	int		ochan;

	if ( ! IS_FUT(fut) || (chan != FUT_NULL_CHAN && ! IS_CHAN(chan)) )
		return (0);

					/* get output channel no. */
	ochan = FUT_CHAN ((int)FUT_OMASK(iomask));

					/* check for invalid channel */
	if ( ochan >= FUT_NOCHAN )
		return (0);

					/* free existing channel and
					   insert new channel into fut */
	fut_free_chan (fut->chan[ochan]);
	fut->chan[ochan] = chan;

	if ( IS_CHAN(chan) ) {
		fut->chanHandle[ochan] = chan->handle;

	/* add itbls to common list if they don't already exist.	
	 * This is a KLUDGE since either all itbls should be common or
	 * none at all.  It is currently necessary for ds_update_colormap
	 * in the prophecy display server
	 */
		for ( i=0; i<FUT_NICHAN; i++ ) {
			if ( !IS_ITBL(fut->itbl[i]) ) {
				fut->itbl[i] = fut_share_itbl(chan->itbl[i]);
				fut->itblHandle[i] = (IS_ITBL(fut->itbl[i])) ?
													fut->itbl[i]->handle : FUT_NULL_HANDLE;
			}
		}
	}

					/* reset the iomask of the fut */
	return (fut_reset_iomask (fut));

} /* fut_replace_chan */

/*
 * fut_swap_chan exchanges the channels (input or output) of a fut
 * according to the value of iomask.  This essentially re-lables the
 * indices of the channels.
 *
 * imask - specifies which input channels to exchange, the first and last
 *	   bits set in the mask define the two channels to swap.
 * omask - specifies which output channels to exchange, the first and last
 *	   bits set in the mask define the two channels to swap.
 *
 * fut_swap_chan currently swaps input channels on separable futs.
 * To do this on non-separable futs may require re-ordering the grid
 * tables.
 *
 * WARNING: since this modifies the structure of the grid table, you had
 * better be certain that the grid table is not shared.  In fact, if the
 * grid table has a non-zero reference count, an error is returned.
 */
int
fut_swap_chan (fut_ptr_t fut, int32 iomask)
{
	int			i, c1, c2;
	fut_chan_ptr_t		chan;
	fut_itbl_ptr_t		itbl;
	fut_gtbl_ptr_t		gtbl;
	int16			size;
	fut_handle				hand;
	int	omask =			(int)FUT_OMASK(iomask);
	int	imask =			(int)FUT_IMASK(iomask);

	if ( ! IS_FUT(fut) )
		return (0);
					/* first do the output channel swap,
					   this is the easiest. */
	if ( omask != 0   &&
	     (c1 = fut_first_chan (omask)) != (c2 = fut_last_chan (omask)) ) {
		chan = fut->chan[c1];
		fut->chan[c1] = fut->chan[c2];
		fut->chan[c2] = chan;
		hand = fut->chanHandle[c1];
		fut->chanHandle[c1] = fut->chanHandle[c2];
		fut->chanHandle[c2] = hand;
	}

					/* now do the input channel swap */
	if ( imask != 0   &&
	     (c1 = fut_first_chan (imask)) != (c2 = fut_last_chan (imask)) ) {
		itbl = fut->itbl[c1];						/* do common itbls */
		fut->itbl[c1] = fut->itbl[c2];
		fut->itbl[c2] = itbl;
		hand = fut->itblHandle[c1];
		fut->itblHandle[c1] = fut->itblHandle[c2];
		fut->itblHandle[c2] = hand;

						/* do each defined chan */
		for ( i=0; i<FUT_NOCHAN; i++ ) {
			if ( (chan = fut->chan[i]) == FUT_NULL_CHAN )
				continue;
						/* return error if shared */
			gtbl = chan->gtbl;
			if ( gtbl->ref > 0 )
				return (0);
							/* swap input tables */
			itbl = chan->itbl[c1];
			chan->itbl[c1] = chan->itbl[c2];
			chan->itbl[c2] = itbl;
			hand = chan->itblHandle[c1];
			chan->itblHandle[c1] = chan->itblHandle[c2];
			chan->itblHandle[c2] = hand;
							/* swap grid sizes */
			size = gtbl->size[c1];
			gtbl->size[c1] = gtbl->size[c2];
			gtbl->size[c2] = size;
							/* recompute input mask */
			chan->imask = fut_gtbl_imask (chan->gtbl);
		}

	}
					/* reset the iomask of the fut */
	return (fut_reset_iomask (fut));

} /* fut_swap_chan */

