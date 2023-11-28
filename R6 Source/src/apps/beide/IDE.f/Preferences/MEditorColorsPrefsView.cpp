//========================================================================
//	MEditorColorsPrefsView.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MEditorColorsPrefsView.h"
#include "MEditorColorView.h"
#include "MBoxControl.h"
#include "MTextControl.h"
#include "MTextView.h"
#include "MDefaultPrefs.h"
#include "IDEMessages.h"
#include "Utils.h"

#include "ColorDropFilter.h"

#include <ColorControl.h>
#include <CheckBox.h>
#include <Box.h>
#include <MenuItem.h>
#include <Debug.h>

const char * titleECV = "Editor Settings";

// ---------------------------------------------------------------------------
//		MEditorColorsPrefsView
// ---------------------------------------------------------------------------
//	Constructor

MEditorColorsPrefsView::MEditorColorsPrefsView(
	MPreferencesWindow&	inWindow,
	BRect				inFrame)
	: MPreferencesView(inWindow, inFrame, "colorsview")
{
	SetPointers(&fOldSettings, &fNewSettings, sizeof(fNewSettings));
	fFocusColorView = nil;
}

// ---------------------------------------------------------------------------
//		~MEditorColorsPrefsView
// ---------------------------------------------------------------------------
//	Destructor

MEditorColorsPrefsView::~MEditorColorsPrefsView()
{
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MEditorColorsPrefsView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgViewClicked:
			fBoxControl->MakeFocus(true);
			ColorViewChanged(*inMessage);
			break;

		case msgColorControlClicked:
			ColorChanged();
			break;

		case msgTextChanged:
		case msgCheckBoxChanged:
			ExtractInfo();
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		ColorViewChanged
// ---------------------------------------------------------------------------
//	One of the color views changed.

void
MEditorColorsPrefsView::ColorViewChanged(
	BMessage& 	inMessage)
{
	MEditorColorView*		focusColorView = nil;
	
	if (B_NO_ERROR == inMessage.FindPointer("source", (void **)&focusColorView) &&
		focusColorView != nil)
	{
		if (focusColorView != fHiliteColorView)
			fHiliteColorView->SetEnabled(false);
		if (focusColorView != fBackgroundView)
			fBackgroundView->SetEnabled(false);

		focusColorView->SetEnabled(true);		// make sure
		fFocusColorView = focusColorView;
		UpdateValues();		
		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		ColorChanged
// ---------------------------------------------------------------------------
//	The color control changed.

void
MEditorColorsPrefsView::ColorChanged()
{
	rgb_color	newColor = fColorMap->ValueAsColor();

	if (fFocusColorView != nil)
	{
		fFocusColorView->SetValue(newColor);
		fFocusColorView->Draw(fFocusColorView->Bounds());		
	}

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		ExtractInfo
// ---------------------------------------------------------------------------

void
MEditorColorsPrefsView::ExtractInfo()
{
	// CheckBoxes
	fNewSettings.pSortFunctionPopup = fSortFunctionCB->Value() == 1;
	fNewSettings.pRelaxedPopupParsing = fRelaxedCB->Value() == 1;
	fNewSettings.pBalanceWhileTyping = fBalanceCB->Value() == 1;
	fNewSettings.pRememberSelection = fSelectionCB->Value() == 1;

	// Flashing delay value
	fNewSettings.pFlashingDelay = fDelayBoxControl->GetValue();
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MEditorColorsPrefsView::GetData(
	BMessage&	inOutMessage,
	bool		/*isProxy*/)
{
	inOutMessage.AddData(kAppEditorPrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings), false);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MEditorColorsPrefsView::SetData(
	BMessage&	inMessage)
{
	ssize_t				length;
	AppEditorPrefs*		inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kAppEditorPrefs, kMWPrefs, (const void**)&inPrefs, &length))
	{
		ASSERT(length == sizeof(AppEditorPrefs));

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
MEditorColorsPrefsView::UpdateValues()
{
	// Checkboxes
	fRelaxedCB->SetValue(fNewSettings.pRelaxedPopupParsing);
	fSortFunctionCB->SetValue(fNewSettings.pSortFunctionPopup);

	// Balance while typing
	fBalanceCB->SetValue(fNewSettings.pBalanceWhileTyping);
	String		text = fNewSettings.pFlashingDelay;
	fDelayBox->SetText(text);

	// Remember selection position
	fSelectionCB->SetValue(fNewSettings.pRememberSelection);
	fHiliteColorView->SetValue(fNewSettings.pHiliteColor);
	fBackgroundView->SetValue(fNewSettings.pBackgroundColor);
	fHiliteColorView->Draw(fHiliteColorView->Bounds());
	fBackgroundView->Draw(fBackgroundView->Bounds());

	// Color
	rgb_color		color;
	if (fFocusColorView != nil)
	{
		color = fFocusColorView->Value();

		fColorMap->SetValue(color);
	}
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------

void
MEditorColorsPrefsView::DoFactorySettings()
{
	MDefaultPrefs::SetAppEditorDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------

void
MEditorColorsPrefsView::DoRevert()
{
	MPreferencesView::DoRevert();
	UpdateValues();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MEditorColorsPrefsView::Title()
{
	return titleECV;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MEditorColorsPrefsView::Targets()
{
	return (kMWDefaults);
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MEditorColorsPrefsView::AttachedToWindow()
{
	BRect				bounds = Bounds();
	BRect				r = bounds;
	BBox*				box;
	BMessage*			msg;
	MEditorColorView*	colorView;
	MBoxControl*		boxControl;
	BCheckBox*			checkBox;
	MTextControl*		textControl;
	BStringView*		caption;
	float				top = 15.0;
	float				left = 10.0;
	float				middle = (bounds.right - bounds.left) / 2.0f;
	float				right = r.right - 10.0;
	float				boxtop;
	const float			kColorViewWidth = 120.0;

	// Box
	box = new BBox(r, "colors");
	box->SetLabel("Editor Settings");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Balance when typing checkbox
	r.Set(left, top, middle, top + 16);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "balance", "Balance while typing", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fBalanceCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Relaxed C Popup rules
	r.Set(left, top, middle, top + 16);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "balance", "Relaxed C popup parsing", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fRelaxedCB = checkBox;
	SetGrey(checkBox, kLtGray);

	top = 15.0;
	left = middle;

	// Sort function popup
	r.Set(left, top, right, top + 16);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "balance", "Sort function popup", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fSortFunctionCB = checkBox;
	SetGrey(checkBox, kLtGray);

	top += 40.0;
	left = 10.0;

	// Flashing delay textcontrol
	r.Set(20, top, 140, top + 16.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "tc1", "Flashing Delay:", "", msg);
	box->AddChild(textControl);
	textControl->SetTarget(this);
	textControl->SetFont(be_plain_font);
	textControl->SetDivider(90);
	SetGrey(textControl, kLtGray);

	fDelayBoxControl = textControl;
	fDelayBox = (BTextView*) textControl->ChildAt(0);
	fDelayBox->SetMaxBytes(2);
	DisallowInvalidChars(*fDelayBox);
	DisallowNonDigitChars(*fDelayBox);
	fDelayBox->SetTabWidth(fDelayBox->StringWidth("M"));
	top += 25.0;
	
	// Remember Selection checkbox
	r.Set(left, top, 250, top + 16);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "selection", "Remember Selection Position", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fSelectionCB = checkBox;
	top += 25.0;

	// Static text
	r.Set(left, top, right, top + 20);
	caption = new BStringView(r, "st1", "Color Settings:"); 
	box->AddChild(caption);
	SetGrey(caption, kLtGray);
	top += 20;

	// ColorViews and BoxControl
	r.Set(left, top, right, top + 20);
	boxControl = new MBoxControl(r, "");
	box->AddChild(boxControl);
	SetGrey(boxControl, kLtGray);
	fBoxControl = boxControl;

	boxtop = 2.0;
	right = (right - left - 2.0) / 2.0f;
	left = 2.0;

	// Hilite Color
	r.Set(left, boxtop, right, boxtop + 16);
	colorView = new MEditorColorView(r, "Hilite", fNewSettings.pHiliteColor);
	boxControl->AddChild(colorView);
	colorView->SetTarget(this);
	SetGrey(colorView, kLtGray);
	fHiliteColorView = colorView;
	
	// Background Color
	left = right + 1.0;
	right = bounds.right - 10.0 - 10.0 - 3.0;
	r.Set(left, boxtop, right, boxtop + 16);
	colorView = new MEditorColorView(r, "Background", fNewSettings.pBackgroundColor);
	boxControl->AddChild(colorView);
	colorView->SetTarget(this);
	SetGrey(colorView, kLtGray);
	fBackgroundView = colorView;

	right = r.right - 10.0;
	top += 40;
	left = 10;

	// Color Control
	msg = new BMessage(msgColorControlClicked);
	BColorControl*	colorMap = new BColorControl(BPoint(left, top), B_CELLS_32x8, 8.0, "color", msg);

	box->AddChild(colorMap);
	colorMap->SetTarget(this);
	fColorMap = colorMap;
	
	colorMap->AddFilter(new ColorDropFilter<BColorControl>(colorMap));
	
	r = colorMap->Bounds();
	top += r.Height() + 5;
	
	fHiliteColorView->SetEnabled(true);
	fFocusColorView = fHiliteColorView;
}
