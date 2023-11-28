// ---------------------------------------------------------------------------
/*
	MInformationMessageWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			10 June 1999

*/
// ---------------------------------------------------------------------------

#include "MInformationMessageWindow.h"
#include "ProjectCommands.h"
#include "IDEApp.h"
#include "IDEMessages.h"
#include "MMessageInfoView.h"
#include "MMessageItem.h"
#include "Utils.h"
#include "MDynamicMenuHandler.h"

#include <String.h>
#include <CheckBox.h>
#include <StringView.h>

// ---------------------------------------------------------------------------
// MInformationMessageWindow member functions
// ---------------------------------------------------------------------------

MInformationMessageWindow::MInformationMessageWindow(const char* title, const char* infoTitle)
						  : MMessageWindow(title, infoTitle)
{
	fInfoCount = 0;
	MDynamicMenuHandler::MessageWindowOpened(this);
}

// ---------------------------------------------------------------------------

MInformationMessageWindow::~MInformationMessageWindow()
{
	MDynamicMenuHandler::MessageWindowClosed(this);
}

// ---------------------------------------------------------------------------

void
MInformationMessageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// Batch find
		case msgAddInfoToMessageWindow:
		case msgAddDefinitionToMessageWindow:
			fMessageView->AddNewInfo(*message, kInfos);
			this->UpdateCounters();
			break;

		// Documentation Lookup
		case msgAddDocInfoToMessageWindow:
			fMessageView->AddNewDocInfo(*message, kInfos);
			this->UpdateCounters();
			break;

		default:
			MMessageWindow::MessageReceived(message);
	}
}

// ---------------------------------------------------------------------------

MMessageInfoView*
MInformationMessageWindow::CreateInfoView(BRect& bounds, const char* infoTitle)
{	
	fInfoView = new MMessageInfoView(infoTitle, bounds, BPoint(90, 4), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	return fInfoView;
}

// ---------------------------------------------------------------------------

void
MInformationMessageWindow::BuildInfoView()
{
	float left = 10.0;
	const float top = 4.0;

	// Get the bitmaps for the picture buttons
	const BBitmap* infoBitMap;
	MMessageItem::GetInfoBitmap(infoBitMap);
	
	// Info
	MMessageWindow::BuildControlGroup(BPoint(left, top), fInfoView, 
									  infoBitMap, 
									  nil,					// no check box 
									  &fInformationPicture, 
									  &fInformationCaption);
}

// ---------------------------------------------------------------------------

void
MInformationMessageWindow::ClearMessages(BMessage& /*inMessage*/)
{
	fMessageView->ClearMessages();
	this->UpdateCounters();
}

// ---------------------------------------------------------------------------

void
MInformationMessageWindow::UpdateCounters()
{
	// Update the stringviews that hold the numbers of information listings
	
	String text;
	int32 counter = fMessageView->Infos();
	if (counter != fInfoCount) {
		text = counter;
		fInformationCaption->SetText(text);
		fInfoCount = counter;
	}
}

// ---------------------------------------------------------------------------

MProjectWindow*
MInformationMessageWindow::GetAssociatedProject()
{
	// It would really be nice to have a project associated with each message window
	// But they can last longer than any project.  Getting the current one
	// IDEApp::GetCurrentProject() would work, but perhaps be confusing.

	return nil;	
}
