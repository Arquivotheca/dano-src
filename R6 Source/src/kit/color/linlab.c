/*********************************************************************/
/*
	Contains:	This module contains functions to create the LinearLab FuT.

				Created by msm, November 7, 1997

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1997 by Eastman Kodak Company, all rights reserved.


	SCCS Revision:
		@(#)linlab.c	1.2	11/10/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** PROPRIETARY NOTICE:     The  software  information   contained ***
 *** herein is the  sole property of  Eastman Kodak Company  and is ***
 *** provided to Eastman Kodak users under license for use on their ***
 *** designated  equipment  only.  Reproduction of  this matter  in ***
 *** whole  or in part  is forbidden  without the  express  written ***
 *** consent of Eastman Kodak Company.                              ***
 ***                                                                ***
 *** COPYRIGHT (c) Eastman Kodak Company, 1993                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "kcptmgr.h"
#include "makefuts.h"

/*---------------------------------------------------------------------
 *  Defaults and other definitions
 *---------------------------------------------------------------------
 */
#define	NEUTRALBYTE	0.50196078431373	/* 128/255; a* = b* = 0.0 */

/*---------------------------------------------------------------------
 *  Global variables (private to this file)
 *---------------------------------------------------------------------
 */
static double	neutralgrid;
static int	channel;

/*---------------------------------------------------------------------
 *  xfun, yfun, zfun -- piecewise-linear input mappings, with gridpoint at
 *	neutral (a* = b* = 0; 8-bit encoding = 128)
 *---------------------------------------------------------------------
 */
static double xfunc (double x)
{
	return x;
}

static double yfunc (double y)
{
	double	delta;

	delta = y - NEUTRALBYTE;
	if (delta < 0.0)
	   y = neutralgrid * (y / NEUTRALBYTE);
	else
	   y = 1.0 - (1.0 - neutralgrid) * ((1.0 - y) / (1.0 - NEUTRALBYTE));
	return RESTRICT (y, 0.0, 1.0);
}

static double zfunc (double z)
{
	double	delta;

	delta = z - NEUTRALBYTE;
	if (delta < 0.0)
	   z = neutralgrid * (z / NEUTRALBYTE);
	else
	   z = 1.0 - (1.0 - neutralgrid) * ((1.0 - z) / (1.0 - NEUTRALBYTE));
	return RESTRICT (z, 0.0, 1.0);

}

/*---------------------------------------------------------------------
 *  trifun -- grid-table functions:  L*a*b* -> L*a*b* (identity)
 *---------------------------------------------------------------------
 */
static double trifunc (double l, double a, double b)
/* representing L* in [0, 100], a* and b* in [-128, 127] */
{
	double	p = 0;

     /* Compute grid-table entry:  */
	switch (channel)
	{
	   case 1:
			p = l;
			break;

	   case 2:
			p = a;
			break;

	   case 3:
			p = b;
			break;
	}

	return p;
}

/*---------------------------------------------------------------------
 *  outfun -- piecewise-linear output-table mappings; inverse of xfun, yfun, zfun
 *---------------------------------------------------------------------
 */
static double outfunc (double p)
{
	double	delta;

	switch (channel)
	{
	   case 1:   /* L* */
			break;

	   case 2:   /* a */
	   case 3:   /* b */
			delta = p - neutralgrid;
			if (delta < 0.0)
			   p = NEUTRALBYTE * (p / neutralgrid);
			else
			   p = 1.0 - (1.0 - NEUTRALBYTE) * ((1.0 - p) / (1.0 - neutralgrid));
			break;
	}
	return p;
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
 *  get_linlab_fut --	construct an LinearLab FuT
 *						to establish a grid for composition
 *----------------------------------------------------------------------
 */
fut_ptr_t
	get_linlab_fut (int size)
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

/*  set_size -- set grid-table dimensions */
	neutralgrid = (double)(size / 2) / (double)(size - 1);

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


