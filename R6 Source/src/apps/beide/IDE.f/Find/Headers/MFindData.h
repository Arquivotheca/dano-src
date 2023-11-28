//========================================================================
//	MFindData.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFINDDATA_H
#define _MFINDDATA_H

struct FindData
{
	bool					fBatch;
	bool					fWrap;
	bool					fIgnoreCase;
	bool					fEntireWord;
	bool					fRegexp;
	bool					fMultiFile;
	bool					fStopAtEOF;
	bool					fResetMulti;
	bool					fOnlyOne;
	bool					fForward;
	bool					fReplaceAll;
	bool					fRestartFile;
	char					FindString[256];
	char					ReplaceString[256];
};

#endif
