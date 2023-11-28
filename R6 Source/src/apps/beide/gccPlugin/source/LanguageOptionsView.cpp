// ---------------------------------------------------------------------------
/*
	LanguageOptionsView.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			28 October 1998

*/
// ---------------------------------------------------------------------------

#include "LanguageOptionsView.h"
#include "IDEMessages.h"
#include "PlugInUtil.h"

#include <CheckBox.h>
#include <Box.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>

// ---------------------------------------------------------------------------

LanguageOptionsView::LanguageOptionsView(BRect inFrame,
									     const char* title,
									     const char* messageName,
									     type_code messageType,
									     UpdateType updateType)
					: GCCOptionsView<LanguageSettings>(inFrame, title, messageName, messageType, updateType)
{
}

// ---------------------------------------------------------------------------

LanguageOptionsView::~LanguageOptionsView()
{
}

// ---------------------------------------------------------------------------

void
LanguageOptionsView::AttachedToWindow()
{
	// create all the checkboxes for all the language options
	BRect r = this->Bounds();
	float top = 15.0;
	float left = 10.0;
	float right = r.right - 10.0;
		
	// create the border around entire area
	BBox* border = new BBox(r, "b1");
	border->SetLabel("Language Options");
	border->SetFont(be_bold_font);
	this->AddChild(border);
	PlugInUtil::SetViewGray(border);

	//
	// Now add in all the options
	//

	// first we have a popup
	//		treat source as:
	//				c++/c depending on file extension
	//				c++
	//				c
	
	r.Set(left, top, left + 400.0, top + 18.0);
	fLanguagePopup = new BPopUpMenu("languageKind");

	BMessage* msg = new BMessage(msgPopupChanged);
	fLanguagePopup->AddItem(new BMenuItem("C/C++ (depending on file extension)", msg));

	msg = new BMessage(msgPopupChanged);
	fLanguagePopup->AddItem(new BMenuItem(fNewSettings.GetUserText(kFilesAreCPlusPlus), msg));

	msg = new BMessage(msgPopupChanged);
	fLanguagePopup->AddItem(new BMenuItem(fNewSettings.GetUserText(kFilesAreC), msg));

	fLanguagePopup->SetTargetForItems(this);
		
	// add a label for the popup
	BMenuField*	popupLabel = new BMenuField(r, NULL, "Treat source files as:", fLanguagePopup);
	border->AddChild(popupLabel);
	popupLabel->SetDivider(110.0);
	PlugInUtil::SetViewGray(popupLabel);
	top += 25.0;

	// Now create all the checkboxes
		
	ELanguageOption whichOption = kANSI;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;	
	
	whichOption = kTrigraphs;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kUnsignedChar;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kUnsignedBitfields;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
}

// ---------------------------------------------------------------------------

void
LanguageOptionsView::UpdateValues()
{
	// update popup and the checkboxes based on our new settings

	// Figure out how to treat the files
	int itemNumber = 0;
	if (fNewSettings.GetOption(kFilesAreCPlusPlus) == true) {
		itemNumber = 1;
	}
	else if (fNewSettings.GetOption(kFilesAreC) == true) {
		itemNumber = 2;
	}
	
	// set the mark on that setting
	BMenuItem* item = fLanguagePopup->ItemAt(itemNumber);
	if (item && item != fLanguagePopup->FindMarked()) {
		item->SetMarked(true);
	}

	// now run right through all the checkboxes
	for (int i = kFirstLanguageCheckBox; i < kMaxLanguageOptions; i++) {
		fCheckBoxList[i]->SetValue(fNewSettings.GetOption((ELanguageOption)i) == true ? 1 : 0);
	}
}

// ---------------------------------------------------------------------------

void
LanguageOptionsView::GetUserTextChanges()
{
	// no user text in the language settings
}

// ---------------------------------------------------------------------------

void
LanguageOptionsView::GetUserCheckBoxesChanges()
{
	// Iterate through all the checkboxes and treat them all generically
	// Set them every time thru
	
	for (int i = kFirstLanguageCheckBox; i < kMaxLanguageOptions; i++) {
		fNewSettings.SetOption((ELanguageOption)i, fCheckBoxList[i]->Value() == 1);
	}
	
	// notify CodeWarrior of user modification
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
LanguageOptionsView::GetUserPopupChanges()
{
	// figure out how to treat the source: c++, c, or depending on extension
		
	// to make things simple, clear everything to begin
	fNewSettings.SetOption(kFilesAreC, false);
	fNewSettings.SetOption(kFilesAreCPlusPlus, false);
	
	// now figure out which one is selected and set that option
	BMenuItem* item = fLanguagePopup->FindMarked();
	if (item) {
		int menuNumber = fLanguagePopup->IndexOf(item);
		switch (menuNumber) {
			case 0:
				// depending on extension - don't set any flag;
				break;
			case 1:
				fNewSettings.SetOption(kFilesAreCPlusPlus, true);
				break;
			case 2:
				fNewSettings.SetOption(kFilesAreC, true);
				break;
		}
	}
	
	// notify BeIDE of user modification
	this->ValueChanged();
}
