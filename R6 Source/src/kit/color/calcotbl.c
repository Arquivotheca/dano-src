/*
 * @(#)calcotbl.c	1.21 97/12/22

	Contains:	calcOtbl0, calcOtbl1, calcOtblN

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


/*---------------------------------------------------------------------------
 *  calcOtbl0, calcOtbl1, calcOtblN -- calculate an output table
 *		to clip extended range, gamma-correct RGB outputs according to 
 *		specified algorithm (linear, power law, or transfer table),
 *		and requantize.
 *---------------------------------------------------------------------------
 */

#if defined(KCP_FPU)			/* using the FPU? */
void
	calcOtbl0FPU (fut_otbldat_ptr_t table)
#else							/* all other programming environments */
void
	calcOtbl0noFPU (fut_otbldat_ptr_t table)
#endif
{
int32			count;
double			p, f1 = 1.0, f0 = 0.0;
double			nument_inv;
double			dblPel12Max;
fut_otbldat_t	odata;

     /* Check input parameter:  */
	if (table == (fut_otbldat_ptr_t)NULL)
	   return;				/* just don't crash! */

     /* Loop over table entries:  */
	dblPel12Max = (double) FUT_MAX_PEL12;

/* first third */
	 /* Rescale to 4080 (= 255 << 4):  */
	odata = (fut_otbldat_t)QUANT (f0, dblPel12Max);
	for (count = -FUT_GRD_MAXVAL; count <= 0; count += 3)
		*(table++) = odata;

/* middle third */
	nument_inv = 1.0 / FUT_GRD_MAXVAL;
	for (; count <= FUT_GRD_MAXVAL; count += 3)
	{
	     /* Treat otbl index as output from a gtbl scaled to 4095:  */
		p = (double)count * nument_inv;		/* in [0, 1] */

	     /* Rescale to 4080 (= 255 << 4):  */
		odata = (fut_otbldat_t)QUANT (p, FUT_MAX_PEL12);
		*(table++) = odata;
	}

/* last third */
	 /* Rescale to 4080 (= 255 << 4):  */
	odata = (fut_otbldat_t)QUANT (f1, dblPel12Max);
	for (; count < 2 * FUT_OUTTBL_ENT; count += 3)
		*(table++) = odata;
}



#if defined(KCP_FPU)			/* using the FPU? */
void
	calcOtbl1FPU (fut_otbldat_ptr_t table, double fwdgamma)
#else							/* all other programming environments */
void
	calcOtbl1noFPU (fut_otbldat_ptr_t table, double fwdgamma)
#endif
{
KpInt32_t		count;
double			p, f1 = 1.0, f0 = 0.0;
double			invgamma;
double			nument_inv;
double			dblPel12Max, dblPel12Max2;
fut_otbldat_t	odata;
KpInt32_t		lp;

     /* Check input parameters:  */
	if (table == (fut_otbldat_ptr_t)NULL)
	   return;				/* just don't crash! */
	if ((fwdgamma == (double)1.0) || (fwdgamma == (double)0.0))
	{					/* either trivial or uninvertible */
#if defined(KCP_FPU)			/* using the FPU? */
	   calcOtbl0FPU (table);
#else							/* all other programming environments */
	   calcOtbl0noFPU (table);
#endif
	   return;
	}
	invgamma = 1.0 / fwdgamma;
	dblPel12Max = (double) FUT_MAX_PEL12;
	dblPel12Max2 = 2.0 * dblPel12Max;

     /* Loop over table entries:  */
/* first third */
	 /* Rescale to 4080 (= 255 << 4):  */
	odata = (fut_otbldat_t)QUANT (f0, dblPel12Max);
	for (count = -FUT_GRD_MAXVAL; count < 0; count += 3) {
		*(table++) = odata;
	}

/* middle third */
	nument_inv = 1.0 / FUT_GRD_MAXVAL;
	for (; count < FUT_GRD_MAXVAL; count += 3)
	{
	     /* Treat otbl index as output from a gtbl scaled to 4095:  */
		p = (double)count * nument_inv;		/* in [0, 1] */

	     /* compute correction from inverse power law:  */
#if defined(KCP_FPU)			/* using the FPU? */
		p = (double)powfpu (p, invgamma);
#else							/* all other programming environments */
		p = (double)pow (p, invgamma);
#endif

	     /* Rescale to 4080 (= 255 << 4):  */
		lp = (KpInt32_t) (p * dblPel12Max2);
		if (lp < 0)
			lp = 0;
		else if (2*FUT_MAX_PEL12 < lp)
			lp = 2*FUT_MAX_PEL12;
		*table = (fut_otbldat_t)((lp + 1) >> 1);	/* in [0, norm - 1] */

		table++;
	}

/* last third */
	 /* Rescale to 4080 (= 255 << 4):  */
	odata = (fut_otbldat_t)QUANT (f1, dblPel12Max);
	for (; count < 2 * FUT_OUTTBL_ENT; count += 3) {
		*(table++) = odata;
	}
}


#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t
	calcOtblNFPU (fut_otbldat_ptr_t table, ResponseRecord* rrp, KpUInt32_t interpMode)
#else							/* all other programming environments */
PTErr_t
	calcOtblNnoFPU (fut_otbldat_ptr_t table, ResponseRecord* rrp, KpUInt32_t interpMode)
#endif
{
PTErr_t		PTErr;
KpUInt32_t	length;
KpUInt16_p	data;
KpInt32_t	count;
double		p, val, f1 = 1.0, f0 = 0.0;
xfer_t		Xfer;
double		nument_inv_l4, dblPel12Max, dblPel12Max2;
fut_otbldat_t	odata;
int				hint = 1;
KpInt32_t		lp;

	if (Kp_IsBadWritePtr (rrp, sizeof (*rrp))) {
		return KCP_BAD_ARG;
	}
	
	length = rrp->count;
	data = rrp->data;

	if ((Kp_IsBadWritePtr (table, (FUT_OUTTBL_ENT) * sizeof(fut_otbldat_t))) ||	/* absent table */
		(length == 0) ||											/* bad size */
		(Kp_IsBadReadPtr (data, length * (KpUInt32_t)sizeof(KpUInt16_t))) ||	/* absent table */
		(data[length - 1] == data[0])) {							/* empty domain */
			return KCP_BAD_ARG;
	}

	/* ==> (length > 1) && (data[length - 1] != data[0]) */
	/* assume monotonically nondecreasing or nonincreasing */

	switch (interpMode) {
	case KCP_TRC_LINEAR_INTERP:

	     /* Loop over table entries:  */
		for (count = 0; count < FUT_OUTTBL_ENT; count++) {
			/* Treat otbl index as output from a gtbl scaled to 4095:  */
			p = DEQUANT ((fut_gtbldat_t)count, FUT_GRD_MAXVAL);
	
		     /* Assume 3X extended range:  */
			p = 3.0 * p - 1.0;		/* [0, 1] -> [-1, 2] */
	
		     /* Clip and rescale to contents of data table:  */
			p = RESTRICT (p, 0.0, 1.0);
			p *= SCALEDOT16;		/* scaled to data table entries */
	
		     /* Find value relative to data table:  */
#if defined(KCP_FPU)			/* using the FPU? */
			val = calcInvertTRCFPU (p, data, length);
#else							/* all other programming environments */
			val = calcInvertTRCnoFPU (p, data, length);
#endif
	
		     /* Rescale and clip to [0, 1] */
			val /= (double)(length - 1);
			val = RESTRICT (val, 0.0, 1.0);
	
		     /* Rescale to 4080 (= 255 << 4):  */
			*(table++) = (fut_otbldat_t)QUANT (val, FUT_MAX_PEL12);
		}

		break;
		
	case KCP_TRC_LAGRANGE4_INTERP:
	
/* !!!!  LAGRANGIAN NOT WORK IF THE SOURCE DATA IS NON-MONOTONIC OR HAS FLAT REGIONS !!!! */

#if defined(KCP_FPU)			/* using the FPU? */
		PTErr = init_xferFPU (&Xfer, rrp);
#else							/* all other programming environments */
		PTErr = init_xfer (&Xfer, rrp);
#endif
		if (PTErr != KCP_SUCCESS) {
		   return KCP_BAD_ARG;
		}
		
#if defined(KCP_FPU)			/* using the FPU? */
		PTErr = set_xferFPU (&Xfer, 1, 0);
#else							/* all other programming environments */
		PTErr = set_xfer (&Xfer, 1, 0);
#endif
		if (PTErr != KCP_SUCCESS) {
		   return KCP_BAD_ARG;
		}
	
		nument_inv_l4 = 1.0 / FUT_GRD_MAXVAL;
		dblPel12Max = (double) FUT_MAX_PEL12;
		dblPel12Max2 = (double)  (2 * FUT_MAX_PEL12);
	
	/* first third */
		 /* Rescale to 4080 (= 255 << 4):  */
		odata = (fut_otbldat_t)QUANT (f0, dblPel12Max);
		for (count = -FUT_GRD_MAXVAL; count < 0; count += 3) {
			*(table++) = odata;
		}
	
	/* middle third */
		for (; count < FUT_GRD_MAXVAL; count += 3) {
		     /* Treat otbl index as output from a gtbl scaled to 4095:  */
			p = (double)count * nument_inv_l4;		/* in [0, 1] */
	
		     /* Clip and compute correction from inverse power law:  */
#if defined(KCP_FPU)			/* using the FPU? */
			p = xferFPU (&Xfer, p, &hint);
#else							/* all other programming environments */
			p = xfer (&Xfer, p, &hint);
#endif
	
		     /* Rescale to 4080 (= 255 << 4):  */
			lp = (KpInt32_t) (p * dblPel12Max2);
			if (lp < 0)
				lp = 0;
			else if (2*FUT_MAX_PEL12 < lp)
				lp = 2*FUT_MAX_PEL12;
			*table = (fut_otbldat_t)((lp + 1) >> 1);	/* in [0, norm - 1] */
	
			table++;
		}
	
	/* last third */
		 /* Rescale to 4080 (= 255 << 4):  */
		odata = (fut_otbldat_t)QUANT (f1, dblPel12Max);
		for (; count < 2 * FUT_OUTTBL_ENT; count += 3)
			*(table++) = odata;
	
		break;
		
	default:
		return KCP_BAD_ARG;
	}
	
	return KCP_SUCCESS;
}

#if defined(KCP_FPU)			/* using the FPU? */
double
	calcInvertTRCFPU (double p, KpUInt16_p data, KpUInt32_t length)
#else							/* all other programming environments */
double
	calcInvertTRCnoFPU (double p, KpUInt16_p data, KpUInt32_t length)
#endif
{
int32 i, j;
double val;

	if (data[length - 1] > data[0])		/* monotonic nondecreasing */
	{
		if (p <= (double)data[0])			/* at bottom or below table */
		{
			p = (double)data[0];			/* clip to bottom */
			i = 0;
			while ((double)data[i + 1] <= p)	/* find last bottom entry */
				i++;
			/* ==> data[i] == p < data[i + 1] */
			val = (double)i;
		}
		else if (p >= (double)data[length - 1])	/* at top or above table */
		{
			p = (double)data[length - 1];		/* clip to top */
			i = length - 1;
			while ((double)data[i - 1] >= p)	/* find first top entry */
				i--;
			/* ==> data[i] == p > data[i - 1] */
			val = (double)i;
		}
		else	/* data[0] < p < data[length - 1] */	/* within table */
		{
			i = 1;
			while (p > (double)data[i])		/* find upper bound */
				i++;
			/* ==> data[i - 1] < p <= data[i] */
	
			if (p < (double)data[i])		/* data[i - 1] < p < data[i] */
			{
				val = (double)(i - 1)		/* interpolate in [i - 1, i] */
					+ (p - (double)data[i - 1])
					/ ((double)data[i] - (double)data[i - 1]);
			}
			else					/* p == data[i] */
			{
				j = i;				/* find end of flat spot */
				while (p >= (double)data[j + 1])
					j++;
				/* ==> data[i - 1] < data[i] == p == data[j] < data[j + 1] */
				val = 0.5 * (double)(i + j);		/* pick midpoint of [i, j] */
			}
		}
	}
	else if (data[0] > data[length - 1]) {	/* monotonic nonincreasing */
		if (p <= (double)data[length - 1])		/* at bottom or below table */
		{
			p = (double)data[length - 1];		/* clip to bottom */
			i = length - 1;
			while ((double)data[i - 1] <= p)	/* find first bottom entry */
				i--;
			/* ==> data[i] == p < data[i - 1] */
			val = (double)i;
		}
		else if (p >= (double)data[0])		/* at top or above table */
		{
			p = (double)data[0];			/* clip to top */
			i = 0;
			while ((double)data[i + 1] >= p)	/* find last top entry */
				i++;
			/* ==> data[i] == p > data[i + 1] */
			val = (double)i;
		}
		else	/* data[0] > p > data[length - 1] */	/* within table */
		{
			i = 1;
			while (p < (double)data[i])		/* find upper bound */
				i++;
			/* ==> data[i - 1] > p >= data[i] */
	
			if (p > (double)data[i])		/* data[i - 1] > p > data[i] */
			{
				val = (double)(i - 1)		/* interpolate in [i - 1, i] */
					+ (p - (double)data[i - 1])
					/ ((double)data[i] - (double)data[i - 1]);
			}
			else					/* p == data[i] */
			{
				j = i;				/* find end of flat spot */
				while (p <= (double)data[j + 1])
					j++;
				/* ==> data[i - 1] > data[i] == p == data[j] > data[j + 1] */
				val = 0.5 * (double)(i + j);		/* pick midpoint of [i, j] */
			}
		}	
	}
	else { /* data[0] == data[length - 1] */
		/* return midpoint */
		val = ((double)length)/2 + 0.5F;
	}

	return (val);
}

