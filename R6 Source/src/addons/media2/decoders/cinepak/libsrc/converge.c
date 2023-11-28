/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/converge.c 2.11 1995/10/02 11:53:34 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: converge.c $
 * Revision 2.11  1995/10/02 11:53:34  bog
 * Separate CClass and VClass parameters for MatchAndRefine for
 * clarity.
 * 
 * Revision 2.10  1995/05/09  09:23:06  bog
 * Move WINVER back into the makefile.  Sigh.
 * 
 * Revision 2.9  1994/10/23  17:22:22  bog
 * Try to allow big frames.
 * 
 * Revision 2.8  1994/10/20  17:27:21  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.7  1994/07/18  13:30:23  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.6  1994/05/01  23:38:02  unknown
 * Move nCodes based max neighbors calculation down to MatchAndRefine().
 * 
 * Revision 2.5  1994/05/01  16:16:27  bog
 * Use NeighborsRounds from SYSTEM.INI rather than a constant.
 * 
 * Revision 2.4  1994/05/01  16:05:41  unknown
 * Build the Neighbors list only in the first two rounds in ConvergeCodeBook.
 * In subsequent rounds, just reuse the last one built.
 * 
 * Revision 2.3  1993/09/24  14:07:30  geoffs
 * Have an approximation of progress for compression in call to Status callback
 * 
 * Revision 2.2  1993/09/24  13:40:41  bog
 * First pass status callback stuff
 * 
 * Revision 2.1  1993/07/06  16:55:27  geoffs
 * 1st pass WIN32'izing
 * 
 * Revision 2.0  93/06/01  14:13:21  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.5  93/04/21  15:46:45  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.4  93/02/03  09:49:31  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 * 
 * Revision 1.3  93/01/31  13:09:36  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 * 
 * Revision 1.2  93/01/25  21:40:46  geoffs
 * Remove debug.
 * 
 * Revision 1.1  93/01/19  14:58:30  bog
 * Initial revision
 * 
 *
 * CompactVideo Converge a codebook
 */

#if	defined(WINCPK)

#define	_WINDOWS
#include <stdio.h>
#include <memory.h>		// included for _fmemcpy
#include <stdlib.h>
#ifdef	NULL
#undef	NULL
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <compddk.h>

#endif

#include "cv.h"
#include "cvcompre.h"


/**********************************************************************
 *
 * ConvergeCodeBook()
 *
 * Converge a codebook
 *
 **********************************************************************/

unsigned long ConvergeCodeBook(
  CCONTEXT *pC,			// compression context
  VECTORBOOK *pVB,		// current codebook
  unsigned char VClass,		// vector class to match; 0xff means any
  unsigned char CClass		// any bit on here must be on in EC Flags
) {
  auto int i;
  auto unsigned long PrevErr, ThisErr;

  for (
    i = 16,
      ThisErr = PrevErr
      = MatchAndRefine(pC, pVB, 2, VClass, CClass);

    /*
      We quit trying to converge when
	a.  user has aborted this operation, or
	b.  we have tried 16 rounds, or
	c.  no error remains, or
	d.  the relative error change is smaller than .005 for detail
	    or .0005 for smooth.

      We don't let MatchAndRefine() recompute the neighbors list after
      the (16 - (NeighborsRounds in [iccvid.drv] in SYSTEM.INI))th call.
     */
    (i--) &&
      PrevErr &&
      (
	ThisErr = MatchAndRefine(
	  pC,
	  pVB,
	  i > pC->NeighborsRounds,// only on 1st iters
	  VClass,
	  CClass
	)
      ) && (
	((PrevErr - ThisErr) * pVB->Converged) >= ThisErr
      );

    PrevErr = ThisErr
  );

  return(ThisErr);
}
