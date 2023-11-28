//========================================================================
//	MEditorView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MEditorView.h"
#include "MDefaultPrefs.h"
#include "MTextControl.h"
#include "MTextView.h"
#include "IDEMessages.h"
#include "Utils.h"
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Box.h>
#include <CheckBox.h>
#include <Roster.h>
#include <Mime.h>
#include <Debug.h>

const char * titleEd = "Editor";

// ---------------------------------------------------------------------------
//		MEditorView
// ---------------------------------------------------------------------------
//	Constructor

MEditorView::MEditorView(
	MPreferencesWindow&	inWindow,
	BRect				inFrame)
	: MPreferencesView(inWindow, inFrame, "editorview")
{
	SetPointers(&fOldSettings, &fNewSettings, sizeof(fNewSettings));
}

// ---------------------------------------------------------------------------
//		~MEditorView
// ---------------------------------------------------------------------------
//	Destructor

MEditorView::~MEditorView()
{
	MEditorView::EmptyAppList();
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MEditorView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgRadioButtonClicked:
		case msgTextChanged:
		case msgCheckBoxChanged:
		case msgPopupChanged:
			ExtractInfo();
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		ExtractInfo
// ---------------------------------------------------------------------------

void
MEditorView::ExtractInfo()
{
	// External editor settings
	fNewSettings.pUseExternalEditor = fUseExternalEditorSettings->Value() == 1;

	// Clear out the current editor signature, just to make it easy
	fNewSettings.pEditorSignature[0] = 0;
	
	// If the user wants to use an external editor, figure out which one
	// and copy over the signature...
	if (fNewSettings.pUseExternalEditor) {
		BMenuItem* item = fEditorAppMenu->FindMarked();
		if (item) {
			int menuNumber = fEditorAppMenu->IndexOf(item);
			if (menuNumber >= kFirstEditorIndex) {
				strcpy(fNewSettings.pEditorSignature, 
					   (const char*)fEditorSignatureList.ItemAtFast(menuNumber));
			}
		}
	}

	// Radio buttons
	fNewSettings.pUseDocFont = fUseDocSettings->Value() == 1;
	fNewSettings.pUseAppFont = fUseAppSettings->Value() == 1;

	this->DisableExclusiveSettings();

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MEditorView::GetData(
	BMessage&	inOutMessage,
	bool		/*isProxy*/)
{
	inOutMessage.AddData(kEditorPrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings), false);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MEditorView::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;
	EditorPrefs*	inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kEditorPrefs, kMWPrefs, (const void**) &inPrefs, &length))
	{
		ASSERT(length == sizeof(EditorPrefs));

		fNewSettings = *inPrefs;
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MEditorView::UpdateValues()
{
	// External editor settings
	fUseExternalEditorSettings->SetValue(fNewSettings.pUseExternalEditor == true ? 1 : 0);

	// If we are to use an external editor, figure out which one to
	// select in our menu.  Iterate our signature list and find a match
	// We need to be prepared to not find a match because our saved signature
	// could have gone away since we last set this value
	// Then select the corresponding menu item
	// (default to 0 which is "Preferrred Application");
	
	int32 itemNumber = 0;
	int32 signatureCount = fEditorSignatureList.CountItems();
	if (strlen(fNewSettings.pEditorSignature) > 0) {
		for (int32 i = kFirstEditorIndex; i < signatureCount; i++) {
			if (strcmp(fNewSettings.pEditorSignature, 
					   (const char*)fEditorSignatureList.ItemAtFast(i)) == 0) {
				itemNumber = i;
				break;
			}
		}
	}

	// finally set the mark on the correct application
	BMenuItem* item = fEditorAppMenu->ItemAt(itemNumber);
	if (item && item != fEditorAppMenu->FindMarked()) {
		item->SetMarked(true);
	}

	// RadioButtons
	// (pUseProjectFont is no longer supported, but some settings
	// might still contain it so we need to check for it.)
	if (fNewSettings.pUseDocFont) {
		fUseDocSettings->SetValue(1);
	}
	else if (fNewSettings.pUseProjectFont || fNewSettings.pUseAppFont) {
		fUseAppSettings->SetValue(1);
	}

	this->DisableExclusiveSettings();
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------

void
MEditorView::DoFactorySettings()
{
	MDefaultPrefs::SetEditorDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MEditorView::Title()
{
	return titleEd;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MEditorView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MEditorView::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BMessage*		msg;
	BRadioButton*	radioButton;
	BStringView*	caption;

	// Box
	r = Bounds();
	box = new BBox(r, "EditInfo");
	box->SetLabel("Editor Options");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Static text
	// title
	float		top = 15.0;
	const float left = 10;
	r.Set(left, top, left + 250, top + 16);
	caption = new BStringView(r, "st1", "When opening a text document:"); 
	box->AddChild(caption);
	SetGrey(caption, kLtGray);
	top += 20;

	// Check box
	// use external editor
	r = BRect(20, top, 250, top + 16.0);
	fUseExternalEditorSettings = new BCheckBox(r, NULL, "Use an external editor", new BMessage(msgCheckBoxChanged)); 
	box->AddChild(fUseExternalEditorSettings);
	fUseExternalEditorSettings->SetFont(be_bold_font);
	fUseExternalEditorSettings->SetTarget(this);
	SetGrey(fUseExternalEditorSettings, kLtGray);
	top += 20.0;

	// Popup
	// Pick external editor
	// add an empty label for the popup
	fEditorAppMenu = new BPopUpMenu("externalEditor");
	this->BuildEditorAppMenu();
	r.Set(40, top, 250, top + 16);
	BMenuField*	popupLabel = new BMenuField(r, NULL, NULL, fEditorAppMenu);
	box->AddChild(popupLabel);
	popupLabel->SetFont(be_bold_font);
	popupLabel->SetDivider(0.0);
	SetGrey(popupLabel, kLtGray);
	top += 35.0;

	// Radio buttons
	// Use doc settings
	r.Set(20, top, 250, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb1", "Use document font settings", msg);
	box->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	SetGrey(radioButton, kLtGray);
	fUseDocSettings = radioButton;
	top += 20;
	
	// Use default/app settings
	r.Set(20, top, 250, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb1", "Use preferences font settings", msg);
	box->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	SetGrey(radioButton, kLtGray);
	fUseAppSettings = radioButton;
	top += 20;
	
	this->DisableExclusiveSettings();
}

// ---------------------------------------------------------------------------

void
MEditorView::DisableExclusiveSettings()
{
	// If the "Use an External Editor" is true
	//		disable radio buttons, enable popup selection
	// If false
	//		enable radio buttons, disable popup selection

	bool doWork = false;
	bool radioSetting = true;
	bool popupSetting = false;
			
	if (fUseExternalEditorSettings->Value() == 1 && fEditorAppMenu->IsEnabled() == false) {
		doWork = true;
		radioSetting = false;
		popupSetting = true;
	}
	else if (fNewSettings.pUseExternalEditor == false && fEditorAppMenu->IsEnabled() == true) {
		doWork = true;
		radioSetting = true;
		popupSetting = false;
	}
	
	// do the enabled/disabling if needed...
	if (doWork) {
		fUseDocSettings->SetEnabled(radioSetting);
		fUseAppSettings->SetEnabled(radioSetting);
		fEditorAppMenu->SetEnabled(popupSetting);
	}
}

// ---------------------------------------------------------------------------

void
MEditorView::BuildEditorAppMenu()
{
	// Get the list of all supporting signatures for text/x-source-code
	// Fill in the menu with corresponding applications
	// If I can't find an application, then don't insert in list
	// Meanwhile, fill up fEditorSignatureList with the signatures
	
	// make sure the list of signatures is empty, then put a NULL entry
	// into slot 0 and 1 (to correspond to Preferred Application and separator
	// of the popup list).

	this->EmptyAppList();
	fEditorSignatureList.AddItem(NULL);
	fEditorSignatureList.AddItem(NULL);
	
	BMimeType mime(kIDETextMimeType);
	BMenuItem* item = new BMenuItem("Preferred Application", new BMessage(msgPopupChanged));
	item->SetTarget(this);
	fEditorAppMenu->AddItem(item);
	
	fEditorAppMenu->AddSeparatorItem();

	// our menu and app list now both contain two items, fill in the dynamic part
	
	BMessage message;
	mime.GetSupportingApps(&message);
	int32 index = 0;
	const char* signature;
	while (true) {
		if (message.FindString("applications", index++, &signature) != B_OK) {
			// no more signatures - break out of loop
			break;
		}
		
		// we got a signature...
		// (but skip it if the signature is for the BeIDE itself)
		if (strcmp(signature, kIDESigMimeType) == 0) {
			continue;
		}
		
		// find the corresponding application
		entry_ref entry;	
		if (be_roster->FindApp(signature, &entry) == B_OK) {
			BMenuItem* item = new BMenuItem(entry.name, new BMessage(msgPopupChanged));
			fEditorAppMenu->AddItem(item);
			char* mySignature = new char[strlen(signature) + 1];
			strcpy(mySignature, signature);
			fEditorSignatureList.AddItem(mySignature);
		}
	}
	fEditorAppMenu->SetTargetForItems(this);
	fEditorAppMenu->ItemAt(0)->SetMarked(true);
}

// ---------------------------------------------------------------------------

void
MEditorView::EmptyAppList()
{
	// Iterate the fEditorSignatureList and delete all entries
	
	int32 count = fEditorSignatureList.CountItems();
	for (int32 i = kFirstEditorIndex; i < count; i++) {
		char* aSignature = (char*)fEditorSignatureList.ItemAtFast(i);
		delete [] aSignature;
	}
	fEditorSignatureList.MakeEmpty();
}
