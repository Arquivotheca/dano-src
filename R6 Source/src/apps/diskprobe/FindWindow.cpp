//--------------------------------------------------------------------
//	
//	FindWindow.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <string.h>
#include <Box.h>

#include "DiskProbe.h"
#include "FindWindow.h"


//====================================================================

TFindWindow* TFindWindow::fFindWindow = NULL;

BRect TFindWindow::fLastPosition(BRect(100, 300, 100 + FIND_WIDTH, 300 + FIND_HEIGHT));

TFindWindow::TFindWindow(prefs *prefs)
			 :BWindow(TFindWindow::fLastPosition, "DiskProbe Find", B_TITLED_WINDOW,
			 								B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect	r;

	r = TFindWindow::fLastPosition;
	r.OffsetTo(0,0);
	BBox	*box = new BBox(r, "box", B_FOLLOW_NONE, B_WILL_DRAW,
		B_PLAIN_BORDER);
	AddChild(box);
	r.InsetBy(1,1);
	fFindView = new TFindView(r, prefs);
	box->AddChild(fFindView);
	fFindWindow = this;
}

//--------------------------------------------------------------------

TFindWindow::~TFindWindow(void)
{
	TFindWindow::fLastPosition = Frame();
	fFindWindow = NULL;
}

//--------------------------------------------------------------------

void TFindWindow::FindAgain(void)
{
	Lock();
	fFindView->DoFind();
	Unlock();
}

//--------------------------------------------------------------------

void TFindWindow::WindowGuess(BWindow *window)
{
	fFindView->WindowGuess(window);
}


//====================================================================

TFindView::TFindView(BRect rect, prefs *prefs)
		  :BView(rect, "", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	BRect	r;
	BRect	text;

	fPrefs = prefs;

	r = rect;
	r.bottom = FIND_BUTTON_Y1 - 8;
	r.InsetBy(8,8);

	text = r;
	text.OffsetTo(B_ORIGIN);
	text.InsetBy(2,2);
	fTextView = new BTextView(r, "", text, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE);
	AddChild(fTextView);

	r.Set(CASE_BUTTON_X1, CASE_BUTTON_Y1, CASE_BUTTON_X2, CASE_BUTTON_Y2);
	fCaseSensitive = new BCheckBox(r, "", CASE_TEXT, new BMessage(M_CASE));
	fCaseSensitive->SetValue(fPrefs->case_sensitive);
	AddChild(fCaseSensitive);

	r.Set(FIND_BUTTON_X1, FIND_BUTTON_Y1, FIND_BUTTON_X2, FIND_BUTTON_Y2);
	fFindButton = new BButton(r, "", FIND_BUTTON_TEXT, new BMessage(M_FIND_IT));
	AddChild(fFindButton);
	
	fWindowGuess = NULL;
}

//--------------------------------------------------------------------

void TFindView::AttachedToWindow(void)
{
	BView::AttachedToWindow();
	SetViewColor(216,216,216);
	Window()->SetDefaultButton(fFindButton);
	fCaseSensitive->SetTarget(this);
	fFindButton->SetTarget(this);
	fTextView->MakeFocus(true);
}

//--------------------------------------------------------------------

void TFindView::Draw(BRect where)
{
	BRect	r;

	r = fTextView->Frame();
	r.InsetBy(-1,-1);
	SetHighColor(128,128,128);
	StrokeRect(r);
	SetHighColor(0,0,0);
}

//--------------------------------------------------------------------

void TFindView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_FIND_IT:
			DoFind();
			break;

		case M_CASE:
			fPrefs->case_sensitive = fCaseSensitive->Value();
			break;

		default:
			BView::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TFindView::DoFind(void)
{
	char		*text;
	int32		i = 0;
	BMessage	msg(M_FIND_IT);
	BView		*view;
	BWindow		*window;

	fTextView->SelectAll();
	text = (char *)fTextView->Text();
	if ((text == NULL) || (text[0] == 0))
		return;

	window = fWindowGuess;
	if (window == NULL) {
		while ((window = be_app->WindowAt(i++))) {
			if ((view = window->FindView("d_header")))
				break;
		}
	}
	if (window == NULL)
		return;
		
	view = window->FindView("d_header");
	if (view) {
		if (window->Lock()) {
			msg.AddString("find_string", text);
			msg.AddBool("case", fPrefs->case_sensitive);
			msg.AddInt32("length", strlen(text));
			window->PostMessage(&msg, view);
			window->Unlock();
		}
	}
}

//--------------------------------------------------------------------

void TFindView::WindowGuess(BWindow *window)
{
	fWindowGuess = window;
}
