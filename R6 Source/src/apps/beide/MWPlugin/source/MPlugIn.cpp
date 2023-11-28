//========================================================================
//	MPlugIn.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>
#include "PlugInPreferences.h"
#include "MWarningsView.h"
#include "MLanguageView.h"
#include "MGlobalOptsView.h"
#include "MProcessorView.h"
#include "MDisassembleView.h"
#include "MLinkerView.h"
#include "MPEFView.h"
#include "MProjectPrefsView.h"
#include "MmwccBuilder.h"
#include "MLinkerBuilder.h"
#include "IDEConstants.h"
#include "Utils.h"

#include <Application.h>
#include <TextView.h>
#include <Alert.h>
#include <Debug.h>

#pragma export on
extern "C" {
status_t MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& outView);
status_t MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder);
status_t MakeAddOnLinker(MPlugInLinker*& outLinker);
}
#pragma export reset

// ---------------------------------------------------------------------------
//		 MakeAddOnView
// ---------------------------------------------------------------------------

status_t
MakeAddOnView(
	int32				inIndex,
	BRect				inRect,
	MPlugInPrefsView*&	outView)
{
	long		result = B_NO_ERROR;

	switch (inIndex)
	{
		case 0:
			outView = new MProjectPrefsView(inRect);
			break;

		case 1:
			outView = new MLanguageView(inRect);
			break;

		case 2:
			outView = new MWarningsView(inRect);
				break;

		case 3:
			outView = new MGlobalOptsView(inRect);
			break;

		case 4:
			outView = new MProcessorView(inRect);
			break;

		case 5:
			outView = new MDisassembleView(inRect);
			break;

		case 6:
			outView = new MLinkerView(inRect);
			break;

		case 7:
			outView = new MPEFView(inRect);
			break;

		default:
			result = B_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//		 MakeAddOnBuilder
// ---------------------------------------------------------------------------

status_t
MakeAddOnBuilder(
	int32				inIndex,
	MPlugInBuilder*&	outBuilder)
{
	long		result = B_NO_ERROR;

	switch (inIndex)
	{
		case 0:
			outBuilder = new MmwccBuilder;
			break;

		default:
			result = B_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//		 MakeAddOnLinker
// ---------------------------------------------------------------------------

status_t
MakeAddOnLinker(
	MPlugInLinker*&	outLinker)
{
	outLinker = new MLinkerBuilder;

	return B_NO_ERROR;	
}

// ---------------------------------------------------------------------------
//		 DisallowInvalidChars
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
//		 SetGrey
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
// MWPlugin uses just pieces of MDefaultPrefs.
// gcc doesn't like the references to stuff that we don't compile
// When compiling with gcc, we have to supply our own stub functions...
// ---------------------------------------------------------------------------

#ifdef __GNUC__
	void GetBFontFamilyAndStyle(
								const BFont&	inFont,
								font_family		outFamily,
								font_style		outStyle)
	{
		TRESPASS();
	}
#endif
