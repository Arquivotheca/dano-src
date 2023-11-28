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
/*
 * fut_evaluators:
 *
 *   fut_eval8 - evaluates a fut at a single point in 8 bit precision
 *   fut_eval12- evaluates a fut at a single point in 12 bit precision
 *   fut_eval  - evaluates a fut at a single point in 8 or 12 bit precision
 *
 *   fut_eval_array8 - evaluates a fut at a list of points (8bit)
 *   fut_eval_array12- evaluates a fut at a list of points (12bit)
 *   fut_eval_array  - evaluates a fut at a list of points (8 or 12 bit)
 *
 *   fut_eval_chan - evaluates a single channel of a fut.
 */

#include "fut.h"
#include "fut_util.h"	/* internal interface file */

void cpeval (fut_ptr_t, int32, int32, fut_generic_ptr_t fut_far*, fut_generic_ptr_t, int32);	/* use the CP interpolator */


/*
 * fut_eval_array evaluates a fut at a set of n points specified by a collection
 * of input arrays, and places the results into a collection of output arrays.
 * The number of input and output arrays are determined by iomask.
 * The input arrays precede the output arrays in the arglist and always follow
 * the order x,y,z,t (e.g. xyzt, xyt, xyz, yzt, xy, xz, xt, yz, yt, etc).
 *
 * Imask specifies which input channels are present in the arglist
 * Omask specifies which output channels are present in the arglist
 * Order specifies the interpolation order to use (overiding fut's default)
 * FUT_VARARGS may be used to specify 2 arrays of pointers:
 *	the first contains the list of input arrays,
 *	the second contains the list of output arrays.
 * FUT_12BITS may used to specify evaluation of 12bit instead of 8bit data.
 */

int
fut_eval_array (fut_ptr_t fut, int32 iomask, int32 n, KCMS_VA_ARG_PTR ap)
{
	fut_generic_ptr_t	out, in[FUT_NICHAN];
	int		imask;
	int		omask;
	int		dattype;
	int		i;
	char_p vap=NULL;	/* for FUT_VARARGS */

	if ( ! IS_FUT (fut) )
		return (0);
					/* by default, all inputs are
					   avaliable for pass-thru */
	iomask |= FUT_PASS(FUT_IMASK(iomask));
	if ( FUT_ORDMASK(iomask) == FUT_DEFAULT )
		iomask |= FUT_ORDER(fut->iomask.order);

					/* check iomask and unpack */
	if ( ! fut_iomask_check (fut,iomask) )
		return (0);

	imask = (int)FUT_IMASK(iomask);
	omask = (int)FUT_OMASK(iomask);
	dattype = (int)FUT_12BMASK(iomask);	/* 1 = 12bit, 0 = 8 bit */

/* if args are specified by arrays, get next array containing inputs. */
	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));

					/* collect input array pointers */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( (imask & FUT_BIT(i)) == 0 ) {
			in[i] = NULL;
		} else {
			in[i] = KCMS_VA_ARG(ap, vap, fut_generic_ptr_t);
		}
	}
					/* if args are specified by arrays,
					   get next array containing outputs. */
	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));

					/* evaluate each specified channel */
	for ( i=0; i<FUT_NOCHAN; i++ ) {

		if ( (omask & FUT_BIT(i)) == 0 )
			continue;	/* output not required */

					/* get pointer to output array */
		out = KCMS_VA_ARG(ap, vap, fut_generic_ptr_t);

					/* pass through undefined channels */
		if ( fut->chan[i] == FUT_NULL_CHAN ) {
			if ( in[i] == 0 )
				return (0);
			else if ( in[i] != out )
				bcopy (in[i], out, (int32)(n<<dattype));

		} else {		/* evaluate this channel */

			if ( ! fut_eval_chan (fut, i, iomask,
						n, in, out,
						(fut_itbldat_ptr_t fut_far*)0) )
				return (0);
		}
	}

	KCMS_VA_END(ap,vap);

	return (1);

} /* fut_eval_array */

/*
 * fut_eval_array8 and fut_eval_array12 are special cases of fut_eval_array
 * which evaluate only 8 bit and 12 bit data respectively.
 */
int
fut_eval_array8 (fut_ptr_t	fut, int32 iomask, int32 n, ... )
{
	KCMS_VA_ARG_PTR	ap;
	char_p vap;

	iomask &= ~FUT_12BITS;	/* make sure 12 bit flag is clear */

	KCMS_VA_START(ap,vap,n);

	return (fut_eval_array(fut, iomask, n, ap));
}

int
fut_eval_array12 (fut_ptr_t	fut, int32 iomask,int32 n, ... )
{
	KCMS_VA_ARG_PTR	ap;
	char_p	vap;

	iomask |= FUT_12BITS;	/* make sure 12 bit flag is set */
	KCMS_VA_START(ap,vap,n);

	return (fut_eval_array(fut, iomask, n, ap));
}

/*
 * fut_eval evaluates a fut at a single point specified by a collection
 * of input values, and places the results into a collection of output values.
 * The number of input and output values are determined by iomask.
 * The input values precede the output values in the arglist and always follow
 * the order x,y,z,t (e.g. xyzt, xyt, xyz, yzt, xy, xz, xt, yz, yt, etc).
 *
 * Imask specifies which input values are present in the arglist
 * Omask specifies which output addresses are present in the arglist
 * Order specifies the interpolation order to use (overiding fut's default)
 * FUT_VARARGS may be used to specify 2 arrays:
 *	the first contains the list of (int) input values,
 *	the second contains the list of output addresses.
 * FUT_12BITS may used to specify evaluation of 12bit instead of 8bit data.
 */
int
fut_eval (fut_ptr_t fut, int32 iomask, KCMS_VA_ARG_PTR ap)
{
	fut_generic_ptr_t	out, in[FUT_NICHAN];
	u_int8			inval8[FUT_NICHAN];
	int16			inval12[FUT_NICHAN];
	int		i;
	int		imask, omask, dattype;
	char_p	vap=NULL;	/* for FUT_VARARGS */

	if ( ! IS_FUT (fut) )
		return (0);
					/* by default, all inputs are
					   avaliable for pass-thru */
	iomask |= FUT_PASS(FUT_IMASK(iomask));
	if ( FUT_ORDMASK(iomask) == FUT_DEFAULT )
		iomask |= FUT_ORDER(fut->iomask.order);

					/* check iomask and unpack */
	if ( ! fut_iomask_check (fut,iomask) )
		return (0);

	imask = (int)FUT_IMASK(iomask);
	omask = (int)FUT_OMASK(iomask);
	dattype = (int)FUT_12BMASK(iomask);	/* 1 = 12bit, 0 = 8 bit */

					/* if args are specified by arrays,
					   get next array containing inputs. */
	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));

				/* collect input values */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( (imask & FUT_BIT(i)) == 0) {
			in[i] = NULL;
		} else {
			/* NOTE: chars and shorts are passed as ints */
			if ( dattype == 0 ) {
				inval8[i] = (u_int8) KCMS_VA_ARG(ap, vap, int);
				in[i] = (fut_generic_ptr_t) &inval8[i];
			} else {
				inval12[i] = (int16) KCMS_VA_ARG(ap, vap, int);
				in[i] = (fut_generic_ptr_t) &inval12[i];
			}
		}
	}
					/* if args are specified by arrays,
					   get next array containing outputs. */
	KCMS_VA_ARRAY(ap,vap,(iomask & FUT_VARARGS));
					/* evaluate each specified channel */
	for ( i=0; i<FUT_NOCHAN; i++ ) {

		if ( (omask & FUT_BIT(i)) == 0 )
			continue;	/* output not required */

					/* get pointer to output array */
		out = KCMS_VA_ARG(ap, vap, fut_generic_ptr_t);

					/* pass through undefined channels */
		if ( fut->chan[i] == FUT_NULL_CHAN ) {
			if ( in[i] == 0 )
				return (0);
			else if ( dattype == 0 )
				*((generic_u_int8_ptr_t)out) = inval8[i];
			else
				*((generic_int16_ptr_t)out)  = inval12[i];

		} else {		/* evaluate this channel */

			if ( ! fut_eval_chan (fut, i, iomask,
						(int32)1, in, out,
						(fut_itbldat_ptr_t fut_far*)0) )
				return (0);
		}
	}

	KCMS_VA_END(ap,vap);

	return (1);

} /* fut_eval */

/**********************************************************
		RLC 910411 Dropped the "n" varible in the lists
		for both fut_eval8 and fut_eval12.  Also correcting
		the calls to fut_eval... They should not have had
		"n" in them.
***************************************************/

/*
 * fut_eval8 and fut_eval12 are special cases of fut_eval
 * which evaluate only 8 bit and 12 bit data respectively.
 */
int
fut_eval8 (fut_ptr_t fut, int32 iomask, ... )
{
	KCMS_VA_ARG_PTR	ap;
	char_p	vap;

	KCMS_VA_START( ap, vap, iomask );

	iomask &= ~FUT_12BITS;	/* make sure 12 bit flag is clear */

	return (fut_eval(fut, iomask, ap));
}

int
fut_eval12 (fut_ptr_t	fut, int32 iomask, ... )
{
	KCMS_VA_ARG_PTR	ap;
	char_p	vap;

	KCMS_VA_START( ap, vap, iomask );

	iomask |= FUT_12BITS;	/* make sure 12 bit flag is set */

	return (fut_eval(fut, iomask, ap));
}

/*
 * fut_eval_chan evaluates a single channel at a set of n points specified by
 * a collection of input arrays, and places the results into an output array.
 * The number of input arrays are determined by the chan's imask.
 *
 * Order specifies the interpolation order to use (overiding fut's default)
 * FUT_12BITS may be used to specify evaluation of 12bit instead of 8bit data.
 *
 * big_itbldat is an optional pointer to an array of large input luts to be
 * used instead of the chan's smaller input luts for the speedier evaluation
 * of 12 bit data.  This is used *ONLY* by fut_comp() to pass in the results of
 * composing one fut's output tables with another one's input tables.
 * It has the side effect of not passing the results of the
 * interpolation through the output table.
 *
 * When evaluating large arrays of 12 bit data and big_itbldat is NULL an
 * optimization is made in which large input tables are pre-interpolated
 * from the channel's small input tables.  This avoids having to interpolate
 * within the small tables on the fly and/or recompute the same values.
 *
 */

int
	fut_eval_chan (fut_ptr_t fut, int32 theChan, int32 iomask,
					int32 n, fut_generic_ptr_t fut_far* in,
					fut_generic_ptr_t out, fut_itbldat_ptr_t fut_far* big_itbldat)
{
int32				i1, order, dattype, returnErr = 1; /* let's assume this will work */
int32				imask, evalMask;
fut_itbldat_ptr_t	itbldat[FUT_NICHAN], theITblDat;
fut_otbldat_ptr_t	otbldat;
fut_chan_ptr_t		chan = fut->chan[theChan];
fut_t				oneChanEvalFut;
fut_chan_t			theOChan;
fut_itbl_t			theITbls[FUT_NICHAN];
fut_otbl_t			theOTbl;


				/* unpack imask, order, and dattype */
	imask = chan->imask;
	dattype = (int)FUT_12BMASK(iomask);	/* 0 = 8 bit, 1 = 12bit */
	order = (int)FUT_ORDMASK(iomask);

	/* if evaluating 12 bit data, then we check to see if large
	 * input tables have been supplied.  This will be the case
	 * if being called from fut_comp().  If they are not supplied,
	 * we see if the data count is large enough to warrant the
	 * optimization in which the chan's small input tables are
	 * pre-interpolated to create large ones.  When using the
	 * large input tables, we must set `dattype' to 2 in order to
	 * get the proper function from fut_interp_fun.
	 *
	 * If evaluating 8 bit data, we always use the chan's own
	 * (small) input tables since no interpolation will be necessary.
	 */

	if (dattype == 1) {
		dattype = 2;		/* 2 = 12bit with big itbls */
	}

	for (i1 = 0; i1 < FUT_NICHAN; i1++) {
		theITblDat = NULL;
		
		if ( (imask & FUT_BIT(i1)) != 0 ) {
			if (dattype == 0) {
				theITblDat = chan->itbl[i1]->tbl;			/* use chan's small table */
			}
			else {
				if ( big_itbldat != NULL ) {				/* use big table if supplied */
					theITblDat = big_itbldat[i1];
				}
				else if (chan->itbl[i1]->tblFlag == 0) {	/* precompute big table */
					theITblDat = fut_expand8to12_itbl (chan->itbl[i1]);
				}
				else if (chan->itbl[i1]->tblFlag > 0) {		/* big table already created */
					theITblDat = chan->itbl[i1]->tbl2;		/* use chan's big table */
				}
			}

			if (theITblDat == NULL) {
				returnErr = 0;	/* missing table */
				goto GetOut;
			}
		}
					
		itbldat[i1] = theITblDat;
	}

	/* if otbl is not defined or big input tables supplied, then no ouptut table */
	if ((chan->otbl == FUT_NULL_OTBL) || ( big_itbldat != NULL)) {
		otbldat = 0;				/* use NULL */
	}
	else {
		otbldat = chan->otbl->tbl;	/* use chan's otbl data */
	}

	/* call the appropriate interpolator */
	switch (imask) {

/* #define KCP_TETRA_COMP (1)	/* */

#if !defined (KCP_TETRA_COMP)
	    case 0 :
		(*fut_interp_fun0(dattype,order)) (out,
			n,
			chan->gtbl->tbl,
			otbldat);
		break;
	    case FUT_X :
		(*fut_interp_fun1(dattype,order)) (out,
			in[FUT_XCHAN],n,
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			otbldat);
		break;
	    case FUT_Y :
		(*fut_interp_fun1(dattype,order)) (out,
			in[FUT_YCHAN],n,
			itbldat[FUT_YCHAN],
			chan->gtbl->tbl,
			otbldat);
		break;
	    case FUT_Z :
		(*fut_interp_fun1(dattype,order)) (out,
			in[FUT_ZCHAN],n,
			itbldat[FUT_ZCHAN],
			chan->gtbl->tbl,
			otbldat);
		break;
	    case FUT_T :
		(*fut_interp_fun1(dattype,order)) (out,
			in[FUT_TCHAN],n,
			itbldat[FUT_TCHAN],
			chan->gtbl->tbl,
			otbldat);
		break;
	    case FUT_X|FUT_Y :
		(*fut_interp_fun2(dattype,order)) (out,
			in[FUT_YCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_YCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_YCHAN],
			otbldat);
		break;
	    case FUT_X|FUT_Z :
		(*fut_interp_fun2(dattype,order)) (out,
			in[FUT_ZCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_ZCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_ZCHAN],
			otbldat);
		break;
	    case FUT_X|FUT_T :
		(*fut_interp_fun2(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			otbldat);
		break;
	    case FUT_Y|FUT_Z :
		(*fut_interp_fun2(dattype,order)) (out,
			in[FUT_ZCHAN],in[FUT_YCHAN],n,
			itbldat[FUT_ZCHAN],
			itbldat[FUT_YCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_ZCHAN],
			otbldat);
		break;
	    case FUT_Y|FUT_T :
		(*fut_interp_fun2(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_YCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_YCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			otbldat);
		break;
	    case FUT_Z|FUT_T :
		(*fut_interp_fun2(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_ZCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_ZCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			otbldat);
		break;
	    case FUT_X|FUT_Y|FUT_Z :
		(*fut_interp_fun3(dattype,order)) (out,
			in[FUT_ZCHAN],in[FUT_YCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_ZCHAN],
			itbldat[FUT_YCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_ZCHAN],
			chan->gtbl->size[FUT_YCHAN],
			otbldat);
		break;
	    case FUT_X|FUT_Y|FUT_T :
		(*fut_interp_fun3(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_YCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_YCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			chan->gtbl->size[FUT_YCHAN],
			otbldat);
		break;
	    case FUT_X|FUT_Z|FUT_T :
		(*fut_interp_fun3(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_ZCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_ZCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			chan->gtbl->size[FUT_ZCHAN],
			otbldat);
		break;
	    case FUT_Y|FUT_Z|FUT_T :
		(*fut_interp_fun3(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_ZCHAN],in[FUT_YCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_ZCHAN],
			itbldat[FUT_YCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			chan->gtbl->size[FUT_ZCHAN],
			otbldat);
		break;
	    case FUT_X|FUT_Y|FUT_Z|FUT_T :
		(*fut_interp_fun4(dattype,order)) (out,
			in[FUT_TCHAN],in[FUT_ZCHAN],in[FUT_YCHAN],in[FUT_XCHAN],n,
			itbldat[FUT_TCHAN],
			itbldat[FUT_ZCHAN],
			itbldat[FUT_YCHAN],
			itbldat[FUT_XCHAN],
			chan->gtbl->tbl,
			chan->gtbl->size[FUT_TCHAN],
			chan->gtbl->size[FUT_ZCHAN],
			chan->gtbl->size[FUT_YCHAN],
			otbldat);
		break;
#endif
	    default:
	    	evalMask = imask | FUT_OUT(FUT_BIT(theChan));

			/* make a static evaluation fut */

			for (i1 = 0; i1 < FUT_NICHAN; i1++) {	/* set up all the input tables */
				theITbls[i1].magic = 0;
				theITbls[i1].ref = 0;
				theITbls[i1].id = 0;
				theITbls[i1].size = chan->gtbl->size[i1];
				theITbls[i1].tblHandle = NULL;
				theITbls[i1].handle = NULL;
				
				if (dattype == 0) {					/* insert input tables according to size */
					theITbls[i1].tblFlag = 0;
					theITbls[i1].tbl = itbldat[i1];
					theITbls[i1].tbl2 = NULL;
				}
				else {
					theITbls[i1].tblFlag = 1;
					theITbls[i1].tbl = NULL;
					theITbls[i1].tbl2 = itbldat[i1];
				}
			}

			theOTbl.magic = 0;			/* set up the single output table */
			theOTbl.ref = 0;
			theOTbl.id = 0;
			theOTbl.tbl = otbldat;		/* insert the output table to use */
			theOTbl.tblHandle = NULL;
			theOTbl.handle = NULL;

			/* define the single output channel */
			theOChan.magic = 0;
			theOChan.imask = imask;					/* input mask */
			theOChan.gtbl = chan->gtbl;				/* grid table */
			theOChan.gtblHandle = NULL;
			theOChan.otbl = &theOTbl;				/* output table */
			theOChan.otblHandle = NULL;
			for (i1 = 0; i1 < FUT_NICHAN; i1++) {	/* input tables */
				theOChan.itbl[i1] = &theITbls[i1];
				theOChan.itblHandle[i1] = NULL;
			}
			theOChan.handle = NULL;

			oneChanEvalFut.magic = 0;	/* so we do not try to free this memory */
			oneChanEvalFut.idstr = NULL;
			oneChanEvalFut.iomask = fut_int_to_iomask (evalMask);
			for (i1 = 0; i1 < FUT_NICHAN; i1++) {
				oneChanEvalFut.itbl[i1] = NULL;
				oneChanEvalFut.itblHandle[i1] = NULL;
			}
			for (i1 = 0; i1 < FUT_NOCHAN; i1++) {
				oneChanEvalFut.chan[i1] = NULL;
				oneChanEvalFut.chanHandle[i1] = NULL;
			}
			oneChanEvalFut.handle = NULL;
			oneChanEvalFut.refNum = 0;
			oneChanEvalFut.modNum = 0;

			oneChanEvalFut.chan[theChan] = &theOChan;	/* load single active chan into correct position */

	    	cpeval (&oneChanEvalFut, evalMask, dattype, in, out, n);	/* use the CP interpolator */
	    	
	} /* switch */

GetOut:
	return (returnErr);

} /* fut_eval_chan */


