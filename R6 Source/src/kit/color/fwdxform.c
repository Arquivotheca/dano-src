/*
 * @(#)fwdxform.c	1.19 97/12/22

	Contains:	makeForwardXformFromMatrix

	Written by:	The Boston White Sox

*/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) 1993-1994 Eastman Kodak Company                  ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/



/*
 *	General definitions
 */

#include <math.h>
#include <string.h>
#include "csmatrix.h"


/*---------------------------------------------------------------------------
 *  makeForwardXformFromMatrix -- make a fut of given gridsize from given matrix data
 *		for forward transform (RGB -> XYZ); return status code
 *---------------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t makeForwardXformFromMatrixFPU (LPMATRIXDATA	mdata,
									u_int32			gridsize,
									KpUInt32_t		interpMode,
									fut_ptr_t *		theFut)
#else							/* all other programming environments */
PTErr_t makeForwardXformFromMatrixNoFPU (LPMATRIXDATA	mdata,
									u_int32			gridsize,
									KpUInt32_t		interpMode,
									fut_ptr_t *		theFut)
#endif
{
PTErr_t		PTErr;
ResponseRecord		*rrp;
int32				dim[3];
int					futReturn;
fut_t				*futp;
fut_itbldat_ptr_t	itblDat;
fut_itbldat_ptr_t	itblDat2;
fut_gtbldat_ptr_t	gtblDat[3];
u_int16					i;
double				fwdgamma;

     /* Check validity and dimensionality of matrix data:  */
	if (mdata == (LPMATRIXDATA)NULL)
	   return KCP_SYSERR_0;
	if (mdata->matrix == (double FAR* FAR*)NULL)
	   return KCP_SYSERR_0;
	if (mdata->response == (ResponseRecord FAR* FAR*)NULL)
	   return KCP_SYSERR_0;
	if (mdata->dim != 3)
	   return KCP_SYSERR_0;

	for (i = 0; i < mdata->dim; i++) {
	    if ( (mdata->matrix[i] == (double FAR*)NULL) ||
			(mdata->response[i] == (ResponseRecord FAR*)NULL) )
			return KCP_SYSERR_0;
	}

     /* Create "empty" FuT (3D -> 3D, gridsize x gridsize x gridsize)--
	identity grid, input & output ramps:  */
	if (gridsize < 2) {
	   return KCP_INCON_PT;
	}

	dim[0] = dim[1] = dim[2] = (int32)gridsize;
	
	futp = fut_new_empty ((int32)3, dim, (int32)3);	
	if (futp == FUT_NULL) {
	   return KCP_NO_MEMORY;
	}
	if ( futp->idstr != NULL ) {
		fut_free_idstr (futp->idstr);
		futp->idstr = NULL;
	}

/* check for same gamma in all channels */
	if ((1 == mdata->response[0]->count)
		&& (1 == mdata->response[1]->count)
		&& (1 == mdata->response[2]->count)
		&& (mdata->response[0]->data[0] == mdata->response[1]->data[0])
		&& (mdata->response[1]->data[0] == mdata->response[2]->data[0])) {

		 /* Get input table:  */
		futReturn = fut_get_itbl (futp, -1, 0, &itblDat);
		if ((futReturn != 1) || (itblDat == (fut_itbldat_ptr_t)NULL))
		{
		   fut_free (futp);
		   return KCP_SYSERR_0;
		}

		fwdgamma = (double)mdata->response[0]->data[0] / SCALEDOT8;
		if (fwdgamma <= 0.0)
		{
		   fut_free (futp);
		   return KCP_INCON_PT;
		}
#if defined(KCP_FPU)			/* using the FPU? */
		calcItbl1FPU (itblDat, dim[0], fwdgamma);
#else							/* all other programming environments */
		calcItbl1noFPU (itblDat, dim[0], fwdgamma);
#endif
		for (i = 1; i < 3; i++)
		{
			 /* Get input table:  */
			futReturn = fut_get_itbl (futp, -1, i, &itblDat2);
			if ((futReturn != 1) || (itblDat2 == (fut_itbldat_ptr_t)NULL))
			{
			   fut_free (futp);
			   return KCP_SYSERR_0;
			}
			memcpy (itblDat2, itblDat, sizeof (*itblDat) * FUT_INPTBL_ENT);
		}
	}
	else {

     /* Replace input LUT's:  */
		for (i = 0; i < 3; i++) {
			 /* Get input table:  */
			futReturn = fut_get_itbl (futp, (int)-1, i, &itblDat);
			if ((futReturn != 1) || (itblDat == (fut_itbldat_ptr_t)NULL)) {
			   fut_free (futp);
			   return KCP_SYSERR_0;
			}

			 /* Get ResponseRecord:  */
			rrp = mdata->response[i];
			if ((rrp->count > 0) && (rrp->data == (KpUInt16_p)NULL)) {
			   fut_free (futp);
			   return KCP_INCON_PT;
			}

			 /* Recompute input table:  */
			switch (rrp->count) {
			   case 0:	/* linear response */
				/* no-op:  leave ramps alone */
				break;
				
			   case 1:	/* power law */
				fwdgamma = (double)rrp->data[0] / SCALEDOT8;
				if (fwdgamma <= 0.0) {
				   fut_free (futp);
				   return KCP_INCON_PT;
				}
#if defined(KCP_FPU)			/* using the FPU? */
				calcItbl1FPU (itblDat, dim[i], fwdgamma);
#else							/* all other programming environments */
				calcItbl1noFPU (itblDat, dim[i], fwdgamma);
#endif
				break;
				
			   case 256:	/* ready-to-use look-up table */
				makeMonotonic (rrp->count, rrp->data);
#if defined(KCP_FPU)			/* using the FPU? */
				calcItbl256FPU (itblDat, dim[i], rrp->data);
#else							/* all other programming environments */
				calcItbl256noFPU (itblDat, dim[i], rrp->data);
#endif
				break;
				
			   default:	/* transfer table of arbitrary length */
				makeMonotonic (rrp->count, rrp->data);
#if defined(KCP_FPU)			/* using the FPU? */
				PTErr = calcItblNFPU (itblDat, dim[i], rrp, interpMode);
#else							/* all other programming environments */
				PTErr = calcItblNnoFPU (itblDat, dim[i], rrp, interpMode);
#endif
				if (PTErr != KCP_SUCCESS) {
					fut_free (futp);
					return KCP_INCON_PT;
				}
				break;
			}
		}
	}

     /* Replace grid tables:  */
	for (i = 0; i < 3; i++)
	{
	   futReturn = fut_get_gtbl (futp, i, &gtblDat[i]);
	   if ((futReturn != 1) || (gtblDat[i] == (fut_gtbldat_ptr_t)NULL))
	   {
		fut_free (futp);
		return KCP_INCON_PT;
	   }
	}
#if defined(KCP_FPU)			/* using the FPU? */
	calcGtbl3FPU (gtblDat, dim, mdata->matrix, False);	/* without offset */
#else							/* all other programming environments */
	calcGtbl3noFPU (gtblDat, dim, mdata->matrix, False);	/* without offset */
#endif

     /* Set fut pointer and return successfully:  */
	*theFut = futp;
	return KCP_SUCCESS;
}


#if !defined(KCP_FPU)			/* using the FPU? */
/*-------------------------------------------------------------------------------
 *  makeMonotonic -- flatten reversals in data table
 *-------------------------------------------------------------------------------
 */
void
	makeMonotonic (KpUInt32_t count, KpUInt16_p table)
{
KpInt32_t	i;
KpUInt16_t	val;

     /* Check inputs:  */
	if ((table == (KpUInt16_p)NULL) || (count < 3)) {
		return;
	}

     /* Flatten from high end to low end, depending on polarity:  */
	if (table[0] <= table[count - 1]) {	/* globally non-decreasing */
		val = table[count - 1];
		for (i = count - 2; i >= 0; i--) {	/* from right to left */
			if (table[i] > val)	{	/* reversal? */
				table[i] = val;		/* flatten! */
			}
			else {					/* no reversal? */
				val = table[i];		/* update */
			}
		}
	}
	else {						/* globally decreasing */
		val = table[0];
		for (i = 1; i < (KpInt32_t)count; i++) {		/* from left to right */
			if (table[i] > val)	{	/* reversal? */
				table[i] = val;		/* flatten! */
			}
			else {					/* no reversal? */
				val = table[i];		/* update */
			}
		}
	}
}
#endif	/* #if !defined(KCP_FPU) */


