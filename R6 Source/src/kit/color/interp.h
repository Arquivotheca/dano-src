/*
 * SCCSID = "@(#)interp.h	13.7 03/12/97"
 *
 * interpolation macros for fut library
 * Used by fut_lin, fut_bilin, fut_trilin, fut_cub, etc.
 */

#ifndef FUT_INTERP_HDR
#define FUT_INTERP_HDR
                                        /* linear_interpolate round-off */
#define NO_ROUNDING	0

#if NO_ROUNDING
#define ROUND_UP	0
#else
#define	ROUND_UP        ( ((int32)1) << ( FUT_INP_FRACBITS - 1 ) )
#endif

                                        /* mask to extract fractional bits of
                                           x, y, or z input values */

#define FRAC_MASK       ( ( ((int32)1) << FUT_INP_FRACBITS ) - 1 )

                                        /* shift to extract fractional bits of
                                           x, y, or z input values */

#define FRAC_SHIFT      (int32)( FUT_INP_DECIMAL_PT - FUT_INP_FRACBITS )

                                        /* 1/2 of a whole in fixed point notation
                                           of x, y, or z input values */

#define FRAC_HALF       ( ((int32)1) << ( FUT_INP_DECIMAL_PT - 1 ) )

                                        /* shift to extract whole part of
                                           x, y, or z input values */

#define WHOLE_SHIFT     ( FUT_INP_DECIMAL_PT )

                                        /* get the short integer pointed to
                                           by the grid table pointer gp and
                                           mask off any trash bits */

#define GRID_VAL(gp) (*((fut_gtbldat_ptr_t) ((fut_generic_ptr_t) gp)))

#define GDATSHIFT 1	/* assumes sizeof(fut_gtbldat_t) == 2 */
#define GDATSIZE (1<<GDATSHIFT)

                                        /* linearly interpolate between
                                           integer values f1 and f2, using
                                           frac as the interpolant */
#define	LINEAR_INTERPOLATE(f1,f2,frac) \
        ((f1) + ((((f2) - (f1)) * (frac) + ROUND_UP) >> FUT_INP_FRACBITS))

					/* linearly interpolate an input table
					   entry between itbl[i] and itbl[i+1]
					   using f as the interpolant, where
					   i = integer part of 12-bit x and
					   f = fractional part.  t and p are
					   (int32) and (int32 *) scratch variables */
#define FUT_ITBL_INTERP(itbl,x,t,p) \
			( p = &itbl[FUT_OTBL_INTEG(x)], \
			  t = *p++, \
			  ((int32)((*p - t)*FUT_OTBL_FRAC(x) + FUT_OTBL_ROUNDUP) \
					>> FUT_OUT_FRACBITS) + t )

/* Arg structure for passing grid table, dimensions, and x,y,z inputs to
 * the interpolators (i.e. fut_lin, fut_bilin, fut_trilin, fut_quadlin,
 * fut_cub, etc.)
 * BEWARE of modifying this if assembly language interpolators are in use!!
 */
typedef struct shortword_s {
#if FUT_MSBF == 0xF
	unsigned short H, L;
#else
	unsigned short L, H;
#endif
} shortword_t;

typedef union longword_s {
	int32 		HL;
	shortword_t	shortword;
} longword_t;

typedef struct fut_interp_s {
	fut_gtbldat_ptr_t	grid;		/* ^ to grid table */
	int32		nt, nz, ny, nx;		/* grid dimensions */
	int32		st, sz, sy, sx;		/* grid strides */
	int32		gt, gz, gy, gx;		/* grid coordinates */
} fut_interp_t;


/*--------------------------------------------- point interpolators */
extern int32	fut_lin     ARGS((fut_interp_t*));
extern int32	fut_bilin   ARGS((fut_interp_t*));
extern int32	fut_trilin  ARGS((fut_interp_t*));
extern int32	fut_quadlin ARGS((fut_interp_t*));

extern int32	fut_cub     ARGS((fut_interp_t*)); /* cubic (1D) */
extern int32	fut_bicub   ARGS((fut_interp_t*)); /* bicubic (2D) */
extern int32	fut_tricub  ARGS((fut_interp_t*)); /* tricubic (3D) */
extern int32	fut_quadcub ARGS((fut_interp_t*)); /* quadcubic (4D) */

extern int32	fut_nn      ARGS((fut_interp_t*));
extern int32	fut_binn    ARGS((fut_interp_t*));
extern int32	fut_trinn   ARGS((fut_interp_t*));
extern int32	fut_quadnn  ARGS((fut_interp_t*));


#include "fut_pcc.h"	/* convolution kernel and definitions */

#if FUT_INP_FRACBITS < PCC_SRBITS
"ERROR: FUT_INP_FRACBITS must be greater than or equal to PCC_SRBITS!!!"
#endif

#if NO_ROUNDING
#define PCC_ROUND_UP	0
#else
#define PCC_ROUND_UP	(1 << ( PCC_FRACBITS - 1 ) )
#endif
                                        /* shift to extract fractional bits of
                                           x, y, or z input values for indexing
					   into PCC array */

#define PCC_FRAC_SHIFT	( FUT_INP_DECIMAL_PT - PCC_SRBITS )

                                        /* mask to extract fractional bits of
                                           x, y, or z input values for indexing
					   into PCC array */

#define PCC_FRAC_MASK	( (1 << PCC_SRBITS) - 1 )

					/* addresses of PCC function nodes: */
#define PCC0	(&fut_pcc_kernel[0])		/* node 0 */
#define PCC1	(&fut_pcc_kernel[PCC_SR])	/* node 1 */
#define PCC2	(&fut_pcc_kernel[2*PCC_SR])	/* node 2 */

			/* interpolate between integer values f0,f1,f2,f3
			   using 4-point (cubic convolution) interpolation,
			   using frac as index into precomputed array */

#define CUBIC_INTERPOLATE(f0,f1,f2,f3,frac) ( \
	(  ((f0)*(*(PCC1+(frac))) + (f1)*(*(PCC0+(frac))) \
	  + (f2)*(*(PCC1-(frac))) + (f3)*(*(PCC2-(frac)))) + PCC_ROUND_UP ) \
	>> PCC_FRACBITS )

/*
 * GET_GRID_VALS gets 4 grid values from a grid, checking boundary
 * conditions and computing suitable replacements if a grid point does not
 * exist.  Cell points to the first of the middle 2, size is the size of the
 * grid in this dimension (* sizeof(fut_gtbldat_t)), and x is the grid table
 * coordinate at which to interpolate (only used to check boundary
 * conditions). t0,t1,t2,t3 should be ints, and cell should be a
 * caddr_t.  The 4 grid values are returned in t0,t1,t2,t3, and
 * may be passed to CUBIC_INTERPOLATE along with frac to complete the
 * 4-point cubic interpolation. x, cell, and size are left unchanged and
 * if possible, size should be a regoster int since it is used twice. 
 */

#define GET_GRID_VALS(t0,t1,t2,t3, x,cell,size) {			\
	t1 = GRID_VAL ((cell));		/* get middle 2 grid values */	\
	t2 = GRID_VAL ((cell) + sizeof(fut_gtbldat_t));			\
	t0 = (int32)(x) >> WHOLE_SHIFT;	/* get whole part of x into reg */ \
				/* now get outer 2 grid values */	\
	if ( size == 2 ) {		/* grid size == 2 */		\
		t0 = 2*t1 - t2;						\
		t3 = 2*t2 - t1;						\
	} else if ( t0 == 0 ) {		/* at left grid edge */		\
		t3 = GRID_VAL (cell + 2*sizeof(fut_gtbldat_t));		\
		t0 = 3*(t1-t2) + t3;					\
	} else if ( (t0+=2) == size ) {	/* at right grid edge */	\
		t0 = GRID_VAL (cell - sizeof(fut_gtbldat_t));		\
		t3 = 3*(t2-t1) + t0;					\
	} else {			/* away from grid edges */	\
		t0 = GRID_VAL (cell - sizeof(fut_gtbldat_t));		\
		t3 = GRID_VAL (cell + 2*sizeof(fut_gtbldat_t));		\
	}								\
}

					/* clip result to valid range */

#define CLIP_RESULT(t)	(((unsigned)t <= FUT_GRD_MAXVAL) ? t :  \
				((t < 0) ? 0 : FUT_GRD_MAXVAL))

/*
 * Functions to perform the various types of interpolation.  The notation
 * is fut_interp_<order><ndim>d<data_type> where,
 *	<order> is interpolation order; either 'nn', 'lin', or 'cub',
 *	<ndim> is dimensionality; either '0', '1', '2', or '3',
 *	<data_type> is type of data on which is operated; either '8' (bit),
 *			'12' (bit), or '0' (which is 12 bit with 4K input
 *			tables - instead of the usual 256 element tables - 
 *			currently used only for fut composition).
 */
		/* N.N. interpolation of a bytes */
extern int	fut_interp_nn1d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bi-N.N. interpolation of a byte array */
extern int	fut_interp_nn2d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* tri-N.N. interpolation of a byte array */
extern int	fut_interp_nn3d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quad-N.N. interpolation of a byte array */
extern int	fut_interp_nn4d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* N.N. interpolation of a short array */
extern int	fut_interp_nn1d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bil-N.N. interpolation of a short array */
extern int	fut_interp_nn2d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* tri-N.N. interpolation of a short array */
extern int	fut_interp_nn3d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quad-N.N. interpolation of a short array */
extern int	fut_interp_nn4d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* N.N. interpolation of a short array (composition)*/
extern int	fut_interp_nn1d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bi-N.N. interpolation of a short array (composition)*/
extern int	fut_interp_nn2d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* tri-N.N interpolation of a short array (composition)*/
extern int	fut_interp_nn3d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quad-N.N interpolation of a short array (composition)*/
extern int	fut_interp_nn4d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* linear interpolation of a byte array */
extern int	fut_interp_lin1d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bilinear interpolation of a byte array */
extern int	fut_interp_lin2d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* trilinear interpolation of a byte array */
extern int	fut_interp_lin3d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t, 
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quadlinear interpolation of a byte array */
extern int	fut_interp_lin4d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t, 
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* linear interpolation of a short array */
extern int	fut_interp_lin1d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
/* bilinear interpolation of a short array */
extern int	fut_interp_lin2d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* trilinear interpolation of a short array */
extern int	fut_interp_lin3d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quadlinear interpolation of a short array */
extern int	fut_interp_lin4d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* linear interpolation of a short array (composition)*/
extern int	fut_interp_lin1d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bilinear interpolation of a short array (composition)*/
extern int	fut_interp_lin2d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* trilinear interpolation of a short array (composition)*/
extern int	fut_interp_lin3d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quadlinear interpolation of a short array (composition)*/
extern int	fut_interp_lin4d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* cubic interpolation of a byte array */
extern int	fut_interp_cub1d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bicubic interpolation of a byte array */
extern int	fut_interp_cub2d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* tricubic interpolation of a byte array */
extern int	fut_interp_cub3d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quadcubic interpolation of a byte array */
extern int	fut_interp_cub4d8 ARGS((
			generic_u_int8_ptr_t,
			generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,generic_u_int8_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* cubic interpolation of a short array */
extern int	fut_interp_cub1d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bicubic interpolation of a short array */
extern int	fut_interp_cub2d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* tricubic interpolation of a short array */
extern int	fut_interp_cub3d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quadcubic interpolation of a short array */
extern int	fut_interp_cub4d12 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* cubic interpolation of a short array */
extern int	fut_interp_cub1d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* bicubic interpolation of a short array */
extern int	fut_interp_cub2d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
	/* tricubic interpolation of a short array */
extern int	fut_interp_cub3d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
	/* quadcubic interpolation of a short array */
extern int	fut_interp_cub4d0 ARGS((
			generic_int16_ptr_t,
			generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,generic_int16_ptr_t,
			int32,
			fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));

	/* constant replication (outputs bytes) */
extern int	fut_interp_0d8 ARGS((
			generic_u_int8_ptr_t,
			int32,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* constant replication (outputs shorts) */
extern int	fut_interp_0d12 ARGS((
			generic_int16_ptr_t,
			int32,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
	/* constant replication (outputs shorts) */
extern int	fut_interp_0d0 ARGS((
			generic_int16_ptr_t,
			int32,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
#endif

