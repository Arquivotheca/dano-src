/*
 * @(#)kcpmgru.h	2.12 97/12/22

	Contains:	KCM Driver utility routines header info

	Written by:	Drivin' Team

  COPYRIGHT (c) 1991-1994 Eastman Kodak Company.
  As  an  unpublished  work pursuant to Title 17 of the United
  States Code.  All rights reserved.
*/

#ifndef _KCPMGRU_H_
#define _KCPMGRU_H_ 1

#include "kcmsos.h"

#if defined (KPMAC68K)

/* functions */
static SetA4 (long newA4) =
{
	0x201F,		/* POP	D0 */
	0xC18C		/* EXG	A4, D0 */
};

pascal void FixPC() = 
{
	0x41FA, 0x000A,
	0x2008,
	0xA055,
	0x2040,
	0x4ED0
};

#define bits32 { \
	FixPC();	\
	host_mmu_mode = GetMMUMode(); \
	mode_32b = 1;	\
	SwapMMUMode(&mode_32b); \
	}

#define bits24 { \
	SwapMMUMode((char *) &host_mmu_mode); \
	}

#endif


/* routines in kcpmgru.c */
KCP_GLOBAL	PTErr_t FAR PASCAL callProgress ARGS((KpInt32_t percent));
void		initProgressPasses ARGS((threadGlobals_p threadGlobalsP, KpInt32_t numPasses, PTProgress_t progress));
void		initProgress ARGS((threadGlobals_p threadGlobalsP, KpInt32_t loopMax, PTProgress_t progress));
PTErr_t		doProgress ARGS((threadGlobals_p threadGlobalsP, KpInt32_t percent));
PTErr_t		KPCPProgressCallback ARGS((KpInt32_t percent));
PTErr_t		SetKCPDataDirProps (ioFileChar *KCPDataDirProps);

#endif

