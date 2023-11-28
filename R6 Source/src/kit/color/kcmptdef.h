/*
	File:           kcmptdef.h	@(#)kcmptdef.h	2.63 11/18/97

	Contains:       Header file for KCMS Processor Library

	Written by:     Drivin' Team

	Copyright:      (c) 1991-1996 by Eastman Kodak Company, all rights reserved.
 */

#ifndef KCMPTDEF_H_
#define KCMPTDEF_H_ 1

#include "kcmsos.h"

#if defined (KPMACPPC)
#pragma options align=mac68k	/* make sure these structures are 68k type for backward compatibility */
#endif

/* define KCMS Processor configurations */

/* Macintosh Platform */
#if defined(KPMAC)				/* PRO version only */
	#define	KCP_ACCEL	1
	#define KCP_COMP_2	1

#if defined (be || 1)
	#define	KCP_EVAL_TH1 1			/* defined is on, else is off */
#endif

#if defined (KPMACPPC)
	#define	KCP_EVAL_TH1 1			/* defined is on, else is off */
	#define KCP_MACPPC_MP 1			/* enable multi-processing */
#endif

#if defined (KPMAC68K)
	#define	KCP_EVAL_TH1 1			/* defined is on, else is off */
#endif

#endif								/* #if defined (KPMAC) */

/* Windows 32 bit Platform */
#if defined(KPWIN32)
	#if defined(KCP_FLAVOR_p)
		#define	KCP_ACCEL	1
		#define KCP_COMP_2	1
		#define KCP_INI_FILE	1
	#endif
	#define	KCP_EVAL_TH1 1			/* defined is on, else is off */
	#define KCP_THREAD_MP 1			/* enable multi-processing */
#endif

/* Windows 16 bit Platform */
#if defined(KPWIN16)					/* PRO version only */
	#define	KCP_ACCEL	1
	#define KCP_COMP_2	1
	#define KCP_INI_FILE	1
	#define	KCP_EVAL_TH1 1				/* defined is on, else is off */
#endif

/* Sun Platform */
#if defined(KPSUN)
/*	#define KCP_THREAD_MP 1 */			/* enable multi-processing */
	#define KCP_DIRECT_LINK	1			/* for future use */
	#if defined(KCP_FLAVOR_p)
/*		#define	KCP_ACCEL	1 */  /* Hardware accelleration not supported yet */
		#define KCP_COMP_2	1
	#endif
	#define	KCP_EVAL_TH1 1				/* defined is on, else is off */
#endif

#if defined(KPSGI) || defined(KPSGIALL)
	/* M000 */
	#define KCP_COMP_2      1
	#define	KCP_EVAL_TH1 1			/* defined is on, else is off */
#endif /* KPSGI */

/* Java Platform */
#if defined(JAVACMM)
	#define KP_NO_IOTBL_COMBINE	1	/* do not compile options that are not needed! */
#endif	/* JAVACMM */

#define PT_COMPOSITION2		1		/* composition bit position */
#define PT_ACCELERATOR		2		/* accelerator bit position */
#define PT_TLI_EVALUATION1	256 	/* tli evaluation type 1 bit position */
#define PT_TLI_EVALUATION2	512 	/* tli evaluation type 2 bit position */
#define PT_LTECH_EVALUATION	1024	/* logtech evaluation bit position */

#define PTCHECKINSIZE	(16384)	/* bytes needed from external database for checkin */
#define MINPTMEMSIZE	(8192)	/* bytes needed between heap and stack after activate */

#define KCMS_PTA_MIN 0
#define KCMS_PTA_MAX KCMS_PTA_MIN+16383
#define CIPG_PTA_MIN KCMS_PTA_MAX+1
#define CIPG_PTA_MAX CIPG_PTA_MIN+16383
#define KODAK_PTA_MIN CIPG_PTA_MAX+1
#define KODAK_PTA_MAX KODAK_PTA_MIN+32767
#define PUBLIC_PTA_MIN KODAK_PTA_MAX+1
#define PUBLIC_PTA_MAX PUBLIC_PTA_MIN+32767

typedef enum PTEvalTypes_e {
	KCP_EVAL_DEFAULT = 0,				/* use default evaluator */
	KCP_EVAL_SW = 1,					/* use software evaluator */
	KCP_EVAL_CTE = 2,					/* evaluate function using Nubus Function Engine */
	KCP_EVAL_LOGTECH = 3,				/* use LOGTECH software evaluator */
	KCP_EVA_TYPE_END = BIG_ENUM_NUMBER
} PTEvalTypes_t, FAR* PTEvalTypes_p, FAR* FAR* PTEvalTypes_h;

#define EVAL_LIST_MAX 10				/* maximum number of evalList_t */

typedef int32 PTType_t, FAR* PTType_p, FAR* FAR* PTType_h;

#define PTTYPE_FUTF (0x66757466L)		/* 'futf' */
#define PTTYPE_FTUF (0x66747566L)		/* 'ftuf' */
#define PTTYPE_MFT1 (0x6d667431L)		/* 'mft1' */
#define PTTYPE_MFT2 (0x6d667432L)		/* 'mft2' */

#define PT_COMBINE_STD		(0)			/* compose modes of operation */
#define PT_COMBINE_ITBL		(2)
#define PT_COMBINE_OTBL		(3)
#define PT_COMBINE_PF_8		(4)
#define PT_COMBINE_PF_16	(5)
#define PT_COMBINE_PF		(6)
#define PT_COMBINE_TYPE		(0xff)
#define PT_COMBINE_CUBIC	(1<<9)		/* 1 = use cubic evaluation */
#define PT_COMBINE_LARGEST	(2<<9)		/* resulatnt grid dimension the largest of pair */
#define PT_COMBINE_ICM	PT_COMBINE_PF_8

#define KCP_EVAL_SYNC (0)				/* synchronous evaluation */
#define KCP_EVAL_ASYNC (1)				/* asynchronous evaluation */

/* KCMS processor library error codes */
typedef enum PTErr_e {
	KCP_SUCCESS = 1,
	KCP_INPROG = 0,
	KCP_FAILURE = -1,
	KCP_NO_CHECKIN_MEM = 100,		/* no memory available for PT check in */
	KCP_INVAL_PT_BLOCK = 101,		/* invalid PT block */
	KCP_NO_KCMS = 102,				/* KCMS processor not present */
	KCP_INVAL_REFNUM = 103,			/* the reference number is not valid */
	KCP_PT_IN_USE = 104,			/* the PT is in use for evaluation */
	KCP_INCON_PT = 105,				/* the activated PT block is inconsistent with the PTRefNum PT block */
	KCP_NOT_CHECKED_IN = 106,		/* the PTRefNum PT has not been checked in */
	KCP_PT_ACTIVE = 107,			/* the PTRefNum PT is currently active */
	KCP_PT_INACTIVE = 108,			/* the PTRefNum PT is not currently active */
	KCP_NO_ACTIVATE_MEM = 109,		/* no memory available for PT activate */
	KCP_INVAL_PTA_TAG = 110,		/* invalid attributeTag */
	KCP_INVAL_IN_VAR = 111,			/* input variable specification does not match PT */
	KCP_INVAL_OUT_CHAN = 112,		/* output channel specification does not match PT */
	KCP_INVAL_EVAL = 113,			/* invalid evalID and/or devNum specification */
	KCP_NOT_COMPLETE = 114,			/* unable to complete evaluation */
	KCP_INVAL_OPREFNUM = 115,		/* invalid opRefNum */
	KCP_INVAL_RN_LIST = 116,		/* invalid PTRefNum in PTList */
	KCP_CHAIN_INVAL = 117,			/* chain validation failed */
	KCP_NO_CHAININIT = 118,			/* no prior PTChainInit call */
	KCP_EXCESS_PTCHAIN = 119,		/* more PTChain calls than defined in PTChainInit */
	KCP_INVAL_PT_SEQ = 120,			/* PTRefNum does not match prior PTChainInit sequence */
	KCP_NOT_CHAINING = 121,			/* no chaining in progress */
	KCP_NOT_CHAINEND = 122,			/* not all PTs have been chained */
	KCP_PT_BLOCK_TOO_SMALL = 123,	/* external PT memory block is too small */
	KCP_EVAL_TABLE_FULL = 124,		/* table already has max number of eval types */
	KCP_NO_ATTR_MEM = 125,			/* no memory available for attribute handle */
	KCP_ATT_SIZE_TOO_SMALL = 126,	/* size allocated to get attribute is too small */
	KCP_CTE_INVAL_INPUTS = 127,		/* # of input variables doesn't match fut. */
	KCP_CTE_INVAL_OUTPUTS = 128,	/* output variables are not a subset of fut. */
	KCP_CTE_TIMEOUT = 129,			/* Color Transform Engine time out. */
	KCP_CTE_DMA_ERROR = 130,		/* Color Transform Engine dma error. */
	KCP_CTE_INVAL_ADDRESS = 131,	/* CTE unable to compute physical address. */
	KCP_NO_KCME1 = 132,				/* .KCME1 processor not present */
	KCP_CHAIN_INPUTS_ERR = 133,		/* PT1 does not have all outputs needed as input to PT2 */
	KCP_CHAIN_OUTPUTS_ERR = 134,	/* PT2 does not have all outputs needed for compose of 1 and 2 */
	KCP_NOT_FUT = 135,				/* the PT is not a fut */
	KCP_NOT_RCS = 136,				/* RCS validation error */
	KCP_BAD_COMP_ATTR = 137,		/* PT has improper composition attribute */
	KCP_BAD_COMP_MODE = 138,		/* non-existent composition mode */
	KCP_AP_RULE7_FAILURE = 139,		/* attribute propogation rule #7 failed */
	KCP_MEM_LOCK_ERR = 140,			/* error locking memory */
	KCP_MEM_UNLOCK_ERR = 141,		/* error unlocking memory */
	KCP_ENCODE_PTHDR_ERR = 142,		/* encode PT header failed */
	KCP_NO_MEMORY = 143,			/* memory allocation failure */
	KCP_NO_SYSMEM = 144,			/* system memory allocation failure */
	KCP_ATTR_TOO_BIG = 145,			/* This really should be called KCP_ATTR_BAD_SIZE
										since this is used when either the attribute
										size is too big or is less than zero. */
	KCP_NO_COMPOSITION = 146,		/* PT composition not supported */
	KCP_COMP_FAILURE = 147,		/* PT composition failed */
	KCP_PT_HDR_WRITE_ERR = 148,	/* write of PT header to memory failed */
	KCP_PT_DATA_WRITE_ERR = 149,	/* write of PT data to memory failed */
	KCP_PTERR_0 = 150,				/* PT consistency check error(not correct table type) */
	KCP_PTERR_1 = 151,				/* PT memory handling error(unlock PT failed) */
	KCP_PTERR_2 = 152,				/* PT memory handling error(lock PT failed) */
	KCP_PTERR_3 = 153,				/* unassigned PT error */
	KCP_PTERR_4 = 154,				/* unassigned PT error */
	KCP_PTERR_5 = 155,				/* unassigned PT error */
	KCP_PTERR_6 = 156,				/* unassigned PT error */
	KCP_PTERR_7 = 157,				/* unassigned PT error */
	KCP_PTERR_8 = 158,				/* unassigned PT error */
	KCP_PTERR_9 = 159,				/* unassigned PT error */
	KCP_SYSERR_0 = 160,				/* internal system error 1(unknown function sent) */
	KCP_SYSERR_1 = 161,				/* internal system error 1(unknown function received) */
	KCP_SYSERR_2 = 162,				/* unassigned system error */
	KCP_SYSERR_3 = 163,				/* unassigned system error */
	KCP_SYSERR_4 = 164,				/* unassigned system error */
	KCP_SYSERR_5 = 165,				/* unassigned system error */
	KCP_SYSERR_6 = 166,				/* unassigned system error */
	KCP_SYSERR_7 = 167,				/* unassigned system error */
	KCP_SYSERR_8 = 168,				/* unassigned system error */
	KCP_SYSERR_9 = 169,				/* unassigned system error */
	KCP_NO_INTABLE = 170,			/* input table does not exist */
	KCP_NO_OUTTABLE = 171,			/* output table does not exist */
	KCP_OUTSPACE_INSPACE_ERR = 172, /* Chaining output space of 1st not equal to input space of 2nd */
	KCP_LOGTECH_FILE_ERR = 173,		/* Can't open or read the logTech log/antilog file */
	KCP_COPYRIGHT_SIZE = 174,		/* The size of the copyright string is
												greater than FUT_COPYRIGHT_MAX_LEN */
	KCP_INV_COPYRIGHT = 175,		/* The copyright string is invalid */
	KCP_NOT_IMPLEMENTED = 176,		/* This function not yet implemented */
	KCP_INVAL_DATA_TYPE = 177,		/* The image data type is invalid */
	KCP_AUX_UNKNOWN = 178,			/* The specified auxilliary PT does not
									   exist and can not be created */
	KCP_AUX_INVALID = 179,			/* The auxPTbuild structure is invalid */
	KCP_AUX_ERROR = 180,			/* An Error occured creating the specified
									   auxilliary PT */
	KCP_SEMAPHORE_ERR = 181,		/* An error occured while performing a 
									   semaphore operation						*/
	KCP_INVAL_GRID_DIM = 182,		/* An error occured trying to validate
									   the dimensionf of a grid table		*/
	KCP_BAD_ARG = 183,				/* argument out of range */
	KCP_REL2ABS_ERROR = 184,		/* Error in generation of relative to absolute PT */

	KCP_BAD_PTR = 300,				/* The pointer (address) is invalid */
	KCP_BAD_CALLBACK_ADDR = 301,	/* The CALLBACK function address is invalid */
	KCP_PT_DATA_READ_ERR = 302,		/* read of PT data to memory failed */
	KCP_INVAL_PTTYPE = 303,			/* Invalid PTType */
	KCP_NO_PROCESS_GLOBAL_MEM = 304, /* Cannot find process global memory */
	KCP_NO_THREAD_GLOBAL_MEM = 305,	/* Cannot find thread global memory */
	KCP_SERIAL_PT = 306,			/* This PT data is only valid for serial evaluations */
	KCP_NOT_SERIAL_PT = 307,		/* This PT data is not valid for serial evaluations */
	KCP_PT_INVISIBLE = 308			/* This PT data is only valid as serial component */
} PTErr_t, FAR* PTErr_p, FAR* FAR* PTErr_h;

typedef int32 PTRefNum_t, FAR* PTRefNum_p, FAR* FAR* PTRefNum_h;
typedef int32_long_ptr	PTRefNum_long_ptr_t;
typedef int32 opRefNum_t, FAR* opRefNum_p, FAR* FAR* opRefNum_h;
typedef int32_long_ptr	opRefNum_long_ptr_t;
typedef void FAR* PTAddr_t, FAR* FAR* PTAddr_p, FAR* FAR* FAR* PTAddr_h;

typedef void FLAT_HUGE* PTImgAddr_t;

typedef struct PTCompDef_s {
	int32		pelStride;			/* input pel stride in bytes */
	int32		lineStride;			/* input line stride in bytes */
	PTImgAddr_t addr;				/* input data addresses in X, Y, Z, T order */
} PTCompDef_t, FAR* PTCompDef_long_ptr_t, FAR* PTCompDef_p;

/*	The size of the pointer to the components
 *	in the PTEvalPB_t structure will be
 *	a near pointer in the Watcom 32-bit
 *	flat model C compiler only.
 */
#if defined(__386__)
typedef PTCompDef_t*			PTCompDef_ptr_t;
#else
typedef PTCompDef_long_ptr_t	PTCompDef_ptr_t;
#endif
typedef struct PTEvalPB_s {
	int32 nPels;					/* # of pels per line */
	int32 nLines;					/* # of lines in image */
	int32 nInputs;					/* # of input components */
	PTCompDef_ptr_t input;			/* input data components X, Y, Z, T order */
	int32 nOutputs;					/* # of output components */
	PTCompDef_ptr_t output; 		/* output data components X, Y, Z, T order */
} PTEvalPB_t, FAR* PTEvalPB_long_ptr_t, FAR* PTEvalPB_p;

typedef struct PTEvalDTPB_s {
	int32 nPels;					/* # of pels per line */
	int32 nLines;					/* # of lines in image */
	int32 nInputs;					/* # of input components */
	int32 dataTypeI;				/* data type of input components */
	PTCompDef_ptr_t input;			/* input data components X, Y, Z, T order */
	int32 nOutputs;					/* # of output components */
	int32 dataTypeO;				/* data type of output components */
	PTCompDef_ptr_t output;			/* output data components X, Y, Z, T order */
} PTEvalDTPB_t, FAR* PTEvalDTPB_long_ptr_t, FAR* PTEvalDTPB_p;


typedef struct evalList_s {
	PTEvalTypes_t	evalID;			/* id of evaluator */
	int32			number;			/* number of these evaluators */
} evalList_t, FAR* evalList_long_ptr_t, FAR* evalList_p;


/*	32-bit near versions of some structures
 *	These are only needed when using the 32 Bit
 *	Watcom compiler with MS Windows
 */
#if defined(__386__)
typedef struct {
	int32		pelStride;		/* input pel stride in bytes */
	int32		lineStride;		/* input line stride in bytes */
	char *		addr;			/* input data addresses in X, Y, Z, T order */
} PTCompDef_near_t;

typedef struct {
	int32 nPels;				/* # of pels per line */
	int32 nLines;				/* # of lines in image */
	int32 nInputs;				/* # of input components */
	int32 dataTypeI;			/* data type of input components */
	PTCompDef_near_t *input;	/* input data components X, Y, Z, T order */
	int32 nOutputs;				/* # of output components */
	int32 dataTypeO;			/* data type of output components */
	PTCompDef_near_t *output;	/* output data components X, Y, Z, T order */
} PTEvalDTPB_near_t;
#else
#define PTEvalDTPB_near_t		PTEvalDTPB_t
#endif

/* control structure for PTNewMatGamAIPT */
typedef struct newMGmode_s {
	KpUInt32_t adaptMode;
	KpUInt32_t interpMode;
} newMGmode_t, FAR* newMGmode_p;

/* definitions for above structure */
#define KCP_NO_ADAPTATION (0)			/* no white point adaptation */
#define KCP_XYZD50_ADAPTATION (1)		/* use "XYZ D50" adaptation */
#define KCP_VONKRIESD50_ADAPTATION (2)	/* use "Von Kries D50" adaptation */
#define KCP_TRC_LINEAR_INTERP (1)		/* use linear interpolation */
#define KCP_TRC_LAGRANGE4_INTERP (2)	/* use 4 point Lagrangian interpolation */
#define KCP_TRC_LINEAR_INTERP_LAB (3)	/* use linear interpolation */
#define KCP_TRC_LAGRANGE4_INTERP_LAB (4)	/* use 4 point Lagrangian interpolation */

/* control structure for PTInitializeEx (Windows Only) */
#if defined (KPWIN)
typedef struct PTInitInfo_s {
	KpUInt32_t	structSize;
	HINSTANCE	appModuleId;
}PTInitInfo_t, FAR* PTInitInfo_p;
#endif

/*-----------------------------------------------------------
 * definitions for some parts of ICC open profile format 
 *-----------------------------------------------------------
 */

typedef struct tagFixedXYZColor_s {
	Fixed_t	X;
	Fixed_t	Y;
	Fixed_t	Z;
} FixedXYZColor,	FixedXYZColor_t,FAR* FixedXYZColor_p, FAR* FAR* FixedXYZColor_h;

typedef struct tagResponseRecord_s {
	u_int32		count;
	u_int16_p	data;
} ResponseRecord, ResponseRecord_t, FAR* ResponseRecord_p, FAR* FAR* ResponseRecord_h;

/* control structure for PTGetRelToAbsPT */
typedef struct PTRelToAbs_s {
	KpUInt32_t 		RelToAbsSize;			/* size of THIS data structure */
	FixedXYZColor_t srcMediaWhitePoint;
	FixedXYZColor_t dstMediaWhitePoint;
	FixedXYZColor_t srcProfileWhitePoint;
	FixedXYZColor_t dstProfileWhitePoint;
	KpUInt32_t 		gridSize;
} PTRelToAbs_t, FAR* PTRelToAbs_p;

#if defined (KPMACPPC)
#pragma options align=reset	/* set alignment back to original in case of succeeding #includes */
#endif

#endif	/* KCMPTDEF_H_ */

