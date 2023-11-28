/*
 * Fnt.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#ifndef FNT_H
#define FNT_H

/* #define DEBUG_FNT ONLY define when debugging, NEVER in a final build */
#if 1 
#include "t2kstrm.h"
#include "tt_prvt.h"
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


/* BEGIN T2K additions */
typedef short ShortFrac;			/* 2.14 */
#define ONESHORTFRAC				(1 << 14)
#define FIXONEHALF			0x00008000
#define FIXROUND( x )			(int16)(((x) + FIXONEHALF) >> 16)

typedef void (*voidFunc)(void);
typedef void (*fs_FuncType)(void*);

typedef int16	LoopCount;
typedef int32	ArrayIndex;

#define GLOBAL_PROTO
#define LOCAL_PROTO	static
#define FUNCTION
#ifndef PASCAL
#define PASCAL
#endif

#define fnt_pixelSize 0x40L
#define fnt_pixelShift 6

#define MAXBYTE_INSTRUCTIONS 256

#define VECTORTYPE					ShortFrac
#define ONEVECTOR					ONESHORTFRAC
#define VECTORMUL(value, component)	ShortFracMul(value, component)
#define VECTORDOT(a,b)				ShortFracDot(a,b)
#define VECTORMULDIV(a,b,c)			ShortMulDiv(a,b,c)
#define ONESIXTEENTHVECTOR			((ONEVECTOR) >> 4)

#define SHORTMUL(a,b)	(int32)((a) * (b))

#define SROUND( x, n, d, halfd ) \
    if ( x > -1 ) { \
	    x = (SHORTMUL(x, n) + halfd)/d; \
	} else { \
	    x = -((SHORTMUL(-x, n) + halfd) / d); \
	}
	
#define ONEFIX 		( 1L << 16 )
#define ONCURVE 1
#define btsbool int
/*
#define STUBCONTROL 0x10000
#define NODOCONTROL 0x20000
*/
#define ACTIVATE_DROPOUTCONTROL	0x10000
#define SMART_DROPOUT 			0x20000
#define INCLUDE_STUBS			0x40000


/* fnt_execute errors */
#define NO_ERR						0x0000
#define UNDEFINED_INSTRUCTION_ERR	0x1101
#define TRASHED_MEM_ERR				0x1102



typedef struct VECTOR {
	VECTORTYPE x;
	VECTORTYPE y;
} VECTOR;

typedef struct {
	F26Dot6 *x;		/* The Points the Interpreter modifies */
    F26Dot6 *y;		/* The Points the Interpreter modifies */
    F26Dot6 *ox;	/* Old Points */
    F26Dot6 *oy;	/* Old Points */
    int16   *oox;	/* Old Unscaled Points, Changed to int16 Dec 23, 1998 ---Sampo */
    int16   *ooy;	/* Old Unscaled Points, Changed to int16 Dec 23, 1998 ---Sampo */
	uint8	*onCurve; /* indicates if a point is on or off the curve */
	int16 	 nc; 	/* Number of contours */
	int16	 padWord;	/* <4> */
	int16	*sp; 	/* Start points */
	int16	*ep; 	/* End points */
	uint8	*f; 	/* Internal flags, one byte for every point */
} fnt_ElementType;

typedef struct {
    int32 start;		/* offset to first instruction */
	uint16 length;		/* number of bytes to execute <4> */
	uint16 pgmIndex;	/* index to appropriate preprogram for this func (0..1) */
} fnt_funcDef;

/* <4> pretty much the same as fnt_funcDef, with the addition of opCode */
typedef struct {
	int32 start;
	uint16 length;
	uint8  pgmIndex;
	uint8  opCode;
} fnt_instrDef;

typedef struct {
	Fract x;
	Fract y;
} fnt_FractPoint;

/* Define the conditional parameter macros here */
/* T2K is reentrant */
/* #define REENTRANT_ALLOC 1 */

typedef struct fnt_LocalGraphicStateType fnt_LocalGraphicStateType_t;
typedef struct fnt_GlobalGraphicStateType fnt_GlobalGraphicStateType_t;

#define REENTRANT_ALLOC 1
#if REENTRANT_ALLOC
#define GSP_DECL0 fnt_LocalGraphicStateType_t *pLocalGS
#define GSP_DECL1 GSP_DECL0,
#else
#define GSP_DECL0 void
#define GSP_DECL1
#endif

/* typedefs for callback functions */

typedef void	(*FntFunc)(GSP_DECL0);
typedef void	(*InterpreterFunc)(GSP_DECL1 uint8*, uint8*);
typedef void	(*FntMoveFunc)(GSP_DECL1 fnt_ElementType*, ArrayIndex, F26Dot6);
typedef F26Dot6	(*FntRoundFunc)(GSP_DECL1 F26Dot6, F26Dot6);
typedef F26Dot6	(*FntProjFunc)(GSP_DECL1 F26Dot6, F26Dot6);
typedef F26Dot6 (*GetCVTEFunc)(GSP_DECL1 ArrayIndex n);
typedef F26Dot6 (*GetSWidFunc)(GSP_DECL0);

/* typedefs for TraceFunc and ScaleFunc callbacks */

typedef void	(*TraceFuncType)(fnt_LocalGraphicStateType_t *);
typedef F26Dot6 (*ScaleFuncType)(fnt_GlobalGraphicStateType_t *, F26Dot6);

typedef struct {

/* PARAMETERS CHANGEABLE BY TT INSTRUCTIONS */

	F26Dot6 wTCI;     				/* width table cut in */
	F26Dot6 sWCI;     				/* single width cut in */
	F26Dot6 scaledSW; 				/* scaled single width */
	int32 scanControl;				/* controls kind and when of dropout control */
	int32 instructControl;			/* controls gridfitting and default setting */
	
	F26Dot6 minimumDistance;		/* moved from local gs  7/1/90  */
	FntRoundFunc RoundValue;		/*								*/
	F26Dot6	periodMask; 			/* ~(gs->period-1) 				*/
	Fract	period45;				/*								*/
	int16	period;					/* for power of 2 periods 		*/
	int16 	phase;					/*								*/
	int16 	threshold;				/* moved from local gs  7/1/90  */
	
	int16 deltaBase;
	int16 deltaShift;
	int16 angleWeight;
	int16 sW;         				/* single width, expressed in the same units as the character */
	int8 autoFlip;   				/* The auto flip Boolean */
	int8 pad;	

} fnt_ParameterBlock;				/* this is exported to client */

#define MAXANGLES		20
#define ROTATEDGLYPH	0x100
#define STRETCHEDGLYPH  0x200
#define NOGRIDFITFLAG	1
#define DEFAULTFLAG		2

typedef enum {
	PREPROGRAM,
	FONTPROGRAM,
	MAXPREPROGRAMS
} fnt_ProgramIndex;

typedef struct fnt_GlobalGraphicStateType {

	F26Dot6* stackBase; 			/* the stack area */
	F26Dot6* store; 				/* the storage area */
	F26Dot6* controlValueTable; 	/* the control value table */
	
	uint16	pixelsPerEm; 			/* number of pixels per em as an integer */
	uint16	pointSize; 				/* the requested point size as an integer */
	Fixed	fpem;					/* fractional pixels per em    <3> */
	F26Dot6 engine[4]; 				/* Engine Characteristics */
	
	fnt_ParameterBlock defaultParBlock;	/* variables settable by TT instructions */
	fnt_ParameterBlock localParBlock;

	/* Only the above is exported to Client throught FontScaler.h */

/* VARIABLES NOT DIRECTLY MANIPULABLE BY TT INSTRUCTIONS  */
	
    FntFunc* function; 				/* pointer to instruction definition area */
	fnt_funcDef* funcDef; 			/* function Definitions identifiers */
	fnt_instrDef* instrDef;			/* instruction Definitions identifiers */
   
	ScaleFuncType ScaleFunc;		/* Call back function to do scaling */
	
	uint8* pgmList[MAXPREPROGRAMS];	/* each program ptr is in here */
	
    /* These are parameters used by the call back function */

	Fixed  fixedScale; 			/* fixed sc aling factor */
	int32  nScale; 				/* numerator required to scale points to the right size*/
	int32  dScale; 				/* denumerator required to scale points to the right size */
	int32  dScaleDiv2;			/* Added Jan 8, 1999 ---Sampo */
	int16  dShift; 				/* 2log of dScale */
	int8   identityTransformation; 	/* true/false  (does not mean identity from a global sense) */
	int8   non90DegreeTransformation; /* bit 0 is 1 if non-90 degree, bit 1 is 1 if x scale doesn't equal y scale */
	Fixed  xStretch; 			/* Tweaking for glyphs under transformational stress <4> */
	Fixed  yStretch;
	
	/** these two together make fnt_AngleInfo **/

	fnt_FractPoint*	anglePoint;
	int16*			angleDistance;

    int8 init; 						/* executing preprogram ?? */
	uint8 pgmIndex;					/* which preprogram is current */
	LoopCount instrDefCount;		/* number of currently defined IDefs */

	long					cvtCount;
	maxpClass				*maxp;
#ifdef XDEBUG
	uint16					glyphIndex_old;
	btsbool					glyphProgram_Old;
#endif

} fnt_GlobalGraphicStateType;


/* 
 * This is the local graphics state  
 */


typedef struct fnt_LocalGraphicStateType {
	uint8 *insPtr; 						/* Pointer to the instruction we are about to execute */
	int opCode; 						/* The instruction we are executing */

	fnt_ElementType *CE0, *CE1, *CE2; 	/* The character element pointers */
	VECTOR proj; 						/* Projection Vector */
	VECTOR free;						/* Freedom Vector */
	VECTOR oldProj; 					/* Old Projection Vector */
	F26Dot6 *stackPointer;

#ifdef OLD
	uint8 *insPtr;						/* Pointer to the instruction we are about to execute */
#endif
    fnt_ElementType *elements;
    fnt_GlobalGraphicStateType *globalGS;

	TraceFuncType TraceFunc;			/* Call back to do tracing */

    ArrayIndex Pt0, Pt1, Pt2; 			/* The internal reference points */
	int16   roundToGrid;			
	LoopCount loop; 					/* The loop variable */	
#ifdef OLD
	uint8 opCode; 						/* The instruction we are executing */
	uint8 padByte;
	int16 padWord;
#endif /* OLD */

	/* Above is exported to client in FontScaler.h */

	VECTORTYPE pfProj; /* = pvx * fvx + pvy * fvy */

	FntMoveFunc MovePoint;
	FntProjFunc Project;
	FntProjFunc OldProject;
	InterpreterFunc Interpreter;
    GetCVTEFunc GetCVTEntry;
    GetSWidFunc GetSingleWidth;

	jmp_buf	env;		/* always be at the end, since it is unknown size */

} fnt_LocalGraphicStateType;

/*
 * Executes the font instructions.
 * This is the external interface to the interpreter.
 *
 * Parameter Description
 *
 * elements points to the character elements. Element 0 is always
 * reserved and not used by the actual character.
 *
 * ptr points at the first instruction.
 * eptr points to right after the last instruction
 *
 * globalGS points at the global graphics state
 *
 * TraceFunc is pointer to a callback functioned called with a pointer to the
 *		local graphics state if TraceFunc is not null. The call is made just before
 *		every instruction is executed.
 *
 * Note: The stuff globalGS is pointing at must remain intact
 *       between calls to this function.
 */

GLOBAL_PROTO int fnt_Execute(fnt_ElementType *elements, uint8 *ptr, uint8 *eptr,
                             fnt_GlobalGraphicStateType *globalGS, fs_FuncType TraceFunc);

/*
 * Init routine, to be called at boot time.
 */

GLOBAL_PROTO void fnt_Init(fnt_GlobalGraphicStateType *globalGS);

GLOBAL_PROTO F26Dot6 fnt_RoundToGrid(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif	/* FNT_H */

/*************** Revision Control Information **********************************
 * 
 * $Header: R:/src/FontFusion/Source/Core/rcs/fnt.h 1.9 2001/05/02 17:20:23 reggers Exp $                                                                       *
 * 
 * $Log: fnt.h $
 * Revision 1.9  2001/05/02 17:20:23  reggers
 * SEAT BELT mode added (Sampo)
 * Revision 1.8  2001/04/27 21:14:08  reggers
 * Changed a manifest from DEBUG to XDEBUG that was causing compiler
 * problems in debug mode compilations on Studio.
 * Revision 1.7  1999/12/09 21:17:31  reggers
 * Sampo: multiple TrueType compatibility enhancements (scan convereter).
 * Revision 1.6  1999/11/11 16:44:30  reggers
 * Removed the UNUSED macro. This is defined at the user level
 * and should not be subject to redefinition errors.
 * Revision 1.5  1999/10/19 16:22:55  shawn
 * Changed UNUSED() macro to '((void)(x))'.
 * 
 * Revision 1.4  1999/09/30 15:11:12  jfatal
 * Added correct Copyright notice.
 * Revision 1.3  1999/07/23 16:46:09  sampo
 * Added typedef's to make forward references to fntGlobal and fntLocal
 * GraphicsState's compile without warnings.
 * Revision 1.2  1999/05/17 15:56:49  reggers
 * Inital Revision
 * Revision 14.1  1997/12/17 14:39:30  shawn
 * Wrapped the header file in a FNT_H macro.
 * 
 * Revision 14.0  1997/03/17 17:53:28  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.2  96/07/02  13:47:50  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.1  95/04/11  12:23:52  roberte
 * Moved some fnt.c local macros to that file.
 * 
 * Revision 10.0  95/02/15  16:22:51  roberte
 * Release
 * 
 * Revision 9.2  95/01/11  13:26:15  shawn
 * Changed IFntFunc typedef to FntFunc
 * Defined typedefs for GetCVTEntry, GetSingleWidth, TraceFuncType and ScaleFuncType
 * Changed TraceFunc parameters to type TraceFuncType
 * Changed traceFunc parameter in GLOBAL_PROTO for fnt_Execute() to type TraceFuncType
 * 
 * Revision 9.1  95/01/04  16:32:20  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:24:10  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:28:47  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:57:06  roberte
 * Release
 * 
 * Revision 6.92  94/03/18  14:00:33  roberte
 * Got rid of nested comment.
 * 
 * Revision 6.91  93/08/30  14:50:33  roberte
 * Release
 * 
 * Revision 6.44  93/03/15  13:08:24  roberte
 * Release
 * 
 * Revision 6.3  92/11/19  15:45:51  roberte
 * Release
 * 
 * Revision 6.2  92/04/30  11:22:06  leeann
 * take out non-Ascii bytes
 * 
 * Revision 6.1  91/08/14  16:45:08  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:26:20  mark
 * Release
 * 
 * Revision 4.3  91/08/07  11:50:12  mark
 * remove rcsstatus string
 * 
 * Revision 4.2  91/08/07  11:39:15  mark
 * added RCS control strings
 *                                                                           *
******************************************************************************/
