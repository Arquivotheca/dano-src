/*********************************************************************/
/*
	Contains:	This module contains the system functions.

				Created by lsh, September 20, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPSYSTEM.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spsystem.c_v  $
		$Revision:   2.2  $
		$Date:   07 Apr 1994 13:24:02  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)spsystem.c	1.34 12/22/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1997                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include "attrcipg.h"
#include <string.h>

#if defined (KPUNIX)
#include "kcmsos.h"
#endif
#include "spuxver.h"

#if defined (KPWIN32)
extern HANDLE SpHInst;
#endif

typedef struct SpInstanceGlobals_s
{
	KpUInt32_t	currentUsers;
	KpUInt32_t	NextUsers;
} SpInstanceGlobals_t, FAR* SpInstanceGlobals_p;

/* Static variables */
static KpThreadMemHdl_t	ICCRootInstanceID;

/* Static function prototypes */
static SpStatus_t SpGetInstanceGlobals (SpInstanceGlobals_p *instanceGlobals);
static SpStatus_t SPAPI SpDoTerminate (SpCallerId_t FAR *CallerId);


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Returns the globals associated with this process.  If the globals
 *	do not exist, then create a new set.
 *------------------------------------------------------------------*/
static SpStatus_t SpGetInstanceGlobals (SpInstanceGlobals_p *instanceGlobals)
{
	SpInstanceGlobals_p	thisInstanceP;

	thisInstanceP = (SpInstanceGlobals_p)KpThreadMemFind (	&ICCRootInstanceID,
															KPPROCMEM);

	if (thisInstanceP == NULL)
	{
		thisInstanceP = KpThreadMemCreate (	&ICCRootInstanceID,
											KPPROCMEM,
											sizeof(SpInstanceGlobals_t));

		if (thisInstanceP != NULL)
		{
			thisInstanceP->currentUsers = 0;
			thisInstanceP->NextUsers = 0;
		}
		else
		{
			return (SpStatMemory);
		}
		
	}
	
	*instanceGlobals = thisInstanceP;

	return (SpStatSuccess);

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do application specific initialization.  This must be the first
 *	Profile Processor function called.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpCallerIdValidate (
				SpCallerId_t	CallerId)
{
	SpCallerIdData_t	FAR *CallerIdData;
	SpStatus_t			status;

	status = SpStatSuccess;
	
	CallerIdData = lockBuffer ((KcmHandle) CallerId);
	if (NULL == CallerIdData)
		status = SpStatBadCallerId;
	else {
		if (SpCallerIdDataSig != CallerIdData->Signature)
			status = SpStatBadCallerId;
		unlockBuffer ((KcmHandle) CallerId);
	}
	
	return status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do application specific initialization.  This must be the first
 *	Profile Processor function called.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpInitialize (
				SpCallerId_t	FAR *CallerId,
				SpProgress_t	ProgressFunc,
				void			FAR *Data)
{
#if defined (KPWIN)
	return (SpInitializeEx (CallerId, ProgressFunc,
					Data, (SpInitInfo_t *)NULL));
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do extended application specific initialization.  This must be 
 *  the first Profile Processor function called.  This function
 *  takes the place of SpInitialize.  It should only be used
 *  when the application needs to provide additional infomation
 *  to the sprofile dll at initialization time.
 *
 *  The moduleId provided is passed to the CP which uses this
 *  Id to identify the app or dll which contains the resources
 *  that are read by the CP.  This allows the application to
 *  use unique registry entries.
 *
 * AUTHOR
 * 	acr
 *
 * DATE CREATED
 *	June 13, 1996
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpInitializeEx (
				SpCallerId_t	FAR *CallerId,
				SpProgress_t	ProgressFunc,
				void			FAR *Data,
				SpInitInfo_t	FAR *InitInfo)
{
#endif
	SpCallerIdData_t	FAR *CallerIdData;
	PTErr_t				PTStat;
	SpStatus_t			status;
	SpInstanceGlobals_p	thisInstanceP;
#if defined (KPWIN)
	PTInitInfo_t		PTInitInfo;
#endif

	SpDoProgress (ProgressFunc, SpIterInit, 0, Data);

/* Get the globals */
	status = SpGetInstanceGlobals (&thisInstanceP);
	if (status != SpStatSuccess)
	{
		return (status);
	}

/* check for no current users, do global initialization */
	if (0 == thisInstanceP->currentUsers) {
#if defined (KPWIN)
		if (InitInfo == NULL)
			PTStat = PTInitialize ();
		else {
			PTInitInfo.structSize = sizeof(PTInitInfo_t);
			PTInitInfo.appModuleId = InitInfo->appModuleId;
			PTStat = PTInitializeEx (&PTInitInfo);
		}
#else
		PTStat = PTInitialize ();
#endif
		if (KCP_SUCCESS != PTStat) {
			SpDoProgress (ProgressFunc, SpIterTerm, 100, Data);
			return SpStatusFromPTErr(PTStat);
		}
		Sp_uvL2Lab.Valid = KPFALSE;
		Sp_Lab2uvL.Valid = KPFALSE;
		KpInitializeCriticalSection (&SpCacheCritFlag);
	}
	SpDoProgress (ProgressFunc, SpIterProcessing, 40, Data);

/* allocate caller id block */
	CallerIdData = SpMalloc (sizeof (*CallerIdData));
	if (NULL == CallerIdData) {
		SpDoProgress (ProgressFunc, SpIterTerm, 100, Data);
		return SpStatMemory;
	}
	SpDoProgress (ProgressFunc, SpIterProcessing, 80, Data);
	CallerIdData->Signature = SpCallerIdDataSig;

/* give the user a pointer to this block */
	*CallerId =  (SpCallerId_t) getHandleFromPtr (CallerIdData);

/* increase the number of current users */
	thisInstanceP->currentUsers++;
	thisInstanceP->NextUsers++;

/* set the caller number */
	CallerIdData->CallerId = thisInstanceP->NextUsers;
	unlockBuffer((KcmHandle)*CallerId);
	
/* Unlock the thread memory */
	KpThreadMemUnlock (&ICCRootInstanceID, KPPROCMEM);

	SpDoProgress (ProgressFunc, SpIterTerm, 100, Data);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do application specific cleanup.  This must be the last
 *	Profile Processor function called.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SPAPI SpDoTerminate (
				SpCallerId_t FAR *CallerId)
{
	SpInstanceGlobals_p	thisInstanceP;
	SpStatus_t			status;
	SpCallerIdData_t	FAR *CallerIdData;

	if (NULL == CallerId)
		return SpStatBadCallerId;

/* free caller id space */
	CallerIdData = lockBuffer ((KcmHandle) *CallerId);
	if (NULL == CallerIdData)
		return SpStatBadCallerId;
	
	SpFree ((void FAR *) CallerIdData);

/* null users handle, don't let them continue with a freed block */
	*CallerId = NULL;

/* decrease the number of current users */
	status = SpGetInstanceGlobals (&thisInstanceP);
	if (status != SpStatSuccess)
	{
		return (status);
	}
	else
	{
		thisInstanceP->currentUsers--;
	}


/* check for no more current users, do global clean-up */
	if (0 == thisInstanceP->currentUsers) {
		if (Sp_Lab2uvL.Valid) {
			PTCheckOut (Sp_Lab2uvL.RefNum);
			Sp_Lab2uvL.Valid = KPFALSE;
		}
		if (Sp_uvL2Lab.Valid) {
			PTCheckOut (Sp_uvL2Lab.RefNum);
			Sp_uvL2Lab.Valid = KPFALSE;
		}
		KpDeleteCriticalSection (&SpCacheCritFlag);
	}

/* Unlock the thread memory */
	KpThreadMemUnlock (&ICCRootInstanceID, KPPROCMEM);

	return SpStatSuccess;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do application specific cleanup.  This must be the last
 *	Profile Processor function called.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTerminate (
				SpCallerId_t FAR *CallerId)
{
	SpInstanceGlobals_p	thisInstanceP;
	SpStatus_t  spStatus;
	
	/* Get the globals */
	spStatus = SpGetInstanceGlobals (&thisInstanceP);
	if (spStatus != SpStatSuccess)
	{
		return (spStatus);
	}

	/* Unlock the thread memory */
	spStatus = SpDoTerminate(CallerId);
	if (spStatus != SpStatSuccess)
		return spStatus;
	
	if (0 == thisInstanceP->currentUsers)
	{
		/* Close the Color Processor */
		PTTerminate ();
		/* Remove thread memory */
		KpThreadMemDestroy (&ICCRootInstanceID, KPPROCMEM);
	}
		
	return spStatus;
}

#if defined (KPMAC)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do application specific initialization.  This must be the first
 *	Profile Processor function called.  If the CPInstance is not 
 *  zero - then we want to share the one initialized by the KCM API
 *  so set the value before initializing.
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	July 14, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpInitializeComp (	KpInt32_t CPInstance,
									SpCallerId_t FAR *CallerId,
									SpProgress_t ProgressFunc,
									void FAR *Data)
{
	if (CPInstance != 0)
	{
		SpSetCPInstance (CPInstance);
	}
	
	return SpInitialize (CallerId,ProgressFunc,Data);

}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do application specific cleanup.  This must be the last
 *	Profile Processor function called. It is called in place of
 *  SpTerminate.  If the value of CPJointInstance is not zero, then the
 *  the application has initialize the KCM API and the Profile
 *  API is using its  (the KCM's) instance of the CP.  This is necessary in order
 *  for the two API to use the same refnum.  The Profile API MUST not close
 *  the CP  - KCM API will close it when it terminates.
 *  
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	July 14, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTerminateComp (	SpCallerId_t FAR *CallerId,
									KpInt32_t CPJointInstance)
{
	SpInstanceGlobals_p	thisInstanceP;
	SpStatus_t			spStatus;
	PTErr_t				ptError;
	
	
	/* Get the globals */
	spStatus = SpGetInstanceGlobals (&thisInstanceP);
	if (spStatus != SpStatSuccess)
	{
		return (spStatus);
	}

	spStatus = SpDoTerminate (CallerId);
	if (spStatus != SpStatSuccess)
		return spStatus;
	
	/* If the CPJointInstance is not zero then the
	 			KCM API initialized and will close the CP */
	if (0 == thisInstanceP->currentUsers)
	{
		if (0 == CPJointInstance)
		{
			/* Close the Color Processor */
			PTTerminate ();
		}
		else
		{
			/* Free the CP's thread memory */
			ptError = PTTermGlue ();
		}

		/* Unlock the thread memory */
		KpThreadMemUnlock (&ICCRootInstanceID, KPPROCMEM);
		/* Remove thread memory */
		KpThreadMemDestroy (&ICCRootInstanceID, KPPROCMEM);
	}

	return spStatus;
}
#endif

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Gets Info about the current ICC and the current CP
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	June 30, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpGetInfo (SpDBInfo *info, KpInt32_t size)
{
	KpInt32_t		nEval;				/* number of evaluators */
	evalList_t	 	evalList[10];		/*  list of evaluators */
	KpInt32_t		len;
	char			tempBuf[256];
	PTErr_t			PTStat;
	
	/*size is currently unused but allows for future expansion*/
	if (size) {}
	
	/*
	 *   get the accelaratorPresent
	 */
	if( (PTStat = PTEvaluators(&nEval,&evalList[0])) != KCP_SUCCESS )
	{
		return SpStatusFromPTErr(PTStat);
	}
	info->acceleratorPresent = nEval-1;		/* Software is always there */
	
	/* Retrieve current version string -- SpHInst is a global defined */
	/* on the widows side in DllMain() function in sprof32.c */
/* #if defined (KPMAC)
	KpGetProductVersion (0, info->toolkitVersion, sizeof (info->toolkitVersion));
#elif */
#if defined (KPWIN32)
	KpGetProductVersion (SpHInst, info->toolkitVersion, sizeof (info->toolkitVersion));
#else
	if (sizeof (SP_VERSION) > sizeof (info->toolkitVersion))
		return SpStatBadBuffer;

	strncpy (info->toolkitVersion, SP_VERSION, sizeof (SP_VERSION));
#endif

	/*
	 * get the CP driver version
	 */
	len = sizeof (tempBuf);
	
	if ((PTStat = PTGetAttribute((int32)1,(int32)KCM_KCP_VERSION,&len,tempBuf)) != KCP_SUCCESS)
	{
		return SpStatusFromPTErr(PTStat);
	}
	
	strncpy(info->colorProcessorVersion, tempBuf, sizeof (info->colorProcessorVersion));

	return SpStatSuccess;
}



#if defined (KPMAC)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	MACINTOSH ONLY CALL - to Get the CP Instance!
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	June 30, 1995
 *------------------------------------------------------------------*/
void SpGetCPInstance (KpInt32_t *theCurCP)
{
	 KCMPTGetComponentInstance (theCurCP);
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	MACINTOSH ONLY CALL - to Set the CP Instance!
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	June 30, 1995
 *------------------------------------------------------------------*/

void SpSetCPInstance (KpInt32_t theCurCP)
{
	 KCMPTSetComponentInstance (theCurCP);
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	MACINTOSH VERSION OF SpInitThread() - DOES NOTHING TODAY!
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	November 23, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpInitThread (
				SpCallerId_t	/*CallerId*/,
				SpProgress_t	/*ProgressFunc*/,
				void			FAR */*Data*/)
{
	return SpStatSuccess;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	MACINTOSH VERSION OF SpTermThread() - DOES NOTHING TODAY!
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	November 23, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTermThread (
				SpCallerId_t /*CallerId*/)
{
	return SpStatSuccess;
}

#else

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do thread specific initialization.  This must be the first
 *	Profile Processor function called from a thread.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 26, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpInitThread (
				SpCallerId_t	CallerId,
				SpProgress_t	ProgressFunc,
				void			FAR *Data)
{
	PTErr_t		PTStat;

	SPArgUsed (CallerId);

	SpDoProgress (ProgressFunc, SpIterInit, 0, Data);
	PTStat = PTInitThread ();
	SpDoProgress (ProgressFunc, SpIterTerm, 100, Data);

	return SpStatusFromPTErr(PTStat);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do thread specific cleanup.  This must be the last
 *	Profile Processor function called from a thread.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 26, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTermThread (
				SpCallerId_t CallerId)
{
	PTErr_t		PTStat;

	SPArgUsed (CallerId);

	PTStat = PTTermThread ();

	return SpStatusFromPTErr(PTStat);
}
#endif


SpStatus_t SpStatusFromPTErr(PTErr_t PTErr)
{
SpStatus_t theStatus;

	switch	(PTErr)
	{
		case KCP_NOT_IMPLEMENTED:
			theStatus = SpStatNotImp;
			break;
			
		case KCP_SUCCESS:
			theStatus =SpStatSuccess;
			break;

		case KCP_NO_CHECKIN_MEM:
		case KCP_NO_ACTIVATE_MEM:
		case KCP_NO_ATTR_MEM:
		case KCP_NO_MEMORY:
		case KCP_NO_SYSMEM:
			theStatus = SpStatMemory;
			break;

		default:
			theStatus = SpStatCPComp;
			break;
	}
	return theStatus;

}

