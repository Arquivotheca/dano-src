//========================================================================
//	MPlugInShepard.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MPlugInShepard.h"
#include "MPlugInPrefsView.h"
#include "IDEMessages.h"

// ---------------------------------------------------------------------------
//		MPlugInShepard
// ---------------------------------------------------------------------------
//	Constructor

MPlugInShepard::MPlugInShepard(
	MPreferencesWindow&		inWindow,
	MPlugInPrefsView&	inView,
	BRect			inFrame,
	const char*		inName)
	: MPreferencesView(inWindow, inFrame, inName),
	fView(inView)
{
	// Does the plug in want the ide to handle saving and reverting?
	void*	oldPtr = nil;
	void*	newPtr = nil;
	int32	prefsLength = 0;
	
	fView.GetPointers(oldPtr, newPtr, prefsLength);
	
	if (oldPtr != nil && newPtr != nil && prefsLength > 0)
	{
		SetPointers(oldPtr, newPtr, prefsLength);
		fPointersAreGood = true;
	}
	else
	{
		fPointersAreGood = false;
	}
	
	// Add the plug-in view to the window
	AddChild(&fView);
	fView.MoveTo(B_ORIGIN);

	// this is mainly for debugging
	String		name = "shepard:";
	name += inName;
	SetName(name);
}

// ---------------------------------------------------------------------------
//		~MPlugInShepard
// ---------------------------------------------------------------------------
//	Destructor

MPlugInShepard::~MPlugInShepard()
{
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MPlugInShepard::GetData(
	BMessage&	inOutMessage,
	bool		/*isProxy*/)	// indicates whether the real data need be added
{								// or just a proxy to indicate the type of data
	fView.GetData(inOutMessage);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MPlugInShepard::SetData(
	BMessage&	inOutMessage)
{
	fView.SetData(inOutMessage);
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.  If the
//	preferences for this view can be represented as a simple struct then set
//	fOldSettingsP, fNewSettingsP, and fSettingsLength from the constructor
//	of the view.  This baseclass will then be able to handle saving, reverting,
//	and value changed.  If the prefs are stored in some other way these
//	functions must be overridden to do the right thing.

void
MPlugInShepard::DoSave()
{
	if (fPointersAreGood)
		MPreferencesView::DoSave();
	else
	{
		fView.DoSave();
		SetDirty(false);
	}
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.

void
MPlugInShepard::DoRevert()
{
	if (fPointersAreGood)
	{
		memcpy(fNewSettingsP, fOldSettingsP, fSettingsLength);
		fView.UpdateValues();
		SetDirty(false);
		Window()->PostMessage(msgPrefsViewModified);
	}
	else
		fView.DoRevert();
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------
//	Update all the fields in the view to reflect the factory settings, or 
//	built-in defaults.

void
MPlugInShepard::DoFactorySettings()
{
	fView.DoFactorySettings();
	UpdateValues();
	MPreferencesView::ValueChanged();
}

// ---------------------------------------------------------------------------
//		ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.

void
MPlugInShepard::ValueChanged()
{
	fView.ValueChanged();
	
	MPreferencesView::ValueChanged();
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MPlugInShepard::UpdateValues()
{
	fView.UpdateValues();	
}

// ---------------------------------------------------------------------------
//		LastCall
// ---------------------------------------------------------------------------
//	Called when the view is about to be taken offscreen.  This will happen
//	either when the preferences window is hidden or when this view is hidden.
//	Hide may also be called if the view is being hidden so don't do the
//	same things in both LastCall and Hide.

void
MPlugInShepard::LastCall()
{
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MPlugInShepard::Targets()
{
	return fView.Targets();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MPlugInShepard::Title()
{
	return fView.Title();
}

// ---------------------------------------------------------------------------
//		ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	Return true if a change in this prefs panel requires that the project be 
//	relinked or recompiled.

bool
MPlugInShepard::ProjectRequiresUpdate(
	UpdateType inType)
{
	return fView.ProjectRequiresUpdate(inType);
}

// ---------------------------------------------------------------------------
//		AdjustDirtyState
// ---------------------------------------------------------------------------

void
MPlugInShepard::AdjustDirtyState()
{
	bool		isdirty = true;

	if (fOldSettingsP && fNewSettingsP)
		isdirty = 0 != memcmp(fOldSettingsP, fNewSettingsP, fSettingsLength);

	if (isdirty != IsDirty())
	{
		SetDirty(isdirty);
	}
}

// ---------------------------------------------------------------------------
//		SetDirty
// ---------------------------------------------------------------------------

void
MPlugInShepard::SetDirty(
	bool	inDirty)
{
	MPreferencesView::SetDirty(inDirty);
	fView.SetDirty(inDirty);
}

// ---------------------------------------------------------------------------
//		IsDirty
// ---------------------------------------------------------------------------

bool
MPlugInShepard::IsDirty()
{
	return fView.IsDirty();
}

// ---------------------------------------------------------------------------
//		Show
// ---------------------------------------------------------------------------

void
MPlugInShepard::Show()
{
	MPreferencesView::Show();
	fView.Show();
}

// ---------------------------------------------------------------------------
//		Hide
// ---------------------------------------------------------------------------

void
MPlugInShepard::Hide()
{
	MPreferencesView::Hide();
	fView.Hide();
}

