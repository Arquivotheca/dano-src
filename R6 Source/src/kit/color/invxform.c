/*
	File:		invxform.c

	Contains:	makeInverseXformFromMatrix

	Written by:	The Boston White Sox

*/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) 1993-1994 Eastman Kodak Company                     ***
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
 *  makeInverseXformFromMatrix -- make a fut of given gridsize from given
 *	matrix data for inverse transform (XYZ -> RGB); return status code
 *---------------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t makeInverseXformFromMatrixFPU (LPMATRIXDATA	mdata,
									u_int32 		gridsize,
									KpUInt32_t		interpMode,
									fut_ptr_t *		theFut)
#else							/* all other programming environments */
PTErr_t makeInverseXformFromMatrixNoFPU (LPMATRIXDATA	mdata,
									u_int32 		gridsize,
									KpUInt32_t		interpMode,
									fut_ptr_t *		theFut)
#endif
{
PTErr_t				PTErr;
ResponseRecord		*rrp;
int32				dim[3];
int					futReturn;
fut_t				*futp;
fut_gtbldat_ptr_t	gtblDat[3];
fut_otbldat_ptr_t	otblDat;
fut_otbldat_ptr_t	otblDat2;
u_int16					i;
double				fwdgamma;
double				one[3];

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

     /* Replace output LUT's:  */

/* check for same gamma in all channels */
	if ((1 == mdata->response[0]->count)
		&& (1 == mdata->response[1]->count)
		&& (1 == mdata->response[2]->count)
		&& (mdata->response[0]->data[0] == mdata->response[1]->data[0])
		&& (mdata->response[1]->data[0] == mdata->response[2]->data[0])) {

		 /* Get input table:  */
		futReturn = fut_get_otbl (futp, 0, &otblDat);
		if ((futReturn != 1) || (otblDat == (fut_otbldat_ptr_t)NULL))
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
		calcOtbl1FPU (otblDat, fwdgamma);
#else							/* all other programming environments */
		calcOtbl1noFPU (otblDat, fwdgamma);
#endif
		for (i = 1; i < 3; i++)
		{
			 /* Get input table:  */
			futReturn = fut_get_otbl (futp, i, &otblDat2);
			if ((futReturn != 1) || (otblDat2 == (fut_otbldat_ptr_t)NULL))
			{
			   fut_free (futp);
			   return KCP_SYSERR_0;
			}
			memcpy (otblDat2, otblDat, sizeof (*otblDat) * FUT_OUTTBL_ENT);
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
		 /* Get output table:  */
			futReturn = fut_get_otbl (futp, i, &otblDat);
			if ((futReturn != 1) || (otblDat == (fut_otbldat_ptr_t)NULL))
			{
				fut_free (futp);
				return KCP_SYSERR_0;
			}

			 /* Get ResponseRecord:  */
			rrp = mdata->response[i];
			if ((rrp->count > 0) && (rrp->data == (unsigned short FAR*)NULL))
			{
				fut_free (futp);
				return KCP_INCON_PT;
			}

			 /* Recompute output table:  */
			switch (rrp->count) {
			case 0:	/* linear response, with clipping */
#if defined(KCP_FPU)			/* using the FPU? */
				calcOtbl0FPU (otblDat);
#else							/* all other programming environments */
				calcOtbl0noFPU (otblDat);
#endif
				break;
				
			case 1:	/* power law */
				fwdgamma = (double)rrp->data[0] / SCALEDOT8;
				if (fwdgamma <= 0.0)
				{
					fut_free (futp);
					return KCP_INCON_PT;
				}
#if defined(KCP_FPU)			/* using the FPU? */
				calcOtbl1FPU (otblDat, fwdgamma);
#else							/* all other programming environments */
				calcOtbl1noFPU (otblDat, fwdgamma);
#endif
				break;
				
			default:	/* look-up table of arbitrary length */
				makeInverseMonotonic (rrp->count, rrp->data);
#if defined(KCP_FPU)			/* using the FPU? */
				PTErr = calcOtblNFPU (otblDat, rrp, interpMode);
#else							/* all other programming environments */
				PTErr = calcOtblNnoFPU (otblDat, rrp, interpMode);
#endif
				if (PTErr != KCP_SUCCESS) {
					fut_free (futp);
					return KCP_INCON_PT;
				}
				break;
				
			}
		}
	}

    /* Rescale given matrix for ICC normalization of XYZ PCS;
		also apply factor of 3 for extended range:  */
	for (i = 0; i < 3; i++)
	{
	    int	j;

		for (j = 0; j < 3; j++)
	        mdata->matrix[i][j] *= 3.0;
	}

     /* Compute inverse matrix (XYZ -> RGB):  */
	one[0] = one[1] = one[2] = 1.0;			/* arbitrary vector */
	 /* replaces matrix with inverse */
	if (solvemat (3, mdata->matrix, one) != 0) {
	    fut_free (futp);
	    return KCP_INCON_PT;				/* singular matrix passed in */
	}

     /* Replace grid tables:  */
	for (i = 0; i < 3; i++) {
	    futReturn = fut_get_gtbl (futp, i, &gtblDat[i]);
	    if ((futReturn != 1) || (gtblDat[i] == (fut_gtbldat_ptr_t)NULL)) {
			fut_free (futp);
			return KCP_INCON_PT;
	    }
	}

#if defined(KCP_FPU)			/* using the FPU? */
	calcGtbl3FPU (gtblDat, dim, mdata->matrix, True);	/* with offset */
#else							/* all other programming environments */
	calcGtbl3noFPU (gtblDat, dim, mdata->matrix, True);	/* with offset */
#endif

     /* Set fut pointer and return successfully:  */
	*theFut = futp;
	return KCP_SUCCESS;
}


#if !defined(KCP_FPU)			/* using the FPU? */
/*-------------------------------------------------------------------------------
 *  makeInverseMonotonic -- flatten reversals in data table
 *-------------------------------------------------------------------------------
 */
void
	makeInverseMonotonic (KpUInt32_t count, KpUInt16_p table)
{
KpInt32_t	i;
KpUInt16_t	val;

     /* Check inputs:  */
	if ((table == (KpUInt16_p)NULL) || (count < 3)) {
		return;
	}

     /* Flatten from high end to low end, depending on polarity:  */
	if (table[0] <= table[count - 1]) {		/* globally non-decreasing */
		val = table[count - 1];
		for (i = count - 2; i >= 0; i--) {	/* from right to left */
			if (table[i] > val) {			/* reversal? */
				table[i] = val;				/* flatten! */
			}
			else {							/* no reversal? */
				val = table[i];				/* update */
			}
		}
	}
	else {									/* globally decreasing */
		val = table[0];
		for (i = 1; i < (KpInt32_t)count; i++)	{		/* from left to right */
			if (table[i] > val)	{			/* reversal? */
				table[i] = val;				/* flatten! */
			}
			else {							/* no reversal? */
				val = table[i];				/* update */
			}
		}
	}
}
#endif	/* #if !defined(KCP_FPU) */


