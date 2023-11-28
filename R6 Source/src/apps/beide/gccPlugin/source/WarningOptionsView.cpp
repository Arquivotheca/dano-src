// ---------------------------------------------------------------------------
/*
	WarningOptionsView.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			28 October 1998

*/
// ---------------------------------------------------------------------------

#include "WarningOptionsView.h"
#include "IDEMessages.h"
#include "PlugInUtil.h"

#include <CheckBox.h>
#include <Box.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>

// ---------------------------------------------------------------------------
// WarningOptionsView member functions
// ---------------------------------------------------------------------------

WarningOptionsView::WarningOptionsView(BRect inFrame,
									   const char* title,
									   const char* messageName,
									   type_code messageType,
									   UpdateType updateType)
				   : GCCOptionsView<WarningSettings>(inFrame, title, messageName, messageType, updateType)
{
}

// ---------------------------------------------------------------------------

WarningOptionsView::~WarningOptionsView()
{
}

// ---------------------------------------------------------------------------

void
WarningOptionsView::AttachedToWindow()
{
	// create all the checkboxes for all the Warning options
	BRect r = this->Bounds();
	float top = 15.0;
	float left = 10.0;
	float right = r.right - 10.0;
		
	// create the border around entire area
	BBox* border = new BBox(r, "b1");
	border->SetLabel("Warning Options");
	border->SetFont(be_bold_font);
	this->AddChild(border);
	PlugInUtil::SetViewGray(border);

	//
	// Now add in all the options
	//	

	// first we have a popup
	//		Warnings are:
	//				enabled (no option passed to compiler)
	//				disabled
	//				treated as errors
	
	r.Set(left, top, right, top + 18.0);
	fWarningPopup = new BPopUpMenu("warninghandling");

	BMessage* msg = new BMessage(msgPopupChanged);
	fWarningPopup->AddItem(new BMenuItem("enabled", msg));

	EWarningOption whichOption = kInhibitWarnings;
	msg = new BMessage(msgPopupChanged);
	fWarningPopup->AddItem(new BMenuItem(fNewSettings.GetUserText(whichOption), msg));

	whichOption = kWarningsAreErrors;
	msg = new BMessage(msgPopupChanged);
	fWarningPopup->AddItem(new BMenuItem(fNewSettings.GetUserText(whichOption), msg));

	fWarningPopup->SetTargetForItems(this);
		
	// add a label for the popup
	BMenuField*	popupLabel = new BMenuField(r, NULL, "Warnings are:", fWarningPopup);
	border->AddChild(popupLabel);
	popupLabel->SetDivider(80.0);
	PlugInUtil::SetViewGray(popupLabel);
	top += 25.0;

	//
	// now handle all the check boxes	
	//
	
	whichOption = kWarnPedantic;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnShadow;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;	
	
	whichOption = kWarnFunctionCast;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnCastQual;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnConverion;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnInline;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnExternInline;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnWriteStrings;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnOverload;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnOldStyleCast;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnEffectiveCPlus;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
}

// ---------------------------------------------------------------------------

void
WarningOptionsView::UpdateValues()
{
	// update the popup and all the checkboxes based on our new settings
	// first the popup, then the checkboxes

	// Figure out how to treat errors
	int itemNumber = 0;
	if (fNewSettings.GetOption(kInhibitWarnings) == true) {
		itemNumber = 1;
	}
	else if (fNewSettings.GetOption(kWarningsAreErrors) == true) {
		itemNumber = 2;
	}
	
	// set the mark on that setting
	BMenuItem* item = fWarningPopup->ItemAt(itemNumber);
	if (item && item != fWarningPopup->FindMarked()) {
		item->SetMarked(true);
	}

	// now run through all the checkboxes		
	for (int i = kFirstWarningCheckBox; i < kMaxWarningOptions; i++) {
		fCheckBoxList[i]->SetValue(fNewSettings.GetOption((EWarningOption)i) == true ? 1 : 0);
	}
}

// ---------------------------------------------------------------------------

void
WarningOptionsView::GetUserTextChanges()
{
	// no user text in the warning settings
}

// ---------------------------------------------------------------------------

void
WarningOptionsView::GetUserCheckBoxesChanges()
{
	// Since all the options are check boxes, we can iterate through and
	// treat them all generically.
	
	for (int i = kFirstWarningCheckBox; i < kMaxWarningOptions; i++) {
		fNewSettings.SetOption((EWarningOption)i, fCheckBoxList[i]->Value() == 1);
	}

	// notify CodeWarrior of user modification
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
WarningOptionsView::GetUserPopupChanges()
{
	// figure out how to treat warnings: normal, disabled, errors
		
	// to make things simple, clear everything to begin
	fNewSettings.SetOption(kInhibitWarnings, false);
	fNewSettings.SetOption(kWarningsAreErrors, false);
	
	// now figure out which one is selected and set that option
	BMenuItem* item = fWarningPopup->FindMarked();
	if (item) {
		int menuNumber = fWarningPopup->IndexOf(item);
		switch (menuNumber) {
			case 0:
				// warnings are treated as normal - don't set any flag;
				break;
			case 1:
				fNewSettings.SetOption(kInhibitWarnings, true);
				break;
			case 2:
				fNewSettings.SetOption(kWarningsAreErrors, true);
				break;
		}
	}
	
	// notify BeIDE of user modification
	this->ValueChanged();
}

// ---------------------------------------------------------------------------
// CommonWarningOptionsView member functions
// ---------------------------------------------------------------------------

CommonWarningOptionsView::CommonWarningOptionsView(BRect inFrame,
												   const char* title,
												   const char* messageName,
												   type_code messageType,
												   UpdateType updateType)
						 : GCCOptionsView<CommonWarningSettings>(inFrame, title, messageName, messageType, updateType)
{
}

// ---------------------------------------------------------------------------

CommonWarningOptionsView::~CommonWarningOptionsView()
{
}

// ---------------------------------------------------------------------------

void
CommonWarningOptionsView::AttachedToWindow()
{
	// create all the checkboxes for all the Warning options
	BRect r = this->Bounds();
	float top = 15.0;
	float left = 10.0;
	float right = r.right - 10.0;
		
	// create the border around entire area
	BBox* border = new BBox(r, "b1");
	border->SetLabel("Common Warning Options");
	border->SetFont(be_bold_font);
	this->AddChild(border);
	PlugInUtil::SetViewGray(border);

	//
	// Now add in all the options
	//	
	ECommonWarningOption whichOption = kWarnAll;
	r = BRect(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	// indent under the -Wall box
	left = left + 15.0;
	whichOption = kWarnParentheses;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnReturnType;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnSwitch;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;	
	
	whichOption = kWarnUnused;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnUninitialized;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnReorder;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnNonVirtualDtor;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;
	
	whichOption = kWarnUnknownPragma;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnSignCompare;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnCharSubscript;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnFormat;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kWarnTrigraphs;
	r.Set(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	this->DisableExclusiveBoxes();
}

// ---------------------------------------------------------------------------

void
CommonWarningOptionsView::UpdateValues()
{
	// update all the checkboxes based on our new settings
	// (All checkboxes - so just run right through them)
	
	for (int i = 0; i < kMaxCommonWarningOptions; i++) {
		fCheckBoxList[i]->SetValue(fNewSettings.GetOption((ECommonWarningOption)i) == true ? 1 : 0);
	}

	// make sure the CommonWarning selections are enabled/disabled properly
	this->DisableExclusiveBoxes();	
}

// ---------------------------------------------------------------------------

void
CommonWarningOptionsView::GetUserTextChanges()
{
	// no user text in the CommonWarning settings
}

// ---------------------------------------------------------------------------

void
CommonWarningOptionsView::GetUserCheckBoxesChanges()
{
	// Since all the options are check boxes, we can iterate through and
	// treat them all generically.
	
	for (int i = 0; i < kMaxCommonWarningOptions; i++) {
		fNewSettings.SetOption((ECommonWarningOption)i, fCheckBoxList[i]->Value() == 1);
	}
	
	// If they selected a CommonWarning, disable the other
	this->DisableExclusiveBoxes();

	// notify CodeWarrior of user modification
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
CommonWarningOptionsView::GetUserPopupChanges()
{
	// no popups in the CommonWarning settings
}

// ---------------------------------------------------------------------------

void
CommonWarningOptionsView::DisableExclusiveBoxes()
{
	// If the -Wall option is set, disable all the others...
	// If the -Wall option is cleared - enable all others...
	// (use the first sub-warning as a trigger for this case)
		
	BCheckBox* warnAllCheckBox = fCheckBoxList[kWarnAll];
	BCheckBox* firstSubWarning = fCheckBoxList[kFirstSubWarning];
			
	if (warnAllCheckBox->Value() == 1 && firstSubWarning->IsEnabled() == true) {
		// disable all warning checkboxes
		for (int i = kFirstSubWarning; i < kMaxCommonWarningOptions; i++) {
			fCheckBoxList[i]->SetEnabled(false);
		}
	}
	else if (fNewSettings.GetOption(kWarnAll) == false && firstSubWarning->IsEnabled() == false) {
		// enable all warning checkboxes
		for (int i = kFirstSubWarning; i < kMaxCommonWarningOptions; i++) {
			fCheckBoxList[i]->SetEnabled(true);
		}
	}
}
