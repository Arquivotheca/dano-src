/*
 * @(#)lab2xyz.c	1.10 97/12/22
	Contains:	This module contains functions to create an lab->XYZ FuT.

				Created by lsh, November 11, 1993
				Modified

	Written by:	The Kodak CMS Engineering Team
 */
/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1994                 ***
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

static int32	gChannel = 0;
static int		gXsize = 0, gYsize = 0, gZsize = 0;

/*---------------------------------------------------------------------
 *  ubyte, xfun, yfun, zfun -- input mappings:  Convert a* and b* to unsigned
 *---------------------------------------------------------------------
 */
static double ubyte (double byte)
{
	return byte;			/* already unsigned */
}

static double
	xfun (double x)
{
	return x;
}

static double
	yfun (double y)
{
	y = 255.0 * ubyte (y);

	if (y <= 128.0) {
		y = ((double)(gYsize / 2) / (double)(gYsize - 1)) * (y / 128.0);
	}
	else {
		y = 1.0 - ((double)(gYsize - 1 - (gYsize / 2)) / (double)(gYsize - 1)) 
			* ((255.0 - y) / 127.0);
	}

	return y;
}

static double
	zfun (double z)
{
	z = 255.0 * ubyte (z);

	if (z <= 128.0) {
		z = ((double)(gZsize / 2) / (double)(gZsize - 1)) * (z / 128.0);
	}
	else {
		z = 1.0 - ((double)(gZsize - 1 - (gZsize / 2)) / (double)(gZsize - 1)) 
			* ((255.0 - z) / 127.0);
	}
	
	return z;
}


/*---------------------------------------------------------------------
 *  trifun -- grid-table functions:  Lab -> XYZ
 *---------------------------------------------------------------------
 */
/* representing L* in [0, 100], a* and b* in [-128, 127] */
static double trifun (double l, double a, double b)
{
	double	tristim = 0;

/* Undo grid mapping of a* and b*:  */
	if (a <= (double)(gYsize / 2) / (double)(gYsize - 1)) {
		a = (128.0 / 255.0) * ((double)(gYsize - 1) / (double)(gYsize / 2)) * a;
	}
	else {
		a = 1.0 - (127.0 / 255.0) * ((double)(gYsize - 1) / (double)(gYsize - 1 - (gYsize / 2))) 
			* (1.0 - a);
	}
	
	if (b <= (double)(gZsize / 2) / (double)(gZsize - 1)) {
		b = (128.0 / 255.0) * ((double)(gZsize - 1) / (double)(gZsize / 2)) * b;
	}
	else {
		b = 1.0 - (127.0 / 255.0) * ((double)(gZsize - 1) / (double)(gZsize - 1 - (gZsize / 2))) 
			* (1.0 - b);
	}

/* Shift and rescale a* and b*:  */
	a = 255.0 * a - 128.0;		/* CIE 1976 a* */
	a = 0.00232 * a;		/* H(X/X_n) - H(Y/Y_n), in [-0.297, 0.295] */
	b = 255.0 * b - 128.0;		/* CIE 1976 b* */
	b = 0.00580 * b;		/* H(Y/Y_n) - H(Z/Z_n), in [-0.742, 0.737] */

/* Separate XYZ channels:  */
	switch (gChannel) {
	case 1:
		tristim = a + l;	/* H(X/X_n), in [-0.297, 1.295 */
		break;

	case 2:
		tristim = l;		/* H(Y/Y_n), in [0, 1] */
		break;

	case 3:
		tristim = l - b;	/* H(Z/Z_n), in [-0.742, 1.737] */
		break;
	}

/* Rescale & return:  */
	tristim = (tristim + 1.0) / 3.0;	/* in [0.086, 0.9123] */
	return RESTRICT (tristim, 0.0, 1.0);
}

static double outfun (double p)
{
	p = (3.0 * p) - 1.0;		/* [0, 1] -> [-1, 2] */
	p = RESTRICT (p, 0.0, 2.0);	/* leave headroom */
	p = H_inverse (p);		/* X/X_n, Y/Y_n, Z/Z_n */

	switch (gChannel) {
	case 1:
		p *= XWHITE;	/* X */
		break;

	case 2:
		p *= YWHITE;	/* Y */
		break;

	case 3:
		p *= ZWHITE;	/* Z */
		break;
	}

     /* Adjust tristimulus values for encoding:  */
	p *= XYZSCALE;			/* rescale for InterColor encoding */
	return RESTRICT (p, 0.0, 1.0);	/* clip to valid range [0, 1] */
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
 *  get_lab2xyz_fut --	construct an Lab to XYZ FuT FuT
 *						to establish a grid for composition
 *----------------------------------------------------------------------
 */
fut_ptr_t
	get_lab2xyz_fut (int size)
{
fut_ptr_t		futp;
fut_itbl_ptr_t	itblx, itbly, itblz;
fut_gtbl_ptr_t	gtbl [3];
fut_otbl_ptr_t	otbl [3];
int32		ichan;
int32		iomask, sizeArray[3];

	/* assume all dimensions are the same */
	gXsize = gYsize = gZsize = size;
	
/* Compute shared input tables:  */
	itblx = fut_new_itbl (gXsize, xfun);
	itbly = fut_new_itbl (gYsize, yfun);
	itblz = fut_new_itbl (gZsize, zfun);

/* Compute grid tables and output tables:  */
	iomask = fut_iomask ("(xyz)");

	sizeArray[0] = gXsize;
	sizeArray[1] = gYsize;
	sizeArray[2] = gZsize;

	for (ichan = 0; ichan < 3; ichan++) {
	   gChannel = ichan + 1;
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

	fut_free_tbls ((int32 FAR *)itblx, itbly, itblz, 
							gtbl [0], gtbl [1], gtbl [2], 
							otbl [0], otbl [1], otbl [2], NULL);

	return (futp);
}

