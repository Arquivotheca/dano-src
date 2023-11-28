//========================================================================
//	MFormatUtils.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFORMATUTILS_H
#define _MFORMATUTILS_H

#include <sys/types.h>

enum TextFormatType
{
	kNewLineFormat,
	kMacFormat,
	kCRLFFormat
};


class MFormatUtils
{
public:

static	TextFormatType			FindFileFormat(
									const char * 	inBuffer);
static	void					ConvertToNewLineFormat( 
									char * 			inBuffer, 
									off_t& 			inLength,
									TextFormatType	inExistingFormat);
static	void					ConvertToNativeFormat( 
									char *&			inBuffer, 
									long&			inLength,
									long			inLines,
									TextFormatType	inNativeFormat);
};

#endif
