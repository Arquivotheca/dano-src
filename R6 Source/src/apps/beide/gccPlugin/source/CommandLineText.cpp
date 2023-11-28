// ---------------------------------------------------------------------------
/*
	CommandLineText.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			22 September 1998

*/
// ---------------------------------------------------------------------------

#include "CommandLineText.h"
#include "PlugIn.h"
#include "PlugInUtil.h"

#include <string.h>

#include <List.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// CommandLineText Static Data Members
// ---------------------------------------------------------------------------

const int32	CommandLineText::kCurrentVersion = 1;

// ---------------------------------------------------------------------------
// CommandLineText Member Functions
// ---------------------------------------------------------------------------

void
CommandLineText::FillArgvList(BList& inArgv)
{
	if (strlen(fText)) {
		PlugInUtil::AddMultipleOptions(inArgv, fText);
	}
}

// ---------------------------------------------------------------------------

void
CommandLineText::SetDefaults()
{
	// default for text is completely clear...
	memset(fText, 0, kTextLength);
	fVersion = kCurrentVersion;
}
