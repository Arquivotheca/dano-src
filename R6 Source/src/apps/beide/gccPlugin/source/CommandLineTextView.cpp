// ---------------------------------------------------------------------------
/*
	CommandLineTextView.cpp
	
	Provides an implementation of Metrowerks plug-in preference view that
	just implements a text box for typing.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#include "CommandLineTextView.h"
#include "PlugIn.h"
#include "MTextView.h"
#include "PlugInUtil.h"

#include <string.h>

#include <Window.h>
#include <Box.h>
#include <View.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// CommandLineTextView method implementations
// ---------------------------------------------------------------------------

CommandLineTextView::CommandLineTextView(BRect inFrame, 
										 const char* title,
										 const char* caption,
										 const char* messageName,
										 type_code messageType,
										 UpdateType updateType,
										 const char* defaultText,
										 GCCBuildHandler* builder)
					: GCCOptionsView<CommandLineText>(inFrame, title, messageName, messageType, updateType)

{
	fCaption = new char[strlen(caption) + 1];
	strcpy(fCaption, caption);
	
	fDefaultText = new char[strlen(defaultText) + 1];
	strcpy(fDefaultText, defaultText);

	fBuilderHandler = builder;
	
	CommandLineTextView::DoFactorySettings();
}

// ---------------------------------------------------------------------------

CommandLineTextView::~CommandLineTextView()
{
	delete [] fCaption;
	delete [] fDefaultText;
	delete fBuilderHandler;
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::DoFactorySettings()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CommandLineTextView::DoFactorySettings\n");
#endif
	memset(fNewSettings.fText, 0, CommandLineText::kTextLength);
	strcpy(fNewSettings.fText, fDefaultText);
	fNewSettings.fVersion = CommandLineText::kCurrentVersion;
}

// ---------------------------------------------------------------------------
	
void
CommandLineTextView::UpdateValues()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CommandLineTextView::UpdateValues\n");
#endif

	// make sure the option string text is updated
	// get and cache the GUI-based option text
	// If we don't have a project, we can't give the feedback...
	fGUIOptionText.Truncate(0);
	if (fProject) {
		fBuilderHandler->GetOptionText(fProject, fGUIOptionText);
	}
	this->SetOptionFeedback(fNewSettings.fText);
		
	// set the user visible text box to contain our current text
	fTextBox->SetText(fNewSettings.fText);
	fTextBox->Invalidate();
	
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::AttachedToWindow()
{
	BRect bounds = this->Bounds();
	
	// create the border around entire area
	BBox* border = new BBox(bounds, "b1");
	border->SetLabel(fCaption);
	border->SetFont(be_bold_font);
	this->AddChild(border);
	// for some reason, the gray needs to be set after adding
	PlugInUtil::SetViewGray(border);

	// tell the user what the current options are from the preference settings
	BRect currentBounds;
	float top = 15.0;
	float left = 10.0;
	float right = bounds.right - 10.0;
	float optionViewSize = 20.0 * 3;
	currentBounds.Set(left, top, bounds.right - 10.0, 32.0);
	BStringView* optionTitle = new BStringView(currentBounds, "t1", "Current options:");
	PlugInUtil::SetViewGray(optionTitle);
	border->AddChild(optionTitle);
	top += 20;
		
	// set up a non-editable area to display the GUI specified options
	currentBounds.Set(left, top, right, top + optionViewSize);
	BRect textRect = currentBounds;
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(2.0, 2.0);
	fGUIOptionsBox = new BTextView(currentBounds, "t2", textRect, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	fGUIOptionsBox->MakeEditable(false);
	PlugInUtil::SetViewGray(fGUIOptionsBox);
	// put a scroll view around the text box so we get a frame
	BScrollView* guiSettingsFrame = new BScrollView("f1", fGUIOptionsBox);
	fGUIOptionsBox->MakeEditable(false);
	border->AddChild(guiSettingsFrame);
	top += optionViewSize + 10.0;
		
	// tell the user what to enter into the text box
	// inset the text from the border
	currentBounds.Set(left, top, right, top + 16.0);
	BStringView* staticText = new BStringView(currentBounds, "t3", "Enter additional desired gcc options:");
	PlugInUtil::SetViewGray(staticText);
	border->AddChild(staticText);
	top += 20.0;
	
	// now offset in some more, and create an edit box for user interaction
	// use Metrowerks' MTextView so that we get notification when the text changes
	currentBounds.Set(left, top, right, top + optionViewSize);
	fTextBox = new MTextView(currentBounds, "tv1");
	fTextBox->SetTarget(this);
	fTextBox->SetMaxBytes(CommandLineText::kTextLength-1);
	fTextBox->SetWordWrap(true);

	// put a scroll view around the text box so we get a frame
	BScrollView* textBoxFrame = new BScrollView("f2", fTextBox);
	border->AddChild(textBoxFrame);
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::SetData(BMessage& inOutMessage)
{
	// Before doing the normal SetData...	
	// Get the project so that our builder handler
	// knows how to ask for the standard command line options (based on project)
	
	MProject* project;
	if (inOutMessage.FindPointer("MProject", (void **) &project) == B_OK) {
		fProject = project;
	}
	else {
		fProject = nil;
	}
		
	// Now that we have the project set up, do the normal SetData
	GCCOptionsView<CommandLineText>::SetData(inOutMessage);
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::GetUserTextChanges()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CommandLineTextView::GetUserModifiedText\n");
#endif

	// the user just typed some command line options
	// we have been notified of the change
	// get the text from our text box and set them in our settings
	
	memset(fNewSettings.fText, 0, CommandLineText::kTextLength);
	strcpy(fNewSettings.fText, fTextBox->Text());

	this->SetOptionFeedback(fNewSettings.fText);
	
	// then notify CodeWarrior of the change
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::GetUserCheckBoxesChanges()
{
	// none
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::GetUserPopupChanges()
{
	// none
}

// ---------------------------------------------------------------------------

void
CommandLineTextView::SetOptionFeedback(const char* commandLineText)
{
	// The complete option feedback consists of the GUI based options
	// plus the current commandline text
	
	BString fullText = fGUIOptionText;
	fullText.Append(commandLineText);
	fGUIOptionsBox->SetText(fullText.String());
	fGUIOptionsBox->Invalidate();
}
