/*
 * FNT.c
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

/* #define D_Stack */

/*
 * This module contains the interpreter that executes font instructions
 *
 * The BASS Project Interpreter and Font Instruction Set sub ERS contains
 * relevant information.
 *
 * (c) Apple Computer Inc. 1987, 1988, 1989, 1990.
 *
 * History:
 * Work on this module began in the fall of 1987
 *
 * Written June 23, 1988 by Sampo Kaasila
 *
 * Rewritten October 18, 1988 by Sampo Kaasila. Added a jump table instead of the
 * switch statement. Also added CALL(), LOOPCALL(), FDEF(), ENDF(), and replaced
 * WX(),WY(), RX(), RY(), MD() with new WC(), RC(), MD(). Reimplemented IUP(). Also
 * optimized the code somewhat. Cast code into a more digestable form using a local
 * and global graphics state.
 *
 * December 20, 1988. Added DELTA(), SDB(), SDS(), and deleted ONTON(), and
 * ONTOFF(). ---Sampo.
 * January 17, 1989 Added DEBUG(), RAW(), RLSB(), WLSB(), ALIGNPTS(), SANGW(), AA().
 *			    Brought up this module to an alpha ready state. ---Sampo
 *
 * January 31, 1989 Added RDTG(), and RUTG().
 *
 * Feb 16, 1989 Completed final bug fixes for the alpha release.
 *
 * March 21, 1989 Fixed a small Twilight Zone bug in MIAP(), MSIRP(), and MIRP().
 *
 * March 28, 1989 Took away the need and reason for directional switches in the
 * control value table. However, this forced a modification of all the code that
 * reads or writes to the control value table and the single width. Also WCVT was
 * replaced by WCVTFOL, WCVTFOD, and WCVTFCVT. ---Sampo
 *
 * April 17, 1989 Modified RPV(), RFV(), WPV(), WFV() to work with an x & y pair
 * instead of just x. --- Sampo
 *
 * April 25, 1989 Fixed bugs in CEILING(), RUTG(), FDEF(), and IF(). Made MPPEM() a
 * function of the projection vector. ---Sampo
 *
 * June 7, 1989 Made ALIGNRP() dependent on the loop variable, and also made it
 * blissfully ignorant of the twilight zone. Also, added ROFF(), removed WCVTFCVT()
 * , renamed WCVTFOL() to WCVT(), made MIRP() and MDRP() compensate for the engine
 * even when there is no rounding. --- Sampo
 *
 * June 8, 1989 Made DELTA() dependent on the Transformation. ---Sampo
 *
 * June 19, 1989 Added JROF() and JROT(). ---Sampo
 *
 * July 14, 1989 Forced a pretty tame behaviour when abs((projection vector) *
 * (freedoom vector)) < 1/16. The old behaviour was to grossly blow up the outline.
 * This situation may happen for low pointsizes when the character is severly
 * distorted due to the gridfitting ---Sampo
 *
 * July 17, 1989 Prevented the rounding routines from changing the sign of a
 * quantity due to the engine compensation. ---Sampo
 *
 * July 19, 1989 Increased nummerical precision of fnt_MovePoint by 8 times.
 * ---Sampo
 *
 * July 28, 1989 Introduced 5 more Delta instructions. (Now 3 are for points and 3
 * for the cvt.) ---Sampo
 *
 * Aug 24, 1989 fixed fnt_GetCVTEntrySlow and fnt_GetSingleWidthSlow bug ---Sampo
 *
 * Sep 26, 1989 changed rounding routines, so that the sign flip check only apply
 * if the input value is nonzero. ---Sampo
 *
 * Oct 26, 1989 Fixed small bugs/features in fnt_RoundUpToGrid() and
 * fnt_RoundToHalfGrid. Added SROUND() and S45ROUND(). ---Sampo
 *
 * Oct 31, 1989 Fixed transformation bug in fnt_MPPEM, fnt_DeltaEngine,
 * fnt_GetCvtEntrySlow, fnt_GetSingleWidthSlow. ---Sampo
 *
 * Nov 3, 1989 Added FLIPPT(), FLIPRGON(), FLIPRGOFF(). ---Sampo
 *
 * Nov 16, 1989 Merged back in lost Nov 3 changes.---Sampo
 *
 * Dec 2, 1989 Added READMETRICS() aand WRITEMETRICS(). --- Sampo
 *
 * Jan 8, 1990 Added SDPVTL(). --- Sampo
 *
 * Jan 8, 1990 Eliminated bug in SFVTPV[] ( old bug ) and SFVTL[] (showed up
 * because of SDPVTL[]). --- Sampo
 *
 * Jan 9, 1990 Added the SCANCTRL[] instruction. --- Sampo
 *
 * Jan 24, 1990 Fixed bug in fnt_ODD and fnt_EVEN. ---Sampo
 *
 * Jan 28, 1990 Fixed bug in fnt_Normalize. --- Sampo
 *
 * 2/9/90	mrr	ReFixed Normalize bug, added ELSE and JMPR.  Added pgmList[] to
 * globalGS in preparation for 3 preprograms.  affected CALL, LOOPCALL, FDEF
 * 2/21/90	mrr	Added RMVT, WMVT.
 * 3/7/90	mrr		put in high precision versions of MUL and DIV.
 */



/*
#include "spdo_prv.h"
#if PROC_TRUETYPE
#include "fino.h"
*/

/* TEMPORARY */

#define PROC_TRUETYPE
#ifdef PROC_TRUETYPE

/* T2K */
#include "syshead.h"
#include "dtypes.h"
#include "config.h"
#include "tsimem.h"
#include "util.h"

#ifdef ENABLE_NATIVE_TT_HINTS


/** FontScalerUs Includes **/
/* #include "fscdefs.h" */
/* #include "fontscal.h" */
/* #include "fontmath.h" */
/* #include "sc.h"*/
#include "fnt.h"
/* #include "fserror.h" */


/****** Macros *******/

/* T2K */
/* #define T2K_MATH */
#define T2K_MATH
#ifdef T2K_MATH



#define FixDiv util_FixDiv
#define FixMul util_FixMul

#ifdef VECTORMULDIV
#undef VECTORMULDIV
#endif
/* +4 seems more compatible than than both +0 and +8 */
#define VECTORMULDIV(a,b,c)			((util_FixMul((a), util_FixDiv((b)<<4,(c)) ) + 4) >> 4);

/* #define VECTORMULDIV(a,b,c)			util_FixMul(a, util_FixDiv(b,c) ) */


#ifdef VECTORDOT
#undef VECTORDOT
#endif
#define VECTORDOT(a,b) ((SHORTMUL((a),(b)) + (1 << 13)) >> 14)

#ifdef VECTORMUL
#undef VECTORMUL
#endif
#define VECTORMUL(a,b) util_ShortFracMul(a,b)



#endif /* T2K_MATH */

/* Need:  Div26Dot6, Mul26Dot6,  */
/*
#define VECTORMUL(value, component)	ShortFracMul(value, component)
#define VECTORDOT(a,b)				ShortFracDot(a,b)
#define VECTORMULDIV(a,b,c)			ShortMulDiv(a,b,c)
*/

#define POP( p )     ( *(--p) )
#define PUSH( p, x ) ( *(p)++ = (x) )

#define BADCOMPILER

#ifdef BADCOMPILER
#define BOOLEANPUSH( p, x ) PUSH( p, ((x) ? 1 : 0) ) /* MPW 3.0 */
#else
#define BOOLEANPUSH( p, x ) PUSH( p, x )
#endif


#define MAX(a,b)	((a) > (b) ? (a) : (b))

#ifdef D_Stack
btsbool examineStack = FALSE;
#endif


#ifdef DEBUG_FNT
GLOBAL_PROTO void CHECK_RANGE(int32 n, int32 min, int32 max);

FUNCTION void CHECK_RANGE(int32 n, int32 min, int32 max)
{
	if (n > max || n < min) {
		Debugger();
	}
}

GLOBAL_PROTO void CHECK_ASSERTION(int expression);

FUNCTION void CHECK_ASSERTION(int expression)
{
	if (!expression) {
		Debugger();
	}
}

GLOBAL_PROTO void CHECK_CVT(fnt_LocalGraphicStateType *pGS, int cvt);

FUNCTION void CHECK_CVT(fnt_LocalGraphicStateType *pGS, int cvt)
{
	CHECK_RANGE(cvt, 0, pGS->globalGS->cvtCount-1);
}

GLOBAL_PROTO void CHECK_FDEF(fnt_LocalGraphicStateType *pGS, int fdef);

FUNCTION void CHECK_FDEF(fnt_LocalGraphicStateType *pGS, int fdef)
{
	CHECK_RANGE(fdef, 0, pGS->globalGS->maxp->maxFunctionDefs-1);
}

GLOBAL_PROTO void CHECK_PROGRAM(int pgmIndex);

FUNCTION void CHECK_PROGRAM(int pgmIndex)
{
	CHECK_RANGE(pgmIndex, 0, MAXPREPROGRAMS-1);
}

GLOBAL_PROTO void CHECK_ELEMENT(fnt_LocalGraphicStateType *pGS, int elem);

FUNCTION void CHECK_ELEMENT(fnt_LocalGraphicStateType *pGS, int elem)
{
	CHECK_RANGE(elem, 0, pGS->globalGS->maxp->maxElements-1);
}

GLOBAL_PROTO void CHECK_ELEMENTPTR(fnt_LocalGraphicStateType *pGS, fnt_ElementType* elem);

FUNCTION void CHECK_ELEMENTPTR(fnt_LocalGraphicStateType *pGS, fnt_ElementType* elem)
{
	if (elem == &pGS->elements[1])
	{
		int maxctrs, maxpts;

		maxctrs = MAX(pGS->globalGS->maxp->maxContours,
					  pGS->globalGS->maxp->maxCompositeContours);
		maxpts  = MAX(pGS->globalGS->maxp->maxPoints,
					  pGS->globalGS->maxp->maxCompositePoints);

		CHECK_RANGE(elem->nc, 1, maxctrs);
		CHECK_RANGE(elem->ep[elem->nc-1], 0, maxpts-1);
	}
	else
		if (elem != &pGS->elements[0])
		{
			Debugger();
		}
}

GLOBAL_PROTO void CHECK_STORAGE(fnt_LocalGraphicStateType *pGS, int index);

FUNCTION void CHECK_STORAGE(fnt_LocalGraphicStateType *pGS, int index)
{
	CHECK_RANGE(index, 0, pGS->globalGS->maxp->maxStorage-1);
}

GLOBAL_PROTO void CHECK_STACK(fnt_LocalGraphicStateType *pGS);

FUNCTION void CHECK_STACK(fnt_LocalGraphicStateType *pGS)
{
	CHECK_RANGE(pGS->stackPointer - pGS->globalGS->stackBase,
				0,
				pGS->globalGS->maxp->maxStackElements-1);
}

GLOBAL_PROTO void CHECK_POINT(fnt_LocalGraphicStateType *pGS, fnt_ElementType* elem, int pt);

FUNCTION void CHECK_POINT(fnt_LocalGraphicStateType *pGS, fnt_ElementType* elem, int pt)
{
	CHECK_ELEMENTPTR(pGS, elem);
	if (pGS->elements == elem)
		CHECK_RANGE(pt, 0, pGS->globalGS->maxp->maxTwilightPoints - 1);
	else
		CHECK_RANGE(pt, 0, elem->ep[elem->nc-1] + 2);	/* phantom points */
}

GLOBAL_PROTO void CHECK_CONTOUR(fnt_LocalGraphicStateType *pGS, fnt_ElementType* elem, int ctr);

FUNCTION void CHECK_CONTOUR(fnt_LocalGraphicStateType *pGS, fnt_ElementType* elem, int ctr)
{
	CHECK_ELEMENTPTR(pGS, elem);
	CHECK_RANGE(ctr, 0, elem->nc - 1);
}

#define CHECK_POP(LocalGS, s)		POP(s)
#define CHECK_PUSH(LocalGS, s, v)	PUSH(s, v)
#else
#define CHECK_RANGE(a,b,c)
#define CHECK_ASSERTION(a)
#define CHECK_CVT(a,b)
#define CHECK_POINT(a,b,c)
#define CHECK_CONTOUR(a,b,c)
#define CHECK_FDEF(a,b)
#define CHECK_PROGRAM(a)
#define CHECK_ELEMENT(a,b)
#define CHECK_ELEMENTPTR(a,b)
#define CHECK_STORAGE(a,b)
#define CHECK_STACK(a)
#define CHECK_POP(LocalGS, s)		POP(s)
#define CHECK_PUSH(LocalGS, s, v)	PUSH(s, v)
#endif

#define GETBYTE(ptr)	( (uint8)*ptr++ )
#define MABS(x)			( (x) < 0 ? (-(x)) : (x) )

/* These have been renamed from the TDIS BIT0 - BIT7 macros
	to avoid confusion. */

#ifdef TT_BIT0
#undef TT_BIT0
#endif
#ifdef TT_BIT1
#undef TT_BIT1
#endif
#ifdef TT_BIT2
#undef TT_BIT2
#endif
#ifdef TT_BIT3
#undef TT_BIT3
#endif
#ifdef TT_BIT4
#undef TT_BIT4
#endif
#ifdef TT_BIT5
#undef TT_BIT5
#endif
#ifdef TT_BIT6
#undef TT_BIT6
#endif
#ifdef TT_BIT7
#undef TT_BIT7
#endif

#define TT_BIT0( t ) ( (t) & 0x01 )
#define TT_BIT1( t ) ( (t) & 0x02 )
#define TT_BIT2( t ) ( (t) & 0x04 )
#define TT_BIT3( t ) ( (t) & 0x08 )
#define TT_BIT4( t ) ( (t) & 0x10 )
#define TT_BIT5( t ) ( (t) & 0x20 )
#define TT_BIT6( t ) ( (t) & 0x40 )
#define TT_BIT7( t ) ( (t) & 0x80 )

/******** 12 BinaryOperators **********/
#define LT_CODE		0x50
#define LTEQ_CODE	0x51
#define GT_CODE		0x52
#define GTEQ_CODE	0x53
#define EQ_CODE		0x54
#define NEQ_CODE	0x55
#define AND_CODE	0x5A
#define OR_CODE		0x5B
#define ADD_CODE	0x60
#define SUB_CODE	0x61
#define DIV_CODE	0x62
#define MUL_CODE	0x63
#define MAX_CODE	0x8b
#define MIN_CODE	0x8c

/******** 9 UnaryOperators **********/
#define ODD_CODE		0x56
#define EVEN_CODE		0x57
#define NOT_CODE		0x5C
#define ABS_CODE		0x64
#define NEG_CODE		0x65
#define FLOOR_CODE		0x66
#define CEILING_CODE	0x67

/******** 6 RoundState Codes **********/
#define RTG_CODE		0x18
#define RTHG_CODE		0x19
#define RTDG_CODE		0x3D
#define ROFF_CODE		0x7A
#define RUTG_CODE		0x7C
#define RDTG_CODE		0x7D

/****** LocalGS Codes *********/
#define POP_CODE	0x21
#define SRP0_CODE	0x10
#define SRP1_CODE	0x11
#define SRP2_CODE	0x12
#define LLOOP_CODE	0x17
#define LMD_CODE	0x1A

/****** Element Codes *********/
#define SCE0_CODE	0x13
#define SCE1_CODE	0x14
#define SCE2_CODE	0x15
#define SCES_CODE	0x16

/****** Control Codes *********/
#define IF_CODE		0x58
#define ELSE_CODE	0x1B
#define EIF_CODE	0x59
#define ENDF_CODE	0x2d
#define MD_CODE		0x49

/* flags for UTP, IUP, MovePoint */
#define XMOVED 0x01
#define YMOVED 0x02

#ifdef SEGMENT_LINK
/* #pragma segment FNT_C */
#endif



/* Define the macros for the function arguments here */

#if REENTRANT_ALLOC
#define LocalGS (*pLocalGS)
#define GSPPARAM0 pLocalGS
#define GSPPARAM1 GSPPARAM0,
#define GSPDECL register fnt_LocalGraphicStateType *pLocalGS;
#else
fnt_LocalGraphicStateType LocalGS;
#define GSPPARAM0
#define GSPPARAM1
#define GSPDECL
#endif

/* Public function prototypes */

GLOBAL_PROTO F26Dot6 fnt_RoundToDoubleGrid(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);
GLOBAL_PROTO F26Dot6 fnt_RoundDownToGrid(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);
GLOBAL_PROTO F26Dot6 fnt_RoundUpToGrid(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);
/* GLOBAL_PROTO F26Dot6 fnt_RoundToGrid(GSP_DECL1 F26Dot6 xin, F26Dot6 engine); */
GLOBAL_PROTO F26Dot6 fnt_RoundToHalfGrid(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);
GLOBAL_PROTO F26Dot6 fnt_RoundOffSLOW(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);
GLOBAL_PROTO F26Dot6 fnt_SuperRound(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);
GLOBAL_PROTO F26Dot6 fnt_Super45Round(GSP_DECL1 F26Dot6 xin, F26Dot6 engine);


GLOBAL_PROTO void fnt_Panic(GSP_DECL1 int error);

LOCAL_PROTO void fnt_IllegalInstruction(GSP_DECL0);
LOCAL_PROTO void fnt_Normalize(F26Dot6 x, F26Dot6 y, VECTOR *v);
LOCAL_PROTO void fnt_MovePoint(GSP_DECL1 fnt_ElementType *element,
				   ArrayIndex point,
				   F26Dot6 delta);
LOCAL_PROTO void fnt_XMovePoint(GSP_DECL1 fnt_ElementType *element,
					ArrayIndex point,
					F26Dot6 delta);
LOCAL_PROTO void fnt_YMovePoint(GSP_DECL1 fnt_ElementType *element,
					ArrayIndex point,
					F26Dot6 delta);
LOCAL_PROTO F26Dot6 fnt_Project(GSP_DECL1 F26Dot6 x,
					F26Dot6 y);
LOCAL_PROTO F26Dot6 fnt_OldProject(GSP_DECL1 F26Dot6 x,
					   F26Dot6 y);
LOCAL_PROTO F26Dot6 fnt_XProject(GSP_DECL1 F26Dot6 x, F26Dot6 y);
LOCAL_PROTO F26Dot6 fnt_YProject(GSP_DECL1 F26Dot6 x, F26Dot6 y);
LOCAL_PROTO Fixed fnt_GetCVTScale(GSP_DECL0);
LOCAL_PROTO F26Dot6 fnt_GetCVTEntryFast(GSP_DECL1 ArrayIndex n);
LOCAL_PROTO F26Dot6 fnt_GetCVTEntrySlow(GSP_DECL1 ArrayIndex n);
LOCAL_PROTO F26Dot6 fnt_GetSingleWidthFast(GSP_DECL0);
LOCAL_PROTO F26Dot6 fnt_GetSingleWidthSlow(GSP_DECL0);
LOCAL_PROTO void fnt_ChangeCvt(GSP_DECL1 fnt_ElementType *element,
				   ArrayIndex number,
				   F26Dot6 delta);
LOCAL_PROTO void fnt_InnerTraceExecute(GSP_DECL1 uint8 *ptr, uint8 *eptr);
LOCAL_PROTO void fnt_InnerExecute(GSP_DECL1 uint8 *ptr, uint8 *eptr);
LOCAL_PROTO void fnt_Check_PF_Proj(GSP_DECL0);
LOCAL_PROTO void fnt_ComputeAndCheck_PF_Proj(GSP_DECL0);
LOCAL_PROTO Fract fnt_QuickDist(Fract dx, Fract dy);
LOCAL_PROTO void fnt_SetRoundValues(GSP_DECL1 int arg1, int normalRound);
LOCAL_PROTO F26Dot6 fnt_CheckSingleWidth(GSP_DECL1 F26Dot6 value);
LOCAL_PROTO fnt_instrDef* fnt_FindIDef(GSP_DECL1 uint8 opCode);
LOCAL_PROTO void fnt_DeltaEngine(GSP_DECL1 FntMoveFunc doIt,
					 int16 base,
					 int16 shift);
LOCAL_PROTO void fnt_DefaultJumpTable(FntFunc *function);

/* Actual instructions for the jump table */
LOCAL_PROTO void fnt_SVTCA_0(GSP_DECL0);
LOCAL_PROTO void fnt_SVTCA_1(GSP_DECL0);
LOCAL_PROTO void fnt_SPVTCA(GSP_DECL0);
LOCAL_PROTO void fnt_SFVTCA(GSP_DECL0);
LOCAL_PROTO void fnt_SPVTL(GSP_DECL0);
LOCAL_PROTO void fnt_SDPVTL(GSP_DECL0);
LOCAL_PROTO void fnt_SFVTL(GSP_DECL0);
LOCAL_PROTO void fnt_WPV(GSP_DECL0);
LOCAL_PROTO void fnt_WFV(GSP_DECL0);
LOCAL_PROTO void fnt_RPV(GSP_DECL0);
LOCAL_PROTO void fnt_RFV(GSP_DECL0);
LOCAL_PROTO void fnt_SFVTPV(GSP_DECL0);
LOCAL_PROTO void fnt_ISECT(GSP_DECL0);
LOCAL_PROTO void fnt_SetLocalGraphicState(GSP_DECL0);
LOCAL_PROTO void fnt_SetElementPtr(GSP_DECL0);
LOCAL_PROTO void fnt_SetRoundState(GSP_DECL0);
LOCAL_PROTO void fnt_SROUND(GSP_DECL0);
LOCAL_PROTO void fnt_S45ROUND(GSP_DECL0);
LOCAL_PROTO void fnt_LMD(GSP_DECL0);
LOCAL_PROTO void fnt_RAW(GSP_DECL0);
/*LOCAL_PROTO void fnt_WLSB(GSP_DECL0);*/
LOCAL_PROTO void fnt_LWTCI(GSP_DECL0);
LOCAL_PROTO void fnt_LSWCI(GSP_DECL0);
LOCAL_PROTO void fnt_LSW(GSP_DECL0);
LOCAL_PROTO void fnt_DUP(GSP_DECL0);
/*LOCAL_PROTO void fnt_POP(GSP_DECL0);*/
LOCAL_PROTO void fnt_CLEAR(GSP_DECL0);
LOCAL_PROTO void fnt_SWAP(GSP_DECL0);
LOCAL_PROTO void fnt_DEPTH(GSP_DECL0);
LOCAL_PROTO void fnt_CINDEX(GSP_DECL0);
LOCAL_PROTO void fnt_MINDEX(GSP_DECL0);
LOCAL_PROTO void fnt_ROTATE(GSP_DECL0);
LOCAL_PROTO void fnt_MDAP(GSP_DECL0);
LOCAL_PROTO void fnt_MIAP(GSP_DECL0);
LOCAL_PROTO void fnt_IUP(GSP_DECL0);
LOCAL_PROTO void fnt_SHP(GSP_DECL0);
LOCAL_PROTO void fnt_SHC(GSP_DECL0);
LOCAL_PROTO void fnt_SHE(GSP_DECL0);
LOCAL_PROTO void fnt_SHPIX(GSP_DECL0);
LOCAL_PROTO void fnt_IP(GSP_DECL0);
LOCAL_PROTO void fnt_MSIRP(GSP_DECL0);
LOCAL_PROTO void fnt_ALIGNRP(GSP_DECL0);
LOCAL_PROTO void fnt_ALIGNPTS(GSP_DECL0);
LOCAL_PROTO void fnt_SANGW(GSP_DECL0);
LOCAL_PROTO void fnt_FLIPPT(GSP_DECL0);
LOCAL_PROTO void fnt_FLIPRGON(GSP_DECL0);
LOCAL_PROTO void fnt_FLIPRGOFF(GSP_DECL0);
LOCAL_PROTO void fnt_SCANCTRL(GSP_DECL0);
LOCAL_PROTO void fnt_SCANTYPE(GSP_DECL0);
LOCAL_PROTO void fnt_INSTCTRL(GSP_DECL0);
LOCAL_PROTO void fnt_AA(GSP_DECL0);
LOCAL_PROTO void fnt_NPUSHB(GSP_DECL0);
LOCAL_PROTO void fnt_NPUSHW(GSP_DECL0);
LOCAL_PROTO void fnt_WS(GSP_DECL0);
LOCAL_PROTO void fnt_RS(GSP_DECL0);
LOCAL_PROTO void fnt_WCVT(GSP_DECL0);
LOCAL_PROTO void fnt_WCVTFOD(GSP_DECL0);
LOCAL_PROTO void fnt_RCVT(GSP_DECL0);
LOCAL_PROTO void fnt_RC(GSP_DECL0);
LOCAL_PROTO void fnt_WC(GSP_DECL0);
LOCAL_PROTO void fnt_MD(GSP_DECL0);
LOCAL_PROTO void fnt_MPPEM(GSP_DECL0);
LOCAL_PROTO void fnt_MPS(GSP_DECL0);
LOCAL_PROTO void fnt_GETINFO(GSP_DECL0);
LOCAL_PROTO void fnt_FLIPON(GSP_DECL0);
LOCAL_PROTO void fnt_FLIPOFF(GSP_DECL0);

#ifndef NOT_ON_THE_MAC
#ifdef DEBUG
LOCAL_PROTO void fnt_DDT(int8 c, int32 n);
#endif
#endif

LOCAL_PROTO void fnt_DEBUG(GSP_DECL0);
LOCAL_PROTO void fnt_SkipPushCrap(GSP_DECL0);
LOCAL_PROTO void fnt_IF(GSP_DECL0);
LOCAL_PROTO void fnt_ELSE(GSP_DECL0);
LOCAL_PROTO void fnt_EIF(GSP_DECL0);
LOCAL_PROTO void fnt_JMPR(GSP_DECL0);
LOCAL_PROTO void fnt_JROT(GSP_DECL0);
LOCAL_PROTO void fnt_JROF(GSP_DECL0);
LOCAL_PROTO void fnt_BinaryOperand(GSP_DECL0);
LOCAL_PROTO void fnt_UnaryOperand(GSP_DECL0);
LOCAL_PROTO void fnt_ROUND(GSP_DECL0);
LOCAL_PROTO void fnt_NROUND(GSP_DECL0);
LOCAL_PROTO void fnt_PUSHB(GSP_DECL0);
LOCAL_PROTO void fnt_PUSHW(GSP_DECL0);
LOCAL_PROTO void fnt_MDRP(GSP_DECL0);
LOCAL_PROTO void fnt_MIRP(GSP_DECL0);
LOCAL_PROTO void fnt_CALL(GSP_DECL0);
LOCAL_PROTO void fnt_FDEF(GSP_DECL0);
LOCAL_PROTO void fnt_LOOPCALL(GSP_DECL0);
LOCAL_PROTO void fnt_IDefPatch(GSP_DECL0);
LOCAL_PROTO void fnt_IDEF(GSP_DECL0);
LOCAL_PROTO void fnt_UTP(GSP_DECL0);
LOCAL_PROTO void fnt_SDB(GSP_DECL0);
LOCAL_PROTO void fnt_SDS(GSP_DECL0);
LOCAL_PROTO void fnt_DELTAP1(GSP_DECL0);
LOCAL_PROTO void fnt_DELTAP2(GSP_DECL0);
LOCAL_PROTO void fnt_DELTAP3(GSP_DECL0);
LOCAL_PROTO void fnt_DELTAC1(GSP_DECL0);
LOCAL_PROTO void fnt_DELTAC2(GSP_DECL0);
LOCAL_PROTO void fnt_DELTAC3(GSP_DECL0);


/*
 * We exit through here, when we detect serious errors.
 */

FUNCTION void fnt_Panic(GSP_DECL1 int error)
{
	longjmp( LocalGS.env, error ); /* Do a gracefull recovery  */
}


/***************************/

#define fnt_NextPt1(pt) ((pt) == ep_ctr ? sp_ctr : pt+1)

/*
 * Illegal instruction panic
 */

FUNCTION LOCAL_PROTO void fnt_IllegalInstruction(GSP_DECL0)
{
	fnt_Panic( GSPPARAM1 UNDEFINED_INSTRUCTION_ERR );
}


#ifdef OLD
FUNCTION LOCAL_PROTO int bitcount(uint32 a)
{
	int count = 0;
	while (a) {
		a >>= 1;
		count++;
	}
	return count;
}
#endif


FUNCTION LOCAL_PROTO void fnt_Normalize(F26Dot6 x, F26Dot6 y, VECTOR *v)
{
	F16Dot16 length;
	
#ifdef OLD_VERSION
	/*
	 *	Since x and y are 26.6, and currently that means they are really 16.6,
	 *	when treated as Fract, they are 0.[8]22, so shift up to 0.30 for accuracy
	 */

	CHECK_RANGE(x, -32768L << 6, 32767L << 6);
	CHECK_RANGE(y, -32768L << 6, 32767L << 6);

	{
		Fract xx = x;
		Fract yy = y;
		int shift;
		if (xx < 0)	xx = -xx;
		if (yy < 0) yy = -yy;
		if (xx < yy) xx = yy;
		/*
		 *	0.5 <= max(x,y) < 1
		 */
		shift = (int)(8 * sizeof(Fract) - 2 - bitcount((uint32)xx));
		x <<= shift;
		y <<= shift;
	}
	{
		Fract length = FracSqrt( FracMul( x, x ) + FracMul( y, y ) );
		v->x = FIXROUND( FracDiv( x, length ) );
		v->y = FIXROUND( FracDiv( y, length ) );
	}
#endif /* OLD_VERSION */
	/* New version Dec 28, 1998 ---Sampo to gain speed and eliminate TT math library FracSqrt dependency */
	x <<= 10;	/* convert to 16.16 */
	y <<= 10;
	length = util_EuclidianDistance( x, y );
	if (length != 0)
	{/* zero test added 3/29/00 */
		x = util_FixDiv( x, length );
		y = util_FixDiv( y, length );
	}

	x = (x+1) >> 2; /* Convert to 2.14, +1 seems more compatible than both +0 and +2 */
	y = (y+1) >> 2;
	v->x = (ShortFrac)x;
	v->y = (ShortFrac)y;
}


/******************** BEGIN Rounding Routines ***************************/

/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_RoundToDoubleGrid(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
/* #pragma unused(gs) */
	register F26Dot6 x = xin;
	
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	if ( x >= 0 )
	{
		x += engine;
		x += fnt_pixelSize/4;
		x &= ~(fnt_pixelSize/2-1);
	}
	else
	{
		x = -x;
		x += engine;
		x += fnt_pixelSize/4;
		x &= ~(fnt_pixelSize/2-1);
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = 0; /* The sign flipped, make zero */
	}

	return x;
}


/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_RoundDownToGrid(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;

	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	if ( x >= 0 )
	{
		x += engine;
		x &= ~(fnt_pixelSize-1);
	}
	else
	{
		x = -x;
		x += engine;
		x &= ~(fnt_pixelSize-1);
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = 0; /* The sign flipped, make zero */
	}

	return x;
}


/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_RoundUpToGrid(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;

	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	if ( x >= 0 )
	{
		x += engine;
		x += fnt_pixelSize - 1;
		x &= ~(fnt_pixelSize-1);
	}
	else
	{
		x = -x;
		x += engine;
		x += fnt_pixelSize - 1;
		x &= ~(fnt_pixelSize-1);
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = 0; /* The sign flipped, make zero */
	}

	return x;
}


/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_RoundToGrid(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;

	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	if ( x >= 0 )
	{
		x += engine;
		x += fnt_pixelSize/2;
		x &= ~(fnt_pixelSize-1);
	}
	else
	{
		x = -x;
		x += engine;
		x += fnt_pixelSize/2;
		x &= ~(fnt_pixelSize-1);
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = 0; /* The sign flipped, make zero */
	}

	return x;
}



/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_RoundToHalfGrid(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;

	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	if ( x >= 0 )
	{
		x += engine;
		x &= ~(fnt_pixelSize-1);
		x += fnt_pixelSize/2;
	}
	else
	{
		x = -x;
		x += engine;
		x &= ~(fnt_pixelSize-1);
		x += fnt_pixelSize/2;
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = xin > 0 ? fnt_pixelSize/2 : -fnt_pixelSize/2; /* The sign flipped, make equal to smallest valid value */
	}

	return x;
}


/*
 * Internal rounding routine
 */
/*#define fnt_RoundOff( x, e ) (x)*/

FUNCTION F26Dot6 fnt_RoundOffSLOW(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	UNUSED(engine);
	return xin;
}

#ifdef OLD 
FUNCTION F26Dot6 fnt_RoundOff(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;

	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	if ( x >= 0 )
	{
		x += engine;
	}
	else
	{
		x -= engine;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin)
	{
		x = 0; /* The sign flipped, make zero */
	}

	return x;
}
#endif

/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_SuperRound(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	if ( x >= 0 )
	{
		x += engine;
		x += pb->threshold - pb->phase;
		x &= pb->periodMask;
		x += pb->phase;
	}
	else
	{
		x = -x;
		x += engine;
		x += pb->threshold - pb->phase;
		x &= pb->periodMask;
		x += pb->phase;
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = xin > 0 ? pb->phase : -pb->phase; /* The sign flipped, make equal to smallest phase */
	}

	return x;
}


/*
 * Internal rounding routine
 */

FUNCTION F26Dot6 fnt_Super45Round(GSP_DECL1 register F26Dot6 xin, F26Dot6 engine)
{
	register F26Dot6 x = xin;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	if ( x >= 0 )
	{
		x += engine;
		x += pb->threshold - pb->phase;
		/* x = FracDiv( x, pb->period45 ); */
		x = util_FixDiv( x, pb->period45 >> 14 );
		x  &= ~(fnt_pixelSize-1);
		/* x = FracMul( x, pb->period45 ); */
		x = util_FixMul( x, pb->period45 >>14 );
		x += pb->phase;
	}
	else
	{
		x = -x;
		x += engine;
		x += pb->threshold - pb->phase;
		/* x = FracDiv( x, pb->period45 ); */
		x = util_FixDiv( x, pb->period45 >> 14 );
		x  &= ~(fnt_pixelSize-1);
		/* x = FracMul( x, pb->period45 ); */
		x = util_FixMul( x, pb->period45 >>14 );
		x += pb->phase;
		x = -x;
	}

	if ( ((int32)(xin ^ x)) < 0 && xin )
	{
		x = xin > 0 ? pb->phase : -pb->phase; /* The sign flipped, make equal to smallest phase */
	}

	return x;
}


/******************** END Rounding Routines ***************************/


/* 3-versions ************************************************************************/

/*
 * Moves the point in element by delta (measured against the projection vector)
 * along the freedom vector.
 */

FUNCTION LOCAL_PROTO void fnt_MovePoint(GSP_DECL1 register fnt_ElementType *element, register ArrayIndex point, register F26Dot6 delta)
{
	register VECTORTYPE pfProj = LocalGS.pfProj;
	register VECTORTYPE fx = LocalGS.free.x;
	register VECTORTYPE fy = LocalGS.free.y;

	CHECK_POINT( &LocalGS, element, point );

	if ( pfProj != ONEVECTOR )
	{
		if ( fx )
		{
			element->x[point] += VECTORMULDIV( delta, fx, pfProj );
			element->f[point] |= XMOVED;
		}
		if ( fy )
		{
			element->y[point] += VECTORMULDIV( delta, fy, pfProj );
			element->f[point] |= YMOVED;
		}
	}
	else
	{
		if ( fx )
		{
			element->x[point] += VECTORMUL( delta, fx );
			element->f[point] |= XMOVED;
		}
		if ( fy )
		{
			element->y[point] += VECTORMUL( delta, fy );
			element->f[point] |= YMOVED;
		}
	}
}


/*
 * For use when the projection and freedom vectors coincide along the x-axis.
 */

FUNCTION LOCAL_PROTO void fnt_XMovePoint(GSP_DECL1 fnt_ElementType *element, ArrayIndex point, register F26Dot6 delta)
{
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	CHECK_POINT( &LocalGS, element, point );
	element->x[point] += delta;
	element->f[point] |= XMOVED;
}

/*
 * For use when the projection and freedom vectors coincide along the y-axis.
 */

FUNCTION LOCAL_PROTO void fnt_YMovePoint(GSP_DECL1 fnt_ElementType *element, ArrayIndex point, F26Dot6 delta)
{
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	CHECK_POINT( &LocalGS, element, point );
	element->y[point] += delta;
	element->f[point] |= YMOVED;
}


/*
 * projects x and y into the projection vector.
 */

FUNCTION LOCAL_PROTO F26Dot6 fnt_Project(GSP_DECL1 F26Dot6 x, F26Dot6 y)
{
	return( VECTORMUL( x, LocalGS.proj.x ) + VECTORMUL( y, LocalGS.proj.y ) );
}

/*
 * projects x and y into the old projection vector.
 */

FUNCTION LOCAL_PROTO F26Dot6 fnt_OldProject(GSP_DECL1 F26Dot6 x, F26Dot6 y)
{
	return( VECTORMUL( x, LocalGS.oldProj.x ) + VECTORMUL( y, LocalGS.oldProj.y ) );
}

/*
 * Projects when the projection vector is along the x-axis
 */

FUNCTION LOCAL_PROTO F26Dot6 fnt_XProject(GSP_DECL1 F26Dot6 x, F26Dot6 y)
{
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	UNUSED(y);
	return( x );
}

/*
 * Projects when the projection vector is along the y-axis
 */

FUNCTION LOCAL_PROTO F26Dot6 fnt_YProject(GSP_DECL1 F26Dot6 x, F26Dot6 y)
{
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
	UNUSED(x);
	return( y );
}


/*************************************************************************/
/*** Compensation for Transformations ***/

/*
 * Internal support routine, keep this guy FAST!!!!!!!		<3>
 */

FUNCTION LOCAL_PROTO Fixed fnt_GetCVTScale(GSP_DECL0)
{
	register VECTORTYPE pvx, pvy;
	register Fixed scale;
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	/* Do as few Math routines as possible to gain speed */

	pvx = LocalGS.proj.x;
	pvy = LocalGS.proj.y;
	if ( pvy )
	{
		if ( pvx )
		{
			pvy = (VECTORTYPE)VECTORDOT( pvy, pvy );
			scale = VECTORMUL( globalGS->yStretch, pvy );
			pvx = (VECTORTYPE)VECTORDOT( pvx, pvx );
			return scale + VECTORMUL( globalGS->xStretch, pvx );
		}
		else	/* pvy == +1 or -1 */
			return globalGS->yStretch;
	}
	else	/* pvx == +1 or -1 */
		return globalGS->xStretch;
}


/*	Functions for function pointer in local graphic state
*/

FUNCTION LOCAL_PROTO F26Dot6 fnt_GetCVTEntryFast(GSP_DECL1 ArrayIndex n)
{
#ifdef USE_SEAT_BELTS
	if ( n < 0 || n >= LocalGS.globalGS->cvtCount ) {
		n = 0;
	}
#endif
	
	CHECK_CVT( &LocalGS, n );
	return LocalGS.globalGS->controlValueTable[ n ];
}

FUNCTION LOCAL_PROTO F26Dot6 fnt_GetCVTEntrySlow(GSP_DECL1 ArrayIndex n)
{
	register Fixed scale;
	register F26Dot6 cvtValue = LocalGS.globalGS->controlValueTable[ n ];

#ifdef USE_SEAT_BELTS
	if ( n < 0 || n >= LocalGS.globalGS->cvtCount ) {
		n = 0;
	}
#endif
	CHECK_CVT( &LocalGS, n );
	scale = fnt_GetCVTScale( GSPPARAM0 );
	return ( (FixMul( cvtValue+cvtValue, scale ) + 1 ) >> 1);
}



FUNCTION LOCAL_PROTO F26Dot6 fnt_GetSingleWidthFast(GSP_DECL0)
{
 	return LocalGS.globalGS->localParBlock.scaledSW;
}


/*
 *
 */

FUNCTION LOCAL_PROTO F26Dot6 fnt_GetSingleWidthSlow(GSP_DECL0)
{
	register Fixed scale;

	scale = fnt_GetCVTScale( GSPPARAM0 );
	return ( FixMul( LocalGS.globalGS->localParBlock.scaledSW, scale ) );
}



/*************************************************************************/



FUNCTION LOCAL_PROTO void fnt_ChangeCvt(GSP_DECL1 fnt_ElementType *elem, ArrayIndex number, F26Dot6 delta)
{
	UNUSED( elem );
	CHECK_CVT( &LocalGS, number );
	
	/* Modify the delta for non-square resolutions */
	if ( !LocalGS.globalGS->identityTransformation )
	{
		Fixed scale = fnt_GetCVTScale( GSPPARAM0 );
		if (  scale != ONEFIX && scale != 0 ) {
			delta = (FixDiv( delta+delta, scale ) + 1) >> 1;
		}
	}

	LocalGS.globalGS->controlValueTable[ number ] += delta;
}


/*
 * This is the tracing interpreter.
 */

FUNCTION LOCAL_PROTO void fnt_InnerTraceExecute(GSP_DECL1 uint8 *ptr, register uint8 *eptr)
{
	register FntFunc* function;
	register uint8 *oldInsPtr;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	oldInsPtr = LocalGS.insPtr;
	LocalGS.insPtr = ptr;
	function = LocalGS.globalGS->function;

	if ( !LocalGS.TraceFunc )
		return; /* so we exit properly out of CALL() */

	while ( LocalGS.insPtr < eptr )
	{
			/* The interpreter does not use LocalGS.roundToGrid, so set it here */

		if ( pb->RoundValue == (FntRoundFunc)fnt_RoundToGrid )
			LocalGS.roundToGrid = 1;
		else
			if ( pb->RoundValue == (FntRoundFunc)fnt_RoundToHalfGrid )
				LocalGS.roundToGrid = 0;
			else
				if ( pb->RoundValue == (FntRoundFunc)fnt_RoundToDoubleGrid )
					LocalGS.roundToGrid = 2;
				else
					if ( pb->RoundValue == (FntRoundFunc)fnt_RoundDownToGrid )
						LocalGS.roundToGrid = 3;
					else
						if ( pb->RoundValue == (FntRoundFunc)fnt_RoundUpToGrid )
							LocalGS.roundToGrid = 4;
						else
							if ( pb->RoundValue == (FntRoundFunc)fnt_RoundOffSLOW )
								LocalGS.roundToGrid = 5;
							else
								if ( pb->RoundValue == (FntRoundFunc)fnt_SuperRound )
									LocalGS.roundToGrid = 6;
								else
									if ( pb->RoundValue == (FntRoundFunc)fnt_Super45Round )
										LocalGS.roundToGrid = 7;
									else
										LocalGS.roundToGrid = -1;

		LocalGS.TraceFunc( &LocalGS );

		if ( !LocalGS.TraceFunc ) break; /* in case the editor wants to exit */

		function[ LocalGS.opCode = *LocalGS.insPtr++ ]( GSPPARAM0 );

	}

	LocalGS.insPtr = oldInsPtr;
}


#ifdef DEBUG
#define LIMIT		65536L*64L

GLOBAL_PROTO void CHECK_STATE(GSP_DECL0);

FUNCTION void CHECK_STATE(GSP_DECL0)
{
	fnt_ElementType* elem;
	F26Dot6* x;
	F26Dot6* y;
	int16 count;
	F26Dot6 xmin, xmax, ymin, ymax;

	if (!LocalGS.globalGS->glyphProgram)
		return;

	elem = &LocalGS.elements[1];
	x = elem->x;
	y = elem->y;
	count = elem->ep[elem->nc - 1];
	xmin = xmax = *x;
	ymin = ymax = *y;

	for (; count >= 0; --count)
	{
		if (*x < xmin)
			xmin = *x;
		else
			if (*x > xmax)
				xmax = *x;

		if (*y < ymin)
			ymin = *y;
		else
			if (*y > ymax)
				ymax = *y;

		x++, y++;
	}

	if (xmin < -LIMIT || xmax > LIMIT || ymin < -LIMIT || ymax > LIMIT)
		Debugger();
}
#else
#define CHECK_STATE(gs)
#endif


#ifdef D_Stack
int aLevel;

GLOBAL_PROTO int32 MY_CHECK_STATE(fnt_LocalGraphicStateType *pGS);

FUNCTION int32 MY_CHECK_STATE(fnt_LocalGraphicStateType *pGS)
{
int32 offset;
	offset = pGS->stackPointer - pGS->globalGS->stackBase;
	return(offset);
}
#endif
/*
 * This is the fast non-tracing interpreter.
 */

FUNCTION LOCAL_PROTO void fnt_InnerExecute(GSP_DECL1 uint8 *ptr, register uint8 *eptr)
{
    register FntFunc* function;
	uint8 *oldInsPtr;
	#ifdef D_Stack
		int aCounter = 0;
		uint8 *origInsPtr;
	#endif

	#ifdef D_Stack
		aLevel++;
	#endif
	
	oldInsPtr 		= LocalGS.insPtr;				/* Save the IP */
	LocalGS.insPtr	= ptr;							/* Set the IP */
	function		= LocalGS.globalGS->function;	/* Cache this. */
	CHECK_STATE( LocalGS );
	
	while ( LocalGS.insPtr < eptr ) {
		#ifdef D_Stack
			origInsPtr = LocalGS.insPtr;
		#endif
		function[LocalGS.opCode = *(LocalGS.insPtr)++ ]( GSPPARAM0 );
		CHECK_STATE( LocalGS );
		#ifdef D_Stack
		{
			int32 stackOffset;
			aCounter++;
			stackOffset = MY_CHECK_STATE( &LocalGS );
			if (examineStack)
			{
				printf("fnt_InnerExecute() Cmd = 0x%x, @ %ld, nest = %d, cnt = %d, stkOff = %d\n",
						(int)*opCodePtr,
						(long)(origInsPtr - ptr),
						aLevel, aCounter, stackOffset);
			}
		}
		#endif
	}

	LocalGS.insPtr	= oldInsPtr;					/* Restore the IP */
#ifdef D_Stack
	aLevel--;
#endif
}



#ifdef DEBUG

LOCAL_PROTO int32 fnt_NilFunction(void);

FUNCTION LOCAL_PROTO int32 fnt_NilFunction(void)
{
#ifdef DEBUG
	Debugger();
#endif
	return 0;
}
#endif


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
 *		local graphics state if TraceFunc is not null.
 *
 * Note: The stuff globalGS is pointing at must remain intact
 *       between calls to this function.
 */

FUNCTION int fnt_Execute(fnt_ElementType *elements, uint8 *ptr, register uint8 *eptr,
						 fnt_GlobalGraphicStateType *globalGS, fs_FuncType TraceFunc)
{

#if REENTRANT_ALLOC
	fnt_LocalGraphicStateType thisLocalGS;
	fnt_LocalGraphicStateType* pLocalGS = &thisLocalGS;
#endif

register int result;

	LocalGS.globalGS = globalGS;

	LocalGS.elements = elements;
	LocalGS.Pt0 = LocalGS.Pt1 = LocalGS.Pt2 = 0;
	if ( globalGS->init ) {
		/* USE_SEAT_BELTS Non-standard but protects from bad fonts that try to use the pre or font program on elem 1 */
		LocalGS.CE0 = LocalGS.CE1 = LocalGS.CE2 = &elements[0];
	} else {
		LocalGS.CE0 = LocalGS.CE1 = LocalGS.CE2 = &elements[1];
	}
	LocalGS.free.x = LocalGS.proj.x = LocalGS.oldProj.x = ONEVECTOR;
	LocalGS.free.y = LocalGS.proj.y = LocalGS.oldProj.y = 0;
	LocalGS.pfProj = ONEVECTOR;
	LocalGS.MovePoint	= fnt_XMovePoint;
	LocalGS.Project		= fnt_XProject;
	LocalGS.OldProject	= fnt_XProject;
	LocalGS.loop 		= 0;		/* 1 less than count for faster loops. mrr */

	LocalGS.GetCVTEntry		= fnt_GetCVTEntryFast;
	LocalGS.GetSingleWidth	= fnt_GetSingleWidthFast;
	
	if ( globalGS->pgmIndex == FONTPROGRAM ) {
#ifdef DEBUG
		LocalGS.GetCVTEntry = fnt_NilFunction;
		LocalGS.GetSingleWidth = fnt_NilFunction;
#endif
		goto ASSIGN_POINTERS;
	}

	if ( globalGS->pixelsPerEm <= 1 )
		return NO_ERR;
	if ( !globalGS->identityTransformation ) {
		LocalGS.GetCVTEntry = fnt_GetCVTEntrySlow;
		LocalGS.GetSingleWidth = fnt_GetSingleWidthSlow;
		if ( FixMul( globalGS->fpem, globalGS->xStretch ) <= ONEFIX ||
			 FixMul( globalGS->fpem, globalGS->yStretch ) <= ONEFIX )
			return NO_ERR;
	}

#define NO_CHECK_TRASHED_MEMORY
	if ( globalGS->init )
	{
#ifndef NO_CHECK_TRASHED_MEMORY
#ifdef CLEANMACHINE
		if ( globalGS->function[ 0x00 ] != fnt_SVTCA_0 )
		{
#else
				/* Clean off high byte for checking .... */
			if ( ((int32)globalGS->function[ 0x00 ] & 0x00ffffff) != ((int32)fnt_SVTCA_0 & 0x00ffffff) ) {
#endif
				/* Who trashed my memory ???? */
				return( TRASHED_MEM_ERR  );
		}
#endif
	}
	else
		if ( globalGS->localParBlock.sW )
		{
				/* We need to scale the single width for this size  */
			globalGS->localParBlock.scaledSW = globalGS->ScaleFunc( globalGS, globalGS->localParBlock.sW );
		}

ASSIGN_POINTERS:

	LocalGS.stackPointer = globalGS->stackBase;

	if ( ( result = setjmp(LocalGS.env) ) != 0 )
		return( result );	/* got an error */

	globalGS->anglePoint = (fnt_FractPoint*)((char*)globalGS->function + MAXBYTE_INSTRUCTIONS * sizeof(FntFunc));
	globalGS->angleDistance = (int16*)(globalGS->anglePoint + MAXANGLES);

	if ( globalGS->anglePoint[1].y != 759250125L )
	{	/* should be same as set to in fnt_Init() */
		/* Who trashed my memory ???? */
		fnt_Panic( GSPPARAM1 TRASHED_MEM_ERR );
	}

		/* first assign */
	LocalGS.Interpreter = (LocalGS.TraceFunc = (TraceFuncType)TraceFunc) != NULL ?
							fnt_InnerTraceExecute : fnt_InnerExecute;
		/* then call */
	LocalGS.Interpreter (GSPPARAM1 ptr, eptr);
	return NO_ERR;
}


/*************************************************************************/


/*** 2 internal LocalGS.pfProj computation support routines ***/

/*
 * Only does the check of LocalGS.pfProj
 */

FUNCTION LOCAL_PROTO void fnt_Check_PF_Proj(GSP_DECL0)
{
	register VECTORTYPE pfProj = LocalGS.pfProj;

	if ( pfProj > -ONESIXTEENTHVECTOR && pfProj < ONESIXTEENTHVECTOR)
	{
		LocalGS.pfProj = (VECTORTYPE)(pfProj < 0 ? -ONEVECTOR : ONEVECTOR); /* Prevent divide by small number */
	}
}


/*
 * Computes LocalGS.pfProj and then does the check
 */

FUNCTION LOCAL_PROTO void fnt_ComputeAndCheck_PF_Proj(GSP_DECL0)
{
	register VECTORTYPE pfProj;

	pfProj = (VECTORTYPE)(VECTORDOT( LocalGS.proj.x, LocalGS.free.x ) + VECTORDOT( LocalGS.proj.y, LocalGS.free.y ));
	if ( pfProj > -ONESIXTEENTHVECTOR && pfProj < ONESIXTEENTHVECTOR)
	{
		pfProj = (VECTORTYPE)(pfProj < 0 ? -ONEVECTOR : ONEVECTOR); /* Prevent divide by small number */
	}
	LocalGS.pfProj = pfProj;
}



/******************************************/
/******** The Actual Instructions *********/
/******************************************/

/*
 * Set Vectors To Coordinate Axis - Y
 */

FUNCTION LOCAL_PROTO void fnt_SVTCA_0(GSP_DECL0)
{
	LocalGS.free.x = LocalGS.proj.x = 0;
	LocalGS.free.y = LocalGS.proj.y = ONEVECTOR;
	LocalGS.MovePoint = fnt_YMovePoint;
	LocalGS.Project = fnt_YProject;
	LocalGS.OldProject = fnt_YProject;
	LocalGS.pfProj = ONEVECTOR;
}


/*
 * Set Vectors To Coordinate Axis - X
 */

FUNCTION LOCAL_PROTO void fnt_SVTCA_1(GSP_DECL0)
{
	LocalGS.free.x = LocalGS.proj.x = ONEVECTOR;
	LocalGS.free.y = LocalGS.proj.y = 0;
	LocalGS.MovePoint = fnt_XMovePoint;
	LocalGS.Project = fnt_XProject;
	LocalGS.OldProject = fnt_XProject;
	LocalGS.pfProj = ONEVECTOR;
}


/*
 * Set Projection Vector To Coordinate Axis
 */

FUNCTION LOCAL_PROTO void fnt_SPVTCA(GSP_DECL0)
{
	if ( TT_BIT0( LocalGS.opCode )  )
	{
		LocalGS.proj.x = ONEVECTOR;
		LocalGS.proj.y = 0;
		LocalGS.Project = fnt_XProject;
		LocalGS.pfProj = LocalGS.free.x;
	}
	else
	{
		LocalGS.proj.x = 0;
		LocalGS.proj.y = ONEVECTOR;
		LocalGS.Project = fnt_YProject;
		LocalGS.pfProj = LocalGS.free.y;
	}
	fnt_Check_PF_Proj( GSPPARAM0 );
	LocalGS.MovePoint = fnt_MovePoint;
	LocalGS.OldProject = LocalGS.Project;
}


/*
 * Set Freedom Vector to Coordinate Axis
 */

FUNCTION LOCAL_PROTO void fnt_SFVTCA(GSP_DECL0)
{
	if ( TT_BIT0( LocalGS.opCode )  )
	{
		LocalGS.free.x = ONEVECTOR;
		LocalGS.free.y = 0;
		LocalGS.pfProj = LocalGS.proj.x;
	}
	else
	{
		LocalGS.free.x = 0;
		LocalGS.free.y = ONEVECTOR;
		LocalGS.pfProj = LocalGS.proj.y;
	}
	fnt_Check_PF_Proj( GSPPARAM0 );
	LocalGS.MovePoint = fnt_MovePoint;
}


/*
 * Set Projection Vector To Line
 */

FUNCTION LOCAL_PROTO void fnt_SPVTL(GSP_DECL0)
{
	register ArrayIndex arg1, arg2;

	arg2 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_POINT( &LocalGS, LocalGS.CE2, arg2 );
	CHECK_POINT( &LocalGS, LocalGS.CE1, arg1 );

	fnt_Normalize( LocalGS.CE1->x[arg1] - LocalGS.CE2->x[arg2], LocalGS.CE1->y[arg1] - LocalGS.CE2->y[arg2], &LocalGS.proj );
	if ( TT_BIT0( LocalGS.opCode ) )
	{
		/* rotate 90 degrees */
		VECTORTYPE tmp	= LocalGS.proj.y;
		LocalGS.proj.y	= LocalGS.proj.x;
		LocalGS.proj.x	= (VECTORTYPE)-tmp;
	}
	fnt_ComputeAndCheck_PF_Proj( GSPPARAM0 );
	LocalGS.MovePoint = fnt_MovePoint;
	LocalGS.Project = fnt_Project;
	LocalGS.OldProject = LocalGS.Project;
}



/*
 * Set Dual Projection Vector To Line
 */

FUNCTION LOCAL_PROTO void fnt_SDPVTL(GSP_DECL0)
{
	register ArrayIndex arg1, arg2;

	arg2 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_POINT( &LocalGS, LocalGS.CE2, arg2 );
	CHECK_POINT( &LocalGS, LocalGS.CE1, arg1 );

	/* Do the current domain */
	fnt_Normalize( LocalGS.CE1->x[arg1] - LocalGS.CE2->x[arg2], LocalGS.CE1->y[arg1] - LocalGS.CE2->y[arg2], &LocalGS.proj );

	/* Do the old domain */
	fnt_Normalize( LocalGS.CE1->ox[arg1] - LocalGS.CE2->ox[arg2], LocalGS.CE1->oy[arg1] - LocalGS.CE2->oy[arg2], &LocalGS.oldProj );

	if ( TT_BIT0( LocalGS.opCode ) )
	{
		/* rotate 90 degrees */
		VECTORTYPE tmp	= LocalGS.proj.y;
		LocalGS.proj.y		= LocalGS.proj.x;
		LocalGS.proj.x		= (VECTORTYPE)-tmp;

		tmp				= LocalGS.oldProj.y;
		LocalGS.oldProj.y	= LocalGS.oldProj.x;
		LocalGS.oldProj.x	= (VECTORTYPE)-tmp;
	}
	fnt_ComputeAndCheck_PF_Proj( GSPPARAM0 );

	LocalGS.MovePoint = fnt_MovePoint;
	LocalGS.Project = fnt_Project;
	LocalGS.OldProject = fnt_OldProject;
}


/*
 * Set Freedom Vector To Line
 */

FUNCTION LOCAL_PROTO void fnt_SFVTL(GSP_DECL0)
{
	register ArrayIndex arg1, arg2;

	arg2 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_POINT( &LocalGS, LocalGS.CE2, arg2 );
	CHECK_POINT( &LocalGS, LocalGS.CE1, arg1 );

	fnt_Normalize( LocalGS.CE1->x[arg1] - LocalGS.CE2->x[arg2], LocalGS.CE1->y[arg1] - LocalGS.CE2->y[arg2], &LocalGS.free );
	if ( TT_BIT0( LocalGS.opCode ) )
	{
		/* rotate 90 degrees */
		VECTORTYPE tmp	= LocalGS.free.y;
		LocalGS.free.y		= LocalGS.free.x;
		LocalGS.free.x		= (VECTORTYPE)-tmp;
	}
	fnt_ComputeAndCheck_PF_Proj( GSPPARAM0 );
	LocalGS.MovePoint = fnt_MovePoint;
}


/*
 * Write Projection Vector
 */

FUNCTION LOCAL_PROTO void fnt_WPV(GSP_DECL0)
{
	LocalGS.proj.y = (VECTORTYPE)CHECK_POP(LocalGS, LocalGS.stackPointer);
	LocalGS.proj.x = (VECTORTYPE)CHECK_POP(LocalGS, LocalGS.stackPointer);

	fnt_ComputeAndCheck_PF_Proj( GSPPARAM0 );

	LocalGS.MovePoint = fnt_MovePoint;
	LocalGS.Project = fnt_Project;
	LocalGS.OldProject = LocalGS.Project;
}


/*
 * Write Freedom vector
 */

FUNCTION LOCAL_PROTO void fnt_WFV(GSP_DECL0)
{
	LocalGS.free.y = (VECTORTYPE)CHECK_POP(LocalGS, LocalGS.stackPointer);
	LocalGS.free.x = (VECTORTYPE)CHECK_POP(LocalGS, LocalGS.stackPointer);

	fnt_ComputeAndCheck_PF_Proj( GSPPARAM0 );

	LocalGS.MovePoint = fnt_MovePoint;
}


/*
 * Read Projection Vector
 */

FUNCTION LOCAL_PROTO void fnt_RPV(GSP_DECL0)
{
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.proj.x );
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.proj.y );
}


/*
 * Read Freedom Vector
 */

FUNCTION LOCAL_PROTO void fnt_RFV(GSP_DECL0)
{
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.free.x );
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.free.y );
}


/*
 * Set Freedom Vector To Projection Vector
 */

FUNCTION LOCAL_PROTO void fnt_SFVTPV(GSP_DECL0)
{
	LocalGS.free = LocalGS.proj;
	LocalGS.pfProj = ONEVECTOR;
	LocalGS.MovePoint = fnt_MovePoint;
}


/*
 * fnt_ISECT()
 *
 * Computes the intersection of two lines without using floating point!!
 *
 * (1) Bx + dBx * t0 = Ax + dAx * t1
 * (2) By + dBy * t0 = Ay + dAy * t1
 *
 *  1  =>  (t1 = Bx - Ax + dBx * t0 ) / dAx
 *  +2 =>   By + dBy * t0 = Ay + dAy/dAx * [ Bx - Ax + dBx * t0 ]
 *     => t0 * [dAy/dAx * dBx - dBy = By - Ay - dAy/dAx*(Bx-Ax)
 *     => t0(dAy*DBx - dBy*dAx) = dAx(By - Ay) + dAy(Ax-Bx)
 *     => t0 = [dAx(By-Ay) + dAy(Ax-Bx)] / [dAy*dBx - dBy*dAx]
 *     => t0 = [dAx(By-Ay) - dAy(Bx-Ax)] / [dBx*dAy - dBy*dAx]
 *     t0 = N/D
 *     =>
 *	    N = (By - Ay) * dAx - (Bx - Ax) * dAy;
 *		D = dBx * dAy - dBy * dAx;
 *      A simple floating point implementation would only need this, and
 *      the check to see if D is zero.
 *		But to gain speed we do some tricks and avoid floating point.
 *
 *    Changed to F16Dot16 from Fract for t to avoid using FracMul and FracDiv...Jan 28, 1998
 */

FUNCTION LOCAL_PROTO void fnt_ISECT(GSP_DECL0)
{
	register F26Dot6 N, D;
	register F16Dot16 t;
	register ArrayIndex arg1, arg2;
	F26Dot6 Bx, By, Ax, Ay;
	F26Dot6 dBx, dBy, dAx, dAy;

	{
		register fnt_ElementType* element = LocalGS.CE0;
		register F26Dot6* stack = LocalGS.stackPointer;

		arg2 = (ArrayIndex)CHECK_POP(LocalGS, stack ); /* get one line */
		arg1 = (ArrayIndex)CHECK_POP(LocalGS, stack );
		dAx = element->x[arg2] - (Ax = element->x[arg1]);
		dAy = element->y[arg2] - (Ay = element->y[arg1]);

		element = LocalGS.CE1;
		arg2 = (ArrayIndex)CHECK_POP(LocalGS, stack ); /* get the other line */
		arg1 = (ArrayIndex)CHECK_POP(LocalGS, stack );
		dBx = element->x[arg2] - (Bx = element->x[arg1]);
		dBy = element->y[arg2] - (By = element->y[arg1]);

		arg1 = (ArrayIndex)CHECK_POP(LocalGS, stack ); /* get the point number */
		LocalGS.stackPointer = stack;
	}

	LocalGS.CE2->f[arg1] |= XMOVED | YMOVED;

	{
		register F26Dot6* elementx = LocalGS.CE2->x;
		register F26Dot6* elementy = LocalGS.CE2->y;
		if ( dAy == 0 ) {
			if ( dBx == 0 ) {
				elementx[arg1] = Bx;
				elementy[arg1] = Ay;
				return; /*****/
			}
			N = By - Ay;
			D = -dBy;
		} else if ( dAx == 0 ) {
			if ( dBy == 0 ) {
				elementx[arg1] = Ax;
				elementy[arg1] = By;
				return;  /*****/
			}
			N = Bx - Ax;
			D = -dBx;
		} else if ( MABS( dAx ) > MABS( dAy ) ) {
			/* To prevent out of range problems divide both N and D with the max */
			t = util_FixDiv( dAy, dAx );
			N = (By - Ay) - util_FixMul( (Bx - Ax), t );
			D = util_FixMul( dBx, t ) - dBy;
		} else {
			t = util_FixDiv( dAx, dAy );
			N = util_FixMul( (By - Ay), t ) - (Bx - Ax);
			D = dBx - util_FixMul( dBy, t );
		}
		/*
		N = (By - Ay) * dAx - (Bx - Ax) * dAy;
		D = dBx * dAy - dBy * dAx;
		*/
		if ( D ) {
			if ( MABS( N ) < MABS( D ) ) {
				/* this is the normal case */
				t = util_FixDiv( N, D );
				elementx[arg1] = Bx + util_FixMul( dBx, t );
				elementy[arg1] = By + util_FixMul( dBy, t );
			} else {
				if ( N ) {
					/* Oh well, invert t and use it instead */
					t = util_FixDiv( D, N );
					elementx[arg1] = Bx + util_FixDiv( dBx, t );
					elementy[arg1] = By + util_FixDiv( dBy, t );
				} else {
					elementx[arg1] = Bx;
					elementy[arg1] = By;
				}
			}
		} else {
			/* degenerate case: parallell lines, put point in the middle */
			elementx[arg1] = (Bx + (dBx>>1) + Ax + (dAx>>1)) >> 1;
			elementy[arg1] = (By + (dBy>>1) + Ay + (dAy>>1)) >> 1;
		}
	}
}


/*
 * Load Minimum Distanc
 */

FUNCTION LOCAL_PROTO void fnt_LMD(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.minimumDistance = CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/*
 * Load Control Value Table Cut In
 */

FUNCTION LOCAL_PROTO void fnt_LWTCI(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.wTCI = CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/*
 * Load Single Width Cut In
 */

FUNCTION LOCAL_PROTO void fnt_LSWCI(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.sWCI = CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/*
 * Load Single Width , assumes value comes from the original domain, not the cvt or outline
 */

FUNCTION LOCAL_PROTO void fnt_LSW(GSP_DECL0)
{
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	register fnt_ParameterBlock *pb = &globalGS->localParBlock;

	pb->sW = (int16)CHECK_POP(LocalGS, LocalGS.stackPointer );

	pb->scaledSW = globalGS->ScaleFunc( globalGS, pb->sW ); /* measurement should not come from the outline */
}



FUNCTION LOCAL_PROTO void fnt_SetLocalGraphicState(GSP_DECL0)
{
	int arg = (int)CHECK_POP(LocalGS, LocalGS.stackPointer );

	switch (LocalGS.opCode)
	{
	case SRP0_CODE:
		LocalGS.Pt0 = (ArrayIndex)arg;
		break;
	case SRP1_CODE:
		LocalGS.Pt1 = (ArrayIndex)arg;
		break;
	case SRP2_CODE:
		LocalGS.Pt2 = (ArrayIndex)arg;
		break;

	case LLOOP_CODE:
		LocalGS.loop = (LoopCount)(arg - 1);
		break;

	case POP_CODE:
		break;
#ifdef DEBUG
	default:
		Debugger();
		break;
#endif
	}
}



FUNCTION LOCAL_PROTO void fnt_SetElementPtr(GSP_DECL0)
{
	ArrayIndex arg = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	fnt_ElementType* element = &LocalGS.elements[ arg ];

	CHECK_ELEMENT( &LocalGS, arg );

	switch (LocalGS.opCode)
	{
	case SCES_CODE:
		LocalGS.CE2 = element;
		LocalGS.CE1 = element;
	case SCE0_CODE:
		LocalGS.CE0 = element;
		break;
	case SCE1_CODE:
		LocalGS.CE1 = element;
		break;
	case SCE2_CODE:
		LocalGS.CE2 = element;
		break;
#ifdef DEBUG
	default:
		Debugger();
		break;
#endif
	}
}


/*
 * Super Round
 */

FUNCTION LOCAL_PROTO void fnt_SROUND(GSP_DECL0)
{
	register int arg1 = (int)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	fnt_SetRoundValues( GSPPARAM1 arg1, true );
	pb->RoundValue = fnt_SuperRound;
}


/*
 * Super Round
 */

FUNCTION LOCAL_PROTO void fnt_S45ROUND(GSP_DECL0)
{
	register int arg1 = (int)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	fnt_SetRoundValues( GSPPARAM1 arg1, false );
	pb->RoundValue = fnt_Super45Round;
}


/*
 *	These functions just set a field of the graphics state
 *	They pop no arguments
 */

FUNCTION LOCAL_PROTO void fnt_SetRoundState(GSP_DECL0)
{
	register FntRoundFunc *rndFunc = &LocalGS.globalGS->localParBlock.RoundValue;

	switch (LocalGS.opCode)
	{
	case RTG_CODE:
		*rndFunc = fnt_RoundToGrid;
		break;
	case RTHG_CODE:
		*rndFunc = fnt_RoundToHalfGrid;
		break;
	case RTDG_CODE:
		*rndFunc = fnt_RoundToDoubleGrid;
		break;
	case ROFF_CODE:
		*rndFunc = fnt_RoundOffSLOW;
		break;
	case RDTG_CODE:
		*rndFunc = fnt_RoundDownToGrid;
		break;
	case RUTG_CODE:
		*rndFunc = fnt_RoundUpToGrid;
		break;
#ifdef DEBUG
	default:
		Debugger();
		break;
#endif
	}
}


#define FRACSQRT2DIV2 759250125
/*
 * Internal support routine for the super rounding routines
 */

FUNCTION LOCAL_PROTO void fnt_SetRoundValues(GSP_DECL1 register int arg1, register int normalRound)
{
	register int tmp;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	tmp = arg1 & 0xC0;

	if ( normalRound )
	{
		switch ( tmp )
		{
		case 0x00:
			pb->period = fnt_pixelSize/2;
			break;
		case 0x40:
			pb->period = fnt_pixelSize;
			break;
		case 0x80:
			pb->period = fnt_pixelSize*2;
			break;
		default:
			pb->period = 999; /* Illegal */
		}
		pb->periodMask = ~(pb->period-1);
	}
	else
	{
		pb->period45 = FRACSQRT2DIV2;
		switch ( tmp )
		{
		case 0x00:
			pb->period45 >>= 1;
			break;
		case 0x40:
			break;
		case 0x80:
			pb->period45 <<= 1;
			break;
		default:
			pb->period45 = 999; /* Illegal */
		}
		tmp = (sizeof(Fract) * 8 - 2 - fnt_pixelShift);
		pb->period = (int16)((pb->period45 + (1L<<(tmp-1))) >> tmp); /*convert from 2.30 to 26.6 */
	}

	tmp = arg1 & 0x30;
	switch ( tmp )
	{
	case 0x00:
		pb->phase = 0;
		break;
	case 0x10:
		pb->phase = (int16)((pb->period + 2) >> 2);
		break;
	case 0x20:
		pb->phase = (int16)((pb->period + 1) >> 1);
		break;
	case 0x30:
		pb->phase = (int16)((pb->period + pb->period + pb->period + 2) >> 2);
		break;
	}
	tmp = arg1 & 0x0f;
	if ( tmp == 0 )
	{
		pb->threshold = (int16)(pb->period-1);
	}
	else
	{
		pb->threshold = (int16)(((tmp - 4) * pb->period + 4) >> 3);
	}
}


/*
 * Read Advance Width
 */

FUNCTION LOCAL_PROTO void fnt_RAW(GSP_DECL0)
{
	fnt_ElementType* elem = &LocalGS.elements[1];
	F26Dot6* ox = elem->ox;
	ArrayIndex index = elem->ep[elem->nc - 1] + 1;		/* lsb point */

	CHECK_PUSH( LocalGS, LocalGS.stackPointer, ox[index+1] - ox[index] );
}


/*
 * DUPlicate
 */

FUNCTION LOCAL_PROTO void fnt_DUP(GSP_DECL0)
{
	F26Dot6 top = LocalGS.stackPointer[-1];
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, top);
}


/*
 * CLEAR stack
 */

FUNCTION LOCAL_PROTO void fnt_CLEAR(GSP_DECL0)
{
	LocalGS.stackPointer = LocalGS.globalGS->stackBase;
}


/*
 * SWAP
 */

FUNCTION LOCAL_PROTO void fnt_SWAP(GSP_DECL0)
{
	register F26Dot6 arg2 = CHECK_POP(LocalGS, LocalGS.stackPointer );
	register F26Dot6 arg1 = CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_PUSH( LocalGS, LocalGS.stackPointer, arg2 );
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, arg1 );
}


/*
 * DEPTH
 */

FUNCTION LOCAL_PROTO void fnt_DEPTH(GSP_DECL0)
{
	F26Dot6 depth = LocalGS.stackPointer - LocalGS.globalGS->stackBase;
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, depth);
}


/*
 * Copy INDEXed value
 */

FUNCTION LOCAL_PROTO void fnt_CINDEX(GSP_DECL0)
{
	register ArrayIndex arg1;
	register F26Dot6 tmp;

	arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	tmp = *(LocalGS.stackPointer - arg1 );
	CHECK_PUSH( LocalGS, LocalGS.stackPointer , tmp );
}


/*
 * Move INDEXed value
 */

FUNCTION LOCAL_PROTO void fnt_MINDEX(GSP_DECL0)
{
	register ArrayIndex arg1;
	register F26Dot6 tmp, *p;
	register F26Dot6* stack = LocalGS.stackPointer;

	arg1 = (ArrayIndex)CHECK_POP(LocalGS, stack );
	tmp = *(p = (stack - arg1));
	if ( arg1 )
	{
		do
		{
			*p = *(p + 1); p++;
		} while ( --arg1 );
		UNUSED( CHECK_POP(LocalGS, stack ) );
	}
	CHECK_PUSH( LocalGS, stack, tmp );
	LocalGS.stackPointer = stack;
}


/*
 *	Rotate element 3 to the top of the stack			<4>
 *	Thanks to Oliver for the obscure code.
 */

FUNCTION LOCAL_PROTO void fnt_ROTATE(GSP_DECL0)
{
	register F26Dot6 *stack = LocalGS.stackPointer;
	register F26Dot6 element1 = *--stack;
	register F26Dot6 element2 = *--stack;
	*stack = element1;
	element1 = *--stack;
	*stack = element2;
	*(stack + 2) = element1;
}


/*
 * Move Direct Absolute Point
 */

FUNCTION LOCAL_PROTO void fnt_MDAP(GSP_DECL0)
{
	register F26Dot6 proj;
	register fnt_ElementType* ce0 = LocalGS.CE0;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	register ArrayIndex ptNum;

	ptNum = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	LocalGS.Pt0 = LocalGS.Pt1 = ptNum;

	if ( TT_BIT0( LocalGS.opCode ) )
	{
		proj = (LocalGS.Project)( GSPPARAM1 ce0->x[ptNum], ce0->y[ptNum] );
		/* proj = pb->RoundValue( GSPPARAM1 proj, LocalGS.globalGS->engine[0] ) - proj; */
		proj = pb->RoundValue( GSPPARAM1 proj, 0 ) - proj;
	}
	else
		proj = 0;		/* mark the point as touched */

	LocalGS.MovePoint( GSPPARAM1 ce0, ptNum, proj );
}


/*
 * Move Indirect Absolute Point
 */

FUNCTION LOCAL_PROTO void fnt_MIAP(GSP_DECL0)
{
	register ArrayIndex ptNum;
	register F26Dot6 newProj, origProj;
	register fnt_ElementType* ce0 = LocalGS.CE0;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	newProj = LocalGS.GetCVTEntry( GSPPARAM1 CHECK_POP(LocalGS, LocalGS.stackPointer ) );
	ptNum = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_POINT(&LocalGS, ce0, ptNum);
	LocalGS.Pt0 = LocalGS.Pt1 = ptNum;

	if ( ce0 == LocalGS.elements )		/* twilightzone */
	{
		ce0->x[ptNum] = ce0->ox[ptNum] = VECTORMUL( newProj, LocalGS.proj.x );
		ce0->y[ptNum] = ce0->oy[ptNum] = VECTORMUL( newProj, LocalGS.proj.y );
	}

	origProj = LocalGS.Project( GSPPARAM1 ce0->x[ptNum], ce0->y[ptNum] );

	if ( TT_BIT0( LocalGS.opCode ) )
	{
		register F26Dot6 tmp = newProj - origProj;
		if ( tmp < 0 )
			tmp = -tmp;
		if ( tmp > pb->wTCI )
			newProj = origProj;
		/* newProj = pb->RoundValue( GSPPARAM1 newProj, LocalGS.globalGS->engine[0] ); */
		newProj = pb->RoundValue( GSPPARAM1 newProj, 0 );
	}

	newProj -= origProj;
	LocalGS.MovePoint( GSPPARAM1 ce0, ptNum, newProj );
}


/*
 * Interpolate Untouched Points
 */

FUNCTION LOCAL_PROTO void fnt_IUP(GSP_DECL0)
{
	register ArrayIndex pt;
	register int16 *ooCoord;          /* Jean made me lie, these are really ints, Changed to int16 Dec 23, 1998 ---Sampo */
	LoopCount ctr;
	F26Dot6 *coord, *oCoord;
	int mask;
	ArrayIndex ptOrg, end;
	ArrayIndex ep_ctr, sp_ctr;
	fnt_ElementType *pCE2 = LocalGS.CE2;
	uint8  *pFlags = pCE2->f;

	if (LocalGS.opCode & 0x01)
	{
			/* do x */
		coord = pCE2->x;
		oCoord = pCE2->ox;
		ooCoord = pCE2->oox;
		mask = XMOVED;
	} 
	else 
	{
			/* do y */
		coord = pCE2->y;
		oCoord = pCE2->oy;
		ooCoord = pCE2->ooy;
		mask = YMOVED;
	}

		/* loop through contours */
	for (ctr = 0; ctr < pCE2->nc; ctr++)
	{
		sp_ctr = pt = pCE2->sp[ctr];
		ep_ctr = pCE2->ep[ctr];
		while (! (pFlags[pt] & mask) && pt <= ep_ctr)
			pt++;
		if (pt > ep_ctr)
			continue;
		ptOrg = pt;

		do
		{
			ArrayIndex start;
			ArrayIndex ptMin, ptMax;
			F26Dot6 _min, o_min, oo_min, dmin, oo_delta;

			do
			{
				start = pt;
				pt = fnt_NextPt1 (pt);
			} while (ptOrg != pt && (pFlags[pt] & mask));

			if (ptOrg == pt)
				break;

			end = pt;
			do
			{
				end = fnt_NextPt1 (end);
			} while (!(pFlags[end] & mask));

			if (ooCoord[start] < ooCoord[end])
			{
				ptMin = start;
				ptMax = end;
			}
			else
			{
				ptMin = end;
				ptMax = start;
			}

			_min  = coord[ptMin];
			o_min = oCoord[ptMin];
			dmin  = _min - o_min;
			oo_delta = ooCoord[ptMax] - (oo_min = ooCoord[ptMin]);

			if (oo_delta)
			{
				F26Dot6 dmax;
				F26Dot6 _delta, o_max, corr, oCoord_pt;
				register int32 tmp;

				o_max = oCoord[ptMax];
				dmax = _delta  = coord[ptMax];
				dmax -= o_max;
				_delta -= _min;

				if (oo_delta < 32768 && _delta < 32768)
				{
					corr = oo_delta >> 1;
					for (; pt != end; pt = fnt_NextPt1 (pt))
					{
						oCoord_pt = oCoord[pt];
						if (oCoord_pt > o_min && oCoord_pt < o_max)
						{
							tmp = SHORTMUL (ooCoord[pt] - oo_min, _delta);
							tmp += corr;
							tmp /= oo_delta;
							coord[pt] = (F26Dot6) tmp + _min;
						}
						else
						{
							if (oCoord_pt >= o_max)
								oCoord_pt += dmax;
							else
								oCoord_pt += dmin;
							coord[pt] = oCoord_pt;
						}
					}
				}
				else
				{
					Fixed ratio = 0;
					int firstTime;

					firstTime = true;
					for (; pt != end; pt = fnt_NextPt1 (pt))
					{
						tmp = oCoord[pt];
						if (tmp <= o_min)
							tmp += dmin;
						else
							if (tmp >= o_max)
								tmp += dmax;
							else
							{
								if (firstTime)
								{
									ratio = FixDiv (_delta, oo_delta);
									firstTime = 0;
								}
								tmp = ooCoord[pt];
								tmp -= oo_min;
								tmp = FixMul (tmp, ratio);
								tmp += _min;
							}
							coord[pt] = (F26Dot6) tmp;
					}
				}
			}
			else
				while (pt != end)
				{
					coord[pt] += dmin;
					pt = fnt_NextPt1 (pt);
				}
		} while (pt != ptOrg);
	}
}



FUNCTION LOCAL_PROTO fnt_ElementType* fnt_SH_Common(GSP_DECL1 F26Dot6 *dx, F26Dot6 *dy, ArrayIndex *point)
{
	F26Dot6 proj;
	ArrayIndex pt;
	fnt_ElementType* element;

	if ( TT_BIT0( LocalGS.opCode ) )
	{
		pt = LocalGS.Pt1;
		element = LocalGS.CE0;
	}
	else
	{
		pt = LocalGS.Pt2;
		element = LocalGS.CE1;
	}
	proj = LocalGS.Project( GSPPARAM1 element->x[pt] - element->ox[pt],
							element->y[pt] - element->oy[pt] );

	if ( LocalGS.pfProj != ONEVECTOR )
	{
		if ( LocalGS.free.x )
			*dx = VECTORMULDIV( proj, LocalGS.free.x, LocalGS.pfProj );
		if ( LocalGS.free.y )
			*dy = VECTORMULDIV( proj, LocalGS.free.y, LocalGS.pfProj );
	}
	else
	{
		if ( LocalGS.free.x )
			*dx = VECTORMUL( proj, LocalGS.free.x );
		if ( LocalGS.free.y )
			*dy = VECTORMUL( proj, LocalGS.free.y );
	}
	*point = pt;
	return element;
}



FUNCTION LOCAL_PROTO void fnt_SHP_Common(GSP_DECL1 F26Dot6 dx, F26Dot6 dy)
{
	register fnt_ElementType* CE2 = LocalGS.CE2;
	register LoopCount count = LocalGS.loop;
	for (; count >= 0; --count)
	{
		ArrayIndex point = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
		if ( LocalGS.free.x )
		{
			CE2->x[point] += dx;
			CE2->f[point] |= XMOVED;
		}
		if ( LocalGS.free.y )
		{
			CE2->y[point] += dy;
			CE2->f[point] |= YMOVED;
		}
	}
	LocalGS.loop = 0;
}


/*
 * SHift Point
 */

FUNCTION LOCAL_PROTO void fnt_SHP(GSP_DECL0)
{
	F26Dot6 dx, dy;
	ArrayIndex point;

	fnt_SH_Common(GSPPARAM1 &dx, &dy, &point);
	fnt_SHP_Common(GSPPARAM1 dx, dy);
}


/*
 * SHift Contour
 */

FUNCTION LOCAL_PROTO void fnt_SHC(GSP_DECL0)
{
	register fnt_ElementType *element;
	register F26Dot6 dx, dy;
	register ArrayIndex contour, point;
	register fnt_ElementType* CE2;

	{
		F26Dot6 x, y;
		ArrayIndex pt;
		element = fnt_SH_Common(GSPPARAM1 &x, &y, &pt);
		point	= pt;
		dx = dy = 0;
		if ( LocalGS.free.x != 0 ) dx = x;
		if ( LocalGS.free.y != 0 ) dy = y;
	}
	contour = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	CE2 = LocalGS.CE2;
	CHECK_CONTOUR(&LocalGS, CE2, contour);

	/* The Apple Palatino shifts ctr # 2 and we only have 0 & 1, so put in the run time if ! */
	if ( contour < CE2->nc ) {
		ArrayIndex currPt	= CE2->sp[contour];
		LoopCount count		= (LoopCount)(CE2->ep[contour] - currPt);
		CHECK_POINT(&LocalGS, CE2, currPt + count);
		
		for (; count >= 0; --count) {
			if ( currPt != point || element != CE2 ) {
				if ( dx ) {
					CE2->x[currPt] += dx;
					CE2->f[currPt] |= XMOVED;
				}
				if ( dy ) {
					CE2->y[currPt] += dy;
					CE2->f[currPt] |= YMOVED;
				}
			}
			currPt++;
		}
	}
}


/*
 * SHift Element			<4>
 */

FUNCTION LOCAL_PROTO void fnt_SHE(GSP_DECL0)
{
	register fnt_ElementType *element;
	register F26Dot6 dx, dy;
	ArrayIndex firstPoint, origPoint, lastPoint, arg1;

	{
		F26Dot6 x, y;
		element = fnt_SH_Common(GSPPARAM1 &x, &y, &origPoint);
		dx = x;
		dy = y;
	}

	arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	CHECK_ELEMENT(&LocalGS, arg1);

	lastPoint = LocalGS.elements[arg1].ep[LocalGS.elements[arg1].nc - 1];
	CHECK_POINT(&LocalGS, &LocalGS.elements[arg1], lastPoint);
	firstPoint  = LocalGS.elements[arg1].sp[0];
	CHECK_POINT(&LocalGS, &LocalGS.elements[arg1], firstPoint);

/*** changed this			<4>
	do {
		if ( origPoint != firstPoint || element != &LocalGS.elements[arg1] ) {
			if ( LocalGS.free.x ) {
				LocalGS.elements[ arg1 ].x[firstPoint] += dx;
				LocalGS.elements[ arg1 ].f[firstPoint] |= XMOVED;
			}
			if ( LocalGS.free.y ) {
				LocalGS.elements[ arg1 ].y[firstPoint] += dy;
				LocalGS.elements[ arg1 ].f[firstPoint] |= YMOVED;
			}
		}
		firstPoint++;
	} while ( firstPoint <= lastPoint );
***** To this ? *********/

	if (element != &LocalGS.elements[arg1])		/* we're in different zones */
		origPoint = -1;						/* no need to skip orig point */
	{
		register int8 mask = 0;
		if ( LocalGS.free.x )
		{
			register F26Dot6 deltaX = dx;
			register F26Dot6* x = &LocalGS.elements[ arg1 ].x[firstPoint];
			register LoopCount count = (LoopCount)(origPoint - firstPoint - 1);
			for (; count >= 0; --count )
				*x++ += deltaX;
			if (origPoint == -1)
				count = (LoopCount)(lastPoint - firstPoint);
			else
			{
				count = (LoopCount)(lastPoint - origPoint - 1);
				x++;							/* skip origPoint */
			}
			for (; count >= 0; --count )
				*x++ += deltaX;
			mask = XMOVED;
		}
		if ( LocalGS.free.y )		/* fix me semore */
		{
			register F26Dot6 deltaY = dy;
			register F26Dot6* y = &LocalGS.elements[ arg1 ].y[firstPoint];
			register uint8* f = &LocalGS.elements[ arg1 ].f[firstPoint];
			register LoopCount count = (LoopCount)(origPoint - firstPoint - 1);
			for (; count >= 0; --count )
			{
				*y++ += deltaY;
				*f++ |= mask;
			}
			if (origPoint == -1)
				count = (LoopCount)(lastPoint - firstPoint);
			else
			{
				count = (LoopCount)(lastPoint - origPoint - 1);
				y++, f++;						/* skip origPoint */
			}
			mask |= YMOVED;
			for (; count >= 0; --count )
			{
				*y++ += deltaY;
				*f++ |= mask;
			}
		}
	}
}


/*
 * SHift point by PIXel amount
 */

FUNCTION LOCAL_PROTO void fnt_SHPIX(GSP_DECL0)
{
	register F26Dot6 proj, dx, dy;

	dx = dy = 0;
	proj = CHECK_POP(LocalGS, LocalGS.stackPointer );
	if ( LocalGS.free.x )
		dx = VECTORMUL( proj, LocalGS.free.x );
	if ( LocalGS.free.y )
		dy = VECTORMUL( proj, LocalGS.free.y );

	fnt_SHP_Common(GSPPARAM1 dx, dy);
}

/*
 * Interpolate Point
 */

FUNCTION LOCAL_PROTO void fnt_IP(GSP_DECL0)
{
	F26Dot6 oldRange, currentRange;
	register ArrayIndex RP1 = LocalGS.Pt1;
	register ArrayIndex pt2 = LocalGS.Pt2;
	register fnt_ElementType* CE0 = LocalGS.CE0;
	register fnt_ElementType* CE1 = LocalGS.CE1;
	register fnt_ElementType* CE2 = LocalGS.CE2;
	register LoopCount loop;
	register Fixed multiplier = ONEFIX;

	/* It's Ok to use ox,oy instead of oox and ooy...*very* small approximation, but we get a more precise multiplier */
	currentRange	= LocalGS.Project( GSPPARAM1 CE1->x[pt2] - CE0->x[RP1], CE1->y[pt2] - CE0->y[RP1] );
	oldRange		= LocalGS.OldProject( GSPPARAM1 CE1->ox[pt2] - CE0->ox[RP1], CE1->oy[pt2] - CE0->oy[RP1] );

	if ( oldRange != 0 ) {
		multiplier = util_FixDiv( currentRange, oldRange );
	}
	for ( loop = LocalGS.loop; loop >= 0; --loop ) {
		register ArrayIndex arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
		register F26Dot6 tmp;
		
		tmp  = LocalGS.OldProject( GSPPARAM1 CE2->ox[arg1] - CE0->ox[RP1], CE2->oy[arg1] - CE0->oy[RP1] );
		tmp  = util_FixMul( tmp, multiplier );
		tmp -= LocalGS.Project( GSPPARAM1 CE2->x[arg1] - CE0->x[RP1], CE2->y[arg1] - CE0->y[RP1] ); /* delta = desired projection - current projection */
		LocalGS.MovePoint( GSPPARAM1 CE2, arg1, tmp );
	}
	LocalGS.loop = 0;
}

#ifdef OLDERVERSION
FUNCTION LOCAL_PROTO void fnt_IP(GSP_DECL0)
{
	F26Dot6 oldRange, currentRange;
	register ArrayIndex RP1 = LocalGS.Pt1;
	register ArrayIndex pt2 = LocalGS.Pt2;
	register fnt_ElementType* CE0 = LocalGS.CE0;
	register fnt_ElementType* CE1 = LocalGS.CE1;
	register fnt_ElementType* CE2 = LocalGS.CE2;
	register LoopCount loop;
#ifdef T2K_MATH
	register Fixed multiplier = ONEFIX;
#endif
	btsbool twilight = CE0 == LocalGS.elements || CE1 == LocalGS.elements || CE2 == LocalGS.elements;

	{
		currentRange = LocalGS.Project( GSPPARAM1 CE1->x[pt2] - CE0->x[RP1], CE1->y[pt2] - CE0->y[RP1] );
		if ( twilight ) {
			oldRange = LocalGS.OldProject( GSPPARAM1 CE1->ox[pt2] - CE0->ox[RP1], CE1->oy[pt2] - CE0->oy[RP1] );
		} else {
			oldRange = LocalGS.OldProject( GSPPARAM1 CE1->oox[pt2] - CE0->oox[RP1], CE1->ooy[pt2] - CE0->ooy[RP1] );
		}
	}
#ifdef T2K_MATH
	if ( oldRange != 0 ) {
		multiplier = util_FixDiv( currentRange, oldRange );
	}
	if ( currentRange == oldRange ) {
		printf("%d %d\n", currentRange, oldRange );
	}
#endif
	if ( twilight ) {
		for ( loop = LocalGS.loop; loop >= 0; --loop ) {
			register ArrayIndex arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
			register F26Dot6 tmp;
			
			tmp = LocalGS.OldProject( GSPPARAM1 CE2->ox[arg1] - CE0->ox[RP1], CE2->oy[arg1] - CE0->oy[RP1] );
			tmp = util_FixMul( tmp, multiplier );
			tmp -= LocalGS.Project( GSPPARAM1 CE2->x[arg1] - CE0->x[RP1], CE2->y[arg1] - CE0->y[RP1] ); /* delta = desired projection - current projection */
			LocalGS.MovePoint( GSPPARAM1 CE2, arg1, tmp );
		}
	} else {
		for ( loop = LocalGS.loop; loop >= 0; --loop ) {
			register ArrayIndex arg1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
			register F26Dot6 tmp;
			
			tmp = LocalGS.OldProject( GSPPARAM1 CE2->oox[arg1] - CE0->oox[RP1],  CE2->ooy[arg1] - CE0->ooy[RP1] );
			tmp = util_FixMul( tmp, multiplier );
			tmp -= LocalGS.Project( GSPPARAM1 CE2->x[arg1] - CE0->x[RP1], CE2->y[arg1] - CE0->y[RP1] ); /* delta = desired projection - current projection */
			LocalGS.MovePoint( GSPPARAM1 CE2, arg1, tmp );
		}
	}


	LocalGS.loop = 0;
}
#endif /* OLDERVERSION */

/*
 * Move Stack Indirect Relative Point
 */

FUNCTION LOCAL_PROTO void fnt_MSIRP(GSP_DECL0)
{
	register fnt_ElementType* CE0 = LocalGS.CE0;
	register fnt_ElementType* CE1 = LocalGS.CE1;
	register ArrayIndex Pt0 = LocalGS.Pt0;
	register F26Dot6 dist = CHECK_POP(LocalGS, LocalGS.stackPointer ); /* distance   */
	register ArrayIndex pt2 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer ); /* point #    */

	if ( CE1 == LocalGS.elements )
	{
		CE1->ox[pt2] = CE0->ox[Pt0] + VECTORMUL( dist, LocalGS.proj.x );
		CE1->oy[pt2] = CE0->oy[Pt0] + VECTORMUL( dist, LocalGS.proj.y );
		CE1->x[pt2] = CE1->ox[pt2];
		CE1->y[pt2] = CE1->oy[pt2];
	}
	dist -= LocalGS.Project( GSPPARAM1 CE1->x[pt2] - CE0->x[Pt0],
							  CE1->y[pt2] - CE0->y[Pt0] );
	LocalGS.MovePoint( GSPPARAM1 CE1, pt2, dist );
	LocalGS.Pt1 = Pt0;
	LocalGS.Pt2 = pt2;
	if ( TT_BIT0( LocalGS.opCode ) )
	{
		LocalGS.Pt0 = pt2; /* move the reference point */
	}
}


/*
 * Align Relative Point
 */

FUNCTION LOCAL_PROTO void fnt_ALIGNRP(GSP_DECL0)
{
	register fnt_ElementType* ce1 = LocalGS.CE1;
	register F26Dot6 pt0x = LocalGS.CE0->x[LocalGS.Pt0];
	register F26Dot6 pt0y = LocalGS.CE0->y[LocalGS.Pt0];

	for (; LocalGS.loop >= 0; --LocalGS.loop)
	{
		register ArrayIndex ptNum = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
		register F26Dot6 proj = -LocalGS.Project( GSPPARAM1 ce1->x[ptNum] - pt0x, ce1->y[ptNum] - pt0y );
		LocalGS.MovePoint( GSPPARAM1 ce1, ptNum, proj );
	}
	LocalGS.loop = 0;
}



/*
 * Align Two Points ( by moving both of them )
 */

FUNCTION LOCAL_PROTO void fnt_ALIGNPTS(GSP_DECL0)
{
	register ArrayIndex pt1, pt2;
	register F26Dot6 move1, dist;

	pt2  = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer ); /* point # 2   */
	pt1  = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer ); /* point # 1   */
		/* We do not have to check if we are in character element zero (the twilight zone)
		   since both points already have to have defined values before we execute this instruction */
	dist = LocalGS.Project( GSPPARAM1 LocalGS.CE0->x[pt2] - LocalGS.CE1->x[pt1],
							 LocalGS.CE0->y[pt2] - LocalGS.CE1->y[pt1] );

	move1 = dist >> 1;
	LocalGS.MovePoint( GSPPARAM1 LocalGS.CE0, pt1, move1 );
	LocalGS.MovePoint( GSPPARAM1 LocalGS.CE1, pt2, move1 - dist ); /* make sure the total movement equals tmp32 */
}


/*
 * Set Angle Weight
 */

FUNCTION LOCAL_PROTO void fnt_SANGW(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.angleWeight = (int16)CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/*
 * Does a cheap approximation of Euclidian distance.
 */

FUNCTION LOCAL_PROTO Fract fnt_QuickDist(register Fract dx, register Fract dy)
{
	if ( dx < 0 )
		dx = -dx;
	if ( dy < 0 )
		dy = -dy;

	return( dx > dy ? dx + ( dy >> 1 ) : dy + ( dx >> 1 ) );
}


/*
 * Flip Point
 */

FUNCTION LOCAL_PROTO void fnt_FLIPPT(GSP_DECL0)
{
	register uint8 *onCurve = LocalGS.CE0->onCurve;
	register F26Dot6* stack = LocalGS.stackPointer;
	register LoopCount count = LocalGS.loop;

	for (; count >= 0; --count)
	{
		register ArrayIndex point = (ArrayIndex)CHECK_POP(LocalGS, stack );
		onCurve[ point ] ^= ONCURVE;
	}
	LocalGS.loop = 0;

	LocalGS.stackPointer = stack;
}


/*
 * Flip On a Range
 */

FUNCTION LOCAL_PROTO void fnt_FLIPRGON(GSP_DECL0)
{
	register ArrayIndex lo, hi;
	register LoopCount count;
	register uint8 *onCurve = LocalGS.CE0->onCurve;
	register F26Dot6* stack = LocalGS.stackPointer;

	hi = (ArrayIndex)CHECK_POP(LocalGS, stack );
	CHECK_POINT( &LocalGS, LocalGS.CE0, hi );
	lo = (ArrayIndex)CHECK_POP(LocalGS, stack );
	CHECK_POINT( &LocalGS, LocalGS.CE0, lo );

	onCurve += lo;
	for (count = (LoopCount)(hi - lo); count >= 0; --count)
		*onCurve++ |= ONCURVE;
	LocalGS.stackPointer = stack;
}


/*
 * Flip On a Range
 */

FUNCTION LOCAL_PROTO void fnt_FLIPRGOFF(GSP_DECL0)
{
	register ArrayIndex lo, hi;
	register LoopCount count;
	register uint8 *onCurve = LocalGS.CE0->onCurve;

	hi = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	CHECK_POINT( &LocalGS, LocalGS.CE0, hi );
	lo = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	CHECK_POINT( &LocalGS, LocalGS.CE0, lo );

	onCurve += lo;
	for (count = (LoopCount)(hi - lo); count >= 0; --count)
		*onCurve++ &= ~ONCURVE;
}


/* 4/22/90 rwb - made more general
 * Sets lower 16 flag bits of ScanControl variable.  Sets scanContolIn if we are in one
 * of the preprograms; else sets scanControlOut.
 *
 * stack: value => -;
 *
 */

FUNCTION LOCAL_PROTO void fnt_SCANCTRL(GSP_DECL0)
{
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	register fnt_ParameterBlock *pb = &globalGS->localParBlock;

	pb->scanControl = (int32)(pb->scanControl & 0xFFFF0000) | CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/* 5/24/90 rwb
 * Sets upper 16 bits of ScanControl variable. Sets scanContolIn if we are in one
 * of the preprograms; else sets scanControlOut.
 */


FUNCTION LOCAL_PROTO void fnt_SCANTYPE(GSP_DECL0)
{
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	register fnt_ParameterBlock *pb = &globalGS->localParBlock;
	register int value = (int)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register int32 *scanPtr = &(pb->scanControl);

#ifdef OLD
	if ( value == 0 )
		*scanPtr &= 0xFFFF;
	else
		if ( value == 1 )
			*scanPtr = (*scanPtr & 0xFFFF) | STUBCONTROL;
		else
			if ( value == 2 )
				*scanPtr = (*scanPtr & 0xFFFF) | NODOCONTROL;
#endif
	switch ( value ) {
	case 0:
		*scanPtr = (*scanPtr & 0xFFFF) | ACTIVATE_DROPOUTCONTROL | INCLUDE_STUBS;
		break; /*****/
	case 1:
		*scanPtr = (*scanPtr & 0xFFFF) | ACTIVATE_DROPOUTCONTROL;
		break; /*****/
	case 2:
	case 3:
	case 6:
	case 7:
		*scanPtr = (*scanPtr & 0xFFFF) | 0;
		break; /*****/
		
	case 4:
		*scanPtr = (*scanPtr & 0xFFFF) | ACTIVATE_DROPOUTCONTROL | SMART_DROPOUT | INCLUDE_STUBS;
		break; /*****/
	case 5:
		*scanPtr = (*scanPtr & 0xFFFF) | ACTIVATE_DROPOUTCONTROL | SMART_DROPOUT;
		break; /*****/
	
	
	
	}

}


/* 6/28/90 rwb
 * Sets instructControl flags in global graphic state.  Only legal in pre program.
 * A selector is used to choose the flag to be set.
 * Bit0 - NOGRIDFITFLAG - if set, then truetype instructions are not executed.
 * 		A font may want to use the preprogram to check if the glyph is rotated or
 * 	 	transformed in such a way that it is better to not gridfit the glyphs.
 * Bit1 - DEFAULTFLAG - if set, then changes in localParameterBlock variables in the
 *		globalGraphics state made in the CVT preprogram are not copied back into
 *		the defaultParameterBlock.  So, the original default values are the starting
 *		values for each glyph.
 *
 * stack: value, selector => -;
 *
 */

FUNCTION LOCAL_PROTO void fnt_INSTCTRL(GSP_DECL0)   /* <13> */
{
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	register int32 *ic = &globalGS->localParBlock.instructControl;
	int selector 	= (int)CHECK_POP(LocalGS, LocalGS.stackPointer );
	int32 value 	= (int32)CHECK_POP(LocalGS, LocalGS.stackPointer );

	if( globalGS->init )
	{
		if( selector == 1 )
		{
			*ic &= ~NOGRIDFITFLAG;
			*ic |= (value & NOGRIDFITFLAG);
		}
		else
			if( selector == 2 )
			{
				*ic &= ~DEFAULTFLAG;
				*ic |= (value & DEFAULTFLAG);
			}
	}
}


/*
 * AdjustAngle         <4>
 */

FUNCTION LOCAL_PROTO void fnt_AA(GSP_DECL0)
{

	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	ArrayIndex ptNum, bestAngle;
	F26Dot6 dx, dy, tmp32;
	Fract pvx, pvy; /* Projection Vector */
	Fract pfProj;
	Fract tpvx, tpvy;
	Fract* anglePoint;		/* x,y, x,y, x,y, ... */
	int16 distance, *angleDistance;
	int32 minPenalty;			/* should this be the same as distance??? mrr-7/17/90 */
	LoopCount i;
	int yFlip, xFlip, xySwap;		/* boolean */


	ptNum = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
		/* save the projection vector */
	pvx = LocalGS.proj.x;
	pvy = LocalGS.proj.y;
	pfProj = LocalGS.pfProj;

	dx = LocalGS.CE1->x[ptNum] - LocalGS.CE0->x[LocalGS.Pt0];
	dy = LocalGS.CE1->y[ptNum] - LocalGS.CE0->y[LocalGS.Pt0];

		/* map to the first and second quadrant */
	yFlip = dy < 0 ? dy = -dy, 1 : 0;

		/* map to the first quadrant */
	xFlip = dx < 0 ? dx = -dx, 1 : 0;

		/* map to the first octant */
	xySwap = dy > dx ? tmp32 = dy, dy = dx, dx = tmp32, 1 : 0;

		/* Now tpvy, tpvx contains the line rotated by 90 degrees, so it is in the 3rd octant */
	{
		VECTOR v;
		fnt_Normalize( -dy, dx, &v );
		tpvx = v.x;
		tpvy = v.y;
	}

		/* find the best angle */
	minPenalty = 10 * fnt_pixelSize; bestAngle = -1;
	anglePoint = &globalGS->anglePoint[0].x;		/* x,y, x,y, x,y, ... */
	angleDistance = globalGS->angleDistance;
	for ( i = 0; i < MAXANGLES; i++ )
	{
		if ( (distance = *angleDistance++) >= minPenalty )
			break; /* No more improvement is possible */
		LocalGS.proj.x = (VECTORTYPE)*anglePoint++;
		LocalGS.proj.y = (VECTORTYPE)*anglePoint++;
			/* Now find the distance between these vectors, this will help us gain speed */
		if ( fnt_QuickDist( LocalGS.proj.x - tpvx, LocalGS.proj.y - tpvy ) > ( 210831287 ) ) /* 2PI / 32 */
			continue; /* Difference is to big, we will at most change the angle +- 360/32 = +- 11.25 degrees */

		tmp32 = fnt_Project( GSPPARAM1 dx, dy ); /* Calculate the projection */
		if ( tmp32 < 0 )
			tmp32 = -tmp32;

		tmp32 = ( globalGS->localParBlock.angleWeight * tmp32 ) >> fnt_pixelShift;
		tmp32 +=  distance;
		if ( tmp32 < minPenalty )
		{
			minPenalty = tmp32;
			bestAngle = i;
		}
	}

	tmp32 = 0;
	if ( bestAngle >= 0 )
	{
		/* OK, we found a good angle */
		LocalGS.proj.x = (VECTORTYPE)globalGS->anglePoint[bestAngle].x;
		LocalGS.proj.y = (VECTORTYPE)globalGS->anglePoint[bestAngle].y;
		/* Fold the projection vector back into the full coordinate plane. */
		if ( xySwap )
		{
			tmp32 = (F26Dot6)LocalGS.proj.y;
			LocalGS.proj.y = LocalGS.proj.x;
			LocalGS.proj.x = (VECTORTYPE)tmp32;
		}
		if ( xFlip )
		{
			LocalGS.proj.x =  (VECTORTYPE)-LocalGS.proj.x;
		}
		if ( yFlip )
		{
			LocalGS.proj.y =  (VECTORTYPE)-LocalGS.proj.y;
		}
		fnt_ComputeAndCheck_PF_Proj( GSPPARAM0 );

		tmp32 = fnt_Project( GSPPARAM1 LocalGS.CE1->x[LocalGS.Pt0] - LocalGS.CE0->x[ptNum], LocalGS.CE1->y[LocalGS.Pt0] - LocalGS.CE0->y[ptNum] );
	}
	fnt_MovePoint( GSPPARAM1 LocalGS.CE1, ptNum, tmp32 );

	LocalGS.proj.x = (VECTORTYPE)pvx; /* restore the projection vector */
	LocalGS.proj.y = (VECTORTYPE)pvy;
	LocalGS.pfProj = (VECTORTYPE)pfProj;
}


/*
 *	Called by fnt_PUSHB and fnt_NPUSHB
 */

FUNCTION LOCAL_PROTO void fnt_PushSomeStuff(GSP_DECL1 register LoopCount count, btsbool pushBytes)
{
	register F26Dot6* stack = LocalGS.stackPointer;
	register uint8* instr = LocalGS.insPtr;

	if (pushBytes)
		for (--count; count >= 0; --count)
			CHECK_PUSH( LocalGS, stack, GETBYTE( instr ));
	else
	{
		for (--count; count >= 0; --count)
		{
			int16 word = *instr++;
			CHECK_PUSH( LocalGS, stack, (int16)((word << 8) + *instr++));
		}
	}
	LocalGS.stackPointer = stack;
	LocalGS.insPtr = instr;
}


/*
 * PUSH Bytes		<3>
 */

FUNCTION LOCAL_PROTO void fnt_PUSHB(GSP_DECL0)
{
	fnt_PushSomeStuff(GSPPARAM1 (LoopCount)(LocalGS.opCode - 0xb0 + 1), true);
}


/*
 * N PUSH Bytes
 */

FUNCTION LOCAL_PROTO void fnt_NPUSHB(GSP_DECL0)
{
	fnt_PushSomeStuff(GSPPARAM1 GETBYTE( LocalGS.insPtr ), true);
}


/*
 * PUSH Words		<3>
 */

FUNCTION LOCAL_PROTO void fnt_PUSHW(GSP_DECL0)
{
	fnt_PushSomeStuff(GSPPARAM1 (LoopCount)(LocalGS.opCode - 0xb8 + 1), false);
}


/*
 * N PUSH Words
 */

FUNCTION LOCAL_PROTO void fnt_NPUSHW(GSP_DECL0)
{
	fnt_PushSomeStuff(GSPPARAM1 GETBYTE( LocalGS.insPtr ), false);
}


/*
 * Write Store
 */

FUNCTION LOCAL_PROTO void fnt_WS(GSP_DECL0)
{
	register F26Dot6 storage;
	register ArrayIndex storeIndex;

	storage = CHECK_POP(LocalGS, LocalGS.stackPointer );
	storeIndex = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

#ifdef USE_SEAT_BELTS
	if ( storeIndex < 0 || storeIndex > LocalGS.globalGS->maxp->maxStorage-1 ) {
		storeIndex = LocalGS.globalGS->maxp->maxStorage-1;
	}
#endif
	CHECK_STORAGE( &LocalGS, storeIndex );

	LocalGS.globalGS->store[ storeIndex ] = storage;
}


/*
 * Read Store
 */

FUNCTION LOCAL_PROTO void fnt_RS(GSP_DECL0)
{
	register ArrayIndex storeIndex;

	storeIndex = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
#ifdef USE_SEAT_BELTS
	if ( storeIndex < 0 || storeIndex > LocalGS.globalGS->maxp->maxStorage-1 ) {
		storeIndex = LocalGS.globalGS->maxp->maxStorage-1;
	}
#endif
	CHECK_STORAGE( &LocalGS, storeIndex );
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.globalGS->store[storeIndex] );
}


/*
 * Write Control Value Table from outLine, assumes the value comes form the outline domain
 */

FUNCTION LOCAL_PROTO void fnt_WCVT(GSP_DECL0)
{
	register ArrayIndex cvtIndex;
	register F26Dot6 cvtValue;
	register F16Dot16 cvtScale = fnt_GetCVTScale( GSPPARAM0 );

	cvtValue = CHECK_POP(LocalGS, LocalGS.stackPointer );
	cvtIndex = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_CVT( &LocalGS, cvtIndex );
	
	if ( cvtScale != ONEFIX && cvtScale != 0 ) {
		LocalGS.globalGS->controlValueTable[ cvtIndex ] = FixDiv( cvtValue, cvtScale );
	} else {
		LocalGS.globalGS->controlValueTable[ cvtIndex ] = cvtValue;
	}
}


/*
 * Write Control Value Table From Original Domain, assumes the value comes from the original domain, not the cvt or outline
 */

FUNCTION LOCAL_PROTO void fnt_WCVTFOD(GSP_DECL0)
{
	register ArrayIndex cvtIndex;
	register F26Dot6 cvtValue;
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;

	cvtValue = CHECK_POP(LocalGS, LocalGS.stackPointer );
	cvtIndex = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	CHECK_CVT( &LocalGS, cvtIndex );
	globalGS->controlValueTable[ cvtIndex ] = globalGS->ScaleFunc( globalGS, cvtValue );
}


/*
 * Read Control Value Table
 */

FUNCTION LOCAL_PROTO void fnt_RCVT(GSP_DECL0)
{
	register ArrayIndex cvtIndex;

	cvtIndex = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.GetCVTEntry( GSPPARAM1 cvtIndex ) );
}


/*
 * Read Coordinate
 */

FUNCTION LOCAL_PROTO void fnt_RC(GSP_DECL0)
{
	ArrayIndex pt;
	fnt_ElementType *element;
	register F26Dot6 proj;

	pt = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	element = LocalGS.CE2;

	if ( TT_BIT0( LocalGS.opCode ) )
		proj = LocalGS.OldProject( GSPPARAM1 element->ox[pt], element->oy[pt] );
	else
		proj = LocalGS.Project( GSPPARAM1 element->x[pt], element->y[pt] );

	CHECK_PUSH( LocalGS, LocalGS.stackPointer, proj );
}


/*
 * Write Coordinate
 */

FUNCTION LOCAL_PROTO void fnt_WC(GSP_DECL0)
{
	register F26Dot6 proj, coord;
	register ArrayIndex pt;
	register fnt_ElementType *element;

	coord = CHECK_POP(LocalGS, LocalGS.stackPointer );/* value */
	pt = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );/* point */
	element = LocalGS.CE2;

	proj = LocalGS.Project( GSPPARAM1 element->x[pt],  element->y[pt] );
	proj = coord - proj;

	LocalGS.MovePoint( GSPPARAM1 element, pt, proj );

	if (element == LocalGS.elements)		/* twilightzone */
	{
		element->ox[pt] = element->x[pt];
		element->oy[pt] = element->y[pt];
	}
}


/*
 * Measure Distance
 */

FUNCTION LOCAL_PROTO void fnt_MD(GSP_DECL0)
{
	register ArrayIndex pt1, pt2;
	register F26Dot6 proj, *stack = LocalGS.stackPointer;
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;

	pt2 = (ArrayIndex)CHECK_POP(LocalGS, stack );
	pt1 = (ArrayIndex)CHECK_POP(LocalGS, stack );
	if ( TT_BIT0( LocalGS.opCode - MD_CODE ) )
	{
		proj  = LocalGS.OldProject( GSPPARAM1 LocalGS.CE0->oox[pt1] - LocalGS.CE1->oox[pt2],
								     LocalGS.CE0->ooy[pt1] - LocalGS.CE1->ooy[pt2] );
		proj = globalGS->ScaleFunc( globalGS, proj );
	}
	else
		proj  = LocalGS.Project( GSPPARAM1 LocalGS.CE0->x[pt1] - LocalGS.CE1->x[pt2],
								  LocalGS.CE0->y[pt1] - LocalGS.CE1->y[pt2] );
	CHECK_PUSH( LocalGS, stack, proj );
	LocalGS.stackPointer = stack;
}


/*
 * Measure Pixels Per EM
 */

FUNCTION LOCAL_PROTO void fnt_MPPEM(GSP_DECL0)
{
	register uint16 ppem;
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;

	ppem = globalGS->pixelsPerEm;

	if ( !globalGS->identityTransformation ) {
		ppem = (uint16)((FixMul( ppem+ppem, fnt_GetCVTScale( GSPPARAM0 ) ) + 1) >> 1); /* May 22, 2000 */
		/* ppem = (uint16)FixMul( ppem, fnt_GetCVTScale( GSPPARAM0) ); */
        }
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, ppem );
}


/*
 * Measure Point Size
 */

FUNCTION LOCAL_PROTO void fnt_MPS(GSP_DECL0)
{
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, LocalGS.globalGS->pointSize );
}


/*
 * Get Miscellaneous info: version number, rotated, stretched 	<6>
 * Version number is 8 bits.  This is version 0x01 : 5/1/90
 *
 */

FUNCTION LOCAL_PROTO void fnt_GETINFO(GSP_DECL0)
{
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	register int selector = (int)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register int info = 0;

	if( selector & 1)								/* version */
		info |= 1;
	if( (selector & 2) && (globalGS->non90DegreeTransformation & 0x1) )
		info |= ROTATEDGLYPH;
	if( (selector & 4) &&  (globalGS->non90DegreeTransformation & 0x2))
		info |= STRETCHEDGLYPH;
	CHECK_PUSH( LocalGS, LocalGS.stackPointer, info );
}


/*
 * FLIP ON
 */

FUNCTION LOCAL_PROTO void fnt_FLIPON(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.autoFlip = true;
}


/*
 * FLIP OFF
 */

FUNCTION LOCAL_PROTO void fnt_FLIPOFF(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.autoFlip = false;
}


#ifndef NOT_ON_THE_MAC
#ifdef DEBUG
/*
 * DEBUG
 */

FUNCTION LOCAL_PROTO void fnt_DEBUG(GSP_DECL0)
{
	register int32 arg;
	int8 buffer[24];

	arg = CHECK_POP(LocalGS, LocalGS.stackPointer );

	buffer[1] = 'D';
	buffer[2] = 'E';
	buffer[3] = 'B';
	buffer[4] = 'U';
	buffer[5] = 'G';
	buffer[6] = ' ';

	if ( arg >= 0 )
	{
		buffer[7] = '+';
	}
	else
	{
		arg = -arg;
		buffer[7] = '-';
	}

	buffer[13] = arg % 10 + '0';
	arg /= 10;
	buffer[12] = arg % 10 + '0';
	arg /= 10;
	buffer[11] = arg % 10 + '0';
	arg /= 10;
	buffer[10] = arg % 10 + '0';
	arg /= 10;
	buffer[ 9] = arg % 10 + '0';
	arg /= 10;
	buffer[ 8] = arg % 10 + '0';
	arg /= 10;

	buffer[14] = arg ? '*' : ' ';


	buffer[0] = 14; /* convert to pascal */
	DebugStr( buffer );
}

#else		/* debug */


FUNCTION LOCAL_PROTO void fnt_DEBUG(GSP_DECL0)
{
	UNUSED( CHECK_POP(LocalGS, LocalGS.stackPointer ) );
}

#endif		/* debug */
#else

FUNCTION LOCAL_PROTO void fnt_DEBUG(GSP_DECL0)
{
	UNUSED( CHECK_POP(LocalGS, LocalGS.stackPointer ) );
}

#endif		/* ! not on the mac */


/*
 *	This guy is here to save space for simple insructions
 *	that pop two arguments and push one back on.
 */

FUNCTION LOCAL_PROTO void fnt_BinaryOperand(GSP_DECL0)
{
	F26Dot6* stack = LocalGS.stackPointer;
	F26Dot6 arg2 = CHECK_POP(LocalGS, stack );
	F26Dot6 arg1 = CHECK_POP(LocalGS, stack );

	switch (LocalGS.opCode)
	{
	case LT_CODE:
		BOOLEANPUSH( stack, arg1 < arg2 );
		break;
	case LTEQ_CODE:
		BOOLEANPUSH( stack, arg1 <= arg2 );
		break;
	case GT_CODE:
		BOOLEANPUSH( stack, arg1 > arg2 );
		break;
	case GTEQ_CODE:
		BOOLEANPUSH( stack, arg1 >= arg2 );
		break;
	case EQ_CODE:
		BOOLEANPUSH( stack, arg1 == arg2 );
		break;
	case NEQ_CODE:
		BOOLEANPUSH( stack, arg1 != arg2 );
		break;

	case AND_CODE:
		BOOLEANPUSH( stack, arg1 && arg2 );
		break;
	case OR_CODE:
		BOOLEANPUSH( stack, arg1 || arg2 );
		break;

	case ADD_CODE:
		CHECK_PUSH( LocalGS, stack, arg1 + arg2 );
		break;
	case SUB_CODE:
		CHECK_PUSH( LocalGS, stack, arg1 - arg2 );
		break;
	case MUL_CODE:
		#ifdef T2K_MATH
			/* CHECK_PUSH( LocalGS, stack, arg1 * arg2 / 64); */
			CHECK_PUSH( LocalGS, stack, util_FixMul26Dot6( arg1, arg2 ));
		#else 
			CHECK_PUSH( LocalGS, stack, Mul26Dot6( arg1, arg2 ));
		#endif
		break;
	case DIV_CODE:
		#ifdef T2K_MATH
			CHECK_PUSH( LocalGS, stack, ( (arg1 << 6) / arg2 ) );
		#else
			CHECK_PUSH( LocalGS, stack, Div26Dot6( arg1, arg2 ));
		#endif
		break;
	case MAX_CODE:
		if (arg1 < arg2)
			arg1 = arg2;
		CHECK_PUSH( LocalGS, stack, arg1 );
		break;
	case MIN_CODE:
		if (arg1 > arg2)
			arg1 = arg2;
		CHECK_PUSH( LocalGS, stack, arg1 );
		break;
#ifdef DEBUG
	default:
		Debugger();
#endif
	}
	LocalGS.stackPointer = stack;
	CHECK_STACK(&LocalGS);
}



FUNCTION LOCAL_PROTO void fnt_UnaryOperand(GSP_DECL0)
{
	F26Dot6* stack = LocalGS.stackPointer;
	F26Dot6 arg = CHECK_POP(LocalGS, stack );
	int opCode = LocalGS.opCode;

	switch (opCode)
	{
	case ODD_CODE:
	case EVEN_CODE:
		arg = fnt_RoundToGrid( GSPPARAM1 arg, 0L );
		arg >>= fnt_pixelShift;
		if ( opCode == ODD_CODE )
			arg++;
		BOOLEANPUSH( stack, (arg & 1) == 0 );
		break;
	case NOT_CODE:
		BOOLEANPUSH( stack, !arg );
		break;

	case ABS_CODE:
		CHECK_PUSH( LocalGS, stack, arg > 0 ? arg : -arg );
		break;
	case NEG_CODE:
		CHECK_PUSH( LocalGS, stack, -arg );
		break;

	case CEILING_CODE:
		arg += fnt_pixelSize - 1;
	case FLOOR_CODE:
		arg &= ~(fnt_pixelSize-1);
		CHECK_PUSH( LocalGS, stack, arg );
		break;
#ifdef DEBUG
	default:
		Debugger();
#endif
	}
	LocalGS.stackPointer = stack;
	CHECK_STACK(&LocalGS);
}


#define NPUSHB_CODE 0x40
#define NPUSHW_CODE 0x41

#define PUSHB_START 0xb0
#define PUSHB_END 	0xb7
#define PUSHW_START 0xb8
#define PUSHW_END 	0xbf

/*
 * Internal function for fnt_IF(), and fnt_FDEF()
 */

FUNCTION LOCAL_PROTO void fnt_SkipPushCrap(GSP_DECL0)
{
	register int opCode = LocalGS.opCode;
	register uint8* instr = LocalGS.insPtr;
	register ArrayIndex count;

	switch(opCode)
	{
		case NPUSHB_CODE:
			count = (ArrayIndex)*instr++;
			instr += count;
			break;
		case NPUSHW_CODE:
			count = (ArrayIndex)*instr++;
			instr += count + count;
			break;
		case PUSHB_START:
		case PUSHB_START+1:
		case PUSHB_START+2:
		case PUSHB_START+3:
		case PUSHB_START+4:
		case PUSHB_START+5:
		case PUSHB_START+6:
		case PUSHB_START+7:
			count = (ArrayIndex)(opCode - PUSHB_START + 1);
			instr += count;
			break;
		case PUSHW_START:
		case PUSHW_START+1:
		case PUSHW_START+2:
		case PUSHW_START+3:
		case PUSHW_START+4:
		case PUSHW_START+5:
		case PUSHW_START+6:
		case PUSHW_START+7:
			count = (ArrayIndex)(opCode - PUSHW_START + 1);
			instr += count + count;
			break;
	}
	LocalGS.insPtr = instr;
}


/*
 * IF
 */

FUNCTION LOCAL_PROTO void fnt_IF(GSP_DECL0)
{
	register int level;

	if ( ! CHECK_POP(LocalGS, LocalGS.stackPointer ) )
	{
		/* Now skip instructions */
		for ( level = 1; level; ) 
		{
			/* level = # of "ifs" minus # of "endifs" */
			switch(LocalGS.opCode = *LocalGS.insPtr++)
			{
				case EIF_CODE:
					level--;
					break;
				case IF_CODE:
					level++;
					break;
				case ELSE_CODE:
					if (level == 1)
						level = 0;	/* kill the loop */
					break;
				default:
					fnt_SkipPushCrap( GSPPARAM0 );
					break;
			}
		}
	}
}


/*
 *	ELSE for the IF
 */

FUNCTION LOCAL_PROTO void fnt_ELSE(GSP_DECL0)
{
	register int level;

	for ( level = 1; level; ) 
	{
		/* level = # of "ifs" minus # of "endifs" */
		switch(LocalGS.opCode = *LocalGS.insPtr++)
		{
			case EIF_CODE:
				level--;
				break;
			case IF_CODE:
				level++;
				break;
			default:
				fnt_SkipPushCrap( GSPPARAM0 );
				break;
		}
	}
}


/*
 * End IF
 */

FUNCTION LOCAL_PROTO void fnt_EIF(GSP_DECL0)
{
	#if REENTRANT_ALLOC
		UNUSED( pLocalGS );
	#endif
}


/*
 * Jump Relative
 */

FUNCTION LOCAL_PROTO void fnt_JMPR(GSP_DECL0)
{
	register ArrayIndex offset;

	offset = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	offset--; /* since the interpreter post-increments the IP */
	LocalGS.insPtr += offset;
}


/*
 * Jump Relative On True
 */

FUNCTION LOCAL_PROTO void fnt_JROT(GSP_DECL0)
{
	register ArrayIndex offset;
	register F26Dot6* stack = LocalGS.stackPointer;

	if ( CHECK_POP(LocalGS, stack ) )
	{
		offset = (ArrayIndex)CHECK_POP(LocalGS, stack );
		--offset; /* since the interpreter post-increments the IP */
		LocalGS.insPtr += offset;
	}
	else
	{
		--stack;/* same as POP */
	}
	LocalGS.stackPointer = stack;
}


/*
 * Jump Relative On False
 */

FUNCTION LOCAL_PROTO void fnt_JROF(GSP_DECL0)
{
	register ArrayIndex offset;
	register F26Dot6* stack = LocalGS.stackPointer;

	if ( CHECK_POP(LocalGS, stack ) )
	{
		--stack;/* same as POP */
	}
	else
	{
		offset = (ArrayIndex)CHECK_POP(LocalGS, stack );
		offset--; /* since the interpreter post-increments the IP */
		LocalGS.insPtr += offset;
	}
	LocalGS.stackPointer = stack;
}


/*
 * ROUND
 */

FUNCTION LOCAL_PROTO void fnt_ROUND(GSP_DECL0)
{
	register F26Dot6 arg1;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	arg1 = CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_RANGE( LocalGS.opCode, 0x68, 0x6B );

	/* arg1 = pb->RoundValue( GSPPARAM1 arg1, LocalGS.globalGS->engine[LocalGS.opCode - 0x68]); */
	arg1 = pb->RoundValue( GSPPARAM1 arg1, 0);
	CHECK_PUSH( LocalGS, LocalGS.stackPointer , arg1 );
}


/*
 * No ROUND
 */

FUNCTION LOCAL_PROTO void fnt_NROUND(GSP_DECL0)
{
	register F26Dot6 arg1;

	arg1 = CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_RANGE( LocalGS.opCode, 0x6C, 0x6F );

	/* arg1 = fnt_RoundOff( GSPPARAM1 arg1, LocalGS.globalGS->engine[LocalGS.opCode - 0x6c] ); */
	/* arg1 = fnt_RoundOff( GSPPARAM1 arg1, 0 ); */
	CHECK_PUSH( LocalGS, LocalGS.stackPointer , arg1 );
}


/*
 * An internal function used by MIRP an MDRP.
 */

FUNCTION LOCAL_PROTO F26Dot6 fnt_CheckSingleWidth(GSP_DECL1 register F26Dot6 value)
{
	register F26Dot6 delta, scaledSW;
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;

	scaledSW = LocalGS.GetSingleWidth( GSPPARAM0 );

	if ( value >= 0 )
	{
		delta = value - scaledSW;
		if ( delta < 0 )
			delta = -delta;
		if ( delta < pb->sWCI )
			value = scaledSW;
	}
	else
	{
		value = -value;
		delta = value - scaledSW;
		if ( delta < 0 )
			delta = -delta;
		if ( delta < pb->sWCI )
			value = scaledSW;
		value = -value;
	}
	return value;
}


/*
 * Move Direct Relative Point
 */

FUNCTION LOCAL_PROTO void fnt_MDRP(GSP_DECL0)
{
	register ArrayIndex pt1, pt0 = LocalGS.Pt0;
	register F26Dot6 tmp, tmpC;
	register fnt_ElementType *element = LocalGS.CE1;
	register fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	register fnt_ParameterBlock *pb = &globalGS->localParBlock;

	pt1 = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_POINT(&LocalGS, LocalGS.CE0, pt0);
	CHECK_POINT(&LocalGS, element, pt1);

	if ( LocalGS.CE0 == LocalGS.elements || element == LocalGS.elements )
	{
		tmp  = LocalGS.OldProject( GSPPARAM1 element->ox[pt1] - LocalGS.CE0->ox[pt0],
								     element->oy[pt1] - LocalGS.CE0->oy[pt0] );
	}
	else
	{
		tmp  = LocalGS.OldProject( GSPPARAM1 element->oox[pt1] - LocalGS.CE0->oox[pt0],
								     element->ooy[pt1] - LocalGS.CE0->ooy[pt0] );
	    tmp = globalGS->ScaleFunc( globalGS, tmp );
	}

	if ( pb->sWCI )
	{
		tmp = fnt_CheckSingleWidth( GSPPARAM1 tmp );
	}

	tmpC = tmp;
	if ( TT_BIT2( LocalGS.opCode )  )
	{
		/* tmp = pb->RoundValue( GSPPARAM1 tmp, globalGS->engine[LocalGS.opCode & 0x03] ); */
		tmp = pb->RoundValue( GSPPARAM1 tmp, 0 );
	}
#ifdef OLD
	else
	{
		/* tmp = fnt_RoundOff( GSPPARAM1 tmp, globalGS->engine[LocalGS.opCode & 0x03] ); */
		tmp = fnt_RoundOff( GSPPARAM1 tmp, 0 );
	}
#endif


	if ( TT_BIT3( LocalGS.opCode ) )
	{
		F26Dot6 tmpB = pb->minimumDistance;
		if ( tmpC >= 0 )
		{
			if ( tmp < tmpB )
			{
				tmp = tmpB;
			}
		}
		else
		{
			tmpB = -tmpB;
			if ( tmp > tmpB )
			{
				tmp = tmpB;
			}
		}
	}

	tmpC = LocalGS.Project( GSPPARAM1 element->x[pt1] - LocalGS.CE0->x[pt0],
							element->y[pt1] - LocalGS.CE0->y[pt0] );
	tmp -= tmpC;
	LocalGS.MovePoint( GSPPARAM1 element, pt1, tmp );
	LocalGS.Pt1 = pt0;
	LocalGS.Pt2 = pt1;
	if ( TT_BIT4( LocalGS.opCode ) )
	{
		LocalGS.Pt0 = pt1; /* move the reference point */
	}
}


/*
 * Move Indirect Relative Point
 */

FUNCTION LOCAL_PROTO void fnt_MIRP(GSP_DECL0)
{
	register ArrayIndex ptNum;
			 ArrayIndex Pt0;
	register F26Dot6 tmp, tmpB, tmpC, tmpProj;
	/* register F26Dot6 *engine = LocalGS.globalGS->engine; */
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_ElementType *pCE0 = LocalGS.CE0;
	fnt_ElementType *pCE1 = LocalGS.CE1;
	unsigned opCode = (unsigned) LocalGS.opCode;

	tmp = LocalGS.GetCVTEntry ( GSPPARAM1 (ArrayIndex)CHECK_POP (LocalGS, LocalGS.stackPointer));

	if (pb->sWCI) tmp = fnt_CheckSingleWidth (GSPPARAM1 tmp);

	ptNum = (ArrayIndex)CHECK_POP (LocalGS, LocalGS.stackPointer);
	LocalGS.Pt1 = Pt0 = LocalGS.Pt0;
	LocalGS.Pt2 = ptNum;

	if (TT_BIT4 (opCode)) LocalGS.Pt0 = ptNum; /* Move the reference point. */

	if (pCE1 == LocalGS.elements) {
		pCE1->x[ptNum] = pCE1->ox[ptNum] = pCE0->ox[Pt0] + (F26Dot6) VECTORMUL(tmp, LocalGS.proj.x);
		pCE1->y[ptNum] = pCE1->oy[ptNum] = pCE0->oy[Pt0] + (F26Dot6) VECTORMUL(tmp, LocalGS.proj.y);
	}

	if (LocalGS.OldProject == (FntProjFunc)fnt_XProject)
		tmpC = pCE1->ox[ptNum] - pCE0->ox[Pt0];
	else if (LocalGS.OldProject == (FntProjFunc)fnt_YProject)
		tmpC = pCE1->oy[ptNum] - pCE0->oy[Pt0];
	else
		tmpC  = (*LocalGS.OldProject) (GSPPARAM1 pCE1->ox[ptNum] - pCE0->ox[Pt0], pCE1->oy[ptNum] - pCE0->oy[Pt0]);

	if (LocalGS.Project == (FntProjFunc)fnt_XProject)
		tmpProj = pCE1->x[ptNum] - pCE0->x[Pt0];
	else if (LocalGS.Project == (FntProjFunc)fnt_YProject)
		tmpProj = pCE1->y[ptNum] - pCE0->y[Pt0];
	else
		tmpProj  = (*LocalGS.Project) (GSPPARAM1 pCE1->x[ptNum] - pCE0->x[Pt0], pCE1->y[ptNum] - pCE0->y[Pt0]);

	if (pb->autoFlip && (((tmpC ^ tmp)) < 0) ) tmp = -tmp; /* Do the auto flip */
			

	if (TT_BIT2 (opCode)) {
		tmpB = tmp - tmpC;
		if (tmpB < 0) tmpB = -tmpB;
		if (tmpB > pb->wTCI) tmp = tmpC;
		/* tmp = pb->RoundValue (GSPPARAM1 tmp, engine[opCode & 0x03]); */
		tmp = pb->RoundValue (GSPPARAM1 tmp, 0 );
	}
		/* else */
		/* tmp = fnt_RoundOff (GSPPARAM1 tmp, engine[opCode & 0x03]); */
		/* tmp = fnt_RoundOff (GSPPARAM1 tmp, 0); */

	if (TT_BIT3 (opCode)) {
		tmpB = pb->minimumDistance;
		if (tmpC >= 0) {
			if (tmp < tmpB) tmp = tmpB;
		} else {
			tmpB = -tmpB;
			if (tmp > tmpB) tmp = tmpB;
		}
	}

	(*LocalGS.MovePoint) (GSPPARAM1 pCE1, ptNum, tmp - tmpProj);
}


/*
 * CALL a function
 */

FUNCTION LOCAL_PROTO void fnt_CALL(GSP_DECL0)
{
	register fnt_funcDef *funcDef;
	uint8 *ins;
	fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	ArrayIndex arg = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_ASSERTION( globalGS->funcDef != 0 );
	CHECK_FDEF( &LocalGS, arg );
	funcDef = &globalGS->funcDef[ arg ];

	CHECK_PROGRAM(funcDef->pgmIndex);
	ins     = globalGS->pgmList[ funcDef->pgmIndex ];

	CHECK_ASSERTION( ins != 0 );

	ins += funcDef->start;
	LocalGS.Interpreter( GSPPARAM1 ins, ins + funcDef->length);
}


/*
 * Function DEFinition
 */

FUNCTION LOCAL_PROTO void fnt_FDEF(GSP_DECL0)
{
	register fnt_funcDef *funcDef;
	uint8* program, *funcStart;
	fnt_GlobalGraphicStateType *globalGS = LocalGS.globalGS;
	ArrayIndex arg = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );

	CHECK_FDEF( &LocalGS, arg );

	funcDef = &globalGS->funcDef[ arg ];
	program = globalGS->pgmList[ funcDef->pgmIndex = globalGS->pgmIndex ];

	CHECK_PROGRAM(funcDef->pgmIndex);
	CHECK_ASSERTION( globalGS->funcDef != 0 );
	CHECK_ASSERTION( globalGS->pgmList[funcDef->pgmIndex] != 0 );

	funcDef->start = LocalGS.insPtr - program;
	funcStart = LocalGS.insPtr;
	while ( (LocalGS.opCode = *LocalGS.insPtr++) != ENDF_CODE )
		fnt_SkipPushCrap( GSPPARAM0 );

	funcDef->length = (uint16)(LocalGS.insPtr - funcStart - 1); /* don't execute ENDF */
}


/*
 * LOOP while CALLing a function
 */

FUNCTION LOCAL_PROTO void fnt_LOOPCALL(GSP_DECL0)
{
	register uint8 *start, *stop;
	register InterpreterFunc Interpreter;
	register fnt_funcDef *funcDef;
	ArrayIndex arg = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register LoopCount loop;

	CHECK_FDEF( &LocalGS, arg );

	funcDef	= &(LocalGS.globalGS->funcDef[ arg ]);
	{
		uint8* ins;

		CHECK_PROGRAM(funcDef->pgmIndex);
		ins = LocalGS.globalGS->pgmList[ funcDef->pgmIndex ];

		start		= &ins[funcDef->start];
		stop		= &ins[funcDef->start + funcDef->length];	/* funcDef->end -> funcDef->length <4> */
	}
	Interpreter = LocalGS.Interpreter;
	loop = (LoopCount)CHECK_POP(LocalGS, LocalGS.stackPointer );
	for (--loop; loop >= 0; --loop )
		Interpreter( GSPPARAM1 start, stop );
}


/*
 *	This guy returns the index of the given opCode, or 0 if not found <4>
 */

FUNCTION LOCAL_PROTO fnt_instrDef* fnt_FindIDef(GSP_DECL1 register uint8 opCode)
{
	register fnt_GlobalGraphicStateType* globalGS = LocalGS.globalGS;
	register LoopCount count = globalGS->instrDefCount;
	register fnt_instrDef* instrDef = globalGS->instrDef;

	for (--count; count >= 0; instrDef++, --count)
		if (instrDef->opCode == opCode)
			return instrDef;

	return 0;
}


/*
 *	This guy gets called for opCodes that has been patch by the font's IDEF	<4>
 *	or if they have never been defined.  If there is no corresponding IDEF,
 *	flag it as an illegal instruction.
 */

FUNCTION LOCAL_PROTO void fnt_IDefPatch(GSP_DECL0)
{
	register fnt_instrDef* instrDef = fnt_FindIDef(GSPPARAM1 (uint8)LocalGS.opCode);
	if (instrDef == 0)
		fnt_IllegalInstruction( GSPPARAM0 );
	else
	{
		register uint8* program;

		CHECK_PROGRAM(instrDef->pgmIndex);
		program = LocalGS.globalGS->pgmList[ instrDef->pgmIndex ];

		program += instrDef->start;
		LocalGS.Interpreter( GSPPARAM1 program, program + instrDef->length);
	}
}


/*
 * Instruction DEFinition	<4>
 */

FUNCTION LOCAL_PROTO void fnt_IDEF(GSP_DECL0)
{
	register uint8 opCode = (uint8)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register fnt_instrDef* instrDef = fnt_FindIDef(GSPPARAM1 opCode);
	register ArrayIndex pgmIndex = (ArrayIndex)LocalGS.globalGS->pgmIndex;
	uint8* program = LocalGS.globalGS->pgmList[ pgmIndex ];
	uint8* instrStart = LocalGS.insPtr;

	CHECK_PROGRAM(pgmIndex);

	if (!instrDef)
		instrDef = LocalGS.globalGS->instrDef + LocalGS.globalGS->instrDefCount++;

	instrDef->pgmIndex = (uint8)pgmIndex;
	instrDef->opCode = opCode;		/* this may or may not have been set */
	instrDef->start = LocalGS.insPtr - program;

	while ( (LocalGS.opCode = *LocalGS.insPtr++) != ENDF_CODE )
		fnt_SkipPushCrap( GSPPARAM0 );

	instrDef->length = (uint16)(LocalGS.insPtr - instrStart - 1); /* don't execute ENDF */
}


/*
 * UnTouch Point
 */

FUNCTION LOCAL_PROTO void fnt_UTP(GSP_DECL0)
{
	register ArrayIndex point = (ArrayIndex)CHECK_POP(LocalGS, LocalGS.stackPointer );
	register uint8* f = LocalGS.CE0->f;

	if ( LocalGS.free.x )
	{
		f[point] &= ~XMOVED;
	}
	if ( LocalGS.free.y )
	{
		f[point] &= ~YMOVED;
	}
}


/*
 * Set Delta Base
 */

FUNCTION LOCAL_PROTO void fnt_SDB(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.deltaBase = (int16)CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/*
 * Set Delta Shift
 */

FUNCTION LOCAL_PROTO void fnt_SDS(GSP_DECL0)
{
	LocalGS.globalGS->localParBlock.deltaShift = (int16)CHECK_POP(LocalGS, LocalGS.stackPointer );
}


/*
 * DeltaEngine, internal support routine
 */

FUNCTION LOCAL_PROTO void fnt_DeltaEngine(GSP_DECL1 FntMoveFunc doIt, int16 base, int16 shift)
{
	register int32 tmp;
	register int32 fakePixelsPerEm, ppem;
	register int32 aim, high;
	register int32 tmp32;

	/* Find the beginning of data pairs for this particular size */
	high = (int32)CHECK_POP(LocalGS, LocalGS.stackPointer ) << 1; /* -= number of pops required */
	LocalGS.stackPointer -= high;

	/* same as fnt_MPPEM() */
	tmp32 = LocalGS.globalGS->pixelsPerEm;

	if ( !LocalGS.globalGS->identityTransformation )
	{
		Fixed scale;

		scale = fnt_GetCVTScale( GSPPARAM0 );
		tmp32 = (FixMul( tmp32 + tmp32, scale ) + 1) >> 1; /* avoid loss of precision */
	}

	fakePixelsPerEm = tmp32 - base;



	if ( fakePixelsPerEm >= 16 ||fakePixelsPerEm < 0 )
		return; /* Not within exception range */
	fakePixelsPerEm <<= 4;

	aim = 0;
	tmp = high >> 1; tmp &= ~1;
	while ( tmp > 2 )
	{
		ppem  = LocalGS.stackPointer[ aim + tmp ]; /* [ ppem << 4 | exception ] */
		if ( (ppem & ~0x0f) < fakePixelsPerEm )
		{
			aim += tmp;
		}
		tmp >>= 1; tmp &= ~1;
	}

	while ( aim < high )
	{
		ppem  = LocalGS.stackPointer[ aim ]; /* [ ppem << 4 | exception ] */
		if ( (tmp = (ppem & ~0x0f)) == fakePixelsPerEm )
		{
			/* We found an exception, go ahead and apply it */
			tmp  = ppem & 0xf; /* 0 ... 15 */
			tmp -= tmp >= 8 ? 7 : 8; /* -8 ... -1, 1 ... 8 */
			tmp <<= fnt_pixelShift; /* convert to pixels */
			tmp >>= shift; /* scale to right size */
			doIt( GSPPARAM1 LocalGS.CE0, LocalGS.stackPointer[aim+1] /* point number */, tmp /* the delta */ );
		}
		else
			if ( tmp > fakePixelsPerEm )
			{
				break; /* we passed the data */
			}
		aim += 2;
	}
}


/*
 * DELTAP1
 */

FUNCTION LOCAL_PROTO void fnt_DELTAP1(GSP_DECL0)
{
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_DeltaEngine( GSPPARAM1 LocalGS.MovePoint, pb->deltaBase, pb->deltaShift );
}


/*
 * DELTAP2
 */

FUNCTION LOCAL_PROTO void fnt_DELTAP2(GSP_DECL0)
{
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_DeltaEngine( GSPPARAM1 LocalGS.MovePoint, (int16)(pb->deltaBase+16), pb->deltaShift );
}


/*
 * DELTAP3
 */

FUNCTION LOCAL_PROTO void fnt_DELTAP3(GSP_DECL0)
{
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_DeltaEngine( GSPPARAM1 LocalGS.MovePoint, (int16)(pb->deltaBase+32), pb->deltaShift );
}


/*
 * DELTAC1
 */

FUNCTION LOCAL_PROTO void fnt_DELTAC1(GSP_DECL0)
{
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_DeltaEngine( GSPPARAM1 (FntMoveFunc)fnt_ChangeCvt, pb->deltaBase, pb->deltaShift );
}


/*
 * DELTAC2
 */

FUNCTION LOCAL_PROTO void fnt_DELTAC2(GSP_DECL0)
{
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_DeltaEngine( GSPPARAM1 (FntMoveFunc)fnt_ChangeCvt, (int16)(pb->deltaBase+16), pb->deltaShift );
}


/*
 * DELTAC3
 */

FUNCTION LOCAL_PROTO void fnt_DELTAC3(GSP_DECL0)
{
	register fnt_ParameterBlock *pb = &LocalGS.globalGS->localParBlock;
	fnt_DeltaEngine( GSPPARAM1 (FntMoveFunc)fnt_ChangeCvt, (int16)(pb->deltaBase+32), pb->deltaShift );
}


/*
 *	Rebuild the jump table		<4>
 */

FUNCTION LOCAL_PROTO void fnt_DefaultJumpTable(register FntFunc *function)
{
	register LoopCount i;

	/***** 0x00 - 0x0f *****/
	*function++ = fnt_SVTCA_0;
	*function++ = fnt_SVTCA_1;
	*function++ = fnt_SPVTCA;
	*function++ = fnt_SPVTCA;
	*function++ = fnt_SFVTCA;
	*function++ = fnt_SFVTCA;
	*function++ = fnt_SPVTL;
	*function++ = fnt_SPVTL;
	*function++ = fnt_SFVTL;
	*function++ = fnt_SFVTL;
	*function++ = fnt_WPV;
	*function++ = fnt_WFV;
	*function++ = fnt_RPV;
	*function++ = fnt_RFV;
	*function++ = fnt_SFVTPV;
	*function++ = fnt_ISECT;

	/***** 0x10 - 0x1f *****/
	*function++ = fnt_SetLocalGraphicState;		/* fnt_SRP0; */
	*function++ = fnt_SetLocalGraphicState;		/* fnt_SRP1; */
	*function++ = fnt_SetLocalGraphicState;		/* fnt_SRP2; */
	*function++ = fnt_SetElementPtr;		/* fnt_SCE0; */
	*function++ = fnt_SetElementPtr;		/* fnt_SCE1; */
	*function++ = fnt_SetElementPtr;		/* fnt_SCE2; */
	*function++ = fnt_SetElementPtr;		/* fnt_SCES; */
	*function++ = fnt_SetLocalGraphicState;		/* fnt_LLOOP; */
	*function++ = fnt_SetRoundState;		/* fnt_RTG; */
	*function++ = fnt_SetRoundState;		/* fnt_RTHG; */
	*function++ = fnt_LMD;				/* fnt_LMD; */
	*function++ = fnt_ELSE;				/* used to be fnt_RLSB */
	*function++ = fnt_JMPR;				/* used to be fnt_WLSB */
	*function++ = fnt_LWTCI;
	*function++ = fnt_LSWCI;
	*function++ = fnt_LSW;

	/***** 0x20 - 0x2f *****/
	*function++ = fnt_DUP;
	*function++ = fnt_SetLocalGraphicState;		/* fnt_POP; */
	*function++ = fnt_CLEAR;
	*function++ = fnt_SWAP;
	*function++ = fnt_DEPTH;
	*function++ = fnt_CINDEX;
	*function++ = fnt_MINDEX;
	*function++ = fnt_ALIGNPTS;
	*function++ = fnt_RAW;
	*function++ = fnt_UTP;
	*function++ = fnt_LOOPCALL;
	*function++ = fnt_CALL;
	*function++ = fnt_FDEF;
	*function++ = fnt_IllegalInstruction; 		/* fnt_ENDF; used for FDEF and IDEF */
	*function++ = fnt_MDAP;
	*function++ = fnt_MDAP;


	/***** 0x30 - 0x3f *****/
	*function++ = fnt_IUP;
	*function++ = fnt_IUP;
	*function++ = fnt_SHP;
	*function++ = fnt_SHP;
	*function++ = fnt_SHC;
	*function++ = fnt_SHC;
	*function++ = fnt_SHE;
	*function++ = fnt_SHE;
	*function++ = fnt_SHPIX;
	*function++ = fnt_IP;
	*function++ = fnt_MSIRP;
	*function++ = fnt_MSIRP;
	*function++ = fnt_ALIGNRP;
	*function++ = fnt_SetRoundState;	/* fnt_RTDG; */
	*function++ = fnt_MIAP;
	*function++ = fnt_MIAP;

	/***** 0x40 - 0x4f *****/
	*function++ = fnt_NPUSHB;
	*function++ = fnt_NPUSHW;
	*function++ = fnt_WS;
	*function++ = fnt_RS;
	*function++ = fnt_WCVT;
	*function++ = fnt_RCVT;
	*function++ = fnt_RC;
	*function++ = fnt_RC;
	*function++ = fnt_WC;
	*function++ = fnt_MD;
	*function++ = fnt_MD;
	*function++ = fnt_MPPEM;
	*function++ = fnt_MPS;
	*function++ = fnt_FLIPON;
	*function++ = fnt_FLIPOFF;
	*function++ = fnt_DEBUG;

	/***** 0x50 - 0x5f *****/
	*function++ = fnt_BinaryOperand;	/* fnt_LT; */
	*function++ = fnt_BinaryOperand;	/* fnt_LTEQ; */
	*function++ = fnt_BinaryOperand;	/* fnt_GT; */
	*function++ = fnt_BinaryOperand;	/* fnt_GTEQ; */
	*function++ = fnt_BinaryOperand;	/* fnt_EQ; */
	*function++ = fnt_BinaryOperand;	/* fnt_NEQ; */
	*function++ = fnt_UnaryOperand;		/* fnt_ODD; */
	*function++ = fnt_UnaryOperand;		/* fnt_EVEN; */
	*function++ = fnt_IF;
	*function++ = fnt_EIF;		/* should this guy be an illegal instruction??? */
	*function++ = fnt_BinaryOperand;	/* fnt_AND; */
	*function++ = fnt_BinaryOperand;	/* fnt_OR; */
	*function++ = fnt_UnaryOperand;		/* fnt_NOT; */
	*function++ = fnt_DELTAP1;
	*function++ = fnt_SDB;
	*function++ = fnt_SDS;

	/***** 0x60 - 0x6f *****/
	*function++ = fnt_BinaryOperand;	/* fnt_ADD; */
	*function++ = fnt_BinaryOperand;	/* fnt_SUB; */
	*function++ = fnt_BinaryOperand;	/* fnt_DIV;  */
	*function++ = fnt_BinaryOperand;	/* fnt_MUL; */
	*function++ = fnt_UnaryOperand;		/* fnt_ABS; */
	*function++ = fnt_UnaryOperand;		/* fnt_NEG; */
	*function++ = fnt_UnaryOperand;		/* fnt_FLOOR; */
	*function++ = fnt_UnaryOperand;		/* fnt_CEILING */
	*function++ = fnt_ROUND;
	*function++ = fnt_ROUND;
	*function++ = fnt_ROUND;
	*function++ = fnt_ROUND;
	*function++ = fnt_NROUND;
	*function++ = fnt_NROUND;
	*function++ = fnt_NROUND;
	*function++ = fnt_NROUND;

	/***** 0x70 - 0x7f *****/
	*function++ = fnt_WCVTFOD;
	*function++ = fnt_DELTAP2;
	*function++ = fnt_DELTAP3;
	*function++ = fnt_DELTAC1;
	*function++ = fnt_DELTAC2;
	*function++ = fnt_DELTAC3;
	*function++ = fnt_SROUND;
	*function++ = fnt_S45ROUND;
	*function++ = fnt_JROT;
	*function++ = fnt_JROF;
	*function++ = fnt_SetRoundState;	/* fnt_ROFF; */
	*function++ = fnt_IllegalInstruction;/* 0x7b reserved for data compression */
	*function++ = fnt_SetRoundState;	/* fnt_RUTG; */
	*function++ = fnt_SetRoundState;	/* fnt_RDTG; */
	*function++ = fnt_SANGW;
	*function++ = fnt_AA;

	/***** 0x80 - 0x8d *****/
	*function++ = fnt_FLIPPT;
	*function++ = fnt_FLIPRGON;
	*function++ = fnt_FLIPRGOFF;
	*function++ = fnt_IDefPatch;		/* fnt_RMVT, this space for rent */
	*function++ = fnt_IDefPatch;		/* fnt_WMVT, this space for rent */
	*function++ = fnt_SCANCTRL;
	*function++ = fnt_SDPVTL;
	*function++ = fnt_SDPVTL;
	*function++ = fnt_GETINFO;			/* <7> */
	*function++ = fnt_IDEF;
	*function++ = fnt_ROTATE;
	*function++ = fnt_BinaryOperand;	/* fnt_MAX; */
	*function++ = fnt_BinaryOperand;	/* fnt_MIN; */
	*function++ = fnt_SCANTYPE;			/* <7> */
	*function++ = fnt_INSTCTRL;			/* <13> */

	/***** 0x8f - 0xaf *****/
	for ( i = 32; i >= 0; --i )
		*function++ = fnt_IDefPatch;		/* potentially fnt_IllegalInstruction  <4> */

	/***** 0xb0 - 0xb7 *****/
	for ( i = 7; i >= 0; --i )
		*function++ = fnt_PUSHB;

	/***** 0xb8 - 0xbf *****/
	for ( i = 7; i >= 0; --i )
		*function++ = fnt_PUSHW;

	/***** 0xc0 - 0xdf *****/
	for ( i = 31; i >= 0; --i )
		*function++ = fnt_MDRP;

	/***** 0xe0 - 0xff *****/
	for ( i = 31; i >= 0; --i )
		*function++ = fnt_MIRP;
}


/*
 *	Init routine, to be called at boot time.
 *	globalGS->function has to be set up when this function is called.
 *	rewrite initialization from p[] to *p++							<3>
 *	restructure fnt_AngleInfo into fnt_FractPoint and int16			<3>
 *
 *	Only LocalGS.function is valid at this time.
 */

FUNCTION void fnt_Init(fnt_GlobalGraphicStateType *globalGS)
{
	fnt_DefaultJumpTable( globalGS->function );

	/* These 20 x and y pairs are all stepping patterns that have a repetition period of less than 9 pixels.
	   They are sorted in order according to increasing period (distance). The period is expressed in
	   	pixels * fnt_pixelSize, and is a simple Euclidian distance. The x and y values are Fracts and they are
		at a 90 degree angle to the stepping pattern. Only stepping patterns for the first octant are stored.
		This means that we can derrive (20-1) * 8 = 152 different angles from this data base */

	globalGS->anglePoint = (fnt_FractPoint *)((char*)globalGS->function + MAXBYTE_INSTRUCTIONS * sizeof(FntFunc));
	globalGS->angleDistance = (int16*)(globalGS->anglePoint + MAXANGLES);
	{
		register Fract* coord = (Fract*)globalGS->anglePoint;
		register int16* dist = globalGS->angleDistance;

		/**		 x			 y						d	**/

		*coord++ = 0L;		*coord++ = 1073741824L;	*dist++ = 64;
		*coord++ = -759250125L; *coord++ = 759250125L;	*dist++ = 91;
		*coord++ = -480191942L; *coord++ = 960383883L;	*dist++ = 143;
		*coord++ = -339546978L; *coord++ = 1018640935L;	*dist++ = 202;
		*coord++ = -595604800L; *coord++ = 893407201L;	*dist++ = 231;
		*coord++ = -260420644L; *coord++ = 1041682578L;	*dist++ = 264;
		*coord++ = -644245094L; *coord++ = 858993459L;	*dist++ = 320;
		*coord++ = -210578097L;	*coord++ = 1052890483L;	*dist++ = 326;
		*coord++ = -398777702L; *coord++ = 996944256L;	*dist++ = 345;
		*coord++ = -552435611L; *coord++ = 920726018L;	*dist++ = 373;
		*coord++ = -176522068L; *coord++ = 1059132411L;	*dist++ = 389;
		*coord++ = -670761200L; *coord++ = 838451500L;	*dist++ = 410;
		*coord++ = -151850025L; *coord++ = 1062950175L;	*dist++ = 453;
		*coord++ = -294979565L; *coord++ = 1032428477L;	*dist++ = 466;
		*coord++ = -422967626L; *coord++ = 986924461L;	*dist++ = 487;
		*coord++ = -687392765L; *coord++ = 824871318L;	*dist++ = 500;
		*coord++ = -532725129L; *coord++ = 932268975L;	*dist++ = 516;
		*coord++ = -133181282L; *coord++ = 1065450257L;	*dist++ = 516;
		*coord++ = -377015925L; *coord++ = 1005375799L;	*dist++ = 547;
		*coord   = -624099758L; *coord   = 873739662L;	*dist   = 551;
	}
}

#endif /* PROC_TRUETYPE */
/* END OF fnt.c */
#endif /* #ifdef ENABLE_NATIVE_TT_HINTS */



/**************** Revision Control Information *********************************
 * 
 * $Header: R:/src/FontFusion/Source/Core/rcs/fnt.c 1.17 2001/05/02 17:20:18 reggers Exp $
 * 
 * $Log: fnt.c $
 * Revision 1.17  2001/05/02 17:20:18  reggers
 * SEAT BELT mode added (Sampo)
 * Revision 1.16  2001/01/16 17:25:59  reggers
 * Removed #include <setjmp.h>
 * Revision 1.15  2000/08/28 21:39:56  reggers
 * Modified fnt_ChangeCvt for non-square resolutions (Sampo).
 * Revision 1.14  2000/05/25 21:58:26  reggers
 * Change calculation of fnt_MPPEM() for precedence binding (Sampo).
 * Revision 1.13  2000/05/22 20:52:01  reggers
 * Increased numerical precision under non-square aspect ratios (Sampo).
 * Revision 1.12  2000/05/22 16:16:06  reggers
 * Fixed calculation of ppem in fnt_MPPEM() (Sampo).
 * Revision 1.11  2000/05/19 22:34:43  reggers
 * #define'd NO_CHECK_TRASHED_MEMORY. We were bailing on
 * a bogus failure here on Palm.
 * Revision 1.10  2000/04/14 19:03:41  reggers
 * Fix to avoid loss of precision in fnt_DeltaEngine().
 * Revision 1.9  2000/04/06 16:28:27  reggers
 * Test for divide by zero error.
 * Revision 1.8  2000/02/25 17:45:35  reggers
 * STRICT warning cleanup.
 * Revision 1.7  1999/12/09 21:17:26  reggers
 * Sampo: multiple TrueType compatibility enhancements (scan convereter).
 * Revision 1.6  1999/10/19 16:22:32  shawn
 * Added UNUSED() macro to both versions of fnt_DEBUG().
 * 
 * Revision 1.5  1999/10/18 16:58:32  jfatal
 * Changed all include file names to lower case.
 * Revision 1.4  1999/09/30 15:11:15  jfatal
 * Added correct Copyright notice.
 * Revision 1.3  1999/08/31 19:19:55  reggers
 * Put parens to make VECTORDOT less ambiguous.
 * Revision 1.2  1999/05/17 15:56:45  reggers
 * Inital Revision
 * Revision 14.1  1997/12/17 14:38:55  shawn
 * Renamed macros BIT0 - BIT7 to TT_BIT0 - TT_BIT7 to avoid
 * confusion with TDIS macros of the same names.
 * Cast font round function pointer comparisons with (FntRoundFunc).
 * 
 * 
 * Revision 14.0  1997/03/17 17:53:19  shawn
 * TDIS Version 6.00
 * 
 * Revision 10.6  97/01/14  17:35:25  shawn
 * Apply casts to avoid compiler warnings.
 * 
 * Revision 10.5  96/08/06  16:10:19  shawn
 * Changed cast in call to fnt_DeltaEngine() to (int16).
 * 
 * Revision 10.4  96/07/02  13:44:11  shawn
 * Changed boolean to btsbool.
 * 
 * Revision 10.3  96/06/05  15:25:04  shawn
 * Added casts to avoid compiler warnings.
 * 
 * Revision 10.2  95/11/17  09:47:39  shawn
 * Changed "FUNCTION static" declarations to "FUNCTION LOCAL_PROTO"
 * 
 * Revision 10.1  95/04/11  12:23:10  roberte
 * Moved some macros here from fnt.h. They are truly local.
 * 
 * Revision 10.0  95/02/15  16:22:26  roberte
 * Release
 * 
 * Revision 9.3  95/01/11  13:13:27  shawn
 * Changed IFntFunc typedef to FntFunc
 * Define function pointer parameters as type FntFunc
 * Changed traceFunc parameter in fnt_Execute to type fs_FuncType
 * 
 * Revision 9.2  95/01/06  12:45:29  shawn
 * Cast fnt_ChangeCvt to (FntMoveFunc) in fnt_DeltaEngine() calls
 * 
 * Revision 9.1  95/01/04  16:30:00  roberte
 * Release
 * 
 * Revision 8.1  95/01/03  13:44:07  shawn
 * Converted to ANSI 'C'
 * Modified for support by the K&R conversion utility
 * 
 * Revision 8.0  94/05/04  09:26:32  roberte
 * Release
 * 
 * Revision 7.0  94/04/08  11:56:50  roberte
 * Release
 * 
 * Revision 6.94  94/03/18  13:58:37  roberte
 * Made function prototypes and declarations agree on static-ness 
 * to silence nagging compiler warnings.
 * 
 * Revision 6.93  94/02/03  12:43:49  roberte
 * Added some debug features, and corrected existing debug features.
 * 
 * Revision 6.92  93/10/01  17:05:19  roberte
 * Added #if PROC_TRUETYPE conditional around whole file.
 * 
 * Revision 6.91  93/08/30  14:50:16  roberte
 * Release
 * 
 * Revision 6.50  93/07/27  12:11:30  roberte
 * Added address operator to parameter in LocalGS.TraceFunc() call.
 * 
 * Revision 6.49  93/07/14  13:54:24  roberte
 * Inserted missing GSPPARAM1 macro in GetCVTEntry() call in fnt_MIRP()
 * 
 * Revision 6.48  93/06/24  17:41:10  roberte
 * Corrections to some selectively re-entrant functions.
 * New fnt_IUP and fnt_MIRP.
 * 
 * Revision 6.47  93/06/15  14:10:08  roberte
 * Major rework allowing this module to be independantly switchable for
 * REENTRANT. If not REENTRANT, declares a static local fnt_LocalGraphicStateType
 * structure, and all functions access that, else, a pointer is passed to
 * an automatic fnt_LocalGraphicStateType structure. Used same convention
 * for argument handling as in Speedo code and 4-in-1 in general. Gives
 * a little performance boost when not REENTRANT.
 * 
 * Revision 6.46  93/05/21  09:46:56  roberte
 * Removed OLDWAY blocks from speedup work.
 * Restored original bitcount() function.
 * 
 * Revision 6.45  93/05/18  09:38:52  roberte
 * Speedup work.  bitcount(), less bits to count if start from left?\
 * Other work- replace if () else if ().. with switch statements.  Usually
 * these execute faster.
 * 
 * Revision 6.44  93/03/15  13:07:41  roberte
 * Release
 * 
 * Revision 6.9  93/03/09  13:06:06  roberte
 * Broke assignment of LocalGS.Interpreter apart from call of the function pointer.
 * Clearer more portable code.
 * 
 * Revision 6.8  93/01/25  09:37:47  roberte
 * Employed PROTO macro for all function prototypes.
 * 
 * Revision 6.7  93/01/19  10:41:09  davidw
 * 80 column cleanup, ANSI compatability cleanup
 * 
 * Revision 6.6  92/12/29  12:49:24  roberte
 * Now includes "spdo_prv.h" first.
 * Also handled conflict for BIT0..BIT7 macros with those in speedo.h.
 * 
 * Revision 6.5  92/12/15  14:11:58  roberte
 * Commented out #pragma.
 * 
 * Revision 6.4  92/11/24  13:35:55  laurar
 * include fino.h
 * 
 * Revision 6.3  92/11/19  15:45:35  roberte
 * Release
 * 
 * Revision 6.2  92/10/15  11:48:47  roberte
 * Changed all ifdef PROTOS_AVAIL statements to if PROTOS_AVAIL.
 * 
 * Revision 6.1  91/08/14  16:44:56  mark
 * Release
 * 
 * Revision 5.1  91/08/07  12:26:08  mark
 * Release
 * 
 * Revision 4.2  91/08/07  11:38:58  mark
 * added RCS control strings
 * 
 *******************************************************************************/

