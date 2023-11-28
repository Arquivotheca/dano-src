// ---------------------------------------------------------------------------
/*
	MEnvironmentPreferencesWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			12 January 1999

*/
// ---------------------------------------------------------------------------

#include "MEnvironmentPreferencesWindow.h"
#include "IDEApp.h"

#include "MEditorView.h"
#include "MFontView.h"
#include "MSyntaxStylePrefsView.h"
#include "MBuildExtras.h"
#include "MKeyBindingView.h"
#include "MEditorColorsPrefsView.h"
#include "IDEMessages.h"

// ---------------------------------------------------------------------------

MEnvironmentPreferencesWindow::MEnvironmentPreferencesWindow()
							 : MPreferencesWindow("Environment Preferences")
{
}

// ---------------------------------------------------------------------------

MEnvironmentPreferencesWindow::~MEnvironmentPreferencesWindow()
{
}
	
// ---------------------------------------------------------------------------

bool
MEnvironmentPreferencesWindow::OKToSave(BMessage& outMessage)
{
	// For environmental preferences, we don't have to check anything
	
	return true;
}

// ---------------------------------------------------------------------------

void
MEnvironmentPreferencesWindow::GetTargetData(BMessage& outMessage)
{
	((IDEApp*)be_app)->GetData(outMessage);
}

// ---------------------------------------------------------------------------

void
MEnvironmentPreferencesWindow::SetTargetData(BMessage& message)
{
	((IDEApp*)be_app)->SetData(message);
}

// ---------------------------------------------------------------------------

bool
MEnvironmentPreferencesWindow::QuitRequested()
{
	// We never really quit preference windows,
	// we just hide them (always return false).
	
	// However, only hide the window once was ask the user
	// about any pending change

	if (this->AttemptCloseView()) {
		MPreferencesView* currentView = this->GetCurrentView();
		if (currentView) {
			currentView->LastCall();
		}
		if (this->IsHidden() == false) {
			this->Hide();
			// don't send a message back if we everything is going down anyway
			if (IDEApp::BeAPP().IsQuitting() == false) {
				be_app_messenger.SendMessage(msgPreferencesWindowHidden);
			}
		}
	}
	
	return false;
}

// ---------------------------------------------------------------------------

void
MEnvironmentPreferencesWindow::BuildViews()
{
	BRect r(fgLeftMargin, fgTopMargin, fgRightMargin, fgBottomMargin);

	// Editor
	this->AddPreferenceCategory("Editor");
	
	MPreferencesView* prefView = new MEditorView(*this, r);
	this->AddView(prefView, nil);

	prefView = new MFontView(*this, r);
	this->AddView(prefView, nil);

	prefView = new MSyntaxStylePrefsView(*this, r);
	this->AddView(prefView, nil);

	// General
	this->AddPreferenceCategory("General");

	prefView = new MBuildExtras(*this, r);
	AddView(prefView, nil);

	prefView = new MKeyBindingView(*this, r);
	AddView(prefView, nil);

	prefView = new MEditorColorsPrefsView(*this, r);
	AddView(prefView, nil);
}
