/*
 * @(#)makefuts.h	1.13 97/12/22

	Contains:	header file for making ICM specific futs

	Written by:	Poe

	Copyright:	(c) 1991-1997 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

 */


#ifndef _MAKEFUTS_H_
#define _MAKEFUTS_H_ 1

#include <math.h>
#include "fut.h"
#include "fut_util.h"

/* These constants are used in logrgb.c and loguvl.c to shape shadows by 
	adding `flare' */

#define	FLARE_YMIN		0.00392156862745	/* Y_min = 1/255, a la DM8000 */
#define  FLARE_YFLARE	0.00393700787402	/* Y_min/(1 + Y_min) */
#define	FLARE_DMAX		2.40654018043395	/* D_max = -(log Y_min) */
#define	FLARE_TANPT		0.01065992873906	/* eY_min */
#define	FLARE_MAPPT		0.18046425546277	/* (log e)/D_max */
#define	FLARE_SLOPE		16.9292178100213	/* MAPPT/TANPT */

/* This constant was define in the non-ansi c compilers math.h.  It does not
	seem to be defined by any of the compilers that we are using   					*/

#define M_LN10				2.30258509299404568402

	/*
		This scale factor is applied to the X, Y, and Z values prior to
		the 12-bit quantization so that, when shifted left 4 bits,
		it will produce an unsigned 16-bit number with one integer bit,
		as per the ColorSync open profile format.  It is *approximately* 
		1/2:  the exact number is 2^15/(4080 << 4), since 1.0 is represented 
		as 2^15 and FUT_MAX_PEL12 (= 4080) is the quantization scale factor.
	*/
#define	XYZSCALE	0.50196078431373	/* = 2^15 / (FUT_MAX_PEL12 << 4) */

	/* restricts t to interval [low, high] */
#define RESTRICT(t, low, high)	\
			(MAX ((low), MIN ((high), (t))))

	/* quantizes t to nearest (short) integer in given scale */
#define QUANT(t, scale)	\
			((short)((double)(scale) * RESTRICT((t), 0.0, 1.0) + 0.5))

	/* quantizes t to nearest integer in given scale */
	/* seperate the lines to fix floating point precision error on 68k */
#define QUANT1(t, scale)	\
			t = (double)RESTRICT((t), 0.0, 1.0); \
			t *= (double)scale; \
			t += (double)0.5

	/* (approximate) inverse of QUANT() */
#define DEQUANT(t, scale)	\
			((double)(t)/(double)(scale))


#define	POW(x, power)	( ((x) > 0.0) ? exp ((power) * log ((x))) \
			 	      : pow ((x), (power)) )

#define	ANTILOG(x)	( exp ((x) * M_LN10) )

	/* CIE visual-response function */
#define H(t)			\
	( ((t) <= 0.008856) ? 9.033 * (t) \
			    : 1.16 * POW((t), 1.0/3.0) - 0.16 )

	/* inverse of H() */
#define H_inverse(t)		\
	( ((t) <= 0.08) ? (t)/9.033 \
			      : POW(((t) + 0.16)/1.16, 3.0) )


/* function prototypes */
int make_lin3d ARGS((char *fname));
int make_lin4d ARGS((char *fname));
int make_lab2xyz ARGS((char *fname));
int make_xyz2lab ARGS((char *fname));
fut_ptr_t get_lin3d_fut ARGS((int));
fut_ptr_t get_lin4d_fut ARGS((int));
fut_ptr_t get_linlab_fut ARGS((int));
fut_ptr_t get_lab2xyz_fut ARGS((int));
fut_ptr_t get_xyz2lab_fut ARGS((int));

#endif	/* _MAKEFUTS_H_ */


