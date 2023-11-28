//========================================================================
//	BResPlugInView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Jon Watte

#define DEBUG 1
#include <ByteOrder.h>
#include <Debug.h>
#include <string.h>
#include <View.h>
#include "PlugInPreferences.h"
#include "BResPlugInView.h"
#include "BResBuilder.h"
#include <Box.h>
#include <StringView.h>
#include <Window.h>

#pragma export on
extern "C" {
status_t MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& ouView);
status_t MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder);
}
#pragma export reset

const char * title = "Res Tool";

// ---------------------------------------------------------------------------
//		MakeAddOnView
// ---------------------------------------------------------------------------

status_t
MakeAddOnView(
	int32				inIndex,
	BRect				inRect,
	MPlugInPrefsView*&	outView)
{
	status_t		result = B_ERROR;

	if (inIndex == 0)
	{
		outView = new BResPlugInView(inRect, "mwbres", 0, 0);
		result = B_NO_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//		MakeAddOnBuilder
// ---------------------------------------------------------------------------

status_t
MakeAddOnBuilder(
	int32				inIndex,
	MPlugInBuilder*&	outBuilder)
{
	status_t		result = B_ERROR;

	if (inIndex == 0)
	{
		outBuilder = new BResBuilder;
		result = B_NO_ERROR;
	}

	return result;	

}

// ---------------------------------------------------------------------------
//		BResPlugInView
// ---------------------------------------------------------------------------
//	Constructor

BResPlugInView::BResPlugInView(
	BRect			inFrame,
	const char*		inName,
	ulong			inResizeMask,
	ulong			inFlags)
	: MPlugInPrefsView(inFrame, inName, inResizeMask, inFlags)
{
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
BResPlugInView::Title()
{
	return title;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
BResPlugInView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
BResPlugInView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
BResPlugInView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
//		case msgTextChanged:
//			ExtractInfo();
//			break;

	default:
		MPlugInPrefsView::MessageReceived(inMessage);
		break;
	}
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
BResPlugInView::AttachedToWindow()
{
	BRect			bounds = Bounds();
	BRect			r;
	BBox*			box;
	BStringView*	caption;
	float			top = 15.0;
	const float		left = 10.0;

	// Box
	r = bounds;
	box = new BBox(r, "mwbrestool");
	box->SetLabel("Res Tool");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box);

	// Static text
	r.Set(left, top, bounds.right - left, top + 16);
	caption = new BStringView(r, "st1", "The res tool 'mwbres' does not have any options."); 
	box->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption);
	top += 20;
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
BResPlugInView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kResMessageName, kResMessageType, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
BResPlugInView::SetData(
	BMessage&	inMessage)
{
	int32			length;
	ResPrefs*		prefs;

	if (inMessage.FindData(kResMessageName, kResMessageType, (const void**) &prefs, &length))
	{
		fNewSettings = *prefs;
		fNewSettings.SwapBigToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		ExtractInfo
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
BResPlugInView::ExtractInfo()
{
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
BResPlugInView::UpdateValues()
{
}

// ---------------------------------------------------------------------------
//		ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.

void
BResPlugInView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.  If the
//	preferences for this view can be represented as a simple struct then
//	return valid values from the GetPointers function.  If the prefs are 
//	stored in some other way then this function should copy the new
//	prefs values to the old prefs values.  If valid values are returned
//	in GetPointers then this function won't be called.

void
BResPlugInView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.  If valid values are returned
//	in GetPointers then this function won't be called.

void
BResPlugInView::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------
//	Update all the fields in the view to reflect the factory settings, or 
//	built-in defaults.

void
BResPlugInView::DoFactorySettings()
{
}

// ---------------------------------------------------------------------------
//		FilterKeyDown
// ---------------------------------------------------------------------------
//	This function allows views to handle tab keys or any other special keys
//	before the keydown is passed to the target view.  Must return false if
//	the key is not to be passed to the target view.
//	We don't do anything special with keys in this plugin.

bool
BResPlugInView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
BResPlugInView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//		SetGrey
// ---------------------------------------------------------------------------
//	Make the view draw in grey.

void
BResPlugInView::SetGrey(
	BView* 		inView)
{
	inView->SetViewColor(kPrefsGray);
	inView->SetLowColor(kPrefsGray);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
ResPrefs::SwapBigToHost()
{
	version = B_BENDIAN_TO_HOST_INT32(version);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
ResPrefs::SwapHostToBig()
{
	version = B_HOST_TO_BENDIAN_INT32(version);
}

