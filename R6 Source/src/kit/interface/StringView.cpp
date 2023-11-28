//*****************************************************************************
//
//	File:		StringView.cpp
//
//	Description:	BStringView class.
//
//	Written By:	Peter Potrebic
//
//	Copyright 1993-6, Be Incorporated
//
//*****************************************************************************

#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <Message.h>
#include <StringView.h>

#include "stdio.h"

#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif

enum {
	B_TRUNCATION_OFF = 0xffff
};

/*---------------------------------------------------------------*/

BStringView::BStringView(BRect bounds, const char *name, const char *text,
		uint32 flags, uint32 drawFlags)
	: BView(bounds, name, flags, drawFlags|B_WILL_DRAW)
{
	SetText(text);
	fAlign = B_ALIGN_LEFT;
	fTruncationMode = B_TRUNCATION_OFF;
	fValidTruncation = true;
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	
/*	BFont font(*be_plain_font);
	font.SetSpacing(B_STRING_SPACING);
	SetFont(&font); */
}

/*---------------------------------------------------------------*/

BStringView::BStringView(BRect bounds, const char *name, const char *text,
		uint32 truncation_mode, uint32 flags, uint32 drawFlags)
	: BView(bounds, name, flags, drawFlags|B_FRAME_EVENTS|B_WILL_DRAW)
{
	SetText(text);
	fAlign = B_ALIGN_LEFT;
	fTruncationMode = truncation_mode;
	fValidTruncation = false;
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	
/*	BFont font(*be_plain_font);
	font.SetSpacing(B_STRING_SPACING);
	SetFont(&font); */
}

/*---------------------------------------------------------------*/

BStringView::~BStringView()
{
}

/* ---------------------------------------------------------------- */

BStringView::BStringView(BMessage *data)
	: BView(data)
{
	if (data->HasString(S_TEXT)) {
		const char *str;
		data->FindString(S_TEXT, &str);
		SetText(str);
	}

	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	long l;
	if (data->FindInt32(S_ALIGN, &l) != B_OK)
		l = B_ALIGN_LEFT;
	SetAlignment((alignment) l);
}

/* ---------------------------------------------------------------- */

status_t BStringView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	if (Text())
		data->AddString(S_TEXT, Text());
	data->AddInt32(S_ALIGN, Alignment());
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BStringView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BStringView"))
		return NULL;
	return new BStringView(data);
}

/*------------------------------------------------------------*/

void	BStringView::AttachedToWindow()
{
	SetColorsFromParent();
	fValidTruncation = false;
}

/*---------------------------------------------------------------*/

void BStringView::Draw(BRect)
{
	BRect		bounds = Bounds();
	BFont       font;
	font_height	fheight;
	edge_info	eInfo;

	const char* str = GetShownText();
	
	if (str && *str) {
		BPoint	loc;

		GetFontHeight(&fheight);

		switch (fAlign) {
			case B_ALIGN_LEFT:
				// If the first char has a negative left edge give it
				// some more room by shifting that much more to the right.
			    GetFont(&font);
				font.GetEdges(str, 1, &eInfo);
				loc.x = bounds.left + (2 - eInfo.left);
				break;
			case B_ALIGN_CENTER:
				{
				float width = StringWidth(str);
				float center = (bounds.right - bounds.left) / 2;
				loc.x = center - (width/2);
				break;
				}
			case B_ALIGN_RIGHT:
				{
				float width = StringWidth(str);
				loc.x = bounds.right - width - 2;
				break;
				}
		}
		loc.y = bounds.bottom - (1 + fheight.descent);
		MovePenTo(loc);
		DrawString(str);
	}
}

/*---------------------------------------------------------------*/

void BStringView::SetText(const char *text)
{
	if (fText != text) {
		fText = text;
		fValidTruncation = false;
		Invalidate();
	}
}

/*---------------------------------------------------------------*/

void BStringView::SetAlignment(alignment align)
{
	if (fAlign != align) {
		fAlign = align;
		Invalidate();
	}
}

/*---------------------------------------------------------------*/

alignment BStringView::Alignment() const
	{ return fAlign; }

/*---------------------------------------------------------------*/

const char *BStringView::Text() const
	{ return fText.String(); }

/*---------------------------------------------------------------*/

void BStringView::SetTruncation(uint32 mode)
{
	if (fTruncationMode != mode) {
		fTruncationMode = (uint16)mode;
		fValidTruncation = false;
		if ((Flags()&B_FRAME_EVENTS) == 0) {
			SetFlags(Flags()|B_FRAME_EVENTS);
		}
	}
}

/*---------------------------------------------------------------*/

void BStringView::ClearTruncation()
{
	SetTruncation(B_TRUNCATION_OFF);
}

/*---------------------------------------------------------------*/

uint32 BStringView::Truncation()
{
	if (fTruncationMode != B_TRUNCATION_OFF) return fTruncationMode;
	return 0;
}

/*---------------------------------------------------------------*/

bool BStringView::HasTruncation()
{
	return fTruncationMode != B_TRUNCATION_OFF;
}
		
/*---------------------------------------------------------------*/

const char *BStringView::GetShownText()
{
	if (!HasTruncation()) return fText.String();
	if (fValidTruncation) {
		if (fTruncatedText.Length() > 0) return fTruncatedText.String();
		return fText.String();
	}
	
	fValidTruncation = true;
	
	// Figure out whether string fits, and truncate if not.
	const float width = StringWidth(fText.String());
	if (width <= Bounds().Width()) {
		fTruncatedText = "";
		return fText.String();
	}
	
	BFont font;
	GetFont(&font);
	const char* orig = fText.String();
	font.GetTruncatedStrings(&orig, 1, Truncation(),
							 Bounds().Width(), &fTruncatedText);
	
	return fTruncatedText.String();
}

/*---------------------------------------------------------------*/

BStringView &BStringView::operator=(const BStringView &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BStringView::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*----------------------------------------------------------------*/

status_t BStringView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BStringView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BStringView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BStringView::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BStringView::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BStringView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void	BStringView::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BStringView::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
	fValidTruncation = false;
}

/*---------------------------------------------------------------*/

void BStringView::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BStringView::GetPreferredSize(float *width, float *height)
{
	// it would be better to cache these values and change them
	// only when the text or font changes, but we don't know when the
	// font changes, so we have to calculate every time.
	font_height fh;
	GetFontHeight(&fh);
	*width = ceil(StringWidth(fText.String()) + 4);
	*height = ceil(fh.ascent + fh.descent + fh.leading + 2);
}

/*---------------------------------------------------------------*/

void BStringView::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BStringView::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void BStringView::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

/*---------------------------------------------------------------*/

status_t BStringView::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BView::GetSupportedSuites(data);
}

/*---------------------------------------------------------------*/

void	BStringView::SetFont(const BFont *font, uint32 mask)
{
	/*
	 This function/override did not exist in R5. So if adding functionality
	 here must consider the implications of that fact.  It is added now
	 because we don't want to lose B_STRING_SPACING if the programmer
	 changes the font.  (But losing it with old programs isn't that big a
	 deal.)
	*/
	BView::SetFont(font, mask/*&~B_FONT_SPACING*/);
}


void BStringView::_ReservedStringView1() {}
void BStringView::_ReservedStringView2() {}
void BStringView::_ReservedStringView3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
