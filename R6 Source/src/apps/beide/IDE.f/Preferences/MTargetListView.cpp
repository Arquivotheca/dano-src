//==================================================================
//	MTargetListView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include "MTargetListView.h"
#include "MTargetTypes.h"
#include "CString.h"
#include "Utils.h"

const float kFileTypeLeft = 5.0;
const float kExtensionLeft = kFileTypeLeft + 160.0;
const float kMakeStageLeft = kExtensionLeft + 50.0;
const float kHasResourcesLeft = kMakeStageLeft + 60.0;
const float kToolNameLeft = kHasResourcesLeft + 35.0;


// ---------------------------------------------------------------------------
//		MTargetListView
// ---------------------------------------------------------------------------
//	Constructor

MTargetListView::MTargetListView(
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
}

// ---------------------------------------------------------------------------
//		~MTargetListView
// ---------------------------------------------------------------------------
//	Destructor

MTargetListView::~MTargetListView()
{
	// Need to delete it here because our DeleteItem won't be called
	// from the MListView destructor
	if (CountRows() > 0)
		DeleteRows(0, CountRows());
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

void
MTargetListView::DrawRow(
	int32 	/*inRow*/,
	void * 	inData,
	BRect	inArea,
	BRect	/*inIntersection*/)
{
	TargetRec*		rec = (TargetRec*) inData;

	if (rec)
	{
		float		bottom = inArea.bottom - GetCachedFontHeight().descent;

		// Draw the text
		// FileType
		if (rec->MimeType[0] != '\0')
		{
			MovePenTo(kFileTypeLeft, bottom);
			DrawString(rec->MimeType);
		}

		// Extension
		if (rec->Extension[0] != 0)
		{
			fText = ".";
			fText += rec->Extension;
			MovePenTo(kExtensionLeft, bottom);
			DrawString(fText);
		}

		// Make stage
		if (rec->Stage == ignoreStage)
			fText = "-";
		else
			fText = rec->Stage;
			
		MovePenTo(kMakeStageLeft, bottom);
		DrawString(fText);
		
		// Has resources flag
		if (TargetHasResources(rec->Flags))
		{
			MovePenTo(kHasResourcesLeft, bottom);
			DrawString("+");
		}
		
		// Tool Name
		if (rec->ToolName[0] != 0)
		{
			MovePenTo(kToolNameLeft, bottom);
			DrawString(rec->ToolName);
		}
	}
}
