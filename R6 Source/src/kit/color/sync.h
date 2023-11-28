/*********************************************************************/
/*
	Contains:	This module contains the synchronization structures 
				and constants.

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1994 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile$
		$Logfile$
		$Revision$
		$Date$
		$Author$

	SCCS Revision:
		@(#)sync.h	1.3    11/4/94

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
 *** COPYRIGHT (c) Eastman Kodak Company, 1994                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#ifndef _SYNC_H_
#define _SYNC_H_ 1

	/***************************/
	/* Windows 32 Bit Includes */
	/***************************/

	#if defined (KPWIN32)
		#include <string.h>
	#endif

	/********************/
	/* Solaris Includes */
	/********************/

	#if defined (KPSOLARIS)
		#include <thread.h>
		#include <stdlib.h>
		#include <sys/types.h>
		#include <sys/ipc.h>
		#include <sys/sem.h>
	#endif

	/*****************************************/
	/* Common type definitions and constants */
	/*****************************************/

	#define KP_SEM_USAGE		0		/*	semaphore 0 of all semaphore
										 	sets is the usage count 
											semaphore.						*/

	/***********************************************************/
	/* Windows 32 Bit Semaphore type definitions and constants */
	/***********************************************************/

	#if defined (KPWIN32)

		typedef HANDLE KpSemId_t, FAR * KpSemId_p, FAR * FAR * KpSemId_h;

		typedef struct {
			KpUInt32_t		NumSemaphores;
			KpSemId_h		semId;
		} KpSemSetData_t, FAR * KpSemSetData_p, FAR * FAR * KpSemSetData_h;

		typedef  KpInt32_t	KpSemInitData_t;

		#define KP_SEM_MAX_VAL	0x7fffffff		

	#endif

	/*************************************************/
	/* Unix Semaphore type definitions and constants */
	/*************************************************/

	#if defined (KPUNIX)

		typedef int KpSemId_t;

		typedef struct {
			KpUInt32_t	NumSemaphores;
			KpSemId_t	SemId;
		} KpSemSetData_t, FAR * KpSemSetData_p, FAR * FAR * KpSemSetData_h;

		typedef KpUInt16_t	KpSemInitData_t;			

		#define KP_IPC_PERM_RWALL 0666

		#define KP_GET_SEMAPHORE		-1
		#define KP_RELEASE_SEMAPHORE	1

		/*
	 	 * for some reason the following is not in the solaris include
		 * files although the documentation says it should be.
		 */

		#if defined (KPSOLARIS)

			typedef union semun_u {
				int val;
				struct semid_ds *buf;
				ushort *array;
			} semun_t;
		
		#endif

	#endif

	/***********************/
	/* Function Prototypes */
	/***********************/
	

#endif



