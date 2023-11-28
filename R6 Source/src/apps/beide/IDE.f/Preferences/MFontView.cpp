//========================================================================
//	MFontView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <stdio.h>
#include <string.h>

#include "MFontView.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "MSyntaxStyleView.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <MenuItem.h>
#include <Box.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <ScrollView.h>
#include <Debug.h>

const char * titleFV = "Font";

// ---------------------------------------------------------------------------
//		MFontView
// ---------------------------------------------------------------------------
//	Constructor

MFontView::MFontView(
	MPreferencesWindow&	inWindow,
	BRect				inFrame)
	: MPreferencesView(inWindow, inFrame, "fontview")
{
	SetPointers(&fOldSettings, &fNewSettings, sizeof(fNewSettings));
}

// ---------------------------------------------------------------------------
//		~MFontView
// ---------------------------------------------------------------------------
//	Destructor

MFontView::~MFontView()
{
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MFontView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgFontChosen:
			FontChanged(*inMessage);
			break;

		case msgSizeChosen:
			SizeChanged(*inMessage);
			break;

		case msgCheckBoxChanged:
			DataChanged();
			break;

		case msgTextChanged:
			fNewSettings.pTabSize = fTabTextControl->GetValue();
			ValueChanged();
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		FontChanged
// ---------------------------------------------------------------------------

void
MFontView::FontChanged(
	BMessage&	inMessage)
{
	BMenuItem*		item;
	BFont			font;

	if (B_NO_ERROR == inMessage.FindPointer("source", (void **) &item) &&
		B_NO_ERROR == inMessage.FindFlat(kFontFamilyAndStyle, &font))
	{
		GetBFontFamilyAndStyle(font, fNewSettings.pFontFamily, fNewSettings.pFontStyle);
		font.SetSize(fNewSettings.pFontSize);
		fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font );

		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		SizeChanged
// ---------------------------------------------------------------------------

void
MFontView::SizeChanged(
	BMessage&	inMessage)
{
	BMenuItem*		item;

	if (B_NO_ERROR == inMessage.FindPointer("source", (void **) &item))
	{
		if (1 == sscanf(item->Label(), "%f", &fNewSettings.pFontSize))
		{
			BFont		font;

			font.SetFamilyAndStyle(fNewSettings.pFontFamily, fNewSettings.pFontStyle);
			font.SetSize(fNewSettings.pFontSize);
			fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font);
			fSampleText->Invalidate();
	
			ValueChanged();
		}
	}
}

// ---------------------------------------------------------------------------
//		DataChanged
// ---------------------------------------------------------------------------
//	Extract the data from the checkbox and the TextView.

void
MFontView::DataChanged()
{
	// AutoIndent CheckBox
	fNewSettings.pDoAutoIndent = fAutoIndentCB->Value() == 1;

	// Tab Size TextView
	sscanf(fTabText->Text(), "%ld", &fNewSettings.pTabSize);
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MFontView::GetData(
	BMessage&	inOutMessage,
	bool		/*isProxy*/)
{
	inOutMessage.AddData(kFontPrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings), false);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MFontView::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;
	FontPrefs*		inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kFontPrefs, kMWPrefs, (const void **) &inPrefs, &length))
	{
		ASSERT(length == sizeof(FontPrefs));

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
MFontView::UpdateValues()
{
	BMenuItem*		item;
	String			text;
	
	// Tab size
	text = fNewSettings.pTabSize;
	fTabText->SetText(text);

	// AutoIndent checkbox
	fAutoIndentCB->SetValue(fNewSettings.pDoAutoIndent);

	// Font Popup
	String		fontName = fNewSettings.pFontFamily;
	
	fontName += ' ';
	fontName += fNewSettings.pFontStyle;

	item = fFontPopup->FindItem(fontName);
	if (!item)
	{
		MSyntaxStyleView::GetDefaultFontMenuName(fontName);
		item = fFontPopup->FindItem(fontName);	// This font should always be present
	}

	ASSERT(item);
	if (item && item != fFontPopup->FindMarked())
		item->SetMarked(true);
	else
	{
		// The specified font isn't on this machine
	}

	BFont		font;
	
	font.SetFamilyAndStyle(fNewSettings.pFontFamily, fNewSettings.pFontStyle);
	font.SetSize(fNewSettings.pFontSize);
	fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font);
	
	// Size Popup
	if (fNewSettings.pFontSize < kFirstFontSize ||
			fNewSettings.pFontSize > kLastFontSize)
			fNewSettings.pFontSize = kDefaultFontSize;

	String		fontSize = (int32) fNewSettings.pFontSize;
	item = fSizePopup->FindItem(fontSize);
	ASSERT(item);
	if (item && item != fSizePopup->FindMarked())
	{
		item->SetMarked(true);
		fSampleText->SetFontSize(fNewSettings.pFontSize);
	}

	fSampleText->Invalidate();
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------

void
MFontView::DoFactorySettings()
{
	MDefaultPrefs::SetFontDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MFontView::Title()
{
	return titleFV;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MFontView::Targets()
{
	return (kMWDefaults | kCurrentProject | kMessageWindow | kTextWindows);
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MFontView::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BMessage*		msg;
	BScrollView*	frame;
	BPopUpMenu*		popup;
	MTextControl*	textControl;
	BMenuField*		menufield;

	// Box
	r = Bounds();
	box = new BBox(r, "FontInfo");
	box->SetLabel("Font & Size Settings");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);
	// Font Popup Menu
	float		top = 15.0;
	r.Set(10, top, 350, top + 16);

	popup = new BPopUpMenu("Font Menu");
	fFontPopup = popup;

	String		fontName;
	BFont		font;
	int32		count = count_font_families();

	for (int32 i = 0; i < count; i++)
	{
		font_family		family;
		get_font_family(i, &family, NULL);
		
		int32	numStyles = count_font_styles(family);
		for (int32 j = 0; j < numStyles; j++) 
		{
			font_style 		style;

			get_font_style(family, j, &style);
			fontName = family;
			fontName += ' ';
			fontName += style;
			
			font.SetFamilyAndStyle(family, style);

			msg = new BMessage(msgFontChosen);
			msg->AddFlat(kFontFamilyAndStyle, &font);
			
			popup->AddItem(new BMenuItem(fontName, msg));
		}
	}

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "applytobar", "Font", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(40);
	SetGrey(menufield, kLtGray);
	top += 28;

	// Font size popup
	// Size
	r.Set(10, top, 250, top + 16);

	popup = new BPopUpMenu("sizemenu");
	fSizePopup = popup;

	for (int32 i = kFirstFontSize; i <= kLastFontSize; i++)
	{
		char	name[5];
		
		sprintf(name, "%ld", i);

		msg = new BMessage(msgSizeChosen);
		popup->AddItem(new BMenuItem(name, msg));
	}

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "applytobar", "Size", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(40);
	SetGrey(menufield, kLtGray);

	top += 30;
	
	// Sample text
	r.Set(10, top, 250, top + 48);
//	fSampleText = new MTextView(r, "sampletext", B_FOLLOW_NONE, B_PULSE_NEEDED | B_WILL_DRAW); 
	fSampleText = new BTextView(r, "sampletext", GetTextRect(r), B_FOLLOW_NONE, B_PULSE_NEEDED | B_WILL_DRAW);

	frame = new BScrollView("frame", fSampleText);		// For the border
	box->AddChild(frame);

	fSampleText->MakeSelectable(false);
	fSampleText->MakeEditable(false);
	fSampleText->SetText("Be all you can Be");
	top += 60;

	// CheckBox
	// AutoIndent
	top += 3;
	r.Set(10, top, 140, top + 16);
	msg = new BMessage(msgCheckBoxChanged);
	fAutoIndentCB = new BCheckBox(r, "autoindent", "Auto Indent", msg); 
	box->AddChild(fAutoIndentCB);
	fAutoIndentCB->SetTarget(this);
	SetGrey(fAutoIndentCB, kLtGray);

	// Tab size
	// TextControl
	top -= 1;
	r.Set(140, top, 240, top + 14);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "tc1", "Tab Size", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(60);
	textControl->SetTarget(this);
	SetGrey(textControl, kLtGray);

	fTabTextControl = textControl;
	fTabText = (BTextView*) textControl->ChildAt(0);
	fTabText->SetMaxBytes(2);
	DisallowInvalidChars(*fTabText);
	DisallowNonDigitChars(*fTabText);

	fTabText->SetText("4");
	fTabText->SetMaxBytes(2);
	fTabTextControl->MakeFocus();
}
