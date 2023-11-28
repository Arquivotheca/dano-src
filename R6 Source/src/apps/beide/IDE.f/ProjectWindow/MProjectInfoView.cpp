//========================================================================
//	MProjectInfoView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MProjectInfoView.h"
#include "MPathPopup.h"
#include "MProjectWindow.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
//		MProjectInfoView
// ---------------------------------------------------------------------------

MProjectInfoView::MProjectInfoView(
	MProjectWindow&	inWindow,
	BRect 		inArea,
	const char *text,
	uint32		resizeMask) :
	BStringView(
		inArea,
		"infoview",
		text,
		resizeMask,
		B_WILL_DRAW),
		fWindow(inWindow)
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MProjectInfoView::Draw(
	BRect /*area*/)
{
	BRect		bounds = Bounds();

	BeginLineArray(6);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kGrey152);
	bounds.top++;
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), white);
	AddLine(bounds.LeftTop(), bounds.RightTop(), white);
	AddLine(bounds.RightTop(), bounds.RightBottom(), kGrey152);
	AddLine(bounds.RightBottom(), bounds.LeftBottom(), kGrey176);
	AddLine(bounds.LeftBottom(), bounds.LeftBottom(), kMenuBodyGray);
	EndLineArray();
	
	BPoint		where(4.0, bounds.bottom - 2.0);
	DrawString(Text(), where);
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------

void
MProjectInfoView::MouseDown(
	BPoint /*inWhere*/)
{
	entry_ref		ref;
	
	if (B_NO_ERROR == fWindow.GetRef(ref))
	{
		MPathPopup		popup("", ref);
		BPoint			where(0.0, fWindow.Bounds().bottom + 3.0);
		fWindow.ConvertToScreen(&where);
	
		// Show the popup
		BMenuItem*		item = popup.Go(where, true);
		if (item)
		{
			popup.OpenItem(item);
		}
	}
}

// ---------------------------------------------------------------------------
//		MStatusInfoView member functions
// ---------------------------------------------------------------------------

MStatusInfoView::MStatusInfoView(BRect bounds,
					const char *name, 
					const char *text,
					uint32 resizeFlags,
					uint32 flags)
				: BStringView(bounds, name, text, resizeFlags, flags)
{
}

// ---------------------------------------------------------------------------

void
MStatusInfoView::Draw(BRect area)
{
	BRect bounds = Bounds();
	if (area.top <= bounds.top) {
		BeginLineArray(1);
		AddLine(bounds.LeftTop(), bounds.RightTop(), kGrey152);
		EndLineArray();
	}
	
	// 6.0 - just a little more indented than file count above, but 8 seemed
	// too much
	
	BPoint where(6.0, bounds.bottom - 2.0);
	DrawString(Text(), where);
}
