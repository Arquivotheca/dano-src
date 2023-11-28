/*
 *
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This listing is supplied under the terms of a license agreement
 *      with INTEL Corporation and may not be copied nor disclosed
 *        except in accordance with the terms of that agreement.
 *
 *************************************************************************
 *
 *               Copyright (C) 1994-1997 Intel Corp.
 *                         All Rights Reserved.
 *
 */

/*
 * tksys.c
 * 
 * Malloc and Free functions, with debug options to monitor memory use.
 *
 * Functions:
 *	SysMalloc
 *	SysFree
 */

#include <setjmp.h>
#include "datatype.h"
#include "pia_main.h"
#include "mc.h"
#include "hufftbls.h"
#include "qnttbls.h"
#include "mbhuftbl.h"
#include "bkhuftbl.h"
#include "ivi5bs.h"
#ifdef SIMULATOR
#include "decsmdef.h"
#include "encsmdef.h"
#endif
#include "bsutil.h"
#include "indeo5.h"

#ifdef CMD_LINE_ENC
#include "cmdlparm.h"
#endif

#include "pia_enc.h"
#include "errhand.h"
#include "tksys.h"

#ifdef MEMDEBUG
#include <stdio.h>
#endif

/******* System Memory Allocation/Freeing routines *********/

/*
 * SysMalloc actually allocates uCnt bytes of memory
 */
#ifdef MEMDEBUG

extern FILE *fp;
I32 iNumMallocs = 0;
I32 iNumFrees = 0;

PDbl
SysMallocFunc(const PChr File, I32 iLine, U32 uCnt, jmp_buf jbEnv)

#else

PDbl
SysMallocFunc(U32 uCnt, jmp_buf jbEnv)

#endif /* MEMDEBUG */
/* 
 * This is the system memory allocation routine.  All errors, such as
 * a request for 0 bytes and unavailability of required storage are 
 * unrecoverable.  If the ReportMallocErrs static variable is false, fatal 
 * errors are not reported, but rather a return code of 0 (the only 
 * illegal value for a pointer) is returned.  This routine returns a 
 * pointer to double-word aligned storage.  If the SDG_MALLOC_GUARD
 * bit in the system debug flag is set, the debugging guard-byte aid
 * is turned on.  For more information on this see the man pages for 
 * SysMalloc.
 */
{
  PDbl pd;
  					
  if (uCnt == 0) 
	longjmp(jbEnv,  (TKSYS   << FILE_OFFSET) |
					(__LINE__ << LINE_OFFSET) |
					(ERR_BAD_PARM << TYPE_OFFSET));
  pd = HiveGlobalAllocPtr(uCnt, TRUE);
  if (pd == NULL) 
	longjmp(jbEnv,  (TKSYS   << FILE_OFFSET) |
					(__LINE__ << LINE_OFFSET) |
					(ERR_ERROR << TYPE_OFFSET));
#ifdef MEMDEBUG
  fprintf(fp, "SysMalloc\t%s\t%d\t%d%8d\t%d\n", File, iLine, iNumMallocs,
  		uCnt, pd);
  iNumMallocs++;
#endif /* MEMDEBUG */
  return pd ;
}

#ifdef MEMDEBUG

void
SysFreeFunc(const PChr File, I32 iLine, PU8 pu8, jmp_buf jbEnv)

#else

void
SysFreeFunc(PU8 pu8, jmp_buf jbEnv)

#endif /* MEMDEBUG */
/*
 * This function frees storage allocated by SysMalloc
 */
{
  if (pu8 == 0) 
	longjmp(jbEnv,  (TKSYS   << FILE_OFFSET) |
					(__LINE__ << LINE_OFFSET) |
					(ERR_NULL_PARM << TYPE_OFFSET));
#ifdef MEMDEBUG
  	fprintf(fp, "SysFree\t\t%s\t%d\t%d%8d\t%d\n", File, iLine, iNumFrees, 0, pu8);
#endif /* MEMDEBUG */
  HiveGlobalFreePtr(pu8);
  return ;
}
