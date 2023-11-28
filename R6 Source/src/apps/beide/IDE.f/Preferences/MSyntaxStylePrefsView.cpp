//========================================================================
//	MSyntaxStylePrefsView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>
#include <stdio.h>

#include "MSyntaxStylePrefsView.h"
#include "MSyntaxStyleView.h"
#include "MBoxControl.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "ColorDropFilter.h"

#include <MenuItem.h>
#include <MenuField.h>
#include <ColorControl.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <Box.h>
#include <ScrollView.h>
#include <Debug.h>

const char * titleFSV = "Syntax Styling";

// ---------------------------------------------------------------------------
//		MSyntaxStylePrefsView
// ---------------------------------------------------------------------------
//	Constructor

MSyntaxStylePrefsView::MSyntaxStylePrefsView(
	MPreferencesWindow&	inWindow,
	BRect				inFrame)
	: MPreferencesView(inWindow, inFrame, "syntaxview")
{
	SetPointers(&fOldSettings, &fNewSettings, sizeof(fNewSettings));
	fFocusStyleView = nil;
}

// ---------------------------------------------------------------------------
//		~MSyntaxStylePrefsView
// ---------------------------------------------------------------------------
//	Destructor

MSyntaxStylePrefsView::~MSyntaxStylePrefsView()
{
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::MessageReceived(
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

		case msgViewClicked:
			fStyleBox->MakeFocus(true);
			SyntaxViewChanged(*inMessage);
			break;

		case msgColorControlClicked:
			ColorChanged();
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
MSyntaxStylePrefsView::FontChanged(
	BMessage&	inMessage)
{
	BMenuItem*		item;
	BFont			font;

	ASSERT(fFocusStyleView != nil);

	if (B_NO_ERROR == inMessage.FindPointer("source", (void **)&item) &&
		B_NO_ERROR == inMessage.FindFlat(kFontFamilyAndStyle, &font))
	{
		if (fFocusStyleView != nil)
			fFocusStyleView->SetFontsID(font);

		font.SetSize(fFocusStyleView->FontsSize());
		fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font);

		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		SizeChanged
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::SizeChanged(
	BMessage&	inMessage)
{
	BMenuItem*		item;
	if (B_NO_ERROR == inMessage.FindPointer("source", (void **)&item))
	{
		float			fontSize;

		if (1 == sscanf(item->Label(), "%f", &fontSize))
		{
			if (fFocusStyleView != nil)
				fFocusStyleView->SetFontsSize(fontSize);

			BFont		font;

			font.SetSize(fontSize);
			fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font, B_FONT_SIZE);

			ValueChanged();
		}
	}
}

// ---------------------------------------------------------------------------
//		ColorChanged
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::ColorChanged()
{
	rgb_color	newColor = fColorMap->ValueAsColor();

	if (fFocusStyleView != nil)
		fFocusStyleView->SetFontsColor(newColor);

	BFont		font;
	rgb_color	oldColor;

	fFocusStyleView->GetFontAndColor(font, oldColor);

	fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font, B_FONT_ALL, &newColor);

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		SyntaxViewChanged
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::SyntaxViewChanged(
	BMessage&	inMessage)
{
	MSyntaxStyleView*		focusStyleView = nil;
	
	if (B_NO_ERROR == inMessage.FindPointer("source", (void **)&focusStyleView) &&
		focusStyleView != nil)
	{
		if (focusStyleView != fTextStyleView)
			fTextStyleView->SetEnabled(false);
		if (focusStyleView != fCommentsStyleView)
			fCommentsStyleView->SetEnabled(false);
		if (focusStyleView != fKeyWordsStyleView)
			fKeyWordsStyleView->SetEnabled(false);
		if (focusStyleView != fStringsStyleView)
			fStringsStyleView->SetEnabled(false);

		focusStyleView->SetEnabled(true);		// make sure
		fFocusStyleView = focusStyleView;
		UpdateValues();		
		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		DataChanged
// ---------------------------------------------------------------------------
//	Extract the data from the checkbox.

void
MSyntaxStylePrefsView::DataChanged()
{
	// Use Syntax Styling 
	fNewSettings.useSyntaxStyling = fUseSyntaxStylingCB->Value() == 1;

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MSyntaxStylePrefsView::GetData(
	BMessage&	inOutMessage,
	bool		/*isProxy*/)
{
	inOutMessage.AddData(kSyntaxStylePrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings), false);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MSyntaxStylePrefsView::SetData(
	BMessage&	inMessage)
{
	ssize_t				length;
	SyntaxStylePrefs*	inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kSyntaxStylePrefs, kMWPrefs, (const void **)&inPrefs, &length))
	{
		ASSERT(length == sizeof(SyntaxStylePrefs));

		fNewSettings = *inPrefs;
		fOldSettings = fNewSettings;

		fTextStyleView->SetData(fNewSettings.text);
		fCommentsStyleView->SetData(fNewSettings.comments);
		fKeyWordsStyleView->SetData(fNewSettings.keywords);
		fStringsStyleView->SetData(fNewSettings.strings);

		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::DoRevert()
{
	MPreferencesView::DoRevert();

	fTextStyleView->InvalidateColorAndFont();
	fCommentsStyleView->InvalidateColorAndFont();
	fKeyWordsStyleView->InvalidateColorAndFont();
	fStringsStyleView->InvalidateColorAndFont();
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MSyntaxStylePrefsView::UpdateValues()
{

	BMenuItem*		item;
	String			text;
	
	// AutoIndent checkbox
	fUseSyntaxStylingCB->SetValue(fNewSettings.useSyntaxStyling);

	String			fontName;
	font_family		family;
	font_style		style;

	if (fFocusStyleView != nil)
	{
		fFocusStyleView->GetFontMenuName(fontName);
		fFocusStyleView->GetFontFamilyAndStyle(family, style);
	}
	else
	{
		MSyntaxStyleView::GetDefaultFontMenuName(fontName);
		be_fixed_font->GetFamilyAndStyle(&family, &style);
	}
	
	// Font Popup
	item = fFontPopup->FindItem(fontName);
	if (! item)
	{
		MSyntaxStyleView::GetDefaultFontMenuName(fontName);
		item = fFontPopup->FindItem(fontName);
	}
	ASSERT(item);
	if (item && item != fFontPopup->FindMarked())
		item->SetMarked(true);
	else
	{
		// The specified font isn't on this machine
	}

	BFont		font;
	fSampleText->GetFont(&font);
	font.SetFamilyAndStyle(family, style);

	// Size Popup
	float		fontSize;

	if (fFocusStyleView != nil)
		fontSize = fFocusStyleView->FontsSize();
	else
		fontSize = kDefaultFontSize;

	if (fontSize < kFirstFontSize || fontSize > kLastFontSize)
		fontSize = kDefaultFontSize;

	String		fontSizeText = (int32) fontSize;
	item = fSizePopup->FindItem(fontSizeText);
	ASSERT(item);
	if (item && item != fSizePopup->FindMarked())
	{
		item->SetMarked(true);
		font.SetSize(fontSize);
	}

	// Color

	rgb_color		color;
	if (fFocusStyleView != nil)
	{
		color = fFocusStyleView->FontsColor();

		fColorMap->SetValue(color);
	}

	fSampleText->SetFontAndColor(0, fSampleText->TextLength(), &font, B_FONT_ALL, &color);
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::DoFactorySettings()
{
	MDefaultPrefs::SetSyntaxStylingDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
	
	// Invalidate these if they are any different from the factory settings
	fTextStyleView->Invalidate();
	fCommentsStyleView->Invalidate();
	fKeyWordsStyleView->Invalidate();
	fStringsStyleView->Invalidate();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MSyntaxStylePrefsView::Title()
{
	return titleFSV;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MSyntaxStylePrefsView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MSyntaxStylePrefsView::AttachedToWindow()
{
	BRect				r = Bounds();
	BBox*				box;
	BMessage*			msg;
	BScrollView*		frame;
	BPopUpMenu*			popup;
	BCheckBox*			checkBox;
	MSyntaxStyleView*	styleView;
	MBoxControl*		styleBox;
	BMenuField*			menufield;
	float				top = 15.0;
	float				left = 100.0;
	float				right = r.right - 10.0;

	// Box
	box = new BBox(r, "SyntaxStyling");
	box->SetLabel("Syntax Styling");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Use Syntax Styling CheckBox
	top += 3;
	r.Set(10, top, 140, top + 16);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "usestyles", "Use Syntax Styling", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fUseSyntaxStylingCB = checkBox;

	// StyleViews and StyleBox
	left = 145.0;
	r.Set(left, top, right, top + 80);
	styleBox = new MBoxControl(r, "");
	box->AddChild(styleBox);
	SetGrey(styleBox, kLtGray);
	fStyleBox = styleBox;

	top = 2.0;
	right = right - left - 2.0;
	left = 2.0;

	// Text
	r.Set(left, top, right, top + 16);
	styleView = new MSyntaxStyleView(r, "Text", fNewSettings.text);
	styleBox->AddChild(styleView);
	styleView->SetTarget(this);
	SetGrey(styleView, kLtGray);
	fTextStyleView = styleView;
	
	// Comments
	top += 20;
	r.Set(left, top, right, top + 16);
	styleView = new MSyntaxStyleView(r, "Comments", fNewSettings.comments);
	styleBox->AddChild(styleView);
	styleView->SetTarget(this);
	SetGrey(styleView, kLtGray);
	fCommentsStyleView = styleView;

	// Keywords
	top += 20;
	r.Set(left, top, right, top + 16);
	styleView = new MSyntaxStyleView(r, "Keywords", fNewSettings.keywords);
	styleBox->AddChild(styleView);
	styleView->SetTarget(this);
	SetGrey(styleView, kLtGray);
	fKeyWordsStyleView = styleView;

	// Strings
	top += 20;
	r.Set(left, top, right, top + 16);
	styleView = new MSyntaxStyleView(r, "Strings", fNewSettings.strings);
	styleBox->AddChild(styleView);
	styleView->SetTarget(this);
	SetGrey(styleView, kLtGray);
	fStringsStyleView = styleView;

	right += 2.0;

	// Font Popup Menu
	top = 100.0;
	left = 10.0;

	// Font Popup Menu
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

	popup = new BPopUpMenu("fontmenu");
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
	
	// Sample text
	r.Set(10, top, r.right + 10, top + 30);
	fSampleText = new BTextView(r, "sampletext", GetTextRect(r), B_FOLLOW_NONE, B_WILL_DRAW); 
//	fSampleText = new MTextView(r, "sampletext", B_FOLLOW_NONE, B_PULSE_NEEDED | B_WILL_DRAW); 

	frame = new BScrollView("frame", fSampleText);		// For the border
	box->AddChild(frame);

	fSampleText->MakeSelectable(false);
	fSampleText->MakeEditable(false);
	fSampleText->SetText("Be all you can Be");

	fTextStyleView->SetEnabled(true);
	fFocusStyleView = fTextStyleView;
}
