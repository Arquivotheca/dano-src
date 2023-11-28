/*
 * @(#)calcxtbl.c	1.6 97/12/22

	Contains:	base on the FPU flag, chooses which routine to call at runtime. 

	Written by:	The Boston White Sox

	Copyright:	1993, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

*/

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
#include "kcmptdef.h"
#include "kcmptlib.h"
#include "kcptmgr.h"
#include "fut.h"
#include "csmatrix.h"

static	KpInt32_t	kcpFPUType = gestaltNoFPU;

#if defined (KPMAC68K)
/************************************************************************/
/* For the macintosh find out if there is a floating point processor.	*/
/*	for everybody else there is not.									*/
/************************************************************************/
void
kcpGetFPU ()
{
	OSErr	myErr;
	
	myErr = Gestalt(gestaltFPUType, &kcpFPUType);
}
#endif

/************************************************************/
/*	Return gestalt status for whether or not FPU is present */
/************************************************************/
KpInt32_t
	kcpIsFPUpresent ()
{
	return (kcpFPUType);
}

#if !defined (KPMAC68K)
/************************************************************/
/* If this is not a KPMAC68K these functions do not exhist. */
/************************************************************/

PTErr_t
	makeOutputMatrixXformFPU (	KpF15d16_t FAR*	matrix,
							u_int32			gridsize,
							fut_ptr_t FAR*	theFut)
{
	if (matrix) {}
	if (gridsize) {}
	if (theFut) {}
	
	return (KCP_SYSERR_2);
}

PTErr_t makeProfileXformFPU (FixedXYZColor_p rXYZ,
							FixedXYZColor_p gXYZ,
							FixedXYZColor_p bXYZ, 
							ResponseRecord_p rTRC,
							ResponseRecord_p gTRC,
							ResponseRecord_p bTRC, 
							u_int32 gridsize,
							bool invert,
							newMGmode_p newMGmodeP,
							fut_ptr_t FAR* theFut)
{
	if (rXYZ) {}
	if (gXYZ) {}
	if (bXYZ) {}
	if (rTRC) {}
	if (gTRC) {}
	if (bTRC) {}
	if (gridsize) {}
	if (invert) {}
	if (newMGmodeP) {}
	if (theFut) {}
	
	return (KCP_SYSERR_2);
}

PTErr_t makeForwardXformFromMatrixFPU (LPMATRIXDATA	mdata,
									u_int32			gridsize,
									KpUInt32_t		interpMode,
									fut_ptr_t *		theFut)
{
	if (mdata) {}
	if (gridsize) {}
	if (interpMode) {}
	if (theFut) {}
	
	return (KCP_SYSERR_2);
}

PTErr_t makeInverseXformFromMatrixFPU (LPMATRIXDATA	mdata,
									u_int32 		gridsize,
									KpUInt32_t		interpMode,
									fut_ptr_t *		theFut)
{
	if (mdata) {}
	if (gridsize) {}
	if (interpMode) {}
	if (theFut) {}
	
	return (KCP_SYSERR_2);
}

void calcGtbl3FPU (fut_gtbldat_ptr_t *table, int32 *gridSizes, 
					double **rows, bool xrange)
{
	if (table) {}
	if (gridSizes) {}
	if (rows) {}
	if (xrange) {}
}

void calcItbl1FPU (fut_itbldat_ptr_t table, int32 gridSize, double gamma)
{
	if (table) {}
	if (gridSize) {}
	if (gamma) {}
}

void calcItbl256FPU (fut_itbldat_ptr_t table, int32 gridSize, unsigned short lut[])
{
	if (table) {}
	if (gridSize) {}
	if (lut) {}
}

PTErr_t calcItblNFPU (fut_itbldat_ptr_t table, int32 gridSize, ResponseRecord* rrp, KpUInt32_t interpMode)
{
	if (table) {}
	if (gridSize) {}
	if (rrp) {}
	if (interpMode) {}
	return (KCP_SUCCESS);
}

void calcOtbl0FPU (fut_otbldat_ptr_t table)
{
	if (table) {}
}

void calcOtbl1FPU (fut_otbldat_ptr_t table, double fwdgamma)
{
	if (table) {}
	if (fwdgamma) {}
}

PTErr_t calcOtblNFPU (fut_otbldat_ptr_t table, ResponseRecord* rrp, KpUInt32_t interpMode)
{
	if (table) {}
	if (rrp) {}
	if (interpMode) {}
	return (KCP_SUCCESS);
}

double calcInvertTRCFPU (double p, KpUInt16_p data, KpUInt32_t length)
{
	if (p) {}
	if (data) {}
	if (length) {}
	return (0);
}

#endif


