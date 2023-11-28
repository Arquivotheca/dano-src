//========================================================================
//	MPreferencesView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MPreferencesView.h"
#include "IDEMessages.h"

// ---------------------------------------------------------------------------
//		MPreferencesView
// ---------------------------------------------------------------------------
//	Constructor

MPreferencesView::MPreferencesView(
	MPreferencesWindow&	inWindow,
	BRect			inFrame,
	const char*		inName,
	uint32			inResizeMask,
	uint32			inFlags)
	: BView(inFrame, inName, inResizeMask, inFlags),
	fWindow(inWindow)
{
	fDirty = false;
	fOldSettingsP = nil;
	fNewSettingsP = nil;
	fSettingsLength = 0;
}

// ---------------------------------------------------------------------------
//		~MPreferencesView
// ---------------------------------------------------------------------------
//	Destructor

MPreferencesView::~MPreferencesView()
{
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MPreferencesView::GetData(
	BMessage&	/*inOutMessage*/,
	bool		/*isProxy*/)	// indicates whether the real data need to be added
{								// or just a proxy to indicate the type of data
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MPreferencesView::SetData(
	BMessage&	/*inOutMessage*/)
{
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MPreferencesView::UpdateValues()
{
}

// ---------------------------------------------------------------------------
//		ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.

void
MPreferencesView::ValueChanged()
{
	bool		isdirty = false;

	if (fOldSettingsP && fNewSettingsP)
		isdirty = 0 != memcmp(fOldSettingsP, fNewSettingsP, fSettingsLength);

	if (isdirty != IsDirty())
	{
		SetDirty(isdirty);

		Window()->PostMessage(msgPrefsViewModified);
	}
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.  If the
//	preferences for this view can be represented as a simple struct then set
//	fOldSettingsP, fNewSettingsP, and fSettingsLength from the constructor
//	of the view.  This baseclass will then be able to handle saving, reverting,
//	and value changed.  If the prefs are stored in some other way then these
//	functions must be overridden to do the right thing.

void
MPreferencesView::DoSave()
{
	if (fOldSettingsP && fNewSettingsP)
	{
		// Update the old settings struct so it matches the new settings struct
		memcpy(fOldSettingsP, fNewSettingsP, fSettingsLength);
		
		SetDirty(false);
	}
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.

void
MPreferencesView::DoRevert()
{
	if (fOldSettingsP && fNewSettingsP)
	{
		// Copy the old settings struct to the new settings struct,
		// update the controls in the window and check if we're dirty
		memcpy(fNewSettingsP, fOldSettingsP, fSettingsLength);
		UpdateValues();
		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------
//	Update all the fields in the view to reflect the factory settings, or 
//	built-in defaults.

void
MPreferencesView::DoFactorySettings()
{
}

// ---------------------------------------------------------------------------
//		Hide
// ---------------------------------------------------------------------------
//	Save the current focus so that we can restore it any time this view
//	comes back.

void
MPreferencesView::Hide()
{
	LastCall();
	BView::Hide();
}

// ---------------------------------------------------------------------------
//		LastCall
// ---------------------------------------------------------------------------
//	Called when the view is about to be taken offscreen.  This will happen
//	either when the preferences window is hidden or when this view is hidden.
//	Hide may also be called if the view is being hidden so don't do the
//	same things in both LastCall and Hide.

void
MPreferencesView::LastCall()
{
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MPreferencesView::Targets()
{
	return kNoTarget;
}

// ---------------------------------------------------------------------------
//		Actions
// ---------------------------------------------------------------------------

MakeActionT
MPreferencesView::Actions()
{
	return 0;
}

// ---------------------------------------------------------------------------
//		ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MPreferencesView::ProjectRequiresUpdate(
	UpdateType /*inType*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		SetDirty
// ---------------------------------------------------------------------------

void
MPreferencesView::SetDirty(
	bool	inDirty)
{
	fDirty = inDirty;
}

// ---------------------------------------------------------------------------
//		IsDirty
// ---------------------------------------------------------------------------

bool
MPreferencesView::IsDirty()
{
	return fDirty;
}
