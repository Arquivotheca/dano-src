/*
 * @(#)outmat.c	1.11 97/12/22

	Contains:	makeOutputMatrixXform

	Written by:	The Boston White Sox

	Copyright:	1993, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

			11/24/93	RFP	Return PTErr_t
			11/21/93	pgt	Port to Win32
			11/17/93	RFP	Return status codes
			11/11/93	RFP	outmat.c:  for output profile
			11/09/93	RFP	lcs.c:  bidirectional
			11/08/93	RFP	Use matrixdata interface
			11/02/93	RFP	Use matrixfut interface
			10/19/93	RFP	Adjust XYZ scaling factor
			10/18/93	RFP	White-point adaptation added
			10/14/93	RFP	rcs2xyz.c:  developed on SunOS

	Windows Revision Level:
		$Workfile$
		$Logfile$
		$Revision$
		$Date$
		$Author$

	SCCS Revision:


	To Do:		

*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/




/*
 *	General definitions
 */

#include "kcms_sys.h"

#if defined (KPMAC68K)
#include <Gestalt.h>
#else
#define gestaltNoFPU 0
#endif

#include <math.h>
#include "csmatrix.h"

/*-------------------------------------------------------------------------------
 *  makeOutputMatrixXform -- given a 3 x 3 matrix as part of a Lut8Bit or Lut16Bit
 *		profile for an output device, compute and return the equivalent PT
 *-------------------------------------------------------------------------------
 */

#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t
	makeOutputMatrixXformFPU (	KpF15d16_t FAR*	matrix,
							u_int32			gridsize,
							fut_ptr_t FAR*	theFut)
#else							/* all other programming environments */
PTErr_t
	makeOutputMatrixXformNoFPU (	KpF15d16_t FAR*	matrix,
							u_int32			gridsize,
							fut_ptr_t FAR*	theFut)
#endif
{
PTErr_t	PTErr;
ResponseRecord	rTRC, gTRC, bTRC, FAR* RR[3];
double		row0[3], row1[3], row2[3], FAR* rowp[3];
double		one[3];
MATRIXDATA	mdata;

     /* Initialize ResponseRecords to do nothing (identity mapping):  */
	rTRC.count = gTRC.count = bTRC.count = 0;
	RR[0] = &rTRC;					/* set record pointers */
	RR[1] = &gTRC;
	RR[2] = &bTRC;

     /* Form backward matrix (XYZ -> ABC):  */
	row0[0]	= (double)matrix[0] / SCALEFIXED;
	row0[1] = (double)matrix[1] / SCALEFIXED;
	row0[2] = (double)matrix[2] / SCALEFIXED;
	row1[0] = (double)matrix[3] / SCALEFIXED;
	row1[1] = (double)matrix[4] / SCALEFIXED;
	row1[2] = (double)matrix[5] / SCALEFIXED;
	row2[0] = (double)matrix[6] / SCALEFIXED;
	row2[1] = (double)matrix[7] / SCALEFIXED;
	row2[2] = (double)matrix[8] / SCALEFIXED;

	rowp[0] = row0;					/* set row pointers */
	rowp[1] = row1;
	rowp[2] = row2;

     /* Invert matrix (ABC -> XYZ):  */
	one[0] = one[1] = one[2] = 1.0;			/* arbitrary vector */
	if (solvemat (3, rowp, one) != 0) {		/* replaces matrix with inverse */
		return KCP_INCON_PT;
	}

     /* Construct matrix-data object:  */
	mdata.dim = 3;					/* always! */
	mdata.matrix = rowp;				/* set pointers */
	mdata.response = RR;

     /* Make and return inverse fut (XYZ -> ABC):  */
	if (gestaltNoFPU == kcpIsFPUpresent()) {
		PTErr = makeInverseXformFromMatrixNoFPU
					(&mdata, gridsize, KCP_TRC_LAGRANGE4_INTERP, theFut);
	} else {
		PTErr = makeInverseXformFromMatrixFPU
					(&mdata, gridsize, KCP_TRC_LAGRANGE4_INTERP, theFut);
	}
	
	return PTErr;
}

