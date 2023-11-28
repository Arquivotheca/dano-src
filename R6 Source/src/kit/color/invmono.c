/*
 * @(#)invmono.c	1.4 97/12/22

	Contains:	makeInverseXformMono

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

#include <math.h>
#include <string.h>
#include "csmatrix.h"
#include "monopt.h"

PTErr_t makeInverseXformMono (ResponseRecord FAR*	grayTRC,
									KpUInt32_t 		gridsize,
									fut_ptr_t *		theFut)
{
PTErr_t			PTErr = KCP_FAILURE;
KpInt32_t		dim[3];
fut_ptr_t		futp;
fut_otbldat_ptr_t	otblDat;
ResponseRecord_t rrt;
int				futReturn;
double			gamma;
KpUInt16_t		rrpData[2] = { 0, RRECORD_DATA_SIZE -1 };

 	/* Check for valid ptrs */
	if (Kp_IsBadReadPtr(grayTRC, sizeof(*grayTRC)))
		return KCP_BAD_PTR;

	if (gridsize < 2) {
	   return KCP_INCON_PT;
	}

	/* assume all dimensions are the same */
	dim[0] = dim[1] = dim[2] = (KpInt32_t)gridsize;
	
	/* Create (3D -> 1D) FuT */
	futp = fut_new_empty ((KpInt32_t)3, dim, (KpInt32_t)1);	
	if (futp == FUT_NULL) {
	   return KCP_NO_MEMORY;
	}

	/* compute new grid table entries */
	if (!fut_calc_gtblA (futp->chan[0]->gtbl,
						(fut_gtbldat_t (*)(double *)) gtblFunc)) {
		fut_free (futp);
		return KCP_SYSERR_0;
	}

	/* compute new output table entries */
	if (!fut_calc_otbl (futp->chan[0]->otbl, otblFunc)) {
		fut_free (futp);
		return KCP_SYSERR_0;
	}

	if ( futp->idstr != NULL ) {
		fut_free_idstr (futp->idstr);
		futp->idstr = NULL;
	}

	/* get address of the first output table */
	futReturn = fut_get_otbl (futp, 0, &otblDat);
	if ((futReturn != 1) || (otblDat == (fut_otbldat_ptr_t)NULL))
	{
		fut_free (futp);
		return KCP_SYSERR_0;
	}

	/* setup the output table */
	switch (grayTRC->count)
	{
		case 0:
				/* setup the responseRecord struct */
				rrt.count = 2;
				rrt.data = rrpData;

				/* make the output table */
				PTErr = calcOtblLN (otblDat, &rrt);
				if (PTErr != KCP_SUCCESS) {
					fut_free (futp);
					return (PTErr);
				}
				break;
		case 1:
				gamma = (double)grayTRC->data[0] / SCALEDOT8;
				if (gamma <= 0.0) {
					fut_free (futp);
					return KCP_INCON_PT;
				}

				/* make the output table */
				PTErr = calcOtblL1 (otblDat, gamma);
				if (PTErr != KCP_SUCCESS) {
					fut_free (futp);
					return (PTErr);
				}
				break;

		default:
				/* make the output table */
				makeInverseMonotonic (grayTRC->count, grayTRC->data);
				PTErr = calcOtblLN (otblDat, grayTRC);
				if (PTErr != KCP_SUCCESS) {
					fut_free (futp);
					return (PTErr);
				}
	}

     /* Set fut pointer and return successfully:  */
	*theFut = futp;
	return (PTErr);
}

