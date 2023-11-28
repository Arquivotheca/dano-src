// ---------------------------------------------------------------------------
/*
	LinkerOptionsView.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			28 October 1998

*/
// ---------------------------------------------------------------------------

#include "LinkerOptionsView.h"
#include "IDEMessages.h"
#include "PlugInUtil.h"

#include <CheckBox.h>
#include <Box.h>

LinkerOptionsView::LinkerOptionsView(BRect inFrame,
									 const char* title,
									 const char* messageName,
									 type_code messageType,
									 UpdateType updateType)
				  : GCCOptionsView<LinkerSettings>(inFrame, title, messageName, messageType, updateType)
{
}

// ---------------------------------------------------------------------------

LinkerOptionsView::~LinkerOptionsView()
{
}

// ---------------------------------------------------------------------------

void
LinkerOptionsView::AttachedToWindow()
{
	// create all the checkboxes for all the Linker options
	BRect r = this->Bounds();	
	float top = 15.0;
	float left = 10.0;
	float right = r.right - 10.0;
		
	// create the border around entire area
	BBox* border = new BBox(r, "b1");
	border->SetLabel("Link Options");
	border->SetFont(be_bold_font);
	this->AddChild(border);
	PlugInUtil::SetViewGray(border);

	//
	// Now add in all the options
	//
	
	ELinkerOption whichOption = kStripSymbols;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kStripLocals;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
}

// ---------------------------------------------------------------------------

void
LinkerOptionsView::UpdateValues()
{
	// update all the checkboxes based on our new settings
	// (All checkboxes - so just run right through them)
	
	for (int i = 0; i < kMaxLinkerOptions; i++) {
		fCheckBoxList[i]->SetValue(fNewSettings.GetOption((ELinkerOption)i) == true ? 1 : 0);
	}
}

// ---------------------------------------------------------------------------

void
LinkerOptionsView::GetUserTextChanges()
{
	// no user text in the Linker settings
}

// ---------------------------------------------------------------------------

void
LinkerOptionsView::GetUserCheckBoxesChanges()
{
	// Since all the options are check boxes, we can iterate through and
	// treat them all generically.
	
	for (int i = 0; i < kMaxLinkerOptions; i++) {
		fNewSettings.SetOption((ELinkerOption)i, fCheckBoxList[i]->Value() == 1);
	}
	
	// notify CodeWarrior of user modification
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
LinkerOptionsView::GetUserPopupChanges()
{
	// no popups in the Linker settings
}
