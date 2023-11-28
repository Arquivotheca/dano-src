/*
 * @(#)kcpmgru.c	2.35 97/12/22

	Contains:	KCM Driver utility routines

	Written by:	Drivin' Team

 *********************************************************************
 *********************************************************************
 *    COPYRIGHT (c) 1992-1997 Eastman Kodak Company.
 *    As  an  unpublished  work pursuant to Title 17 of the United
 *    States Code.  All rights reserved.
 *********************************************************************
 *********************************************************************
 */

#include "kcms_sys.h"

#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "kcptmgr.h"
#include "kcpmgru.h"

#include <string.h>

#if defined (KPMAC)

#include <Types.h>
#include <OSUtils.h>

#endif

#if defined (KPMACPPC)

#include <Traps.h>

/* declare the universal procedure pointer info */
enum {
	uppCallProgressProcInfo =
		kCStackBased |
		RESULT_SIZE (SIZE_CODE (sizeof (PTErr_t) ) ) |
		STACK_ROUTINE_PARAMETER (1, SIZE_CODE (sizeof (int32) ) ),

	upprelayProcInfo =
		kCStackBased |
		RESULT_SIZE (SIZE_CODE (sizeof (PTErr_t) ) ) |
		STACK_ROUTINE_PARAMETER (1, SIZE_CODE (sizeof (long) ) ) |
		STACK_ROUTINE_PARAMETER (2, SIZE_CODE (sizeof (long) ) ) |
		STACK_ROUTINE_PARAMETER (3, SIZE_CODE (sizeof (PTProgress_t) ) ) |
		STACK_ROUTINE_PARAMETER (4, SIZE_CODE (sizeof (int32) ) )
};

#endif

#if defined (KPMAC)

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * callProgress - Macintosh Version
 *
 * DESCRIPTION
 * This function calls the user's call back function
 * must be done as separate subroutine so that progress function
 * address is addressed via the A6 stack
 *
 *--------------------------------------------------------------------*/
PTErr_t FAR PASCAL
	callProgress (KpInt32_t percent)
{
threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
PTProgress_t 	LprogressFunc;
long 			lHostA5, lHostA4;
PTRelay_t		relay;
PTErr_t 		theReturn;

#if defined (KPMACPPC)
long			thisA5;
#endif

	threadGlobalsP = KCMDloadGlobals();	/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}

	LprogressFunc = threadGlobalsP->progressFunc;

	lHostA4 = threadGlobalsP->gHostA4;	/* get host's global world */
	lHostA5 = threadGlobalsP->gHostA5;	/* get host's global world */

	relay = threadGlobalsP->processGlobalsP->iGP->callBackRelay;

	KCMDunloadGlobals();				/* unlock this apps Globals */

	/* do the progress call-back */
#if defined (KPMACPPC)
	thisA5 = SetA5 (lHostA5);
	if ( ((UniversalProcPtr)LprogressFunc)->goMixedModeTrap == _MixedModeMagic) {	/* callback code is PPC */
		theReturn = (PTErr_t) CallUniversalProc ((UniversalProcPtr)LprogressFunc, uppCallProgressProcInfo, percent);
	}
	else {								/* callback code is 68K */
		theReturn = (PTErr_t) CallUniversalProc ((UniversalProcPtr)relay, upprelayProcInfo, lHostA5, lHostA4, LprogressFunc, percent);
	}
	SetA5 (thisA5);
#endif
#if defined (KPMAC68K)
		theReturn =(PTErr_t)relay (lHostA5, lHostA4, LprogressFunc, percent);
#endif

	return (theReturn);
}


#else

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * callProgress - Windows and all others version
 *
 * DESCRIPTION
 * This function calls the user's call back function
 * must be done as separate subroutine so that progress function
 * address is addressed via the A6 stack
 *
 *--------------------------------------------------------------------*/
PTErr_t FAR PASCAL
	callProgress (KpInt32_t percent)
{
	threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
	PTProgress_t 	LprogressFunc;
	PTErr_t 		theReturn;

	threadGlobalsP = KCMDloadGlobals();			/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}

 	LprogressFunc = threadGlobalsP->progressFunc;
	KCMDunloadGlobals();						/* unlock this apps Globals */

	/* do the progress call-back */
	theReturn = LprogressFunc (percent);

	return (theReturn);
}

#endif



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * initProgressPasses
 *
 * DESCRIPTION
 * This function sets up the total number of passes that will
 * be performed.
 *
 *--------------------------------------------------------------------*/
void
	initProgressPasses (	threadGlobals_p threadGlobalsP,
							KpInt32_t numPasses,
							PTProgress_t progress)
{
	threadGlobalsP->currPasses = 0;				/* number passes completed 	*/
	threadGlobalsP->totalPasses = numPasses;	/* total passes to be done 	*/
	threadGlobalsP->apiProgressFunc = progress;	/* define the progress call-back */
	threadGlobalsP->lastProg100 = False;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * initProgress
 *
 * DESCRIPTION
 * This function sets up the progress call back function
 *
 *--------------------------------------------------------------------*/
void
	initProgress (	threadGlobals_p threadGlobalsP,
					KpInt32_t loopMax,
					PTProgress_t progress)
{
	threadGlobalsP->progressFunc = progress;	/* define the progress call-back */
	threadGlobalsP->loopStart = loopMax;		/* define the repetition rate */
	threadGlobalsP->loopCount = 0;				/* force call-back next time */
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * doFMGRProgress
 *
 * DESCRIPTION
 * This function executes the call back function.
 *
 *--------------------------------------------------------------------*/
PTErr_t
	doProgress (	threadGlobals_p	threadGlobalsP,
						KpInt32_t		percent)
{
PTErr_t theReturn = KCP_SUCCESS;

	if (threadGlobalsP != NULL) {
		if ((threadGlobalsP->loopCount <= 0) || (percent == 100)) {

			theReturn = KPCPProgressCallback (percent);				/* do the progress call-back */
			threadGlobalsP->loopCount = threadGlobalsP->loopStart;
		}
		else {
			threadGlobalsP->loopCount--;
		}
	}
	
	return (theReturn);
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KPCPProgressCallback
 *
 * DESCRIPTION
 * This function executes the call back function.  It calculates the 
 * actual progress based on the number of passes completed and the 
 * number of passes remaining and then calls the normal progress 
 * routine.
 *
 * This function is called with the A5 world set to the color processor A5 world.
 *
 *--------------------------------------------------------------------*/
PTErr_t
	KPCPProgressCallback (KpInt32_t percent)
{
PTErr_t			theReturn = KCP_SUCCESS;
KpInt32_t		realPercent;
KpInt32_t 		currPasses;
KpInt32_t 		totalPasses;
threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */

	threadGlobalsP = KCMDloadGlobals ();	/* Setup this apps Globals */

	/* if previous percent was 100 and this one is not, go to next pass */
	if ((percent != 100) && threadGlobalsP->lastProg100) {
		threadGlobalsP->currPasses++;
		threadGlobalsP->lastProg100 = False;
	}
	
	currPasses = threadGlobalsP->currPasses;
	totalPasses = threadGlobalsP->totalPasses;
	
	KCMDunloadGlobals();				/* unlock this apps Globals */

	if (threadGlobalsP->apiProgressFunc != NULL) {
		realPercent = ((threadGlobalsP->currPasses * 100) + percent) / threadGlobalsP->totalPasses;
	
	/* do the progress call-back */
		theReturn = callProgress (realPercent);

	/* see if this pass is done  - don't incr if multiple 100% in a row */
		if (percent == 100) {
			threadGlobalsP = KCMDloadGlobals ();
			
			threadGlobalsP->lastProg100 = True;
			
			KCMDunloadGlobals();
		}
	}
	
	return (theReturn);
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * SetKCPDataDirProps
 *
 * DESCRIPTION
 * This function returns the Color Processor working directory
 *
 *--------------------------------------------------------------------*/

PTErr_t SetKCPDataDirProps (ioFileChar *KCPDataDirProps)
{

#if defined (KPMAC)
	strcpy (KCPDataDirProps->creatorType, "KEPS");	/* set up file properties */
	strcpy (KCPDataDirProps->fileType, "PT  ");
	KCPDataDirProps->vRefNum = GetBlessed ();	/* get the system folder volume reference number */
#else
	memset (KCPDataDirProps, 0, sizeof (ioFileChar));
#endif

	return (KCP_SUCCESS);
}


