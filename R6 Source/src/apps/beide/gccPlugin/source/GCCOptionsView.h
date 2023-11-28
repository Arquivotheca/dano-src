// ---------------------------------------------------------------------------
/*
	GCCOptionsView.h
	
	Provides an implementation of Metrowerks plug-in preference view that
	implements a base for all the gcc option preferences views.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			28 October 1998

*/
// ---------------------------------------------------------------------------

#ifndef _GCCOPTIONSVIEW_H
#define _GCCOPTIONSVIEW_H

#include "PlugInUtil.h"
#include "MPlugInPrefsView.h"
#include "PlugInPreferences.h"
#include "IDEMessages.h"
#include "MTextView.h"

#include <string.h>
#include <Window.h>
#include <Message.h>
#include <CheckBox.h>
class BBox;

// ---------------------------------------------------------------------------
//	GCCOptionsView
//	A base class that implements all the busy work for handling options
// ---------------------------------------------------------------------------

template <class T>
class GCCOptionsView : public MPlugInPrefsView
{
public:	
						GCCOptionsView(BRect inFrame,
									   const char* title,
									   const char* messageName,
									   type_code messageType,
									   UpdateType updateType);
	virtual				~GCCOptionsView();

	// MPlugInPrefsView Overrides
	virtual const char*	Title();
	virtual TargetT		Targets();

	virtual void		GetData(BMessage& inOutMessage);
	virtual void		SetData(BMessage& inOutMessage);

	virtual void		GetPointers(void*& outOld, void*& outNew, long& outLength);
	virtual void		DoSave();
	virtual void		DoRevert();
	virtual void		DoFactorySettings();
	
	virtual void		ValueChanged();

	virtual	bool		FilterKeyDown(ulong aKey);	
	virtual	bool		ProjectRequiresUpdate(UpdateType inType);

	// BView Overrides
	virtual void		MessageReceived(BMessage* inMessage);

	// derived classes must override the following 5 methods (public & protected)
	virtual void	AttachedToWindow() = 0;
	virtual void	UpdateValues() = 0;

protected:
	virtual void	GetUserTextChanges() = 0;
	virtual void	GetUserCheckBoxesChanges() = 0;
	virtual void	GetUserPopupChanges() = 0;

	// helper methods
	BCheckBox*			CreateCheckBox(BRect& r, const char* boxLabel, BBox& parent);

protected:
	T				fNewSettings;
	T				fOldSettings;
	type_code		fMessageType;
	UpdateType		fUpdateType;
	char*			fMessageName;	
	char*			fTitle;
};

// ---------------------------------------------------------------------------
//	GCCOptionsView member functions
// ---------------------------------------------------------------------------

template <class T>
GCCOptionsView<T>::GCCOptionsView(BRect inFrame,
								  const char* title,
								  const char* messageName,
								  type_code messageType,
								  UpdateType updateType)
				  : MPlugInPrefsView(inFrame, title, B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fMessageType = messageType;
	fUpdateType = updateType;

	fMessageName = new char[strlen(messageName) + 1];
	strcpy(fMessageName, messageName);
		
	fTitle = new char[strlen(title) + 1];
	strcpy(fTitle, title);
}

// ---------------------------------------------------------------------------

template <class T>
GCCOptionsView<T>::~GCCOptionsView()
{
	delete [] fMessageName;
	delete [] fTitle;
}

// ---------------------------------------------------------------------------

template <class T>
const char*
GCCOptionsView<T>::Title()
{
	return fTitle;
}

// ---------------------------------------------------------------------------

template <class T>
TargetT
GCCOptionsView<T>::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::GetData(BMessage& inOutMessage)
{
	fNewSettings.SwapHostToLittle();
	inOutMessage.AddData(fMessageName, fMessageType, &fNewSettings, sizeof(T));
	fNewSettings.SwapLittleToHost();
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::SetData(BMessage& inOutMessage)
{
	// set the panel to reflect the values from inOutMessage
	
	T*	settings;
	long settingLength;

	if (inOutMessage.FindData(fMessageName, fMessageType, 
							  (const void**) &settings, &settingLength) == B_OK) {
		fNewSettings = *settings;
		fNewSettings.SwapLittleToHost();
		fOldSettings = fNewSettings;
				
		this->UpdateValues();
	}
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::GetPointers(void*& outOld, void*& outNew, long& outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(T);
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::DoSave()
{
	// not needed because we support GetPointers
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::DoRevert()
{
	// not needed because we support GetPointers
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::DoFactorySettings()
{
	fNewSettings.SetDefaults();
	
	UpdateValues();
}


// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::ValueChanged()
{
	// copied from Metrowerks examples...
	
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------


template <class T>
bool
GCCOptionsView<T>::FilterKeyDown(ulong /* aKey */)
{
	return true;
}

// ---------------------------------------------------------------------------

template <class T>
bool
GCCOptionsView<T>::ProjectRequiresUpdate(UpdateType inType)
{
	// return true if inType is what we are dealing with in this view
	return inType == fUpdateType;
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCOptionsView<T>::MessageReceived(BMessage* inMessage)
{
	// The user changed something in the panel - update our settings
	// to reflect that change
	switch (inMessage->what)
	{
		case msgTextChanged:
			this->GetUserTextChanges();
			break;

		case msgCheckBoxChanged:
			this->GetUserCheckBoxesChanges();
			break;

		case msgPopupChanged:
			this->GetUserPopupChanges();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

template <class T>
BCheckBox*
GCCOptionsView<T>::CreateCheckBox(BRect& r, const char* boxLabel, BBox& parent)
{
	BMessage* msg = new BMessage(msgCheckBoxChanged);
	BCheckBox* checkBox = new BCheckBox(r, NULL, boxLabel, msg); 
	parent.AddChild(checkBox);
	checkBox->SetTarget(this);
	PlugInUtil::SetViewGray(checkBox);
	
	return checkBox;
}

#endif
