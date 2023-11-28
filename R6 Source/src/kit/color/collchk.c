/*
 * @(#)collchk.c	2.7 97/12/22

	Contains:	routine to get space left between heap end and stack
				pointer.

	Written by:	drivin' team

	Copyright:	(C) 1991-1994 by Eastman Kodak Company, all rights reserved.

 */

#include "kcms_sys.h"
#include "kcmptlib.h"
#include "kcmptdef.h"
#include "kcptmgr.h"

#if defined (KPMAC68K)

#include <Memory.h>
#include <LowMem.h>
#include <Resources.h>

int32 CollisionCheck() 
{
	int stackSize;
	Ptr stackPtr = nil;
	Ptr heapPtr = nil;
	int err;
	int32 zoneSize;
	
	stackSize = StackSpace();
	err = LMGetResErr();
	if (err != noErr) {
		LMSetResErr (0);
	}
	stackPtr = GetApplLimit();
	err = LMGetResErr();
	if (err != noErr) {
		LMSetResErr (0);
	}
	stackPtr += stackSize;
	heapPtr = LMGetHeapEnd();
	zoneSize = (int32) (stackPtr - heapPtr);
	return zoneSize;
}

#else			/* not a Macintosh */

/*
 *   if not a KPMAC, just return a very large number
 *   because the whole thing is irrelevant
 */
int32 CollisionCheck() 
{
	return (0x7fffffffL);
}

#endif			/* Macintosh */

