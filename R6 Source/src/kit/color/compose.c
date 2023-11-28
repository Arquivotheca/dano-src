/*********************************************************************/
/*
	compose.c

	Copyright:	(C) 1991-1997 by Eastman Kodak Company, all rights reserved.

	SCCS Revision:
	@(#)compose.c	13.12	10/09/97
*/
/*********************************************************************/

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

#include "fut.h"
#include "fut_util.h"		/* internal interface file */
#include "interp.h"		/* for input table interpolation */

static int	input_gtbl_ij ARGS((fut_chan_ptr_t fut_far*,
				fut_chan_ptr_t fut_far*,int,
				fut_gtbl_ptr_t (fut_far*)[FUT_NICHAN],
				int,int));

/*
 * fut_comp composes one fut with another.
 *
 * Omask specifies which channels are to be defined for the result fut.
 *	An empty omask indicates use omask of fut1.
 * Pmask specifies those channels of fut0 which are to be passed on to the
 *	result fut, in the event that fut1 cannot supply a channel stated
 *	in omask.
 * Imask specifies those input channels for which default constants are
 *	provided in the event fut0 can't supply a channel required by fut1
 *	to produce the channels specified in omask.  The default values,
 *	in 12 bit precision, follow the iomask in the arglist in the proper
 *	order (e.g. XYZT, XY, XZT).
 * Order indicates the desired interpolation order to be performed in
 *	evaluating fut1.  (The result fut will have the same order as
 *	fut0, since their input grids are the same).
 * INPLACE may be set indicating to place result in fut0.
 * VARARGS may be used to specify an (int) array of 12-bit constants.
 */
fut_ptr_t
fut_comp (fut_ptr_t fut1, fut_ptr_t fut0, int32 iomask, ...)
{
	int				ok = 1;
	int				i, j;
	fut_ptr_t		fut2;
	fut_gtbl_ptr_t	ingtbl[FUT_NOCHAN][FUT_NICHAN];
	fut_gtbl_ptr_t	(fut_far *in_grid_tbl)[FUT_NICHAN];
	int32			n[FUT_NOCHAN];
	int				c[FUT_NICHAN];
	int				omask, imask, pmask, order, inplace;
	iotbl_cache_t	iotblCache [CACHE_SIZE];	/* this is the cache */


				/* extract component masks from iomask */
	imask = (int)FUT_IMASK(iomask);	/* any c[j]'s ? */
	omask = (int)FUT_OMASK(iomask);	/* which output chans? */
	pmask = (int)FUT_PMASK(iomask);	/* which ones allowed to pass through? */
	inplace = (int)FUT_IPMASK(iomask);	/* do it in place (on fut0)? */
	order = (int)FUT_ORDMASK(iomask);	/* which interpolation to use? */

	if ( ! IS_FUT(fut0) )
		return (FUT_NULL);

				/* treat a NULL fut1 as an identity fut */
	if ( fut1 == FUT_NULL ) {
		return (inplace ? fut0 : fut_copy(fut0));
	} else if ( ! IS_FUT(fut1) ) {
		return (FUT_NULL);
	}

					/* unpack constants from vararglist */
	if ( imask == 0 ) {
		for ( j=0; j<FUT_NICHAN; j++) {
			c[j] = 0;
		}
	}
	else {
		KCMS_VA_ARG_PTR		ap;	/* arg pointer */
		char_p	vap;	/* for FUT_VARARGS */

		KCMS_VA_START(ap,vap,iomask);
		KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));

		for ( j=0; j<FUT_NICHAN; j++) {
			c[j] = (imask & FUT_BIT(j)) ? KCMS_VA_ARG(ap, vap, int) : 0;
		}

		KCMS_VA_END(ap,vap);
	}

				/* adjust masks for iomask_check below */
	imask |= fut0->iomask.out;	/* available inputs for fut1 */
	pmask &= fut0->iomask.out;	/* required for "pass through" */
	if ( omask == 0 )		/* required outputs (0 means all) */
		omask = fut1->iomask.out;
	if ( order == FUT_DEFAULT )
		order = fut1->iomask.order;

					/* see if fut0 (and supplied defaults)
					   can provide required inputs to fut1 */
	iomask = FUT_OUT(omask) | FUT_IN(imask) | FUT_PASS(pmask);
	if ( ! fut_iomask_check (fut1, iomask) )
		return (FUT_NULL);


	/* fut1 will be used to process the grid tables of fut0, placing the
	 * results in the grid tables of fut2.  Fut0's grid table data must first
	 * be passed through its output tables before sending it through fut1's
	 * input tables.  This is accomplished more efficiently by composing
	 * fut1's input tables with fut0's output tables and using these directly
	 * on fut0 grid data rather than the normal input tables.  This
	 * composition is performed below in the main processing loop
	 *
	 * Create the result fut (fut2) which will be the composition of fut1
	 * and fut0.  Fut2 will inherit the input tables of fut0 and the output
	 * tables of fut1.  Its grid data will be in the same color coordinates
	 * as fut1's. If fut2 is not NULL, then we will restructure it only as
	 * required to conform to the composition.  This may save some time.
	 */
	fut2 = fut_new (FUT_IN(FUT_ALLIN)|FUT_VARARGS, fut0->itbl);
	if ( fut2 == FUT_NULL )
		return (FUT_NULL);


	/* for each desired channel i in fut2, create a new grid table.  The
	 * dimensions of each new grid table are derived from fut0 and fut1
	 * like so:  for every input required for channel i of fut1, form the
	 * union of the input sets of all corresponding fut0 outputs.
	 *
	 * We then expand the dimensions of all fut0 grid tables required
	 * as inputs to fut1 channel i, to match the dimensions
	 * of the new grid table.  All this is necessary since there must be a
	 * one to one correspondence between points input from fut0's grid
	 * table and points output to fut2's grid table.
	 */

					/* null all input grid table pointers */
	bzero ((fut_generic_ptr_t)ingtbl, (int32)sizeof(ingtbl));

	for ( i=0; i<FUT_NOCHAN && ok; i++ ) {
		int32		size[FUT_NICHAN];
		fut_gtbl_ptr_t	gtbl;
		int32	imask1, imask2;

					/* is this output channel specified? */
		if ( ! (omask & FUT_BIT(i)) ) {
			n[i] = 0;
			continue;
		}
					/* if a specified output is to be passed
					   through from fut0, do that here */
		if ( fut1->chan[i] == FUT_NULL_CHAN &&
		     fut0->chan[i] != FUT_NULL_CHAN ) {

			ok = fut_defchan (fut2, FUT_OUT(FUT_BIT(i)),
					  fut0->chan[i]->gtbl,
					  fut0->chan[i]->otbl);

			n[i] = 0;
			continue;
		}

		/*
		 * At this point we know that (fut1->chan[i] != 0).  We also
		 * have determined (from iomask_check above) that either
		 * fut0->chan[j] != 0 or a constant c[j] was provided.
		 */
		imask1 = fut1->chan[i]->imask;
		imask2 = 0;		/* determine input mask for this channel */

		for ( j=0; j<FUT_NICHAN; j++ ) {
			if ( (imask1 & FUT_BIT(j)) && (fut0->chan[j] != 0) )
				imask2 |= fut0->chan[j]->imask;
		}
					/* determine required dimensions from mask */
		n[i] = 1;
		for ( j=0; j<FUT_NICHAN; j++ ) {
			size[j] = (imask2 & (int32)FUT_BIT(j)) ? fut0->itbl[j]->size : 1;
			n[i] *= size[j];
		}
						/* create the new grid table and
						   insert it along with fut1's
						   output table into fut2 */
		gtbl = fut_new_gtblA (FUT_IN(FUT_ALLIN),
				     FUT_NULL_GFUN, size);
		ok = fut_defchan (fut2, FUT_OUT(FUT_BIT(i)),
					gtbl,
					fut1->chan[i]->otbl);
		fut_qfree_gtbl (gtbl);

						/* expand all input grid table
						   dimensions to the same size. */
		in_grid_tbl = ingtbl;
		for ( j=0; j<FUT_NICHAN && ok; j++ ) {
			if ( (imask1 & FUT_BIT(j)) != 0 ) {
				ok = input_gtbl_ij ((fut_chan_ptr_t fut_far*)fut2->chan,
						(fut_chan_ptr_t fut_far*)fut0->chan,
						c[j],
						in_grid_tbl,
						i, j);
			}

		}
	}

#ifndef FUT_COMPOSE_CACHE
	iotblInitC((iotbl_cache_ptr_t)&iotblCache);	/* flush the iotbl cache here until it can be done globally */
#endif
					/* now we are ready to pass fut0's
					   grid tables through fut1 */
	for ( i=0; i<FUT_NOCHAN && ok; i++ ) {
		fut_gtbldat_ptr_t	 outdat;
		fut_gtbldat_ptr_t	 indat[FUT_NICHAN];
		fut_itbldat_ptr_t	 iotbl[FUT_NICHAN];
		int32		 iomask1;

		if ( n[i] == 0 )
			continue;	/* output not required */

		/*
		 * pre-compose fut0's otbls with fut1's itbls.  This is
		 * much more efficient than trying to do it on the fly
		 * using the fut_interp_fun functions.
		 * NOTE: we rely on the caching ability of fut_comp_iotbl()
		 * to avoid redundantly recomputing the same iotbl.
		 */
		for ( j=0; j<FUT_NICHAN && ok; j++ ) {
					/* if this input channel is not required
					   for the output channel then skip it */
			if ( ingtbl[i][j] == 0 ) continue;

					/* get input grid table pointers */
			indat[j] = ingtbl[i][j]->tbl;

			iotbl[j] = fut_comp_iotblC (FUT_CHAN_ITBL(fut1->chan[i], j),
						   FUT_CHAN_OTBL(fut0->chan[j]), (iotbl_cache_ptr_t)&iotblCache);

			ok = (iotbl[j] != 0);
		}

		if ( ok ) {
					/* get output grid table pointer */
			outdat = fut2->chan[i]->gtbl->tbl;


					/* evaluate fut1 */
			iomask1 = FUT_IN(fut1->chan[i]->imask) |
				  FUT_ORDER(order) | FUT_12BITS;

			ok = fut_eval_chan (fut1, i, iomask1, n[i],
					    (fut_generic_ptr_t fut_far*)indat,
			                    (fut_generic_ptr_t)outdat,
			                    (fut_itbldat_ptr_t fut_far*)iotbl);
		}
	}

/* must always free up the input grid tables, even if an error occurred! */
	for ( i=FUT_NOCHAN; --i >= 0; ) {
		for ( j=FUT_NICHAN; --j >= 0; ) {
			fut_qfree_gtbl (ingtbl[i][j]);
		}
	}
#ifndef FUT_COMPOSE_CACHE
	iotblFlushC((iotbl_cache_ptr_t)&iotblCache);	/* flush the iotbl cache here until it can be done globally */
#endif

					/* check for errors */
	if ( !ok )
		return (fut_free (fut2));

	/*
	 * If "in place", exchange all data between fut2 and fut0
	 * (except for idstr and interpolation order) and free fut2.
	 */
	if ( inplace ) {
		fut_exchange (fut2, fut0);
		fut_free (fut2);
		return (fut0);
	}

	return (fut2);

} /* fut_comp */

/*
 * fut_comp_iotbl composes an output table with an input table, allocating
 * the resulting table if necessary (if iotbl is NULL).  The composite table
 * has the same data width and format as an input table, but is the size of
 * an output table.
 *
 * We use a caching scheme based on the itbl and otbl unique id numbers
 * to reduce the amount of time spent composing tables in a typical
 * application.  (e.g. in an interactive loop, the required tables will
 * likely already exist.)
 */

/* caching support routines */

/* flush the iotbl cache */
void
	iotblInitC (iotbl_cache_ptr_t iotblCache)
{
	iotbl_cache_t* p1, * pmax;

	for ( p1 = iotblCache, pmax = p1 + CACHE_SIZE; p1 < pmax; p1++ ) {
		p1->iotbl = NULL;		/* mark entry empty */
		p1->lru_count = 0;
		p1->itbl_id = 0;
		p1->otbl_id = 0;
	}
}



/* flush the iotbl cache */
void
	iotblFlushC (iotbl_cache_ptr_t iotblCache)
{
	iotbl_cache_t* p1, * pmax;

	for ( p1 = iotblCache, pmax = p1 + CACHE_SIZE; p1 < pmax; p1++ ) {
		if ( p1->iotbl != 0 ) {
					/* release the memory */
			fut_mfree((fut_generic_ptr_t)p1->iotbl, "i");
		}

		p1->iotbl = NULL;		/* mark entry empty */
		p1->lru_count = 0;
		p1->itbl_id = 0;
		p1->otbl_id = 0;
	}
}


/* this is the real thing */
fut_itbldat_ptr_t
	fut_comp_iotblC (fut_itbl_ptr_t itbl, fut_otbl_ptr_t otbl, iotbl_cache_ptr_t iotblCache)
{
	int32		iid, oid;
	iotbl_cache_t* lru_ent = iotblCache;

	oid = (otbl==0) ? (int32)(-2) : otbl->id;	/* give NULL otbl special id */
	iid = itbl->id;

					/* look through cache */
	if ( iid == 0 || oid == 0 ) {
					/* if either id is 0 (always unique)
					   don't bother looking in cache.  Just
					   find least recently used table  */
		iotbl_cache_t	* p, * pmax;
		int		lru_count_max = 0;

		for ( p = iotblCache, pmax = p + CACHE_SIZE; p < pmax; p++ ) {
			if ( p->lru_count > lru_count_max ) {
				lru_ent = p;
				lru_count_max = p->lru_count;
			}
			p->lru_count++;
		}

	} else {			/* if neither id is 0, check for a match. */

		iotbl_cache_t	* p, * pmax;
		int		lru_count_max = 0;

		for ( p = iotblCache, pmax = p + CACHE_SIZE; p < pmax; p++ ) {

			if ( (p->itbl_id == iid) && (p->otbl_id == oid) ) {
				break;
			} else if ( p->lru_count > lru_count_max ) {
				lru_ent = p;
				lru_count_max = p->lru_count;
			}
			p->lru_count++;
		}
					/* if found a matching entry in the
					   cache return the existing iotbl */
		if ( p < pmax) {
					/* finish incrementing remaining
					   cache entry lru_counts */
			while ( --pmax > p )
				pmax->lru_count++;

			p->lru_count = 0;
			return (p->iotbl);
		}
	}

	/*
	 * At this point, we have determined the least recently used
	 * cache entry (no cache hit).  Save ids for next time, allocate
	 * a buffer if necessary, and do the table composition
	 */
	lru_ent->lru_count = 0;		/* zero the lru_count */
	lru_ent->itbl_id = iid;		/* save ids for next time */
	lru_ent->otbl_id = oid;

					/* if first time for this buffer
					   must allocate iotbl */
	if ( lru_ent->iotbl == 0 ) {
		lru_ent->iotbl = (fut_itbldat_ptr_t) fut_malloc
				((int32)FUT_OUTTBL_ENT*sizeof(fut_itbldat_t), "i");
		if ( lru_ent->iotbl == 0 )
			return (0);
	}
					/* use linear interpolation for table composition */
	{
		int32		t, x;
		fut_itbldat_ptr_t	p;
		fut_itbldat_ptr_t	ioptr = lru_ent->iotbl;
		fut_itbldat_ptr_t	iptr = itbl->tbl;
		int			i = FUT_OUTTBL_ENT;
		fut_otbldat_ptr_t	optr;

		if ( otbl == 0 ) {
						/* NULL otbl is ramp */
			for ( x=0; x<FUT_OUTTBL_ENT; x++ ) {
				*ioptr++ = FUT_ITBL_INTERP (iptr,x,t,p);
			}

		} /* 8bit */
		else if (itbl->tblFlag == 0) {
			optr = otbl->tbl;
			while ( --i >= 0 ) {
				x = *optr++;
				*ioptr++ = FUT_ITBL_INTERP (iptr,x,t,p);
			}
		} /* 12bit */
		else {
			optr = otbl->tbl;
			while ( --i >= 0 ) {
				x = *optr++;
				*ioptr++ = itbl->tbl2[x];
			}
		}
	}

	return (lru_ent->iotbl);

} /* fut_comp_iotbl */



fut_itbldat_ptr_t
	fut_expand8to12_itbl (fut_itbl_ptr_t itbl)
{
	int32		t, x;
	fut_itbldat_ptr_t	p, desPtr, bigPtr, srcPtr;
	fut_itbldat_ptr_t	newTblPtr;
	fut_itbldat_ptr_t	oldTblPtr = itbl->tbl;
	fut_itbldat_t		interpData;

	/* allocate the new big itbl */
	newTblPtr = fut_alloc_itbldat (ITBLMODE12BIT);
	if (newTblPtr == 0) {
		return (0);
	}

	bigPtr = newTblPtr + (FUT_INPTBL_ENT+1);

	/* copy old small table into new allocated small table */
	srcPtr = oldTblPtr;
	desPtr = newTblPtr;
	for (x = 0; x < (FUT_INPTBL_ENT+1); x++) {
		*desPtr++ = *srcPtr++;
	}

	/* use linear interpolation for table composition */
	desPtr = bigPtr;
	for ( x=0; x<FUT_INPTBL_ENT2; x++ ) {
		interpData = FUT_ITBL_INTERP (oldTblPtr,x,t,p);
		*desPtr++ = interpData;
	}

	/* write to the last entry (4097)*/
	*desPtr = interpData;

	/* fill the itbl structure */
	itbl->tbl2 = bigPtr;
	itbl->tbl = newTblPtr;
	itbl->tblHandle = getHandleFromPtr((fut_generic_ptr_t)newTblPtr);
	itbl->tblFlag = FUT_INPTBL_ENT;

	/* free the old itbl */
	fut_mfree ((fut_generic_ptr_t)oldTblPtr, "i");

	return (itbl->tbl2);

} /* fut_comp_iotbl */

/*
 * input_gtbl_ij creates the j-th input grid table (from fut0) for the
 * i-th output grid table evaluation (to fut2).  The grid tables are
 * expanded if necessary by fut_expand_gtbl() which will replicates rows,
 * columns, or planes of a lower dimensional table to make a higher
 * dimensional one.  (To compute the grid tables for fut2, we need to have
 * all the input grid tables the same size and dimensionality.)
 *
 * Chan2 is the list of channel pointers from fut2, chan0 is the list of
 * channel pointers from fut0, and c is a constant for creating a uniform
 * grid table if one is not defined in fut0.  The resulting gtbl pointer
 * is placed in ingtbl[i][j] and must be eventually freed.
 *
 * If fut0's grid tables are already the correct size (a likely
 * possibility), we simply use fut_share_gtbl() to get the required grid
 * table.  Otherwise, if one of fut0's grid tables has previously been
 * expanded to the same dimensions (for another output channel), we find
 * it and use fut_share_gtb() on it.  Otherwise, we perform the
 * expansion.
 */
static
int
input_gtbl_ij (fut_chan_ptr_t fut_far* chan2, fut_chan_ptr_t fut_far* chan0,
				int c, fut_gtbl_ptr_t (fut_far* ingtbl)[FUT_NICHAN],
				int i, int j)
{
#if !defined (FUT_ICC_ONLY)
	fut_gtbl_ptr_t	gtbl;
	int	k;
#else
	if (c) {}	/* input parameter is unused */
#endif

					/* if fut0 grid j is already the
					   same size as output grid i,
					   use it for input */
	if ( (chan0[j] != FUT_NULL_CHAN) &&
	     (chan0[j]->imask == chan2[i]->imask) ) {
		ingtbl[i][j] = fut_share_gtbl(chan0[j]->gtbl);
		return (ingtbl[i][j] != FUT_NULL_GTBL);
	}
#if !defined (FUT_ICC_ONLY)

					/* otherwise, look back to see if fut0
					   grid j has been previously expanded
					   (as input to another channel, k) */
	for ( k=i; --k >= 0; ) {
		if ( (chan2[k] != FUT_NULL_CHAN) &&
		     (chan2[k]->imask == chan2[i]->imask) ) {
			ingtbl[i][j] = fut_share_gtbl(ingtbl[k][j]);
			return (ingtbl[i][j] != FUT_NULL_GTBL);
		}
	}
					/* if search failed, must expand.
					   if input grid j is present we
					   expand it.  Otherwise we create
					   a 0 dimensional (constant) grid
					   to expand. */
	if ( chan0[j] != FUT_NULL_CHAN ) {
		gtbl = fut_share_gtbl (chan0[j]->gtbl);
	} else {
		gtbl = fut_new_gtblA (FUT_IN(0), FUT_GCONST((fut_gtbldat_t)c), NULL);
	}

	ingtbl[i][j] = fut_expand_gtbl (gtbl, chan2[i]->gtbl->size);

	fut_free_gtbl (gtbl);

	return (ingtbl[i][j] != FUT_NULL_GTBL);
#else
	return 0;	/* error - this version should never 
			   need to expand gtbls */
#endif

} /* input_gtbl_ij */


