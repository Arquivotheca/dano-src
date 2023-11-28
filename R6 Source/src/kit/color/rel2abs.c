/*
 * @(#)rel2abs.c	1.8 97/12/22

	Contains:	PTGetRelToAbsPT

	Written by:	Drivin' Team

	Copyright:	1997, by Eastman Kodak Company (all rights reserved)

 *** COPYRIGHT (c) Eastman Kodak Company, 1997                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************/


#include "kcms_sys.h"
#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "attrib.h"
#include "attrcipg.h"
#include "fut.h"
#include "matdata.h"
#include "kcpfut.h"
#include "kcptmgr.h"
#include "kcpfchn.h"

typedef struct FloatXYZColor_s {
	KpFloat32_t	X;
	KpFloat32_t	Y;
	KpFloat32_t	Z;
} FloatXYZColor_t;

#if defined (KPMAC68K)
#include <Gestalt.h>
#else
#define gestaltNoFPU 0
#endif

/*------------------------------------------------------------------------------
 *  PTGetRelToAbsPT -- generate a PT which
 					converts from ICC relative colorimetry to ICC absolute colorimetry
 					
 	(absolute color) = ((media white point) / (profile white point)) * (relative color)

 	so
 	(source absolute color) = ((source media white point) / (source profile white point)) * (source relative color)
 	and
 	(dest absolute color) = ((dest media white point) / (dest profile white point)) * (dest relative color)

	equating source and dest absolute colors
	 	(dest relative color) = ((source media white point) / (dest media white point))
	 						  * ((dest profile white point) / (source profile white point))
	 						  * (source relative color)

 *------------------------------------------------------------------------------
 */
PTErr_t
	 PTGetRelToAbsPT(	KpUInt32_t		RelToAbsMode,
						PTRelToAbs_p	PTRelToAbs,
						PTRefNum_p		PTRefNumPtr)
{
PTErr_t				PTErr;
KpInt32_t			ret;
FloatXYZColor_t		sMWP, dMWP, sPWP, dPWP;
KpInt8_t			cie_attrib [KCM_MAX_ATTRIB_VALUE_LENGTH+1];
Fixed_t				matrix[MF_MATRIX_DIM * MF_MATRIX_DIM];
fut_ptr_t			theFutFromMatrix = NULL;

	if (RelToAbsMode != 0) {			/* just one mode now.  allows for future expansion of the function */
		return KCP_NOT_IMPLEMENTED;
	}

	sMWP.X = PTRelToAbs->srcMediaWhitePoint.X / (KpFloat32_t)KpF15d16Scale;	/* convert fixed point XYZ to floating point */
	sMWP.Y = PTRelToAbs->srcMediaWhitePoint.Y / (KpFloat32_t)KpF15d16Scale;
	sMWP.Z = PTRelToAbs->srcMediaWhitePoint.Z / (KpFloat32_t)KpF15d16Scale;
	dMWP.X = PTRelToAbs->dstMediaWhitePoint.X / (KpFloat32_t)KpF15d16Scale;
	dMWP.Y = PTRelToAbs->dstMediaWhitePoint.Y / (KpFloat32_t)KpF15d16Scale;
	dMWP.Z = PTRelToAbs->dstMediaWhitePoint.Z / (KpFloat32_t)KpF15d16Scale;
	sPWP.X = PTRelToAbs->srcProfileWhitePoint.X / (KpFloat32_t)KpF15d16Scale;
	sPWP.Y = PTRelToAbs->srcProfileWhitePoint.Y / (KpFloat32_t)KpF15d16Scale;
	sPWP.Z = PTRelToAbs->srcProfileWhitePoint.Z / (KpFloat32_t)KpF15d16Scale;
	dPWP.X = PTRelToAbs->dstProfileWhitePoint.X / (KpFloat32_t)KpF15d16Scale;
	dPWP.Y = PTRelToAbs->dstProfileWhitePoint.Y / (KpFloat32_t)KpF15d16Scale;
	dPWP.Z = PTRelToAbs->dstProfileWhitePoint.Z / (KpFloat32_t)KpF15d16Scale;
		
	matrix[0] = (Fixed_t)(((sMWP.X * dPWP.X) / (dMWP.X * sPWP.X) * KpF15d16Scale) + 0.5);	/* fill in the matrix */
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = (Fixed_t)(((sMWP.Y * dPWP.Y) / (dMWP.Y * sPWP.Y) * KpF15d16Scale) + 0.5);
	matrix[5] = 0;
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = (Fixed_t)(((sMWP.Z * dPWP.Z) / (dMWP.Z * sPWP.Z) * KpF15d16Scale) + 0.5);

	if (gestaltNoFPU == kcpIsFPUpresent()) {
		ret = makeOutputMatrixXformNoFPU ((Fixed_p)&matrix, PTRelToAbs->gridSize, &theFutFromMatrix);
	} else {
		ret = makeOutputMatrixXformFPU ((Fixed_p)&matrix, PTRelToAbs->gridSize, &theFutFromMatrix);
	}
	if (ret != 1) {
		goto GetOut2;
	}

	PTErr = fut2PT (theFutFromMatrix, PTRefNumPtr);	/* make into PT */
	if (PTErr != KCP_SUCCESS) {
		goto GetOut2;
	}

	KpItoa(KCM_CIE_XYZ, cie_attrib);	/* Stuff the input and output space attributes with XYZ */

	PTErr = PTSetAttribute (*PTRefNumPtr, KCM_IN_SPACE, cie_attrib);
	if (PTErr != KCP_SUCCESS) {
		goto GetOut2;
	}
	PTErr = PTSetAttribute (*PTRefNumPtr, KCM_OUT_SPACE, cie_attrib);
	if (PTErr != KCP_SUCCESS) {
		goto GetOut2;
	}

/*	PTErr = PTStoreD (*PTRefNumPtr, "d50.pt");	/* for debug */

	return (KCP_SUCCESS);

	
GetOut2:
	return (KCP_REL2ABS_ERROR);
}



