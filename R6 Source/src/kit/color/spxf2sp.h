/*********************************************************************/
/*
	Contains:	This header defines the KCM API interface to the Profile
				Processor.

				Created by acr, January 31, 1996

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1996 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   
		$Logfile:   
		$Revision$
		$Date$
		$Author$

	SCCS Revision:
		@(#)spxf2sp.h	1.5	9/24/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** PROPRIETARY NOTICE:     The  software  information   contained ***
 *** herein is the  sole property of  Eastman Kodak Company  and is ***
 *** provided to Eastman Kodak users under license for use on their ***
 *** designated  equipment  only.  Reproduction of  this matter  in ***
 *** whole  or in part  is forbidden  without the  express  written ***
 *** consent of Eastman Kodak Company.                              ***
 ***                                                                ***
 *** COPYRIGHT (c) Eastman Kodak Company, 1996                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/
#ifndef SPXF2SP_H
#define SPXF2SP_H

#if defined(__cplusplus)
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#if defined KPUNIX
#if !defined (NOAPI)
#define NOAPI
#endif
#else
#define SPXF2SP 1
#include "kcm.h"

SpStatus_t SPAPI SpXformToKcmXform(
				SpXform_t spXform,
				KcmXform FAR *kcmXform);

SpStatus_t SPAPI SpXformFromKcmXform (
				KcmXform kcmXform,
				SpXform_t FAR *spXform);
				
SpStatus_t SPAPI
	SpXformFromKcmXformNC (KcmXform 	kcmXform,
						   SpXform_t 	FAR *spXform);

SpStatus_t SPAPI
	SpAndKcmInitialize (SpCallerId_t	FAR *CallerId,
						SpProgress_t	ProgressFunc,
						KpInt32_t		*CPInstance,
						void			FAR *Data);
SpStatus_t SPAPI
	SpAndKcmTerminate (	KpInt32_t	CPInstance, SpCallerId_t	FAR *CallerId);					

#endif

#if defined (KPMACPPC)
#pragma options align=reset
#endif

#if defined(__cplusplus)
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif	/* SPXF2SP_H */
ÿ