// ---------------------------------------------------------------------------
/*
	CodeGenerationOptionsView.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			28 October 1998

*/
// ---------------------------------------------------------------------------

#include "CodeGenerationOptionsView.h"
#include "IDEMessages.h"
#include "PlugInUtil.h"

#include <CheckBox.h>
#include <Box.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>

CodeGenerationOptionsView::CodeGenerationOptionsView(BRect inFrame,
													 const char* title,
													 const char* messageName,
													 type_code messageType,
													 UpdateType updateType)
						  : GCCOptionsView<CodeGenerationSettings>(inFrame, title, messageName, messageType, updateType)
{
}

// ---------------------------------------------------------------------------

CodeGenerationOptionsView::~CodeGenerationOptionsView()
{
}

// ---------------------------------------------------------------------------

void
CodeGenerationOptionsView::AttachedToWindow()
{
	// create all the checkboxes for all the CodeGeneration options
	
	BRect bounds = this->Bounds();
	float top = 15.0;
	float left = 10.0;
	float right = bounds.right - 10.0;
		
	// create the border around entire area
	BRect r = this->Bounds();
	BBox* border = new BBox(r, "b1");
	border->SetLabel("x86 Code Generation/Optimization");
	border->SetFont(be_bold_font);
	this->AddChild(border);
	PlugInUtil::SetViewGray(border);

	//
	// Now add in all the options - first the check boxes
	//
	// Notice that this doesn't match the order in the ECodeGenerationOption enumeration
	// That is ok since we don't increment any indices here...
	ECodeGenerationOption whichOption = kPIC;
	r = BRect(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kExplicitTemplates;
	r = BRect(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	whichOption = kKeepInlines;
	r = BRect(left, top, right, top + 15.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *border);
	top += 20.0;

	// create a border for the profiling
	top += 10.0;
	r = BRect(left, top, right, top + 50.0);
	BBox* profileBorder = new BBox(r, "b4");
	profileBorder->SetLabel("Profiling");
	profileBorder->SetFont(be_bold_font);
	border->AddChild(profileBorder);
	PlugInUtil::SetViewGray(profileBorder);
	top += 50.0;
	
	// add the profiling checkbox
	whichOption = kGenerateProfileCode;
	r = BRect(left, 20.0, right - 15.0, 40.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *profileBorder);

	// create another border for the debugging options
	top += 10.0;		
	r = BRect(left, top, right, top + 50.0);
	BBox* debugBorder = new BBox(r, "b2");
	debugBorder->SetLabel("Debugging Information");
	debugBorder->SetFont(be_bold_font);
	border->AddChild(debugBorder);
	PlugInUtil::SetViewGray(debugBorder);
	top += 50.0;

	// add the debugging symbol checkbox to debugBorder
	whichOption = kGenerateDebugSymbols;
	r = BRect(left, 20.0, right - 15.0, 40.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *debugBorder);
	
	// create another border for the optimizations options
	top += 10.0;		
	r = BRect(left, top, right, top + 70.0);
	BBox* optBorder = new BBox(r, "b3");
	optBorder->SetLabel("Optimizations");
	optBorder->SetFont(be_bold_font);
	border->AddChild(optBorder);
	PlugInUtil::SetViewGray(optBorder);

	// now add the popup for optimization level
	r.Set(left, 15.0, right - 10.0, 35.0);
	fOptimizationPopup = new BPopUpMenu("optLevel");

	// iterate through the levels and get their user text
	for (int i = kFirstOptLevel; i <= kLastOptLevel; i++) {
		BMessage* msg = new BMessage(msgPopupChanged);
		fOptimizationPopup->AddItem(new BMenuItem(fNewSettings.GetUserText((ECodeGenerationOption)i), msg));
	}
	fOptimizationPopup->SetTargetForItems(this);
	
	// add a label for the popup
	BMenuField*	popupLabel = new BMenuField(r, NULL, "Optimization Level:", fOptimizationPopup);
	optBorder->AddChild(popupLabel);
	popupLabel->SetDivider(105.0);
	PlugInUtil::SetViewGray(popupLabel);

	// add a space optimization checkbox
	whichOption = kOptSpace;
	r = BRect(left, 40.0, right-15.0, 60.0);
	fCheckBoxList[whichOption] = this->CreateCheckBox(r, fNewSettings.GetUserText(whichOption), *optBorder);
	
	this->DisableExclusiveBoxes();

}

// ---------------------------------------------------------------------------

void
CodeGenerationOptionsView::UpdateValues()
{
	// update all the checkboxes based on our new settings
	// (All checkboxes - so just run right through them)
	
	for (int i = kFirstCheckBoxOption; i <= kLastCheckBoxOption; i++) {
		fCheckBoxList[i]->SetValue(fNewSettings.GetOption((ECodeGenerationOption)i) == true ? 1 : 0);
	}
	
	// Figure out which optimization level is selected
	int itemNumber = 0;
	for (int i = kFirstOptLevel; i <= kLastOptLevel; i++) {
		if (fNewSettings.GetOption((ECodeGenerationOption)i) == true) {
			switch ((ECodeGenerationOption)i) {
				case kOptLevel0:
					itemNumber  = 0;
					break;
				case kOptLevel1:
					itemNumber = 1;
					break;
				case kOptLevel2:
					itemNumber = 2;
					break;
				case kOptLevel3:
					itemNumber = 3;
					break;
				default:
					itemNumber = 4;
					break;
			}
		
		}
	}
	
	// finally set the mark on that level
	BMenuItem* item = fOptimizationPopup->ItemAt(itemNumber);
	if (item && item != fOptimizationPopup->FindMarked()) {
		item->SetMarked(true);
	}

	// make sure the debugging/optimizations are enabled/disabled properly
	this->DisableExclusiveBoxes();	

}

// ---------------------------------------------------------------------------

void
CodeGenerationOptionsView::GetUserTextChanges()
{
	// no user text in the code generation settings
}

// ---------------------------------------------------------------------------

void
CodeGenerationOptionsView::GetUserCheckBoxesChanges()
{
	// iterate all check boxes - getting their current value
	
	for (int i = kFirstCheckBoxOption; i <= kLastCheckBoxOption; i++) {
		fNewSettings.SetOption((ECodeGenerationOption)i, fCheckBoxList[i]->Value() == 1);
	}

	// If they selected debugging info, disable the optimizations
	this->DisableExclusiveBoxes();
	
	// notify CodeWarrior of user modification
	this->ValueChanged();
}

// ---------------------------------------------------------------------------

void
CodeGenerationOptionsView::GetUserPopupChanges()
{
	// figure out which optimization level is set in the popup
	// pick that level, and clear the others
	
	// to make things simple, clear everything to begin
	for (int i = kFirstOptLevel; i <= kLastOptLevel; i++) {
		fNewSettings.SetOption((ECodeGenerationOption)i, false);
	}
	
	// now figure out which one is selected and set that option
	BMenuItem* item = fOptimizationPopup->FindMarked();
	if (item) {
		int menuNumber = fOptimizationPopup->IndexOf(item);
		ECodeGenerationOption whichLevel = kOptLevel0;
		switch (menuNumber) {
			case 0:
				whichLevel = kOptLevel0;
				break;
			case 1:
				whichLevel = kOptLevel1;
				break;
			case 2:
				whichLevel = kOptLevel2;
				break;
			case 3:
				whichLevel = kOptLevel3;
				break;
		}
		fNewSettings.SetOption(whichLevel, true);
	}
	
	// notify CodeWarrior of user modification
	this->ValueChanged();
	
}

// ---------------------------------------------------------------------------

void
CodeGenerationOptionsView::DisableExclusiveBoxes()
{
	// If the debugging symbols option is selected, set (and disable) optimizations
	// If the debugging sysmbols option is cleared, enable optimizations setting

	BCheckBox* debuggingCheckBox = fCheckBoxList[kGenerateDebugSymbols];
			
	if (debuggingCheckBox->Value() == 1 && fOptimizationPopup->IsEnabled() == true) {
		// first, clear all optimizationlevels (+ -Os)
		for (int i = kFirstOptLevel; i <= kLastOptLevel; i++) {
			fNewSettings.SetOption((ECodeGenerationOption)i, false);
		}
		fNewSettings.SetOption(kOptSpace, false);

		// then set -O0
		fNewSettings.SetOption(kOptLevel0, true);
		// set the mark on that level
		BMenuItem* item = fOptimizationPopup->ItemAt(0);
		if (item && item != fOptimizationPopup->FindMarked()) {
			item->SetMarked(true);
		}
		// and disable the popup
		fOptimizationPopup->SetEnabled(false);
		// finally, clear and disable the -Os checkbox
		fCheckBoxList[kOptSpace]->SetEnabled(false);
		fCheckBoxList[kOptSpace]->SetValue(0);

	}
	else if (fNewSettings.GetOption(kGenerateDebugSymbols) == false && fOptimizationPopup->IsEnabled() == false) {
		// enable popup and optimize for space (but leave at current settings)
		
		fOptimizationPopup->SetEnabled(true);
		fCheckBoxList[kOptSpace]->SetEnabled(true);
	}
}

