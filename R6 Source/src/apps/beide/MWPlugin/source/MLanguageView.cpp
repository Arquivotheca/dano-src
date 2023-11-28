//========================================================================
//	MLanguageView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MLanguageView.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "CString.h"
#include "MPlugin.h"

#include <Box.h>
#include <PopUpMenu.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Window.h>
#include <CheckBox.h>
#include <Debug.h>

const char * titleLV = "Language/C/C++ Language";

// ---------------------------------------------------------------------------
//		 MLanguageView
// ---------------------------------------------------------------------------
//	Constructor

MLanguageView::MLanguageView(
	BRect				inFrame)
	: MPlugInPrefsView(inFrame, "languageview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		 ~MLanguageView
// ---------------------------------------------------------------------------
//	Destructor

MLanguageView::~MLanguageView()
{
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MLanguageView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
			memset(fNewSettings.pPrefixFile, 0, sizeof(fNewSettings.pPrefixFile));
			strcpy(fNewSettings.pPrefixFile, fPrefixBox->Text());
			ValueChanged();
			break;

		case msgCheckBoxChanged:
			UpdateCheckBoxes();
			break;

		case msgPopupChanged:
			UpdatePopups();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MLanguageView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kLanguagePrefs, kMWCCPlugType, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MLanguageView::SetData(
	BMessage&	inMessage)
{
	LanguagePrefs*		prefs;
	long			length;

	if (B_NO_ERROR == inMessage.FindData(kLanguagePrefs, kMWCCPlugType, (const void**) &prefs, &length))
	{
		ASSERT(length == sizeof(LanguagePrefs));

		fNewSettings = *prefs;
		fNewSettings.SwapBigToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
	else 
	printf("MLanguageView::SetData: nodata\n");
}

// ---------------------------------------------------------------------------
//		 UpdateCheckBoxes
// ---------------------------------------------------------------------------

void
MLanguageView::UpdateCheckBoxes()
{
	fNewSettings.pActivateCpp = fActivateCpp->Value() == 1;
	fNewSettings.pEnableExceptions = fEnableExceptions->Value() == 1;
	fNewSettings.pEnableRTTI = fEnableRTTI->Value() == 1;
	fNewSettings.pRequireProtos = fRequireProtos->Value() == 1;
	fNewSettings.pPoolStrings = fPoolStrings->Value() == 1;
	fNewSettings.pDontReuseStrings = fDontReuseStrings->Value() == 1;
	fNewSettings.pANSIStrict = fANSIStrict->Value() == 1;
	fNewSettings.pANSIKeywordsOnly = fANSIKeywords->Value() == 1;
	fNewSettings.pExpandTrigraphs = fExpandTrigraphs->Value() == 1;
	fNewSettings.pEnumsAreInts = fEnumsRInts->Value() == 1;
	fNewSettings.pAutoInline = fAutoInline->Value() == 1;
	fNewSettings.pUseUnsignedChars = fUnsignedChars->Value() == 1;
	fNewSettings.pEnableBool = fEnableBool->Value() == 1;
	fNewSettings.pMPWPointerRules = fRelaxedPointers->Value() == 1;

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 UpdatePopups
// ---------------------------------------------------------------------------

void
MLanguageView::UpdatePopups()
{
	BMenuItem*		item;

	// Inline Popup
	item = fInlinePopup->FindMarked();
	ASSERT(item);
	if (item)
		fNewSettings.pInlineKind = fInlinePopup->IndexOf(item);

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MLanguageView::UpdateValues()
{
	// Prefix file name
	fPrefixBox->SetText(fNewSettings.pPrefixFile);
	fPrefixBox->Select(0, 256);

	// CheckBoxes
	fActivateCpp->SetValue(fNewSettings.pActivateCpp);
	fEnableExceptions->SetValue(fNewSettings.pEnableExceptions);
	fEnableRTTI->SetValue(fNewSettings.pEnableRTTI);
	fRequireProtos->SetValue(fNewSettings.pRequireProtos);
	fPoolStrings->SetValue(fNewSettings.pPoolStrings);
	fDontReuseStrings->SetValue(fNewSettings.pDontReuseStrings);
	fANSIStrict->SetValue(fNewSettings.pANSIStrict);
	fANSIKeywords->SetValue(fNewSettings.pANSIKeywordsOnly);
	fExpandTrigraphs->SetValue(fNewSettings.pExpandTrigraphs);
	fEnumsRInts->SetValue(fNewSettings.pEnumsAreInts);
	fAutoInline->SetValue(fNewSettings.pAutoInline);
	fEnableBool->SetValue(fNewSettings.pEnableBool);
	fUnsignedChars->SetValue(fNewSettings.pUseUnsignedChars);
	fRelaxedPointers->SetValue(fNewSettings.pMPWPointerRules);

	// Inline Popup
	BMenuItem*		item;
	item = fInlinePopup->ItemAt(fNewSettings.pInlineKind);
	ASSERT(item);
	if (item && item != fInlinePopup->FindMarked())
		item->SetMarked(true);
}

// ---------------------------------------------------------------------------
//		 DoFactorySettings
// ---------------------------------------------------------------------------

void
MLanguageView::DoFactorySettings()
{
	MDefaultPrefs::SetLanguageDefaults(fNewSettings);

	UpdateValues();
}

// ---------------------------------------------------------------------------
//		 GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MLanguageView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		 Title
// ---------------------------------------------------------------------------

const char *
MLanguageView::Title()
{
 	return titleLV;
}

// ---------------------------------------------------------------------------
//		 Targets
// ---------------------------------------------------------------------------

TargetT
MLanguageView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		 ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MLanguageView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//		 DoSave
// ---------------------------------------------------------------------------

void
MLanguageView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		 DoRevert
// ---------------------------------------------------------------------------

void
MLanguageView::DoRevert()
{
}
// ---------------------------------------------------------------------------
//		 FilterKeyDown
// ---------------------------------------------------------------------------

bool
MLanguageView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		 ValueChanged
// ---------------------------------------------------------------------------

void
MLanguageView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		 AttachedToWindow
// ---------------------------------------------------------------------------

void
MLanguageView::AttachedToWindow()
{
	BRect			r;
	BRect			rr;
	BBox*			box;
	BCheckBox*		checkBox;
	BMessage*		msg;
	BPopUpMenu*		popup;
	MTextControl*	textControl;
	BTextView*		text;
	BMenuField*		menufield;
	float			top = 15.0;
	float			left = 10.0;
	float			left2 = 180.0;

	// Box
	r = Bounds();
	box = new BBox(r, "AppInfo");
	box->SetLabel("Language Settings");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Activate C++ Compiler
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Activate C++ Compiler", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fActivateCpp = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Enable C++ Exceptions
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Enable C++ Exceptions", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fEnableExceptions = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Enable RTTI
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Enable RTTI", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fEnableRTTI = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Require Function Prototypes
	r.Set(left, top, left2 + 20, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Require Function Prototypes", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fRequireProtos = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 25.0;

	// Inline type popup
	r.Set(left, top, left + 300, top + 18.0);

	popup = new BPopUpMenu("inlining");
	fInlinePopup = popup;
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Don't Inline", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Smart Inline", msg));
	
	String		depth;
	
	for (int i = 1; i <= 8; i++)
	{
		depth = i;
		msg = new BMessage(msgPopupChanged);
		popup->AddItem(new BMenuItem(depth, msg));
	}
	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "inlinebar", "Inline Depth:", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(80);
	SetGrey(menufield, kLtGray);
	top += 28;

	// Auto Inline
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Auto Inline", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fAutoInline = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Enums are Always Int
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Enums are Always Int", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fEnumsRInts = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Pool Strings
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Pool Strings", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fPoolStrings = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Don't Reuse Strings
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Don't Reuse Strings", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fDontReuseStrings = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	top = 15.0;
	left = 180.0;
	left2 = 335.0;

	// ANSI Strict
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "ANSI Strict", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fANSIStrict = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// ANSI Keywords Only
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "ANSI Keywords Only", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fANSIKeywords = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Expand Trigraphs
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Expand Trigraphs", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fExpandTrigraphs = checkBox;
	SetGrey(checkBox, kLtGray);

	top += 70;

	// Enable bool Support
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Enable bool Support", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fEnableBool = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Use Unsigned Chars
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Use Unsigned Chars", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fUnsignedChars = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;
	
	// Relaxed Pointer Type Rules
	r.Set(left, top, left2, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb", "Relaxed Pointer Type Rules", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fRelaxedPointers = checkBox;
	SetGrey(checkBox, kLtGray);

	// Prefix file box
	top = 220.0;

	r.Set(10, top, 260, top + 14);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st1", "Prefix File", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(70);
	textControl->SetTarget(this);
	SetGrey(textControl, kLtGray);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(64);
	fPrefixBox = text;

	fPrefixBox->SetMaxBytes(B_FILE_NAME_LENGTH);
	fPrefixBox->MakeFocus();
}
