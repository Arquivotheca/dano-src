// ===========================================================================
//	FindWindow.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "FindWindow.h"

#include <TextView.h>
#include <Button.h>
#include <Application.h>
#include <stdlib.h>

#include "HTMLWindow.h"
#include "HTMLView.h"
#include "NPApp.h"

extern const char *kFindButtonTitle;
extern const char *kFindWindowTitle;

// ============================================================================
//	FindWindow is modeless...

#define FINDBUTTON 'find'

static BString sPreviousFind = "";

FindWindow* FindWindow::mFindWindow = NULL;
BRect FindWindow::mLastPosition(BRect(100,300,300,379));

class FindPanel : public BView {
public:
					FindPanel(BRect rect);
	virtual			~FindPanel();
	virtual	void	AttachedToWindow();
	virtual	void	MessageReceived(BMessage *msg);
	virtual	void	Draw(BRect updateRect);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
			void	Find();
	virtual void	MouseDown(BPoint point);

protected:
		BButton*	mFindButton;
		BTextView*	mBTextView;
};


void FindWindow::DoFind(BWindow *window, const char *text)
{
	if (window == NULL) {
		long i=0;
		while ((bool)(window = be_app->WindowAt(i++))) {	// Send the text to a waiting window
			if (window != mFindWindow) {
				if (dynamic_cast<HTMLWindow *>(window) != NULL)
					break;	// Found a window
			}
		}
	}
	
	if (window == NULL)
		return;
		
//	Found a window, send a find message
	
	BView *focus = window->CurrentFocus();
	if (!window->Lock())
		return;
	if (dynamic_cast<HTMLView *>(focus) == NULL)
		focus = dynamic_cast<HTMLView *>(window->FindView("NetPositive")->FindView("HTMLAreaView")->FindView(kHTMLViewName));
	window->Unlock();

	if (focus)
	{
		BMessage msg('find');
		msg.AddString("findthis",text);
		window->PostMessage(&msg, focus);
	}
}

FindPanel::FindPanel(BRect frame) : BView(frame, "FindPanel", B_FOLLOW_ALL, B_WILL_DRAW)
{
	const float inset = 2.0;
	BRect rect(frame);
	rect.InsetBy(12.0,12.0);
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	rect.bottom = rect.top + (fontHeight.ascent + fontHeight.descent) + (2 * inset);
	
	BRect text(rect);
	text.OffsetTo(B_ORIGIN);
	text.InsetBy(inset,inset);
	mBTextView = new DialogTextView(rect,"BTextView",text,B_FOLLOW_NONE ,B_WILL_DRAW);
	mBTextView->DisallowChar('\n');
	mBTextView->SetText(sPreviousFind.String());
	AddChild(mBTextView);
	
	rect.top = rect.bottom + 14.0;
	rect.right--;
	rect.left = rect.right - max_c(be_plain_font->StringWidth("Font") + 12.0, 66.0);
	// This is pretty much useless since BButton will automatically resize itself...
	rect.bottom = rect.top + (fontHeight.ascent + fontHeight.descent);
	mFindButton = new BButton(rect,"FINDBUTTON",kFindButtonTitle, new BMessage(FINDBUTTON),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	mFindButton->SetEnabled(sPreviousFind.Length());
	AddChild(mFindButton);
}

FindPanel::~FindPanel()
{
	sPreviousFind = mBTextView->Text();
}

void FindPanel::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(216,216,216);
	Window()->SetDefaultButton(mFindButton);
	mFindButton->SetTarget(this);
	mBTextView->MakeFocus(true);
	mBTextView->SelectAll();
	
	// Resize window if needed...
	if((abs((int)(Frame().bottom - mFindButton->Frame().bottom))) < 10.0)
		Window()->ResizeBy(0.0, 10.0 - abs((int)(Frame().bottom - mFindButton->Frame().bottom)));
}

void FindPanel::MouseDown(BPoint point)
{
	Window()->Activate();
	BView::MouseDown(point);
}

void TextBevel(BView& view, BRect r);

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

//	Send copy to html view?

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

//	Send the find message to the nearest html window

void FindPanel::Find()
{
	mBTextView->SelectAll();
	const char *text = mBTextView->Text();
	if (text == NULL || text[0] == 0) return;

	HTMLWindow *window = HTMLWindow::FrontmostHTMLWindow();
	if (window)
		FindWindow::DoFind(window, text);
}

// ============================================================================


FindWindow::FindWindow() : FFMWarpingWindow(FindWindow::mLastPosition, kFindWindowTitle, B_FLOATING_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect r = FindWindow::mLastPosition;
	r.OffsetTo(0,0);
	mFindPanel = new FindPanel(r);
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
