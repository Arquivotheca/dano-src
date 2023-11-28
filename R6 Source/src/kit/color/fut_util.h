/* fut_util.h	"@(#)fut_util.h	13.14 10/30/95"

 Fut (function table) internal interface file.  These definitions
 and macros are used only *internally* by the library and are
 considered private to it.  The header file is therefore not
 installed along with fut.h.  If something here needs to become
 public, please move it to fut.h rather than make this entire
 file public.

 PROPRIETARY NOTICE: The  software information contained
 herein is the sole property of Eastman Kodak Company and is
 provided to Eastman Kodak Company users under license for use on their
 designated equipment only.  Reproduction of this matter in
 whole or in part is forbidden without the express written
 consent of Eastman Kodak Company.

 COPYRIGHT (c) 1989-1994 Eastman Kodak Company.
 As  an  unpublished  work pursuant to Title 17 of the United
 States Code.  All rights reserved.
 */


#ifndef FUT_UTIL_HEADER
#define FUT_UTIL_HEADER

#include "fut.h"
#include "bytes.h"

/* define NULL if necessary. */
#ifndef NULL
#define NULL	0
#endif

/* itbl mode select */
#define ITBLMODE8BIT	1
#define ITBLMODE12BIT	2
#define ITBLMODE16BIT	3

#define CACHE_SIZE	(FUT_NICHAN*FUT_NOCHAN)
/* The abslolute minimum cache size is FUT_NICHAN since
 * this many tables may be required to evaluate one
 * channel of a fut.  However, FUT_NICHAN*FUT_NOCHAN is
 * a good number when a fut has heterogeneous grid
 * tables since there may be this many distinct
 * input-output table combinations during one fut composition
 */

typedef struct iotbl_cache_s {
	fut_itbldat_ptr_t	iotbl;		/* ^ iotbl */
	int32			itbl_id;	/* input table id number */
	int32			otbl_id;	/* output table id number */
	int			lru_count;	/* least recent use count */
}	iotbl_cache_t, FAR* iotbl_cache_ptr_t;

/* functions referenced only internally by the library: */
#if defined(__cplusplus) || defined(SABR)
extern "C" {
#endif

#if !defined(KPNONANSIC)
void				iotblInitC ARGS((iotbl_cache_ptr_t));
void				iotblFlushC ARGS((iotbl_cache_ptr_t));
fut_itbldat_ptr_t	fut_comp_iotblC ARGS((fut_itbl_ptr_t,fut_otbl_ptr_t, iotbl_cache_ptr_t));
fut_gtbl_ptr_t		fut_expand_gtbl ARGS((fut_gtbl_ptr_t,int16 fut_far*));
KpInt32_t fut_getItblFlag ARGS((fut_ptr_t fut, KpUInt32_p theFlag));
fut_itbldat_ptr_t	fut_expand8to12_itbl ARGS((fut_itbl_ptr_t itbl));

fut_itbl_ptr_t	fut_itbl_gcoords ARGS((int,int fut_far*));
int		fut_gtbl_imask ARGS((fut_gtbl_ptr_t));
int32	fut_iomask_check ARGS((fut_ptr_t, int32));
int32	fut_required_inputs ARGS((fut_ptr_t, int32));
int		fut_exchange ARGS((fut_ptr_t, fut_ptr_t));
int		fut_reset_iomask ARGS((fut_ptr_t));
int		fut_first_chan ARGS((int));
int		fut_last_chan ARGS((int));
int32	fut_mfutInfo (fut_ptr_t, int32_p, int32_p, int32_p);
int32	fut_iomask ARGS((char*));
int32	fut_iomask_to_int ARGS((fut_iomask_t));
fut_iomask_t	fut_int_to_iomask ARGS((int32));
int 	fmt_analyze ARGS((fut_flat_far_ptr_t fut_far*,
					int32_p, fut_flat_far_ptr_t fut_far*, int32 iomask));

int		fut_unique_id ARGS((void));

fut_ptr_t	fut_alloc_fut ARGS((void));
fut_chan_ptr_t	fut_alloc_chan ARGS((void));
fut_itbl_ptr_t	fut_alloc_itbl ARGS((void));
fut_otbl_ptr_t	fut_alloc_otbl ARGS((void));
fut_gtbl_ptr_t	fut_alloc_gtbl ARGS((void));

fut_itbldat_ptr_t	fut_alloc_itbldat ARGS((KpUInt32_t mode));
fut_otbldat_ptr_t	fut_alloc_otbldat ARGS((void));
fut_gtbldat_ptr_t	fut_alloc_gtbldat ARGS((int32));

char fut_far	*fut_alloc_idstr ARGS((int32));
void		fut_free_idstr ARGS((char fut_far*));
fut_generic_ptr_t	fut_malloc		ARGS((int32, char_p, ...));
void		 		fut_mfree 		ARGS((fut_generic_ptr_t, char_p));

#endif	/* end !defined nonansi c */

/* macros for itbl element size and dimensionality */
#define ITBL_ELEMENT_SHIFT 16
#define ITBL_DIM_SIZE(x) (x & (( 1 << ITBL_ELEMENT_SHIFT) -1))
#define ITBL_ELEMENT_SIZE(x) (x >> ITBL_ELEMENT_SHIFT)

/* Quick, inline free commands only call free if table is not shared.
 * Also, does not check for table's magic number. */
#define fut_qfree_itbl(itbl)	{					\
		if ( (itbl) !=0 )					\
			if ( (itbl)->ref == 0 ) fut_free_itbl (itbl);	\
			else if ( (itbl)->ref > 0 ) (itbl)->ref--;	\
	}
#define fut_qfree_otbl(otbl)	{					\
		if ( (otbl) !=0 )					\
			if ( (otbl)->ref == 0 ) fut_free_otbl (otbl);	\
			else if ( (otbl)->ref > 0 ) (otbl)->ref--;	\
	}
#define fut_qfree_gtbl(gtbl)	{					\
		if ( (gtbl) !=0 )					\
			if ( (gtbl)->ref == 0 ) fut_free_gtbl (gtbl);	\
			else if ( (gtbl)->ref > 0 ) (gtbl)->ref--;	\
	}


/* Magic numbers for runtime checking */
#define FUT_MAGIC	0x66757466	/* = "futf", (char)FUT_MAGIC == 'f' */
#define FUT_CMAGIC	0x66757463	/* = "futc", (char)FUT_CMAGIC == 'c' */
#define FUT_IMAGIC	0x66757469	/* = "futi", (char)FUT_IMAGIC == 'i' */
#define FUT_OMAGIC	0x6675746f	/* = "futo", (char)FUT_OMAGIC == 'o' */
#define FUT_GMAGIC	0x66757467	/* = "futg", (char)FUT_GMAGIC == 'g' */

#define FUT_CIGAM	0x66747566	/* = "ftuf", byte-swapped for file I/O */
#define FUT_CIGAMI	0x69747566	/* = "ituf", byte-swapped for file I/O */
#define FUT_CIGAMO	0x6f747566	/* = "otuf", byte-swapped for file I/O */
#define FUT_CIGAMG	0x67747566	/* = "gtuf", byte-swapped for file I/O */

#if defined(__cplusplus) || defined (SABR)
}
#endif

/* Quick in-line checks on validity of futs, chans, itbls, otbls and gtbls. */
#define IS_FUT(x)	((x) != 0 && (x)->magic == FUT_MAGIC)
#define IS_CHAN(x)	((x) != 0 && (x)->magic == FUT_CMAGIC)
#define IS_ITBL(x)	((x) != 0 && (x)->magic == FUT_IMAGIC)
#define IS_OTBL(x)	((x) != 0 && (x)->magic == FUT_OMAGIC)
#define IS_GTBL(x)	((x) != 0 && (x)->magic == FUT_GMAGIC)

/* IS_SHARED() checks if an input, output, or grid table is referenced
 * by anyone else. */
#define IS_SHARED(tbl)	((tbl) != NULL && (tbl)->ref != 0)

/* Sometimes it is inconvenient to repeatedly test for a null channel before
 * trying to access one of its members.  In many cases, a null channel may be
 * assumed to have a null imask and null tables.
 * FUT_CHAN_IMASK() conveniently returns a null imask for a null channel.
 * FUT_CHAN_[IGO]TBL() conveniently returns a null [igo]tbl for a null channel. */
#define FUT_CHAN_IMASK(chan)	((chan==FUT_NULL_CHAN) ? (int32)0 : chan->imask)
#define FUT_CHAN_GTBL(chan)	((chan==FUT_NULL_CHAN) ? FUT_NULL_GTBL : chan->gtbl)
#define FUT_CHAN_OTBL(chan)	((chan==FUT_NULL_CHAN) ? FUT_NULL_OTBL : chan->otbl)
#define FUT_CHAN_ITBL(chan,i)	((chan==FUT_NULL_CHAN) ? FUT_NULL_ITBL : chan->itbl[i])

 /* FUT_DOUBLE_EVEN rounds a number up to a double even value.
 * This is used to pad the idstring (with newlines) so that the
 * tables in binary files are more human readable. */
#define FUT_DOUBLE_EVEN(x)	(((x)+3) & ~3)

/* format types for evaluation */
#define FMT_GENERAL     		(0)
#define FMT_COMPATIBLE  		(1)
#define FMT_QD					(2)
#define FMT_GENERAL_12BIT		(3)
#define FMT_BIGENDIAN24			(4)
#define FMT_LITTLEENDIAN24		(5)
#define FMT_BIGENDIAN32			(6)
#define FMT_LITTLEENDIAN32		(7)

/* diagnotstics (error messages) will be normally printed to stderr
 * unless QUIET is defined */
#define QUIET

#ifndef QUIET
#include <stdio.h>
#define DIAG(s,x)	(void)fprintf(stderr,s,x)
#else
#define DIAG(s,x)
#endif /* QUIET */

/* define data type specifying a pointer to function returning an int.
 * fut_interp_fun() is a function which returns a pointer to the proper
 * interpolator function given the data type, interpolation order, and
 * input dimension. */
#if defined(KPNONANSIC)
typedef int	(*fut_pfi_t0) ();
typedef int	(*fut_pfi_t1) ();
typedef int	(*fut_pfi_t2) ();
typedef int	(*fut_pfi_t3) ();
typedef int	(*fut_pfi_t4) ();
#else
typedef int	(*fut_pfi_t0) ARGS((
			fut_generic_ptr_t,
			int32,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
fut_pfi_t0	fut_interp_fun0 ARGS((int dattype, int order));

typedef int	(*fut_pfi_t1) ARGS((
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			fut_otbldat_ptr_t
			));
fut_pfi_t1	fut_interp_fun1 ARGS((int dattype, int order));

typedef int	(*fut_pfi_t2) ARGS((
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_itbldat_ptr_t,
			fut_gtbldat_ptr_t,
			int32,
			fut_otbldat_ptr_t
			));
fut_pfi_t2	fut_interp_fun2 ARGS((int dattype, int order));

typedef int	(*fut_pfi_t3) ARGS((
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_itbldat_ptr_t,
			fut_itbldat_ptr_t, 
			fut_gtbldat_ptr_t,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
fut_pfi_t3	fut_interp_fun3 ARGS((int dattype, int order));

typedef int	(*fut_pfi_t4) ARGS((
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			fut_generic_ptr_t,
			int32,
			fut_itbldat_ptr_t,
			fut_itbldat_ptr_t,
			fut_itbldat_ptr_t,
			fut_itbldat_ptr_t, 
			fut_gtbldat_ptr_t,
			int32,
			int32,
			int32,
			fut_otbldat_ptr_t
			));
fut_pfi_t4	fut_interp_fun4 ARGS((int dattype, int order));
#endif

#endif /* FUT_UTIL_HEADER */

