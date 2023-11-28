//========================================================================
//	MPictureMenuField.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MPictureMenuField.h"

#include <Picture.h>
#include <PopUpMenu.h>

const float kTopMarginPM = 2.0;
const float kLeftMarginPM = 4.0;

// ---------------------------------------------------------------------------
//		MPictureMenuField
// ---------------------------------------------------------------------------
//	Constructor

MPictureMenuField::MPictureMenuField(
	BRect			inFrame,
	const char *	title,
	const char *	inLabel,
	BPicture*		inPicture,
	BPopUpMenu*		inMenu,
	uint32 			resizeMask)
	: BView(inFrame, title, resizeMask, B_WILL_DRAW | B_NAVIGABLE),
	fPicture(inPicture),
	fMenu(inMenu)
{
	fDivider = inFrame.Width() / 2.0f;
	fRect = inFrame;
	fRect.left = fDivider;
	fLabel = nil;
	
	SetLabel(inLabel);
}

// ---------------------------------------------------------------------------
//		~MPictureMenuField
// ---------------------------------------------------------------------------
//	Destructor

MPictureMenuField::~MPictureMenuField()
{
	delete[] fLabel;
	delete fPicture;
	delete fMenu;
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------
//	Draw the correct picture depending on whether we're enabled or disabled.

void
MPictureMenuField::Draw(
	BRect 			inFrame)
{
	// Draw the label
	DrawLabel(inFrame);

	// Draw the picture
	if (inFrame.Intersects(fRect))
	{
		if (fPicture != nil)
		{
			BPoint			where(fDivider + kTopMarginPM, kTopMarginPM);		
			DrawPicture(fPicture, where);
		}
	
		// If focused draw the box around the picture menu bar
		if (IsFocus())
		{
			DrawBorder(true);
		}
	}
}

// ---------------------------------------------------------------------------
//		DrawLabel
// ---------------------------------------------------------------------------
//	Draw the label.

void
MPictureMenuField::DrawLabel(
	BRect 			inFrame)
{
	// Draw the label
	if (fLabel != nil && inFrame.left <= fDivider)
	{
		font_height		info;
		GetFontHeight(&info);
		info.ascent = ceil(info.ascent);
		info.leading = ceil(info.leading);

		BPoint			where(kLeftMarginPM, info.ascent + info.leading + kTopMarginPM);
		DrawString(fLabel, where);
	}
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------

void
MPictureMenuField::MouseDown(
	BPoint /*where*/)
{
	ShowPopup();
}

// ---------------------------------------------------------------------------
//		ShowPopup
// ---------------------------------------------------------------------------

void
MPictureMenuField::ShowPopup()
{
	if (fMenu)
	{
		const rgb_color		fillGrey = { 160, 160, 160, 255 };
		BRect				bounds = Bounds();
	
		// Fill label with grey background
		rgb_color	highColor = HighColor();
		rgb_color	lowColor = LowColor();
		BRect		back = bounds;
		back.right = fDivider - 1;

		font_height		info;
		GetFontHeight(&info);
		info.ascent = ceil(info.ascent);
		info.descent = ceil(info.descent);
		info.leading = ceil(info.leading);

		back.bottom = info.ascent+info.descent+info.leading+kTopMarginPM;
		SetHighColor(fillGrey);
		FillRect(back);
		SetHighColor(highColor);
		SetLowColor(fillGrey);
		DrawLabel(bounds);
		SetLowColor(lowColor);

		// Start up the popup menu
		// Always open the popup at the same spot
		BRect		clickToOpenRect = bounds;
		BPoint		where(fDivider + 2.0, 2.0);		
		ConvertToScreen(&where);
		clickToOpenRect.right = fDivider;
		ConvertToScreen(&clickToOpenRect);
		
		bool		isFocus = IsFocus();
		if (isFocus)
		{
			DrawBorder(false);			// hide the border if it's there
			Sync();						// else it doesn't go away (?!)
		}

		fMenu->Go(where, true, true, clickToOpenRect);
		
		if (isFocus)
			DrawBorder(true);			// reshow the border

		// Remove the grey background
		FillRect(back, B_SOLID_LOW);
		DrawLabel(bounds);
	}
}

// ---------------------------------------------------------------------------
//		MakeFocus
// ---------------------------------------------------------------------------

void
MPictureMenuField::MakeFocus(
	bool inBecomeFocus)
{
	DrawBorder(inBecomeFocus);			

	BView::MakeFocus(inBecomeFocus);
}

// ---------------------------------------------------------------------------
//		DrawBorder
// ---------------------------------------------------------------------------

void
MPictureMenuField::DrawBorder(
	bool inShowBorder)
{
	rgb_color	highcolor = HighColor();
	if (inShowBorder)
	{
		SetHighColor(keyboard_navigation_color());		// frame color
	}
	else
	{
		rgb_color	color;
		BView*	parent = Parent();
		if (parent)
			color = parent->LowColor();
		SetHighColor(color);			// parent's low color					
	}
	
	StrokeRect(fRect);
	SetHighColor(highcolor);			// restorecolor					
}

// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------

void
MPictureMenuField::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	if (IsFocus())
	{
		switch (inBytes[0])
		{
			case ' ':
			case B_ENTER:
				ShowPopup();
				break;
		}
	}
	
	BView::KeyDown(inBytes, inNumBytes);
}

// ---------------------------------------------------------------------------
//		SetDivider
// ---------------------------------------------------------------------------

void
MPictureMenuField::SetDivider(
	float inDividingLine)
{
	fDivider = inDividingLine;
	fRect.OffsetTo(inDividingLine, 0.0);
}

// ---------------------------------------------------------------------------
//		SetLabel
// ---------------------------------------------------------------------------

void
MPictureMenuField::SetLabel(
	const char * inLabel)
{
	delete[] fLabel;

	if (inLabel != nil)
	{
		int32		len = strlen(inLabel) + 1;
		fLabel = new char[len];
		memcpy(fLabel, inLabel, len);
	}
	else
		fLabel = nil;
}

// ---------------------------------------------------------------------------
//		SetPictureFrame
// ---------------------------------------------------------------------------
//	Specify the bounding rect for the picture.

void
MPictureMenuField::SetPictureFrame(
	BRect	inRect)
{
	fRect = inRect;
	fRect.right += 3.0;
	fRect.bottom += 3.0;
	fRect.OffsetTo(fDivider, 0.0);
}

