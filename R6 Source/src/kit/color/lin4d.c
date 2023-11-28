/*
 * @(#)lin4d.c	1.16 97/12/22

	Contains:	This module contains functions to create the Lin4d FuT.

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

static int	channel = 0;

/*---------------------------------------------------------------------
 *  xfun, yfun, zfun, tfun -- input mappings
 *---------------------------------------------------------------------
 */
static double xfun (double x)
{
	return x;
}

static double yfun (double y)
{
	return y;
}

static double zfun (double z)
{
	return z;
}

static double tfun (double t)
{
	return t;
}

/*---------------------------------------------------------------------
 *  quadfun -- grid-table functions (identity)
 *---------------------------------------------------------------------
 */
static double quadfun (double x, double y, double z, double t)
{
	switch (channel) {
    case 1:
		return x;
    case 2:
		return y;
    case 3:
		return z;
    case 4:
		return t;
	}

	return 6.023e+23;	/* why not? */
}

/*---------------------------------------------------------------------
 *  outfun -- output mapping
 *---------------------------------------------------------------------
 */
static double outfun (double s)
{
	return s;
}

/*------------------------------------------------------------------
 *  gfun, ofun -- quantization call-back functions for FuT library
 *------------------------------------------------------------------
 */
static fut_gtbldat_t gfun (double *args)
{

double	g, x, y, z, t;

	x = args[0];
	y = args[1];
	z = args[2];
	t = args[3];

	g = quadfun (x, y, z, t);
	return QUANT (g, FUT_GRD_MAXVAL);
}

static fut_otbldat_t ofun (fut_gtbldat_t q)
{
	double	s;

	s = DEQUANT (q, FUT_GRD_MAXVAL);
	s = outfun (s);
	return QUANT (s, FUT_MAX_PEL12);
}

/*----------------------------------------------------------------------
 *  get_lin4d_fut --	construct an identity FuT
 *						to establish a grid for composition
 *----------------------------------------------------------------------
 */
fut_ptr_t
	get_lin4d_fut (int size)
{
fut_ptr_t		futp;
fut_itbl_ptr_t	itblx, itbly, itblz, itblt;
fut_gtbl_ptr_t	gtbl[4];
fut_otbl_ptr_t	otbl[4];
int				ichan;
int32			iomask, sizeArray[4];
int				xsize, ysize, zsize, tsize;
threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	KpEnterCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);

	/* assume all dimensions are the same */
	xsize = ysize = zsize = tsize = size;
	
/* Compute shared input tables:  */
	itblx = fut_new_itbl (xsize, xfun);
	itbly = fut_new_itbl (ysize, yfun);
	itblz = fut_new_itbl (zsize, zfun);
	itblt = fut_new_itbl (tsize, tfun);

/* Compute grid tables and output tables:  */
	iomask = fut_iomask ("(xyzt)");

	sizeArray[0] = xsize;
	sizeArray[1] = ysize;
	sizeArray[2] = zsize;
	sizeArray[3] = tsize;

	for (ichan = 0; ichan < 4; ichan++) {
	   channel = ichan + 1;
	   gtbl [ichan] = fut_new_gtblA (iomask,
									 (fut_gtbldat_t (*)(double *)) gfun,
									 sizeArray);
	   otbl [ichan] = fut_new_otbl (ofun);
	}

/* Assemble FuT:  */
	iomask = fut_iomask ("xyzt(xyzt)");
	futp = fut_new (iomask, itblx, itbly, itblz, itblt,
						gtbl [0], otbl [0],
						gtbl [1], otbl [1],
						gtbl [2], otbl [2],
						gtbl [3], otbl [3]);

	if ( futp->idstr != NULL ) {
		fut_free_idstr (futp->idstr);
		futp->idstr = NULL;
	}

	fut_free_tbls ((int32 FAR *) itblx, itbly, itblz, itblt, 
						gtbl [0], gtbl [1], gtbl [2], gtbl [3],
						otbl [0], otbl [1], otbl [2], otbl [3], NULL);

	KpLeaveCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	KCMDunloadGlobals();					/* Unlock this apps Globals */

	return (futp);
}

