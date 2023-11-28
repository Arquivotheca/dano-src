/* Util.h    Random utilities used by the core.
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#ifndef _UTIL_H
#define _UTIL_H
#if defined(__cplusplus)
   extern "C" {
#endif 

#ifndef NDEBUG
  void JSE_CFUNC InstantDeath(enum textcoreID TextID,...);
#endif

void ParseSourceTextIntoArgv(jsechar *SourceText,int *_argc, jsechar ***_argv);
void FreeArgv(int argc,jsechar *argv[]);

/* The following flags are used to determine the current reason why
 * the current statement is quitting.  Note that DoCall filters out
 * all but FlowNoReasonToQuit, FlowError, FlowExit, and FlowEndThread.Also note
 * that the QUITable flags are only in the low uword8
 */
typedef uword8  FlowFlag;
#define FlowNoReasonToQuit  0
#define FlowError           0x01
#define FlowExit            0x02
#ifdef __MULTITHREAD__
#  define FlowEndThread     0x04
#endif

#define TestFlowQuitFlags(flowFlag) (flowFlag)

#ifndef NDEBUG
   #define AssertNoReasonToQuit(Reason); \
      { assert( FlowNoReasonToQuit == Reason ); }
#else
   #define AssertNoReasonToQuit(Reason);     /* don't be so anal */
#endif

#if (defined(__JSE_WIN16__) || defined(__JSE_DOS16__)) \
 && (defined(__JSE_DLLLOAD__) || defined(__JSE_DLLRUN__))
   typedef uword32 (JSE_CFUNC *ClientFunction)(jseContext jsecontext,...);
   uword32 JSE_CFUNC FAR_CALL DispatchToClient(uword16 ClientDataSegment,
                                               ClientFunction FClient,...);
#endif

#ifndef NDEBUG
   #define JSE_DEBUG_FEEDBACK(val) (val)
#endif

#if defined(__cplusplus)
   }
#endif 
#endif
