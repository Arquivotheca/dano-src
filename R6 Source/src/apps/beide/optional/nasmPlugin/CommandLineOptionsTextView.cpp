// ---------------------------------------------------------------------------
/*
	CommandLineOptionsTextView.cpp
	
	Provides an implementation of Metrowerks plug-in preference view that
	just implements a text box for typing.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#include "CommandLineOptionsTextView.h"
#include "PlugIn.h"
#include "MTextView.h"
#include "PlugInUtil.h"

#include <string.h>

#include <Window.h>
#include <Box.h>
#include <View.h>
#include <StringView.h>
#include <ScrollView.h>
#include <String.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// CommandLineOptionsTextView method implementations
// ---------------------------------------------------------------------------

CommandLineOptionsTextView::CommandLineOptionsTextView(BRect inFrame, 
										 const char* title,
										 const char* caption,
										 const char* messageName,
										 type_code messageType,
										 UpdateType updateType,
										 const char* defaultText)
					: GCCOptionsView<CommandLineText>(inFrame, title, messageName, messageType, updateType)

{
	fCaption = new char[strlen(caption) + 1];
	strcpy(fCaption, caption);
	
	fDefaultText = new char[strlen(defaultText) + 1];
	strcpy(fDefaultText, defaultText);
	
	CommandLineOptionsTextView::DoFactorySettings();
}

// ---------------------------------------------------------------------------

CommandLineOptionsTextView::~CommandLineOptionsTextView()
{
	delete [] fCaption;
	delete [] fDefaultText;
}

// ---------------------------------------------------------------------------

void
CommandLineOptionsTextView::DoFactorySettings()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CommandLineOptionsTextView::DoFactorySettings\n");
#endif
	memset(fNewSettings.fText, 0, CommandLineText::kTextLength);
	strcpy(fNewSettings.fText, fDefaultText);
	fNewSettings.fVersion = CommandLineText::kCurrentVersion;
}

// ---------------------------------------------------------------------------
	
void
CommandLineOptionsTextView::UpdateValues()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CommandLineOptionsTextView::UpdateValues\n");
#endif
		
	// set the user visible text box to contain our current text
	fTextBox->SetText(fNewSettings.fText);
	fTextBox->Invalidate();
	
}

// ---------------------------------------------------------------------------

void
CommandLineOptionsTextView::AttachedToWindow()
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
				
	// tell the user what to enter into the text box
	// inset the text from the border
	currentBounds.Set(left, top, right, top + 16.0);
	BStringView* staticText = new BStringView(currentBounds, "t3", "Enter NASM options:");
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
CommandLineOptionsTextView::GetUserTextChanges()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CommandLineOptionsTextView::GetUserModifiedText\n");
#endif

	// the user just typed some command line options
	// we have been notified of the change
	// get the text from our text box and set them in our settings
	
	memset(fNewSettings.fText, 0, CommandLineText::kTextLength);
	strcpy(fNewSettings.fText, fTextBox->Text());
	
	// then notify CodeWarrior of the change
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
CommandLineOptionsTextView::GetUserCheckBoxesChanges()
{
	// none
}

// ---------------------------------------------------------------------------

void
CommandLineOptionsTextView::GetUserPopupChanges()
{
	// none
}
