/*
 * @(#)lin3d.c	1.11 97/12/22

	Contains:	This module contains functions to create the Lin3d FuT.

				Created by lsh, November 11, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993 by Eastman Kodak Company, all rights reserved.

	Macintosh
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

#include "kcptmgr.h"
#include "makefuts.h"

static int32	channel = 0;

/*---------------------------------------------------------------------
 *  xfunc, yfunc, zfunc -- input mappings
 *---------------------------------------------------------------------
 */
static double xfunc (double x)
{
	return x;
}

static double yfunc (double y)
{
	return y;
}

static double zfunc (double z)
{
	return z;
}

/*---------------------------------------------------------------------
 *  trifunc -- grid-table functions (identity)
 *---------------------------------------------------------------------
 */
static double trifunc (double x, double y, double z)
{
	switch (channel) {
    case 1:
		return x;

    case 2:
		return y;

    case 3:
		return z;
	}

	return 6.023e+23;	/* why not? */
}

/*---------------------------------------------------------------------
 *  outfunc -- output mapping
 *---------------------------------------------------------------------
 */
static double outfunc (double s)
{
	return s;
}

/*------------------------------------------------------------------
 *  gfunc, ofunc -- quantization call-back functions for FuT library
 *------------------------------------------------------------------
 */
static fut_gtbldat_t gfunc (double *args)
{

double	g, x, y, z;

	x = args[0];
	y = args[1];
	z = args[2];

	g = trifunc (x, y, z);
	return QUANT (g, FUT_GRD_MAXVAL);
}

static fut_otbldat_t ofunc (fut_gtbldat_t q)
{
	double	s;

	s = DEQUANT (q, FUT_GRD_MAXVAL);
	s = outfunc (s);
	return QUANT (s, FUT_MAX_PEL12);
}

/*----------------------------------------------------------------------
 *  get_lin3d_fut --	construct an identity FuT
 *						to establish a grid for composition
 *----------------------------------------------------------------------
 */
fut_ptr_t
	get_lin3d_fut (int size)
{
fut_ptr_t		futp;
fut_itbl_ptr_t	itblx, itbly, itblz;
fut_gtbl_ptr_t	gtbl [3];
fut_otbl_ptr_t	otbl [3];
int32			ichan;
int32			iomask, sizeArray[3];
int				xsize, ysize, zsize;
threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	/* assume all dimensions are the same */
	xsize = ysize = zsize = size;
	
/* Compute shared input tables:  */
	itblx = fut_new_itbl (xsize, xfunc);
	itbly = fut_new_itbl (ysize, yfunc);
	itblz = fut_new_itbl (zsize, zfunc);

/* Compute grid tables and output tables:  */
	iomask = fut_iomask ("(xyz)");

	sizeArray[0] = xsize;
	sizeArray[1] = ysize;
	sizeArray[2] = zsize;

	for (ichan = 0; ichan < 3; ichan++) {
	   channel = ichan + 1;
	   gtbl [(int) ichan] = fut_new_gtblA (iomask,
										   (fut_gtbldat_t (*)(double *)) gfunc,
										   sizeArray);
	   otbl [(int) ichan] = fut_new_otbl (ofunc);
	}

/* Assemble FuT:  */
	iomask = fut_iomask ("xyz(xyz)");
	futp = fut_new (iomask, itblx, itbly, itblz,
							gtbl [0], otbl [0],
							gtbl [1], otbl [1],
							gtbl [2], otbl [2]);

	if ( futp->idstr != NULL ) {
		fut_free_idstr (futp->idstr);
		futp->idstr = NULL;
	}

	fut_free_tbls ((int32 FAR *)itblx, itbly, itblz, 
							gtbl [0], gtbl [1], gtbl [2], 
							otbl [0], otbl [1], otbl [2], NULL);

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	KCMDunloadGlobals();					/* Unlock this apps Globals */

	return (futp);
}

