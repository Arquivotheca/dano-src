//==================================================================
//	MMultiFileListView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include "MMultiFileListView.h"
#include "MSourceFile.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "stDrawingMode.h"
#include <Bitmap.h>
#include <Application.h>

const float kNameLeftMargin = 12.0;
const float kBlueArrowHeight = 9.0;
const float kBlueArrowWidth = 9.0;

#define _ 0xff
#define B 0
#define u 0x6c

const char blueArrowData[] = {
	_,_,_,B,B,_,_,_,_,
	_,_,_,B,u,B,_,_,_,
	_,B,B,B,u,u,B,_,_,
	_,B,u,u,u,u,u,B,_,
	_,B,u,u,u,u,u,B,B,
	_,B,u,u,u,u,u,B,_,
	_,B,B,B,u,u,B,_,_,
	_,_,_,B,u,B,_,_,_,
	_,_,_,B,B,_,_,_,_,
};

#undef _
#undef B
#undef u

// ---------------------------------------------------------------------------
//		MMultiFileListView
// ---------------------------------------------------------------------------
//	Constructor

MMultiFileListView::MMultiFileListView(
	BRect			inFrame,
	const char*		inName,
	uint32 			resizeMask,
	uint32 			flags)
	: MDLOGListView(
		inFrame,
		inName,
		resizeMask,
		flags)
{
	SetMultiSelect(false);
	fBlueRow = -1;
	fBlueArrow = LoadBitmap(blueArrowData, kBlueArrowWidth, kBlueArrowHeight);
}

// ---------------------------------------------------------------------------
//		~MMultiFileListView
// ---------------------------------------------------------------------------
//	Destructor

MMultiFileListView::~MMultiFileListView()
{
	delete fBlueArrow;
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

void
MMultiFileListView::DrawRow(
	int32 	inRow,
	void * 	inData,
	BRect	inArea,
	BRect	/*inIntersection*/)
{
	MSourceFile*		sourceFile = (MSourceFile*) inData;

	if (sourceFile)
	{
		font_height		info = GetCachedFontHeight();
		MovePenTo(kNameLeftMargin, inArea.bottom - info.descent);

		ASSERT(sourceFile->GetFileName() != nil);
		DrawString(sourceFile->GetFileName());
	}
	
	if (inRow == fBlueRow)
	{
		stDrawingMode		mode(this, B_OP_OVER);
		DrawBitmap(fBlueArrow, inArea.LeftTop());	
	}
}

// ---------------------------------------------------------------------------
//		MouseMoved
// ---------------------------------------------------------------------------
//	Make ourself the focus when we're about to be dragged onto

void
MMultiFileListView::MouseMoved(
	BPoint				/*where*/,
	uint32				code,
	const BMessage *	inMessage)
{
	if (code == B_INSIDE_VIEW && inMessage != nil && inMessage->HasRef("refs"))
	{
		MakeFocus();
	}
}

// ---------------------------------------------------------------------------
//		InvokeRow
// ---------------------------------------------------------------------------

void
MMultiFileListView::InvokeRow(
	int32	inRow)
{
	MSourceFile*		sourceFile = (MSourceFile*) GetList()->ItemAt(inRow);
	ASSERT(sourceFile);
	
	entry_ref		ref;
	
	if (B_NO_ERROR == sourceFile->GetRef(ref))
	{
		BMessage		msg(msgOpenSourceFile);
		
		msg.AddRef("refs", &ref);
		be_app_messenger.SendMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		SetBlueRow
// ---------------------------------------------------------------------------

void
MMultiFileListView::SetBlueRow(
	int32	inRow)
{
	if (inRow != fBlueRow)
	{
		BRect		rowRect;
		
		if (fBlueRow >= 0 && fBlueRow < CountRows())
		{
			GetRowRect(fBlueRow, &rowRect);
			rowRect.right = kBlueArrowWidth;
			FillRect(rowRect, B_SOLID_LOW);
		}
		
		if (CountRows() == 0)
			fBlueRow = -1;
		else
		if (inRow >= CountRows())
			fBlueRow = CountRows() - 1;
		else
			fBlueRow = inRow;
		
		if (fBlueRow >= 0)
		{
			GetRowRect(fBlueRow, &rowRect);

			if (! Bounds().Intersects(rowRect))
				ScrollRowIntoView(inRow);

			stDrawingMode		mode(this, B_OP_OVER);
			DrawBitmap(fBlueArrow, rowRect.LeftTop());
		}
	}
}

// ---------------------------------------------------------------------------
//		ClickHook
// ---------------------------------------------------------------------------

bool
MMultiFileListView::ClickHook(
	BPoint	inWhere,
	int32	inRow,
	uint32 /* modifiers */,
	uint32 /* buttons */)
{
	if (inWhere.x <= kNameLeftMargin)
	{
		SetBlueRow(inRow);
		PostToTarget(msgBlueRowChanged);

		return true;
	}
	else
		return false;
}

// ---------------------------------------------------------------------------
//		DeleteItem
// ---------------------------------------------------------------------------

void
MMultiFileListView::DeleteItem(
	void * /*inItem*/)
{
	ASSERT(false);
}

// ---------------------------------------------------------------------------
//		SetEnabled
// ---------------------------------------------------------------------------

void
MMultiFileListView::SetEnabled(
	bool 	inEnabled)
{
	if (inEnabled != IsEnabled())
	{
		MListView::SetEnabled(inEnabled);
		
		if (inEnabled)
			SetHighColor(black);
		else
			SetHighColor(kGrey136);
		Invalidate();	
	}
}
