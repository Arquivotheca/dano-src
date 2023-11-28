//========================================================================
//	MSectionLine.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS


#include "MSectionLine.h"
#include "MProjectView.h"
#include "MSourceFileLine.h"
#include "MChooseNameWindow.h"
#include "MPopupMenu.h"
#include "MHiliteColor.h"
#include "IDEConstants.h"
#include "IDEMessages.h"

#include <string.h>
#include <Bitmap.h>
#include <Menu.h>
#include <MenuItem.h>

const bigtime_t kAnimationDelay = 100000; // 100 ms

BBitmap * MSectionLine::sContractedBitmap;
BBitmap * MSectionLine::sIntermediateBitmap;
BBitmap * MSectionLine::sExpandedBitmap;

const int32 kArrowBitMapWidth = 8;
const int32 kArrowBitMapHeight = 12;

#define _ 0xff
#define X 0x26
#define x 0x6c

static char sContractedData[] = {
	_,_,_,_,_,_,_,_,
	_,_,X,_,_,_,_,_,
	_,_,X,X,_,_,_,_,
	_,_,X,x,X,_,_,_,
	_,_,X,x,x,X,_,_,
	_,_,X,x,x,X,_,_,
	_,_,X,x,X,_,_,_,
	_,_,X,X,_,_,_,_,
	_,_,X,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
};

static char sIntermediateData[] = {
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,X,_,_,
	_,_,_,_,X,X,_,_,
	_,_,_,X,x,X,_,_,
	_,_,X,x,x,X,_,_,
	_,X,x,x,x,X,_,_,
	X,X,X,X,X,X,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
};

static char sExpandedData[] = {
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	X,X,X,X,X,X,X,X,
	_,X,x,x,x,x,X,_,
	_,_,X,x,x,X,_,_,
	_,_,_,X,X,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
};

#undef _
#undef X
#undef x

// ---------------------------------------------------------------------------
//		MSectionLine
// ---------------------------------------------------------------------------
//	Constructor for new section

MSectionLine::MSectionLine(
	MProjectView& 	inProjectView, 
	BList& 			inFileList)
	: MProjectLine(*this, inProjectView),
		fFileList(inFileList),
		fName("New Group")
{
	InitBitmaps();

	fExpanded = true;
	fSection = this;
	fLinesInSection = 0;
	fFirstLineInSection = nil;
}

// ---------------------------------------------------------------------------
//		MSectionLine
// ---------------------------------------------------------------------------
//	Constructor for existing section

MSectionLine::MSectionLine(
	MProjectView& 	inProjectView, 
	BList& 			inFileList,
	MBlockFile&		inFile)
	: MProjectLine(*this, inProjectView),
		fFileList(inFileList)
{
	InitBitmaps();

	SectionBlock		sectionBlock;

	inFile.GetBytes(sizeof(sectionBlock), &sectionBlock);

	sectionBlock.SwapBigToHost();
	fCodeSize = sectionBlock.sCodeSize;
	fDataSize = sectionBlock.sDataSize;
	fExpanded = sectionBlock.sExpanded;
	fName = sectionBlock.sName;

	fLinesInSection = 0;
	fSection = this;
	fFirstLineInSection = nil;
}

// ---------------------------------------------------------------------------
//		~MSectionLine
// ---------------------------------------------------------------------------
//	Destructor

MSectionLine::~MSectionLine()
{
}

// ---------------------------------------------------------------------------
//		InitBitmaps
// ---------------------------------------------------------------------------

void
MSectionLine::InitBitmaps()
{
	ASSERT(sizeof(SectionBlock) == 80);
	if (sContractedBitmap)
		return;
	BRect bounds(0,0, kArrowBitMapWidth-1,kArrowBitMapHeight-1);

	sContractedBitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	char * 	ptr = sContractedData;
	int 	ix;
	for (ix = 0; ix < kArrowBitMapHeight; ix++)
	{
		sContractedBitmap->SetBits(ptr, kArrowBitMapWidth, sContractedBitmap->BytesPerRow()*ix, B_COLOR_8_BIT);
		ptr += kArrowBitMapWidth;
	}

	sIntermediateBitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	ptr = sIntermediateData;
	for (ix = 0; ix < kArrowBitMapHeight; ix++)
	{
		sIntermediateBitmap->SetBits(ptr, kArrowBitMapWidth, sIntermediateBitmap->BytesPerRow()*ix, B_COLOR_8_BIT);
		ptr += kArrowBitMapWidth;
	}

	sExpandedBitmap = new BBitmap(bounds, B_COLOR_8_BIT);
	ptr = sExpandedData;
	for (ix = 0; ix < kArrowBitMapHeight; ix++)
	{
		sExpandedBitmap->SetBits(ptr, kArrowBitMapWidth, sExpandedBitmap->BytesPerRow()*ix, B_COLOR_8_BIT);
		ptr += kArrowBitMapWidth;
	}
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MSectionLine::Draw(
	BRect 			inFrame, 
	BRect			/*inIntersection*/,
	MProjectView& 	inView)
{
	// Draw the section name
	inView.SetFont(be_bold_font);
	inView.MovePenTo(inFrame.left + kTriangleWidth, inFrame.bottom - kSourceBottomMargin);
	inView.DrawString(fName);

	// Draw the arrow button
	fProjectView.GetPainter().DrawBitmap(MProjectLinePainter::kDownArrow, inFrame, inView);
	
	// Draw the triangle
	if (fExpanded)
	{
		DrawExpanded(inFrame, inView);
	}
	else
	{
		DrawContracted(inFrame, inView);
	}
}

// ---------------------------------------------------------------------------
//		DrawContracted
// ---------------------------------------------------------------------------

void
MSectionLine::DrawContracted(BRect inFrame, MProjectView& inView)
{
	inView.SetDrawingMode(B_OP_OVER);
	inView.DrawBitmap(sContractedBitmap, inFrame.LeftTop());
	inView.SetDrawingMode(B_OP_COPY);
}

// ---------------------------------------------------------------------------
//		DrawExpanded
// ---------------------------------------------------------------------------

void
MSectionLine::DrawExpanded(BRect inFrame, MProjectView& inView)
{
	inView.SetDrawingMode(B_OP_OVER);
	inView.DrawBitmap(sExpandedBitmap, inFrame.LeftTop());
	inView.SetDrawingMode(B_OP_COPY);
}

// ---------------------------------------------------------------------------
//		DrawIntermediate
// ---------------------------------------------------------------------------

void
MSectionLine::DrawIntermediate(BRect inFrame)
{
	fProjectView.SetDrawingMode(B_OP_OVER);
	fProjectView.DrawBitmap(sIntermediateBitmap, inFrame.LeftTop());
	fProjectView.SetDrawingMode(B_OP_COPY);
}

// ---------------------------------------------------------------------------
//		AddLine
// ---------------------------------------------------------------------------
//	A new line is being added to this section.  It has already been added
//	to the listView.  This doesn't deal properly with adding lines to
//	a contracted section (although we could assume that adding to a contracted
//	section never adds at the beginning of the section).

void
MSectionLine::AddLine(
	MProjectLine* inLine)
{
	fLinesInSection++;
	inLine->SetSection(this);

	if (fFirstLineInSection == nil)
		fFirstLineInSection = inLine;
	else
	{
		int32		firstLineIndex = fFileList.IndexOf(fFirstLineInSection);
		int32		newLineIndex = fFileList.IndexOf(inLine);
		
		ASSERT(firstLineIndex >= 0 && newLineIndex >= 0);

		// Is it the first line in our section?
		if (newLineIndex <= firstLineIndex)
		{
			fFirstLineInSection = inLine;
		}
	}
}

// ---------------------------------------------------------------------------
//		RemoveLine
// ---------------------------------------------------------------------------
//	A line is being removed from this section.  It still exists in the listView.

void
MSectionLine::RemoveLine(
	MProjectLine* inLine)
{
	fLinesInSection--;
	
	if (inLine == fFirstLineInSection)
	{
		if (fLinesInSection > 0)
		{
			BList&		fileList = fProjectView.GetFileList();
			int32		newIndex = fileList.IndexOf(fFirstLineInSection);
			fFirstLineInSection = (MProjectLine*) fileList.ItemAt(newIndex + 1);
		}
		else
		{
			fFirstLineInSection = nil;

			// Tell the projectView to remove and delete this section if it's not the first
			fProjectView.KillSection(this);
		}
	}
}

// ---------------------------------------------------------------------------
//		DoClick
// ---------------------------------------------------------------------------

bool
MSectionLine::DoClick(
	BRect		inFrame,
	BPoint		inPoint,
	bool 		inIsSelected,
	uint32		/*inModifiers*/,
	uint32		/*inButtons*/)
{
	BRect		triangleRect(inFrame.left, inFrame.top, inFrame.left + kTriangleWidth, inFrame.bottom);
	BPoint		hit = inPoint;
	hit.y -= inFrame.top;
	
	BRect arrowRect = fProjectView.GetPainter().GetArrowRect();
	// In the arrow button?
	if (arrowRect.Contains(hit))
	{
		MPopupMenu headerPopup("header");
		
		BuildPopupMenu(headerPopup);
		headerPopup.SetTargetForItems(fProjectView.Window());
		
		BPoint where = arrowRect.RightTop();
		
		where.x += 3.0;
		where.y += inFrame.top;
		fProjectView.ConvertToScreen(&where);
	
		// Show the popup
		BMenuItem* item = headerPopup.Go(where, true);
	}
	// Is the click in the triangle?
	else
	if (triangleRect.Contains(inPoint))
	{
		if (fExpanded)
			Contract(inFrame, inIsSelected);
		else
			Expand(inFrame, inIsSelected);

		fProjectView.Invalidate(inFrame);

		fExpanded = ! fExpanded;
		fProjectView.SetDirty();
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
//		DoArrowKey
// ---------------------------------------------------------------------------
//	Handle a command right arrow or command left arrow to expand or contract
//	the section if it isn't already in that state.  returns true if the section
//	changed.  inArrowMessage is one of msgCommandRightArrow, msgCommandLeftArrow,
//	defined in IDEMessages.h.

bool
MSectionLine::DoArrowKey(
	BRect	inFrame,
	uint32	inArrowMessage,
	bool	inIsSelected)
{
	bool	changed = false;

	switch (inArrowMessage)
	{
		case msgCommandRightArrow:
			if (! fExpanded)
			{
				Expand(inFrame, inIsSelected);
				changed = true;
			}
			break;
			
		case msgCommandLeftArrow:
			if (fExpanded)
			{
				Contract(inFrame, inIsSelected);
				changed = true;			
			}
			break;
	}
	
	if (changed)
	{
		fExpanded = ! fExpanded;
		fProjectView.Invalidate(inFrame);
		fProjectView.SetDirty();	
	}
	
	return changed;
}

// ---------------------------------------------------------------------------
//		SelectImmediately
// ---------------------------------------------------------------------------

bool
MSectionLine::SelectImmediately(
	BRect	inFrame,
	BPoint	inPoint,
	bool	/*inIsSelected*/,
	uint32	/*inModifiers*/,
	uint32	/*inButtons*/)
{
	inPoint.y -= inFrame.top;

	return fProjectView.GetPainter().PointInArrow(inPoint);
}

// ---------------------------------------------------------------------------
//		Invoke
// ---------------------------------------------------------------------------
//	Show a dialog to allow setting of the section's name.

void
MSectionLine::Invoke()
{
	MChooseNameWindow*		wind = new MChooseNameWindow(*fProjectView.Window(), *this, fName);
	
	wind->Show();
}

// ---------------------------------------------------------------------------
//		BuildPopupMenu
// ---------------------------------------------------------------------------
//	Build a header popup menu for this section line.

void
MSectionLine::BuildPopupMenu(
	MPopupMenu & 	inMenu) const
{
	BMessage*		msg;
	String			text;
	int32			index = fFileList.IndexOf(fFirstLineInSection);
	int32			last = index + fLinesInSection;
	
	for (int32 i = index; i < last; i++)
	{
		MSourceFileLine*	sourceFile = (MSourceFileLine*) fFileList.ItemAt(i);
		
		text = sourceFile->GetFileName();
		// Add the menu item to the menu
		entry_ref		ref;

		if (B_NO_ERROR == sourceFile->GetSourceFile()->GetRef(ref))
		{
			msg = new BMessage(msgOpenSourceFile);
			msg->AddRef("refs", &ref);
			inMenu.AddItem(new BMenuItem(text, msg));
		}
	}
}

// ---------------------------------------------------------------------------
//		Contract
// ---------------------------------------------------------------------------
//	Remove our lines from the list view.

void
MSectionLine::Contract(BRect inFrame, bool inIsSelected)
{
	rgb_color	lowColor;
	BRect		bitmapFrame(inFrame.left, inFrame.top, 
		inFrame.left+kArrowBitMapWidth, inFrame.top+kArrowBitMapHeight);

	if (inIsSelected)
	{
		lowColor = fProjectView.LowColor();
		fProjectView.SetLowColor(HiliteColor());
	}
	
	fProjectView.Sync();
	fProjectView.FillRect(bitmapFrame, B_SOLID_LOW);
	DrawIntermediate(inFrame); 
	fProjectView.Sync();
	snooze(kAnimationDelay);
	fProjectView.FillRect(bitmapFrame, B_SOLID_LOW);
	DrawContracted(inFrame, fProjectView); 
	fProjectView.Sync();
	if (inIsSelected)
		fProjectView.SetLowColor(lowColor);
	
	int32		firstIndex = fProjectView.IndexOf((void *) this) + 1;
	int32		lastIndex = firstIndex + fLinesInSection - 1;

	fProjectView.RemoveRows(firstIndex, fLinesInSection);
}

// ---------------------------------------------------------------------------
//		Expand
// ---------------------------------------------------------------------------
//	Add our lines from the fileList to the listView.

void
MSectionLine::Expand(BRect inFrame, bool inIsSelected)
{
	rgb_color	lowColor;
	BRect		bitmapFrame(inFrame.left, inFrame.top, 
		inFrame.left+kArrowBitMapWidth, inFrame.top+kArrowBitMapHeight);

	if (inIsSelected)
	{
		lowColor = fProjectView.LowColor();
		fProjectView.SetLowColor(HiliteColor());
	}
	fProjectView.Sync();
	fProjectView.FillRect(bitmapFrame, B_SOLID_LOW);
	DrawIntermediate(inFrame);
	fProjectView.Sync();
	snooze(kAnimationDelay);
	fProjectView.FillRect(bitmapFrame, B_SOLID_LOW);
	DrawExpanded(inFrame, fProjectView);
	fProjectView.Sync();
	if (inIsSelected)
		fProjectView.SetLowColor(lowColor);

	int32		ourIndex = fProjectView.IndexOf((void *) this);
	int32		lastIndex = ourIndex + fLinesInSection;
	int32		fileListIndex = fFileList.IndexOf((void *) fFirstLineInSection);

	for (int32 i = ourIndex + 1; i <= lastIndex; i++)
	{
		void*		item = fFileList.ItemAt(fileListIndex++);
		fProjectView.InsertRow(i, item);
	}
}

// ---------------------------------------------------------------------------
//		SetName
// ---------------------------------------------------------------------------
//	Change the name of this section.

void
MSectionLine::SetName(const char* inName)
{
	fName = inName;
}

// ---------------------------------------------------------------------------
//		WriteToFile
// ---------------------------------------------------------------------------
//	Write our Block to the stream.

void
MSectionLine::WriteToFile(
	MBlockFile & inFile)
{
	// Write the section block out
	SectionBlock		sectionBlock;

	sectionBlock.sLinesInSection = fLinesInSection;
	sectionBlock.sCodeSize = fCodeSize;
	sectionBlock.sDataSize = fDataSize;
	sectionBlock.sExpanded = fExpanded;
	strcpy(sectionBlock.sName, fName);
	sectionBlock.sunused1 = sectionBlock.sunused2 = 0;
	sectionBlock.SwapHostToBig();		// swap bytes

	inFile.StartBlock(kSectionBlockType);

	inFile.PutBytes(sizeof(sectionBlock), &sectionBlock);

	inFile.EndBlock();
	
	// Tell all of the files to write themselves out too
	int32				first = fFileList.IndexOf(fFirstLineInSection);
	int32				last = first + fLinesInSection;

	ASSERT(first >= 0 || fLinesInSection == 0);
	if (first >= 0)
	{
		for (int32 i = first; i < last; i++)
		{
			MSourceFileLine*	item = (MSourceFileLine*) fFileList.ItemAtFast(i);
	
			item->WriteToFile(inFile);
		}
	}
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
SectionBlock::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		sLinesInSection = B_BENDIAN_TO_HOST_INT32(sLinesInSection);
		sCodeSize = B_BENDIAN_TO_HOST_INT32(sCodeSize);
		sDataSize = B_BENDIAN_TO_HOST_INT32(sDataSize);
	}
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
SectionBlock::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		sLinesInSection = B_HOST_TO_BENDIAN_INT32(sLinesInSection);
		sCodeSize = B_HOST_TO_BENDIAN_INT32(sCodeSize);
		sDataSize = B_HOST_TO_BENDIAN_INT32(sDataSize);
	}
}
