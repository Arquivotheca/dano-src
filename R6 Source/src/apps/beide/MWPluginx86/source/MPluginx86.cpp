//========================================================================
//	MPlugIn.cpp
//	Copyright 1996 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>
#include "PlugInPreferences.h"
#include "MWarningsView.h"
#include "MLanguageView.h"
#include "MProjectPrefsViewx86.h"
#include "MGlobalOptsx86.h"
#include "MCodeGenx86.h"
#include "MLinkerViewx86.h"
#include "MmwccBuilderx86.h"
#include "MLinkerBuilderx86.h"
#include "IDEConstants.h"
#include "Utils.h"

#include "MLimited.h"

#pragma export on
extern "C" {
status_t MakePlugInView(long inIndex, BRect inRect, MPlugInPrefsView*& ouView);
status_t MakePlugInBuilder(long inIndex, MPlugInBuilder*& outBuilder);
status_t MakePlugInLinker(MPlugInLinker*& outLinker);
}
#pragma export reset

// ---------------------------------------------------------------------------
//		¥ MakePlugInView
// ---------------------------------------------------------------------------

status_t
MakePlugInView(
	long				inIndex,
	BRect				inRect,
	MPlugInPrefsView*&	outView)
{
#ifdef LIMITED
	if (LinkerSize() != kMWLDSize)
		return B_ERROR;
#endif
	status_t		result = B_NO_ERROR;

	switch (inIndex)
	{
		case 0:
			outView = new MProjectPrefsViewx86(inRect);
			break;

		case 1:
			outView = new MLanguageView(inRect);
			break;

		case 2:
			outView = new MWarningsView(inRect);
				break;

		case 3:
			outView = new MGlobalOptsx86(inRect);
			break;

		case 4:
			outView = new MCodeGenx86(inRect);
			break;

		case 5:
			outView = new MLinkerViewx86(inRect);
			break;

		default:
			result = B_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//		¥ MakePlugInBuilder
// ---------------------------------------------------------------------------

status_t
MakePlugInBuilder(
	long				inIndex,
	MPlugInBuilder*&	outBuilder)
{
#ifdef LIMITED
	if (LinkerSize() != kMWLDSize)
		return B_ERROR;
#endif
	status_t		result = B_NO_ERROR;

	switch (inIndex)
	{
		case 0:
			outBuilder = new MmwccBuilderx86;
			break;

		default:
			result = B_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//		¥ MakePlugInLinker
// ---------------------------------------------------------------------------

status_t
MakePlugInLinker(
	MPlugInLinker*&	outLinker)
{
#ifdef LIMITED
	if (LinkerSize(true) != kMWLDSize)
		return B_ERROR;
#endif

	outLinker = new MLinkerBuilderx86;

	return B_NO_ERROR;	
}

// ---------------------------------------------------------------------------
//		¥ DisallowInvalidChars
// ---------------------------------------------------------------------------
//	Prevent any of these useless and illegal characters to appear in our
//	text window and various edit boxes.

void
DisallowInvalidChars(
	BTextView& 		inText)
{
	// None of these keys do anything or can be allowed in C++ text
	inText.DisallowChar(B_ESCAPE);
	inText.DisallowChar(B_INSERT);
	inText.DisallowChar(B_FUNCTION_KEY);
	inText.DisallowChar(B_PRINT_KEY);
	inText.DisallowChar(B_SCROLL_KEY);
	inText.DisallowChar(B_PAUSE_KEY);
}

// ---------------------------------------------------------------------------
//		¥ SetGrey
// ---------------------------------------------------------------------------
//	Make this view draw in grey.

void
SetGrey(
	BView* 		inView,
	rgb_color	inColor)
{
	inView->SetViewColor(inColor);
	inView->SetLowColor(inColor);
}

// ---------------------------------------------------------------------------
//		¥ LinkerSize
// ---------------------------------------------------------------------------

off_t
LinkerSize(
	bool	inShowAlert)
{
	static bool			initialized = false;
	static entry_ref	linkerRef;

	if (! initialized)
	{
		app_info		info;
		
		be_app->GetAppInfo(&info);
		BEntry			entry(&info.ref);
		BDirectory		developDir;
		entry.GetParent(&developDir);
		developDir.FindEntry("tools/mwldx86", &entry);
		entry.GetRef(&linkerRef);
		initialized = true;
	}	

	int32			size = 0;
	BEntry			linker(&linkerRef);
	status_t		err = GetAddonSize(linker, size);

	if (inShowAlert && size != kMWLDSize)
	{
		BAlert*		alert = new BAlert("", "Couldn't find valid copy of mwldx86.", "OK");
		alert->Go();
	}
	
//	printf("mwld size = %d, showalert = %d, err = %x\n", size, (int) inShowAlert, err);

	return size;
}
