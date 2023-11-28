//========================================================================
//	MShellPlugInView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#define DEBUG 1
#include <Debug.h>
#include <string.h>
#include <View.h>
#include <Box.h>
#include <ByteOrder.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Window.h>
#include "PlugInPreferences.h"
#include "MShellPlugInView.h"
#include "MShellBuilder.h"
#include "MTextView.h"

#pragma export on
extern "C" {
status_t MakeAddOnView(long inIndex, BRect inRect, MPlugInPrefsView*& ouView);
status_t MakeAddOnBuilder(long inIndex, MPlugInBuilder*& outBuilder);
}
#pragma export reset


const char * title = "Shell Tool";

// ---------------------------------------------------------------------------
//	MakeAddOnView
// ---------------------------------------------------------------------------

status_t
MakeAddOnView(
	int32				inIndex,
	BRect				inRect,
	MPlugInPrefsView*&	outView)
{
	long		result = B_ERROR;

	if (inIndex == 0)
	{
		outView = new MShellPlugInView(inRect, "shell", 0, 0);
		result = B_NO_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//	MakeAddOnBuilder
// ---------------------------------------------------------------------------

status_t
MakeAddOnBuilder(
	int32				inIndex,
	MPlugInBuilder*&	outBuilder)
{
	status_t		result = B_ERROR;

	if (inIndex == 0)
	{
		outBuilder = new MShellBuilder;
		result = B_NO_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
//	MShellPlugInView
// ---------------------------------------------------------------------------
//	Constructor

MShellPlugInView::MShellPlugInView(
	BRect			inFrame,
	const char*		inName,
	ulong			inResizeMask,
	ulong			inFlags)
	: MPlugInPrefsView(inFrame, inName, inResizeMask, inFlags)
{
}

// ---------------------------------------------------------------------------
//	Title
// ---------------------------------------------------------------------------

const char *
MShellPlugInView::Title()
{
	return title;
}

// ---------------------------------------------------------------------------
//	Targets
// ---------------------------------------------------------------------------

TargetT
MShellPlugInView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//	GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MShellPlugInView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//	MessageReceived
// ---------------------------------------------------------------------------

void
MShellPlugInView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
			ExtractInfo();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//	AttachedToWindow
// ---------------------------------------------------------------------------

void
MShellPlugInView::AttachedToWindow()
{
	BRect			bounds = Bounds();
	BRect			r;
	BBox*			box;
	MTextView*		textBox;
	BScrollView*	frame;
	BStringView*	caption;
	float			top = 15.0;
	const float		left = 10.0;
	// Box
	r = bounds;
	box = new BBox(r, "shelltool");
	box->SetLabel("Shell Tool");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box);

	// Static text
	r.Set(left, top, bounds.right - left, top + 16);
	caption = new BStringView(r, "st1", "Enter command line options for the shell tool:"); 
	box->AddChild(caption);
	box->SetFont(be_bold_font);
	SetGrey(caption);
	top += 20;

	// Edit Box
	r.Set(left, top,  bounds.right - left, top + 48);
	BRect	rr = r;
	rr.OffsetTo(B_ORIGIN);
	rr.InsetBy(2.0, 2.0);
	textBox = new MTextView(r, "textview");
	textBox->SetTarget(this);
	fTextBox = textBox;

	frame = new BScrollView("frame", textBox);		// For the border
	box->AddChild(frame);

	textBox->SetMaxBytes(255);
	textBox->SetWordWrap(true);
}

// ---------------------------------------------------------------------------
//	GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.
//	The prefs struct is stored as big endian in the project.  We only swap
//	in GetData and SetData.  Everywhere else the prefs struct is host-endian.

void
MShellPlugInView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();		// prefs are stored big endian in the project
	inOutMessage.AddData(kShellMessageName, kShellMessageType, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//	SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MShellPlugInView::SetData(
	BMessage&	inMessage)
{
	ShellPrefs*		prefs;
	int32			length;

	if (B_NO_ERROR == inMessage.FindData(kShellMessageName, kShellMessageType, (const void**) &prefs, &length))
	{
		fNewSettings = *prefs;
		fNewSettings.SwapBigToHost();		// swap to host endian
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//	ExtractInfo
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MShellPlugInView::ExtractInfo()
{
	memset(fNewSettings.options, 0, sizeof(fNewSettings.options));
	strcpy(fNewSettings.options, fTextBox->Text());

	ValueChanged();
}

// ---------------------------------------------------------------------------
//	UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MShellPlugInView::UpdateValues()
{
	fTextBox->SetText(fNewSettings.options);
	fTextBox->Invalidate();
}

// ---------------------------------------------------------------------------
//	ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.

void
MShellPlugInView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//	DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.  If the
//	preferences for this view can be represented as a simple struct then
//	return valid values from the GetPointers function.  If the prefs are 
//	stored in some other way then this function should copy the new
//	prefs values to the old prefs values.  If valid values are returned
//	in GetPointers then this function won't be called.

void
MShellPlugInView::DoSave()
{
}

// ---------------------------------------------------------------------------
//	DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.  If valid values are returned
//	in GetPointers then this function won't be called.

void
MShellPlugInView::DoRevert()
{
}

// ---------------------------------------------------------------------------
//	DoFactorySettings
// ---------------------------------------------------------------------------
//	Update all the fields in the view to reflect the factory settings, or 
//	built-in defaults.

void
MShellPlugInView::DoFactorySettings()
{
	memset(fNewSettings.options, 0, sizeof(fNewSettings.options));
	strcpy(fNewSettings.options, "-noprofile");
}

// ---------------------------------------------------------------------------
//	FilterKeyDown
// ---------------------------------------------------------------------------
//	This function allows views to handle tab keys or any other special keys
//	before the keydown is passed to the target view.  Must return false if
//	the key is not to be passed to the target view.
//	We don't do anything special with keys in this plugin.

bool
MShellPlugInView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//	ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MShellPlugInView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//	SetGrey
// ---------------------------------------------------------------------------
//	Make the view draw in grey.

void
MShellPlugInView::SetGrey(
	BView* 		inView)
{
	inView->SetViewColor(kPrefsGray);
	inView->SetLowColor(kPrefsGray);
}

// ---------------------------------------------------------------------------
//	SwapHostToBig
// ---------------------------------------------------------------------------

void
ShellPrefs::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		version = B_HOST_TO_BENDIAN_INT32(version);
	}
}

// ---------------------------------------------------------------------------
//	SwapHostToBig
// ---------------------------------------------------------------------------

void
ShellPrefs::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		version = B_BENDIAN_TO_HOST_INT32(version);
	}
}
