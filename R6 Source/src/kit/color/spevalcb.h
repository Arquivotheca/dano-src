/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1997                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

/*  spevalcb.h

This file provides macros which allow the same code to work for both PPC and non-PPC
It helps setting up Universal Proc Pointers for Mac PPC Mixed Mode.

It is used by the app to set up a Universal Proc for a callBack and by the profile
to call the callBack - only used for pteval callback

	SCCS Info:
		@(#)spevalcb.h	1.4  12/22/97
*/

#ifndef _SPEVALCB_H_
#define _SPEVALCB_H_


#if defined (KPMACPPC)

typedef  UniversalProcPtr spEvalCBUPP;
enum {
	uppspEvalCBProcInfo = kPascalStackBased
			| RESULT_SIZE(SIZE_CODE(sizeof(PTErr_t)))  
			| STACK_ROUTINE_PARAMETER(2, (SIZE_CODE(sizeof(KpInt32_t))))  /*percent */
};
#define CallSPEvalCBFunc(userRoutine, thePercent) \
		CallUniversalProc((spEvalCBUPP)userRoutine,uppspEvalCBProcInfo,thePercent)

#define NewSPEvalCBFunc(userRoutine)		\
		 (spEvalCBUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppspEvalCBProcInfo,  GetCurrentArchitecture())


#else


typedef PTProgress_t spEvalCBUPP;

#define CallSPEvalCBFunc(userRoutine, thePercent) \
		(*userRoutine)( thePercent)
 
#define NewSPEvalCBFunc(userRoutine)		\
		(spEvalCBUPP)(userRoutine)


#endif

#endif /*_SPEVALCB_H_*/

