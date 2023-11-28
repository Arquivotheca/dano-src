//******************************************************************************
//
//
//	Copyright 2001, Be Incorporated
//
//******************************************************************************

#include "main.h"

#include "ColorSelector.h"

#include <Application.h>

#include <Button.h>
#include <ColorControl.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <StringView.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <Debug.h>
#include "StreamIO.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(int , char* [])
{
	BApplication	app("application/x-vnd.Be.ColorPref");
	BRect			default_rect(80, 60, 80+380, 60+160);
	BRect			wr(default_rect);

	BPath			path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("UI_settings");
		BFile	file(path.Path(), O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			BPoint	p;
			long n = file.ReadAttr("_color_wd_p", B_RAW_TYPE, 0, &p,
				sizeof(p));
			if (n == sizeof(p))
				wr.OffsetTo(p);
		}
	}

	BScreen	s;

	if (!s.Frame().Intersects(wr))
		wr = default_rect;

	(new ColorWindow(wr, "Colors"))->Show();
	app.Run();

	return 0;
}

#define LEFT_W_MARGIN	25
#define RIGHT_W_MARGIN	25
#define HORZ_W_MARGIN	(LEFT_W_MARGIN + RIGHT_W_MARGIN)
#define TOP_W_MARGIN	11
#define BOTTOM_W_MARGIN	44
#define VERT_W_MARGIN	(TOP_W_MARGIN + BOTTOM_W_MARGIN)

ColorWindow::ColorWindow(BRect frame, const char *title)
	: BWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect			wbounds;
	BRect			b;

	BMessage settings;
	BMessage names;
	get_ui_settings(&settings, &names);
	
	wbounds = frame;
	wbounds.OffsetTo(B_ORIGIN);
	b = wbounds;

	// Note that we explicitly don't use SetViewUIColor(), so that our
	// settings changes won't effect this window (too much).
	BView *topv = new BView(b, "top", B_FOLLOW_ALL, B_WILL_DRAW);
	topv->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topv);

	b.top += TOP_W_MARGIN;
	b.left += LEFT_W_MARGIN;
	b.right -= RIGHT_W_MARGIN;
	b.bottom -= BOTTOM_W_MARGIN;

	fSelector = new ColorSelector(	b, "Colors", "Color: ", settings, names,
									new BMessage(CMD_CHANGE_COLOR),
									B_FOLLOW_ALL);
	topv->AddChild(fSelector);
	fSelector->ResizeToPreferred();
	
	//BRect f = fSelector->Frame();
	
	BButton	*but;

	b.left = wbounds.left + 10;
	b.right = b.left + 100;
	b.top = wbounds.bottom - 35;
	b.bottom = b.top + 20;

	but = new BButton(b, "", "Defaults", new BMessage(CMD_DEFAULTS),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	but->ResizeToPreferred();
	but->MoveTo(wbounds.left+10, wbounds.bottom - 10 - but->Bounds().Height());
	topv->AddChild(but);
	fDefault = but;
	b = but->Frame();
	
	b.OffsetBy(but->Bounds().Width() + 9, 0);

	but = new BButton(b, "", "Revert", new BMessage(CMD_REVERT),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	topv->AddChild(but);
	but->SetEnabled(false);
	fRevert = but;
}

/*------------------------------------------------------------*/
ColorWindow::~ColorWindow()
{
}

/*------------------------------------------------------------*/

void ColorWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case CMD_CHANGE_COLOR: {
			BMessage changes;
			if (msg->FindMessage("changes", &changes) == B_OK) {
				update_ui_settings(changes);
			}
		} break;
		
		case CMD_DEFAULTS: {
			BMessage settings;
			if (get_default_settings(&settings) == B_OK) {
				BMessage colors;
				ColorSelector::ExtractColors(&colors, settings);
				update_ui_settings(colors);
			}
		} break;
		
		case CMD_REVERT: {
			update_ui_settings(fSelector->InitialColors());
		} break;
		
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

/*------------------------------------------------------------*/

void ColorWindow::CheckDirty()
{
	fRevert->SetEnabled(fSelector->IsDirty());
}

/*------------------------------------------------------------*/

bool ColorWindow::QuitRequested()
{
	BPath	path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("UI_settings");
		BFile	file(path.Path(), O_RDWR);
		if (file.InitCheck() == B_NO_ERROR) {
			BPoint p(Frame().LeftTop());
			file.WriteAttr("_color_wd_p", B_RAW_TYPE, 0, &p, sizeof(p));
		}
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

/*------------------------------------------------------------*/

status_t ColorWindow::UISettingsChanged(const BMessage* changes, uint32 )
{
	fSelector->Update(*changes);
	CheckDirty();
	return B_OK;
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
