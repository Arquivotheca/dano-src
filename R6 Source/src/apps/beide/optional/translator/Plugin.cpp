// ---------------------------------------------------------------------------
/*
	GenericBuilder.cpp
	
	Author:	John R. Dance
			26 February 1999

*/
// ---------------------------------------------------------------------------

#include "GenericBuilder.h"
#include "PlugInPreferences.h"

#include "FlexHelper.h"
#include "BisonHelper.h"
#include "ResHelper.h"
#include "RezHelper.h"

// ---------------------------------------------------------------------------
// for PPC
// ---------------------------------------------------------------------------
#pragma export on
extern "C" {
status_t MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& ouView);
status_t MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder);
status_t MakeAddOnLinker(MPlugInLinker*& outLinker);
}
#pragma export reset

// ---------------------------------------------------------------------------

extern "C"
status_t
MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& outView)
{
	// currently we don't support any preference views
	return B_ERROR;
}

// ---------------------------------------------------------------------------

extern "C"
status_t
MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder)
{
	long result;

	switch (inIndex) {
		case 0:
			outBuilder = new GenericBuilder(new FlexHelper);
			result = B_OK;
			break;
		case 1:
			outBuilder = new GenericBuilder(new BisonHelper);
			result = B_OK;
			break;
		case 2:
			outBuilder = new GenericBuilder(new ResHelper);
			result = B_OK;
			break;
		case 3:
			outBuilder = new GenericBuilder(new RezHelper);
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
	// generic translations do not involve linking
	return B_ERROR;	
}

