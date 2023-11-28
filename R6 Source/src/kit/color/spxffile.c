/*********************************************************************/
/*
	Contains:	This module contains a platform specific code to handle
				on disk conversion PTs and debugging code.

				Created by lcc, Nov. 30, 1994

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1994-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	SCCS Revision:
		@(#)spxffile.c	1.15 9/24/97

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
 *** COPYRIGHT (c) Eastman Kodak Company, 1994-1997                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"

/*-------------------------------------------------------------------*/
/*
 * Functions:
 *		1. to get a PT from disk and return a refnum
 *		2. determine if intermediate lab<->uvl PTs should be saved 
 */
#if defined(KPWIN) && defined(_DEBUG)

KpBool_t writePTs(void)
{
#if defined(_DEBUG)
	char	prefFile[256];
	DWORD	numChars;
	KpChar_p DEBUG_SECTION = "Debug";
	KpChar_p DEBUG_ENTRY = "Write PTs";
#endif
	KpBool_t	writeEm;

//#if defined(_DEBUG)
	/* Locate Module Preference File */
    numChars=GetModuleFileName(NULL,prefFile,(DWORD)(sizeof(prefFile)-1));
    prefFile[numChars-4]='\0';
    lstrcat(prefFile,".ini");

	/* get the value of "use fixed conversions" */
	writeEm = (KpBool_t)GetPrivateProfileInt(DEBUG_SECTION, DEBUG_ENTRY, 
												0, prefFile);
//#else
//	writeEm = KPFALSE;
//#endif
	return (writeEm);
}

KpBool_t useFixed(void)
{
	char	prefFile[256];
    DWORD	numChars;
	KpChar_p FIXED_SECTION = "Fixed";
	KpChar_p FIXED_ENTRY = "Fixed Conversions";
	KpBool_t	useFixedConversions;

	/* Locate Module Preference File */
    numChars=GetModuleFileName(NULL,prefFile,(DWORD)(sizeof(prefFile)-1));
    prefFile[numChars-4]='\0';
    lstrcat(prefFile,".ini");

	/* get the value of "use fixed conversions" */
	useFixedConversions = (KpBool_t)GetPrivateProfileInt(FIXED_SECTION, FIXED_ENTRY, 
												0, prefFile);
	return (useFixedConversions);
}

KpInt32_t getPTFromFile(SpDirection_t direction, KpInt32_t Render, PTRefNum_t *thePT)
{
	KpMapFile_t	mappedFile;
	KpFileProps_t	props;
	KpChar_p	file;

	if (direction == SpDir2LAB) 
		file = uvl2labName;
	else
	{
		switch (Render)
		{
			case KCM_COLORIMETRIC:
				file = lab2uvlCName; 
				break;
			case KCM_SATURATION_MATCH:
				file = lab2uvlSName; 
				break;
			case KCM_PERCEPTUAL:
			default:
				file = lab2uvlPName; 
				break;
		}
	}

	if( !KpMapFileEx(file,&props,"r",&mappedFile) )
		return -1;

	if( PTCheckIn(thePT,mappedFile.Ptr) != KCP_SUCCESS )  {
		KpUnMapFile(&mappedFile);
		return -1;
	}

	if( PTActivate(*thePT,mappedFile.NumBytes,mappedFile.Ptr) != KCP_SUCCESS )  {
		KpUnMapFile(&mappedFile);
		PTCheckOut( *thePT );
		return -1;
	}

	KpUnMapFile(&mappedFile);

	return   0;
}

/* Used only by pt2pf to specify that fixed conversion PT files are being used */
void displayWarning(KpChar_p message)
{

char			prefFile[256];
char			logFileName[256];
char			*charPtr;
DWORD			numChars;
KpChar_p		LOG_SECTION = "Log";
KpChar_p		 LOG_ENTRY = "ConsoleLogFile";
BOOL			doLog;
char			eol[2];
KpFileProps_t	fileProps;
KpFileId		logFile;
int				kpStatus;

	/* Locate Module Preference File */
    numChars=GetModuleFileName(NULL,prefFile,(DWORD)(sizeof(prefFile)-1));
    prefFile[numChars-4]='\0';
    lstrcat(prefFile,".ini");

	/* get the value of "Write LogFile" */
	doLog = (KpBool_t)GetPrivateProfileString(LOG_SECTION, LOG_ENTRY, NULL, 
												logFileName, 256, prefFile);
	if (!doLog)
		return;
	/* Check if there is a '\' in the file name.  If so, assume
		   full path and file name is specified.  If not, assume
		   file name only and get path for .exe file */
	charPtr = strchr( logFileName, '\\' );
	if (charPtr != 0) {
		strcpy (prefFile, logFileName);
	}
	else {
	/* Generate the full name for the log file by reading
		   the file name from the .ini file and then getting the
		   path to the .exe file and making a new path to the .log
		   file.  */
		charPtr = strrchr( prefFile, '\\' );
		numChars = charPtr - prefFile + 1;
		prefFile[numChars] = '\0';
		strcat(prefFile, logFileName);
	}

   	eol[0] = 0x0d;
	eol[1] = 0x0a;

/* open the logging file */
	kpStatus =  KpFileOpen (prefFile, "w", &fileProps, &logFile);
	if (kpStatus != KCMS_IO_SUCCESS) return;

/* position to end of file */
	if (KpFilePosition(logFile, FROM_END, 0L) == 0) {
		return;
	}

	/* write buffer to log file */
	KpFileWrite (logFile, message, strlen(message)-1);
	KpFileWrite (logFile, eol, 2);

/* close the log file */
	KpFileClose (logFile);
}

#else
#if defined(_DEBUG)
KpBool_t writePTs(void)
{
	return 0;	/* return FALSE */
}
#endif

KpBool_t useFixed(void)
{
	return 0;	/* return FALSE */
}

KpInt32_t getPTFromFile(SpDirection_t direction, KpInt32_t Render, PTRefNum_t *thePT)
{
	if (direction){}
	if (Render){}
	if (thePT){}

	return -1;
}

void displayWarning(KpChar_p message)
{
	if (message){}
}
#endif



