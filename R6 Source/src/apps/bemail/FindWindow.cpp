// ===========================================================================
//	FindWindow.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "FindWindow.h"
#include "Mail.h"

#include <TextView.h>
#include <Button.h>
#include <Application.h>
#include <String.h>
#include <Box.h>

// ============================================================================
void TextBevel(BView& view, BRect r)
{
	r.InsetBy(-1,-1);
	view.SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT));
	view.MovePenTo(r.left,r.bottom);
	view.StrokeLine(BPoint(r.left,r.top));
	view.StrokeLine(BPoint(r.right,r.top));
	view.SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view.StrokeLine(BPoint(r.right,r.bottom));
	view.StrokeLine(BPoint(r.left,r.bottom));
	r.InsetBy(-1,-1);
	view.SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	view.MovePenTo(r.left,r.bottom);
	view.StrokeLine(BPoint(r.left,r.top));
	view.StrokeLine(BPoint(r.right,r.top));
	view.SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_MAX_TINT));
	view.StrokeLine(BPoint(r.right,r.bottom));
	view.StrokeLine(BPoint(r.left,r.bottom));
}

//	FindWindow is modeless...

#define FINDBUTTON 'find'

static BString sPreviousFind = "";

FindWindow* FindWindow::mFindWindow = NULL;
BRect FindWindow::mLastPosition(BRect(100,300,300,374));

void FindWindow::DoFind(BWindow *window, const char *text)
{
	if (window == NULL) {
		long i=0;
		while ((bool)(window = be_app->WindowAt(i++))) {	// Send the text to a waiting window
			if (window != mFindWindow)
				if (dynamic_cast<TMailWindow *>(window) != NULL)
					break;	// Found a window
		}
	}
	
	/* ask that window who is in the front */
	window = dynamic_cast<TMailWindow *>(window)->FrontmostWindow();
	if (window == NULL)
		return;
		
//	Found a window, send a find message
	
	if (!window->Lock())
		return;
	BView *focus = window->FindView("m_content");
	window->Unlock();

	if (focus)
	{
		BMessage msg(M_FIND);
		msg.AddString("findthis",text);
		window->PostMessage(&msg, focus);
	}
}

FindPanel::FindPanel(BRect rect) : BBox(rect, "FindPanel", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	BRect r = Bounds();
	r.InsetBy(8,8);
	r.bottom -= 44;
	BRect text = r;
	text.OffsetTo(B_ORIGIN);
	text.InsetBy(2,2);

	mBTextView = new DialogTextView(r,"BTextView",text,B_FOLLOW_ALL,B_WILL_DRAW);
	mBTextView->DisallowChar('\n');
	mBTextView->SetText(sPreviousFind.String());
	mBTextView->MakeFocus();
	AddChild(mBTextView);
	
	mFindButton = new BButton(BRect(0,0,90,20),"FINDBUTTON","Find", new BMessage(FINDBUTTON),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(mFindButton);
	r = mFindButton->Bounds();
	
	mFindButton->MoveTo(Bounds().right - r.Width() - 8, Bounds().bottom - r.Height() - 8);
	mFindButton->SetEnabled(sPreviousFind.Length());
}

FindPanel::~FindPanel()
{
	sPreviousFind = mBTextView->Text();
}

void FindPanel::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	Window()->SetDefaultButton(mFindButton);
	mFindButton->SetTarget(this);
	mBTextView->MakeFocus(true);
	mBTextView->SelectAll();
}

void FindPanel::MouseDown(BPoint point)
{
	Window()->Activate();
	BView::MouseDown(point);
}

void FindPanel::Draw(BRect)
{
	TextBevel(*this,mBTextView->Frame());
}

void FindPanel::KeyDown(const char *, int32)
{
	int32 length = mBTextView->TextLength();
	bool enabled = mFindButton->IsEnabled();

	if (length > 0 && !enabled)
		mFindButton->SetEnabled(true);
	else if (length == 0 && enabled)
		mFindButton->SetEnabled(false);
}

void FindPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case FINDBUTTON:
			Find();
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void FindPanel::Find()
{
	mBTextView->SelectAll();
	const char *text = mBTextView->Text();
	if (text == NULL || text[0] == 0) return;

	BWindow *window = NULL;
	long i=0;
	while ((bool)(window = be_app->WindowAt(i++))) {	// Send the text to a waiting window
		if (window != FindWindow::mFindWindow)
			break;	// Found a window
	}

	if (window)
		FindWindow::DoFind(window, text);
}

// ============================================================================


FindWindow::FindWindow() :
	BWindow(FindWindow::mLastPosition, "Find", B_FLOATING_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	mFindPanel = new FindPanel(Bounds());
	AddChild(mFindPanel);
	mFindWindow = this;
	Show();
}

FindWindow::~FindWindow()
{
	FindWindow::mLastPosition = Frame();
	mFindWindow = NULL;
}

void FindWindow::Find(BWindow *window)
{
	if (mFindWindow == NULL) {
		mFindWindow = new FindWindow();
	} else
		mFindWindow->Activate();
}

void FindWindow::FindAgain(BWindow *window)
{
	if (mFindWindow) {
		mFindWindow->Lock();
		mFindWindow->mFindPanel->Find();
		mFindWindow->Unlock();
	} else if (sPreviousFind.Length() != 0)
		DoFind(window, sPreviousFind.String());
	else
		Find(window);
}

void FindWindow::SetFindString(const char *string)
{
	sPreviousFind = string;
}

const char *FindWindow::GetFindString()
{
	return sPreviousFind.String();
}


DialogTextView::DialogTextView(BRect frame, const char *name, BRect textRect, uint32 resizingMode, uint32 flags)
	: BTextView(frame, name, textRect, resizingMode, flags)
{
}

void DialogTextView::KeyDown(const char *bytes, int32 numBytes)
{
	BTextView::KeyDown(bytes, numBytes);
	if (Parent())
		Parent()->KeyDown(bytes, numBytes);
}

void DialogTextView::MouseDown(BPoint point)
{
	Window()->Activate();
	BTextView::MouseDown(point);
}

void DialogTextView::InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns)
{
	BTextView::InsertText(inText, inLength, inOffset, inRuns);
	if (Parent())
		Parent()->KeyDown(NULL, 0);
}

void DialogTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	if (Parent())
		Parent()->KeyDown(NULL, 0);
}
