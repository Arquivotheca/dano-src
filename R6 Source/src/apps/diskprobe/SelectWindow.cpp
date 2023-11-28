//--------------------------------------------------------------------
//	
//	SelectWindow.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <PopUpMenu.h>

#include "DiskProbe.h"
#include "SelectWindow.h"


//====================================================================

TSelectWindow::TSelectWindow(BRect rect)
			:BWindow(rect, "DiskProbe", B_TITLED_WINDOW, B_NOT_RESIZABLE |
														   B_NOT_ZOOMABLE)
{
	BFont			font = be_plain_font;
	BBox			*box;
	BButton			*button;
	BMenuField		*menu;
	BRadioButton	*radio;
	BRect			r;
	BStringView		*string;

	fType = W_SELECT_CANCEL;
	r = Bounds();
//+	r.InsetBy(-1, -1);
	box = new BBox(r, "box", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(box);

	r.Set(TEXT1_X1, TEXT1_Y1, TEXT1_X2, TEXT1_Y2);
	box->AddChild(string = new BStringView(r, "", TEXT1_TEXT));
	string->SetFont(&font);

	r.Set(TEXT2_X1, TEXT2_Y1, TEXT2_X2, TEXT2_Y2);
	box->AddChild(string = new BStringView(r, "", TEXT2_TEXT));
	string->SetFont(&font);

	r.Set(DEVICE_X1, DEVICE_Y1, DEVICE_X2, DEVICE_Y2);
	fDevice = new BRadioButton(r, "", DEVICE_TEXT, new BMessage(M_DEVICE));
	fDevice->SetValue(1);
	fDevice->SetFont(&font);
	box->AddChild(fDevice);

	r.Set(MENU_X1, MENU_Y1, MENU_X2, MENU_Y2);
	fDeviceMenu = new BPopUpMenu("");
	ScanDir("/dev/disk", fDeviceMenu);
	menu = new BMenuField(r, "", MENU_TEXT, fDeviceMenu, true,
				B_FOLLOW_ALL,
				B_WILL_DRAW |
				B_NAVIGABLE |
				B_NAVIGABLE_JUMP);
	menu->SetFont(&font);
	menu->SetDivider(font.StringWidth(MENU_TEXT) + 7);
	box->AddChild(menu);
	fDeviceMenu->ItemAt(0)->SetMarked(true);

	r.Set(FILE_X1, FILE_Y1, FILE_X2, FILE_Y2);
	box->AddChild(radio = new BRadioButton(r, "", FILE_TEXT, new BMessage(M_FILE)));
	radio->SetFont(&font);

	r.Set(CANCEL_BUTTON_X1, CANCEL_BUTTON_Y1, CANCEL_BUTTON_X2, CANCEL_BUTTON_Y2);
	button = new BButton(r, "cancel", CANCEL_BUTTON_TEXT, new BMessage(M_CANCEL));
	box->AddChild(button);

	r.Set(OK_BUTTON_X1, OK_BUTTON_Y1, OK_BUTTON_X2, OK_BUTTON_Y2);
	button = new BButton(r, "ok", OK_BUTTON_TEXT, new BMessage(M_OK));
	button->MakeDefault(true);
	box->AddChild(button);
}

//--------------------------------------------------------------------

TSelectWindow::~TSelectWindow(void)
{
	BMessage	msg(M_WINDOW_CLOSED);

	msg.AddInt32("kind", fType);
	be_app->PostMessage(&msg);
}

//--------------------------------------------------------------------

void TSelectWindow::MessageReceived(BMessage* msg)
{
	BMessage	message;

	switch(msg->what) {
		case M_OPEN_DEVICE:
			fDevice->SetValue(1);
			break;

		case M_OK:
			if (fDevice->Value()) {
				message.what = M_OPEN_DEVICE;
				message.AddString("path", fDeviceMenu->FindMarked()->Label());
			}
			else
				message.what = M_OPEN_FILE;
			be_app->PostMessage(&message);
			fType = W_SELECT_OK;

		case M_CANCEL:
			Quit();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TSelectWindow::ScanDir(const char *directory, BMenu *menu)
{
	const char			*name;
	BPath				path;
	BDirectory			dir;
	BEntry				entry;
	BMenuItem			*item;
	BMessage			*msg;

	dir.SetTo(directory);
	if (dir.InitCheck() == B_NO_ERROR) {
		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			entry.GetPath(&path);
			name = path.Path();
			if (entry.IsDirectory())
				ScanDir(name, menu);
			else if (!strstr(name, "rescan")) {
				msg = new BMessage(M_OPEN_DEVICE);
				msg->AddString("path", name);
				menu->AddItem(item = new BMenuItem(name, msg));
			}
		}
	}
}
