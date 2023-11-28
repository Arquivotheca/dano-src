//==================================================================
//	MAccessPathListView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include "MAccessPathListView.h"
#include "MAccessPathsView.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "MFileUtils.h"
#include "stDrawingMode.h"
#include "MDynamicMenuHandler.h"
#include "MProjectWindow.h"

#include <Autolock.h>
#include <Entry.h>
#include <Path.h>

const float kPathNameMargin = 15.0;
const float kBitMapWidth = 10;
const float kBitMapHeight = 10;

#define B 0x00
#define T 255
#define G 24

const char recursiveIconData[] = 
{
	T,T,T,T,T,T,T,T,T,T,
	T,B,B,B,T,T,T,T,T,T,
	B,G,G,G,B,B,B,T,T,T,
	B,G,G,G,G,G,G,B,T,T,
	B,G,G,B,B,B,G,B,T,T,
	B,G,B,G,G,G,B,B,B,T,
	B,B,B,G,G,G,G,G,G,B,
	T,T,B,G,G,G,G,G,G,B,
	T,T,B,G,G,G,G,G,G,B,
	T,T,B,B,B,B,B,B,B,B,
};

// All of our static bitmaps and rects
BBitmap * 	MAccessPathListView::sFoldersBitmap;
BRect		MAccessPathListView::sFoldersRect;

// ---------------------------------------------------------------------------
// Helper function to convert an access path into full BPath
// ---------------------------------------------------------------------------

status_t
ConvertToPath(AccessPathData& accessPath, BPath& outPath, MProjectWindow* project)
{
	status_t result = B_ERROR;
	
	switch (accessPath.pathType)
	{
		case kAbsolutePath:
			result = outPath.SetTo(accessPath.pathName);
			break;
		
		case kPathToProjectTree:
		case kProjectRelativePath:
		{
			if (project != NULL) {
				BAutolock autolock(project);
				BEntry projEntry(&project->GetProjectDirectory(), NULL);
				BPath projectPath(&projEntry);
				char* leaf = accessPath.pathType == kPathToProjectTree ? 
							 NULL : accessPath.pathName;
				// the leaf can't start with a "/"
				if (leaf && leaf[0] == '/') {
					leaf++;
				}
				result = outPath.SetTo(projectPath.Path(), leaf);
			}
			break;
		}
		
		case kPathToSystemTree:
		case kSystemRelativePath:
		{
			BEntry entry;
			BPath systemPath;
			MFileUtils::SystemDirectory().GetEntry(&entry);
			entry.GetPath(&systemPath);
			char* leaf = accessPath.pathType == kPathToSystemTree ? 
						 NULL : accessPath.pathName;
			// the leaf can't start with a "/"
			if (leaf && leaf[0] == '/') {
				leaf++;
			}
			result = outPath.SetTo(systemPath.Path(), leaf);
			break;
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		MAccessPathListView
// ---------------------------------------------------------------------------
//	Constructor

MAccessPathListView::MAccessPathListView(BRect inFrame, const char* inName)
					: MDLOGListView(inFrame, inName)
{
	if (! sFoldersBitmap)
	{
		sFoldersBitmap = LoadBitmap(recursiveIconData, kBitMapWidth, kBitMapHeight);
		sFoldersRect.Set(0.0, 0.0, kBitMapWidth, kBitMapHeight);
		sFoldersRect.OffsetBy(1.0, 0.0);
	}

	SetMultiSelect(false);
	SetDragNDropCells(true);
	fProject = nil;
}

// ---------------------------------------------------------------------------
//		~MAccessPathListView
// ---------------------------------------------------------------------------
//	Destructor

MAccessPathListView::~MAccessPathListView()
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
MAccessPathListView::DrawRow(
	int32 	/*inRow*/,
	void * 	inData,
	BRect	inArea,
	BRect	/*inIntersection*/)
{
	AccessPathData*		accessPath = (AccessPathData*) inData;

	if (accessPath)
	{
		if (accessPath->recursiveSearch)
		{
			// Draw the folders icon
			stDrawingMode		mode(this, B_OP_OVER);
			DrawBitmapAsync(sFoldersBitmap, 
				BPoint(sFoldersRect.left, inArea.top + sFoldersRect.top));
		}

		font_height		info;
		
		info = GetCachedFontHeight();

		MovePenTo(kPathNameMargin, inArea.bottom - info.descent);
		if (accessPath->pathType == kProjectRelativePath)
			DrawString(kProjectPathName);
		else
		if (accessPath->pathType == kSystemRelativePath)
			DrawString(kSystemPathName);

		DrawString(accessPath->pathName);
	}
}

// ---------------------------------------------------------------------------
//		MouseMoved
// ---------------------------------------------------------------------------
//	make ourself the focus when we're about to be dragged onto

void
MAccessPathListView::MouseMoved(
	BPoint		/*where*/,
	uint32		code,
	const BMessage *	inMessage)
{
	if (code == B_INSIDE_VIEW && inMessage != nil && (inMessage->what == B_SIMPLE_DATA ||
		inMessage->HasRef("refs")))
	{
		MakeFocus();
	}
}

// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------
//	make ourself the focus when we're about to be dragged onto

void
MAccessPathListView::KeyDown(
	const char *bytes, 
	int32 		numBytes)
{
	switch (bytes[0])
	{
		case B_BACKSPACE:
		case 'R':		// Remove
		case 'r':
			PostToTarget(msgRemoveAccessPath);
			break;

		case 'D':		// Add Default
		case 'd':
			PostToTarget(msgAddDefaultAccessPath);
			break;

		case 'A':		// Add
		case 'a':
			PostToTarget(msgAddAccessPath);
			break;

		case 'C':		// Change
		case 'c':
			PostToTarget(msgChangeAccessPath);
			break;
		
		default:
			MDLOGListView::KeyDown(bytes, numBytes);
			break;
	}
}

// ---------------------------------------------------------------------------
//		ClickHook
// ---------------------------------------------------------------------------

bool
MAccessPathListView::ClickHook(
	BPoint	inWhere,
	int32	inRow,
	uint32 /* modifiers */,
	uint32 /* buttons */)
{
	BRect			folderRect = sFoldersRect;
	BRect 			frame;

	GetRowRect(inRow, &frame);

	folderRect.OffsetTo(sFoldersRect.left, frame.top);

	if (folderRect.Contains(inWhere))
	{
		AccessPathData*		accessPath = (AccessPathData*) GetList()->ItemAt(inRow);
		ASSERT(accessPath);

		if (accessPath)
			accessPath->recursiveSearch = ! accessPath->recursiveSearch;
		
		Invalidate(folderRect);
		
		PostToTarget(msgValueChanged);
	}

	return false;
}

// ---------------------------------------------------------------------------
//		InitiateDrag
// ---------------------------------------------------------------------------
//	Hook function called to start a drag.

void
MAccessPathListView::InitiateDrag(
	BPoint 	/*inPoint*/,
	int32	inRow)
{
	BRect dragRect;
	BMessage msg(B_SIMPLE_DATA);

	GetRowRect(inRow, &dragRect);
	msg.AddPointer(kListObject, this);
	msg.AddInt32(kLineNumber, inRow);

	// Add the ref for the directory so we can drag this elsewere also
	AccessPathData* accessPath = (AccessPathData*) GetList()->ItemAt(inRow);
	BPath path;
	entry_ref ref;
	if (::ConvertToPath(*accessPath, path, fProject) == B_OK &&
					get_ref_for_path(path.Path(), &ref) == B_OK) {
		msg.AddRef("refs", &ref);
	}

	DragMessage(&msg, dragRect);
}

// ---------------------------------------------------------------------------
//		DeleteItem
// ---------------------------------------------------------------------------

void
MAccessPathListView::DeleteItem(
	void * inItem)
{
	delete (AccessPathData*) inItem;
}

// ---------------------------------------------------------------------------

status_t
MAccessPathListView::ConvertItemToPath(int32 index, BPath& outPath)
{
	status_t result = B_ERROR;
	
	if (index >= 0 && index <= GetList()->CountItems()) {
		AccessPathData* accessPath = (AccessPathData*) GetList()->ItemAt(index);
		result = ::ConvertToPath(*accessPath, outPath, fProject);
	}
	
	return result;
}


