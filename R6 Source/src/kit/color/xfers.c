/*
 * @(#)xfers.c	1.12 97/12/22

	Contains:	init_xfer, (free_xfer), set_xfer, xfer

	Written by:	The Boston White Sox

	Copyright:	1993-1995, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

			11/24/93	RFP	Use PTErr_t
			11/21/93	PGT	port to Win32
			11/17/93	RFP	Use xfers.h; xfer_t* arguments
			11/16/93	RFP	Non-re-entrant, for now:  NUMXFER = 1
			11/11/93	RFP	xfers.c:  Re-entrant version for DLL
			11/09/93	RFP	New version for ColorSync profiles
		 	11/18/92	RFP	gamxfer.c:  Coarse-fine version for gammafut
							(from xfer.c in transferfut)

*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1995                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/




/*
 *	General definitions
 */

#include <math.h>
#include "kcmsos.h"
#include "fut.h"
#include "matdata.h"
#include "csmatrix.h"
#include "xfers.h"

/*-------------------------------------------------------------------
 *  init_xfer -- initialize the transfer tables by transferring coarse
 *			control points from a ColorSync ResponseRecord
 *			and interpolating the fine tables from them;
 *			returns +1 for success, -1 for failure to
 *			allocate memory for coarse table
 *-------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t init_xferFPU (xfer_t FAR* xferp, ResponseRecord FAR* rrp)
#else							/* all other programming environments */
PTErr_t init_xfer (xfer_t FAR* xferp, ResponseRecord FAR* rrp)
#endif
{
	double	val;			/* input variables */
	int	numcoarse;		/* number of input control points */
	double	FAR* coarse[2];		/* storage for control points */
	int	i;			/* control-point index */
	int	hint;

     /* Verify inputs:  */
	if (	(xferp == (xfer_t FAR*)NULL) ||
		(rrp == (ResponseRecord FAR*)NULL) ||
		(rrp->count < 2) ||
		(rrp->data == (unsigned short FAR*)NULL)	)
	   return KCP_SYSERR_0;
	   
     /* Allocate space for coarse tables:  */
	numcoarse = rrp->count - 1;	/* skip zero entry to avoid infinity in logarithm */
	coarse[0] = (double *)ALLOC (numcoarse, sizeof(double));
	if (coarse[0] == NULL)
	{
	   return KCP_NO_MEMORY;
	}
	coarse[1] = (double *)ALLOC (numcoarse, sizeof(double));
	if (coarse[1] == NULL)
	{
	   DALLOC (coarse[0]);	/* release storage */
	   return KCP_NO_MEMORY;
	}

     /* Build coarse tables from ResponseRecord:  */
	for (i = 0; i < numcoarse; i++)
	{
	   val = (double)(i + 1) / (double)numcoarse;	/* skip zero to avoid infinite log */
#if defined(KCP_FPU)			/* using the FPU? */
	   coarse[0][i] = -log10fpu (val);
#else							/* all other programming environments */
	   coarse[0][i] = -log10 (val);
#endif
	   val = (double)rrp->data[i + 1] / SCALEDOT16;
	   val = MAX (val, 1.0e-12);			/* clip to avoid infinite log */
#if defined(KCP_FPU)			/* using the FPU? */
	   coarse[1][i] = -log10fpu (val);
#else							/* all other programming environments */
	   coarse[1][i] = -log10 (val);
#endif
	}

     /* Build fine tables by interpolating in coarse tables:  */
	hint = 1;
	for (i = 0; i < NUMFINE; i++)				/* spaced code values */
	{
	   double	code;

	   code = (double)i * 2.4 / (double)(NUMFINE - 1);	/* equally spaced in [0, 2.4] */
	   xferp->nonlinear[i] = code;
#if defined(KCP_FPU)			/* using the FPU? */
	   xferp->linear[i] = f4lFPU (code, coarse[0], coarse[1], numcoarse, &hint);
#else							/* all other programming environments */
	   xferp->linear[i] = f4l (code, coarse[0], coarse[1], numcoarse, &hint);
#endif
	}

     /* Delete coarse tables:  */
	DALLOC (coarse[0]);
	DALLOC (coarse[1]);

	return KCP_SUCCESS;
}

/*-------------------------------------------------------------------
 *  free_xfer -- 
 *-------------------------------------------------------------------
 */
/********  CURRENTLY POINTLESS  ********
void free_xfer (xfer_t FAR* xferp)
{
	;
}
****************************************/

/*-------------------------------------------------------------------
 *  set_xfer -- select source and destination channels for transfer;
 *		returns +1 for success, -2 for null xferp, -3 for bad
 *		channel arguments
 *-------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t set_xferFPU (xfer_t FAR* xferp, int source, int dest)
#else							/* all other programming environments */
PTErr_t set_xfer (xfer_t FAR* xferp, int source, int dest)
#endif
{
     /* Validate arguments:  */
	if (xferp == (xfer_t FAR*)NULL)
	   return KCP_SYSERR_0;
	if ((source < 0) || (source > 1))
	   return KCP_SYSERR_0;
	if ((dest < 0) || (dest > 1))
	   return KCP_SYSERR_0;

     /* Set channels:  */
	xferp->from = (source == 0)	? xferp->nonlinear : xferp->linear;
	xferp->to = (dest == 0)		? xferp->nonlinear : xferp->linear;

	return KCP_SUCCESS;
}

/*-------------------------------------------------------------------
 *  xfer -- transfer function interpolated in given table
 *-------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
double xferFPU (xfer_t FAR* xferp, double inval, int *hint)
#else							/* all other programming environments */
double xfer (xfer_t FAR* xferp, double inval, int *hint)
#endif
{
	double	indens, outdens;	/* densities */

#if KCP_OLDCODE
     /* Special case (no-op):  */
	if (xferp == (xfer_t FAR*)NULL)
	   return inval;		/* default:  identity transfer */
#endif

     /* Interpolate in density space:  */
	inval = MAX (inval, 1.0e-12);	/* Dmax = 12 */
#if defined(KCP_FPU)			/* using the FPU? */
	indens = -log10fpu (inval);
	outdens = f4lFPU (indens, xferp->from, xferp->to, NUMFINE, hint);
	return powfpu (0.10, outdens);
#else							/* all other programming environments */
	indens = -log10 (inval);
	outdens = f4l (indens, xferp->from, xferp->to, NUMFINE, hint);
	return pow (0.10, outdens);
#endif
}


