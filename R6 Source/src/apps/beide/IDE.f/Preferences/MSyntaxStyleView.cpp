//========================================================================
//	MSyntaxStyleView.cpp
//	Copyright 1995 - 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MSyntaxStyleView.h"
#include "IDEMessages.h"
#include "CString.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Message.h>
#include <stdio.h>
#include <stdlib.h>

const float kColorMargin = 65.0;
const float kColorWidth = 20.0;
const float kColorHeight = 12.0;

// ---------------------------------------------------------------------------
//		Message constants
// ---------------------------------------------------------------------------

const char* kRGBColor = "RGBColor";
const char* kFontFamily = "font_family";
const char* kFontStyle = "font_style";
const char* kFontSize = "font_size";

// allows colors to interact with ColorSelector
// (see http://www.xs4all.nl/~marcone/be.html)
const uint32 msgColorDrop = 'PSTE';
// should we switch to !FNT and flattented FFont?
// (see http://www.angryredplanet.com/beos/)
const uint32 msgFontDrop = 'FFam';

// ---------------------------------------------------------------------------
//		MSyntaxStyleView
// ---------------------------------------------------------------------------

MSyntaxStyleView::MSyntaxStyleView(
	BRect 		inArea,
	const char*	inName,
	FontInfo&	inFontInfo)
	: MBoxControlChild(
		inArea,
		inName),
		fInfo(inFontInfo)
{
	BMessage*		msg = new BMessage(msgViewClicked);
	msg->AddPointer("source", this);
	SetMessage(msg);
	SetFontsID(inFontInfo.family, inFontInfo.style);
	SetEnabled(false);
}

// ---------------------------------------------------------------------------
//		~MSyntaxStyleView
// ---------------------------------------------------------------------------

MSyntaxStyleView::~MSyntaxStyleView()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::Draw(
	BRect	/*inArea*/)
{
	const BRect		bounds = Bounds();
	
	// Name
	MovePenTo(bounds.left + 2, bounds.bottom - 2);
	DrawString(Name());
	DrawString(":");

	// Color Box
	fColorBox.Set(bounds.left + kColorMargin, bounds.bottom - 2 - kColorHeight, 
					bounds.left + kColorMargin + kColorWidth, bounds.bottom - 2);
	this->DrawColorBox(fColorBox, this);
	
	// Font family & size
	fFontBox.Set(bounds.left + 2 + kColorMargin + kColorWidth, bounds.top, 
				 bounds.right, bounds.bottom);
	this->DrawFontBox(fFontBox, this);

	DrawFrame();
}

// ---------------------------------------------------------------------------

void
MSyntaxStyleView::DrawColorBox(const BRect& inBox, BView* inView)
{
	// draw the color box in the view provided
	// we modify the rect, so do the modifications to a local copy
	BRect colorBox(inBox);
	
	inView->StrokeRect(colorBox);
	rgb_color highColor = inView->HighColor();
	inView->SetHighColor(fInfo.color);
	colorBox.InsetBy(1.0, 1.0);
	inView->FillRect(colorBox);
	inView->SetHighColor(highColor);	
}

// ---------------------------------------------------------------------------

void
MSyntaxStyleView::DrawFontBox(const BRect& inBox, BView* inView)
{
	// FontName/Size	
	String fontName = fInfo.family;
	fontName += ' ';
	fontName += fInfo.style;
	fontName += '/';
	fontName += (int32) fInfo.size;
	inView->MovePenTo(inBox.left + 4, inBox.bottom - 2);
	inView->DrawString(fontName);
}

// ---------------------------------------------------------------------------
//		MouseMoved
// ---------------------------------------------------------------------------
//	make ourself the focus when we're about to be dragged onto

void
MSyntaxStyleView::MouseMoved(
	BPoint		/*where*/,
	uint32		code,
	const BMessage *	inMessage)
{
	if (code == B_ENTERED_VIEW 
			 && inMessage != nil 
			 && (inMessage->HasData(kRGBColor, B_RGB_COLOR_TYPE) || inMessage->HasString(kFontFamily))) {
		MakeFocus(true);
	}
}

// ---------------------------------------------------------------------------

void
MSyntaxStyleView::MouseDown(BPoint point)
{
	// handle selection correctly
	MBoxControlChild::MouseDown(point);
	
	if (this->MouseMovedWhileDown(point)) {
		// the user wants to drag the font or color information
		// figure out which one, and drag it	
		BMessage msg;
		BBitmap* dragImage = NULL;
		BPoint dragStart;
		if (fColorBox.Contains(point)) {
			dragStart.x = point.x - fColorBox.left;
			dragStart.y = point.y - fColorBox.top;
			dragImage = this->CreateColorBitmap(point);
			msg.what = msgColorDrop;
			msg.AddData(kRGBColor, B_RGB_COLOR_TYPE, (const void*) &fInfo.color, sizeof(rgb_color));
		}
		else if (fFontBox.Contains(point)) {
			dragStart.x = point.x - fFontBox.left;
			dragStart.y = point.y - fFontBox.top;
			dragImage = this->CreateFontBitmap();
			msg.what = msgFontDrop;
			msg.AddString(kFontFamily, fInfo.family);
			msg.AddString(kFontStyle, fInfo.style);
			msg.AddInt32(kFontSize, (int32) fInfo.size);
		}
		this->DragMessage(&msg, dragImage, B_OP_BLEND, dragStart);
//		this->DragMessage(&msg, dragImage, dragStart);
	}
}

// ---------------------------------------------------------------------------

const int32 dragDelta = 3;

bool
MSyntaxStyleView::MouseMovedWhileDown(BPoint point)
{
	while (true) {
		BPoint newLoc;
		uint32 buttons;
	
		this->GetMouse(&newLoc, &buttons);
		
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0) {
			return false;
		}
		else if (abs((int)(newLoc.x - point.x)) > dragDelta || abs((int)(newLoc.y - point.y)) > dragDelta) {
			return true;
		}
		else {
			snooze(30000);
		}
	}
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::MessageReceived(
	BMessage * inMessage)
{
	
	bool handledHere = false;
	
	if (inMessage->WasDropped()) {
		rgb_color* color;
		int32 len;
		if (inMessage->FindData(kRGBColor, B_RGB_COLOR_TYPE, (const void**)&color, &len) == B_OK) {
			this->SetValue(*color);
			this->Invoke();
			handledHere = true;
		}
		else if (inMessage->HasString(kFontFamily) == true) {			
			const char* family = inMessage->FindString(kFontFamily);
			const char* style = inMessage->FindString(kFontStyle);
			int32 fontSize = inMessage->FindInt32(kFontSize);
			font_family fontFamily;
			font_style fontStyle;
			strcpy(fontFamily, family);
			strcpy(fontStyle, style);
			this->SetFontsID(fontFamily, fontStyle);
			this->SetFontsSize(fontSize);
			this->Invoke();
			handledHere = true;
		}
	}

	if (handledHere == false) {
		BView::MessageReceived(inMessage);
	}
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::SetData(
	const FontInfo&	inData)
{
	SetFontsID(inData.family, inData.style);
	SetFontsSize(inData.size);
	SetFontsColor(inData.color);
	// make sure changes are displayed
	this->InvalidateColorAndFont();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::GetData(
	FontInfo&	outData)
{
	outData = fInfo;
}

// ---------------------------------------------------------------------------
//		GetFontMenuName
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::GetFontFamilyAndStyle(
	font_family		outFamily,
	font_style		outStyle)
{
	strcpy(outFamily, fInfo.family);
	strcpy(outStyle, fInfo.style);
}

// ---------------------------------------------------------------------------
//		GetFontMenuName
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::GetFontMenuName(
	String&		outName)
{
	outName = fInfo.family;
	outName += ' ';
	outName += fInfo.style;
}

// ---------------------------------------------------------------------------
//		GetDefaultFontMenuName
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::GetDefaultFontMenuName(
	String&		outName)
{
	font_family		family;
	font_style		style;

	be_fixed_font->GetFamilyAndStyle(&family, &style);
	outName = family;
	outName += ' ';
	outName += style;
}

// ---------------------------------------------------------------------------
//		GetFont
// ---------------------------------------------------------------------------

void 
MSyntaxStyleView::GetFontAndColor(
	BFont&		outFont,
	rgb_color&	outColor)
{
	outFont.SetTo(fFont, B_FONT_FAMILY_AND_STYLE);
	outFont.SetSize(fInfo.size);
	outColor = fInfo.color;
}

// ---------------------------------------------------------------------------
//		SetFontsID
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::SetFontsID(
	const font_family	inFamily,
	const font_style	inStyle)
{
	BFont		font;
	
	font.SetFamilyAndStyle(inFamily, inStyle);
	fFont.SetTo(font, B_FONT_FAMILY_AND_STYLE);
	
	strcpy(fInfo.family, inFamily);
	strcpy(fInfo.style, inStyle);

	BRect	r = Bounds();
	r.left = kColorMargin + kColorWidth;
	Invalidate(r);
}

// ---------------------------------------------------------------------------
//		SetFontsID
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::SetFontsID(
	const BFont&	inFont)
{
	if (fFont.Compare(inFont, B_FONT_FAMILY_AND_STYLE) != 0)
	{
		font_family family;
		font_style style;
		fFont.SetTo(inFont, B_FONT_FAMILY_AND_STYLE);
		fFont.GetFamilyAndStyle(&family, &style);
		strcpy(fInfo.family, family);
		strcpy(fInfo.style, style);		
		BRect	r = Bounds();
		r.left = kColorMargin + kColorWidth;
		Invalidate(r);
	}
}

// ---------------------------------------------------------------------------
//		SetFontsSize
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::SetFontsSize(
	float	inFontSize)
{
	if (fInfo.size != inFontSize)
	{
		fInfo.size = inFontSize;
		BRect	r = Bounds();
		r.left = kColorMargin + kColorWidth;

		Invalidate(r);
	}
}

// ---------------------------------------------------------------------------
//		SetFontsColor
// ---------------------------------------------------------------------------
//	Dependent on operator!= existing for rgb_color

void
MSyntaxStyleView::SetFontsColor(
	rgb_color	inFontColor)
{
	if (fInfo.color != inFontColor) {
		fInfo.color = inFontColor;
		Draw(fColorBox);
	}
}

// ---------------------------------------------------------------------------
//		SetValue
// ---------------------------------------------------------------------------
//	Dependent on operator!= existing for rgb_color
//	This is to match the BColorControl api so the ColorControlDropFilter
//	will work.

void
MSyntaxStyleView::SetValue(
	rgb_color	inFontColor)
{
	SetFontsColor(inFontColor);
}

// ---------------------------------------------------------------------------
//		InvalidateColorAndFont
// ---------------------------------------------------------------------------

void
MSyntaxStyleView::InvalidateColorAndFont()
{
	BRect	r = Bounds();
	r.left = kColorMargin;
	Invalidate(r);
}

// ---------------------------------------------------------------------------

BBitmap*
MSyntaxStyleView::CreateColorBitmap(const BPoint&)
{
	// create a little clone of our color box

	BRect rect(fColorBox);
	rect.InsetBy(2.0, 2.0);
	rect.OffsetTo(BPoint(0.0, 0.0));
	BBitmap* bitmap = new BBitmap(rect, B_COLOR_8_BIT, true);
	bitmap->Lock();
	BView* drawingView = new BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW);
	bitmap->AddChild(drawingView);
	this->DrawColorBox(rect, drawingView);
	drawingView->Sync();
	bitmap->Unlock();
	
	return bitmap;
}

// ---------------------------------------------------------------------------

BBitmap*
MSyntaxStyleView::CreateFontBitmap()
{
	// create a clone of the font/size area
	
	BRect rect(fFontBox);
	rect.OffsetTo(BPoint(0.0, 0.0));
	BBitmap* bitmap = new BBitmap(rect, B_COLOR_8_BIT, true);
	bitmap->Lock();
	BView* drawingView = new BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW);
	bitmap->AddChild(drawingView);
	this->DrawFontBox(rect, drawingView);
	drawingView->Sync();
	bitmap->Unlock();
	
	return bitmap;
}

