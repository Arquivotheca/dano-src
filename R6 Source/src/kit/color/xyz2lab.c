/*********************************************************************/
/*
	Contains:	This module contains functions to create an XYZ->lab FuT.

				Created by lsh, November 11, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile$
		$Logfile$
		$Revision$
		$Date$
		$Author$

	SCCS Revision:
		@(#)xyz2lab.c	1.11 12/22/97

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

#include "makefuts.h"

/******
#define	D65	1
******/
#undef	D65

#ifdef	D65				/* white point for CIELAB */
#define	XWHITE	0.950
#define	YWHITE	1.000
#define ZWHITE	1.089
#else	/* D50 */
#define	XWHITE	0.9642
#define	YWHITE	1.0000
#define	ZWHITE	0.8249
#endif

#define	AB_UNSIGNED	1		/* use unsigned rep for a* and b* */

static int32	channel = 0, xsize, ysize, zsize;


/*---------------------------------------------------------------------
 *  xfun, yfun, zfun -- input mappings
 *---------------------------------------------------------------------
 */
static double xfun (double x)
{
	x /= (XYZSCALE * XWHITE);
	x = H (x);
	x *= (double)(xsize - 2) / (double)(xsize - 1);
	return RESTRICT (x, 0.0, 1.0);
}

static double yfun (double y)
{
	y /= (XYZSCALE * YWHITE);
	y = H (y);
	y *= (double)(ysize - 2) / (double)(ysize - 1);
	return RESTRICT (y, 0.0, 1.0);
}

static double zfun (double z)
{
	z /= (XYZSCALE * ZWHITE);
	z = H (z);
	z *= (double)(zsize - 2) / (double)(zsize - 1);
	return RESTRICT (z, 0.0, 1.0);
}

/*---------------------------------------------------------------------
 *  trifun -- grid-table functions:
 *				(H(X/X_n), H(Y/Y_n), H(Z/Z_n)) --> (L*, a*, b*) 
 *---------------------------------------------------------------------
 */
static double trifun (double x, double y, double z)
{
	double	p = 0;
	double	tempd[3];

/* Convert to CIELAB, with L* in [0, 100], a* & b* in [-200, 200]:  */
	switch (channel) {
    case 1:	/* (L*)/100 */
		tempd[0] = (double)(ysize - 1);
		tempd[1] = (double)(ysize - 2);
		tempd[2] = tempd[0] / tempd[1];
		tempd[2] *= y ;
		p = tempd[2];
		break;

   case 2:	/* (2048/4095)[1 + (a*)/200] */
		x *= (double)(xsize - 1) / (double)(xsize - 2);
		y *= (double)(ysize - 1) / (double)(ysize - 2);
		p = 0.50012210012210 * (1.0 + 2.15517241379310 * (x - y));
		break;

   case 3:	/* (2048/4095)[1 + (b*)/200] */
		y *= (double)(ysize - 1) / (double)(ysize - 2);
		z *= (double)(zsize - 1) / (double)(zsize - 2);
		p = 0.50012210012210 * (1.0 + 0.86206896551724 * (y - z));
		break;

   default:
   		p = 6.023e+23;	/* Avogadro's number */
		break;
	}
	return RESTRICT (p, 0.0, 1.0);	
}

/*----------------------------------------------------------------------------
 *  outfun -- rescale and clip a* and b* and convert to signed representation
 *----------------------------------------------------------------------------
 */
static double outfun (double p)
{
	switch (channel) {
	case 1:	break;
	case 2:
	case 3:
		p *= 1.99951171875;
		p = 200.0 * (p - 1.0);	/* CIE 1976 a*, b*, in [-200, 200] */
		p = RESTRICT (p, -128.0, 127.0);	/* clip to [-128, 127] */
#ifdef AB_UNSIGNED
		p = p + 128.0;		/* -> [0, 255] */
#else
		if (p <= -1.0)		/* in [-128, -1] */
		   p += 256.0;		/* -> [128, 255] */
		else if (p < -0.5)	/* round to -1 */
		   p = 255.0;
		else if (p < 0.0)	/* round to 0 */
		   p = 0.0;
		else			/* in [0, 127] */
		   ;			/* leave as is */
#endif
		p /= 255.0;		/* from [0, 255] to [0, 1] */
		break;

   default:
		p = 6.023e+23;
		break;
	}
	return RESTRICT (p, 0.0, 1.0);	/* superfluous, but whatthehell */
			/* L* in [0, 100]; a* & b* in [-128, 127] */
}

/*------------------------------------------------------------------
 *  gfun, ofun -- quantization call-back functions for FuT library
 *------------------------------------------------------------------
 */
static fut_gtbldat_t gfun (double *args)
{

double	g, x, y, z;

	x = args[0];
	y = args[1];
	z = args[2];

	g = trifun (x, y, z);
	g = QUANT1 (g, FUT_GRD_MAXVAL);
	
	return ((fut_gtbldat_t)g);
}

static fut_otbldat_t ofun (fut_gtbldat_t q)
{
	double	s;

	s = DEQUANT (q, FUT_GRD_MAXVAL);
	s = outfun (s);
	s =  QUANT1 (s, FUT_MAX_PEL12);
	
	return ((fut_otbldat_t)s);

}

/*----------------------------------------------------------------------
 *  get_xyz2lab_fut --	construct an XYZ to Lab FuT
 *						to establish a grid for composition
 *----------------------------------------------------------------------
 */
fut_ptr_t
	get_xyz2lab_fut (int size)
{
fut_ptr_t		futp;
fut_itbl_ptr_t	itblx, itbly, itblz;
fut_gtbl_ptr_t	gtbl [3];
fut_otbl_ptr_t	otbl [3];
int32		ichan;
int32		iomask, sizeArray[3];

	/* assume all dimensions are the same */
	xsize = ysize = zsize = size;
	
/* Compute shared input tables:  */
	itblx = fut_new_itbl (xsize, xfun);
	itbly = fut_new_itbl (ysize, yfun);
	itblz = fut_new_itbl (zsize, zfun);

/* Compute grid tables and output tables:  */
	iomask = fut_iomask ("(xyz)");

	sizeArray[0] = xsize;
	sizeArray[1] = ysize;
	sizeArray[2] = zsize;

	for (ichan = 0; ichan < 3; ichan++) {
	   channel = ichan + 1;
	   gtbl [(int) ichan] = fut_new_gtblA (iomask,
										   (fut_gtbldat_t (*)(double *)) gfun,
										   sizeArray);
	   otbl [(int) ichan] = fut_new_otbl (ofun);
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

	fut_free_tbls ((int32 fut_far *)itblx, itbly, itblz, 
							gtbl [0], gtbl [1], gtbl [2], 
							otbl [0], otbl [1], otbl [2], NULL);

	return (futp);
}

