/*
	File:	kcmptlib.h	@(#)kcmptlib.h	2.31 06/25/96

	Contains:       Header file for KCMS Processor Library

	Written by:     Drivin' Team

	Copyright (c) 1992-1996 Eastman Kodak Company, all rights reserved.
 */

#ifndef _KCMSPTLIB_H
#define _KCMSPTLIB_H 1

#include "kcms_sys.h"
#include "kcmptdef.h"

#ifdef KPWIN
typedef PTErr_t (FAR PASCAL *PTProgress_t) (int32);
#else
typedef PTErr_t (*PTProgress_t) (int32);
#endif


#ifdef __cplusplus
extern "C" {
#endif
PTErr_t PTCheckIn ARGS((PTRefNum_p, PTAddr_t));
PTErr_t PTCheckOut ARGS((PTRefNum_t));
PTErr_t PTActivate ARGS((PTRefNum_t, int32, PTAddr_t));
PTErr_t PTDeActivate ARGS((PTRefNum_t));
PTErr_t PTGetPTInfo ARGS((PTRefNum_t,
					PTAddr_h, PTAddr_h, PTAddr_h));
PTErr_t PTNewPT ARGS((PTAddr_p, PTAddr_p, PTRefNum_p));
PTErr_t PTGetAttribute ARGS((PTRefNum_t, int32, int32_p, char_p));
PTErr_t PTSetAttribute ARGS((PTRefNum_t, int32, char_p));
PTErr_t PTGetTags ARGS((PTRefNum_t, int32_p, int32_p));
PTErr_t PTGetSize ARGS((PTRefNum_t, int32_p));
PTErr_t PTGetPT ARGS((PTRefNum_t, int32, PTAddr_t));
PTErr_t PTGetSizeF ARGS((PTRefNum_t, PTType_t, int32_p));
PTErr_t PTGetPTF ARGS((PTRefNum_t, PTType_t, int32, PTAddr_t));
PTErr_t PTEval ARGS((PTRefNum_t, PTEvalPB_p,
					PTEvalTypes_t, int32, int32, opRefNum_p, PTProgress_t));
PTErr_t PTEvalDT ARGS((PTRefNum_t, PTEvalDTPB_p,
					PTEvalTypes_t, int32, int32, opRefNum_p, PTProgress_t));
PTErr_t PTEvalRdy ARGS((opRefNum_t, int32_p));
PTErr_t PTEvalCancel ARGS((opRefNum_t));
PTErr_t PTChainValidate ARGS((int32, PTRefNum_p, int32_p));
PTErr_t PTChainInit ARGS((int32, PTRefNum_p, int32, int32_p));
PTErr_t PTChainInitM ARGS((int32, PTRefNum_p, int32, int32));
PTErr_t PTChain ARGS((PTRefNum_t));
PTErr_t PTChainEnd ARGS((PTRefNum_p));
PTErr_t PTCombine ARGS((int32, PTRefNum_t, PTRefNum_t, PTRefNum_p));
PTErr_t PTEvaluators ARGS((int32_p, evalList_p));
PTErr_t PTProcessorReset ARGS((void));
PTErr_t PTInitialize ARGS((void));
#if defined (KPWIN)
PTErr_t PTInitializeEx ARGS((PTInitInfo_t * InitInfo));
#endif
PTErr_t PTTerminate ARGS((void));
PTErr_t PTInitGlue ARGS((void));
PTErr_t PTTermGlue ARGS((void));
PTErr_t PTNewEmpty ARGS((int32, int32_long_ptr, int32, PTRefNum_long_ptr_t));
PTErr_t PTNewEmptySep ARGS((int32, int32_long_ptr, PTRefNum_long_ptr_t));
PTErr_t PTGetItbl ARGS((PTRefNum_t, int32, int32, KcmHandle FAR*));
PTErr_t PTGetGtbl ARGS((PTRefNum_t, int32,
					int32_long_ptr, int32_long_ptr, KcmHandle FAR*));
PTErr_t PTGetOtbl ARGS((PTRefNum_t, int32, KcmHandle FAR*));
PTErr_t PTNewMatGamAIPT ARGS((FixedXYZColor_p, FixedXYZColor_p, 
			FixedXYZColor_p, ResponseRecord_p, ResponseRecord_p,
			ResponseRecord_p, KpUInt32_t, bool, newMGmode_p, PTRefNum_p));
PTErr_t PTNewMatGamPT ARGS((FixedXYZColor_p, FixedXYZColor_p, 
			FixedXYZColor_p, ResponseRecord_p, ResponseRecord_p,
			ResponseRecord_p, u_int32, bool, PTRefNum_p));
PTErr_t PTNewMonoPT ARGS((	ResponseRecord_p	grayTRC,
							KpUInt32_t			gridsize,
							bool				invert,
							PTRefNum_p			thePTRefNumP));
PTErr_t PTGetRelToAbsPT ARGS((	KpUInt32_t		RelToAbsMode,
								PTRelToAbs_p	PTRelToAbs,
								PTRefNum_p		PTRefNumPtr));
PTErr_t PTApplInitialize ARGS((KcmHandle, int32));
PTErr_t PTGetFlavor ARGS((KpInt32_t * kcpFlavor));
#if defined(KPMAC)
void KCMPTSetComponentInstance ARGS((int32 theNewCP));
void KCMPTGetComponentInstance ARGS((int32_long_ptr theCurCP));
#else
PTErr_t PTInitThread ARGS((void));
PTErr_t PTTermThread ARGS((void));
#endif
#ifdef __cplusplus
}
#endif

#endif

