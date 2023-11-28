// ---------------------------------------------------------------------------
/*
	MErrorMessageWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			10 June 1999

*/
// ---------------------------------------------------------------------------

#include "MErrorMessageWindow.h"
#include "ProjectCommands.h"
#include "IDEMessages.h"
#include "MMessageInfoView.h"
#include "MMessageItem.h"
#include "Utils.h"
#include "MProjectWindow.h"

#include <Point.h>
#include <String.h>
#include <CheckBox.h>
#include <StringView.h>

// ---------------------------------------------------------------------------
// MErrorMessageWindow member functions
// ---------------------------------------------------------------------------

MErrorMessageWindow::MErrorMessageWindow(MProjectWindow* project)
					: MMessageWindow("Errors & Warnings", project->Title())
{
	fErrors = 0;
	fWarnings = 0;
	fVisibleTypes = kErrorsAndWarnings;
	fProject = project;
}

// ---------------------------------------------------------------------------

MErrorMessageWindow::~MErrorMessageWindow()
{
}

// ---------------------------------------------------------------------------

bool
MErrorMessageWindow::QuitRequested()
{
	// We never really quit Errors&Warning windows,
	// we just hide them (always return false).
	
	if (this->IsHidden() == false) {
		this->Hide();
	}
	
	return false;
}

// ---------------------------------------------------------------------------

void
MErrorMessageWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// File menu
		case cmd_Close:
			Hide();
			break;

		case msgCheckBoxChanged:
		case msgPictureButtonChanged:
			this->UpdateCheckBoxes(*message);
			break;

		// Compile objects
		case msgAddToMessageWindow:
			fMessageView->AddNewMessage(*message, fVisibleTypes);
			this->UpdateCounters();
			break;

		// non-ideAware Compile objects
		case msgAddErrorsToMessageWindow:
			fMessageView->AddErrors(*message, fVisibleTypes);
			this->UpdateCounters();
			break;

		case cmd_ErrorMessageWindow:
			// bring myself up in the current workspace,
			// (don't take the user to the workspace that happens to
			// currently hold me)
			this->SetWorkspaces(B_CURRENT_WORKSPACE);
			this->ShowAndActivate();
			break;

		// send make/cancel to the owning project
		case cmd_Make:
		case cmd_Cancel:
			fProject->PostMessage(message);
			break;

		default:
			MMessageWindow::MessageReceived(message);
	}
}

// ---------------------------------------------------------------------------

MMessageInfoView*
MErrorMessageWindow::CreateInfoView(BRect& bounds, const char* infoTitle)
{
	BString title = "Errors and Warnings for ";
	title += infoTitle;
	
	fInfoView = new MMessageInfoView(title.String(), bounds, BPoint(190, 4), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	return fInfoView;
}

// ---------------------------------------------------------------------------

void
MErrorMessageWindow::BuildInfoView()
{
	float left = 10.0;
	const float top = 4.0;

	// Get the bitmaps for the picture buttons
	const BBitmap* errorBitMap;
	const BBitmap* warningsBitMap;
	MMessageItem::GetErrorBitmap(errorBitMap);
	MMessageItem::GetWarningBitmap(warningsBitMap);
	
	// Errors
	MMessageWindow::BuildControlGroup(BPoint(left, top), fInfoView, 
									  errorBitMap, 
									  &fErrorsCheckBox, 
									  &fErrorsPicture, 
									  &fErrorsCaption);
	
	// Warnings
	MMessageWindow::BuildControlGroup(BPoint(left+90.0, top), fInfoView, 
									  warningsBitMap, 
									  &fWarningsCheckBox, 
									  &fWarningsPicture, 
									  &fWarningsCaption);

	// Start with Errors/Warnings both on
	fErrorsCheckBox->SetValue(B_CONTROL_ON);
	fWarningsCheckBox->SetValue(B_CONTROL_ON);
}

// ---------------------------------------------------------------------------

void
MErrorMessageWindow::ClearMessages(BMessage& /*inMessage*/)
{
	fMessageView->ClearMessages();
	this->UpdateCounters();
}

// ---------------------------------------------------------------------------

void
MErrorMessageWindow::UpdateCounters()
{
	// Update the stringviews that hold the numbers of errors and warnings
	// when messages are added or removed from the window.
	// Notice that if the first time we get an error/warning or info
	// we make sure the user can see them. 
	
	String text;
	int32 counter = fMessageView->Errors();
	
	if (counter != fErrors) {
		text = counter;
		fErrorsCaption->SetText(text);
		// sorry, we are going to be a little authoritarian about errors
		// if we have errors but the user has unchecked errors, we are
		// going to turn errors back on and show them (and hide info messages)
		if (fErrors == 0 && counter > 0 && fErrorsCheckBox->Value() == B_CONTROL_OFF) {
			fErrorsCheckBox->SetValue(B_CONTROL_ON);
			this->UpdateMessagesInView();
		}
		fErrors = counter;
	}
	
	counter = fMessageView->Warnings();
	if (counter != fWarnings) {
		text = counter;
		fWarningsCaption->SetText(text);
		// (see message about errors - same for warnings)
		if (fWarnings == 0 && counter > 0 && fWarningsCheckBox->Value() == B_CONTROL_OFF) {
			fWarningsCheckBox->SetValue(B_CONTROL_ON);
			this->UpdateMessagesInView();
		}
		fWarnings = counter;
	}
}

// ---------------------------------------------------------------------------

void
MErrorMessageWindow::UpdateMessagesInView()
{
	// Update which messages are visible or hidden in the message view after
	// a checkbox or picture button is changed.

	bool showErrors = fErrorsCheckBox->Value() == B_CONTROL_ON;
	bool showWarnings = fWarningsCheckBox->Value() == B_CONTROL_ON;
		
	if (showErrors && showWarnings) {
		fVisibleTypes = kErrorsAndWarnings;
	}
	else if (showErrors) {
		fVisibleTypes = kErrors;
	}
	else if (showWarnings) {
		fVisibleTypes = kWarnings;
	}
	else {
		fVisibleTypes = kNoMessages;
	}
	fMessageView->Update(fVisibleTypes);
}

// ---------------------------------------------------------------------------

void
MErrorMessageWindow::UpdateCheckBoxes(BMessage& inMessage)
{
	if (inMessage.HasPointer("source")) {
		switch (inMessage.what) {
			case msgCheckBoxChanged:
				BCheckBox* checkBox;
				if (inMessage.FindPointer("source", (void**) &checkBox) == B_OK) {
					this->UpdateMessagesInView();
				}
				break;

			case msgPictureButtonChanged:
				BPictureButton*	pictureButton;
				if (inMessage.FindPointer("source",  (void**) &pictureButton) == B_OK) {
					BCheckBox* checkBox = (pictureButton == fErrorsPicture) ? fErrorsCheckBox
																			: fWarningsCheckBox;
					checkBox->SetValue(1 - checkBox->Value());	// toggle the value
					this->UpdateMessagesInView();
				}
				break;
		}
	}
}

// ---------------------------------------------------------------------------

MProjectWindow*
MErrorMessageWindow::GetAssociatedProject()
{
	return fProject;
}
