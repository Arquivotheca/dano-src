//========================================================================
//	MMessageInfoView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MMessageInfoView.h"
#include "Utils.h"

#include <StringView.h>
#include <Box.h>
#include <Font.h>
#include <TextControl.h>

#include <stdio.h>
#include <string.h>


// ---------------------------------------------------------------------------
// MMessageInfoView member functions
// ---------------------------------------------------------------------------

const float kInfoLength = 400.0;

MMessageInfoView::MMessageInfoView(const char* infoString, const BRect& inArea, const BPoint& titleLocation, uint32 resizeMask)
				 : BView(inArea, "infoview", resizeMask, B_WILL_DRAW)
{
	SetGrey(this, kLtGray);
	this->SetHighColor(kMenuDarkHilite);

	BRect frame(titleLocation.x, titleLocation.y, titleLocation.x+kInfoLength, titleLocation.y+15.0);
	char* truncatedString = new char[strlen(infoString) + 3];
	be_fixed_font->GetTruncatedStrings(&infoString, 1, B_TRUNCATE_MIDDLE, kInfoLength, &truncatedString);
	MMessageInfoTitleView* textBox = new MMessageInfoTitleView(frame, "title", truncatedString);
	delete [] truncatedString;
	
	textBox->SetFont(be_fixed_font);
	this->AddChild(textBox);
}

// ---------------------------------------------------------------------------

MMessageInfoView::MMessageInfoView(BRect inArea, uint32 resizeMask)
				 : BView(inArea, "infoview", resizeMask, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------

void
MMessageInfoView::Draw(BRect inArea)
{
	BRect bounds = Bounds();

	if (inArea.bottom >= bounds.bottom) {
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	}
}

// ---------------------------------------------------------------------------
//	MMessageInfoTitleView member functions
// ---------------------------------------------------------------------------

MMessageInfoTitleView::MMessageInfoTitleView(BRect bounds, const char *name, const char *text)
					  : BStringView(bounds, name, text)
{
}
	
// ---------------------------------------------------------------------------

void
MMessageInfoTitleView::AttachedToWindow()
{
	SetGrey(this, kMenuBodyGray);
}

// ---------------------------------------------------------------------------

void
MMessageInfoTitleView::Draw(BRect inArea)
{
	BStringView::Draw(inArea);

	BRect bounds = this->Bounds();
	BeginLineArray(4);
	AddLine(bounds.LeftBottom(), bounds.RightBottom(), white);
	AddLine(bounds.RightBottom(), bounds.RightTop(), white);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kGrey152);
	AddLine(bounds.LeftTop(), bounds.LeftBottom(), kGrey152);
	EndLineArray();
}
