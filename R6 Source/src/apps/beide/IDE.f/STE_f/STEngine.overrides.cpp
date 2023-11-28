// ============================================================
//  STEngine.overrides.cpp	©1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5


#include "STEngine.h"

#include <stdlib.h>

#include <ScrollBar.h>
#include <Region.h>
#include <Application.h>
#include <OS.h>
#include <Window.h>

// ------------------------------------------------------------
// 	AttachedToWindow
// ------------------------------------------------------------
// Reset some member variables, recalculate the line breaks, 
// redraw the text

void
STEngine::AttachedToWindow()
{
	BView::AttachedToWindow();

	Window()->SetPulseRate(500000.0);

	mCaretVisible = false;
	mCaretTime = 0;
	mClickCount = 0;
	mClickTime = 0;
	mDragOffset = -1;
	mDragOwner = false;
	mActive = false;

	Refresh(0, mText.Length(), true, false);
}

// ------------------------------------------------------------
// 	Draw
// ------------------------------------------------------------
// Draw any lines that need to be updated and display the 
// caret or the current selection

void
STEngine::Draw(
	BRect	inRect)
{
	if (! IsPrinting())
	{
		// what lines need to be drawn?
		int32 startLine = PixelToLine(inRect.top);
		int32 endLine = PixelToLine(inRect.bottom);

		DrawLines(startLine, endLine);

		// draw the caret/hilite the selection
		if (mSelStart != mSelEnd)
		{
			if (! mActive)
				DrawSelection(mSelStart, mSelEnd);
		}
		else
		if (mActive)
		{
			if (mCaretVisible)
				DrawCaret(mSelStart);
		}
	}
	else
	{
		// what lines need to be drawn?
		int32 startLine = mLines.PixelToLine(inRect.top);
		int32 endLine = mLines.PixelToLine(inRect.bottom)-1;
	
		DrawLines(startLine, endLine);	
	}
}


// ------------------------------------------------------------
// 	MouseDown
// ------------------------------------------------------------
// Move the caret and track the mouse while it's down

void
STEngine::MouseDown(
	BPoint	where)
{
	// should we even bother?
	if (/*(!mEditable) &&*/ (!mSelectable))
		return;
	
	// if this view isn't the focus, make it the focus and return
	if (!IsFocus()) {
		MakeFocus();
		return;
	}
	
	// hide the caret if it's visible	
	if (mCaretVisible)
		InvertCaret();
	
	BWindow		*window = Window();
	BMessage	*message = window->CurrentMessage();
	bool		shiftDown = message->FindInt32("modifiers") & B_SHIFT_KEY;
	uint32		clickButtons = message->FindInt32("buttons");
	int32		mouseOffset = PointToOffset(where);	

	// get the system-wide click speed
	bigtime_t 		clickSpeed = 0;
	bigtime_t 		nowTime = system_time();
	get_click_speed(&clickSpeed);
	bool			isMultiClick = clickSpeed > (nowTime - mClickTime);

	// should we initiate a drag?
	if (mSelStart != mSelEnd && !shiftDown && ! isMultiClick) {
		// was the primary button clicked?
		if (clickButtons == B_PRIMARY_MOUSE_BUTTON) {
			// was the click within the selection range?
			if ((mouseOffset >= mSelStart) && (mouseOffset < mSelEnd) && WaitMouseUp(where)) {
				InitiateDrag();
				return;
			}
		}
	}
	
	// is this a double/triple click, or is it a new click?
	if ( (clickSpeed > (nowTime - mClickTime)) &&
		 (mouseOffset == mClickOffset) ) {
		if (mClickCount > 1) {
			// triple click
			mClickCount = 0;
			mClickTime = 0;
		}
		else {
			// double click
			mClickCount = 2;
			mClickTime = nowTime;
		}
	}
	else {
		// new click
		mClickOffset = mouseOffset;
		mClickCount = 1;
		mClickTime = nowTime;
	
		if (!shiftDown)
			Select(mouseOffset, mouseOffset);
	}
	
	// no need to track the mouse if we can't select
	if (!mSelectable)
		return;
		
	// track the mouse while it's down
	int32		start = 0;
	int32		end = 0;
	int32		anchor = (mouseOffset > mSelStart) ? mSelStart : mSelEnd;
	BPoint		curMouse = where;
	uint32		buttons = 0;
	BScrollBar	*hScroll = ScrollBar(B_HORIZONTAL);
	BScrollBar	*vScroll = ScrollBar(B_VERTICAL);
	do {
		if (mouseOffset > anchor) {
			start = anchor;
			end = mouseOffset;
		}
		else {
			start = mouseOffset;
			end = anchor;
		}
		
		switch (mClickCount) {
			case 0:
				// triple click, select line by line
				start = mLines[OffsetToLine(start)]->offset;
				end = (mLines[OffsetToLine(end)] + 1)->offset;
				break;
												
			case 2:
			{
				// double click, select word by word
				int32 anOffset = 0;
				FindWord(start, &start, &anOffset);
				FindWord(end, &anOffset, &end);
				
				break;
			}
				
			default:
				// new click, select char by char
				break;			
		}
		if (shiftDown) {
			if (mouseOffset > anchor)
				start = anchor;
			else
				end = anchor;
		}
		Select(start, end);
		
		// Should we scroll the view?
		BRect bounds = Bounds();
		if (!bounds.Contains(curMouse)) {	
			int32 hDelta = 0;
			int32 vDelta = 0;
			
			if (hScroll != NULL) {
				if (curMouse.x < bounds.left)
					hDelta = (int32) (curMouse.x - bounds.left);
				else {
					if (curMouse.x > bounds.right)
						hDelta = (int32) (curMouse.x - bounds.right);
				}
				
				if (hDelta != 0)
					hScroll->SetValue(hScroll->Value() + hDelta);
			}
			
			if (vScroll != NULL) {
				if (curMouse.y < bounds.top)
					vDelta = (int32) (curMouse.y - bounds.top);
				else {
					if (curMouse.y > bounds.bottom)
						vDelta = (int32) (curMouse.y - bounds.bottom);
				}
				
				if (vDelta != 0)
					vScroll->SetValue(vScroll->Value() + vDelta);
			}
			
			if ((hDelta != 0) || (vDelta != 0))
				window->UpdateIfNeeded();
		}
		
		// Give those LED meters a rest
		snooze(30000);
		
		GetMouse(&curMouse, &buttons);
		mouseOffset = PointToOffset(curMouse);		
	} while (buttons != 0);
	
	if (mSelStart == mSelEnd)
		be_app->SetCursor(B_I_BEAM_CURSOR);
	else
		be_app->SetCursor(B_HAND_CURSOR);

}

const float kDragDist = 3.0;

// ------------------------------------------------------------
// 	WaitMouseUp
// ------------------------------------------------------------
//	Track the mouse to determine if we should initiate a drag.

bool
STEngine::WaitMouseUp(
	BPoint inPoint)
{
	while (true)
	{
		BPoint		newLoc;
		uint32		buttons;
	
		GetMouse(&newLoc, &buttons);
		
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
			return false;
		else
		if (abs((int)(inPoint.x - newLoc.x)) > kDragDist || abs((int)(inPoint.y - newLoc.y)) > kDragDist)
			return true;
		else
			snooze(30000);
	}
}

// ------------------------------------------------------------
// 	MouseMoved
// ------------------------------------------------------------
// Set the cursor to the I-Beam when it's above this view and
// track any drags that are over this view

void
STEngine::MouseMoved(
	BPoint			where,
	uint32			code, 
	const BMessage	*message)
{
	switch (code) {
		case B_ENTERED_VIEW:
			if ((mActive) && (message == NULL))
				SetMouse(where);
			break;

		case B_INSIDE_VIEW:
			if ((mEditable) && (message != NULL)) {
				if ((mDragOwner) || (CanDrop(message)))
					TrackDrag(where);
			}
			else
			if (mActive && mSelectable)
			{
				SetMouse(where);
			}
			break;

		case B_EXITED_VIEW:
			DragCaret(-1);
			if (mActive)
				be_app->SetCursor(B_HAND_CURSOR);
			break;

		default:
			BView::MouseMoved(where, code, message);
			break;
	}
}

// ------------------------------------------------------------
// 	SetMouse
// ------------------------------------------------------------
//	Utility to set the cursor depending on the button state
//	and whether the mouse is over a region of text that
//	can be dragged.  Only call this when the window is active.

void
STEngine::SetMouse(
	BPoint	where)
{
	ASSERT(mActive);
	
	// We only get here if the window is active and no message is being
	// dragged.  If the mouse button is down it means that the user
	// is dragging the scrollbar thumb and we don't want to set the cursor
	// in that case.
	uint32		buttons = Window()->CurrentMessage()->FindInt32("buttons");
	
	if (buttons == 0)
	{
		if (mSelStart == mSelEnd)
			be_app->SetCursor(B_I_BEAM_CURSOR);
		else
		{
			BRegion		hilite;
			GetHiliteRegion(mSelStart, mSelEnd, &hilite);
			if (hilite.Contains(where))
				be_app->SetCursor(B_HAND_CURSOR);
			else
				be_app->SetCursor(B_I_BEAM_CURSOR);
		}
	}
}

// ------------------------------------------------------------
// 	WindowActivated
// ------------------------------------------------------------
// Activate this view if and only if its window is active and
// the view is the focus of that window

void
STEngine::WindowActivated(
	bool	state)
{
	BView::WindowActivated(state);
	
	if (state && IsFocus()) {
		if (!mActive)
			Activate();
	}
	else {
		if (mActive)
			Deactivate();
	} 
}

// ------------------------------------------------------------
// 	KeyDown
// ------------------------------------------------------------
// Respond to key presses

void
STEngine::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
//	if (!mEditable)
//		return;
		
	// hide the cursor and caret
	be_app->ObscureCursor();
	if (mCaretVisible)
		InvertCaret();
	
	switch (inBytes[0]) {
		case B_BACKSPACE:
			if (mEditable)
				HandleBackspace();
			break;
			
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			HandleArrowKey(inBytes[0]);
			break;
		
		case B_DELETE:
			if (mEditable)
				HandleDelete();
			break;
			
		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			HandlePageKey(inBytes[0]);
			break;
			
		case B_ESCAPE:
		case B_INSERT:
		case B_FUNCTION_KEY:
			// ignore, pass key up to superclass
			BView::KeyDown(inBytes, inNumBytes);
			break;
			
		default:
			if (mEditable)
				HandleAlphaKey(inBytes, inNumBytes);
			break;
	}

	// draw the caret
	if (mSelStart == mSelEnd) {
		if (!mCaretVisible)
			InvertCaret();
	}
}

// ------------------------------------------------------------
// 	Pulse
// ------------------------------------------------------------
// Flash the caret at 1/2 second intervals and track drags

void
STEngine::Pulse()
{	
	if ((mActive) /*&& (mEditable)*/ && (mSelStart == mSelEnd)) {
		if (system_time() > (mCaretTime + 500000))
			InvertCaret();
	}
	
	if (mDragOwner) {
		BPoint	where;
		uint32 	buttons;
		GetMouse(&where, &buttons);
		
		if (buttons == 0) {
			// our drag must has been dropped elsewhere
			mDragOwner = false;
			Window()->SetPulseRate(500000.0);
		}
		else {
			if (mEditable)
				// assume that we know how to handle this drag 
				TrackDrag(where);
		}
	}
}

// ------------------------------------------------------------
// 	FrameResized
// ------------------------------------------------------------
// Update the scroll bars to mirror the visible area

void
STEngine::FrameResized(
	float	width,
	float 	height)
{
	BView::FrameResized(width, height);

	UpdateScrollbars();
}

// ------------------------------------------------------------
// 	MakeFocus
// ------------------------------------------------------------
// Activate this view if and only if its window is active and
// the view is the focus of that window

void
STEngine::MakeFocus(
	bool	focusState)
{
	BView::MakeFocus(focusState);
	
	if (focusState && Window()->IsActive()) {
		if (!mActive)
			Activate();
	}
	else {
		if (mActive)
			Deactivate();
	} 
}

// ------------------------------------------------------------
// 	MessageReceived
// ------------------------------------------------------------
// Check for dropped messages and respond to the standard 
// Cut, Copy, and Paste messages

void
STEngine::MessageReceived(
	BMessage	*message)
{
	// was this message dropped?
	if (message->WasDropped() && message->what != B_MESSAGE_NOT_UNDERSTOOD) {
		BPoint dropLoc;
		BPoint offset;
		 
		dropLoc = message->DropPoint(&offset);
		ConvertFromScreen(&dropLoc);
		ConvertFromScreen(&offset);

		if (!MessageDropped(message, dropLoc, offset))
			BView::MessageReceived(message);
		
		return;
	}

	switch (message->what) {
		case B_CUT:
			Cut();
			break;
			
		case B_COPY:
			Copy();
			break;
			
		case B_PASTE:
			Paste();
			break;
			
		default:
			BView::MessageReceived(message);
			break;
	}
}
