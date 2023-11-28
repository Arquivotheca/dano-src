
/*
 * @(#)calcmtbl.c	1.5 97/12/22

	Contains:	sets up the ouput tables for monochrome	transforms

	Written by:	Color Processor group

*/

/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1997                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "kcms_sys.h"

#include <math.h>
#include "csmatrix.h"
#include "monopt.h"
#include "makefuts.h"

/*---------------------------------------------------------------------------
 *  calcOtblLSN -- calculate an output table by doing a device to L(TRC) & L to
 					L*  conversion.
 *---------------------------------------------------------------------------
 */

PTErr_t calcOtblLSN (fut_otbldat_ptr_t table, ResponseRecord* rrp)
{
KpUInt32_t	length;
KpUInt16_p	data;
KpUInt32_t	count, ix;
double		val;
double		x;
double		frac, nument_inv_li;

	/* Check input and initialize:  */
	if (Kp_IsBadWritePtr (rrp, sizeof (*rrp))) {
		return KCP_BAD_ARG;
	}
	
	length = rrp->count;
	data = rrp->data;

	if ((Kp_IsBadWritePtr (table, (FUT_OUTTBL_ENT +1) * sizeof(fut_otbldat_t))) ||
		(length == 0) ||											/* bad size */
		(Kp_IsBadReadPtr (data, length * (KpUInt32_t)sizeof(KpUInt16_t)))) {	/* no transfer table */
			return KCP_BAD_ARG;
	}

	nument_inv_li = (double)(length - 1) / (double)(FUT_OUTTBL_ENT - 1);
	
     /* Loop over regular entries, converting index to floating-point variable:  */
	for (count = 0; count < FUT_OUTTBL_ENT; count++) {
		
	 	/* Compute linearized value by interpolating in transfer table:  */
		x = (double)count * nument_inv_li;		/* in [0, length - 1] */
		ix = (KpUInt32_t)x;						/* integer part */
		if (ix >= (length - 1)) {				/* off end of data table */
			val = (double)data[length - 1];	/* take last value */
		}
		else {								/* within data table, interpolate */
			frac = x - (double)ix;			/* fractional part */
			val = (double)data[ix] + frac * ((double)data[ix + 1] - (double)data[ix]);
		}						/* in [0, 1] */

		/* scale and calculate L to L* */
		val /= RRECORD_DATA_SIZE - 1;
		val = H (val);
	
		/* Rescale to 4080 (= 255 << 4):  */
		*(table++) = (fut_otbldat_t)QUANT (val, FUT_MAX_PEL12);
	}
	
	return KCP_SUCCESS;
}

/*---------------------------------------------------------------------------
 *  calcOtblLS1 -- calculate an output table gamma value by doing a power law & L to
 					L*  conversion.
 *---------------------------------------------------------------------------
 */

PTErr_t calcOtblLS1 (fut_otbldat_ptr_t table, double gamma)
{
	KpUInt32_t	count;
	double		val;
	double		x;
	double		nument_inv;

     /* Check input parameters and initialize:  */
	if (table == (fut_otbldat_ptr_t)NULL)
	   return KCP_BAD_ARG;				/* just don't crash */

	nument_inv = 1.0 / (double)(FUT_OUTTBL_ENT - 1);

     /* Loop over regular entries, converting index to floating-point variable:  */
	for (count = 0; count < FUT_OUTTBL_ENT; count++)
	{
		x = (double)count * nument_inv;		/* in [0, 1] */

		/* Compute linearized value by power law:  */
		val = pow (x, gamma);				/* in [0, 1] */

		if (1.0 < val)
			val = 1.0;

		/* calculate L to L* */
		val = H (val);
	
		/* Rescale to 4080 (= 255 << 4):  */
		*(table++) = (fut_otbldat_t)QUANT (val, FUT_MAX_PEL12);

	}

	return KCP_SUCCESS;
}

/*---------------------------------------------------------------------------
 *  calcOtblLN -- calculate an output table by doing a L* to L & 
 					L to device(inverted TRC) conversion.
 *---------------------------------------------------------------------------
 */

PTErr_t calcOtblLN (fut_otbldat_ptr_t table, ResponseRecord* rrp)
{
KpUInt32_t	length;
KpUInt16_p	data;
KpUInt32_t	count;
double		val, p;

	/* Check input and initialize:  */
	if (Kp_IsBadWritePtr (rrp, sizeof (*rrp))) {
		return KCP_BAD_ARG;
	}
	
	length = rrp->count;
	data = rrp->data;

	if ((Kp_IsBadWritePtr (table, (FUT_OUTTBL_ENT +1) * sizeof(fut_otbldat_t))) ||
		(length == 0) ||
		(data[length - 1] == data[0]) ||						/* empty domain */											/* bad size */
		(Kp_IsBadReadPtr (data, length * (KpUInt32_t)sizeof(KpUInt16_t)))) {	/* no transfer table */
			return KCP_BAD_ARG;
	}
	
     /* Loop over regular entries, converting index to floating-point variable:  */
	for (count = 0; count < FUT_OUTTBL_ENT; count++) {
		
		/* scale and calcuate L* to L */
		p = DEQUANT ((fut_gtbldat_t)count, FUT_GRD_MAXVAL);
		p = H_inverse (p);

		/* rescale to TRC length */
		p = RESTRICT (p, 0.0, 1.0);
		p *= RRECORD_DATA_SIZE;

		/* Find value relative to data table:  */
#if defined(KCP_FPU)			/* using the FPU? */
		val = calcInvertTRCFPU (p, data, length);
#else							/* all other programming environments */
		val = calcInvertTRCnoFPU (p, data, length);
#endif

		/* rescale and clip to [0, 1] */
		val /= (double) (length - 1);
		val = RESTRICT (val, 0.0, 1.0);

		/* Rescale to 4080 (= 255 << 4):  */
		*(table++) = (fut_otbldat_t)QUANT (val, FUT_MAX_PEL12);
	}
	
	return KCP_SUCCESS;
}

PTErr_t calcOtblL1 (fut_otbldat_ptr_t table, double gamma)
{
	KpUInt32_t	count;
	double		val, invgamma;
	double		x;
	double		nument_inv;

     /* Check input parameters and initialize:  */
	if (table == (fut_otbldat_ptr_t)NULL)
	   return KCP_BAD_ARG;				/* just don't crash */
	if (gamma == 0.0F)
	   return KCP_BAD_ARG;

	/* invert the gamma */
	invgamma = 1.0F / gamma;

     /* Loop over regular entries, converting index to floating-point variable:  */
	for (count = 0; count < FUT_OUTTBL_ENT; count++)
	{
		nument_inv = (double)count / (double)(FUT_OUTTBL_ENT - 1);

		/* calculate L* to L */
		x = H_inverse (nument_inv);		/* in [0, 1] */

		/* Compute linearized value by power law:  */
		val = pow (x, invgamma);				/* in [0, 1] */

		if (1.0 < val)
			val = 1.0;
	
		/* Rescale to 4080 (= 255 << 4):  */
		*(table++) = (fut_otbldat_t)QUANT (val, FUT_MAX_PEL12);

	}

	return KCP_SUCCESS;
}

/* grid table ramp, using x channel */
fut_gtbldat_t gtblFunc (double *args)
{

double	x;

	x = args[0];

	return QUANT (x, FUT_GRD_MAXVAL);
}

/* output table function */
fut_otbldat_t otblFunc (fut_gtbldat_t q)
{
	if (q) {}

	return (0x800);
}

