/*
 * @(#)calcitbl.c	1.16 97/12/22

	Contains:	calcItbl1, calcItbl256, calcItblN

	Written by:	The Boston White Sox

*/

/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) 1993-1995 Eastman Kodak Company                  ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

/*
 *	General definitions
 */

#include <math.h>
#include "csmatrix.h"
#include "makefuts.h"
#include "xfers.h"

/*----------------------------------------------------------------------------
 *  calcItbl1, calcItbl256, calcItblN -- calculate an input table
 *		to linearize RGB inputs according to specified algorithm (power law, 
 *		256-entry LUT, or transfer table)
 *----------------------------------------------------------------------------
 */

#if defined(KCP_FPU)			/* using the FPU? */
void calcItbl1FPU (fut_itbldat_ptr_t table, int32 gridSize, double gamma)
#else							/* all other programming environments */
void calcItbl1noFPU (fut_itbldat_ptr_t table, int32 gridSize, double gamma)
#endif
{
	int32		count;
	double		val;
	double		norm, norm2;
	int32		lval, lnorm2;
	double		x;
	double		nument_inv;

     /* Check input parameters and initialize:  */
	if (table == (fut_itbldat_ptr_t)NULL)
	   return;				/* just don't crash */
	if (gamma == (double)1.0)
	   return;				/* trivial case */
	nument_inv = 1.0 / (double)(FUT_INPTBL_ENT - 1);
	norm = (double)((gridSize - 1) << FUT_INP_DECIMAL_PT);
	norm2 = 2.0*norm;
	lnorm2 = (int32) norm2;

     /* Loop over regular entries, converting index to floating-point variable:  */
	if (gamma > 0.0)
	{
		for (count = 0; count < FUT_INPTBL_ENT; count++)
		{
			x = (double)count * nument_inv;		/* in [0, 1] */

			 /* Compute linearized value by power law:  */
#if defined(KCP_FPU)			/* using the FPU? */
			val = (double)powfpu (x, gamma);				/* in [0, 1] */
#else							/* all other programming environments */
			val = (double)pow (x, gamma);				/* in [0, 1] */
#endif

			 /* Rescale, clip, and quantize for input table:  */
/* the following is the original code - pre speedup */
/*			val = val * norm + 0.5;				in [1/2, norm + 1/2] */
/*			if (val >= norm) */
/*				val = norm - 1.0;				in [1/2, norm - 1/2] */
/*			*(table) = (fut_itbldat_t)val;		in [0, norm - 1] */
/* end of original code */

/* replacement code */
			lval = (int32) (val * norm2);
			lval++;
			if (lval >= lnorm2) {
				lval = lnorm2 - 2;
			}
			*(table) = ((fut_itbldat_t)lval) >> 1;	/* in [0, norm - 1] */
/* end of replacement code */

			table++;
		}
	}
	else
	{
		for (count = 0; count < FUT_INPTBL_ENT; count++)
		{
			x = (double)count * nument_inv;		/* in [0, 1] */

			 /* Compute linearized value by power law:  */
#if defined(KCP_FPU)			/* using the FPU? */
			val = (double)powfpu (x, gamma);				/* in [0, 1] */
#else							/* all other programming environments */
			val = (double)pow (x, gamma);				/* in [0, 1] */
#endif

			 /* Rescale, clip, and quantize for input table:  */
			if (1.0 < val)
				val = 1.0;
			lval = (int32) (val * norm2);
			lval++;
			if (lval >= lnorm2) {
				lval = lnorm2 - 2;
			}
			*(table) = ((fut_itbldat_t)lval) >> 1;	/* in [0, norm - 1] */
			table++;
		}
	}
		
     /* Set the last entry to the value of the previous one.  This will perform
	automatic clipping of input greater than 4080 to the valid gridspace,
	which is defined only for input in the range [0, 255] or [0 << 4, 255 << 4]:  */
	*table = *(table - 1);
}

/*------------------------------------------------------------------------*/
#if defined(KCP_FPU)			/* using the FPU? */
void calcItbl256FPU (fut_itbldat_ptr_t table, int32 gridSize, unsigned short lut[])
#else							/* all other programming environments */
void calcItbl256noFPU (fut_itbldat_ptr_t table, int32 gridSize, unsigned short lut[])
#endif
{
	int32		count;
	double		val;
	double		norm, norm2;
	int32		lval, lnorm2;

     /* Check input and initialize:  */
	if (table == (fut_itbldat_ptr_t)NULL)
	   return;				/* just don't crash! */
	if (lut == (unsigned short *)NULL)
	   return;
	norm = (double)((gridSize - 1) << FUT_INP_DECIMAL_PT);
	norm2 = 2.0*norm;
	lnorm2 = (int32) norm2;

     /* Loop over regular entries, converting index to floating-point variable:  */
	for (count = 0; count < FUT_INPTBL_ENT; count++)	/* in [0, 255] */
	{
	     /* Compute linearized value by table look-up:  */
		val = (double)lut[count] / SCALEDOT16;		/* in [0, 1] */

	     /* Rescale, clip, and quantize for input table:  */
		val = RESTRICT (val, 0.0, 1.0);		/* just to be sure */
		lval = (int32) (val * norm2);
		lval++;
		if (lval >= lnorm2) {
			lval = lnorm2 - 2;
		}
		*(table) = ((fut_itbldat_t)lval) >> 1;	/* in [0, norm - 1] */
		table++;
	}
		
     /* Set the last entry to the value of the previous one.  This will perform
	automatic clipping of input greater than 4080 to the valid gridspace,
	which is defined only for input in the range [0, 255] or [0 << 4, 255 << 4]:  */
	*table = *(table - 1);
}

/*---------------------  calcItblN  --------------------------------*/
/*
fut_itbldat_ptr_t	table;		pointer to input-table data
KpUInt32_t			gridSize;	dimension of grid table to be addressed
KpUInt32_t			length;		length of data table
KpUInt16_p			data;		table of linearization data
KpUInt32_t 			interpMode;	interpolation mode to use
*/

#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t
	calcItblNFPU (fut_itbldat_ptr_t table, int32 gridSize, ResponseRecord* rrp, KpUInt32_t interpMode)
#else							/* all other programming environments */
PTErr_t
	calcItblNnoFPU (fut_itbldat_ptr_t table, int32 gridSize, ResponseRecord* rrp, KpUInt32_t interpMode)
#endif
{
PTErr_t		PTErr;
KpUInt32_t	length;
KpUInt16_p	data;
KpUInt32_t	count, ix;
double		val;
double		norm, norm2;
KpInt32_t	lval, lnorm2;
double		x;
double		frac, nument_inv_li, nument_inv_l4;
int			hint = 1;
xfer_t		Xfer;

	/* Check input and initialize:  */
	if (Kp_IsBadWritePtr (rrp, sizeof (*rrp))) {
		return KCP_BAD_ARG;
	}
	
	length = rrp->count;
	data = rrp->data;

	if ((Kp_IsBadWritePtr (table, (FUT_INPTBL_ENT +1) * sizeof(fut_itbldat_t))) ||
		(gridSize < 2) || (gridSize > FUT_GRD_MAXDIM) ||			/* bad size */
		(length == 0) ||											/* bad size */
		(Kp_IsBadReadPtr (data, length * (KpUInt32_t)sizeof(KpUInt16_t)))) {	/* no transfer table */
			return KCP_BAD_ARG;
	}
	   
#if defined(KCP_FPU)			/* using the FPU? */
	PTErr = init_xferFPU (&Xfer, rrp);
#else							/* all other programming environments */
	PTErr = init_xfer (&Xfer, rrp);
#endif
	if (PTErr != KCP_SUCCESS) {
	   return KCP_BAD_ARG;
	}
	
#if defined(KCP_FPU)			/* using the FPU? */
	PTErr = set_xferFPU (&Xfer, 0, 1);
#else							/* all other programming environments */
	PTErr = set_xfer (&Xfer, 0, 1);
#endif
	if (PTErr != KCP_SUCCESS) {
	   return KCP_BAD_ARG;
	}

	nument_inv_li = (double)(length - 1) / (double)(FUT_INPTBL_ENT - 1);
	nument_inv_l4 = 1.0 / (double)(FUT_INPTBL_ENT - 1);
	
	norm = (double)((gridSize - 1) << FUT_INP_DECIMAL_PT);
	norm2 = 2.0 * norm;
	lnorm2 = (int32) norm2;

     /* Loop over regular entries, converting index to floating-point variable:  */
	for (count = 0; count < FUT_INPTBL_ENT; count++) {
		
		switch (interpMode) {
		case KCP_TRC_LINEAR_INTERP:

	 	    /* Compute linearized value by interpolating in transfer table:  */
			x = (double)count * nument_inv_li;		/* in [0, length - 1] */
			ix = (KpUInt32_t)x;						/* integer part */
			if (ix >= (length - 1)) {				/* off end of data table */
				val = (double)data[length - 1] / SCALEDOT16;	/* take last value */
			}
			else {								/* within data table, interpolate */
				frac = x - (double)ix;			/* fractional part */
				val = (double)data[ix] + frac * ((double)data[ix + 1] - (double)data[ix]);
				val /= SCALEDOT16;
			}						/* in [0, 1] */

			break;
		
		case KCP_TRC_LAGRANGE4_INTERP:
	
/* !!!!  LAGRANGIAN NOT WORK IF THE SOURCE DATA IS NON-MONOTONIC OR HAS FLAT REGIONS !!!! */

			/* Compute linearized value by interpolating in transfer table:  */
			x = (double)count * nument_inv_l4;	/* in [0, 1] */
#if defined(KCP_FPU)			/* using the FPU? */
			val = xferFPU (&Xfer, x, &hint);		/* in [0, 1] */
#else							/* all other programming environments */
			val = xfer (&Xfer, x, &hint);		/* in [0, 1] */
#endif

			break;
		
		default:
			return KCP_BAD_ARG;
		}

	     /* Rescale, clip, and quantize for input table:  */
		val = RESTRICT (val, 0.0, 1.0);		/* just to be sure */
		lval = (KpInt32_t) (val * norm2);
		lval++;
		if (lval >= lnorm2) {
			lval = lnorm2 - 2;
		}
		*(table) = ((fut_itbldat_t)lval) >> 1;	/* in [0, norm - 1] */
		table++;
	}
		
     /* Set the last entry to the value of the previous one.  This will perform
	automatic clipping of input greater than 4080 to the valid gridspace,
	which is defined only for input in the range [0, 255] or [0 << 4, 255 << 4]:  */
	*table = *(table - 1);
	
	return KCP_SUCCESS;
}

