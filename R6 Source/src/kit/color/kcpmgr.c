/*
 * @(#)kcpmgr.c	2.90 97/12/22

	Contains:       execute a KCMS function which was initially called through KCMS_proc_send

	Written by:     Drivin' Team

	Copyright:      (c) 1991-1997 by Eastman Kodak Company, all rights reserved.
 */


#define KCP_GLOBAL

#include "kcms_sys.h"
#include "bytes.h"

#include <string.h>

#include "kcmptlib.h"
#include "kcmptdef.h"
#include "kcptmgr.h"
#include "kcpmgru.h"
#if defined (KCP_ACCEL)
#include "ctelib.h"
#endif


/* standard functions */
static void KCPinitEvaluators (initializedGlobals_p iGblP);


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * PTGetFlavor
 *
 * DESCRIPTION
 * This function returns the Color Processor flavor
 *
 *--------------------------------------------------------------------*/

PTErr_t PTGetFlavor (KpInt32_t * kcpFlavor)
{
threadGlobals_p	threadGlobalsP;
KpInt32_t		ptFlavor = 0;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}
      
#if defined (KCP_COMP_2)
	ptFlavor |= PT_COMPOSITION2;
#endif

#if defined (KCP_ACCEL)
	ptFlavor |= PT_ACCELERATOR;
#endif

/* determine the evaluation type */
#if defined (KCP_EVAL_TH1)
	ptFlavor |= PT_TLI_EVALUATION1;
#endif


	*kcpFlavor = ptFlavor;
	
	return (KCP_SUCCESS);
}


/* set up the main driver */
int32
	KCMDsetup (KpGenericPtr_t FAR* IGPtr)
{
initializedGlobals_p	iGblP;			/* initialized globals */

	/* get memory for initialized globals */
	iGblP = (initializedGlobals_p) allocSysBufferPtr (sizeof(initializedGlobals_t));
	if (iGblP == NULL) {
		return (KCMS_FAIL);
	}	

	bzero ((KpGenericPtr_t) iGblP, sizeof(initializedGlobals_t));	/* set them all to 0 */

	KCPInitIGblP(IGPtr, iGblP);		/* do platform specific initialization */

	*IGPtr = (KpGenericPtr_t) iGblP;	/* return pointer to initialized globals */

#if defined (KCP_COMP_2)
	KCPChainSetup (iGblP);			/* initialize chaining rules */
#endif
	
	/* setup default pt size */
	iGblP->PTCubeSize = EIGHT_CUBE;
	KCPinitEvaluators(iGblP);	/* Setup evaluator list */

	return (KCMS_SUCCESS);
}


/* set up the globals for each application */

int32
	KCPappSetup (void* IGPtr)
{
threadGlobals_p	threadGlobalsP;
PTErr_t 		PTErr;

	/* get memory for thread globals */
	threadGlobalsP = KpInitThread();
	if (threadGlobalsP == NULL) {
		return (KCMS_FAIL);
	}

	/* get memory for process globals */
	threadGlobalsP->processGlobalsP = KpThreadMemCreate (&theRootID, KPPROCMEM, sizeof(processGlobals_t));
	if (threadGlobalsP->processGlobalsP == NULL) {
		KpTermThread(threadGlobalsP);
		return (KCMS_FAIL);
	}	

	/* initialize the thread count */
	threadGlobalsP->processGlobalsP->threadCount = 1;

	threadGlobalsP->processGlobalsP->iGP = IGPtr;
	threadGlobalsP->processGlobalsP->PTcriticalSection.SyncFlag = 0;
	KpInitializeCriticalSection(&threadGlobalsP->processGlobalsP->PTcriticalSection);
	
	PTErr = SetKCPDataDirProps(&threadGlobalsP->processGlobalsP->KCPDataDirProps);
	if (PTErr != KCP_SUCCESS) {
		diagWindow (" KCPappSetup: SetKCPDataDirProps failed", PTErr);
		KCMDunloadGlobals();					/* Unlock this apps Globals */
		return (PTErr);
	}
	
	PTErr = initPTTable (threadGlobalsP);	/* initialize the PT table */
	if (PTErr != KCP_SUCCESS) {
		diagWindow (" KCPappSetup: initPTTable failed", PTErr);
		KCMDunloadGlobals();					/* Unlock this apps Globals */
		return (PTErr);
	}
 
#if defined (KCP_COMP_2)
	clearChain(threadGlobalsP);			/* no chaining in progress */
#endif

#if defined (KCP_MACPPC_MP)
	KCPInitializeMP (threadGlobalsP);
#endif

	KCMDunloadGlobals();	/* Unlock this apps Globals */

	return (KCMS_SUCCESS);
}


/* reset the color processor to post-initialization state */
PTErr_t
	PTProcessorReset(void)
{
threadGlobals_p threadGlobalsP;
PTErr_t PTErr;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (NULL == threadGlobalsP) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	} 
      
#if defined (KCP_ACCEL)
	PTErr = PTProcessorReset_cte(); 		/* reset CTE */
#endif

#if defined (KCP_COMP_2)
	clearChain(threadGlobalsP);				/* wipe out any chaining */
#endif

	PTErr = freeApplPT (threadGlobalsP);	/* release this application's checked in PTs */

	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return (PTErr);
}


PTErr_t
	KCMDTerminate (void)
{
threadGlobals_p threadGlobalsP;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}
      
#if defined (KCP_COMP_2)
	if (threadGlobalsP->processGlobalsP->iGP->composeRuleDB != NULL) {
		freeSysBufferPtr(threadGlobalsP->processGlobalsP->iGP->composeRuleDB);	/* release memory used for composition */
	}
#endif
	
#if !defined (KPMAC)
	freeSysBufferPtr(threadGlobalsP->processGlobalsP->iGP);	/* release memory used for globals */
	threadGlobalsP->processGlobalsP->iGP = NULL;
#endif

	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return (KCP_SUCCESS);
}


PTErr_t
	PTTerminate(void)
{
threadGlobals_p threadGlobalsP;
PTErr_t	PTErr;

	threadGlobalsP = KCMDloadGlobals ();	/* Setup this apps Globals */
	if (NULL == threadGlobalsP) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	} 

	PTErr = PTTerminatePlatform (threadGlobalsP);

	return (PTErr);
}

/* Thread Stuff */

/***************************************/
/* initialize any thread specific data */
/***************************************/

#if !defined (KPMAC)
PTErr_t
	PTInitThread (void)
{
threadGlobals_p	threadGlobalsP;
PTErr_t	PTErr;
	
	threadGlobalsP = KpInitThread ();

	if (threadGlobalsP != NULL) {
		PTErr = KCP_SUCCESS;
	} else {
		PTErr = KCP_NO_MEMORY;
	}

	KCMDunloadGlobals();					/* Unlock this apps Globals */

	return (KCP_SUCCESS);
}
#endif


threadGlobals_p
	KpInitThread (void)
{
threadGlobals_p	threadGlobalsP;

	threadGlobalsP = KpThreadMemCreate (&theRootID, KPTHREADMEM, sizeof(threadGlobals_t));
	if (threadGlobalsP == NULL) {
		return (NULL);
	}

	/* set thread count */
	threadGlobalsP->threadUseCount = 1;

#if defined (KCP_EVAL_TH1)
	threadGlobalsP->evalTh1Cache = NULL;			/* tetrahedral 1 cache */
#endif
	threadGlobalsP->loopStart = 0;
	threadGlobalsP->loopCount = 0;
	threadGlobalsP->progressFunc = (PTProgress_t) NULL;

	return (threadGlobalsP);
}


/**************************************/
/* terminate any thread specific data */
/**************************************/

#if !defined (KPMAC)
PTErr_t
	PTTermThread (void)
{
threadGlobals_p threadGlobalsP;
PTErr_t	PTErr;
	
	threadGlobalsP = (threadGlobals_p)KpThreadMemFind (&theRootID, KPTHREADMEM);

	if (threadGlobalsP == NULL) {
		return (KCP_FAILURE);
	} else {
		PTErr = KpTermThread (threadGlobalsP);
	}

	return (PTErr);
}
#endif

PTErr_t
	KpTermThread (threadGlobals_p threadGlobalsP)
{
	kcpFreeCache (threadGlobalsP);

	KpThreadMemUnlock (&theRootID, KPTHREADMEM);
	KpThreadMemDestroy (&theRootID, KPTHREADMEM);

	return (KCP_SUCCESS);
}

PTErr_t
	KpTermProcess (threadGlobals_p threadGlobalsP)
{

	if (threadGlobalsP == NULL)
		return (KCP_FAILURE);

	KpThreadMemUnlock (&theRootID, KPPROCMEM);
	KpThreadMemDestroy (&theRootID, KPPROCMEM);

	return (KCP_SUCCESS);
}

/* handle the loading of globals here for all platforms! */
threadGlobals_p
	KCMDloadGlobals (void)
{
threadGlobals_p	threadGlobalsP;
	
	threadGlobalsP = (threadGlobals_p)KpThreadMemFind (&theRootID, KPTHREADMEM);
	if (threadGlobalsP == NULL) {
		return (NULL);
	}

	threadGlobalsP->processGlobalsP = (processGlobals_p) KpThreadMemFind (&theRootID, KPPROCMEM);
	if (threadGlobalsP->processGlobalsP == NULL) {
		KpThreadMemUnlock (&theRootID, KPTHREADMEM);
		return (NULL);
	}
	
	return (threadGlobalsP);
}


/* handle the unloading of globals here for all platforms! */
void
KCMDunloadGlobals (void)
{
		KpThreadMemUnlock (&theRootID, KPTHREADMEM);
		KpThreadMemUnlock (&theRootID, KPPROCMEM);
}


/* Setup which evaluators are present */
static void
KCPinitEvaluators (initializedGlobals_p	iGblP)
{
#if defined (KCP_ACCEL)
PTErr_t 	PTErr;
KpInt16_t	num;
#endif

	initList (iGblP->evalList);	/* initialize the evaluators list */

	AddEvaluator (KCP_EVAL_SW, iGblP->evalList);	/* always have software evaluator */
	
#if defined (KCP_ACCEL)
	if (iGblP->SWalways == 0) {
	/* Either we're not a mac, or we are but there was no command/s. */
		PTErr = PTProcessorReset_cte ();		/* initialize the CTE driver */
		if (PTErr == KCP_SUCCESS) {
			PTErr = PTEvaluators_cte (&num);	/* check for the CTE driver */
			if ((PTErr == KCP_SUCCESS) && (num > 0)) {
				AddEvaluator (KCP_EVAL_CTE, iGblP->evalList);   /* got one, add to evaluators list */
/*				PTErr = PTIcmGridSize(&iGblP->PTCubeSize);		/* not needed now */
			}
		}
	}
#endif

}


