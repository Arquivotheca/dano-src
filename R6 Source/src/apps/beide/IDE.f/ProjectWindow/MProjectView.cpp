//========================================================================
//	MProjectView.cpp
//	Copyright 1995 - 96 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	This view is the heart of the project.  It keeps track of all the
//	files in the project and starts all the compile and link processes.
//	This class could be factored into a view class and a project class
//	such that all the project-specific actions were implemented by the
//	project class.
//	BDS


#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "MProjectView.h"
#include "MProjectWindow.h"
#include "MTextWindow.h"
#include "MSectionLine.h"
#include "MSourceFileLine.h"
#include "MLibraryFileLine.h"
#include "MPCHFileLine.h"
#include "MSubProjectFileLine.h"
#include "MIgnoreFileLine.h"
#include "MPostLinkFileLine.h"
#include "MBlockFile.h"
#include "MInformationMessageWindow.h"
#include "MErrorMessageWindow.h"
#include "MDynamicMenuHandler.h"
#include "MDefaultPrefs.h"
#include "MFindFilesThread.h"
#include "MMessageItem.h"
#include "MFileSet.h"
#include "MFindDefinitionWindow.h"
#include "MFindDefinitionTask.h"
#include "MBuildCommander.h"
#include "MPlugInLinker.h"
#include "MProject.h"
#include "MTargetTypes.h"
#include "MBuildersKeeper.h"
#include "MAlert.h"
#include "MExceptions.h"
#include "ProjectCommands.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"

#include "MScripting.h"
#include "MScriptUtils.h"
#include "Coercion.h"
#include "Scripting.h"
#include "MProjectFileHandler.h"
#include "MHiliteColor.h"
#include "MLocker.h"
#include "MWEditUtils.h"

#include <Autolock.h>
#include <Beep.h>
#include <Bitmap.h>
#include <MessageQueue.h>
#include <String.h>
#include <Clipboard.h>
#include <vector.h>
#include <algorithm>
#include <typeinfo>

const float kMaximumWidth = 500.0;
const bigtime_t kKeyInterval = 500000;		// half a second
	
// ---------------------------------------------------------------------------
// Used for specifying what we are dragging
// ---------------------------------------------------------------------------

enum DragType
{
	DNotDragging,
	DFilesOnly,
	DSectionsOnly,
	DFilesAndSections
};

// ---------------------------------------------------------------------------
//		MProjectView
// ---------------------------------------------------------------------------
//	Constructor

MProjectView::MProjectView(BRect inFrame, const char* inName, MProjectWindow* ownerProject)
			 : MListView(inFrame, inName),
			   ScriptHandler(NewScriptID(), inName),
			   fFileList(50),
			   fAllFileList(50),
			  fFileListLocker("filelist")
{
	InitProjectView();
	fBuildCommander = new MBuildCommander(*this, fFileList, fFileListLocker);
	BRect r(0, 0, kMaximumWidth, kMaximumWidth);
	fResizeMap = nil;
	fChildView = new MProjectView(r, "", true);
	
	fErrorMessageWindow = new MErrorMessageWindow(ownerProject);
}

// ---------------------------------------------------------------------------
//		MProjectView
// ---------------------------------------------------------------------------
//	Constructor
//	private constructor for the childview so it doesn't create another
//	bitmap and childview.

MProjectView::MProjectView(BRect inFrame, char *inName, bool /*inTask*/)
		: MListView(inFrame, inName),
		ScriptHandler(NewScriptID(), inName)
{
	InitProjectView();

	fBuildCommander = nil;
	fProject = nil;
	fResizeMap = nil;
	fErrorMessageWindow = nil;
}

// ---------------------------------------------------------------------------
//		InitProjectView
// ---------------------------------------------------------------------------

void
MProjectView::InitProjectView()
{
	fFilesFound = 0;
	fDragIndex = -1;
	fKeyDownTime = 0;
	fTypingAhead = false;
	fFileSets = nil;
	fFileSetsSize = 0;
	fTargetPrefs.pTargetArray = nil;
	fTargetPrefs.pCount = 0;
	fFinder = nil;
	fFindDefinitionTask = nil;
	fFindDefinitionWindow = nil;
	SetDragSelect(false);
	SetDragNDropCells(true);
}

// ---------------------------------------------------------------------------
//		~MProjectView
// ---------------------------------------------------------------------------
//	Destructor

MProjectView::~MProjectView()
{
	// Tell all the builders that we're going away
	if (fProject != nil) {
		MBuildersKeeper::ProjectChanged(*fProject, kProjectClosed);
		delete fProject;
	}

	// Delete the file finder if it still exists
	if (fFinder != nil) {
		delete fFinder;
		fFinder = nil;
	}

	// Cancel (which deletes) the FindDefinitionTask if it still exists
	if (fFindDefinitionTask != nil) {
		fFindDefinitionTask->Cancel();
		fFindDefinitionTask = nil;
	}

	// Close the find definition window if it's still open
	if (fFindDefinitionWindow != nil && fFindDefinitionWindow->Lock()) {
		fFindDefinitionWindow->Quit();
	}

	delete fBuildCommander;
	delete fResizeMap;
	
	// Delete all the SourceFileLine objects
	int32	count = fFileList.CountItems();
	int 	i;
	
	for (i = 0; i < count; i++)
	{
		MSourceFileLine*	item = (MSourceFileLine*) fFileList.ItemAt(i);
		delete item;
	}

	// Delete all the SectionLine objects
	count = fSectionList.CountItems();
	for (i = 0; i < count; i++)
	{
		MSectionLine*	item = (MSectionLine*) fSectionList.ItemAt(i);
		delete item;
	}
	
	// Delete all the SourceFile objects
	count = fAllFileList.CountItems();
	for (i = 0; i < count; i++)
	{
		MSourceFile*	item = (MSourceFile*) fAllFileList.ItemAt(i);
		delete item;
	}
	
	// Delete all the path structs and directory objects
	MAccessPathsView::EmptyList(fSystemPathList);
	MAccessPathsView::EmptyList(fProjectPathList);
	MFileUtils::EmptyDirectoryList(fSystemDirectories, &fProjectDirectory);
	MFileUtils::EmptyDirectoryList(fProjectDirectories, &fProjectDirectory);

	// Remove all the rows from the visible list
	MListView::RemoveRows(0, CountRows());

	// Delete the file sets and the target records
	::operator delete(fFileSets);
	delete [] fTargetPrefs.pTargetArray;
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MProjectView::AttachedToWindow()
{
	// Set the row height to the max of the bold_font and fixed_font height.
	// These are the two fonts used by the project window.
	font_height		height;
	float			fixedHeight;
	float			boldHeight;
	
	be_fixed_font->GetHeight(&height);
	fixedHeight = height.ascent + height.descent + height.leading;
	be_bold_font->GetHeight(&height);
	boldHeight = height.ascent + height.descent + height.leading;
	
	MListView::AttachedToWindow();
	SetDefaultRowHeight(max(fixedHeight, boldHeight));
	ShowFileCount();

	fOldWidth = (int32) Bounds().Width();
	fProjectLinePainter.AdjustSizes(fOldWidth);
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MProjectView::Draw(
	BRect	inRect)
{
	if (fResizeMap == nil) {
		MListView::Draw(inRect);
	}
	else
	{
		// Copy the bitmap
		BRect		source = inRect;
		BRect		dest = inRect;
		float		right = fProjectLinePainter.GetNameRight();

		// Draw the name
		if (inRect.left < right)
		{
			source.right = dest.right = right;
		
			DrawBitmapAsync(fResizeMap, source, dest);
		}
		
		// Draw the code size, data size and arrow
		if (inRect.right > right)
		{
			source.right = dest.right = inRect.right;
			// draw up to what we drew above (right + 1) not over it
			source.left = dest.left = max(inRect.left, right+1);
			source.OffsetBy(fResizeMap->Bounds().right - Bounds().right, 0.0);
			
			DrawBitmapAsync(fResizeMap, source, dest);
		}
		
		Flush();
	}
}

// ---------------------------------------------------------------------------
//		FrameResized
// ---------------------------------------------------------------------------

void
MProjectView::FrameResized(
	float new_width, 
	float new_height)
{
	// Only redraw if there are no more resize messages in the message queue
	if (nil == Window()->MessageQueue()->FindMessage(B_VIEW_RESIZED, 1))
	{
		MListView::FrameResized(new_width, new_height);
	
		if (new_width != fOldWidth)
		{
			const BRect	bitBounds = fResizeMap->Bounds();
			const BRect	bounds = Bounds();
			BRect		source = bounds;
			BRect		dest = bounds;
	
			fProjectLinePainter.AdjustSizes(new_width);
	
			// Draw the names
			if (new_width > fOldWidth)
			{
				source.right = dest.right = fProjectLinePainter.GetNameRight();
				source.left = dest.left = fOldWidth - kCodeDataArrowWidth;
				DrawBitmapAsync(fResizeMap, source, dest);
			}
	
			// Draw code, data, arrow bitmap
			source.left = bitBounds.right - kCodeDataArrowWidth;
			source.right = bitBounds.right;
	
			dest.right = max(bounds.right, new_width);
			dest.left = dest.right - kCodeDataArrowWidth;
	
			DrawBitmapAsync(fResizeMap, source, dest);
	
			fOldWidth = (int32) new_width;
			Flush();
		}
	}
}

// ---------------------------------------------------------------------------
//		GenerateBitmap
// ---------------------------------------------------------------------------

void
MProjectView::GenerateBitmap(
	float		inHeight,
	int32		inFromRow,
	float		inRowTop)
{
	BBitmap*		temp = fResizeMap;

	if (temp)
		temp->RemoveChild(fChildView);

	BRect		r(0, 0, kMaximumWidth, inHeight);
	fResizeMap = new BBitmap(r, B_COLOR_8_BIT, true);

	fChildView->ResizeTo(r.right, r.bottom);
	fResizeMap->AddChild(fChildView);

	if (inFromRow > 0 && temp != nil)
	{
		fResizeMap->Lock();
		r.bottom = inRowTop - 1.0;

		fChildView->DrawBitmap(temp, r, r);	// copy the existing bitmap

		fResizeMap->Unlock();
	}
	
	delete temp;
}

// ---------------------------------------------------------------------------
//		UpdateBitmap
// ---------------------------------------------------------------------------

void
MProjectView::UpdateBitmap(
	int32	inFromRow)
{
	// Generate a new bitmap if we have to
	float		height = GetMaxYPixel();
	BRect		rowRect;

	if (inFromRow > 0)
		GetRowRect(inFromRow, &rowRect);

	if (fResizeMap == nil || height > fResizeMap->Bounds().bottom)
		GenerateBitmap(height, inFromRow, rowRect.top);
	
	fResizeMap->Lock();
	BRect		eraseRect = fResizeMap->Bounds();

	if (inFromRow > 0)
		eraseRect.top = rowRect.top;
 
	fChildView->FillRect(eraseRect, B_SOLID_LOW);		// Erase it

	// Draw all the rows
	int32		row = inFromRow;
	int32		stop = CountRows();
	BList&		list = *GetList();
	fProjectLinePainter.AdjustSizes(kMaximumWidth);

	const rgb_color 	viewColor = fChildView->ViewColor();
	const float			left = 0.0;
	const float			right = eraseRect.right;

	while (row < stop)
	{
		GetRowRect(row, &rowRect);

		rowRect.left = left;
		rowRect.right = right;

		MProjectLine*	line = (MProjectLine*) list.ItemAt(row);
		bool			selected = IsRowSelected(row);

		if (selected)
		{
			fChildView->SetLowColor(HiliteColor());
			fChildView->FillRect(rowRect, B_SOLID_LOW);
		}

		line->Draw(rowRect, rowRect, *fChildView);

		if (selected)
			fChildView->SetLowColor(viewColor);

		row++;
	}

	fProjectLinePainter.AdjustSizes(Bounds().right);

	fChildView->Sync();
	fResizeMap->Unlock();
}

// ---------------------------------------------------------------------------
//		UpdateBitmap
// ---------------------------------------------------------------------------
//	Only update the specified rows.  this is called in response to
//	adding or removing rows from the view.

void
MProjectView::UpdateBitmap(
	int32	inFromRow,
	int32	inHowMany,
	bool	inAdding)
{
//	MLocker<BBitmap>		lock(fResizeMap);
	fResizeMap->Lock();

	// Set up the rects that will be moved inside the view
	BRect		r1;
	BRect		r2;

	GetRowRect(inFromRow, &r1);
	GetRowRect(inFromRow + inHowMany - 1, &r2);
	
	float		end = GetMaxYPixel();
	float		height = end - (r2.bottom + 1);

	BRect		r3(0, r1.top, kMaximumWidth, r1.top + height);
	BRect		r4(0, r2.bottom + 1, kMaximumWidth, end);
	BRect		eraseRect = fResizeMap->Bounds();

	if (inAdding)
	{
		float		height = GetMaxYPixel();
	
		if (height > fResizeMap->Bounds().bottom)
		{
			fResizeMap->Unlock();
			GenerateBitmap(height, inFromRow, height + 1);
			fResizeMap->Lock();
		}

		// Adding rows to the view
		fChildView->CopyBits(r3, r4);

		eraseRect.top = r3.top;
		eraseRect.bottom = r2.bottom;
	 
		fChildView->FillRect(eraseRect, B_SOLID_LOW);		// Erase it
	
		// Draw the new rows in the bitmap
		int32		row = inFromRow;
		int32		stop = row + inHowMany;
		BList&		list = *GetList();
		fProjectLinePainter.AdjustSizes(kMaximumWidth);
	
		BRect				rowRect;
		const rgb_color 	viewColor = fChildView->ViewColor();
		const float			left = 0.0;
		const float			right = eraseRect.right;
	
		while (row < stop)
		{
			GetRowRect(row, &rowRect);
			rowRect.left = left;
			rowRect.right = right;
	
			MProjectLine*	line = (MProjectLine*) list.ItemAt(row);
			bool			selected = IsRowSelected(row);
	
			if (selected)
			{
				fChildView->SetLowColor(HiliteColor());
				fChildView->FillRect(rowRect, B_SOLID_LOW);
			}
	
			line->Draw(rowRect, rowRect, *fChildView);
	
			if (selected)
				fChildView->SetLowColor(viewColor);
	
			row++;
		}
	
		fProjectLinePainter.AdjustSizes(Bounds().right);
	}
	else
	{
		// Removing rows from the view
		if (inFromRow + inHowMany < CountRows())		// last section?
			fChildView->CopyBits(r4, r3);

		eraseRect.top = r3.bottom;
		eraseRect.bottom = end;
	 
		fChildView->FillRect(eraseRect, B_SOLID_LOW);	// Erase it
	}

	fChildView->Sync();
	fResizeMap->Unlock();
}

// ---------------------------------------------------------------------------
//		RemoveRows
// ---------------------------------------------------------------------------

void
MProjectView::RemoveRows(
	int32 	inFromRow,
	int32 	inHowMany)
{
	UpdateBitmap(inFromRow, inHowMany, false);

	MListView::RemoveRows(inFromRow, inHowMany);
}

// ---------------------------------------------------------------------------
//		UpdateRow
// ---------------------------------------------------------------------------
//	Should be rolled into UpdateBitmap.

void
MProjectView::UpdateRow(
	int32	inRow)
{
	fProjectLinePainter.AdjustSizes(kMaximumWidth);

	BRect		rowRect;

	GetRowRect(inRow, &rowRect);
	rowRect.right = kMaximumWidth;

	MProjectLine*	line = (MProjectLine*) GetList()->ItemAt(inRow);

	fResizeMap->Lock();
	bool			selected = IsRowSelected(inRow);
	rgb_color 		viewColor;

	if (selected)
	{
		viewColor = fChildView->ViewColor();
		fChildView->SetLowColor(HiliteColor());
	}

	fChildView->FillRect(rowRect, B_SOLID_LOW);	// Erase the row or hilite it
	line->Draw(rowRect, rowRect, *fChildView);

	if (selected)
		fChildView->SetLowColor(viewColor);
	fChildView->Sync();
	fResizeMap->Unlock();

	fProjectLinePainter.AdjustSizes(Bounds().right);
}

// ---------------------------------------------------------------------------
//		UpdateLine
// ---------------------------------------------------------------------------
//	The checkmark has been added or removed so update the appearance
//	of the line.

void
MProjectView::UpdateLine(
	BMessage&	inMessage)
{
	MProjectLine*	line;
	if (B_NO_ERROR == inMessage.FindPointer(kProjectLine, (void**) &line))
	{
		int32			row = IndexOf(line);
		
		if (row >= 0)
		{
			InvalidateRow(row);
			Window()->UpdateIfNeeded();
		}
	}
}

// ---------------------------------------------------------------------------
//		HiliteColorChanged
// ---------------------------------------------------------------------------
//	If any rows are selected update them in the bitmap and draw them on screen.

void
MProjectView::HiliteColorChanged()
{
	int32		row = -1;
	while (NextSelected(row))
	{
		HiliteRowInChildView(row, true);
		DrawRow(row);
	}
}

// ---------------------------------------------------------------------------
//		HiliteRow
// ---------------------------------------------------------------------------

void
MProjectView::HiliteRow(
	int32 		inRow,
	BRect 		inArea)
{
	if (fResizeMap)
	{
		HiliteRowInChildView(inRow, true);
		Draw(inArea);
	}
	else
		MListView::HiliteRow(inRow, inArea);
}

// ---------------------------------------------------------------------------
//		UnHiliteRow
// ---------------------------------------------------------------------------

void
MProjectView::UnHiliteRow(
	int32 		inRow,
	BRect 		inArea)
{
	if (fResizeMap)
	{
		HiliteRowInChildView(inRow, false);
		Draw(inArea);
	}
	else
		MListView::UnHiliteRow(inRow, inArea);
}

// ---------------------------------------------------------------------------
//		InvalidateRow
// ---------------------------------------------------------------------------

void
MProjectView::InvalidateRow(
	int32 	inRow)
{
	MListView::InvalidateRow(inRow);
	UpdateRow(inRow);
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

void
MProjectView::DrawRow(
	int32 	inRow)
{
	BRect 		rowRect;

	GetRowRect(inRow, &rowRect);
	Draw(rowRect);
}

// ---------------------------------------------------------------------------
//		HiliteRowInChildView
// ---------------------------------------------------------------------------

void
MProjectView::HiliteRowInChildView(
	int32 		inRow,
	bool		inDrawSelected)
{
	fProjectLinePainter.AdjustSizes(kMaximumWidth);
	BRect 		rowRect;

	GetRowRect(inRow, &rowRect);

	rowRect.right = kMaximumWidth;
	fResizeMap->Lock();

	rgb_color 		viewColor;
	MProjectLine*	line = (MProjectLine*) GetList()->ItemAt(inRow);

	if (inDrawSelected)
	{
		viewColor = fChildView->ViewColor();
		fChildView->SetLowColor(HiliteColor());
	}
	
	fChildView->FillRect(rowRect, B_SOLID_LOW);
	line->Draw(rowRect, rowRect, *fChildView);

	if (inDrawSelected)
		fChildView->SetLowColor(viewColor);
	fChildView->Sync();
	fResizeMap->Unlock();
	fProjectLinePainter.AdjustSizes(Bounds().right);
}

// ---------------------------------------------------------------------------
//		SelectRow
// ---------------------------------------------------------------------------

void
MProjectView::SelectRow(
	int32 	row,
	bool 	keepOld,
	bool 	toSelect)
{
	if (row >= 0 && row < CountRows())
	{
		if (! keepOld)
		{
			int32 cnt = CountRows();
			while (cnt-- > 0)
			{
				if (cnt != row && IsRowSelected(cnt))
					HiliteRowInChildView(cnt, false);	// Unselect the row
			}
			
			if (! toSelect && IsRowSelected(row))
				HiliteRowInChildView(row, toSelect);	// Change the selection
		}
	}
	
	bool		needToUpdateRow = keepOld && !toSelect && IsRowSelected(row);

	MListView::SelectRow(row, keepOld, toSelect);

	if (needToUpdateRow)
		HiliteRowInChildView(row, false);				// Unselect the row
}

// ---------------------------------------------------------------------------

void
MProjectView::SelectAllLines()
{
	this->SelectRows(0, this->CountRows()-1, false, true);
}


// ---------------------------------------------------------------------------

void
MProjectView::ClearSelection()
{
	// iterate through any rows selected and de-select them
	int32 row = -1;
	while (this->NextSelected(row)) {
		this->SelectRow(row, false, false);
		this->DrawRow(row);
	}
}

// ---------------------------------------------------------------------------

void
MProjectView::DoCopy()
{
	// Iterate through the selection and copy the external name of each item
	// Put that into the clipboard

	BString text;
	
	int32 row = -1;
	long count = 0;
	while (this->NextSelected(row)) {
		MProjectLine* line = (MProjectLine*) GetList()->ItemAt(row);
		char nameBuffer[B_PATH_NAME_LENGTH];
		line->ExternalName(nameBuffer);
		
		// add this name to our complete list
		// if this is 2..n, prepend with a newline
		if (count) {
			text += '\n';
		}
		text += nameBuffer;
		count += 1;
	}

	// Now that we have all our names in text, shove it into the clipboard
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
		be_clipboard->Data()->AddData(kTextPlain, B_MIME_TYPE, text.String(), text.Length());
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}

// ---------------------------------------------------------------------------
//		SelectRows
// ---------------------------------------------------------------------------

void
MProjectView::SelectRows(
	int32 	fromRow,
	int32 	toRow,
	bool 	keepOld,
	bool 	toSelect)
{
	ASSERT(fromRow <= toRow && fromRow >= 0 && toRow < CountRows());

	if (! keepOld)
	{
		int32 cnt = CountRows();
		while (cnt-- > 0)
		{
			if (cnt >= toRow && cnt <= fromRow && IsRowSelected(cnt))
				HiliteRowInChildView(cnt, false);
		}
	}
	
	MListView::SelectRows(fromRow, toRow, keepOld, toSelect);
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MProjectView::MessageReceived(
	BMessage * message)
{
	switch (message->what)
	{
		case cmd_CreateGroup:
			this->AddSectionAtSelection();
			break;

		case cmd_SortGroup:
			this->SortSelectionGroup();
			break;
			
		case cmd_RemoveFiles:
			this->RemoveSelection();
			break;

		case msgFindDefinitionClosed:
			if (fFindDefinitionWindow->Lock()) {
				fFindDefinitionWindow->Quit();
			}
			fFindDefinitionWindow = nil;
			break;

		case msgUpdateLine:
			this->UpdateLine(*message);
			break;

		case cmd_Cancel:
			this->Cancel();
			break;
			
		case msgCommandRightArrow:
		case msgCommandLeftArrow:
			this->DoArrowKey(message->what);
			break;

		case msgShowIdleStatus:
			this->ShowIdleStatus();
			break;

		case cmd_RevealInFinder:
			this->RevealSelectionInTracker();
			break;

		case cmd_OpenCurrent:
			this->OpenCurrentSelection();
			break;

		case B_SIMPLE_DATA:
			this->MessageDropped(message);
			break;

		default:
			MListView::MessageReceived(message);
			break;
	}
}

// ---------------------------------------------------------------------------
//		Cancel
// ---------------------------------------------------------------------------

void
MProjectView::Cancel()
{
	CompileState		state = CompilingState();

	if (state != sNotCompiling && state != sCancelling)
	{
		fBuildCommander->Cancel();
		fBuildCommander->ShowCompileStatus();
		PulseOn(this);
	}
}

// ---------------------------------------------------------------------------
//		DoArrowKey
// ---------------------------------------------------------------------------
//	Handle a command right arrow or command left arrow to expand or contract
//	the section.

void
MProjectView::DoArrowKey(
	uint32	inArrowMessage)
{
	int32				row = -1;
	
	if (NextSelected(row))
	{
		MSectionLine*	section = SectionAt(row);
		int32			goodRow = row;

		if (section != nil && ! NextSelected(row))
		{		
			BRect		rowRect;
			
			GetRowRect(goodRow, &rowRect);

			if (section->DoArrowKey(rowRect, inArrowMessage, IsRowSelected(row)))
				UpdateBitmap(goodRow);		
		}
	}
}

// ---------------------------------------------------------------------------
//		SectionAt
// ---------------------------------------------------------------------------
//	return the section at the specified row or nil if it's not a section.

MSectionLine*
MProjectView::SectionAt(
	int32	inRow)
{
	MSectionLine*		section = (MSectionLine*) ItemAt(inRow);

	if (fSectionList.HasItem(section))
		return section;
	else
		return nil;
}

// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------
//	Deal with type ahead selection of lines in the list.

void
MProjectView::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	switch (inBytes[0])
	{
		case B_RETURN:
		case MAC_RETURN:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_TAB:
			MListView::KeyDown(inBytes, inNumBytes);
			break;

		case B_HOME:
			SelectRow(0);
			ScrollRowIntoView(0);
			break;

		case B_END:
		{
			int32		row = CountRows() - 1;
			SelectRow(row);
			ScrollRowIntoView(row);
		}
			break;

		case B_PAGE_UP:
		{
			int32		i = -1;

			if (FirstSelected(i) > 1 || i != 0)
			{
				i = -1;
				while (NextSelected(i))
				{
					SelectRow(i, false, false);
					DrawRow(i);
				}
			}
				
			PageUp();
			BRect		bounds = Bounds();
			BRect		frame = Frame();
			ConvertToScreen(&bounds);
			ConvertToScreen(&frame);
			int32		row = GetPosRow(bounds.top - frame.top);
			SelectRow(row);
			ScrollRowIntoView(row);
		}
			break;

		case B_PAGE_DOWN:
		{
			int32		i = -1;
			if (FirstSelected(i) > 1 || i != CountRows() - 1)
			{
				i = -1;
				while (NextSelected(i))
				{
					SelectRow(i, false, false);
					DrawRow(i);
				}
			}
	
			PageDown();
			BRect		bounds = Bounds();
			BRect		frame = Frame();
			ConvertToScreen(&bounds);
			ConvertToScreen(&frame);
			int32		row = GetPosRow(bounds.bottom - frame.top);
			SelectRow(row);
			ScrollRowIntoView(row);
		}
			break;

		default:
			bigtime_t		nowTime = system_time();
			uchar			key[4];	// a two char string
			for (int i = 0; i < inNumBytes; i++)
				key[i] = inBytes[i];
			key[inNumBytes] = '\0';

			if (nowTime > fKeyDownTime)
			{
				// Reset the search string
				if (key[0] < ' ')		// is the key valid?
					key[0] = 0;

				fSearchString = (const char *) key;
			}
			else
			{
				// Append a char to the search string
				if (key[0] == B_BACKSPACE)
				{
					if (fSearchString.GetLength() > 0)
					{
						int		glyphWidth = fSearchString.GlyphWidth(fSearchString.GetLength() - 1);
						fSearchString.Replace("", fSearchString.GetLength() - glyphWidth, glyphWidth);
					}
				}
				else
				if (key[0] >= ' ')			// simple filter for valid keystrokes
					fSearchString += (const char *) key;
			}
			
			int32	row = FindRowByName(fSearchString);
			if (row >= 0 && row < CountRows())
			{	
				SelectRow(row, false);
				ScrollRowIntoView(row);
			}

			BMessage		msg(msgSetStatusText);
			msg.AddString(kText, fSearchString);

			Window()->PostMessage(&msg);

			fKeyDownTime = nowTime + kKeyInterval;
			fTypingAhead = true;
			PulseOn(this);
			break;
	}
}

// ---------------------------------------------------------------------------
//		FindRowByName
// ---------------------------------------------------------------------------
//	Return the first row that matches the name specified.

int32
MProjectView::FindRowByName(
	const char *	inName)
{
	int32		result = -1;
	int32		len = strlen(inName);
	BList*		list = GetList();
	String		lineName;
	String		name = inName;
	name.ToUpper();

	for (int32 i = 0; i < CountRows(); i++)
	{
		MProjectLine*		line = (MProjectLine*) list->ItemAt(i);
		lineName = line->Name();
		lineName.ToUpper();

		if (0 == strncmp(name, lineName, len))
		{
			result = i;
			break;
		}
	}

	return result;	
}

// ---------------------------------------------------------------------------
//		AddSectionAtSelection
// ---------------------------------------------------------------------------

bool
MProjectView::AddSectionAtSelection()
{
	int32 	theRow;

	if (FirstSelected(theRow) < 1)
		theRow = 0;

	return AddSectionAtIndex(theRow);
}

// ---------------------------------------------------------------------------
//		AddSectionAtIndex
// ---------------------------------------------------------------------------
//	Add a new section to the view.

bool
MProjectView::AddSectionAtIndex(int32 inIndex)
{
	if ((inIndex < 0 ) || (inIndex > CountRows()))
		return false;

	MProjectLine*		line = (MProjectLine*) GetList()->ItemAt(inIndex);
	MProjectLine*		prevline = (MProjectLine*) GetList()->ItemAt(inIndex - 1);

	// Verify that line and previous line aren't sections, is it in the sectionlist?
	if (fSectionList.HasItem((MSectionLine*) line) || 
			fSectionList.HasItem((MSectionLine*) prevline))
		return false;

	// Make new section and add it to the listView
	MSectionLine*		newSection = new MSectionLine(*this, fFileList);
	MSectionLine*		existingSection = line->GetSection();

	InsertRow(inIndex, newSection);
	// Add it to the section list
	int32			indexInSectionList = fSectionList.IndexOf(existingSection);
	fSectionList.AddItem(newSection, indexInSectionList + 1);

	// Syncronize everything
	MProjectLine*	firstLine = existingSection->GetFirstLine();
	uint32			numInExistingSection = existingSection->GetLines();
	int32			indexOfFirstLine = IndexOf((void *) firstLine);

	newSection->SetFirstLine(line);
	int32			firstIndex = IndexOf((void *) newSection) + 1;

	int32			lastIndex = firstIndex + (numInExistingSection - (firstIndex - indexOfFirstLine - 1));
	for (int32 i = firstIndex; i < lastIndex; i++)
	{
		MProjectLine*		item = (MProjectLine*) GetList()->ItemAt(i);
		
		existingSection->RemoveLine(item);
		newSection->AddLine(item);
	}
	
	UpdateBitmap(inIndex);

	// Save the change
	SaveChanges();

	return true;
}

// ---------------------------------------------------------------------------

void
MProjectView::SortSelectionGroup()
{
	// For each item in the selection, make sure it is
	// a section, and if not, use its section
	// Make sure we haven't already sorted this item
	// Finally, sort the section
	
	MSectionLineList sortedList;
	int32 theRow = -1;
	while (NextSelected(theRow)) {
		MProjectLine* line = (MProjectLine*) ItemAt(theRow);
		MSectionLine* section = (MSectionLine*) line;
		
		// if we don't have a section selected, get the section
		// of the selection
		if (fSectionList.IndexOf(section) < 0) {
			section = line->GetSection();
		}

		// make sure we haven't already sorted this section
		if (sortedList.IndexOf(section) < 0) {
			this->SortSection(section);
			sortedList.AddItem(section);
		}
	}

	// this is a shortcut for moving a bunch of stuff around,
	// update the window and notify builders
	UpdateBitmap();
	SetDirty();
	Window()->UpdateIfNeeded();
	MBuildersKeeper::ProjectChanged(*fProject, kFilesRearranged);
	fBuildCommander->ProjectChanged();
}

// ---------------------------------------------------------------------------
//		AddDefaultSection
// ---------------------------------------------------------------------------
//	Add an empty section for a new project.

void
MProjectView::AddDefaultSection()
{
	MSectionLine*		newSection = new MSectionLine(*this, fFileList);

	InsertRow(0, newSection);
	fSectionList.AddItem(newSection, 0);
	
	UpdateBitmap();

	fProject = new MProject((MProjectWindow&) *Window());
}

// ---------------------------------------------------------------------------
//		RemoveSelection
// ---------------------------------------------------------------------------
//	Remove the current selection from the view in response to the Remove file
//	or Remove Group menu items.

void
MProjectView::RemoveSelection()
{
	int32 	theRow;

	while ((theRow = CurrentSelection()) >= 0)
	{
		MProjectLine*	line = (MProjectLine*) GetList()->ItemAt(theRow);
		int32			index = fSectionList.IndexOf((MSectionLine*) line);

		if (index < 0)
			RemoveFile((MSourceFileLine*) line, false);
		else
		if (index > 0)
			RemoveSection((MSectionLine*) line, false);
		else
		{
			// Can't remove the first group if there are no other groups
			if (fSectionList.CountItems() > 1)
				RemoveSection((MSectionLine*) line, false);
			else
				break;	// exit to prevent infinite loop
		}
	}

	UpdateBitmap();
	SaveChanges();
	ShowFileCount();
	fBuildCommander->ProjectChanged();
	MBuildersKeeper::ProjectChanged(*fProject, kFilesRemoved);
}

// ---------------------------------------------------------------------------
//		RemoveSection
// ---------------------------------------------------------------------------
//	Remove all the lines included in this section and the section itself.

void
MProjectView::RemoveSection(
	MSectionLine* 	inSection,
	bool			inUpdate)
{
	int32				count = inSection->GetLines();
	bool				expanded = inSection->IsExpanded();
	MProjectLine*		line = inSection->GetFirstLine();
	int32				index = fFileList.IndexOf(line);
	MSourceFileLine*	sourceLine;

	ASSERT(index >= 0 || count == 0);

	for (int i = 0; i < count; i++)
	{
		line = (MProjectLine*) fFileList.ItemAt(index);
		if (expanded)
			RemoveItem(line);
		fFileList.RemoveItemAt(index);
		sourceLine = dynamic_cast<MSourceFileLine*>(line);
		if (sourceLine)
			sourceLine->DeleteObjectFile();
		inSection->RemoveLine(line);			// Deletes the section when no more files in it
		delete line;
	}

	// Need to delete or unselect the section if it had no files in it
	if (count == 0)
		KillSection(inSection);

	if (inUpdate)
	{
		UpdateBitmap();
		SaveChanges();
		ShowFileCount();
		fBuildCommander->ProjectChanged();
		MBuildersKeeper::ProjectChanged(*fProject, kFilesRemoved);
	}
}

// ---------------------------------------------------------------------------
//		KillSection
// ---------------------------------------------------------------------------
//	Remove a section from the view.

void
MProjectView::KillSection(
	MSectionLine* inSection)
{
	ASSERT(inSection);
	ASSERT(fSectionList.IndexOf(inSection) >= 0);

	// Only remove it if it's not the only section
	if (fSectionList.CountItems() > 1)
	{
		RemoveItem(inSection);
		fSectionList.RemoveItem(inSection);
		delete inSection;
	}
	else
	{
		// Unselect this section so RemoveSelection won't go on infinitely
		SelectRow(IndexOf(inSection), true, false);
	}
}

// ---------------------------------------------------------------------------
// utility function for SortSection
// ---------------------------------------------------------------------------

bool
CompareProjectLines(MProjectLine* left, MProjectLine* right)
{
	// The result is "left < right"
	// given two MProjectLine's, which are really MSourceFileLines
	// do a string compare on their names (case insensitive,
	// but allow distinct ordering if exact matches)

	int32 result = CompareStringsNoCase(left->Name(), right->Name());
	if (result == 0) {
		result = strcmp(left->Name(), right->Name());
	}	
	return result < 0;	
}

// ---------------------------------------------------------------------------

void
MProjectView::SortSection(MSectionLine* section)
{
	vector<MProjectLine*> sortVector;
	
	// It would be easier to remove all the lines and then
	// put them back.  However once the section lines are gone
	// the section is gone and it is alot more work to create
	// a new one than be very careful as we move the items
	// around in the section
	
	// Get all the lines of the section into a vector for sorting
	int32 sectionCount = section->GetLines();

	// if we only have one line in the section, it is sorted!
	if (sectionCount <= 1) {
		return;
	}

	bool expanded = section->IsExpanded();
	MProjectLine* projectLine = section->GetFirstLine();
	int32 listIndex = this->IndexOf(projectLine);
	int32 firstFileIndex = fFileList.IndexOf(projectLine);
	int32 fileIndex = firstFileIndex;
	for (int32 i = 0; i < sectionCount; i++) {
		projectLine = (MProjectLine*) fFileList.ItemAt(fileIndex++);
		sortVector.push_back(projectLine);
	}
		
	// do the sorting
	sort(sortVector.begin(), sortVector.end(), CompareProjectLines);	
	
	// iterate through the sorted list
	// remove the item from its old location, and put it at
	// the top of the section...
	for (int i = 0; i < sectionCount; i++) {
		projectLine = sortVector[i];
		// remove
		if (expanded) {
			this->RemoveItem(projectLine);
		}
		section->RemoveLine(projectLine);
		fFileList.RemoveItem(projectLine);
				
		// add
		if (expanded) {
			this->InsertRow(listIndex++, projectLine);
		}
		fFileList.AddItem(projectLine, firstFileIndex++);
		section->AddLine(projectLine);
	}
}

// ---------------------------------------------------------------------------
//		RemoveFile
// ---------------------------------------------------------------------------
//	Remove a single file that is selected in the project view.

void
MProjectView::RemoveFile(
	MSourceFileLine* 	inSourceFileLine,
	bool				inUpdate)
{
	MSectionLine*		section = inSourceFileLine->GetSection();
	
	section->RemoveLine(inSourceFileLine);
	fFileList.RemoveItem(inSourceFileLine);
	RemoveItem(inSourceFileLine);
	inSourceFileLine->DeleteObjectFile();

	// If the window is open notify it that it's been terminated
	MSourceFile*		sourceFile = inSourceFileLine->GetSourceFile();
	entry_ref			ref;

	if (sourceFile && B_NO_ERROR == sourceFile->GetRef(ref))
	{
		MTextWindow*	wind = MDynamicMenuHandler::FindWindow(ref);
	
		if (wind && wind->Lock())
		{
			BMessage		msg(msgProjectClosed);
			msg.AddPointer(kProjectMID, Window());
			wind->PostMessage(&msg);

			BMessage		msg1(msgProjectOpened);
			msg1.AddPointer(kProjectMID, Window());
			wind->PostMessage(&msg1);
			wind->Unlock();
		}
	}

	delete inSourceFileLine;
	
	if (inUpdate)
	{
		UpdateBitmap();
		ShowFileCount();
		SaveChanges();
		fBuildCommander->ProjectChanged();
		MBuildersKeeper::ProjectChanged(*fProject, kFilesRemoved);
	}
}

// ---------------------------------------------------------------------------
//		SourceFileLineFromScript
// ---------------------------------------------------------------------------

status_t
MProjectView::SourceFileLineFromScript(
	BMessage* 			inMessage,
	MSourceFileLine*&	outLine)
{
	type_code	type;
	int32 		count;
	entry_ref	ref;
	status_t	err = B_NO_ERROR;

	if (B_NO_ERROR != inMessage->GetInfo(kDefaultDataName, &type, &count))
		err = SCRIPT_MISSING_DATA;
	else
	if (type == B_REF_TYPE)
	{
		err = inMessage->FindRef(kDefaultDataName, &ref);
	}
	else
	{
		if (!GetCoercions().GetRef(ref, inMessage, kDefaultDataName))
			err = SCRIPT_BAD_TYPE;
	}

	if (err == B_NO_ERROR)
	{ 
		BEntry		file(&ref);
		
		if (!file.IsFile())
			err = SCRIPT_BAD_TYPE;
	}

	if (err == B_NO_ERROR)
	{
		if (! GetSourceFileLineByRef(ref, outLine))
			err = B_ERROR;
	}

	return err;
}

// ---------------------------------------------------------------------------
//		RemoveFileFromScript
// ---------------------------------------------------------------------------
//	Remove a single file that is identified in a message from a
//	script or another app.

int32
MProjectView::RemoveFileFromScript(
	BMessage* 			inMessage,
	BMessage* 			/*inReply*/,
	MSourceFileLine*	inLine)
{
	int32		result = B_NO_ERROR;

	if (inLine != nil)
		RemoveFile(inLine);
	else
	{
		MSourceFileLine*	line;
		result = SourceFileLineFromScript(inMessage, line);
		if (result == B_NO_ERROR)
			RemoveFile(line);
	}

	if (result == B_NO_ERROR)
	{
		MBuildersKeeper::ProjectChanged(*fProject, kFilesRemoved);
		fBuildCommander->ProjectChanged();
	}

	return result;
}

// ---------------------------------------------------------------------------
//		SectionIsSelected
// ---------------------------------------------------------------------------
//	Is there a section in the current selection.

bool
MProjectView::SectionIsSelected()
{
	bool		result = false;
	int32		row = -1;
	
	while (NextSelected(row))
	{
		MSectionLine*		section = (MSectionLine*) ItemAt(row);

		if (fSectionList.HasItem(section))
		{
			result = true;
			break;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		FileIsSelected
// ---------------------------------------------------------------------------
//	Is there a file in the current selection.

bool
MProjectView::FileIsSelected()
{
	bool		result = false;
	int32		row = -1;
	
	while (NextSelected(row))
	{
		MSourceFileLine*		fileLine = (MSourceFileLine*) ItemAt(row);

		if (fFileList.HasItem(fileLine))
		{
			result = true;
			break;
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------
//	Tell the line to draw itself.

void
MProjectView::DrawRow(
	int32 		/*index*/,
	void *		data,
	BRect 		frame,
	BRect		intersectionRect)
{
	MProjectLine*	line = (MProjectLine*) data;
	line->Draw(frame, intersectionRect, *this);
}

// ---------------------------------------------------------------------------
//		DoRefsReceived
// ---------------------------------------------------------------------------

bool
MProjectView::DoRefsReceived(
	BMessage &	inMessage, 
	BPoint 		loc)
{
	// we need this here for dragging files into the project
	if (this->OKToModifyProject() == false) {
		return false;
	}
	
	int32		count;
	type_code	messageType;
	bool		result = false;
	
	if (B_NO_ERROR == inMessage.GetInfo("refs", &messageType, &count))
	{
		entry_ref		ref;

		for (int32 i = count - 1; i >= 0; i--)
		{
			if (B_NO_ERROR == inMessage.FindRef( "refs", i, &ref ))
			{
				BEntry		entry(&ref);

				if (entry.IsFile())
				{
					// It's a file
					result |= AddFile(entry, loc);
				}
				else
				if (entry.IsSymLink())
				{
					//It's a link
					result |= AddSymLink(entry, loc);
				}
				else
				if (entry.IsDirectory())
				{
					// It's a folder
					BDirectory		folder(&ref);
					
					result |= AddFolder(folder, loc);
				}
				else
				{
					// It's something else (?!)
					String			text = "Don't know how to deal with this item.";
		
					MAlert			alert(text);
					alert.Go();
				}
				
				Window()->UpdateIfNeeded();	// relieves the monotony while adding files
			}
		}
	}

	if (result)
		UpdateBitmap();

	return result;
}

// ---------------------------------------------------------------------------
//		GetIndex
// ---------------------------------------------------------------------------
//	Get the index that is under the point.  If the point is beyond the bottom
//	of the view the index of the last member is returned.  The point is in
//	view coordinates.

inline int32
MProjectView::GetIndex(BPoint inPoint)
{
	return GetPosRow(inPoint.y);
}

// ---------------------------------------------------------------------------
//		AddToProject
// ---------------------------------------------------------------------------
//	Add a text file to the project.  Called usually when a text window asks 
//	to be added to the project.  The window field in the message is optional.

void
MProjectView::AddToProject(
	BMessage& inMessage)
{
	// Get the window
	MTextWindow*		window;
	
	if (B_NO_ERROR == inMessage.FindPointer(kTextWindow, (void**) &window))
	{
		entry_ref			ref;
	
		if (B_NO_ERROR == inMessage.FindRef(kTextFileRef, &ref))
		{

			BEntry				file(&ref);
			int32				index = CurrentSelection();
	
			if ((index < 0) || index >= CountRows())
				index = CountRows() - 1;// Add at the end if there's no selection
	
			// Add it to the project
			if (AddFileAtIndex(file, index))
			{
				// Tell it that it's been added
				if (window)
				{
					BMessage		msg(msgProjectOpened);
			
					msg.AddPointer(kProjectMID, Window());	
					window->PostMessage(&msg);
				}
					
				UpdateBitmap(index);
				SaveChanges();
				MBuildersKeeper::ProjectChanged(*fProject, kFilesAdded);
				fBuildCommander->ProjectChanged();
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		FileWasAddedToProject
// ---------------------------------------------------------------------------

void
MProjectView::FileWasAddedToProject(
	const BEntry& inFile)
{
	entry_ref		ref;
	
	if (B_NO_ERROR == inFile.GetRef(&ref))
	{
		MTextWindow*		window = MDynamicMenuHandler::FindWindow(ref);
		
		if (window != nil)
		{
			BMessage		msg(msgProjectOpened);
	
			msg.AddPointer(kProjectMID, Window());	
			window->PostMessage(&msg);
		}
	}
}

// ---------------------------------------------------------------------------
//		AddFolder
// ---------------------------------------------------------------------------
//	Add all text files to the project that are in the Directory.
//	Probably should do lib files ???? Should do all
//	files in the folder that have targets. ????

bool
MProjectView::AddFolder(
	BDirectory& 	inFolder, 
	BPoint 			inLoc)
{
	int32 index = GetIndex(inLoc);
	// when we do our first add, it will become index #<firstAddedIndex>
	int32 firstAddedIndex = index + 1;
	bool result = false;
	BEntry file;

	while (B_NO_ERROR == inFolder.GetNextEntry(&file))
	{
		// Verify that the file's not already in the project.
		if (file.IsFile() && ! FileIsInProjectByName(&file))
		{
			if (AddFileAtIndex(file, index))
			{
				index++;
				result = true;
			}
		}
		// Add a SymLink to the project
		else
		if (file.IsSymLink() && ! FileIsInProjectByName(&file))
		{
			entry_ref ref;
			file.GetRef(&ref);
			
			file.SetTo(&ref, true);
			if (file.IsFile())
			{
				/* get the appropriate target rec */		
				ProjectTargetRec *inRec = fBuildCommander->GetTargetRec(file);
				
				file.SetTo(&ref);
				
				if (AddFileAtIndex(file, index, inRec))
				{
					index++;
					result = true;
				}
			}
		}
	}

	// when adding a folder, group all the contents into a new section
	// and name it the same as the folder name
	// (when adding a section, we specify the first line in the section, that
	// is why we use firstAddedIndex)
	if (result && index >= firstAddedIndex && this->AddSectionAtIndex(firstAddedIndex)) {
		MSectionLine* section = (MSectionLine*) ItemAt(firstAddedIndex);
		char directoryName[B_FILE_NAME_LENGTH];
		BEntry dirEntry;
		if (inFolder.GetEntry(&dirEntry) == B_OK && dirEntry.GetName(directoryName) == B_OK) {
			BMessage msg;
			msg.AddString(kName, directoryName);
			msg.AddPointer(kSection, section);
			this->SetSectionName(msg);
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		NewProjectSaved
// ---------------------------------------------------------------------------

void
MProjectView::NewProjectSaved()
{
	MBuildersKeeper::ProjectChanged(*fProject, kProjectOpened);
	fBuildCommander->ProjectChanged();
}

// ---------------------------------------------------------------------------
//		RunMenuItemChanged
// ---------------------------------------------------------------------------

void
MProjectView::RunMenuItemChanged()
{
	if (fProject != nil)
		fBuildCommander->RunMenuItemChanged(*fProject);
}

// ---------------------------------------------------------------------------
//		IsRunnable
// ---------------------------------------------------------------------------
//	Can the file generated by this project be run?  Called to enable/disable
//	the run/debug menu items.  Shared libs can't be run, for example.

bool
MProjectView::IsRunnable()
{
	return fBuildCommander->IsRunnable();
}

// ---------------------------------------------------------------------------
//		AddLibs
// ---------------------------------------------------------------------------
//	Add libbe.so and libdll.a to a project.

void
MProjectView::AddLibs()
{
	// Add libroot, libbe and libdll to the new project
	entry_ref		ref;
	entry_ref		ref2;
	BEntry			libFile;
	bool			foundFile = false;
	BPoint			where(10, 100);

	// libroot.so
	if (MFileUtils::FindFileInDirectoryList("libroot.so", fSystemDirectories, ref))
	{
		libFile.SetTo(&ref);
		foundFile = true;
	}
	else
	{	
		if (B_NO_ERROR == get_ref_for_path("/boot/beos/system/lib", &ref2))
		{
			BDirectory		dir(&ref2);
			
			if (B_NO_ERROR == FindFile(dir, "libroot.so", &libFile))
			{
				foundFile = true;
			}
		}
	}
	if (foundFile)
	{
		AddFile(libFile, where);
	}

	// libbe.so
	if (MFileUtils::FindFileInDirectoryList("libbe.so", fSystemDirectories, ref))
	{
		libFile.SetTo(&ref);
		foundFile = true;
	}
	else
	{	
		if (B_NO_ERROR == get_ref_for_path("/boot/beos/system/lib", &ref2))
		{
			BDirectory		dir(&ref2);
			
			if (B_NO_ERROR == FindFile(dir, "libbe.so", &libFile))
			{
				foundFile = true;
			}
		}
	}
	if (foundFile)
	{
		AddFile(libFile, where);
	}

	// libdll.a
	foundFile = false;
	if (MFileUtils::FindFileInDirectoryList("libdll.a", fSystemDirectories, ref))
	{
		libFile.SetTo(&ref);
		foundFile = true;
	}
	else
	{
		if (B_NO_ERROR == get_ref_for_path("/boot/beos/develop/lib", &ref2))
		{
			BDirectory		dir(&ref2);
			
			if (B_NO_ERROR == FindFile(dir, "libdll.a", &libFile))
			{
				foundFile = true;
			}
		}
	}
	if (foundFile)
	{
		AddFile(libFile, where);
	}

	UpdateBitmap();
}

// ---------------------------------------------------------------------------
//		AddFile
// ---------------------------------------------------------------------------
//	Add a file to the project at the specified location.  return true if
//	we really did it.

bool
MProjectView::AddFile(
	const BEntry& 	inFile, 
	BPoint 			inLoc)
{
	int32		index = GetIndex(inLoc);
	bool		fileWasAdded = AddFileAtIndex(inFile, index);

	if (fileWasAdded)
	{
		FileWasAddedToProject(inFile);
	}
	
	return fileWasAdded;
}

// ---------------------------------------------------------------------------
//		AddSymLink
// ---------------------------------------------------------------------------
//	Add a symlink to the project at the specified location.
//	if the symlink is a file, add the file with the symlink path.
//  return true if we really did it.
bool	
MProjectView::AddSymLink(const BEntry &link, BPoint loc)
{
	BEntry entry = link;

	entry_ref ref;
	entry.GetRef(&ref);

	bool  result = false;
	
	entry.SetTo(&ref, true);
	
	if (entry.IsFile()) {

		/* get the appropriate target rec */		
		ProjectTargetRec *inRec = fBuildCommander->GetTargetRec(entry);
		
		int32 index = GetIndex(loc);
		//use original link for adding the file
		result = AddFileAtIndex(link, index, inRec);
		
		if (result)
		{
			FileWasAddedToProject(entry);
		}

	}
	else
	{
		// It's something else (?!)
		String			text = "Don't know how to deal with this item.";

		MAlert			alert(text);
		alert.Go();
	}
	
	return result;
}


// ---------------------------------------------------------------------------
//		GetLine
// ---------------------------------------------------------------------------
//	Build a sourcefileline for this file.

MSourceFileLine*
MProjectView::GetLine(
	const BEntry&		inFile,
	MSectionLine&		inSection,
	ProjectTargetRec*	inRec,
	const char *		inName)
{
	bool				systemTree = IsInSystemTree(inFile);
	MSourceFileLine*	outLine = nil;

	switch (inRec->Target.Stage)		// Build the sourcefileline
	{
		case ignoreStage:
			outLine = new MIgnoreFileLine(inFile, systemTree, inSection, *this, inRec, inName);
			break;

		case precompileStage:
			if (strcmp(inRec->Target.MimeType, kProjectMimeType) == 0) {
				outLine = new MSubProjectFileLine(inFile, systemTree, inSection, *this, inRec, inName);
			}
			else {
				outLine = new MPCHFileLine(inFile, systemTree, inSection, *this, inRec, inName);
			}
			break;

		case compileStage:
			outLine = new MSourceFileLine(inFile, systemTree, MSourceFile::kSourceFileKind, inSection, *this, inRec, inName);
			break;

		case linkStage:
			outLine = new MLibraryFileLine(inFile, systemTree, inSection, *this, inRec, inName);
			break;

		case postLinkStage:
			outLine = new MPostLinkFileLine(inFile, systemTree, inSection, *this, inRec, inName);
			break;

		default:
			ASSERT(false);
			break;
	}
	
	return outLine;
}

// ---------------------------------------------------------------------------
//		GetNewSourceFileLine
// ---------------------------------------------------------------------------
//	Build a sourcefileline for this file.

status_t
MProjectView::GetNewSourceFileLine(
	const BEntry& 		inFile,
	MSectionLine*		inSection,
	MSourceFileLine*&	outLine,
	String&				inoutErrorText,
	ProjectTargetRec*	inRec)				// can be nil
{
	status_t			err = B_NO_ERROR;

	if (inRec == nil)
	{
		inRec = fBuildCommander->GetTargetRec(inFile);

		if (inRec == nil)
		{
			uint32		type = MimeType(inFile);

			// Attempt to fix up the file type
			if (type == kNULLType && FixFileType(&inFile, fTargetPrefs.pTargetArray, fTargetPrefs.pCount))
				inRec = fBuildCommander->GetTargetRec(inFile);
		}

		if (inRec == nil)
		{
			// Report 'no target record' error
			err = NoTargetError(inFile, inoutErrorText);
		}
	}

	if (err == B_NO_ERROR)
	{
		// Find out the access path this file is in and whether it's recursive or not
		char			filepath[kPathSize];
		const char *	name = nil;
		DirectoryInfo*	accessPath = nil;

		ValidatePath(inFile, accessPath);
		
		// If it's not recursive then we save the file name as a relative path from the access path
		// If it's recursive then we save just the filename
		if (accessPath != nil)
		{
			if (accessPath->dRecursiveSearch)
			{
				// Get the filename
				err = inFile.GetName(filepath);
				name = &filepath[0];
			}
			else
			{
				// Get the relative path to the file
				filepath[0] = '\0';
				BPath pathObject;
				BEntry dir(&accessPath->dDir);
				err = dir.GetPath(&pathObject);
				int32	len = strlen(pathObject.Path());
				if (err == B_NO_ERROR)
					err = inFile.GetPath(&pathObject);
				if (err == B_NO_ERROR)
				{
					strcpy(filepath, pathObject.Path());
					name = &filepath[len + 1];	// skip over the '/'
				}
			}
		}
		
		// Duplicate name?
		MSourceFileLine*	line = nil;
		if (err == B_NO_ERROR && FileIsInProjectByName(name, &line))
		{
			// Report 'duplicate name' error
			err = DuplicateNameError(inFile, line, inoutErrorText);
		}

		if (err == B_NO_ERROR) {
			if (inFile.IsSymLink()){
				BEntry entry = inFile;
				entry_ref ref;
				entry.GetRef(&ref);
				entry.SetTo(&ref, true);
				if(FileIsInProjectByFile(&entry)) {
					//Report 'duplicate file' error
					err = DuplicateFileError(entry, line, inoutErrorText);
				}
			} else {
				if(FileIsInProjectByFile(&inFile)) {
					//Report 'duplicate file' error
					err = DuplicateFileError(inFile, line, inoutErrorText);
				}
			}
		}
		if (err == B_NO_ERROR)
			outLine = GetLine(inFile, *inSection, inRec, name);
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		AddFileAtIndex
// ---------------------------------------------------------------------------
//	Add the file at the index specified in the view.

bool
MProjectView::AddFileAtIndex(
	const BEntry&		inFile,
	int32				inIndex,
	ProjectTargetRec*	inRec)
{
	MSectionLine*		section = GetSection(inIndex);
	MSourceFileLine*	line = nil;
	String				errorText;

	int32	err = GetNewSourceFileLine(inFile, section, line, errorText, inRec);

	if (err != B_NO_ERROR || line == nil)
		ReportAddFileError(inFile, err, errorText);
	else
	{
		inIndex++;
		int32			indexInFileList = GetIndexInFileList(*section, inIndex);

		if (section->IsExpanded())
		{
			InsertRow(inIndex, line);
			UpdateBitmap(inIndex);
		}
		fFileList.AddItem(line, indexInFileList);
	
		section->AddLine(line);		// Notify the section line
	
		LockFileList();				// Prevent concurrent access to fAllFileList
		fAllFileList.AddItem(line->GetSourceFile());
		UnlockFileList();

		ShowFileCount();
	}

	return line != nil;
}

// ---------------------------------------------------------------------------
//		DuplicateNameError
// ---------------------------------------------------------------------------
//	Build an error string for a 'duplicate name' error.

int32
MProjectView::DuplicateNameError(
	const BEntry&		inFile,
	MSourceFileLine*	inLine,
	String&				inoutErrorText)
{
	// Build 'duplicate name' error
	char		filepath[1024] = { '\0' };
	BPath		path;
	inFile.GetPath(&path);

	inoutErrorText = "The file "B_UTF8_OPEN_QUOTE;
	inoutErrorText += path.Leaf();
	inoutErrorText += B_UTF8_CLOSE_QUOTE" wasn't added to the project because\n";
	
	// Is this file itself in the project or is it another file with the same name?
	entry_ref	fileRef;
	(void) inFile.GetRef(&fileRef);
	bool		samefile = fileRef == inLine->GetSourceFile()->Ref();
	if (samefile)
	{
		inoutErrorText += "it's already in the project.";			
	}
	else
	{
		inoutErrorText += "another file with the same name is already in the project.\n";
		inoutErrorText += path.Path();
		inoutErrorText += "\n";
		inLine->GetSourceFile()->GetPath(filepath);
		inoutErrorText += filepath;
	}

	return M_DUPLICATE_FILE;
}

// ---------------------------------------------------------------------------
//		DuplicateFileError
// ---------------------------------------------------------------------------
//	Build an error string for a 'duplicate file' error.

int32
MProjectView::DuplicateFileError(
	const BEntry&		inFile,
	MSourceFileLine*	/* inLine */,
	String&				inoutErrorText)
{
	// Build 'duplicate name' error
	BPath		path;
	inFile.GetPath(&path);

	inoutErrorText = "The file "B_UTF8_OPEN_QUOTE;
	inoutErrorText += path.Leaf();
	inoutErrorText += B_UTF8_CLOSE_QUOTE" wasn't added to the project because\n";
	
	inoutErrorText += "it's already in the project.";			

	return M_DUPLICATE_FILE;
}


// ---------------------------------------------------------------------------
//		NoTargetError
// ---------------------------------------------------------------------------
//	Build an error string for a 'no target' error.

int32
MProjectView::NoTargetError(
	const BEntry&		inFile,
	String&				inoutErrorText)
{
	// Build' no target record' error
	char		name[B_FILE_NAME_LENGTH] = { '\0' };

	inFile.GetName(name);
	inoutErrorText = "The file "B_UTF8_OPEN_QUOTE;
	inoutErrorText += name;
	inoutErrorText += B_UTF8_CLOSE_QUOTE" wasn't added to the project because\n"
	 			"there is no matching target record in the Targets preferences panel.";

	return M_NO_TARGET;
}

// ---------------------------------------------------------------------------
//		ReportAddFileError
// ---------------------------------------------------------------------------

void
MProjectView::ReportAddFileError(
	const BEntry&		inFile,
	status_t			inError,
	String&				inText)
{
	switch (inError)
	{
		case M_DUPLICATE_FILE:
		case M_NO_TARGET:
			// Text is already correct for these so do nothing
			break;

		default:
			char		name[B_FILE_NAME_LENGTH] = { '\0' };
			inFile.GetName(name);
			inText = "The file "B_UTF8_OPEN_QUOTE;
			inText += name;
			inText += B_UTF8_CLOSE_QUOTE" wasn't added to the project because an unexpected error occurred.\n";
			inText += inError;
			break;
	}
	
	InfoStruct	 	info;

	info.iTextOnly = true;
	strncpy(info.iLineText, inText, kLineTextLength);
	info.iLineText[kLineTextLength-1] = '\0';
	
	BMessage		msg(msgAddInfoToMessageWindow);
	
	msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
	
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);
}

// ---------------------------------------------------------------------------
//		AddFileAtIndexAbsolute
// ---------------------------------------------------------------------------
//	Add the file at the index specified in the filelist.

status_t
MProjectView::AddFileAtIndexAbsolute(
	const BEntry&		inFile,
	int32				inIndexInFileList)
{
	int32				indexInView;
	MSectionLine*		section;
	MSourceFileLine*	line = nil;

	if (inIndexInFileList < 0 || inIndexInFileList > fFileList.CountItems())
		inIndexInFileList = fFileList.CountItems();
	
	if (inIndexInFileList >= 0)
		line = (MSourceFileLine*) fFileList.ItemAt(inIndexInFileList);

	if (line != nil)
	{
		section = line->GetSection();
		indexInView = IndexOf(line);
		line = nil;
	}
	else
	if (inIndexInFileList == 0)
	{
		// empty project
		section = fSectionList.ItemAt(0);	
		indexInView = 1;
	}
	else
	{
		// add at end
		section = fSectionList.LastItem();
		indexInView = GetIndexInView(inIndexInFileList - 1) + 1;	// won't be correct if section isn't expanded
	}
	
	String		errorString;
	status_t	err = GetNewSourceFileLine(inFile, section, line, errorString);

	ASSERT(err == B_NO_ERROR || line == nil);

	if (line)
	{
		if (section->IsExpanded())
		{
			InsertRow(indexInView, line);
			UpdateBitmap(indexInView);
		}
		fFileList.AddItem(line, inIndexInFileList);
	
		section->AddLine(line);		// Notify the section line
	
		LockFileList();				// Prevent concurrent access to fAllFileList
		fAllFileList.AddItem(line->GetSourceFile());
		UnlockFileList();

		ShowFileCount();
	}

	return err;
}

// ---------------------------------------------------------------------------
//		AddFilesMessage
// ---------------------------------------------------------------------------
//	Add a file to the project.  This message comes from the add files panel.

void
MProjectView::AddFilesMessage(
	BMessage& inMessage)
{
	// Put together a location for refsreceived
	int32		index = LastSelected();
	
	if (index < 0)
		index = CountRows() - 1;
						
	BRect		r;

	GetRowRect(index, &r);
	r.left++;

	if (DoRefsReceived(inMessage, r.LeftTop()))
	{
		Window()->UpdateIfNeeded();
		MBuildersKeeper::ProjectChanged(*fProject, kFilesAdded);
		fBuildCommander->ProjectChanged();
		SaveChanges();
	}
	else
		beep();
}

// ---------------------------------------------------------------------------
//		AddFilesFromScript
// ---------------------------------------------------------------------------
//	Add a file to the project.  This message comes from a script
//	or another app.

status_t
MProjectView::AddFilesFromScript(
	BMessage* 	inMessage,
	BMessage*	/*inReply*/,
	int32		inIndex)	// index in file list
{
	type_code 		type;
	int32 			count;
	entry_ref		ref;
	status_t		err;

	if (B_NO_ERROR != inMessage->GetInfo(kDefaultDataName, &type, &count))
		return SCRIPT_MISSING_DATA;
	else
	if (type == B_REF_TYPE)
	{
		err = inMessage->FindRef(kDefaultDataName, &ref);
		if (err != B_NO_ERROR)
			return err;
	}
	else
	{
		if (!GetCoercions().GetRef(ref, inMessage, kDefaultDataName))
			return SCRIPT_BAD_TYPE;
	}

	BEntry		file(&ref);
	
	if (!file.IsFile())
	{
		return SCRIPT_BAD_TYPE;
	}

	err = AddFileAtIndexAbsolute(file, inIndex);

	if (err == B_NO_ERROR)
	{
		FileWasAddedToProject(file);
		MBuildersKeeper::ProjectChanged(*fProject, kFilesAdded);
		fBuildCommander->ProjectChanged();
		SetDirty();
	}

	return err;
}

// ---------------------------------------------------------------------------
//		ValidatePath
// ---------------------------------------------------------------------------
//	When a file is added to the project we need to check that it is in one
//	of the existing access paths.  If it isn't we need to add the access path
//	that it is in to the project.

void
MProjectView::ValidatePath(
	const BEntry& 			inFile,
	DirectoryInfo*&			outInfo)
{
	if (! MFileUtils::FileIsInDirectoriesList(fProjectDirectories, inFile, outInfo) &&
			! MFileUtils::FileIsInDirectoriesList(fSystemDirectories, inFile, outInfo))
	{
		BDirectory			dir;

		if (B_NO_ERROR == inFile.GetParent(&dir))
		{
			// Add the new access path to the project
			AccessPathData* newAccessPath = new AccessPathData;
			char path[kPathSize];
			BEntry dirEntry;
			dir.GetEntry(&dirEntry);

			if (MFileUtils::BuildRelativePath(fProjectDirectory, dir, path))
			{
				newAccessPath->pathType = kProjectRelativePath;
				strcpy(newAccessPath->pathName, path);
			}
			else
			{
				BPath pathObject;
				dirEntry.GetPath(&pathObject);

				newAccessPath->pathType = kAbsolutePath;
				strcpy(newAccessPath->pathName, pathObject.Path());
			}			

			// Set up the DirectoryInfo for this access path
			DirectoryInfo* dirInfo = new DirectoryInfo;
			dirEntry.GetRef(&dirInfo->dDir); 

			// both the following have to match
			newAccessPath->recursiveSearch = true;
			dirInfo->dRecursiveSearch = true;

			fProjectPathList.AddItem(newAccessPath);
			fProjectDirectories.AddItem(dirInfo);
			fAccessPathsPrefs.pProjectPaths++;
			outInfo = dirInfo;

			// Post an info message to the message window
			String text = "The following access path has been added to the project:\n";

			if (newAccessPath->pathType == kProjectRelativePath)
				text += kProjectPathName;			
			text += newAccessPath->pathName;
			
			InfoStruct	 	info;
	
			info.iTextOnly = true;
			strncpy(info.iLineText, text, kLineTextLength);
			
			BMessage msg(msgAddInfoToMessageWindow);
			
			msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
			
			MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
			MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);

			SetDirty();
		}
	}
}

// ---------------------------------------------------------------------------
//		SaveAsForSourceFile
// ---------------------------------------------------------------------------
//	An open source file window has been saved as.  We need to change the
//	line for this file to reflect its new name and we need to change the
//	source file object to point to the new file.

void
MProjectView::SaveAsForSourceFile(
	BMessage& 		inMessage)
{
	type_code	type;
	int32		count;

	if (B_NO_ERROR == inMessage.GetInfo(kTextFileRef, &type, &count) && count == 2)
	{
		entry_ref		oldRef;
		entry_ref		newRef;
		
		if (B_NO_ERROR == inMessage.FindRef(kTextFileRef, 0, &oldRef) &&
			B_NO_ERROR == inMessage.FindRef(kTextFileRef, 1, &newRef))
		{
			entry_ref		ref3;
			bool			found = false;
	
			LockFileList();			// Prevent concurrent access to fAllFileList
	
			int32				i = 0;
			MProjectLine*		projectLine;
			MSourceFileLine*	sourceFileLine;
			MSourceFile*		sourceFile;

			while (fFileList.GetNthItem(projectLine, i++))
			{
				sourceFileLine = dynamic_cast<MSourceFileLine*>(projectLine);
				ASSERT(sourceFileLine);
				sourceFile = sourceFileLine->GetSourceFile();

				if (sourceFile)
				{
					if (B_NO_ERROR == sourceFile->GetRef(ref3) && oldRef == ref3)
					{
						found = true;
						break;
					}
				}
			}
		
			if (found)
			{
				sourceFile->SetRef(newRef, true);
				fAllFileList.RemoveItem(sourceFile);	// stored in sort order by name
				fAllFileList.AddItem(sourceFile);
				
				sourceFileLine->DeleteObjectFile();
				sourceFileLine->UpdateSuffixType();
				int32		row = IndexOf(sourceFileLine);

				if (row >= 0)
				{
					InvalidateRow(row);
					Window()->UpdateIfNeeded();
				}
				
				SaveChanges();
			}
	
			UnlockFileList();
		}
	}
}

// ---------------------------------------------------------------------------
//		GetIndexInFileList
// ---------------------------------------------------------------------------
//	Find the index in the file list of a line from the view.
//	inIndex is the index of the line in the view.

int32
MProjectView::GetIndexInFileList(
	const MSectionLine& inSection,
	int32 				inIndex) const
{
	MProjectLine*		firstLine =	inSection.GetFirstLine();
	int32				firstIndex = fFileList.IndexOf(firstLine);
	int32				indexInFileList = 0;
	
	if (firstIndex != -1)				// Empty section - empty project?
	{
		int32			firstIndexInView = IndexOf((void*) firstLine);

		if (firstIndexInView == -1)		// Section contracted
		{
			indexInFileList = firstIndex;
		}
		else
		{
			indexInFileList = firstIndex + (inIndex - firstIndexInView);
			if (indexInFileList > fFileList.CountItems())
				indexInFileList = fFileList.CountItems();
			else
			if (indexInFileList < 0)
				indexInFileList = 0;
		}
	}

	return indexInFileList;
}

// ---------------------------------------------------------------------------
//		GetIndexInView
// ---------------------------------------------------------------------------
//	Find the index in the view of a line from the filelist.
//	inIndex is the index of the line in the filelist.

int32
MProjectView::GetIndexInView(
	int32 				inIndex) const
{
	int32			indexInView = -1;
	MProjectLine*	line = fFileList.ItemAt(inIndex);
	
	if (line != nil)
	{
		indexInView = IndexOf(line);
	}
	
	return indexInView;
}

// ---------------------------------------------------------------------------
//		GetSection
// ---------------------------------------------------------------------------
//	Get the section at the specified location.  Each ProjectLine Object
//	knows its section.

MSectionLine*
MProjectView::GetSection(BPoint inLoc)
{
	// Get the line at the location
	int32				index = GetIndex(inLoc);
	MProjectLine*		line = (MProjectLine*) GetList()->ItemAt(index);
	ASSERT(line);

	// Get the section
	MSectionLine*		section = line->GetSection();

	return section;
}

// ---------------------------------------------------------------------------
//		GetSection
// ---------------------------------------------------------------------------
//	Get the section for the line at the specified index.  If inIndex is equal
//	to or past the end of the list then return the section for the end of
//	the list.

MSectionLine*
MProjectView::GetSection(int32 inIndex)
{
	// Get the line at the index
	MProjectLine*		line = (MProjectLine*) GetList()->ItemAt(inIndex);
	
	if (line == nil && inIndex >= CountRows())
		line = (MProjectLine*) GetList()->ItemAt(CountRows() - 1);
	ASSERT(line);

	// Get the section
	MSectionLine*		section = line->GetSection();

	return section;
}

// ---------------------------------------------------------------------------
//		SetSectionName
// ---------------------------------------------------------------------------
//	Set the section name.  This message comes from the setsectionname
//	dialog.

void
MProjectView::SetSectionName(
	BMessage& inMessage)
{
	MSectionLine*		section;
	const char*			name;

	if (B_NO_ERROR == inMessage.FindPointer(kSection, (void**) &section) &&
		B_NO_ERROR == inMessage.FindString(kName, &name))
	{
		ASSERT(section);
		ASSERT(name);
		if (section != nil && name != nil)
		{
			section->SetName(name);
			int32			index = IndexOf(section);
			InvalidateRow(index);
			Window()->UpdateIfNeeded();

			SaveChanges();
		}
	}
}

// ---------------------------------------------------------------------------
//		ClickHook
// ---------------------------------------------------------------------------
//	Called for a single click.

bool
MProjectView::ClickHook(
	BPoint	inWhere,
	int32	inRow,
	uint32	modifiers,
	uint32	buttons)
{
	bool			result = false;
	MProjectLine *	line = (MProjectLine *) GetList()->ItemAt(inRow);

	if (line)
	{
		bool			isSelected = IsRowSelected(inRow);
		const int32		rowcount = CountRows();
		BRect 			frame;
		GetRowRect(inRow, &frame);

		if (! isSelected && line->SelectImmediately(frame, inWhere, isSelected, modifiers, buttons))
			SelectRow(inRow, false, true);

		result = line->DoClick(frame, inWhere, isSelected, modifiers, buttons);
		
		if (result)
			UpdateRow(inRow);

		const int32		newrowcount = CountRows();

		if (newrowcount > rowcount)
		{
			UpdateBitmap(inRow + 1, newrowcount - rowcount, true);
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		MessageDropped
// ---------------------------------------------------------------------------

bool
MProjectView::MessageDropped(
	BMessage *	inMessage)
{
	bool result = false;
	BPoint loc = inMessage->DropPoint();
	this->ConvertFromScreen(&loc);

	// We don't accept files while we're compiling
	if (this->CompilingState() == sNotCompiling) {
		this->EraseDragLine();
		this->Window()->UpdateIfNeeded();

		// we have two types of drags here, both with B_SIMPLE_DATA
		// If we have a source_view, and it is us, then the drag
		// was started locally, so we do HandleDragSelf.  
		// If we don't have a source_view (or it doesn't match)
		// then the drag was started in another project or
		// in the Tracker, and we add those files to the project
		if (inMessage->what == B_SIMPLE_DATA) {		
			if (inMessage->FindInt32("source_view") == (int32)this) {
				this->HandleDragSelf(*inMessage, GetIndex(loc));
				result = true;
			}
			else {
				result = this->DoRefsReceived(*inMessage, loc);
				if (result) {
					this->Window()->UpdateIfNeeded();
					this->SaveChanges();
					MBuildersKeeper::ProjectChanged(*fProject, kFilesAdded);
					fBuildCommander->ProjectChanged();
				}
				else {
					this->ShowFileCount();
					this->Window()->UpdateIfNeeded();
				}
			}
		}
	}
	
	// lame feedback for a drop failure
	if (result == false) {
		beep();
	}
	
	return result;
}

// ---------------------------------------------------------------------------

bool
MProjectView::AddRefToMessage(BMessage& msg, MProjectLine* projectLine) 
{
	// given a line in the project, add it as an entry_ref to the message

	bool added = false;
	MSourceFileLine* sourceLine = dynamic_cast<MSourceFileLine*>(projectLine);
	if (sourceLine != NULL) {
		MSourceFile* sourceFile = sourceLine->GetSourceFile();
		entry_ref ref;
		if (sourceFile != NULL && sourceFile->GetRef(ref) == B_OK) {				
			msg.AddRef("refs", &ref);
			bool added = true;
		}
	}
	
	return added;
}

// ---------------------------------------------------------------------------

void
MProjectView::InitiateDrag(
	BPoint 	/*inPoint*/,
	int32	/*inRow*/)
{
	//	Hook function called to start a drag.

	if (CompilingState() == sNotCompiling) {
		BMessage msg(B_SIMPLE_DATA);
		bool draggingFiles = false;
		bool draggingSections = false;
		int32 theRow = -1;
		int32 firstRow = -1;
		int32 lastRow = -1;
		
		while (NextSelected(theRow))
		{
			MProjectLine*		line = (MProjectLine*) ItemAt(theRow);
			MSectionLine*		section = (MSectionLine*) line;
		
			// keep track of first/last rows for dragging
			if (firstRow == -1) {
				firstRow = theRow;
			}
				
			if (fSectionList.IndexOf(section) >= 0) {
				// It's a section
				msg.AddPointer(kProjectLine, line);
				draggingSections = true;

				// Add all the lines in a section as refs 
				// (only used in case of dragging outside project, 
				// but I couldn't figure out how to modify 
				// the message when leaving the view)
				int32 count = section->GetLines();
				int32 index = fFileList.IndexOf(section->GetFirstLine());
				MProjectLine* aLine;		
				for (int i = 0; i < count; i++) {
					aLine = fFileList.ItemAt(index+i);
					this->AddRefToMessage(msg, aLine);
				}

				// (for internal drags...) Skip til the next section
				theRow += count;
			}
			else {
				// It's a file
				msg.AddPointer(kProjectLine, line);
				draggingFiles = true;
				
				// also add the file's ref for dragging outside project
				this->AddRefToMessage(msg, line);
			}
		}
		// keep track of the last row selected also
		lastRow = theRow;

		// Add the type of drag to the message
		// (drags from the Tracker don't have kDragType)
		DragType drag = DNotDragging;
		
		if (draggingFiles && ! draggingSections) {
			drag = DFilesOnly;
		}
		else if (draggingSections && ! draggingFiles) {
			drag = DSectionsOnly;
		}
		else if (draggingSections && draggingFiles) {
			drag = DFilesAndSections;
		}

		// keep track of the drag type for when dropped and also a this pointer 
		// (as a unique number) to distinguish between origination 
		// (internal or from external) of drag
		msg.AddInt32(kDragType, drag);
		msg.AddInt32("source_view", (int32)this);
		
		BRect selectRect;
		this->GetRowRect(firstRow, &selectRect);
		selectRect.right = fProjectLinePainter.GetNameRight();
		// create a union rectangle for the drag		
		if (firstRow != lastRow) {
			BRect lastRect;
			this->GetRowRect(lastRow, &lastRect);
			selectRect.bottom = lastRect.bottom;
		}
		
		this->DragMessage(&msg, selectRect);
	}
}

// ---------------------------------------------------------------------------
//		HandleDragSelf
// ---------------------------------------------------------------------------
//	Handle the message that is dropped when a drag within this view is 
//	completed.

void
MProjectView::HandleDragSelf(
	BMessage&	inMessage, 
	int32 		inIndex)
{
	int32			count;
	type_code		type;
	MProjectLine*	newLine;

	if (B_NO_ERROR == inMessage.GetInfo(kProjectLine, &type, &count) && type == B_POINTER_TYPE &&
		B_NO_ERROR == inMessage.FindPointer(kProjectLine, (void**) &newLine))
	{
		int32			index = IndexOf(newLine);
		DragType		drag = DNotDragging;
		
		// Don't do anything if the message was dropped
		// at the same place it was started from
		// (or if the project is locked)
		if (index != inIndex &&
			B_NO_ERROR == inMessage.FindInt32(kDragType, (int32*) &drag) &&
			this->OKToModifyProject() == true)
		{
			ASSERT(drag != DNotDragging);

			switch (drag)
			{
				case DFilesOnly:
					DragFiles(inMessage, inIndex, count);
					break;

				case DSectionsOnly:
					DragSections(inMessage, inIndex, count);
					break;

				case DFilesAndSections:
					DragFilesAndSections(inMessage, inIndex, count);
					break;
			}
			
			UpdateBitmap();
			SetDirty();
			Window()->UpdateIfNeeded();
			MBuildersKeeper::ProjectChanged(*fProject, kFilesRearranged);
			fBuildCommander->ProjectChanged();
		}
	}
}

// ---------------------------------------------------------------------------
//		DragFiles
// ---------------------------------------------------------------------------
//	Handle dragging of one or more files.

void
MProjectView::DragFiles(
	BMessage&	inMessage, 
	int32 		inIndex,
	int32		inCount)
{
	MList<MProjectLine*>	list;
	MProjectLine*	projectLine = (MProjectLine*) ItemAt(inIndex);
	MSectionLine*	newSection = projectLine->GetSection();
	int32			newIndex = inIndex + 1;
	MProjectLine*	firstLine;

	status_t	err = inMessage.FindPointer(kProjectLine, (void**) &firstLine);

	// Build a list containing all the files to be moved
	for (int32 i = 0; i < inCount; i++)
	{
		err = inMessage.FindPointer(kProjectLine, i, (void**) &projectLine);
		if (err != B_NO_ERROR)
			continue;

		MSectionLine*		section = (MSectionLine*) projectLine;
		int32				oldIndex;

		list.AddItem(projectLine);
		oldIndex = IndexOf(projectLine);

		// Remove from the old section, the view, and the file list
		section = projectLine->GetSection();
		section->RemoveLine(projectLine);
		fFileList.RemoveItem(projectLine);
		RemoveItem(projectLine);

		if (newIndex > oldIndex)
			newIndex--;
	}

	// Move the files to their new locations
	int32		k = 0;
	bool		expanded = newSection->IsExpanded();
	int32		indexInFileList = GetIndexInFileList(*newSection, newIndex);
	
	if (! expanded)
		indexInFileList += newSection->GetLines();	// Add at end of section

	while (list.GetNthItem(projectLine, k++))
	{
		if (expanded)
			InsertRow(newIndex++, projectLine);
		fFileList.AddItem(projectLine, indexInFileList++);
		newSection->AddLine(projectLine);			// Notify the section line
	}

	// Select the lines that were just moved
	if (expanded)
	{
		int32			linesMoved = list.CountItems();
		int32			firstIndex = IndexOf(firstLine);

		SelectRows(firstIndex, firstIndex + linesMoved - 1, false, true);
	}
}

// ---------------------------------------------------------------------------
//		DragFilesAndSections
// ---------------------------------------------------------------------------
//	Handle dragging of one or more files and one or more sections that have
//	been selected together.

void
MProjectView::DragFilesAndSections(
	BMessage&	inMessage, 
	int32 		inIndex,
	int32		inCount)
{
	MList<MProjectLine*>	list;
	MProjectLine*			projectLine = (MProjectLine*) ItemAt(inIndex);
	MSectionLine*			newSection = projectLine->GetSection();
	int32					sectioncount = 0;
	int32					newIndex = inIndex + 1;
	int32					oldIndex;
	int32					firstIndex;
	status_t				err;

	// Build a list containing all the files to be moved
	for (int32 i = 0; i < inCount; i++)
	{
		err = inMessage.FindPointer(kProjectLine, i, (void**) &projectLine);
		if (err != B_NO_ERROR)
			continue;
		
		MSectionLine*		section = (MSectionLine*) projectLine;

		// Is it a section?
		if (fSectionList.IndexOf(section) >= 0)
		{
			// It's a section
			projectLine = section->GetFirstLine();
			firstIndex = fFileList.IndexOf(projectLine);
			oldIndex = IndexOf(section);

			bool			expanded = section->IsExpanded();
			int32			term = section->GetLines();

			// Copy all the projectlines in this section to the list
			for (int32 j = 0; j < term; j++)
			{
				projectLine = (MProjectLine*) fFileList.ItemAt(firstIndex);
				list.AddItem(projectLine);
				fFileList.RemoveItem(projectLine);
				if (expanded)
					RemoveItem(projectLine);
			}
			
			// Delete the section
			RemoveItem(section);
			fSectionList.RemoveItem(section);
			delete section;

			if (newIndex > oldIndex)
				newIndex -= term + 1;
			
			sectioncount++;
		}
		else
		{
			// It's a file
			list.AddItem(projectLine);
			oldIndex = IndexOf(projectLine);

			// Remove from the old section, the view, and the file list
			section = projectLine->GetSection();
			section->RemoveLine(projectLine);
			fFileList.RemoveItem(projectLine);
			RemoveItem(projectLine);

			if (newIndex > oldIndex)
				newIndex--;
		}
	}
	
	// Move the files to their new locations
	int32		k = 0;
	bool		expanded = newSection->IsExpanded();
	int32		indexInFileList = GetIndexInFileList(*newSection, newIndex);

	if (! expanded)
		indexInFileList += newSection->GetLines();	// Add at end of section

	firstIndex = newIndex;

	while (list.GetNthItem(projectLine, k++))
	{
		if (expanded)
			InsertRow(newIndex++, projectLine);
		fFileList.AddItem(projectLine, indexInFileList++);
		newSection->AddLine(projectLine);			// Notify the section line
	}

	// Select the lines that were just moved
	if (expanded)
	{
		int32			linesMoved = list.CountItems();
		if (linesMoved)
			SelectRows(firstIndex, firstIndex + linesMoved - 1, false, true);
	}
}

// ---------------------------------------------------------------------------
//		DragSections
// ---------------------------------------------------------------------------
//	Handle dragging of one or more sections.

void
MProjectView::DragSections(
	BMessage&	inMessage, 
	int32 		inIndex,
	int32		inCount)
{
	int32			linesMoved = 0;
	int32			newIndex = GetSectionDropIndex(inIndex) + 1;

	MSectionLine*	firstSection;
	status_t		err = inMessage.FindPointer(kProjectLine, (void**) &firstSection);

	// Insert from front to back
	if (err == B_NO_ERROR)
	for (int32 i = 0; i < inCount; i++)
	{
		int32				indexInFileList;
		int32				firstIndexInSection;
		int32				oldIndex;
		MProjectLine*		projectLine;
		MSectionLine*		newSection;
		MSectionLine*		section;
		err = inMessage.FindPointer(kProjectLine, i, (void**) &section);
		ASSERT(err == B_NO_ERROR);
	
		projectLine = (MProjectLine*) ItemAt(newIndex - 1);
		newSection = projectLine->GetSection();
		// check if we are just moving to our current position for this section
		// if we are, skip the move (otherwise looking up newSection below will
		// lead to incorrect ordering of sections in the fSectionList)
		if (newSection == section) {
			linesMoved += section->GetLines() + 1;
			continue;
		}
		projectLine = section->GetFirstLine();
		firstIndexInSection = fFileList.IndexOf(projectLine);
		oldIndex = IndexOf(section);

		MList<MProjectLine*>		list;

		// Copy all the projectlines in this section to a temp list
		bool			expanded = section->IsExpanded();
		int32			term = section->GetLines();

		// Copy all the projectlines in this section to the list
		for (int32 j = 0; j < term; j++)
		{
			projectLine = (MProjectLine*) fFileList.ItemAt(firstIndexInSection);
			list.AddItem(projectLine);
			fFileList.RemoveItem(projectLine);
			if (expanded)
				RemoveItem(projectLine);
		}

		// Move the section to its new location
		RemoveItem(section);
		fSectionList.RemoveItem(section);
		
		int32		linesWeMoved = 1;
		if (expanded)
			linesWeMoved += list.CountItems();

		if (newIndex > oldIndex)
			newIndex -= linesWeMoved;

		linesMoved += linesWeMoved;

		indexInFileList = GetIndexInFileList(*newSection, newIndex);

		projectLine = (MProjectLine*) ItemAt(newIndex - 1);
		int32	newSectionsIndex = fSectionList.IndexOf(newSection) + 1;

		InsertRow(newIndex++, section);
		fSectionList.AddItem(section, newSectionsIndex);

		// Move the files to their new locations
		int32		k = 0;
		while (list.GetNthItem(projectLine, k++))
		{
			if (expanded)
				InsertRow(newIndex++, projectLine);
			fFileList.AddItem(projectLine, indexInFileList++);
		}
	}
	
	// Select the lines that were just moved
	if (linesMoved)
	{
		int32	firstIndex = IndexOf(firstSection);
		SelectRows(firstIndex, firstIndex + linesMoved - 1, false, true);
	}
}

// ---------------------------------------------------------------------------
//		GetSectionDropIndex
// ---------------------------------------------------------------------------
//	When dragging a section it can only be dropped at the end of other
//	sections.  This function returns the index of a projectline that is at
//	the end of a section based on the input point.

int32
MProjectView::GetSectionDropIndex(
	int32 		inIndex)
{
	int32			index = inIndex;
	MProjectLine*	line = (MProjectLine*) ItemAt(index);
	MSectionLine*	section1 = line->GetSection();
	int32			index2 = fSectionList.IndexOf(section1) + 1;
	MSectionLine*	section2 = (MSectionLine*) fSectionList.ItemAt(index2);
	
	int32			index1 = IndexOf(section1);
	if (section2) {
		index2 = IndexOf(section2);
	}
	else {
		index2 = CountRows();
	}
		
	if (index1 == 0 || section2 == nil || index >= index2 - ((index2 - index1) / 2)) {
		index = index2 - 1;
	}
	else {
		index = index1 - 1;
	}
	
	return index;
}

// ---------------------------------------------------------------------------
//		MouseMoved
// ---------------------------------------------------------------------------
//	Called during drags.  We draw the visual feedback in the appropriate
//	location.  Drags can come from the Browser or from within this view.

void
MProjectView::MouseMoved(
	BPoint 			inPoint,
	uint32 			inCode,
	const BMessage *inMessage)
{
	bool goodMessage = CompilingState() == sNotCompiling && (inMessage != nil && 
		(inMessage->what == B_SIMPLE_DATA || inMessage->HasRef("refs")));

	if (goodMessage) {
		DragType drag = DFilesOnly;		// Drags from the Browser won't have this field
		int32 index = GetIndex(inPoint);

		(void) inMessage->FindInt32(kDragType, (int32*) &drag);

		if (drag == DSectionsOnly) {
			index = this->GetSectionDropIndex(index);
		}

		switch (inCode)
		{
			case B_ENTERED_VIEW:
				this->DrawDragLine(index);
				break;

			case B_INSIDE_VIEW:
				this->DrawDragLine(index);
				break;

			case B_EXITED_VIEW:
				this->EraseDragLine();
				break;
		}

		Sync();
	}
}

// ---------------------------------------------------------------------------
//		DrawDragLine
// ---------------------------------------------------------------------------

void
MProjectView::DrawDragLine(
	int32 	inIndex)
{
	if (fDragIndex != inIndex)
	{
		if (fDragIndex >= 0) {
			EraseDragLine();
		}
		
		BRect rowRect;
		this->GetRowRect(inIndex, &rowRect);
		this->SetPenSize(2.0);
		// the normal mode is B_OP_COPY, but it doesn't work after section collapsing
		// (if we switch back to B_OP_COPY, then the color should be kDkGray)
		this->SetDrawingMode(B_OP_BLEND);

		this->BeginLineArray(1);
		this->AddLine(rowRect.LeftBottom(), rowRect.RightBottom(), kGrey120);
		this->EndLineArray();

		this->SetDrawingMode(B_OP_COPY);
		this->SetPenSize(1.0);

		fDragIndex = inIndex;
	}
}

// ---------------------------------------------------------------------------
//		EraseDragLine
// ---------------------------------------------------------------------------

void
MProjectView::EraseDragLine()
{
	ASSERT(fDragIndex >= 0);

	
	if (fDragIndex >= 0)
	{
		BRect		rowRect;
	
		GetRowRect(fDragIndex, &rowRect);
		rowRect.bottom++;	// not sure why this is needed but on Intel only
							// it misses by one
		Draw(rowRect);

		fDragIndex = -1;
	}
}

// ---------------------------------------------------------------------------
//		InvokeRow
// ---------------------------------------------------------------------------
//	Called for a double click.

void
MProjectView::InvokeRow(
	int32 inIndex)
{
	MProjectLine*		line = (MProjectLine*) GetList()->ItemAt(inIndex);
	
	ASSERT(line);

	line->Invoke();
}

// ---------------------------------------------------------------------------
//		IsInSystemTree
// ---------------------------------------------------------------------------
//	Is this file anywhere in the system tree?

bool
MProjectView::IsInSystemTree(
	const BEntry& inFile)
{
	FileNameT		fileName;
	entry_ref		ref;
	bool			found = false;

	if (B_NO_ERROR == inFile.GetName(fileName) && 
		B_NO_ERROR == inFile.GetRef(&ref))
	{
		found = MFileUtils::FindFileInDirectoryList(fileName, fSystemDirectories, ref);
	}

	return found;
}

// ---------------------------------------------------------------------------
//		FileIsInProjectByName
// ---------------------------------------------------------------------------
//	Check if the name of the specified file is already in the project.

bool
MProjectView::FileIsInProjectByName(
	BEntry* 		inFile) const
{
	char		newFileName[B_FILE_NAME_LENGTH];
	bool		found = false;

	if (B_NO_ERROR == inFile->GetName(newFileName) &&
		FileIsInProjectByName(newFileName))
		found = true;
	
	return found;
}

// ---------------------------------------------------------------------------
//		FileIsInProjectByName
// ---------------------------------------------------------------------------
//	Check if the name of the specified file is already in the project.

bool
MProjectView::FileIsInProjectByName(
	const char* 		inFileName,
	MSourceFileLine**	outLine) const
{
	ASSERT(inFileName != nil && inFileName[0] != 0);
	bool				result = false;
	MProjectLine*		projectItem;
	int32				i = 0;
	
	while (fFileList.GetNthItem(projectItem, i++))
	{
		MSourceFileLine* item = dynamic_cast<MSourceFileLine*>(projectItem);
		ASSERT(item);
		if (0 == strcmp(inFileName, item->GetFileName()))
		{
			result = true;
			if (outLine != nil)
				*outLine = item;
			break;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		FileIsInProjectByFile
// ---------------------------------------------------------------------------
//	Check if a specified file is already in the project, based on its entry_ref.

bool
MProjectView::FileIsInProjectByFile(
	const BEntry* 		inFile)
{
	LockFileList();			// Prevent concurrent access to fAllFileList
	bool				result = false;
	int32				i = 0;
	MProjectLine*		projectItem;
	MSourceFile*		sourceFile;
	entry_ref			fileRef;
	
	if (B_NO_ERROR == inFile->GetRef(&fileRef))
	{
		while (fFileList.GetNthItem(projectItem, i++))
		{
		MSourceFileLine* item = dynamic_cast<MSourceFileLine*>(projectItem);
		ASSERT(item);
			sourceFile = item->GetSourceFile();
	
			if (sourceFile != nil && fileRef == sourceFile->Ref())
			{
				result = true;
				break;
			}
		}
	}
	
	UnlockFileList();

	return result;
}

// ---------------------------------------------------------------------------

status_t
MProjectView::FindHeader(const HeaderQuery& inQuery, 
						 HeaderReply& outReply, 
						 MSourceFile*& outSourceFile,
						 bool& outFoundInSystemTree)
{
	// Find a header file in this project.  
	// Called from IDE-aware compilers through the MCompileObj.
	
	MSourceFile* sourceFile = this->FindHeader(inQuery.fileName, inQuery.inSysTree, outFoundInSystemTree);
	
	status_t err = B_OK;
	if (sourceFile) {
		outSourceFile = sourceFile;
		if (B_NO_ERROR != sourceFile->GetPath(outReply.filePath, MAX_PATH_LENGTH)) {
			err = B_FILE_NOT_FOUND;
		}
		outReply.errorCode = B_NO_ERROR;
	}
	else {
		outReply.errorCode = err = B_FILE_NOT_FOUND;
	}
	
	return err;
}

// ---------------------------------------------------------------------------

MSourceFile*
MProjectView::FindHeader(const char* fileName, bool inSystemTree, bool& foundInSystemTree)
{
	// Common FindHeader called from FindHeader (above) and from non IDE-aware
	// compilers through GenerateDependencies
	
	MSourceFile* sourceFile = nil;
	MSourceFile* result = nil;
	bool found = false;
	
	// we return the result in a reference parameter - go ahead and use it while we do our work
	foundInSystemTree = false;
	
	LockFileList();
	// First search through our list of files to see if we already 
	// know where this file is	
	int32 index;
	bool searchInSystemTree = ! (fAccessPathsPrefs.pSearchInProjectTreeFirst || !inSystemTree);

	found = fAllFileList.FindItem(fileName, searchInSystemTree, index);
	if (found)
	{
		sourceFile = (MSourceFile*) fAllFileList.ItemAtFast(index);
		foundInSystemTree = sourceFile->IsInSystemTree();
	}
	else
	{
		// If we didn't find it there then search the appropriate
		// directory tree for the file
		entry_ref ref;

		if (! searchInSystemTree)
		{
			found = MFileUtils::FindFileInDirectoryList(fileName, fProjectDirectories, ref);
		}

		if (! found)
		{
			found = MFileUtils::FindFileInDirectoryList(fileName, fSystemDirectories, ref);
			foundInSystemTree = found;
		}

		if (found)
		{
			// If it wasn't where we looked for it then it might still be in 
			// the file list.
			if (foundInSystemTree != searchInSystemTree && 
				fAllFileList.FindItem(fileName, foundInSystemTree, index))
			{
				sourceFile = (MSourceFile*) fAllFileList.ItemAtFast(index);
			}
			else
			{
				sourceFile = new MSourceFile(ref, foundInSystemTree, MSourceFile::kHeaderFileKind, fileName, this);
	
				fAllFileList.AddItem(sourceFile, index);
			}
		}
	}
	
	if (found && sourceFile->FileExists())
	{
		result = sourceFile;
	}
	
	UnlockFileList();

	return result;
}

// ---------------------------------------------------------------------------

MSourceFile*
MProjectView::FindPartialPathHeader(BEntry& inFile)
{
	// FindPartialPathHeader is called from GenerateDependencies when we can't find
	// "foo.h" by FindHeader above.  However, since this file came from the 
	// compiler as an entry_ref, we know the thing exists.  The user probably 
	// did some sort of #include "partialPath/foo.h".  If this is the case, 
	// we need to first see if we can find partialPath/foo.h in the access paths.  
	// If so, we then use this partial name to FindHeader because we don't want 
	// an absolute path in our dependencies.  If we can't find the path in our 
	// access paths, we finally revert to using the full path name.  
	// Notice that this will/might cause recompiles when the project is moved 
	// if that full path doesn't also exist on the target machine.
	
	DirectoryInfo* accessPath = NULL;
	bool inSystemDirectory = false;
	BPath fullPath;
	char pathBuffer[B_PATH_NAME_LENGTH];
	inFile.GetPath(&fullPath);
	strcpy(pathBuffer, fullPath.Path());
	// default is to use the full path name
	const char* lookupPath = pathBuffer;
	
	// first look in both the project directories and system directories
	// (keep track of where we find it so we can speed up the FindHeader)
	if (MFileUtils::FileIsInDirectoriesList(fProjectDirectories, inFile, accessPath) == true) {
		inSystemDirectory = false;
	}
	else if (MFileUtils::FileIsInDirectoriesList(fSystemDirectories, inFile, accessPath) == true) {
		inSystemDirectory = true;
	}

	// create a partial path if we found the file in the accesspath	
	if (accessPath) {
		BEntry dir(&accessPath->dDir);
		status_t err = dir.InitCheck();
		BPath dirPath;
		err = dir.GetPath(&dirPath);
		int32 dirLength = strlen(dirPath.Path());
		if (err == B_OK) {
			lookupPath += (dirLength + 1); // skip over the path to access directory and '/'
		}
	}
	
	// when we get to here, lookupPath contains the smallest path we can manage
	bool dontCareResult;
	MSourceFile* sourceFile = this->FindHeader(lookupPath, inSystemDirectory, dontCareResult);
	return sourceFile;
}

// ---------------------------------------------------------------------------
//		GetFile
// ---------------------------------------------------------------------------

MSourceFile*
MProjectView::GetFile(
	const char*	inName,
	bool		inSystemTree)
{
	HeaderQuery 		query;
	HeaderReply 		reply;
	MSourceFile*		sourceFile = nil;
	bool				foundInSystemTree;

	strcpy(query.fileName, inName);
	query.inSysTree = inSystemTree;

	FindHeader(query, reply, sourceFile, foundInSystemTree);

	return sourceFile;
}

// ---------------------------------------------------------------------------
//		FileSize
// ---------------------------------------------------------------------------
//	MFileGetter override.

off_t
MProjectView::FileSize(
	const char*	inName,
	bool		inSystemTree)
{
	MSourceFile*		sourceFile = GetFile(inName, inSystemTree);
	int32				size = -1;

	if (sourceFile != nil)
	{
		size = sourceFile->Size();
	}

	return size;
}

// ---------------------------------------------------------------------------
//		WriteFileToBlock
// ---------------------------------------------------------------------------
//	MFileGetter override.

status_t
MProjectView::WriteFileToBlock(
	void*		inBlock,
	off_t&		ioSize,
	const char*	inName,
	bool		inSystemTree)
{
	MSourceFile*		sourceFile = GetFile(inName, inSystemTree);
	status_t			result = B_ERROR;

	if (sourceFile != nil)
	{
		entry_ref		ref;
		sourceFile->GetRef(ref);
		MTextWindow*	window = nil;

		if (B_NO_ERROR == sourceFile->GetRef(ref))
			window = MDynamicMenuHandler::FindWindow(ref);

		if (window != nil && window->Lock())
		{
			result = window->WriteToBlock((char*) inBlock, ioSize);
			window->Unlock();
		}
		else
			result = sourceFile->WriteToBlock((char*) inBlock, ioSize);
	}

	return result;
}

// ---------------------------------------------------------------------------
//		FindSourceFile
// ---------------------------------------------------------------------------
//	Find a source file in this project.

bool
MProjectView::FindSourceFile(
	MSourceFile&		inSourceFile)
{
	entry_ref			ref;
	bool				found = false;
	bool				isInSystemTree = inSourceFile.IsInSystemTree();
	const char *		name = inSourceFile.GetFileName();

	LockFileList();	// not exactly the right mutex but protects against
					// accesses while the finder thread is active
	if (! isInSystemTree || fAccessPathsPrefs.pSearchInProjectTreeFirst ||
			inSourceFile.IsSourceFile())
		found = MFileUtils::FindFileInDirectoryList(name, fProjectDirectories, ref);
	if (!found)
		found = MFileUtils::FindFileInDirectoryList(name, fSystemDirectories, ref);

	UnlockFileList();

	if (found)
		inSourceFile.SetRef(ref);
		
	return found;
}

// ---------------------------------------------------------------------------
//		AddHeaderFile
// ---------------------------------------------------------------------------
//	In order to prevent multiple SourceFile objects in the AllFileList for
//	the same header file we provide this utility to SourceFileLines.  They
//	can find out if a given header file already exists in the AllFilesList.
//	inSourceFile is the headerFile to be looked for and outSourceFile is
//	the already-existing header file object.  returns true if it added the
//	header file.  Adds the header file to the list if it's not already there.

bool
MProjectView::AddHeaderFile(	
	MSourceFile*		inSourceFile,
	MSourceFile*&		outSourceFile)
{
	LockFileList();	// Prevent concurrent access to fAllFileList
	bool				found = false;
	const char * 		name = inSourceFile->GetFileName();
	bool				isInSystemTree = inSourceFile->IsInSystemTree();

	// First search through our list of files to see if we already 
	// know where this file is	
	int32		index;

	found = fAllFileList.FindItem(name, isInSystemTree, index);
	if (found)
	{
		outSourceFile = (MSourceFile*) fAllFileList.ItemAtFast(index);
	}
	else
		fAllFileList.AddItem(inSourceFile, index);

	UnlockFileList();

	return ! found;
}

// ---------------------------------------------------------------------------
//		FindHeaderFile
// ---------------------------------------------------------------------------
//	In order to prevent multiple SourceFile objects in the AllFileList for
//	the same header file we provide this utility to SourceFileLines.  They
//	can find out if a given header file already exists in the AllFilesList.
//	inSourceFile is the headerFile to be looked for and outSourceFile is
//	the already-existing header file object.  Adds the header file to the 
//	list if it's not already there and inAddit is true.  Note the default
//	parameter.

MSourceFile*
MProjectView::FindHeaderFile(
	const char * 	inName,	
	bool			inSystemTree,
	bool			inAddit)
{
	LockFileList();	// Prevent concurrent access to fAllFileList
	bool				found = false;
	MSourceFile*		result = nil;
	int32				index;

	found = fAllFileList.FindItem(inName, inSystemTree, index);
	if (found)
	{
		result = (MSourceFile*) fAllFileList.ItemAtFast(index);
	}
	else
	if (inAddit)
	{
		entry_ref		ref;

		result = new MSourceFile(ref, inSystemTree, MSourceFile::kHeaderFileKind, inName, this);
		fAllFileList.AddItem(result, index);
	}

	UnlockFileList();

	return result;
}

MSourceFile*
MProjectView::FindHeaderFile(
	const char * 		inName,	
	SourceFileBlock&	inBlock)
{
	LockFileList();	// Prevent concurrent access to fAllFileList
	bool				found = false;
	MSourceFile*		result = nil;
	int32				index;

	found = fAllFileList.FindItem(inName, inBlock.sSystemTree, index);
	if (found)
	{
		result = (MSourceFile*) fAllFileList.ItemAtFast(index);
	}
	else
	{
		result = new MSourceFile(inName, inBlock, this);
		fAllFileList.AddItem(result, index);
	}

	UnlockFileList();

	return result;
}

// ---------------------------------------------------------------------------
//		FillCompileList
// ---------------------------------------------------------------------------
//	Fill the compile list with the MSourceFileLine objects that are selected.
// 	and which can be executed.  usesFileCache indicates if the targets for any 
//	of the files in the list use the file cache.

void
MProjectView::FillCompileList(
	BList& 		inList,
	bool&		outUsesFileCache)
{
	inList.MakeEmpty();

	bool		usesFileCache = false;
	int32		theRow = -1;

	Window()->Lock();
	LockFileList();

	while (NextSelected(theRow))
	{
		MProjectLine*		line = (MProjectLine*) GetList()->ItemAt(theRow);
		MSectionLine*		section = dynamic_cast<MSectionLine*>((MProjectLine*)GetList()->ItemAt(theRow));

		if (section != nil)
		{
			// Add all the lines in a section
			int32				count = section->GetLines();
			int32				index = fFileList.IndexOf(section->GetFirstLine());
			MSourceFileLine *	aline;
			
			ASSERT(index >= 0);
		
			for (int i = 0; i < count; i++)
			{
				aline = static_cast<MSourceFileLine*>(fFileList.ItemAt(index+i));
				ASSERT(aline != NULL);
				if (!inList.HasItem(aline) && aline->CanBeExecuted())
				{
					inList.AddItem(aline);
					if (!usesFileCache)
						usesFileCache = aline->UsesFileCache();
				}
			}
		}
		else
		{
			MSourceFileLine*	sourceLine = static_cast<MSourceFileLine*>(line);
			if (! inList.HasItem(sourceLine) && sourceLine->CanBeExecuted()) 	// Add just this line
			{
				inList.AddItem(sourceLine);
				if (!usesFileCache)
					usesFileCache = sourceLine->UsesFileCache();
			}
		}
	}

	outUsesFileCache = usesFileCache;

	UnlockFileList();
	Window()->Unlock();
}

// ---------------------------------------------------------------------------

void
MProjectView::OpenCurrentSelection()
{
	// open the current selection (in the BeIDE even if external editor in use)
	// by sending B_REFS_RECEIVED to application
	
	LockFileList();
	int32 theRow = -1;

	while (this->NextSelected(theRow)) {
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>((MProjectLine*)GetList()->ItemAt(theRow));
		if (line != nil) {
		
			MSourceFile* sourceFile = line->GetSourceFile();
			entry_ref ref;
			if (sourceFile != nil && sourceFile->GetRef(ref) == B_OK) {				
				// Build the message and send to application
				BMessage msg(B_REFS_RECEIVED);
				msg.AddRef("refs", &ref);
				be_app_messenger.SendMessage(&msg);
			}
		}
	}

	UnlockFileList();
}

// ---------------------------------------------------------------------------
//		RevealSelectionInTracker
// ---------------------------------------------------------------------------
//	Open the parent dir in the Tracker.

void
MProjectView::RevealSelectionInTracker()
{
	LockFileList();
	BMessenger		tracker(kTrackerSig);
	int32		theRow = -1;

	while (NextSelected(theRow))
	{
		MSourceFileLine*		line = dynamic_cast<MSourceFileLine*>((MProjectLine*)GetList()->ItemAt(theRow));

		if (line != nil)
		{
			MSourceFile*		sourceFile = line->GetSourceFile();
			entry_ref			ref;

			if (sourceFile != nil && B_NO_ERROR == sourceFile->GetRef(ref))
			{
				BEntry		entry(&ref);
				BDirectory	parent;
				
				entry.GetParent(&parent);
				parent.GetEntry(&entry);
				entry.GetRef(&ref);
				
				// Build the message
				BMessage		msg(B_REFS_RECEIVED);
				
				msg.AddRef("refs", &ref);						// the file's ref
			
				tracker.SendMessage(&msg);
			}
		}
	}

	UnlockFileList();
}

// ---------------------------------------------------------------------------
//		DoAndyFeature
// ---------------------------------------------------------------------------
//	Respond to alt-tab in this window.  Ask the app to do the work.
//	Can't do anything for an unsaved file.

void 
MProjectView::DoAndyFeature()
{
	int32		theRow = -1;

	LockFileList();

	while (NextSelected(theRow))
	{
		MSourceFileLine*		line = dynamic_cast<MSourceFileLine*>((MProjectLine*)GetList()->ItemAt(theRow));

		if (line != nil)
		{
			MSourceFile*		sourceFile = line->GetSourceFile();
			entry_ref			ref;

			if (sourceFile != nil && B_NO_ERROR == sourceFile->GetRef(ref))
			{		
				// Build the message
				BMessage msg(cmd_AndyFeature);
				msg.AddRef(kTextFileRef, &ref);								// the file's ref
				msg.AddString(kFileName, sourceFile->GetFileName());		// the filename
				msg.AddPointer(kProjectMID, (MProjectWindow*)Window());		// our project
				be_app_messenger.SendMessage(&msg);
			}
		}
	}

	UnlockFileList();
}

// ---------------------------------------------------------------------------
//		OpenSelection
// ---------------------------------------------------------------------------
//	Called by the app in response to a cmd-d.  We search our file list
//	for the file.  If we can't find it we search the disk in the project tree.
//	If not found the app will search the system tree.
//	The last two bools specify whether the search should include the inMemory
//	lists or should include the disk.
//	inSystemTree indicates search in system tree only or in project tree only.

bool
MProjectView::OpenSelection(
	const char * 	inName,
	bool			inSystemTree,
	entry_ref&		outRef,
	bool			inSearchInMemory,
	bool			inSearchOnDisk)
{
	LockFileList();	// Prevent concurrent access to fAllFileList
	bool				found = false;
	int32				index;

	if (inSearchInMemory)
	{
		// Look through our file list to see if we know about this file
		found = fAllFileList.FindItem(inName, inSystemTree, index);
		if (! found && ! inSystemTree)
			found = fAllFileList.FindItem(inName, false, index);
	}

	if (found)
	{
		MSourceFile*		source = (MSourceFile*) fAllFileList.ItemAtFast(index);

		if (B_NO_ERROR != source->GetRef(outRef))
			found = false;
	}
	else
	if (inSearchOnDisk)
	{
		if (inSystemTree)
			found = MFileUtils::FindFileInDirectoryList(inName, fSystemDirectories, outRef);
		else
			found = MFileUtils::FindFileInDirectoryList(inName, fProjectDirectories, outRef);
	}

	UnlockFileList();

	return found;
}

// ---------------------------------------------------------------------------
//		FindDefinition
// ---------------------------------------------------------------------------
//	Called either from a text window or from the find definition window.

void
MProjectView::FindDefinition(
	BMessage&	inMessage)
{
	const char * 	token = nil;
	MTextWindow *	wind = nil;
	
	(void) inMessage.FindPointer(kTextWindow, (void**) &wind);		// optional parameter

	if (B_NO_ERROR == inMessage.FindString(kText, &token))
	{
		if (token == nil || token[0] == 0)
		{
			// Open find definition window
			if (fFindDefinitionWindow)
				fFindDefinitionWindow->Activate();
			else
			{
				fFindDefinitionWindow = new MFindDefinitionWindow((MProjectWindow&)*Window());
				fFindDefinitionWindow->Show();
			}
		}
		else
		if (fFindDefinitionTask == nil)
		{
			fFindDefinitionTask = new MFindDefinitionTask(token, *this, wind);
			fFindDefinitionTask->Run();
		}
	}
}

// ---------------------------------------------------------------------------
//		ExecuteFindDefinition
// ---------------------------------------------------------------------------
//	Called from the FindDefinitionTask.

void
MProjectView::ExecuteFindDefinition(
	const char *			inToken,
	MTextWindow *			inWindow,
	MFindDefinitionTask&	inTask)
{
	ASSERT(inToken != 0 && inToken[0] != 0);
	InfoStructList		list;
	InfoStruct*			info;
	MSourceFileLine*	line = nil;
	MSourceFileLine*	line1 = nil;
	bool				found;
	bool				cancelled = false;
	int32				i = 0;
	
	if (inWindow == nil)
	{
		inWindow = MDynamicMenuHandler::GetFrontTextWindow();
	}
	
	if (inWindow)
	{
		LockFileList();
		found = GetSourceFileLineByName(inWindow->Title(), line1);
		UnlockFileList();

		if (found)
			line1->SearchObjectDef(inToken, true, list);
	}

	while (true)
	{
		LockFileList();
		MProjectLine* projectLine = nil;
		found = fFileList.GetNthItem(projectLine, i++);
		UnlockFileList();
		cancelled = inTask.Cancelled();
		if (!found || cancelled)
			break;
		line = dynamic_cast<MSourceFileLine*>(projectLine);	
		if (line != line1)
			line->SearchObjectDef(inToken, false, list);
	}

	if (! cancelled)
	{
		// Save the current window in the goback list
		if (list.CountItems() > 0 && inWindow != nil && inWindow->Lock())
		{
			inWindow->PutGoBack();
			inWindow->Unlock();
		}
		
		// Display the result, if any
		switch (list.CountItems())
		{
			case 0:
				// Didn't find anything
				beep();
				break;
			
			case 1:
				// Tell the app to open the window and select the token
				if (list.GetNthItem(info, 0))
				{
					BMessage		msg(msgOpenSourceFile);

					msg.AddRef("refs", info->iRef);
					msg.AddData(kTokenIdentifier, kTokenIDType, &info->iToken, sizeof(info->iToken));
					be_app_messenger.SendMessage(&msg);
				}
				break;
			
			default:
				// Send all the token identifiers to a message result window
				MMessageWindow* messageWindow = new MInformationMessageWindow("Multiple Definitions", inToken);
				i = 0;
				
				while (list.GetNthItem(info, i++))
				{
					BMessage msg(msgAddDefinitionToMessageWindow);
					msg.AddData(kInfoStruct, kInfoType, info, sizeof(InfoStruct));

					// Post the message
					messageWindow->PostMessage(&msg);			
				}

				messageWindow->PostMessage(msgShowAndActivate);
				break;
		}

		if (Window()->Lock())
		{
			fFindDefinitionTask = nil;
			Window()->Unlock();
		}
	}

	// Empty the list
	i = 0;
	while (list.GetNthItem(info, i++))
	{
		delete info;
	}
}

// ---------------------------------------------------------------------------
//		SetWorkingDirectory
// ---------------------------------------------------------------------------
//	Update the default folder for POSIX tools.

void
MProjectView::SetWorkingDirectory()
{
	BPath			filePath;
	BEntry			projDir;

	if (B_NO_ERROR == fProjectDirectory.GetEntry(&projDir) &&
		B_NO_ERROR <= projDir.GetPath(&filePath))
	{
		chdir(filePath.Path());
	}

	MFileUtils::BuildDirectoriesList(fProjectPathList, fProjectDirectories, &fProjectDirectory);
	MFileUtils::BuildDirectoriesList(fSystemPathList, fSystemDirectories, &fProjectDirectory);
}

// ---------------------------------------------------------------------------
//		ResetPrecompiledHeaderFile
// ---------------------------------------------------------------------------
//	The compiler deletes the existing precompiled header file and writes a
//	new file so we have to reset the file path so the new copy is refound

void
MProjectView::ResetPrecompiledHeaderFile(
	const char*		inFileName)
{
	String			name = inFileName;

	int32			offset = name.ROffsetOf('.');
	if (offset >= 0)
		name.Replace("", offset, name.GetLength() - offset);
	
	MSourceFile* 	sourceFile = FindHeaderFile(name, true, false);

	if (! sourceFile)
		sourceFile = FindHeaderFile(name, false, false);

	if (sourceFile)
		sourceFile->ResetFilePath();
}

// ---------------------------------------------------------------------------
//		TouchAllSourceFiles
// ---------------------------------------------------------------------------
//	Touch all the source files after doing a precompile.

void
MProjectView::TouchAllSourceFiles(
	bool	inDoAll)
{
	int32			count = 0;
	MProjectLine*	projectLine;

	while (fFileList.GetNthItem(projectLine, count++))
	{
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		SuffixType		suffix = line->GetSuffix();
		
		if (inDoAll || suffix == kCPSuffix || suffix == kCSuffix)
		{
			line->Touch();
		}
	}
}

// ---------------------------------------------------------------------------
//		BuildPrecompileArgv
// ---------------------------------------------------------------------------

status_t
MProjectView::BuildPrecompileArgv(
	BuilderRec*		inBuilderRec,
	StringList&		inArgv,
	MFileRec& 		inFileRec)
{
	return fBuildCommander->BuildPrecompileArgv(inBuilderRec, inArgv, inFileRec);
}

// ---------------------------------------------------------------------------
//		BuildCompileArgv
// ---------------------------------------------------------------------------

status_t
MProjectView::BuildCompileArgv(
	MakeActionT 	inAction,
	BuilderRec*		inBuilderRec,
	StringList&		inArgv,
	MFileRec& 		inFileRec)
{
	return fBuildCommander->BuildCompileArgv(inAction, inBuilderRec, inArgv, inFileRec);
}

// ---------------------------------------------------------------------------
//		BuildPostLinkArgv
// ---------------------------------------------------------------------------

status_t
MProjectView::BuildPostLinkArgv(
	BuilderRec*		inBuilderRec,
	StringList&		inArgv,
	MFileRec& 		inFileRec)
{
	return fBuildCommander->BuildPostLinkArgv(inBuilderRec, inArgv, inFileRec);
}

// ---------------------------------------------------------------------------
//		BuildPostLinkArgv
// ---------------------------------------------------------------------------
//	(added by John Dance)

status_t
MProjectView::ParseLinkerMessageText(
	const char*	inText,
	BList& 		outList)
{
	return fBuildCommander->ParseLinkerMessageText(inText, outList);
}

// ---------------------------------------------------------------------------
//		CompilingState
// ---------------------------------------------------------------------------

CompileState 
MProjectView::CompilingState()
{
	return fBuildCommander->CompilingState();
}

// ---------------------------------------------------------------------------
//		IsIdle
// ---------------------------------------------------------------------------

bool					
MProjectView::IsIdle()
{
	return fBuildCommander->IsIdle();
}

// ---------------------------------------------------------------------------

void
MProjectView::SetBuildParameters(const BuildExtrasPrefs& buildparams)
{
	fBuildCommander->SetConcurrentCompiles(buildparams.concurrentCompiles);
	fBuildCommander->SetStopOnErrors(buildparams.stopOnError);
	fBuildCommander->SetBuildPriority(buildparams.compilePriority);
	fBuildCommander->SetAutoOpenErrorWindow(buildparams.openErrorWindow);
}
	
// ---------------------------------------------------------------------------
//		CompileDone
// ---------------------------------------------------------------------------
//	Called from the compiler generator object when it's done compiling.

void
MProjectView::CompileDone()
{
	fBuildCommander->CompileDone();
}

// ---------------------------------------------------------------------------
//		LastState
// ---------------------------------------------------------------------------
//	Reset the compiling state to idle.

void
MProjectView::LastState()
{
	ShowIdleStatus();
}

// ---------------------------------------------------------------------------
//		OneCompileDone
// ---------------------------------------------------------------------------
//	Called from each file compiler object when it's done compiling.

void
MProjectView::OneCompileDone(
	BMessage&	inMessage)
{
	MSourceFileLine*	line;

	if (B_NO_ERROR == inMessage.FindPointer(kProjectLine, (void**) &line))
	{
		int32				index = IndexOf(line);
		
		if (index >=0)
			UpdateCodeDataRect(index, true);
	}

	fBuildCommander->OneCompileDone();
}

// ---------------------------------------------------------------------------
//		LinkDone
// ---------------------------------------------------------------------------
//	Link is completed.  Called from the LinkObject.

void
MProjectView::LinkDone()
{
	fCompileObj = nil;

	CompileDone();
}

// ---------------------------------------------------------------------------
//		LinkDone
// ---------------------------------------------------------------------------
//	Link is completed.  Called from the BuildCommander so that we can notify
//	the linker object.

void
MProjectView::LinkDone(
	MPlugInLinker*	inLinker)
{
	if (Window()->Lock())
	{
		inLinker->ProjectChanged(*fProject, kLinkDone);
		Window()->Unlock();
	}
}

// ---------------------------------------------------------------------------
//		PreCompileDone
// ---------------------------------------------------------------------------
//	Precompile is completed.  Called from the PreCompileObject.

void
MProjectView::PreCompileDone()
{
	fCompileObj = nil;
	
	CompileDone();
}

// ---------------------------------------------------------------------------
//		RemoveObjects
// ---------------------------------------------------------------------------
//	Deletes the object file for each source file in the project.  Only
//	looks in the objectfiledirectory for the object files.
//	Also removes any browse data and if RemovingAllObjects deletes the
//	file dependencies as well.

void
MProjectView::RemoveObjects(
	uint32	inRemoveCommand)
{
	MProjectLine*		projectLine;
	int32				i = 0;
	bool				removeAll = inRemoveCommand == cmd_RemoveBinariesCompact;

	while (fFileList.GetNthItem(projectLine, i++))
	{
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		line->DeleteObjectFile();
		line->RemoveObjects(removeAll);
	}

	UpdateCodeDataRect(-1);
	SaveChanges();
}

// ---------------------------------------------------------------------------
//		UpdateCodeDataRect
// ---------------------------------------------------------------------------
//	The code/data for a file has changed so the rect containing these areas
//	needs to be invalidated.  inIndex is the index of the sourceFileLine in
//	this view.  If index isn't valid then invalidate the rect
//	for all files.  If inCheckMarkcolumn is true then invalidate the
//	checkmark column also.  Note the default paramter.

void
MProjectView::UpdateCodeDataRect(
	int32 inIndex,
	bool inCheckMarkColumn)
{
	BRect		invalRect;

	// Is index valid?
	if (inIndex >= 0 && inIndex < CountRows())
	{
		UpdateRow(inIndex);
		GetRowRect(inIndex, &invalRect);
	}
	else
	{
		UpdateBitmap();
		invalRect = Bounds();
	}

	invalRect.right = fProjectLinePainter.GetDataRight();
	invalRect.left = fProjectLinePainter.GetNameRight();

	Invalidate(invalRect);

	if (inCheckMarkColumn)
	{
		invalRect.right = kCheckMarkRight;
		invalRect.left = kCheckMarkLeft;
		Invalidate(invalRect);
	}
}

// ---------------------------------------------------------------------------
//		ResetFilePaths
// ---------------------------------------------------------------------------
//	Place all of the sourceFile objects in a state such that they will have
//	to refind their entry_refs the next time that they are accessed.  This
//	doesn't affect the dependency info.

void
MProjectView::ResetFilePaths()
{
	LockFileList();			// Prevent concurrent access to fAllFileList

	// Just triggering the refinding of entry_refs isn't enough 
	// Perhaps directories have been moved around, or volumes mounted
	// In any of these cases, the fSystemDirectories and fProjectDirectories
	// could be stale.  Besides triggering all sourceFile objects, reconstruct
	// the fSystemDirectories and fProjectDirectories
	
	MFileUtils::EmptyDirectoryList(fSystemDirectories, &fProjectDirectory);
	MFileUtils::EmptyDirectoryList(fProjectDirectories, &fProjectDirectory);

	if (fAccessPathsPrefs.pSystemPaths > 0) {
		MFileUtils::BuildDirectoriesList(fSystemPathList, fSystemDirectories, &fProjectDirectory);
	}
	if (fAccessPathsPrefs.pProjectPaths > 0) {
		MFileUtils::BuildDirectoriesList(fProjectPathList, fProjectDirectories, &fProjectDirectory);
	}
	
	MSourceFile* sourceFile;
	int32 i = 0;

	while (fAllFileList.GetNthItem(sourceFile, i++)) {
		sourceFile->ResetFilePath();
	}

	UnlockFileList();

	StartFinder();
}

// ---------------------------------------------------------------------------
//		BuildHeaderPopup
// ---------------------------------------------------------------------------
//	Build the header popup menu for the specified window.

void
MProjectView::BuildPopupMenu(
	const char *	inName,
	MPopupMenu& 	inPopup)
{
	MSourceFileLine*		line;

	if (GetSourceFileLineByName(inName, line))
	{
		line->BuildPopupMenu(inPopup);
	}
}

// ---------------------------------------------------------------------------
//		TouchFile
// ---------------------------------------------------------------------------

void
MProjectView::TouchFile(
	BMessage&	inMessage)
{
	if (inMessage.HasString(kFileName))
	{
		const char *		filename = inMessage.FindString(kFileName);
		
		MSourceFileLine*	line;
		
		if (GetSourceFileLineByName(filename, line))
		{
			line->Touch();
			SetDirty();
		}
	}
}

// ---------------------------------------------------------------------------
//		GetSourceFileLineByName
// ---------------------------------------------------------------------------

bool
MProjectView::GetSourceFileLineByName(
	const char* 		inName, 
	MSourceFileLine* &	outLine)
{
	MProjectLine*		projectItem;
	bool				found = false;	
	int32				i = 0;

	while (fFileList.GetNthItem(projectItem, i++))
	{
		MSourceFileLine* item = dynamic_cast<MSourceFileLine*>(projectItem);
		ASSERT(item);
		const char*	foundFileName;
		
		foundFileName = item->GetFileName();
		if (0 == strcmp(foundFileName, inName))
		{
			found = true;
			outLine = item;
			break;
		}
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//		GetSourceFileLineByRef
// ---------------------------------------------------------------------------

bool
MProjectView::GetSourceFileLineByRef(
	entry_ref& 			inRef, 
	MSourceFileLine* &	outLine)
{
	MProjectLine*		projectItem;
	bool				found = false;	
	int32				i = 0;
	entry_ref			lineRef;

	while (fFileList.GetNthItem(projectItem, i++))
	{
		MSourceFileLine* item = dynamic_cast<MSourceFileLine*>(projectItem);
		ASSERT(item);
		if (B_NO_ERROR == item->GetSourceFile()->GetRef(lineRef)
			&& inRef == lineRef)
		{
			found = true;
			outLine = item;
			break;
		}
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//		ShowFileCount
// ---------------------------------------------------------------------------
//	Show the number of files in the status caption.

void
MProjectView::ShowFileCount()
{
	BMessage		msg(msgSetCaptionText);
	int32			count = fFileList.CountItems();
	String			text;

	if (count == 1)
	{
		text = "1 file";
	}
	else
	{
		text = count;
		text += " files";
	}

	msg.AddString(kText, text);

	Window()->PostMessage(&msg);
}

// ---------------------------------------------------------------------------

void
MProjectView::ShowIdleStatus()
{
	// Clear out all text from the status caption
	BMessage		msg(msgSetStatusText);
	String			text = " ";
	msg.AddString(kText, text);

	Window()->PostMessage(&msg);
}

// ---------------------------------------------------------------------------
//		SetDirty
// ---------------------------------------------------------------------------
//	For minor changes just set dirty.  When the project window is closed these
//	changes will be saved.

void
MProjectView::SetDirty()
{
	Window()->PostMessage(cmdSetDirty);
}

// ---------------------------------------------------------------------------
//		SaveChanges
// ---------------------------------------------------------------------------
//	For substantial changes save immediately.

void
MProjectView::SaveChanges()
{
	Window()->PostMessage(cmdSetDirty);
	Window()->PostMessage(cmd_Save);
}

// ---------------------------------------------------------------------------
//		StartFinder
// ---------------------------------------------------------------------------

void
MProjectView::StartFinder()
{
	// Start the find files thread object
	// Kill the current version if it is still running
	
	if (fFinder) {
		delete fFinder;
		fFinder = nil;
	}

	fFinder = new MFindFilesThread(this);
	status_t err = fFinder->Run();

	if (err != B_NO_ERROR) {
		delete fFinder;
		fFinder = nil;
		ASSERT(B_NO_ERROR == err);
	}
}

// ---------------------------------------------------------------------------
//		FindAllFiles
// ---------------------------------------------------------------------------
//	This function is called from the find file object.

void
MProjectView::FindAllFiles(
	MFindFilesThread&	inThread,
	NodeList&			inNodeList)
{
	LockFileList();
	MSourceFileList			allFileList(fAllFileList);	// Copy of all the files
	UnlockFileList();

	MSourceFileList			slashFileList;				// List of files with slashes in their names
	MSourceFile*			sourceFile;

	for (int32 i = allFileList.CountItems() - 1; i >= 0; i--)
	{
		sourceFile = allFileList.ItemAtFast(i);

		if (0 != strchr(sourceFile->GetFileName(), '/'))
		{
			allFileList.RemoveItemAt(i);	// remove by index
			slashFileList.AddItem(sourceFile);
		}
	}
	
	FindFilesInDirectoryList(allFileList, slashFileList, fProjectDirectories, inNodeList, false, inThread);
	FindFilesInDirectoryList(allFileList, slashFileList, fSystemDirectories, inNodeList, true, inThread);

	Window()->PostMessage(msgAllFilesFound);
}

// ---------------------------------------------------------------------------
//		FindFilesInDirectoryList
// ---------------------------------------------------------------------------

void
MProjectView::FindFilesInDirectoryList(
	MSourceFileList&	inFileList,
	MSourceFileList&	inSlashFileList,
	DirectoryList&		inDirList,
	NodeList&			inNodeList,
	bool				inIsSystem,
	MFindFilesThread&	inThread)
{
	DirectoryInfo*	info;
	int32			i = 0;

	while (inDirList.GetNthItem(info, i++)) {
		// Notice: Use BAutolock here so that an exception being
		// thrown in FindFilesInDirectory will unlock as it passes through...
		BAutolock localFileLocker(fFileListLocker);
		BDirectory dir(&info->dDir);
		inThread.FindFilesInDirectory(inFileList, inSlashFileList, dir, inNodeList, info->dRecursiveSearch, inIsSystem);
	}
}

// ---------------------------------------------------------------------------
//		FindOneFile
// ---------------------------------------------------------------------------
//	This function is called from the find file object.

bool
MProjectView::FindOneFile(
	int32	&ioCount)
{
	LockFileList();
	bool				result = false;
	
	if (ioCount < fAllFileList.CountItems())
	{
		MSourceFile*		sourceFile;

		if (fAllFileList.GetNthItem(sourceFile, ioCount++))
		{
			sourceFile->FileExists();			// force it to find the file
		}

		result = true;
	}
	
	UnlockFileList();

	return result;
}

// ---------------------------------------------------------------------------
//		UpdateOneFile
// ---------------------------------------------------------------------------
//	This function is called from the find file object.  return true if a 
//	file was found.

bool
MProjectView::UpdateOneFile(
	int32	&ioCount)
{
	Window()->Lock();
	LockFileList();
	bool				result = false;
	
	if (ioCount < fFileList.CountItems())
	{
		MProjectLine*		sourceLine = fFileList.ItemAtFast(ioCount);
		MSourceFileLine*	line = dynamic_cast<MSourceFileLine*>(sourceLine);
		
		if (line && line->UpdateType(fTargetPrefs.pTargetArray, fTargetPrefs.pCount))
		{
			UpdateOneTarget(line);
			int32	index = IndexOf(line);
			if (index >= 0)
				InvalidateRow(index);
			SetDirty();
		}

		result = true;
		ioCount++;
	}
	
	UnlockFileList();
	Window()->Unlock();

	return result;
}

// ---------------------------------------------------------------------------
//		UpdateAllTargets
// ---------------------------------------------------------------------------
//	The target panel has been changed so we need to update the targets for all
//	files in the project.

void
MProjectView::UpdateAllTargets()
{
	MProjectLine*	projectLine;
	int32			i = 0;

	while (fFileList.GetNthItem(projectLine, i++))
	{
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		UpdateOneTarget(line);
	}
}

// ---------------------------------------------------------------------------
//		UpdateOneTarget
// ---------------------------------------------------------------------------
//	The target panel has been changed so we need to update the targets for all
//	files in the project.

void
MProjectView::UpdateOneTarget(
	MSourceFileLine*	inLine)
{
	const char *		extension = strrchr(inLine->Name(), '.');
	if (extension != nil)
		extension++;
	else
		extension = "";

	ProjectTargetRec*	rec = fBuildCommander->GetTargetRec(inLine->MimeType(), extension);
	
//	ASSERT(rec);
	if (rec == nil)
		inLine->SetTarget(nil);
	else
	{
		if (rec->Target.Stage == inLine->CompileStage())
		{
			// Just update it
			inLine->SetTarget(rec);
		}
		else
		{
			// Replace it
			MSourceFile*	sourcefile = inLine->GetSourceFile();
			entry_ref		ref;
			ASSERT(sourcefile);
			if (sourcefile && B_NO_ERROR == sourcefile->GetRef(ref))
			{
				BEntry				file(&ref);
				MSectionLine*		section = inLine->GetSection();
				String				name = sourcefile->GetFileName();
				MSourceFileLine*	newLine = GetLine(file, *section, rec, name);
				int32				index = IndexOf(inLine);
				int32				indexInFileList = GetIndexInFileList(*section, index);

				if (section->IsExpanded())
				{
					InsertRow(index, newLine);
					UpdateBitmap(index);
				}
				fFileList.AddItem(newLine, indexInFileList);
			
				section->AddLine(newLine);	// Notify the section line
				RemoveFile(inLine, false);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		InvalidateModificationDates
// ---------------------------------------------------------------------------

void
MProjectView::InvalidateModificationDates()
{
	LockFileList();			// Prevent concurrent access to fAllFileList

	MSourceFile*		sourceFile;
	int32				i = 0;

	while (fAllFileList.GetNthItem(sourceFile, i++))
	{
		sourceFile->InvalidateModTime();
	}

	UnlockFileList();
}

// ---------------------------------------------------------------------------
//		Pulse
// ---------------------------------------------------------------------------

void
MProjectView::Pulse()
{
	// Check if any CompileServe threads still are running.
	// If not, turn off pulse and set the state to idle.
	if (CompilingState() == sCancelling &&
		B_NAME_NOT_FOUND == find_thread("CompileServe"))
	{
		PulseOff(this);
		fBuildCommander->CompileDone();
	}

	if (fTypingAhead && system_time() > fKeyDownTime)
	{
		// Remove the search string from the info view
		if (CompilingState() != sCancelling && ! IsAutoScrolling())
			PulseOff(this);

		if (CompilingState() == sNotCompiling)
		{
			this->ShowIdleStatus();
		}
		else
		{
			fBuildCommander->ShowCompileStatus();
		}
		
		fTypingAhead = false;
	}

	MListView::Pulse();
}

// ---------------------------------------------------------------------------
//		WriteToFile
// ---------------------------------------------------------------------------

void
MProjectView::WriteToFile(
	MBlockFile & inFile)
{
	// Write out all the preferences blocks
	status_t		err;
	uint32			version = B_HOST_TO_BENDIAN_INT32(kCurrentProjectVersion);
	int32			endianflag;		// indicates endianess of the last host this project was written out on
	if (B_HOST_IS_BENDIAN)
		endianflag = 0;				// zero and -1 are the same on both endianesses
	else
		endianflag = -1;

	err = inFile.StartBlock(kPreferencesBlockType);		// Start prefs block
	ThrowIfError_(err);

	err = inFile.StartBlock(kEndianBlockType);
	ThrowIfError_(err);
	err = inFile.PutBytes(sizeof(endianflag), &endianflag);
	ThrowIfError_(err);
	err = inFile.EndBlock();
	ThrowIfError_(err);
	err = inFile.StartBlock(kProjectVersionBlockType);
	ThrowIfError_(err);
	err = inFile.PutBytes(sizeof(version), &version);
	ThrowIfError_(err);
	err = inFile.EndBlock();
	ThrowIfError_(err);

	err = inFile.StartBlock(kPrivateBlockType);
	ThrowIfError_(err);
	fPrivatePrefs.SwapHostToBig();
	err = inFile.PutBytes(sizeof(fPrivatePrefs), &fPrivatePrefs);
	fPrivatePrefs.SwapBigToHost();
	ThrowIfError_(err);
	err = inFile.EndBlock();
	ThrowIfError_(err);

	// --- Run Preferences --
	err = inFile.StartBlock(kRunPrefsBlockType);
	ThrowIfError_(err);
	fRunPrefs.SwapHostToBig();
	err = inFile.PutBytes(sizeof(fRunPrefs), &fRunPrefs);
	fRunPrefs.SwapBigToHost();
	ThrowIfError_(err);
	err = inFile.EndBlock();
	ThrowIfError_(err);

	err = inFile.StartBlock(kAccessPathsBlockType);
	ThrowIfError_(err);
	fAccessPathsPrefs.SwapHostToBig();
	err = inFile.PutBytes(sizeof(fAccessPathsPrefs), &fAccessPathsPrefs);
	fAccessPathsPrefs.SwapBigToHost();
	ThrowIfError_(err);
	err = inFile.EndBlock();
	ThrowIfError_(err);

	// Write out all the access paths
	AccessPathData*		accessPath;
	int32				i = 0;
	while (fSystemPathList.GetNthItem(accessPath, i++))
	{
		err = inFile.StartBlock(kSystemPathBlockType);
		ThrowIfError_(err);
		accessPath->SwapHostToBig();
		err = inFile.PutBytes(sizeof(AccessPathData), accessPath);
		accessPath->SwapBigToHost();
		ThrowIfError_(err);
		err = inFile.EndBlock();
		ThrowIfError_(err);
	}
	i = 0;
	while (fProjectPathList.GetNthItem(accessPath, i++))
	{
		err = inFile.StartBlock(kProjectPathBlockType);
		ThrowIfError_(err);
		accessPath->SwapHostToBig();
		err = inFile.PutBytes(sizeof(AccessPathData), accessPath);
		accessPath->SwapBigToHost();
		ThrowIfError_(err);
		err = inFile.EndBlock();
		ThrowIfError_(err);
	}

	// File Sets
	// swap the header inline
	if (fFileSets)
	{
		err = inFile.StartBlock(kFileSetBlockType);
		ThrowIfError_(err);
		// the start of the blob that is fFileSets is the
		// FileSetHeader - swap that to big for storage
		FileSetHeader* fileSetHeader = (FileSetHeader*) fFileSets;
		fileSetHeader->SwapHostToBig();
		err = inFile.PutBytes(fFileSetsSize, fFileSets);
		fileSetHeader->SwapBigToHost();
		ThrowIfError_(err);
		err = inFile.EndBlock();
		ThrowIfError_(err);
	}
	
	// Targets
	TargetPrefs		targetprefs = fTargetPrefs;		// shallow copy
	err = inFile.StartBlock(kTargetsBlockType);
	ThrowIfError_(err);
	targetprefs.SwapHostToBig();		// swaps the target rec array
	err = inFile.PutBytes(sizeof(fTargetPrefs), &targetprefs);
//	ThrowIfError_(err);

	if (err == B_OK && fTargetPrefs.pCount > 0)
	{
		err = inFile.PutBytes(fTargetPrefs.pCount * sizeof(TargetRec), fTargetPrefs.pTargetArray);
		ThrowIfError_(err);
	}
	targetprefs.SwapBigToHost();	// swaps the target rec array
	err = inFile.EndBlock();		// Targets block
	ThrowIfError_(err);

	fBuildCommander->WriteToFile(inFile);

	err = inFile.EndBlock();		// End the preferences block
	ThrowIfError_(err);

	// Write out all the section blocks
	// Each section block writes out its own sourcefile blocks
	int32			numSections = fSectionList.CountItems();
	
	for (int32 i = 0; i < numSections; i++)
	{
		MSectionLine*		section = (MSectionLine*) fSectionList.ItemAt(i);
		
		section->WriteToFile(inFile);
	}
}

// ---------------------------------------------------------------------------
//		ReadFromFile
// ---------------------------------------------------------------------------

void
MProjectView::ReadFromFile(
	MBlockFile & inFile)
{	
	BlockType			type;
	MSectionLine*		currentSection = nil;
	bool				expanded;
	bool				corrupt = false;
	uint32				version = 0;

	//	Scan file for data to fill in
	while (B_NO_ERROR == inFile.ScanBlock(type) && ! corrupt) 
	{
		MSourceFileLine*	currentSourceLine = nil;

		switch (type) 
		{
			case kPreferencesBlockType:
				version = ReadPreferences(inFile);
				break;

			case kSectionBlockType:
				currentSection = new MSectionLine(*this, fFileList, inFile);
				expanded = currentSection->IsExpanded();
				InsertRow(CountRows(), currentSection);
				fSectionList.AddItem(currentSection);
				break;

			case kIgnoreFileBlockType:
				currentSourceLine = 
					new MIgnoreFileLine(*currentSection, *this, inFile, type, version);
				break;

			case kPrecompileFileBlockType:
			case kPCHFileBlockTypeOld:
				currentSourceLine = 
					new MPCHFileLine(*currentSection, *this, inFile, type, version);
				break;

			case kCompileFileBlockType:
			case kFileBlockTypeOld:
				currentSourceLine = 
					new MSourceFileLine(*currentSection, *this, inFile, type, version);
				break;

			case kLinkFileBlockType:
			case kLibFileBlockTypeOld:
				currentSourceLine = 
					new MLibraryFileLine(*currentSection, *this, inFile, type, version);
				break;

			case kPostLinkFileBlockType:
				currentSourceLine = 
					new MPostLinkFileLine(*currentSection, *this, inFile, type, version);
				break;

			case kSubProjectFileBlockType:
				currentSourceLine =
					new MSubProjectFileLine(*currentSection, *this, inFile, type, version);
				break;
				
			default:
			{
				ASSERT(false);
				// This project is probably corrupt
				corrupt = true;
				MAlert		alert("This project may be corrupt.");
				alert.Go();
			}
				break;
		}

		if (currentSourceLine)
		{
			if (expanded)
				InsertRow(CountRows(), currentSourceLine);
			fFileList.AddItem(currentSourceLine);
			currentSection->AddLine(currentSourceLine);
			fAllFileList.AddItem(currentSourceLine->GetSourceFile());
			currentSourceLine->SetTargetRec(*fBuildCommander);
		}

		inFile.DoneBlock(type);	//	done with whatever block we got; move to next
	}

	if (version == 0)
		UpdateForDR8();

	UpdateBitmap();
	ShowFileCount();

	if (version <= kDR8ProjectVersion)
		PrefsToBuildCommander(true);
	
	if (! corrupt)
		StartFinder();

	// Set up our linker and let all the builders know about us
	fProject = new MProject((MProjectWindow&) *Window());
	fBuildCommander->SetLinker(MBuildersKeeper::GetLinker(fTargetPrefs.pLinkerName));
	MBuildersKeeper::ProjectChanged(*fProject, kProjectOpened);
}

// ---------------------------------------------------------------------------

void
MProjectView::InitProject()
{
	fBuildCommander->InitCommander(fTargetPrefs.pTargetArray, fTargetPrefs.pCount);
}

// ---------------------------------------------------------------------------
//		ReadPreferences
// ---------------------------------------------------------------------------
//	Read in all the preferences blocks.  These are stored at the beginning
//	of the project file, before the source line blocks.

uint32
MProjectView::ReadPreferences(
	MBlockFile&		inFile)
{
	BlockType			type;
	size_t				blockSize;
	AccessPathData*		newAccessPath;
	bool				hasProcessor = false;
	bool				hasWarnings = false;
	bool				hasPEF = false;
	bool				hasTargets = false;
	bool				hasDisassemble = false;
	bool				hasPrivatePrefs = false;
	uint32				version = 0;
	uint32				endianflag = 0;		// indicates endianess of the last host this project was written out on

	ASSERT(sizeof(AccessPathsPrefs) == 16);

	//	Scan file for data to fill in
	while (B_NO_ERROR == inFile.ScanBlock(type)) 
	{
		blockSize = inFile.GetCurBlockSize();


		switch (type) 
		{
			case kProjectVersionBlockType:
				inFile.GetBytes(sizeof(version), &version);
				if (B_HOST_IS_LENDIAN)
					version = B_BENDIAN_TO_HOST_INT32(version);

				break;

			case kEndianBlockType:
				inFile.GetBytes(sizeof(endianflag), &endianflag);
				break;

			case kRunPrefsBlockType:
				inFile.GetBytes(sizeof(fRunPrefs), &fRunPrefs);
				fRunPrefs.SwapBigToHost();
				break;

			case kEditorBlockType:
			case kFontBlockType:
			case kSyntaxStyleBlockType:
			case kBuildExtraBlockType:
				// editor, font, syntax, build extra
				// preferences are now global (not project settings)
				// ...blocks ignored
				break;

			case kAccessPathsBlockType:
				inFile.GetBytes(sizeof(fAccessPathsPrefs), &fAccessPathsPrefs);
				fAccessPathsPrefs.SwapBigToHost();				
				break;

			case kSystemPathBlockType:
				newAccessPath = new AccessPathData;
				ASSERT(blockSize == sizeof(AccessPathData));
				inFile.GetBytes(blockSize, newAccessPath);
				newAccessPath->SwapBigToHost();
				fSystemPathList.AddItem(newAccessPath);
				break;

			case kProjectPathBlockType:
				newAccessPath = new AccessPathData;
				ASSERT(blockSize == sizeof(AccessPathData));
				inFile.GetBytes(blockSize, newAccessPath);
				newAccessPath->SwapBigToHost();
				fProjectPathList.AddItem(newAccessPath);
				break;

			case kFileSetBlockType:
				FileSetHeader		header;
				
				// old version file sets are discarded
				inFile.GetBytes(sizeof(header), &header);
				header.SwapBigToHost();
				if (header.Version == kFileSetVersion)
				{
					fFileSets = ::operator new(blockSize);
					memcpy(fFileSets, &header, sizeof(header));
					char*	temp = (char*) fFileSets + sizeof(header);
					inFile.GetBytes(blockSize - sizeof(header), temp);
					fFileSetsSize = blockSize;
				}
				break;

			case kTargetsBlockType:
			{
				TargetHeader		header;
				int32				count;
				inFile.GetBytes(sizeof(int32), &header.version);
				
				header.SwapBigToHost();
				if (header.version >= kDR9TargetVersion)
				{
					inFile.GetBytes(sizeof(TargetPrefs) - sizeof(int32), &fTargetPrefs.pCount);
					count = B_BENDIAN_TO_HOST_INT32(fTargetPrefs.pCount);
					if (count > 0)
					{
						fTargetPrefs.pTargetArray = new TargetRec[count];
						inFile.GetBytes(count * sizeof(TargetRec), fTargetPrefs.pTargetArray);
					}
					else
						fTargetPrefs.pTargetArray = nil;
					fTargetPrefs.SwapBigToHost();
					fTargetPrefs.pVersion = header.version;
					
					if (header.version < kCurrentTargetVersion)
						MDefaultPrefs::UpdateTargets(fTargetPrefs);
				}
				else
				{
					// ignore old format
					MDefaultPrefs::SetTargetsDefaults(fTargetPrefs);
				}
				hasTargets = true;
			}
				break;

			case kGenericPrefsBlockType:
				fBuildCommander->ReadFromFile(inFile);
				break;

			case kPrivateBlockType:
				inFile.GetBytes(sizeof(fPrivatePrefs), &fPrivatePrefs);
				fPrivatePrefs.SwapBigToHost();
				hasPrivatePrefs = true;
				break;

// MWCC/MWLD specific
// Obsolete
			case kProjectBlockType:
				if (blockSize != sizeof(fProjectPrefs))
				{
					MDefaultPrefs::SetProjectDefaults(fProjectPrefs);
					fProjectPrefs.SwapHostToBig();
					blockSize = min(blockSize, sizeof(fProjectPrefs));
				}
				inFile.GetBytes(blockSize, &fProjectPrefs);
				fProjectPrefs.SwapBigToHost();
				fProjectPrefs.pVersion = kCurrentVersion;
				break;

			case kProcessorBlockType:
				inFile.GetBytes(sizeof(fProcessorPrefs), &fProcessorPrefs);
				fProcessorPrefs.SwapBigToHost();
				hasProcessor = true;
				break;

			case kLanguageBlockType:
				if (blockSize < sizeof(fLanguagePrefs))
				{
					MDefaultPrefs::SetLanguageDefaults(fLanguagePrefs);
					fProjectPrefs.SwapHostToBig();
				}
				inFile.GetBytes(blockSize, &fLanguagePrefs);
				fLanguagePrefs.SwapBigToHost();
				break;

			case kWarningsBlockType:
				inFile.GetBytes(sizeof(fWarningsPrefs), &fWarningsPrefs);
				fWarningsPrefs.SwapBigToHost();
				hasWarnings = true;
				break;

			case kLinkerBlockType:
				if (blockSize < sizeof(fLinkerPrefs))
				{
					MDefaultPrefs::SetLinkerDefaults(fLinkerPrefs);
					fLinkerPrefs.SwapHostToBig();				
				}
				inFile.GetBytes(blockSize, &fLinkerPrefs);
				fLinkerPrefs.SwapBigToHost();
				break;

			case kPEFBlockType:
				inFile.GetBytes(sizeof(fPEFPrefs), &fPEFPrefs);
				fPEFPrefs.SwapBigToHost();
				hasPEF = true;
				break;

			case kDisAsmBlockType:
				inFile.GetBytes(sizeof(fDisassemblePrefs), &fDisassemblePrefs);
				fDisassemblePrefs.SwapBigToHost();
				hasDisassemble = true;
				break;
// MWCC/MWLD specific

			default:
				ASSERT(false);
				break;
		}

		inFile.DoneBlock(type);	//	done with whatever block we got; move to next
	}

	// Existing projects won't have these fields
	// They were added for d2
	if (! hasWarnings)
		MDefaultPrefs::SetWarningsDefaults(fWarningsPrefs);
	if (! hasProcessor)
		MDefaultPrefs::SetProcessorDefaults(fProcessorPrefs);
	if (! hasPEF)
		MDefaultPrefs::SetPEFDefaults(fPEFPrefs);

	if (fAccessPathsPrefs.pVersion < 2)
	{
		fAccessPathsPrefs.pSystemPaths = fSystemPathList.CountItems();
		fAccessPathsPrefs.pProjectPaths = fProjectPathList.CountItems();
	}
	
	// Added for 1.0
	if (! hasTargets)
		MDefaultPrefs::SetTargetsDefaults(fTargetPrefs);
//	if (! hasDisassemble)
//		MDefaultPrefs::SetDisassemblerDefaults(fDisassemblePrefs);

	if (version <= kDR8plusProjectVersion || fAccessPathsPrefs.pVersion < kCurrentAccessPathsVersion)
	{
		FixUpSystemPaths(fAccessPathsPrefs, fSystemPathList);
		Window()->PostMessage(cmd_RemoveBinariesCompact);		// remove all objects fixes up some problems
	}

	// Added for 1.4
	if (! hasPrivatePrefs)
	{
		MDefaultPrefs::SetPrivateDefaults(fPrivatePrefs);
		fPrivatePrefs.runsWithDebugger = fProjectPrefs.pRunsWithDebugger;

		// Transfer the project prefs to the generic prefs container
		// since the project prefs panel is now in the mwcc/mwld plugin
		BMessage		msg;
		
		fProjectPrefs.SwapHostToBig();
		msg.AddData(kProjectNamePrefs, kMWLDType, &fProjectPrefs, sizeof(fProjectPrefs));
		fProjectPrefs.SwapBigToHost();
		fBuildCommander->SetData(msg);
	}
		
	SetWorkingDirectory();
	InitProject();

	return version;
}

// ---------------------------------------------------------------------------
//		UpdateForDR8
// ---------------------------------------------------------------------------
// Goofy changes for DR8

void
MProjectView::UpdateForDR8()
{
	// Post a message to the message window
	InfoStruct	 	info;

	info.iTextOnly = true;
	strcpy(info.iLineText, 		
		"Updating project to DR9 format\n"
		"Turning on RTTI\n"
		"Turning on Exceptions\n"
	);
	
	BMessage		msg(msgAddInfoToMessageWindow);
	
	msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));

	MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);

	// Update the options
	fLanguagePrefs.pEnableRTTI = true;
	fLanguagePrefs.pEnableExceptions = true;

	MSourceFileLine*		libpos;

	// Change libpos to libbe
	if (GetSourceFileLineByName("libpos.so", libpos))
	{
		LockFileList();
		MSourceFileLine*	libbe;
		MSourceFile*		sourceFile = libpos->GetSourceFile();

		if (sourceFile)
		{
			if (! GetSourceFileLineByName("libbe.so", libbe))
			{
				// Have libpos and not libbe
				entry_ref		ref;

				if (MFileUtils::FindFileInDirectoryList("libbe.so", fSystemDirectories, ref) ||
					MFileUtils::FindFileInDirectoryList("libbe.so", fProjectDirectories, ref))
				{
					// Change the line and its source file
					sourceFile->SetRef(ref, true);
					fAllFileList.RemoveItem(sourceFile);	// stored in sort order by name
					fAllFileList.AddItem(sourceFile);
				}
			}
			else
			{
				// Have both libbe and libpos already
				// so delete libpos
				RemoveFile(libpos);
			}
		}
		
		UnlockFileList();
	}
	
	SaveChanges();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind specified in the
//	BMessage.  In memory all prefs structs are in host endianness so we don't
//	need to swap here or in the prefs panels. 

void
MProjectView::GetData(
	BMessage&	inOutMessage)
{
	status_t	err;

	fBuildCommander->GetData(inOutMessage);

	if (inOutMessage.HasData(kAccessPathsPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kAccessPathsPrefs, kMWPrefs, &fAccessPathsPrefs, sizeof(fAccessPathsPrefs));
		MFileUtils::AddAccessPathsToMessage(inOutMessage, fSystemPathList, fProjectPathList);
	}
	if (inOutMessage.HasData(kFileSet, kMWPrefs))
	{
		if (fFileSets)
		{
			err = inOutMessage.ReplaceData(kFileSet, kMWPrefs, fFileSets, fFileSetsSize);
		}
	}
	if (inOutMessage.HasData(kTargetPrefs, kMWPrefs))
	{
		err = inOutMessage.ReplaceData(kTargetPrefs, kMWPrefs, &fTargetPrefs, sizeof(fTargetPrefs));
		if (fTargetPrefs.pTargetArray)
			inOutMessage.AddData(kTargetBlockPrefs, kMWPrefs, fTargetPrefs.pTargetArray, fTargetPrefs.pCount * sizeof(TargetRec), false);
	}
	
	if (inOutMessage.HasData(kRunPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kRunPrefs, kMWPrefs, &fRunPrefs, sizeof(fRunPrefs));
	}
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Get the preferences from the message and update the correct prefs struct.

void
MProjectView::SetData(
	BMessage&	inOutMessage)
{
	ssize_t		length = 0;

	PrivateProjectPrefs*	privatePrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kPrivatePrefs, kMWPrefs, (const void**) &privatePrefs, &length))
	{
		ASSERT(length == sizeof(PrivateProjectPrefs));
		fPrivatePrefs = *privatePrefs;
		inOutMessage.RemoveName(kPrivatePrefs);
	}

	TargetPrefs*	target;
	if (CompilingState() == sNotCompiling && B_NO_ERROR == inOutMessage.FindData(kTargetPrefs, kMWPrefs, (const void**) &target, &length))
	{
		ASSERT(length == sizeof(TargetPrefs));

		delete [] fTargetPrefs.pTargetArray;
		bool	linkerChanged = 0 != strcmp(fTargetPrefs.pLinkerName, target->pLinkerName);
		fTargetPrefs = *target;

		TargetRec*		targetArray;
		if (fTargetPrefs.pCount > 0 && B_NO_ERROR == inOutMessage.FindData(kTargetBlockPrefs, kMWPrefs, (const void**) &targetArray, &length))
		{
			ASSERT(length == fTargetPrefs.pCount * sizeof(TargetRec));
			if (targetArray)
			{
				fTargetPrefs.pTargetArray = new TargetRec[fTargetPrefs.pCount];
				memcpy(fTargetPrefs.pTargetArray, targetArray, fTargetPrefs.pCount * sizeof(TargetRec));
			}
		}
		
		fBuildCommander->InitCommander(fTargetPrefs.pTargetArray, fTargetPrefs.pCount);
		if (linkerChanged)
			fBuildCommander->SetLinker(MBuildersKeeper::GetLinker(fTargetPrefs.pLinkerName));

		MBuildersKeeper::ProjectChanged(*fProject, kProjectOpened);
		fBuildCommander->ProjectChanged();

		UpdateAllTargets();
		inOutMessage.RemoveName(kTargetPrefs);
		inOutMessage.RemoveName(kTargetBlockPrefs);
	}

	AccessPathsPrefs*	accessPaths;
	if (B_NO_ERROR == inOutMessage.FindData(kAccessPathsPrefs, kMWPrefs, (const void**) &accessPaths, &length))
	{
		ASSERT(length == sizeof(AccessPathsPrefs));
		fAccessPathsPrefs = *accessPaths;
		
		MFileUtils::GetAccessPathsFromMessage(inOutMessage, fSystemPathList, fProjectPathList, 
							fAccessPathsPrefs.pSystemPaths, fAccessPathsPrefs.pProjectPaths);

		// while the caches are also reset in ResetFilePaths,
		// we need them done here syncronously before the plugin turns
		// around and asks for them as a result of ProjectChanged below
		MFileUtils::EmptyDirectoryList(fSystemDirectories, &fProjectDirectory);
		MFileUtils::EmptyDirectoryList(fProjectDirectories, &fProjectDirectory);
	
		if (fAccessPathsPrefs.pSystemPaths > 0) {
			MFileUtils::BuildDirectoriesList(fSystemPathList, fSystemDirectories, &fProjectDirectory);
		}
		if (fAccessPathsPrefs.pProjectPaths > 0) {
			MFileUtils::BuildDirectoriesList(fProjectPathList, fProjectDirectories, &fProjectDirectory);
		}
		
		inOutMessage.RemoveName(kAccessPathsPrefs);
		inOutMessage.RemoveName(kAccessPathsData);

		// If a path has been removed we need to remove references
		// to files that are in that path.  Unfortunately resetting
		// the paths is slow.  Perhaps a faster method could be used
		// based on which paths are removed and which path a particular
		// include file is in. 
		Window()->PostMessage(cmd_ResetFilePaths);
	}
	
	RunPreferences* runPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kRunPrefs, kMWPrefs, (const void**) &runPrefs, &length))
	{
		ASSERT(length == sizeof(RunPrefs));
		fRunPrefs = *runPrefs;
		
		inOutMessage.RemoveName(kRunPrefs);
	}
	
	FileSetRec*	recs;
	if (B_NO_ERROR == inOutMessage.FindData(kFileSet, kMWPrefs, (const void**) &recs, &length))
	{
		::operator delete(fFileSets);
		fFileSets = ::operator new(length);
		memcpy(fFileSets, recs, length);
		fFileSetsSize = length;
		inOutMessage.RemoveName(kFileSet);
	}

	int32		updateType;
	if (B_NO_ERROR == inOutMessage.FindInt32(kNeedsUpdating, &updateType))
	{
		switch (updateType)
		{
			case kCompileUpdate:
				TouchAllSourceFiles(true);
				break;

			case kLinkUpdate:
				fBuildCommander->ProjectChanged();
				break;
		}
	}
	
	fBuildCommander->SetData(inOutMessage);
	SetDirty();
	MBuildersKeeper::ProjectChanged(*fProject, kPrefsChanged);
}

// ---------------------------------------------------------------------------
//		SetPrefs
// ---------------------------------------------------------------------------
//	Add or replace the preferences specified.  Called from a plugin builder 
//	via the MProject object.

void
MProjectView::SetPrefs(
	BMessage&	inMessage,
	uint		inUpdateType)
{
	fBuildCommander->SetData(inMessage);
	if (inUpdateType != 0)
	{
		if ((inUpdateType & kCompileUpdate) != 0)
			TouchAllSourceFiles(true);
	
		if ((inUpdateType & kLinkUpdate) != 0)
			fBuildCommander->ProjectChanged();
	
		SetDirty();
	}

	MBuildersKeeper::ProjectChanged(*fProject, kPrefsChanged);
}

// ---------------------------------------------------------------------------
//		GetNthFile
// ---------------------------------------------------------------------------
//	Get the nth file in the project window.  returns false
//	if inIndex is out of bounds.

bool
MProjectView::GetNthFile(
	MFileRec&	outRec,
	int32		inIndex)
{
	MProjectLine* projectLine;
	
	if (fFileList.GetNthItem(projectLine, inIndex))
	{
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		line->FillFileRec(outRec);
	
		return true;
	}
	else
		return false; 
}

// ---------------------------------------------------------------------------
//		GetNthFile
// ---------------------------------------------------------------------------
//	Get the nth file in the project window.  returns false
//	if inIndex is out of bounds.

bool
MProjectView::GetNthFile(
	MFileRec&	outRec,
	BList&		outTargetList,
	int32		inIndex)
{
	MProjectLine* projectLine;
	
	if (fFileList.GetNthItem(projectLine, inIndex))
	{
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		line->FillTargetFilePathList(outTargetList);
		line->FillFileRec(outRec);
	
		return true;
	}
	else
		return false; 
}

// ---------------------------------------------------------------------------
//		ValidateGenericData
// ---------------------------------------------------------------------------

void
MProjectView::ValidateGenericData()
{
	fBuildCommander->ValidateGenericData();
}

// ---------------------------------------------------------------------------
//		PrefsToBuildCommander
// ---------------------------------------------------------------------------
//	Only used for old projects.

void
MProjectView::PrefsToBuildCommander(
	bool	inInitialize)
{
	BMessage		msg;
	
	// PPC Compiler
	msg.AddData(kLanguagePrefs, kMWCCType, &fLanguagePrefs, sizeof(fLanguagePrefs));
	msg.AddData(kProcessorPrefs, kMWCCType, &fProcessorPrefs, sizeof(fProcessorPrefs));
	msg.AddData(kWarningsPrefs, kMWCCType, &fWarningsPrefs, sizeof(fWarningsPrefs));

	// Linker
	msg.AddData(kLinkerPrefs, kMWLDType, &fLinkerPrefs, sizeof(fLinkerPrefs));
	msg.AddData(kPEFPrefs, kMWLDType, &fPEFPrefs, sizeof(fPEFPrefs));
	
	// PPC Disassembler
	msg.AddData(kDisAsmPrefs, kMWCCType, &fDisassemblePrefs, sizeof(fDisassemblePrefs));

	if (inInitialize)
		fBuildCommander->InitializeData(msg);		
	else
		fBuildCommander->SetData(msg);
}

// ---------------------------------------------------------------------------
//		FillFileList
// ---------------------------------------------------------------------------
//	Utility for the Find window to give it all the files for multi-find.

void
MProjectView::FillFileList(
	MSourceFileList&	inList, 
	SourceListT			inKind)
{
	int32				i = 0;
	MProjectLine*		line;
	MSourceFile*		sourceFile;

	switch (inKind)
	{
		case kSourceFiles:
			while (fFileList.GetNthItem(line, i++))
			{
				if (typeid(*line) == typeid(MSourceFileLine))
				{
					sourceFile = ((MSourceFileLine*)line)->GetSourceFile();
					
					inList.AddItem(new MSourceFile(*sourceFile));
				}
			}
			break;

		case kProjectHeaderFiles:
			while (fAllFileList.GetNthItem(sourceFile, i++))
			{
				if (sourceFile->IsHeaderFile() && ! sourceFile->IsInSystemTree() && ! sourceFile->IsPrecompiledHeader())
				{
					inList.AddItem(new MSourceFile(*sourceFile));
				}
			}
			break;

		case kSystemHeaderFiles:
			while (fAllFileList.GetNthItem(sourceFile, i++))
			{
				if (sourceFile->IsHeaderFile() && sourceFile->IsInSystemTree()  && ! sourceFile->IsPrecompiledHeader())
				{
					inList.AddItem(new MSourceFile(*sourceFile));
				}
			}
			break;
	}
}

// ---------------------------------------------------------------------------
//		FillFileList
// ---------------------------------------------------------------------------
//	Utility to return entry_refs for all the files in the project.

void
MProjectView::FillFileList(
	BList*				inList, 
	SourceListT			inKind)
{
	int32				i = 0;
	MProjectLine*		line;
	MSourceFile*		sourceFile;
	MSourceFileLine*	sourceFileLine;

	switch (inKind)
	{
		case kSourceFilesRefs:
			while (fFileList.GetNthItem(line, i++))
			{
				sourceFileLine = dynamic_cast<MSourceFileLine*>(line);
				if (sourceFileLine != nil)
				{
					sourceFile = sourceFileLine->GetSourceFile();
					entry_ref*		ref = new entry_ref;
					if (B_NO_ERROR == sourceFile->GetRef(*ref))
						inList->AddItem(ref);
				}
			}
			break;
	}
}

// ---------------------------------------------------------------------------
//		RunWithDebugger
// ---------------------------------------------------------------------------

void 
MProjectView::RunWithDebugger(
	bool	inRunWithDebugger)
{
	fPrivatePrefs.runsWithDebugger = inRunWithDebugger;
}

// ---------------------------------------------------------------------------
//		GetLineByIndex
// ---------------------------------------------------------------------------
//	return the sourcefileline at this index where the index
//	ignores section lines.

MSourceFileLine *
MProjectView::GetLineByIndex(
	int32	inIndex)
{
	if (inIndex < 0)
		inIndex = 0;
	else
	if (inIndex >= fFileList.CountItems())
		inIndex = fFileList.CountItems() - 1;

	return (MSourceFileLine *) fFileList.ItemAtFast(inIndex);
}

// ---------------------------------------------------------------------------
//		PerformScriptAction
// ---------------------------------------------------------------------------
//	Perform actions from scripts or remote apps.

status_t
MProjectView::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				wasDeferred)
{
	status_t		result;

	switch (message->what) 
	{
		case kMakeProject:
			result = fBuildCommander->PerformScriptAction(message, reply, wasDeferred);
			break;

		// Add file
		case kCreateVerb:
			result = AddFilesFromScript(message, reply, -1);
			break;

		// Remove file
		case kDeleteVerb:
			result = RemoveFileFromScript(message, reply, nil);
			break;

		default:
			result = SCRIPT_BAD_VERB;
			break;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		BuildAction
// ---------------------------------------------------------------------------
//	These just forward the build actions to the BuildCommander.

void 
MProjectView::BuildAction(
	uint32 inCommand)
{
	MBuildersKeeper::ProjectChanged(*fProject, kBuildStarted);
	fBuildCommander->BuildAction(inCommand);
}

void 
MProjectView::BuildAction(
	BMessage& inMessage)
{
	MBuildersKeeper::ProjectChanged(*fProject, kBuildStarted);
	fBuildCommander->BuildAction(inMessage);
}

// ---------------------------------------------------------------------------

void 
MProjectView::SetProject(const entry_ref& inProject)
{
	// remember if this project is writable
	
	BNode projectStat(&inProject);
	fOKToWriteProject = FileWriteableState(projectStat) == kIsWritable;
		
	fBuildCommander->SetProject(inProject);
}

// ---------------------------------------------------------------------------

bool
MProjectView::OKToModifyProject()
{
	// if the project is writable, then it is modifiable
	// if not writable, then ask if the user would like to
	// "unlock" the project ... at which point we change
	// fOKToWriteProject to true

	if (fOKToWriteProject == false) {
		MAlert alert("The project file is locked and cannot be modified.  Do you want to unlock this project and proceed with the action?", "Unlock", "Don't Unlock");
		fOKToWriteProject = (alert.Go() == kOKButton);
	}
	
	return fOKToWriteProject;
}

// ---------------------------------------------------------------------------
//		GetAccessDirectories
// ---------------------------------------------------------------------------
//	Iterate fProjectDirectories and fSystemDirectories and give the plugin
//	AccessDirectoryInfo for each entry  (AccessDirectoryInfo is defined at
//	the MProject level so the plugin knows about the type.)

void
MProjectView::GetAccessDirectories(
	BList& inOutProjectList,
	BList& inOutSystemList,
	bool& outSearchProjectTreeFirst)
{
	// fill in the list of project directories and system directories
	inOutProjectList.MakeEmpty();
	inOutSystemList.MakeEmpty();
	
	DirectoryInfo* dirInfo;
	
	// Convert DirectoryInfo into AccessDirectoryInfo that plugins know about
	int32 projectCount = 0;
	while (fProjectDirectories.GetNthItem(dirInfo, projectCount++)) {
		AccessDirectoryInfo* accessInfo = new AccessDirectoryInfo(dirInfo->dDir, dirInfo->dRecursiveSearch);
		inOutProjectList.AddItem(accessInfo);
	}
	int32 systemCount = 0;
	while (fSystemDirectories.GetNthItem(dirInfo, systemCount++)) {
		AccessDirectoryInfo* accessInfo = new AccessDirectoryInfo(dirInfo->dDir, dirInfo->dRecursiveSearch);
		inOutSystemList.AddItem(accessInfo);
	}
	
	outSearchProjectTreeFirst = fAccessPathsPrefs.pSearchInProjectTreeFirst;
}
