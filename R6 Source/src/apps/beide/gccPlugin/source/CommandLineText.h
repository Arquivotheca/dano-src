// ---------------------------------------------------------------------------
/*
	CommandLineText.h
	
	A BMessage type for a bunch of commandline options
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			22 September 1998

*/
// ---------------------------------------------------------------------------

#ifndef _COMMANDLINETEXT_H
#define _COMMANDLINETEXT_H

#include <SupportDefs.h>
#include <ByteOrder.h>

class BList;

class CommandLineText
{
public:
	enum						{ kTextLength = 1024 };
	int32						fVersion;
	char						fText[kTextLength];

	static const int32			kCurrentVersion;

	// notice that CommandLineText must be a struct.. no virtual functions	
	void						FillArgvList(BList& inArgv);
	void 						SetDefaults();

	void SwapLittleToHost()		{ fVersion = B_LENDIAN_TO_HOST_INT32(fVersion); }
	void SwapHostToLittle()		{ fVersion = B_HOST_TO_LENDIAN_INT32(fVersion); }

};

#endif
