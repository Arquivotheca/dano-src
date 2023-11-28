//========================================================================
//	MTextInfoView.cpp
//	Copyright 1995 - 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MTextInfoView.h"
#include "MTextWindow.h"
#include "MIDETextView.h"
#include "MFunctionPopup.h"
#include "MPathPopup.h"
#include "IDEMessages.h"
#include "CString.h"
#include "stDrawingMode.h"
#include <StorageKit.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Roster.h>

BRect		MTextInfoView::sHeaderRect;
BRect		MTextInfoView::sFunctionRect;
BRect		MTextInfoView::sEOLRect;
BRect		MTextInfoView::sLockedRect;
BRect		MTextInfoView::sLineTextRect;
MList<BBitmap*>		MTextInfoView::sButtonList;
bool				MTextInfoView::sBitmapInited;

// ---------------------------------------------------------------------------
//		MTextInfoView
// ---------------------------------------------------------------------------

MTextInfoView::MTextInfoView(
	BRect area,
	MTextWindow & window,
	MIDETextView & view) :
	BView(
		area,
		"info",
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_PULSE_NEEDED),
	fText(view),
	fWindow(window)
{
	fLastColumn = 1;
	fLastLine = 1;
	fVScroller = nil;
	fMaxRange = -1;
	fProportion = 0.0;
	fState = kUnsavedFile;
	InitBitmaps();
}

// ---------------------------------------------------------------------------
//		~MTextInfoView
// ---------------------------------------------------------------------------

MTextInfoView::~MTextInfoView()
{
}

// ---------------------------------------------------------------------------
//		MMessageItem::InitBitmaps
// ---------------------------------------------------------------------------

const float kBitMapWidth = 19;
const float kBitMapHeight = 14;
const float kLineMargin = 4 * kBitMapWidth + 7.0;

const type_code kIconKind = 'ICON';		// works for either endianess

enum {
	kHeaderUp = 200,
	kHeaderDn = 201,
	kFunctionUp = 202,
	kFunctionDn = 203,
	kEOLUp = 204,
	kEOLDn = 205,
	kLockedUp = 206,
	kLockedDn = 207,
	kUnlockedUp = 208,
	kUnlockedDn = 209,
	kReadOnlyUp = 210,
	kReadOnlyDn = 211
};

enum {
	kHeaderUpIndex,
	kHeaderDnIndex,
	kFunctionUpIndex,
	kFunctionDnIndex,
	kEOLUpIndex,
	kEOLDnIndex,
	kLockedUpIndex,
	kLockedDnIndex,
	kUnlockedUpIndex,
	kUnlockedDnIndex,
	kReadOnlyUpIndex,
	kReadOnlyDnIndex
};


void
MTextInfoView::InitBitmaps()
{
	if (sBitmapInited)
		return;

	app_info 		info;

	be_app->GetAppInfo(&info);

	BFile 			appfile(&info.ref, B_READ_ONLY);
	BResources 		resFile(&appfile);
	
	// this causes a crash if the resources aren't found
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kHeaderUp, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kHeaderDn, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kFunctionUp, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kFunctionDn, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kEOLUp, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kEOLDn, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kLockedUp, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kLockedDn, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kUnlockedUp, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kUnlockedDn, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kReadOnlyUp, 32, kBitMapHeight - 1));
	sButtonList.AddItem(LoadBitmap(resFile, kIconKind, kReadOnlyDn, 32, kBitMapHeight - 1));

	BRect bounds(0, 0, kBitMapWidth - 1, kBitMapHeight - 1);

	sHeaderRect = bounds;
	sHeaderRect.OffsetBy(1.0, 1.0);

	sFunctionRect = bounds;
	sFunctionRect.OffsetBy(kBitMapWidth + 1, 1.0);

	sEOLRect = bounds;
	sEOLRect.OffsetBy((2.0f * kBitMapWidth) + 1, 1.0);

	sLockedRect = bounds;
	sLockedRect.OffsetBy((3.0f * kBitMapWidth) + 1, 1.0);
	
	sLineTextRect = Bounds();
	sLineTextRect.left = kLineMargin;
	sLineTextRect.top++;
	
	sBitmapInited = true;
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MTextInfoView::AttachedToWindow()
{
	SetViewColor(kGrey217);
	SetLowColor(kGrey217);
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MTextInfoView::Draw(
	BRect	inArea)
{
	BRect 		bounds(0, 0, kBitMapWidth - 1, kBitMapHeight - 1);

	if (inArea.Intersects(sLineTextRect))
	{
		String string("Line ");
		string += fLastLine;
		string += "  Column ";
		string += fLastColumn;
		
		MovePenTo(kLineMargin, 12.0);
		DrawString(string);
	}

{
	stDrawingMode		mode(this, B_OP_OVER);

	if (inArea.Intersects(sHeaderRect))
		DrawBitmapAsync(sButtonList.ItemAt(kHeaderUpIndex), bounds, sHeaderRect);
	if (inArea.Intersects(sFunctionRect))
		DrawBitmapAsync(sButtonList.ItemAt(kFunctionUpIndex), bounds, sFunctionRect);
	if (inArea.Intersects(sEOLRect))
		DrawBitmapAsync(sButtonList.ItemAt(kEOLUpIndex), bounds, sEOLRect);
	if (inArea.Intersects(sLockedRect))
	{
		int32		upindex;
		int32		dnindex;

		GetLockedIndexes(upindex, dnindex);

		DrawBitmapAsync(sButtonList.ItemAt(upindex), bounds, sLockedRect);
	}
}

	bounds = Bounds();
	BeginLineArray(1);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kGrey144);
	EndLineArray();
}

// ---------------------------------------------------------------------------
//		Pulse
// ---------------------------------------------------------------------------
//	Adjust the linenumber and columnnumber in the view.

void
MTextInfoView::Pulse()
{
	uchar   ch;
	int32 	currentLine, currentColumn;
	int32   i;
	int32   lineStart, selStart, selFinish;
	int32   tabSize = (int32) (fText.TabWidth() / fText.StringWidth("M"));
	
	currentLine = fText.CurrentLine() + 1;
	
	fText.GetSelection(&selStart, &selFinish);
    lineStart = fText.OffsetAt(currentLine - 1);
    
    if (selStart < lineStart) selStart = lineStart;
    currentColumn = 1;
    
    for (i = lineStart; i < selStart; i++)  // Loop through line chars
      {
        ch = fText.ByteAt(i);               // Get next char
        
        if (ch == '\t')                     // If it's a tab
          {
            while (currentColumn % tabSize) // Until we get to next tab stop
              currentColumn++;              // Add another column
              
            currentColumn++;                // To next column
          }
        else if ((ch & 0xC0) != 0x80)       // If UTF-8 start char
          {
            currentColumn++;                // One char further
          }
      }

	if ((currentLine != fLastLine) || 
	(currentColumn != fLastColumn))
	{
		fLastColumn = currentColumn;
		fLastLine = currentLine;
		Invalidate(sLineTextRect);
		Window()->UpdateIfNeeded();
	}
}

// ---------------------------------------------------------------------------
//		GetLockedIndexes
// ---------------------------------------------------------------------------

void
MTextInfoView::GetLockedIndexes(
	int32& 	outUpIndex,
	int32&	outDnIndex)
{
	switch (fWindow.WritableState())
	{
		case kIsWritable:
		case kUnsavedFile:
			outUpIndex = kUnlockedUpIndex;
			outDnIndex = kUnlockedDnIndex;
			break;

		case kPermissionLocked:
			outUpIndex = kLockedUpIndex;
			outDnIndex = kLockedDnIndex;
			break;

		case kReadOnly:
			outUpIndex = kReadOnlyUpIndex;
			outDnIndex = kReadOnlyDnIndex;
			break;
	}
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------

void
MTextInfoView::MouseDown(
	BPoint inWhere)
{
	BRect 		bounds(0, 0, kBitMapWidth - 1, kBitMapHeight - 1);
	stDrawingMode		mode(this, B_OP_OVER);

	if (sHeaderRect.Contains(inWhere))
	{
		DrawBitmap(sButtonList.ItemAt(kHeaderDnIndex), bounds, sHeaderRect);
		DoHeaderPopup();
		DrawBitmap(sButtonList.ItemAt(kHeaderUpIndex), bounds, sHeaderRect);
	}
	else
	if (sFunctionRect.Contains(inWhere))
	{
		DrawBitmap(sButtonList.ItemAt(kFunctionDnIndex), bounds, sFunctionRect);
		DoFunctionPopup();
		DrawBitmap(sButtonList.ItemAt(kFunctionUpIndex), bounds, sFunctionRect);
	}
	else
	if (sEOLRect.Contains(inWhere))
	{
		DrawBitmap(sButtonList.ItemAt(kEOLDnIndex), bounds, sEOLRect);
		DoEOLTypePopup();
		DrawBitmap(sButtonList.ItemAt(kEOLUpIndex), bounds, sEOLRect);
	}
	else
	if (sLockedRect.Contains(inWhere))
	{
		int32		upindex;
		int32		dnindex;

		GetLockedIndexes(upindex, dnindex);
		DrawBitmap(sButtonList.ItemAt(dnindex), bounds, sLockedRect);
		DoLockPopup();
		GetLockedIndexes(upindex, dnindex);		// it may have changed
		DrawBitmap(sButtonList.ItemAt(upindex), bounds, sLockedRect);
	}
	else
		DoPathPopup();
}

// ---------------------------------------------------------------------------
//		DoHeaderPopup
// ---------------------------------------------------------------------------

void
MTextInfoView::DoHeaderPopup()
{
	MPopupMenu		headerPopup("header");
	
	headerPopup.SetFont(be_plain_font);

	// The header popup is passed along through
	// several intermediaries to the sourcefileline object
	// for this file where it is actually built
	fWindow.BuildPopupMenu(headerPopup);
	headerPopup.SetTargetForItems(&fWindow);
	
	BPoint		where(sHeaderRect.right + 3.0, sHeaderRect.bottom - 3.0);

	ConvertToScreen(&where);
	headerPopup.SetBottomLeft(where);

	// Show the popup
	(void) headerPopup.Go(where, true);
}

// ---------------------------------------------------------------------------
//		DoEOLTypePopup
// ---------------------------------------------------------------------------

void
MTextInfoView::DoEOLTypePopup()
{
	MPopupMenu eolPopup("filetypes", false, false);
	BMessage* msg;
	
	eolPopup.SetFont(be_plain_font);
	
	// Add the three EOLTypes to the menu
	msg = new BMessage(msgSetFileType);
	msg->AddInt32(kEOLType, kNewLineFormat);
	eolPopup.AddItem(new BMenuItem("Be", msg));

	msg = new BMessage(msgSetFileType);
	msg->AddInt32(kEOLType, kMacFormat);
	eolPopup.AddItem(new BMenuItem("Macintosh", msg));

	msg = new BMessage(msgSetFileType);
	msg->AddInt32(kEOLType, kCRLFFormat);
	eolPopup.AddItem(new BMenuItem("DOS", msg));

	eolPopup.SetTargetForItems(Window());
	
	// Select the current EOLType
	BMenuItem* item = eolPopup.ItemAt(fWindow.GetEOLType());
	ASSERT(item);
	if (item) {
		item->SetMarked(true);
	}
	
	BPoint where(sEOLRect.right + 3.0, sEOLRect.bottom - 3.0);

	ConvertToScreen(&where);
	eolPopup.SetBottomLeft(where);

	// Show the popup
	// It posts a message to the window if the type is changed
	(void) eolPopup.Go(where, true);
}

// ---------------------------------------------------------------------------
//		DoFunctionPopup
// ---------------------------------------------------------------------------

void
MTextInfoView::DoFunctionPopup()
{
	MFunctionPopup		popup(fText, fWindow, (modifiers() & (B_OPTION_KEY | B_CONTROL_KEY)) != 0);
	BPoint				where(sFunctionRect.right + 3.0, sFunctionRect.bottom - 3.0);

	ConvertToScreen(&where);
	popup.SetBottomLeft(where);

	// Show the popup
	BMenuItem*		item = popup.Go(where, false);
	
	if (item)
	{
		popup.SelectAFunction(item);
	}
}

// ---------------------------------------------------------------------------
//		DoPathPopup
// ---------------------------------------------------------------------------

void
MTextInfoView::DoPathPopup()
{
	entry_ref		ref;
	
	if (B_NO_ERROR == fWindow.GetRef(ref))
	{
		MPathPopup		popup("", ref);
		BPoint			where(0.0, fWindow.Bounds().bottom + 3.0);
		fWindow.ConvertToScreen(&where);
	
		// Show the popup
		// It posts a message to itself if an item is chosen and the message
		// is forwarded to the Tracker or the app to open the file or directory
		BMenuItem*		item = popup.Go(where, true);
		if (item)
		{
			popup.OpenItem(item);
		}
	}
}

// ---------------------------------------------------------------------------
//		DoLockPopup
// ---------------------------------------------------------------------------

void
MTextInfoView::DoLockPopup()
{
	MPopupMenu		lockPopup("lockstate", false, false);
	const char *	name;
	bool			enabled = true;
	
	lockPopup.SetFont(be_plain_font);
	
	switch (fWindow.WritableState())
	{
		case kIsWritable:
			name = "Lock File";
			break;

		case kPermissionLocked:
			name = "Unlock File";
			break;

		case kReadOnly:
			name = "Read Only File";
			enabled = false;
			break;

		case kUnsavedFile:
			name = "Unsaved File";
			enabled = false;
			break;
	}
	
	// Add the item to the menu
	BMessage*		msg = new BMessage;
	BMenuItem*		item = new BMenuItem(name, msg);
	
	lockPopup.AddItem(item);
	if (!enabled)
		item->SetEnabled(false);

	// Show the popup
	BPoint		where(sLockedRect.right + 3.0, sLockedRect.bottom - 3.0);
	ConvertToScreen(&where);
	lockPopup.SetBottomLeft(where);

	item = lockPopup.Go(where, false);
	if (item)
	{
		fWindow.ChangeWritableState();
	}
}
