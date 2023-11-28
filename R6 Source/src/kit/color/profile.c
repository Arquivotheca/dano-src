/*
 * @(#)profile.c	1.21 97/12/22

	Contains:	PTNewMatGamPT, makeProfileXform

	Written by:	The Boston White Sox

	Copyright:	1993-1996, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1996                 ***
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
#include "makefuts.h"
#include "matrix.h"
#include "kcpfut.h"

/*
 *	Module-specific definitions
 */

#define	X_D50		         0.9642		/* standard D50 white point */
#define	Y_D50		         1.0000
#define	Z_D50		         0.8249

static KpInt32_t xyz2cone (	KpMatrix_p src, KpMatrix_p dest);
static KpInt32_t cone2xyz (	KpMatrix_p src, KpMatrix_p dest);


#if !defined(KCP_FPU)			/* using the FPU? */
/*------------------------------------------------------------------------------
 *  PTNewMatGamPT -- Make a PT from a matrix and a set of 1D gamma tables, without white point adaptation.
 *------------------------------------------------------------------------------
 */
PTErr_t
	 PTNewMatGamPT(	FixedXYZColor_p		rXYZ,
					FixedXYZColor_p		gXYZ,
					FixedXYZColor_p		bXYZ,
					ResponseRecord_p	rTRC,
					ResponseRecord_p	gTRC,
					ResponseRecord_p	bTRC,
					u_int32			gridsize,
					bool			invert,
					PTRefNum_p		thePTRefNumP)
{
PTErr_t		PTErr;
newMGmode_t	newMGmode;

	newMGmode.adaptMode = KCP_NO_ADAPTATION;
	newMGmode.interpMode = KCP_TRC_LINEAR_INTERP;

	PTErr = PTNewMatGamAIPT (rXYZ, gXYZ, bXYZ, rTRC, gTRC, bTRC,
							gridsize, invert, &newMGmode, thePTRefNumP);

	return (PTErr);
}


/*------------------------------------------------------------------------------
 *  PTNewMatGamAIPT -- Make a PT from a matrix and a set of 1D gamma tables
 *------------------------------------------------------------------------------
 */
PTErr_t
	 PTNewMatGamAIPT(	FixedXYZColor_p		rXYZ,
						FixedXYZColor_p		gXYZ,
						FixedXYZColor_p		bXYZ,
						ResponseRecord_p	rTRC,
						ResponseRecord_p	gTRC,
						ResponseRecord_p	bTRC,
						KpUInt32_t			gridsize,
						bool				invert,
						newMGmode_p			newMGmodeP,
						PTRefNum_p			thePTRefNumP)
{
PTErr_t	errnum;
PTErr_t	theReturn;
fut_ptr_t theFut;

/* Check for valid ptrs */
	if (Kp_IsBadReadPtr(rXYZ, sizeof(*rXYZ)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(gXYZ, sizeof(*gXYZ)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(bXYZ, sizeof(*bXYZ)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(rTRC, sizeof(*rTRC)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(gTRC, sizeof(*gTRC)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(bTRC, sizeof(*bTRC)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadWritePtr(thePTRefNumP, sizeof(*thePTRefNumP)))
		return (KCP_BAD_PTR);
	if (Kp_IsBadReadPtr(newMGmodeP, sizeof(*newMGmodeP)))
		return (KCP_BAD_PTR);

	if ((newMGmodeP->interpMode != KCP_TRC_LINEAR_INTERP) &&
						(newMGmodeP->interpMode != KCP_TRC_LAGRANGE4_INTERP)) {
		return (KCP_BAD_ARG);
	}
	/* pass the input arguments along to the fut maker */
	if (gestaltNoFPU == kcpIsFPUpresent()) {
		theReturn = makeProfileXformNoFPU (rXYZ, gXYZ, bXYZ,	rTRC, gTRC, bTRC, 
									gridsize, invert, newMGmodeP, &theFut);
	} else {
		theReturn = makeProfileXformFPU (rXYZ, gXYZ, bXYZ,	rTRC, gTRC, bTRC, 
									gridsize, invert, newMGmodeP, &theFut);
	}
	if (theReturn == KCP_SUCCESS) {
		errnum = fut2PT (theFut, thePTRefNumP);	/* make into PT */
	}
	else {
		errnum = theReturn;
	}
	
	return (errnum);
}
#endif /* #if !defined(KCP_FPU) */


/*-------------------------------------------------------------------------------
 *  makeProfileXform -- given a set of tags defining a profile for a monitor or scanner,
 *		not having a 3D LUT, compute and return a PT that will transform 
 *		RGB to XYZ or vice versa
 *-------------------------------------------------------------------------------
 */
#if defined(KCP_FPU)			/* using the FPU? */
PTErr_t
	makeProfileXformFPU (	FixedXYZColor FAR*	rXYZ,
						FixedXYZColor FAR*	gXYZ, 
						FixedXYZColor FAR*	bXYZ,
						ResponseRecord FAR*	rTRC, 
						ResponseRecord FAR*	gTRC,
						ResponseRecord FAR*	bTRC, 
						u_int32				gridsize,
						bool				invert,
						newMGmode_p			newMGmodeP,
						fut_ptr_t FAR*		theFut)
#else							/* all other programming environments */
PTErr_t
	makeProfileXformNoFPU (	FixedXYZColor FAR*	rXYZ,
						FixedXYZColor FAR*	gXYZ, 
						FixedXYZColor FAR*	bXYZ,
						ResponseRecord FAR*	rTRC, 
						ResponseRecord FAR*	gTRC,
						ResponseRecord FAR*	bTRC, 
						u_int32				gridsize,
						bool				invert,
						newMGmode_p			newMGmodeP,
						fut_ptr_t FAR*		theFut)
#endif
{
PTErr_t		errnum;
ResponseRecord	FAR* RR[3];
double		row0[3], row1[3], row2[3], FAR* rowp[3];
MATRIXDATA	mdata;
double		xfactor, yfactor, zfactor;
KpMatrix_t	inputXYZ, curWP, adaptedXYZ;
KpMatrix_t	newWP, scale, newWPcone, curWPcone, inputCone, adaptedCone;
KpUInt32_t	row, col;
KpInt32_t	error1, error2;
		

	/* Create matrix from Profile tags:  */
	inputXYZ.nRows = 3;
	inputXYZ.nCols = 3;
	inputXYZ.coef[0][0] = (double)rXYZ->X / SCALEFIXED;	/* convert to double */
	inputXYZ.coef[0][1] = (double)gXYZ->X / SCALEFIXED;
	inputXYZ.coef[0][2] = (double)bXYZ->X / SCALEFIXED;
	inputXYZ.coef[1][0] = (double)rXYZ->Y / SCALEFIXED;
	inputXYZ.coef[1][1] = (double)gXYZ->Y / SCALEFIXED;
	inputXYZ.coef[1][2] = (double)bXYZ->Y / SCALEFIXED;
	inputXYZ.coef[2][0] = (double)rXYZ->Z / SCALEFIXED;
	inputXYZ.coef[2][1] = (double)gXYZ->Z / SCALEFIXED;
	inputXYZ.coef[2][2] = (double)bXYZ->Z / SCALEFIXED;

	/* Adjust given matrix by applying chromatic adaptation from current white point to D50  */
	adaptedXYZ.nRows = 3;	/* set up resultant matrix */
	adaptedXYZ.nCols = 3;

	curWP.nRows = 3;		/* set up vector for current white point */
	curWP.nCols = 1;
	KpMatZero (&curWP);

	curWP.coef[0][0] = inputXYZ.coef[0][0] + inputXYZ.coef[0][1] + inputXYZ.coef[0][2];	/* = X_RGB */
	if (curWP.coef[0][0] <= 0.0) {
		return KCP_INCON_PT;
	}

	curWP.coef[1][0] = inputXYZ.coef[1][0] + inputXYZ.coef[1][1] + inputXYZ.coef[1][2];	/* = Y_RGB */
	if (curWP.coef[1][0] <= 0.0) {
		return KCP_INCON_PT;
	}

	curWP.coef[2][0] = inputXYZ.coef[2][0] + inputXYZ.coef[2][1] + inputXYZ.coef[2][2];	/* = Z_RGB */
	if (curWP.coef[2][0] <= 0.0) {
		return KCP_INCON_PT;
	}

	switch (newMGmodeP->adaptMode) {
	
	case KCP_NO_ADAPTATION:
	
		error1 = KpMatCopy (&inputXYZ, &adaptedXYZ);	/* use input XYZs without modification */
		if (error1 != 1) {
			return KCP_INCON_PT;
		}

		break;
		
	case KCP_XYZD50_ADAPTATION:
	
		/* calculate X, Y, and Z adaptation factors */
		xfactor = X_D50 / curWP.coef[0][0];
		yfactor = Y_D50 / curWP.coef[1][0];
		zfactor = Z_D50 / curWP.coef[2][0];

		/* apply adaptation to matrix */
		for (col = 0; col < 3; col++) {
			adaptedXYZ.coef[0][col] = inputXYZ.coef[0][col] * xfactor;
			adaptedXYZ.coef[1][col] = inputXYZ.coef[1][col] * yfactor;
			adaptedXYZ.coef[2][col] = inputXYZ.coef[2][col] * zfactor;
		}
		
		break;
		
	case KCP_VONKRIESD50_ADAPTATION:
	
		newWP.nRows = 3;			/* set up new white point vector */
		newWP.nCols = 1;

		newWP.coef[0][0] = X_D50;	/* which is D50 */
		newWP.coef[1][0] = Y_D50;
		newWP.coef[2][0] = Z_D50;

		scale.nRows = 3;			/* set up scaling vector */
		scale.nCols = 1;

		error1 = xyz2cone (&newWP, &newWPcone);	/* convert new WP to cone primaries */
		error2 = xyz2cone (&curWP, &curWPcone);	/* convert current WP to cone primaries */
		if ((error1 != 1) || (error2 != 1)) {
			return KCP_INCON_PT;
		}

		error1 = KpMatDotDiv (&newWPcone, &curWPcone, &scale);	/* calculate ratios */
		if (error1 != 1) {
			return KCP_INCON_PT;
		}

		for (col = 1; col < 3; col++) {	/* replicate scale vector to 2nd and 3rd columns */
			for (row = 0; row < 3; row++) {
				scale.coef[row][col] = scale.coef[row][0];
			}
		}

		scale.nCols = 3;			/* has 3 columns now */
		
		error1 = xyz2cone (&inputXYZ, &inputCone);	/* convert input XYZs to cone primaries */
		if (error1 != 1) {
			return KCP_INCON_PT;
		}
		
		error1 = KpMatDotMul (&inputCone, &scale, &adaptedCone);	/* adapt cone primaries */
		if (error1 != 1) {
			return KCP_INCON_PT;
		}

		error1 = cone2xyz (&adaptedCone, &adaptedXYZ);	/* convert input XYZs to cone primaries */
		if (error1 != 1) {
			return KCP_INCON_PT;
		}
		
		break;
		
	default:
		return KCP_BAD_ARG;
	}

	/* Create matrix-data structure:  */
	rowp[0] = row0;				/* set row pointers */
	rowp[1] = row1;
	rowp[2] = row2;

	RR[0] = rTRC;				/* set record pointers */
	RR[1] = gTRC;
	RR[2] = bTRC;

	mdata.dim = 3;				/* always! */
	mdata.matrix = rowp;		/* set pointers */
	mdata.response = RR;

	row0[0] = adaptedXYZ.coef[0][0] * XYZSCALE;	/* load matrix coefficients */
	row0[1] = adaptedXYZ.coef[0][1] * XYZSCALE;	/* and scale for ICC */
	row0[2] = adaptedXYZ.coef[0][2] * XYZSCALE;
	row1[0] = adaptedXYZ.coef[1][0] * XYZSCALE;
	row1[1] = adaptedXYZ.coef[1][1] * XYZSCALE;
	row1[2] = adaptedXYZ.coef[1][2] * XYZSCALE;
	row2[0] = adaptedXYZ.coef[2][0] * XYZSCALE;
	row2[1] = adaptedXYZ.coef[2][1] * XYZSCALE;
	row2[2] = adaptedXYZ.coef[2][2] * XYZSCALE;

	/* Create import or export PT, according to user choice: */
	if (invert == False) {
#if defined(KCP_FPU)			/* using the FPU? */
		errnum = makeForwardXformFromMatrixFPU
					(&mdata, gridsize, newMGmodeP->interpMode, theFut);
	}
	else {
		errnum = makeInverseXformFromMatrixFPU
					(&mdata, gridsize, newMGmodeP->interpMode, theFut);
#else							/* all other programming environments */
		errnum = makeForwardXformFromMatrixNoFPU
					(&mdata, gridsize, newMGmodeP->interpMode, theFut);
	}
	else {
		errnum = makeInverseXformFromMatrixNoFPU
					(&mdata, gridsize, newMGmodeP->interpMode, theFut);
#endif
	}
	
	return errnum;
}


/* Convert tristimulus values to the rho, gamma, beta
 * cone promaries.  See Hunt, Measuring Colour pg. 71.
 */

static KpInt32_t
	xyz2cone (	KpMatrix_p src,
				KpMatrix_p dest)
{
KpMatrix_t toCone = {3, 3,
					 0.40024,	0.70760,	-0.08081,
					-0.22630,	1.16532,	 0.04570,
					 0.0,		0.0,		 0.91822};
KpInt32_t	error;

	error = KpMatMul (&toCone, src, dest);
	
	return error;
}


/* Convert tristimulus values to the rho, gamma, beta
 * cone promaries.  See Hunt, Measuring Colour pg. 71.
 */

static KpInt32_t
	cone2xyz (	KpMatrix_p src,
				KpMatrix_p dest)
{
KpMatrix_t fromCone = {3, 3,
					1.85993638745584,	-1.12938161858009,	 0.21989740959619,
					0.36119143624177,	 0.63881246328504,	-0.00000637059684,
					0.0,				 0.0,				 1.08906362309686};
KpInt32_t	error;

	error = KpMatMul (&fromCone, src, dest);
	
	return error;
}

