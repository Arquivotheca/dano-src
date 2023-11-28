// ---------------------------------------------------------------------------
/*
	Plugin.cpp
	
	Author:	John R. Dance
			21 June 1999

*/
// ---------------------------------------------------------------------------

#include "NASMBuilder.h"
#include "PlugInPreferences.h"
#include "GCCBuilderHandler.h"
#include "CommandLineOptionsTextView.h"

const char* kNASMOptionsMessageName = "NASMOptions";
ulong kNASMMessageType = 'nasm';

// ---------------------------------------------------------------------------

extern "C"
status_t
MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& outView)
{
	// currently we don't support any preference views

	long result = B_OK;
	
	switch (inIndex) {
		case 0:
			outView = new CommandLineOptionsTextView(inRect,
													 "NASM Options",
													 "Command Line Options",
													 kNASMOptionsMessageName,
													 kNASMMessageType, 
													 kCompileUpdate,
													 "");
			break;
		default:
			result = B_ERROR;
			break;
	}
	
	return result;
}

// ---------------------------------------------------------------------------

extern "C"
status_t
MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder)
{
	long result;

	switch (inIndex) {
		case 0:
			outBuilder = new NASMBuilder;
			result = B_OK;
			break;
		default:
			result = B_ERROR;
			break;
	}

	return result;
}

// ---------------------------------------------------------------------------

extern "C"
status_t
MakeAddOnLinker(MPlugInLinker*& outLinker)
{
	// assembly only - no linking
	return B_ERROR;	
}

