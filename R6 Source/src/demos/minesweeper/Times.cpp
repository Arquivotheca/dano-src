//--------------------------------------------------------------------
//	
//	Times.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <Application.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Times.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Path.h>

extern	TBestWindow		*best_window;
extern	BPath			settings_path;


//====================================================================

TBestWindow::TBestWindow(BRect rect, char *title)
			:BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE |
												   B_NOT_ZOOMABLE)
{
	BButton		*button;
	BRect		r;

	r.Set(BEST_RESET_H, BEST_BUTTON_V,
		  BEST_RESET_H + BEST_BUTTON_WIDTH,
		  BEST_BUTTON_V + BEST_BUTTON_HEIGHT);
	button = new BButton(r, "button", BEST_BUTTON1, new BMessage(1));
	AddChild(button);

	r.left = BEST_OK_H;
	r.right = r.left + BEST_BUTTON_WIDTH;
	button = new BButton(r, "button", BEST_BUTTON2, new BMessage(2));
	button->MakeDefault(true);
	AddChild(button);

	r = Frame();
	r.OffsetTo(0, 0);
	fView = new TBestView(r, "BestView");
	AddChild(fView);
}

//--------------------------------------------------------------------

void TBestWindow::MessageReceived(BMessage* theMessage)
{
	int32	time = 999;
	BFile	*file;

	switch(theMessage->what) {
		case 1:
			file = new BFile(settings_path.Path(), O_RDWR);
			if (file->InitCheck() == B_NO_ERROR) {
				file->WriteAttr("begin_time", B_INT16_TYPE, 0,
								&time, sizeof(time));
				file->WriteAttr("begin_name", B_STRING_TYPE, 0,
								FAST_DEFAULT_NAME, 256);
				file->WriteAttr("intermediate_time", B_INT16_TYPE, 0,
								&time, sizeof(time));
				file->WriteAttr("intermediate_name", B_STRING_TYPE, 0,
								FAST_DEFAULT_NAME, 256);
				file->WriteAttr("expert_time", B_INT16_TYPE, 0,
								&time, sizeof(time));
				file->WriteAttr("expert_name", B_STRING_TYPE, 0,
								FAST_DEFAULT_NAME, 256);
				fView->Draw(Bounds());
			}
			delete file;
			break;

		case 2:
			best_window = NULL;
			if (Lock())
				Close();
			break;

		default:
			inherited::MessageReceived(theMessage);
			break;
	}
}

//--------------------------------------------------------------------

bool TBestWindow::QuitRequested()
{
	best_window = NULL;
	return true;
}


//====================================================================

TBestView::TBestView(BRect rect, char *title)
		  :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW |
											B_FRAME_EVENTS |
											B_FULL_UPDATE_ON_RESIZE)
{
	rgb_color	c;

	c.red = c.green = c.blue = 216;
	SetViewColor(c);
}

//--------------------------------------------------------------------

void TBestView::Draw(BRect /* where */)
{
	char	string[512];
	char	name[256];
	int32	width;
	int32	time;
	BFile	*file;
	BRect	r;

	r = Bounds();
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	SetHighColor(136, 136, 136);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));

	SetHighColor(0, 0, 0);
	SetLowColor(216, 216, 216);
	SetFont(be_bold_font);
	SetFontSize(12);

	width = (int32)StringWidth(BEST_TEXT1);
	r = Bounds();
	MovePenTo((r.Width() - width) / 2, BEST_LINE1_V);
	DrawString(BEST_TEXT1);

	MovePenTo(BEST_CAT, BEST_LINE2_V);
	DrawString(BEST_TEXT2);
	MovePenTo(BEST_CAT, BEST_LINE3_V);
	DrawString(BEST_TEXT3);
	MovePenTo(BEST_CAT, BEST_LINE4_V);
	DrawString(BEST_TEXT4);

	file = new BFile(settings_path.Path(), O_RDWR);
	if (file->InitCheck() == B_NO_ERROR) {
		time = 999;
		file->ReadAttr("begin_time", B_INT16_TYPE, 0, &time, sizeof(time));
		sprintf(string, "%3d %s", time, BEST_TEXT5);
		SetHighColor(216, 216, 216);
		r.Set(BEST_TIME, BEST_LINE2_V - 12, Bounds().right - 12, BEST_LINE2_V + 2);
		FillRect(r);
		SetHighColor(0, 0, 0);
		MovePenTo(BEST_TIME, BEST_LINE2_V);
		DrawString(string);

		time = 999;
		file->ReadAttr("intermediate_time", B_INT16_TYPE, 0, &time, sizeof(time));
		sprintf(string, "%3d %s", time, BEST_TEXT5);
		SetHighColor(216, 216, 216);
		r.Set(BEST_TIME, BEST_LINE3_V - 12, Bounds().right - 12, BEST_LINE3_V + 2);
		FillRect(r);
		SetHighColor(0, 0, 0);
		MovePenTo(BEST_TIME, BEST_LINE3_V);
		DrawString(string);

		time = 999;
		file->ReadAttr("expert_time", B_INT16_TYPE, 0, &time, sizeof(time));
		sprintf(string, "%3d %s", time, BEST_TEXT5);
		SetHighColor(216, 216, 216);
		r.Set(BEST_TIME, BEST_LINE4_V - 12, Bounds().right - 12, BEST_LINE4_V + 2);
		FillRect(r);
		SetHighColor(0, 0, 0);
		MovePenTo(BEST_TIME, BEST_LINE4_V);
		DrawString(string);

		MovePenTo(BEST_NAME, BEST_LINE2_V);
		sprintf(name, "%s", FAST_DEFAULT_NAME);
		file->ReadAttr("begin_name", B_STRING_TYPE, 0, name, 256);
		DrawString(name);

		MovePenTo(BEST_NAME, BEST_LINE3_V);
		sprintf(name, "%s", FAST_DEFAULT_NAME);
		file->ReadAttr("intermediate_name", B_STRING_TYPE, 0, name, 256);
		DrawString(name);

		MovePenTo(BEST_NAME, BEST_LINE4_V);
		sprintf(name, "%s", FAST_DEFAULT_NAME);
		file->ReadAttr("expert_name", B_STRING_TYPE, 0, name, 256);
		DrawString(name);
	}
	delete file;

	r = Bounds();
	r.left += 10;
	r.right -= 10;
	r.top = BEST_LINE2_V - 15;
	r.bottom = BEST_LINE4_V + 7;
	SetHighColor(255, 255, 255);
	StrokeRect(r);
	r.OffsetBy(-1, -1);
	SetHighColor(136, 136, 136);
	StrokeRect(r);

	Sync();
}


//====================================================================

TFastWindow::TFastWindow(BRect rect, char *title, int32 type, char *name,
						 int32 time)
			:BWindow(rect, title, B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	BButton		*button;
	BRect		r;
	rgb_color	c;

	fName = name;
	fTime = time;
	fType = type;

	r.Set(FAST_TEXT_H + 3, FAST_TEXT_V + 3,
		  FAST_TEXT_H + FAST_TEXT_WIDTH - 3,
		  FAST_TEXT_V + FAST_TEXT_HEIGHT - 2);
	fText = new BTextControl(r, "", "", name, new BMessage(2));
	fText->SetDivider(0.0);
	c.red = c.green =c.blue = 216;
	((BTextView *)fText->ChildAt(0))->SetViewColor(c);
	AddChild(fText);

	r.Set(FAST_BUTTON_H, FAST_BUTTON_V,
		  FAST_BUTTON_H + FAST_BUTTON_WIDTH,
		  FAST_BUTTON_V + FAST_BUTTON_HEIGHT);
	button = new BButton(r, "button", FAST_BUTTON_TEXT, new BMessage(1));
	button->MakeDefault(true);
	AddChild(button);

	r = Bounds();
	r.OffsetTo(0, 0);
	fView = new TFastView(r, "FastView", type, fText);
	AddChild(fView);
	Lock();
	fText->MakeFocus(true);
	Unlock();
}

//--------------------------------------------------------------------

void TFastWindow::MessageReceived(BMessage* theMessage)
{
	int32		length;
	BMessage	message('FAST');

	switch(theMessage->what) {
		case 1:
			message.AddString("name", fText->Text());
			message.AddInt32("type", fType);
			message.AddInt32("time", fTime);
			be_app->PostMessage(&message);
			if (Lock())
				Close();
			break;

		default:
			break;
	}
}


//====================================================================

TFastView::TFastView(BRect rect, char *title, int32 type, BTextControl *text)
		  :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW |
											B_FRAME_EVENTS |
											B_FULL_UPDATE_ON_RESIZE)
{
	rgb_color	c;

	fType = type;
	fText = text;
	c.red = c.green = c.blue = 216;
	SetViewColor(c);
}

//--------------------------------------------------------------------

void TFastView::Draw(BRect where)
{
	int32	width;
	BRect	r;

	r = Bounds();
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	SetHighColor(136, 136, 136);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));

	SetHighColor(0, 0, 0);
	SetLowColor(216, 216, 216);
	SetFont(be_bold_font);
	SetFontSize(12);

	width = (int32)StringWidth(FAST_TEXT1);
	MovePenTo(r.left + ((r.Width() - width) / 2), FAST_LINE1_V);
	DrawString(FAST_TEXT1);

	switch (fType) {
		case 1:
			width = (int32)StringWidth(FAST_TEXT3);
			break;
		case 2:
			width = (int32)StringWidth(FAST_TEXT4);
			break;
		case 3:
			width = (int32)StringWidth(FAST_TEXT5);
			break;
	}
	MovePenTo(r.left + ((r.Width() - width) / 2), FAST_LINE2_V);
	switch (fType) {
		case 1:
			DrawString(FAST_TEXT3);
			break;
		case 2:
			DrawString(FAST_TEXT4);
			break;
		case 3:
			DrawString(FAST_TEXT5);
			break;
	}

	width = (int32)StringWidth(FAST_TEXT2);
	MovePenTo(r.left + ((r.Width() - width) / 2), FAST_LINE3_V);
	DrawString(FAST_TEXT2);
}
