/*
  File:		fut.h		@(#)fut.h	13.55 10/22/97
  Author:	Kit Enscoe, George Pawle

  Fut (function table) interface file.
  It is the only one needed for most applications.

  PROPRIETARY NOTICE: The  software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on their
  designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1989-1997 Eastman Kodak Company.
  As  an  unpublished  work pursuant to Title 17 of the United
  States Code.  All rights reserved.

 */

#ifndef FUT_HEADER
#define FUT_HEADER

/* version number */
/*	Only define this sucker only
 *	for prophecy
 */
#if defined(KPPROPHECY) && !defined(lint)
static char libfutVer[] = "libfutV14.0a";
#endif

#include "kcms_sys.h"
#include "fut_va.h"

/*	If this is ANSI C include stdlib.h.	This should
 *	be OK because fut.h is included at the top of
 *	the source files.
 */
#ifdef GOT_STDLIB_H
#include <stdlib.h>
#endif

/* If we are doing prototypes, be sure to include
 * stdio.h because the type FILE* is in some
 * calls for I/O of FUTs to a file
 */
#include <stdio.h>


#if defined (JAVACMM)
#define FUT_ICC_ONLY
#endif

#if defined (JAVACMM) || defined (KPSGI) || defined (SOLARIS__CMM)
#define NO_FUT_NEAREST
#define NO_FUT_CUBIC
#endif

#if defined(KPMSBFIRST)
#define FUT_MSBF 0xf
#endif

#if defined(KPLSBFIRST)
#define FUT_MSBF 0x0
#endif

#define fut_far	FAR
#define fut_huge KPHUGE
#define fut_flat_far FLAT_FAR

typedef KcmHandle			fut_handle;
typedef char	KPHUGE*		fut_generic_ptr_t;
typedef u_int8	KPHUGE*		generic_u_int8_ptr_t;
typedef int16	KPHUGE*		generic_int16_ptr_t;
typedef char	FLAT_FAR*	fut_flat_far_ptr_t;
typedef u_int16 FLAT_FAR*	fut_flat_far_u_int16_ptr_t;

#define MF_ID_MAKER(c1, c2, c3, c4) ((c1<<24)|(c2<<16)|(c3<<8)|(c4<<0))

#define FUT_IDSTR_MAX (16384-500)			/* max size of an ID string */

/* the current year is concatenated to FUT_COPYRIGHT_PREFIX and then 
	FUT_COPYRIGHT_SUFFIX is concatenated to that resultant string			*/
#define FUT_COPYRIGHT_PREFIX	"Copyright (c) Eastman Kodak Company, 1991-"
#define FUT_COPYRIGHT_SUFFIX	", all rights reserved."

/* Remember that FUT_COPYRIGHT_PREFIX + FUT_COPYRIGHT_SUFFIX +4 for current year
	cannot be greater than or equal to FUT_COPYRIGHT_MAX_LEN (256) */
#define FUT_COPYRIGHT_MAX_LEN 256
 
#define FUT_NCHAN		8				/* absolute maximum number of channels */
#define FUT_NICHAN		8				/* number of FUT input channels (X,Y,Z,T,U,V,W,S) */
#define FUT_NOCHAN		8				/* number of FUT output channels (X,Y,Z,T,U,V,W,S) */

#define FUT_INPTBL_ENT	256				/* # of input table entries */
#define FUT_INPTBL_ENT2 4096			/* # of input table entries */
#define FUT_INPTBL_ENT3 65536			/* # of input table entries -- 16 bit  */
#define FUT_OUTTBL_ENT	4096			/* # of output table entries */

#define FUT_GRD_BITS		12			/* # of bits in grid table entry */
#define FUT_GRD_MAXVAL	4095			/* max value of a grid table entry */
#define FUT_GRD_MAX_Z_DIM	64			/* max size of Z dimension */
#define FUT_GRD_MAX_Y_DIM	64			/* max size of Y dimension */
#define FUT_GRD_MAX_X_DIM	64			/* max size of X dimension */
#define FUT_GRD_MAX_T_DIM	64			/* max size of T dimension */
#if defined (KPWIN16)
#define FUT_GRD_MAX_ENT		((0x10000L-1)/2)	/* 16 bit Windows restricts FAR pointers to 64K-1 BYTES */
#else
#define FUT_GRD_MAX_ENT		(FUT_GRD_MAX_X_DIM * FUT_GRD_MAX_Y_DIM * FUT_GRD_MAX_Z_DIM * FUT_GRD_MAX_T_DIM)	/* max entries in a grid table */
#endif
#define FUT_GRD_MAXDIM		64			/* max size of any given dimension */
#define FUT_INP_FRACBITS	16			/* # of bits in input table fraction */
#define FUT_INP_DECIMAL_PT	16			/* bit position of decimal pt in
											input table scaled integers */
#define FUT_OUT_INTBITS		12			/* # of bits in output table integer */
#define FUT_OUT_FRACBITS	4			/* # of bits in output table fraction */
#define FUT_OUT_DECIMAL_PT	4			/* bit position of decimal pt in
											output table scaled integers */
#define FUT_MAX_PEL8		255			/* maximum 8-bit pel value */
#define FUT_MAX_PEL12		4080		/* maximum 12-bit pel value */

				/* matrix fut definitions */
#define MF_MATRIX_DIM		3			/* size of matrix dimension */
#define MF1_TBL_BITS		8			/* # of bits in 2^3 input, grid, or output table entry */
#define MF2_TBL_BITS		16			/* # of bits in 2^4 input, grid, or output table entry */
#define MF1_TBL_MAXVAL		255			/* max value of a 2^3 grid table entry */
#define MF2_TBL_MAXVAL		65535		/* max value of a 2^4 grid table entry */
#define MF1_TBL_ENT			256			/* # of table entries for 2^3 input or output tables */
#define MF2_MIN_TBL_ENT		2			/* minimum # of table entries for 2^4 input or output tables */
#define MF2_MAX_TBL_ENT		4096		/* maximum # of table entries for 2^4 input or output tables */
#define MF_GRD_MAXDIM		255			/* max size of any given dimension */
#define MF1_TBL_ID			MF_ID_MAKER('m', 'f', 't', '1')	/* 2^3 table ID */
#define MF2_TBL_ID			MF_ID_MAKER('m', 'f', 't', '2')	/* 2^4 table ID */


				/* channel definitions */
#define FUT_XCHAN		0				/* X channel # */
#define FUT_YCHAN		1				/* Y channel # */
#define FUT_ZCHAN		2				/* Z channel # */
#define FUT_TCHAN		3				/* T channel # */
#define FUT_UCHAN		4				/* U channel # */
#define FUT_VCHAN		5				/* V channel # */
#define FUT_WCHAN		6				/* W channel # */
#define FUT_SCHAN		7				/* S channel # */
#define FUT_BIT(x)		((KpUInt32_t)1<<x)
#define FUT_CHAN(x)	(fut_first_chan(x))
#define FUT_X			FUT_BIT(FUT_XCHAN)
#define FUT_Y			FUT_BIT(FUT_YCHAN)
#define FUT_Z			FUT_BIT(FUT_ZCHAN)
#define FUT_T			FUT_BIT(FUT_TCHAN)
#define FUT_U			FUT_BIT(FUT_UCHAN)
#define FUT_V			FUT_BIT(FUT_VCHAN)
#define FUT_W			FUT_BIT(FUT_WCHAN)
#define FUT_S			FUT_BIT(FUT_SCHAN)
#define FUT_XY			(FUT_X|FUT_Y)
#define FUT_XYZ			(FUT_X|FUT_Y|FUT_Z)
#define FUT_XYZT		(FUT_X|FUT_Y|FUT_Z|FUT_T)
#define FUT_XYZTU		(FUT_X|FUT_Y|FUT_Z|FUT_T|FUT_U)
#define FUT_XYZTUV		(FUT_X|FUT_Y|FUT_Z|FUT_T|FUT_U|FUT_V)
#define FUT_XYZTUVW		(FUT_X|FUT_Y|FUT_Z|FUT_T|FUT_U|FUT_V|FUT_W)
#define FUT_XYZTUVWS	(FUT_X|FUT_Y|FUT_Z|FUT_T|FUT_U|FUT_V|FUT_W|FUT_S)
#define FUT_ALL			((KpUInt32_t)(1<<FUT_NCHAN)-1)
#define FUT_ALLIN		((KpUInt32_t)(1<<FUT_NICHAN)-1)
#define FUT_ALLOUT		((KpUInt32_t)(1<<FUT_NOCHAN)-1)

				/* iomask bit-field extraction macros */
#define FUT_IMASK(x)	((KpUInt32_t)((x)>>0) & 0xff)		/* input channel mask */
#define FUT_OMASK(x)	((KpUInt32_t)((x)>>8) & 0xff)		/* output channel mask */
#define FUT_PMASK(x)	((KpUInt32_t)((x)>>16) & 0xff)		/* pass-thru channel mask */
#define FUT_ORDMASK(x)	((KpUInt32_t)((x)>>24) & 0xf)		/* interp. order mask */
#define FUT_IPMASK(x)	((KpUInt32_t)((x)>>28) & 0x1)		/* "inplace" bit */
#define FUT_VAMASK(x)	((KpUInt32_t)((x)>>29) & 0x1)		/* varargs bit */
#define FUT_12BMASK(x)	((KpUInt32_t)((x)>>30) & 0x1)		/* 12 bit datum */

				/* iomask bit-field insertion macros */
#define FUT_IN(x)		((KpUInt32_t)((x) & 0xff)<<0)
#define FUT_OUT(x)		((KpUInt32_t)((x) & 0xff)<<8)
#define FUT_PASS(x)		((KpUInt32_t)((x) & 0xff)<<16)
#define FUT_ORDER(x)	((KpUInt32_t)((x) & 0xf)<<24)
#define FUT_INPLACE		((KpUInt32_t)1<<28)
#define FUT_VARARGS		((KpUInt32_t)1<<29)
#define FUT_12BITS		((KpUInt32_t)1<<30)
#define FUT_EVAL_CACHE	((KpUInt32_t)1<<31)

				/* interpolation orders */
#define FUT_DEFAULT	(0)			/* linear or order of fut */
#define FUT_NEAREST	(1)			/* nearest-neighbor */
#define FUT_LINEAR	(2)			/* linear (bilinear, trilinear, ...) */
#define FUT_CUBIC	(3)			/* 4x4x4... cubic convolution */
#define FUT_BSPLINE	FUT_CUBIC	/* may actually implement this somneday */

#define FUT_EVAL_BUFFER_SIZE 256	/* bytes per component buffer for format general */

typedef struct fut_iomask_s {
#if (FUT_MSBF == 0xF)
	unsigned int	funcmod : 4;	/* function modifiers (see above) */
	unsigned int	order	: 4;	/* interpolation order */
	unsigned int	pass	: 8;	/* pass channels */
	unsigned int	out	: 8;		/* output channels */
	unsigned int	in		: 8;	/* input channels */
#else
#if (FUT_MSBF == 0x0)
	unsigned int	in		: 8;	/* input channels */
	unsigned int	out	: 8;		/* output channels */
	unsigned int	pass	: 8;	/* pass channels */
	unsigned int	order	: 4;	/* interpolation order */
	unsigned int	funcmod : 4;	/* function modifiers (see above) */

#else
===	/* Unsupported byte ordering - cause compiler error */
#endif /* (FUT_MSBF == 0x0) */
#endif /* (FUT_MSBF == 0xF) */
} fut_iomask_t;

/* Structure defining an input table.	The table data (array) is allocated
 * separately.
 *
 * NOTE: the last (257th) table entry is used to perform automatic clipping
 *		when evaluating 12-bit values.	It should always be identical to the
 *		previous (256th) entry.
 */
typedef int32 fut_itbldat_t, fut_far* fut_itbldat_ptr_t, fut_far* fut_far* fut_itbldat_h;

typedef struct fut_itbl_s {
	int32			magic;	/* magic number for runtime checking */
	int32			ref;	/* reference count (<0 => external) */
	int32			id;	/* unique id number */
	int32			size;	/* size of grid in this dimension */
	fut_itbldat_ptr_t tbl;	/* pointer to FUT_INPTBL_ENT+1 entries */
	fut_handle		tblHandle;
	fut_handle		handle;
	KpUInt32_t		tblFlag; /* determines the tbl entry size */
	fut_itbldat_ptr_t tbl2;	/* pointer to FUT_INPTBL_ENT2+1 entries */
} fut_itbl_t, fut_far* fut_itbl_ptr_t;


/* extract integer and fractional parts of an input table entry */
#define FUT_ITBL_INTEG(i)		((i) >> FUT_INP_FRACBITS)
#define FUT_ITBL_FRAC(i)		((i) & ((1<<FUT_INP_FRACBITS)-1))

/* Structure defining an output table.	The table data (array) is allocated
 * separately.
 */
typedef int16 fut_otbldat_t, fut_far* fut_otbldat_ptr_t, fut_far* fut_far* fut_otbldat_h;

typedef struct fut_otbl_s {
	int32			magic;	/* magic number for runtime checking */
	int32			ref;	/* reference count (<0 => external) */
	int32			id;	/* unique id number */
	fut_otbldat_ptr_t tbl;	/* pointer to FUT_OUTTBL_ENT entries */
	fut_handle		tblHandle; /* ... and its memory handle */
	fut_handle		handle; /* memory handle of this struct */
} fut_otbl_t, fut_far* fut_otbl_ptr_t;

/* extract integer and fractional parts of an output table entry.	Also define
 * rounding constant and macro to compute "nearest integer" (instead of truncation)
 */
#define FUT_OTBL_INTEG(o)		((o) >> FUT_OUT_FRACBITS)
#define FUT_OTBL_FRAC(o)		((o) & ((1<<FUT_OUT_FRACBITS)-1))
#define FUT_OTBL_ROUNDUP		((int32)1<<(FUT_OUT_FRACBITS-1))
#define FUT_OTBL_NINT(o)		(((o) > FUT_MAX_PEL12) ? (u_int8)FUT_MAX_PEL8 : \
				(u_int8)FUT_OTBL_INTEG ((o) + FUT_OTBL_ROUNDUP))


/* Structure defining a grid table.	The table data (array) is allocated
 * separately and may easily be bigger than 64K (hence separate pointer
 * data type for I86 architectures).
 */
typedef int16 fut_gtbldat_t, fut_far* fut_gtbldat_ptr_t, fut_far* fut_far* fut_gtbldat_h;

typedef struct fut_gtbl_s {
	int32			magic;				/* magic number for runtime checking */
	int32			ref;				/* reference count (<0 => external) */
	int32			id;					/* unique id number */
	fut_gtbldat_ptr_t tbl;				/* pointer to grid table data */
	fut_handle		tblHandle;			/* ... and its handle */
	int32			tbl_size;			/* size of tbl(in bytes) */
	int16			size[FUT_NCHAN];	/* grid table dimensions */
	fut_handle		handle;				/* memory handle of this struct */
} fut_gtbl_t, fut_far* fut_gtbl_ptr_t;


/* Structure defining a single output channel of a fut.
 * Each fut may have up to FUT_NOCHAN of these.
 */
typedef struct fut_chan_s {
	int32			magic;					/* magic number for runtime checking */
	int32			imask;					/* input mask for this channel */
	fut_gtbl_ptr_t	gtbl;					/* grid table */
	fut_handle		gtblHandle;				/* ... and its memory handle */
	fut_otbl_ptr_t	otbl;					/* optional output table */
	fut_handle		otblHandle;				/* ... and its memory handle */
	fut_itbl_ptr_t	itbl[FUT_NICHAN];		/* input tables */
	fut_handle		itblHandle[FUT_NICHAN];	/* ... and their memory handles */
	fut_handle		handle;					/* memory handle of this struct */
} fut_chan_t, fut_far* fut_chan_ptr_t;


/* Structure defining a fut. */
typedef struct fut_s {
	int32			magic;					/* magic number for runtime checking */
	char_p			idstr;					/* optional id string */
	fut_iomask_t	iomask;					/* input/output mask for fut */
	fut_itbl_ptr_t	itbl[FUT_NICHAN];		/* input tables common to all chans */
	fut_handle		itblHandle[FUT_NICHAN]; /* ... and their memory handles */
	fut_chan_ptr_t	chan[FUT_NOCHAN];		/* output channels */
	fut_handle		chanHandle[FUT_NOCHAN]; /* ... and their memory handles */
	fut_handle		handle;					/* memory handle of this struct */
	int32			refNum;					/* fut caching tag */
	int32			modNum;					/* modification number of this fut */
} fut_t, fut_far* fut_ptr_t;


/* To (soft) share a table, especially common for input and output tables,
 * we just increment the reference count of the table.	A negative reference
 * count is reserved for externally define (or static) tables.	A reference
 * count of zero means that the table is referenced only once (and will be freed
 * on the next call to fut_free[igo]tbl
 */
#define fut_share_itbl(itbl)	(((itbl)==FUT_NULL_ITBL || (itbl)->ref<0) ? (itbl) \
					: ((itbl)->ref++,(itbl)))
#define fut_share_otbl(otbl)	(((otbl)==FUT_NULL_OTBL || (otbl)->ref<0) ? (otbl) \
					: ((otbl)->ref++,(otbl)))
#define fut_share_gtbl(gtbl)	(((gtbl)==FUT_NULL_GTBL || (gtbl)->ref<0) ? (gtbl) \
					: ((gtbl)->ref++,(gtbl)))

#if defined (__cplusplus) || defined(SABR)
extern "C" {
#endif

#if !defined(KPNONANSIC)	/* these function prototypes are defined for ansi C compilers only */
fut_ptr_t		fut_share (fut_ptr_t);

fut_ptr_t	fut_free			(fut_ptr_t);
fut_ptr_t	fut_free_futH 		(fut_handle);
void	fut_free_chan			(fut_chan_ptr_t);
void	fut_free_itbl			(fut_itbl_ptr_t);
void	fut_free_otbl			(fut_otbl_ptr_t);
void	fut_free_gtbl			(fut_gtbl_ptr_t);
void	fut_free_itbl_list		(fut_itbl_ptr_t fut_far*);
void	fut_free_chan_list		(fut_chan_ptr_t fut_far*);
void	fut_free_tbl			(int32_p);
void	fut_free_tbls			(int32_p,...);

fut_ptr_t		fut_new			(int32,...);
fut_itbl_ptr_t	fut_new_itbl	(int, double (*)(double));
fut_otbl_ptr_t	fut_new_otbl	(fut_otbldat_t (*)(fut_gtbldat_t));
fut_gtbl_ptr_t	fut_new_gtblA	(int32,fut_gtbldat_t(*gfun) (double FAR*), int32 FAR*);
fut_chan_ptr_t	fut_new_chan	(int32,...);
fut_ptr_t		fut_new_empty	(int32, int32_p, int32);

int			fut_defchan	(fut_ptr_t,int32,...);
int			fut_add_chan	(fut_ptr_t,int32,fut_chan_ptr_t);
int		fut_count_chan	(int mask);
int			fut_replace_chan (fut_ptr_t,int32,fut_chan_ptr_t);
int			fut_swap_chan	(fut_ptr_t,int32);
fut_ptr_t		fut_new_ident	(int32,...);
fut_ptr_t		fut_new_linear_mix (int32,double,double,...);

int			fut_is_separable		(fut_ptr_t);
fut_ptr_t		fut_share				(fut_ptr_t);
int			fut_to_lut				(fut_ptr_t,int32,int,int,...);
fut_chan_ptr_t	fut_share_chan			(fut_chan_ptr_t);

int fut_get_itbl (fut_ptr_t, int, int, fut_itbldat_ptr_t*);
int fut_get_gtbl (fut_ptr_t, int, fut_gtbldat_ptr_t*);
int fut_get_otbl (fut_ptr_t, int, fut_otbldat_ptr_t*);

int	is_23_splitable (fut_ptr_t);
int fut_split23 (fut_ptr_t, fut_ptr_t fut_far*, fut_ptr_t fut_far*);

int	fut_calc_itbl	(fut_itbl_ptr_t, double (*)(double));
int	fut_calc_otbl	(fut_otbl_ptr_t, fut_otbldat_t (*)(fut_gtbldat_t));
int	fut_calc_gtblA	(fut_gtbl_ptr_t, fut_gtbldat_t (* gfun) (double FAR*));
int	fut_eval			(fut_ptr_t,int32,KCMS_VA_ARG_PTR);
int	fut_eval8			(fut_ptr_t,int32,...);
int	fut_eval12			(fut_ptr_t,int32,...);
int	fut_eval_array		(fut_ptr_t,int32,int32,KCMS_VA_ARG_PTR);
int	fut_eval_array8		(fut_ptr_t,int32,int32,...);
int	fut_eval_array12	(fut_ptr_t,int32,int32,...);
int	fut_eval_chan		(fut_ptr_t, int32, int32, int32, fut_generic_ptr_t fut_far*,
							fut_generic_ptr_t, fut_itbldat_ptr_t fut_far*);
int	fut_eval_gen			(fut_ptr_t,int32,int32,...);
void fut_eval_cache_free	(fut_handle);
fut_ptr_t fut_comp			(fut_ptr_t, fut_ptr_t, int32,...);
fut_ptr_t fut_comp_itbl		(fut_ptr_t, fut_ptr_t, int32);
fut_ptr_t fut_comp_ilut		(fut_ptr_t,int32,...);
fut_ptr_t fut_comp_otbl		(fut_ptr_t, fut_ptr_t, int32);
fut_ptr_t fut_normalize		(int32,int,...);
fut_ptr_t fut_comp_norm		(int32,fut_ptr_t,fut_ptr_t);
fut_ptr_t fut_comp_norm_list (int32,int,...);
fut_ptr_t fut_project		(fut_ptr_t, int32,...);

int	fut_cmp			(fut_ptr_t,fut_ptr_t,double);
int	fut_cmp_chan	(fut_chan_ptr_t,fut_chan_ptr_t,double);
int	fut_cmp_itbl	(fut_itbl_ptr_t,fut_itbl_ptr_t,double);
int	fut_cmp_otbl	(fut_otbl_ptr_t,fut_otbl_ptr_t,double);
int	fut_cmp_gtbl	(fut_gtbl_ptr_t,fut_gtbl_ptr_t,double);

fut_ptr_t		fut_copy		(fut_ptr_t);
fut_chan_ptr_t	fut_copy_chan	(fut_chan_ptr_t);
fut_itbl_ptr_t	fut_copy_itbl	(fut_itbl_ptr_t);
fut_otbl_ptr_t	fut_copy_otbl	(fut_otbl_ptr_t);
fut_gtbl_ptr_t	fut_copy_gtbl	(fut_gtbl_ptr_t);

/* fut i/o */
int		fut_print	(FILE*, fut_ptr_t,int32);
int		fut_graph	(fut_ptr_t,fut_iomask_t,int,char*,short,short);
int32	fut_iomask	(char *);

int			fut_open		(char_p, ...);
int			fut_close		(int);

fut_ptr_t	fut_loadf		(char_p);
int			fut_storef		(fut_ptr_t,char_p);
fut_ptr_t	fut_readf		(FILE fut_far*);
int			fut_writef		(FILE fut_far*,fut_ptr_t);

fut_ptr_t	fut_load		(char_p);
fut_ptr_t	fut_load_fp		(char_p, ioFileChar);
int			fut_store		(fut_ptr_t, char_p);
int32		fut_store_fp	(fut_ptr_t, char_p, ioFileChar);
fut_ptr_t	fut_read		(int);
fut_ptr_t	fut_read_Kp 	(KpFd_p fd);
int			fut_write		(int, fut_ptr_t);
int			fut_write_Kp 	(KpFd_p fd, fut_ptr_t fut);

fut_ptr_t	fut_loadMFut	(char_p, Fixed_p);
fut_ptr_t	fut_loadMFut_fp	(char_p, ioFileChar, Fixed_p);
int32		fut_storeMFut	(fut_ptr_t, Fixed_p, int32, int32, char_p);
int32		fut_storeMFut_fp	(fut_ptr_t, Fixed_p, int32, int32, char_p, ioFileChar);
fut_ptr_t	fut_readMFut	(int, Fixed_p);
fut_ptr_t	fut_readMFut_Kp	(KpFd_p, Fixed_p);
int32		fut_writeMFut	(int, fut_ptr_t, Fixed_p, int32, int32);
int32		fut_writeMFut_Kp	(KpFd_p, fut_ptr_t, Fixed_p, int32, int32);

int			fut_cal_crc		(fut_ptr_t, int32_p);

#ifdef FUT_XDR_SUPPORTED
int	xdr_fut_ptr	(XDR*,fut_ptr_t*);
int	idr_fut_ptr	(XDR*,fut_ptr_t*);
#endif

/* initialization functions - may be passed to fut_calc_tbl routines */
double			fut_iramp		(double);
fut_otbldat_t	fut_oramp		(fut_gtbldat_t);
fut_gtbldat_t	fut_gramp		(double FAR*);
fut_gtbldat_t	fut_gconst		(double FAR*);
void			fut_set_gconst	(fut_gtbldat_t);

/* idstring stuff */
int		fut_new_idstr	(fut_ptr_t, char_p);
int		fut_set_idstr	(fut_ptr_t, char_p);
char_p	fut_idstr	(fut_ptr_t);
char_p	fut_get_idstr	(fut_ptr_t, char_p);
char_p	fut_put_idstr	(fut_ptr_t, char_p);
char_p	fut_idstr_new	(int, int32_p);
int		fut_idstr_renew	(char_p fut_far*, int32, int32_p);
void	fut_idstr_free	(char_p);
int		fut_idstr_add	(char_p fut_far*, char_p, char_p, int32_p);
int		fut_idstr_remove (char_p fut_far*, char_p, int32_p);
int		fut_idstr_change (char_p fut_far*, char_p, char_p, int32_p);
int		fut_idstr_enumerate (char_p, int (*)(char_p, char_p));
char_p	fut_idstr_find	(char_p, char_p);
int32	fut_idstr_optcode	(char_p);
int		fut_idstr_scan	(char_p, int32_p, int32_p);
void	fut_itoa	(int32 n, char_p);
int32	fut_atoi	(char_p);
int		fut_make_copyright (char_p);
 

/* fut handle stuff */
fut_ptr_t		fut_lock_fut	(fut_handle);
fut_itbl_ptr_t	fut_lock_itbl	(fut_handle);
fut_otbl_ptr_t	fut_lock_otbl	(fut_handle);
fut_gtbl_ptr_t	fut_lock_gtbl	(fut_handle);
fut_chan_ptr_t	fut_lock_chan	(fut_handle);

fut_handle		fut_unlock_fut	(fut_ptr_t);
fut_handle		fut_unlock_itbl	(fut_itbl_ptr_t);
fut_handle		fut_unlock_otbl	(fut_otbl_ptr_t);
fut_handle		fut_unlock_gtbl	(fut_gtbl_ptr_t);
fut_handle		fut_unlock_chan	(fut_chan_ptr_t);

#endif	/* end of ansi C prototypes */

#if defined (__cplusplus)
}
#endif

#define FUT_GCONST(x)	(fut_set_gconst(x), fut_gconst)
			/* NOTE: this macro is useful for initializing a grid
			*		table to a constant 12-bit value.	It calls
			*		fut_set_gconst() which sets a static variable
			*		to the desired constant, and then evaluates to
			*		a pointer to the function fut_gconst() which
			*		which always returns this value.
			*/

#define FUT_NULL_IFUN	((double (*)(double))0)
#define FUT_NULL_OFUN	((fut_otbldat_t (*)(fut_gtbldat_t))0)
#define FUT_NULL_GFUN	((fut_gtbldat_t (*)(double FAR*))0)
#define FUT_NULL_CHAN	((fut_chan_ptr_t)0)
#define FUT_NULL		((fut_ptr_t)0)
#define FUT_NULL_ITBL	((fut_itbl_ptr_t)0)
#define FUT_NULL_ITBLDAT ((fut_itbldat_ptr_t)0)
#define FUT_NULL_OTBL	((fut_otbl_ptr_t)0)
#define FUT_NULL_OTBLDAT ((fut_otbldat_ptr_t)0)
#define FUT_NULL_GTBL	((fut_gtbl_ptr_t)0)
#define FUT_NULL_GTBLDAT ((fut_gtbldat_ptr_t)0)
#define FUT_NULL_HANDLE ((fut_handle)0)

#endif /*	FUT_HEADER */

