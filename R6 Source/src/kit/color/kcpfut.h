/*
 * @(#)kcpfut.h	2.18 97/12/22

	Contains:	Header file for fut evaluation routines.

	Written by:	Drivin' Team

	Copyright:	(c) 1992-1997 by Eastman Kodak Company, all rights reserved.

*/

#ifndef _KCMFUT_H_
#define _KCMFUT_H_ 1

#include "kcptmgr.h"

typedef struct futEvalInfo_s {
	fut_ptr_t		futP;		/* fut to use for evaluation */
	KpHandle_t		futH;		/* fut handle */
	KpUInt32_t		nOutputs;	/* # of outputs evaluated using this fut */
	KpUInt32_t		evalMask;	/* I/O mask for the evaluation */
} futEvalInfo_t, FAR* futEvalInfo_p, FAR* FAR* futEvalInfo_h;

#if defined KPMAC
#define KCP_DEREFERENCE(type, handle) (*(type FAR*)handle)
#else
#define KCP_DEREFERENCE(type, handle) ((type)handle)
#endif

PTErr_t PTEvalSeq (threadGlobals_p, KpInt32_t, futEvalInfo_p, PTEvalDTPB_p, PTProgress_t);

PTErr_t unlockPT ARGS((KcmHandle, fut_ptr_t));

PTErr_t PTNewEmpty ARGS((int32, int32_p, int32, PTRefNum_p));
PTErr_t PTNewEmptySep ARGS((int32, int32_p, PTRefNum_p));
PTErr_t fut2PT ARGS((fut_ptr_t, PTRefNum_p));
PTErr_t PTGetItbl ARGS((PTRefNum_t, int32, int32, KcmHandle FAR*));
PTErr_t PTGetGtbl ARGS((PTRefNum_t, int32, int32_p, int32_p, KcmHandle FAR*));
PTErr_t PTGetOtbl ARGS((PTRefNum_t, int32, KcmHandle FAR*));

/* functions in combine.c */
fut_ptr_t get_lab2xyz ARGS((int));

#endif

