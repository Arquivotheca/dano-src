/* ++++++++++

   FILE:  PS_Print.cpp
   REVS:  $Revision: 1.3 $
   NAME:  Robert Polic

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <fcntl.h>  
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <Node.h>
#include <Path.h>
#include <FindDirectory.h>

#include <PrintJob.h>
#include "PS_Print.h"

#include <Screen.h>
#include <ctype.h>

//====================================================================

BPrint::BPrint(BMessage *msg, char *name)
	   :BWindow(BRect(100, 100, 100 + PRINT_WIDTH, 100 + PRINT_HEIGHT),
				name, B_TITLED_WINDOW, B_NOT_RESIZABLE | 
									   B_NOT_MINIMIZABLE |
									   B_NOT_ZOOMABLE)
{
	BRect screenFrame = BScreen(this).Frame();
	BPoint pt = screenFrame.LeftTop() + BPoint((screenFrame.Width() - Bounds().Width())*0.5f, (screenFrame.Height() - Bounds().Height())*0.5f);
	if (screenFrame.Contains(pt))
		MoveTo(pt);

	char	print_name[256];

	fPrintMessage = msg;
	fResult = 0;

	sprintf(print_name, "%s Print", name);
	SetTitle(print_name);

	Lock();
	fView = new TPrintView(Bounds(), msg);
	AddChild(fView);

	fPrintSem = create_sem(0, "PrintSem");


	// DR9 feature
	
	fFilter = new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN,
								 &PrintKeyFilter);
	AddCommonFilter(fFilter);


	Unlock();
}

//--------------------------------------------------------------------

BPrint::~BPrint()
{
	Lock();
	RemoveCommonFilter(fFilter);
	Unlock();
	delete fFilter;
}

//--------------------------------------------------------------------

void BPrint::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case PRINT_OK:
			fView->UpdateMessage(fPrintMessage);
			fView->AddSaveToFileFlag(fPrintMessage);
			fResult = B_NO_ERROR;
			// by releasing this semaphore, the Go() method will unblock.
			release_sem(fPrintSem);
			break;

		case PRINT_CANCEL:
			fResult = B_ERROR;
			// by releasing this semaphore, the Go() method will unblock.
			release_sem(fPrintSem);
			break;

		case PRINT_COPIES:
		case RANGE_ALL:
		case RANGE_SELECTION:
		case RANGE_FROM:
		case RANGE_TO:
		case QUALITY_DRAFT:
		case QUALITY_GOOD:
		case QUALITY_BEST:
			PostMessage(msg, fView);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool BPrint::QuitRequested()
{
	fResult = B_ERROR;
	release_sem(fPrintSem);
	return false;
}

//--------------------------------------------------------------------

long BPrint::Go(void)
{
	long value;

	Show();

	acquire_sem(fPrintSem);
	delete_sem(fPrintSem);

	// synchronous call to close the alert window. Remember that this will
	// 'delete' the object that we're in. That's why the return value is
	// saved on the stack.
	value = fResult;
	Lock();
	Quit();
	return(value);
}

//--------------------------------------------------------------------

filter_result PrintKeyFilter(BMessage *msg, BHandler **target, BMessageFilter *filter)
{
	long		key;
	ulong		mods = 0;
	TPrintView	*view;

	BLooper *looper = filter->Looper();
	view = ((BPrint *)looper)->fView;

	if ((*target != view->fCopies->ChildAt(0)) &&
		(*target != view->fFrom->ChildAt(0)) &&
		(*target != view->fTo->ChildAt(0)))
		return B_DISPATCH_MESSAGE;

	mods = msg->FindInt32("modifiers");
	if (mods & B_COMMAND_KEY)
		return B_DISPATCH_MESSAGE;

	const uchar *bytes = NULL;
	if (msg->FindString("bytes", (const char **)&bytes) != B_NO_ERROR)
		return B_DISPATCH_MESSAGE;

	key = bytes[0];

	if (isdigit(key) || iscntrl(key))
		return B_DISPATCH_MESSAGE;

	if (isascii(key))
		return B_SKIP_MESSAGE;

	return B_DISPATCH_MESSAGE;
}


//====================================================================

TPrintView::TPrintView(BRect frame, BMessage *msg)
		   :BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fPageSettings = msg;

	if (msg->HasInt32("quality"))
		fPrintQuality = msg->FindInt32("quality" + QUALITY_DRAFT);
	else
		fPrintQuality = QUALITY_BEST;

	if (msg->HasInt32("first_page"))
	{
		begin = msg->FindInt32("first_page");
	}
	else
	{
		begin = 0;
	}
	
	if (msg->HasInt32("last_page"))
	{
		end = msg->FindInt32("last_page");
		if((end == -1) && (begin == 1))
		{
			begin = end = 0;
		}
	}
	else
	{
		end = 0;
	}
	
	if (msg->HasInt32("copies")) copies = msg->FindInt32("copies");
	else copies = 1;
}

//--------------------------------------------------------------------

void TPrintView::AttachedToWindow(void)
{
	BButton		*button;
	//BPopUpMenu	*menu;
	BRect		r;

	r.Set(COPIES_H, COPIES_V,
		  COPIES_H + COPIES_WIDTH, COPIES_V + COPIES_HEIGHT);
	fCopies = new BTextControl(r, "", COPIES_TEXT, "1",
								new BMessage(PRINT_COPIES));
	AddChild(fCopies);
	fCopies->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fCopies->SetDivider(StringWidth(COPIES_TEXT) + 7);
	char string[25];
	sprintf(string,"%ld",copies);
	fCopies->SetText(string);

	r.Set(ALL_H, ALL_V, ALL_H + ALL_WIDTH, ALL_V + ALL_HEIGHT);
	fAllButton = new BRadioButton(r, "", ALL_TEXT, new BMessage(RANGE_ALL));
	AddChild(fAllButton);
	Window()->Lock();
	fAllButton->SetValue(1);
	Window()->Unlock();

	r.Set(SELECTION_H, SELECTION_V,
		  SELECTION_H + SELECTION_WIDTH, SELECTION_V + SELECTION_HEIGHT);
	fFromButton = new BRadioButton(r, "", SELECTION_TEXT,
								   new BMessage(RANGE_SELECTION));
	AddChild(fFromButton);

	r.Set(FROM_H, FROM_V, FROM_H + FROM_WIDTH, FROM_V + FROM_HEIGHT);
	fFrom = new BTextControl(r, "", FROM_TEXT, "", new BMessage(RANGE_FROM));
	AddChild(fFrom);
	fFrom->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fFrom->SetDivider(StringWidth(FROM_TEXT) + 7);
	if (begin > 0) {
		char string[25];
		sprintf(string,"%ld",begin);
		fFrom->SetText(string);
	}
	fFrom->ChildAt(0)->ResizeBy(-1,-1);

	if(begin || end)
	{
		fFromButton->SetValue(1);
		fAllButton->SetValue(0);
	}
	

	r.Set(TO_H, TO_V, TO_H + TO_WIDTH, TO_V + TO_HEIGHT);
	fTo = new BTextControl(r, "", TO_TEXT, "", new BMessage(RANGE_TO));
	AddChild(fTo);
	fTo->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fTo->SetDivider(StringWidth(TO_TEXT) + 7);
	if (end > 0) {
		char string[25];
		sprintf(string,"%ld",end);
		fTo->SetText(string);
	}
	fTo->ChildAt(0)->ResizeBy(-1,-1);

	r.Set(PRINT_CANCEL_BUTTON_H, PRINT_CANCEL_BUTTON_V,
		  PRINT_CANCEL_BUTTON_H + PRINT_BUTTON_WIDTH,
		  PRINT_CANCEL_BUTTON_V + PRINT_BUTTON_HEIGHT);
	button = new BButton(r, "", PRINT_CANCEL_BUTTON_TEXT,
						new BMessage(PRINT_CANCEL));
	AddChild(button);

	r.Set(PRINT_SAVE_CHECKBOX_H, PRINT_SAVE_CHECKBOX_V,
		  PRINT_SAVE_CHECKBOX_H + PRINT_SAVE_CHECKBOX_WIDTH,
		  PRINT_SAVE_CHECKBOX_V + PRINT_SAVE_CHECKBOX_HEIGHT);

	fSaveToFileCheckBox = new BCheckBox(r, "FileSaveBox",
										PRINT_SAVE_CHECKBOX_TEXT, NULL);
	if(IsPrintToFile()){
		fSaveToFileCheckBox->SetValue(1);
		fSaveToFileCheckBox->SetEnabled(false);
	}
	AddChild(fSaveToFileCheckBox);
	
	
	r.Set(PRINT_OK_BUTTON_H, PRINT_OK_BUTTON_V,
		  PRINT_OK_BUTTON_H + PRINT_BUTTON_WIDTH,
		  PRINT_OK_BUTTON_V + PRINT_BUTTON_HEIGHT);
	button = new BButton(r, "", PRINT_OK_BUTTON_TEXT, new BMessage(PRINT_OK));
	AddChild(button);
	button->MakeDefault(TRUE);
}

//--------------------------------------------------------------------

void TPrintView::Draw(BRect rect)
{
	BRect	r;

	r = Bounds();

	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top + 1), BPoint(r.left, r.bottom - 1));
	StrokeLine(BPoint(r.left + 3, PRINT_LINE_V + 4),
			   BPoint(r.right - 3, PRINT_LINE_V + 4));

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.right - 1, r.bottom), BPoint(r.left, r.bottom));
	StrokeLine(BPoint(r.left + 3, PRINT_LINE_V + 3),
			   BPoint(r.right - 3, PRINT_LINE_V + 3));

	SetHighColor(0, 0, 0);
	MovePenTo(RANGE_H, RANGE_V);
	DrawString(RANGE_TEXT);
}

//--------------------------------------------------------------------

void TPrintView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case PRINT_COPIES:
			break;

		case RANGE_ALL:
		{
			Window()->Lock();
			fFromButton->SetValue(0);
			fAllButton->SetValue(1);
			Window()->Unlock();
			break;
		}

		case RANGE_SELECTION:
		{
			Window()->Lock();
			fFromButton->SetValue(1);
			fAllButton->SetValue(0);
			Window()->Unlock();
			break;
		}

		case RANGE_FROM:
		case RANGE_TO:		
			Window()->Lock();
			fFromButton->SetValue(1);
			fAllButton->SetValue(0);
			Window()->Unlock();
			break;

		case QUALITY_DRAFT:
		case QUALITY_GOOD:
		case QUALITY_BEST:
			fPrintQuality = msg->what;
			break;
	}
}

//--------------------------------------------------------------------

void TPrintView::AddSaveToFileFlag(BMessage *msg)
{
	if (fSaveToFileCheckBox->Value() == B_CONTROL_ON)
	{
		msg->AddBool("save", true);
	}
	else
	{
		msg->RemoveName("save");
	}
}

//--------------------------------------------------------------------

bool TPrintView::IsPrintToFile()
{
	BPath		path;
	char 		transport_name[256];
		
	find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true);
	path.Append ("printers/");
	path.Append(fPageSettings->FindString("current_printer"));

	BNode node(path.Path());
	node.ReadAttr("transport", B_STRING_TYPE, 0,	transport_name,	128);
	if(!strcmp(transport_name, "File")){
		return true;
	} else {
		return false;
	}
}

//--------------------------------------------------------------------
void TPrintView::UpdateMessage(BMessage *msg)
{
	long	start;
	long	end;

	if (msg->HasInt32("copies"))
		msg->ReplaceInt32("copies", atoi(fCopies->Text()));
	else
		msg->AddInt32("copies", atoi(fCopies->Text()));

	if (fAllButton->Value()) {
		start = 1;
		end = -1;
	}
	else {
		start = atoi(fFrom->Text());
		end = atoi(fTo->Text());		
	}
	if (msg->HasInt32("first_page"))
		msg->ReplaceInt32("first_page", start);
	else
		msg->AddInt32("first_page", start);

	if (msg->HasInt32("last_page"))
		msg->ReplaceInt32("last_page", end);
	else
		msg->AddInt32("last_page", end);

	if (msg->HasInt32("quality"))
		msg->ReplaceInt32("quality", fPrintQuality - QUALITY_DRAFT);
	else
		msg->AddInt32("quality", fPrintQuality - QUALITY_DRAFT);
}
