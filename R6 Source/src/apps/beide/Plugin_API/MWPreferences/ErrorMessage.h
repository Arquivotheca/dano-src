//========================================================================
//	ErrorMessage.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _ERRORMESSAGE_H
#define _ERRORMESSAGE_H

const long MAX_ERROR_TEXT_LENGTH = 1024;
const long MAX_ERROR_PATH_LENGTH = 256;

struct ErrorMessage {
	long			linenumber;		/*	error linenumber in file	*/
	long			offset;			/*	offset of error in file		*/
	short			length;			/*	length of error token (in program text)	*/
	char			sync[32];		/*	32 bytes of data to find the error		*/
	short			synclen;		/*	length of sync array	*/
	short			syncoffset;		/*	selection offset in sync array	*/	
	short			errorlength;	/*	length of error token in following errorstring data		*/
	short			erroroffset;	/*	offset of errorposition in following errorstring data	*/
	bool			textonly;		/*	only the following fields are valid	*/
	bool			isWarning;		/*	TRUE: is warning; FALSE: is error	*/
	char			filename[MAX_ERROR_PATH_LENGTH];
	char			errorMessage[MAX_ERROR_TEXT_LENGTH];
};

#endif
