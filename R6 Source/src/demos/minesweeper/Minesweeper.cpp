//--------------------------------------------------------------------
//	
//	Minesweeper.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Minesweeper.h"
#include "MinePicts.h"
#include "Times.h"
#include "Custom.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>

int32				width	= 8;
int32				height	= 8;
int32				bombs	= 10;
BMenuItem*			pause_menu;
TBestWindow*		best_window = NULL;
TCustomWindow*		custom = NULL;
BPath				settings_path;
static const char	*settings_fn = "Minesweeper_Settings";


//====================================================================

int main(void)
{	
	TMinesApp	*myApp;

	srand((uint32)(fmod(system_time(), 1000000000.0)));
	myApp = new TMinesApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TMinesApp::TMinesApp(void)
		  :BApplication("application/x-vnd.Be-BOMB"),
          fWindow(NULL),
          fPrefs(NULL)
{
	int32			vers = 0;
	BPath			dir_path;
	BDirectory		*dir;
	BPoint			win_pos;
	BMenuItem		*item;

	fMenu = new BPopUpMenu("Minesweeper", false, false);
	fMenu->AddItem(item = new BMenuItem("New Game", new BMessage(M_NEW), 'N'));
	item->SetTarget(be_app);
	fMenu->AddItem(item = pause_menu = new BMenuItem("Pause",
		new BMessage(M_PAUSE), 'P'));
	item->SetTarget(be_app);
	pause_menu->SetEnabled(false);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(item = new BMenuItem("Beginner", new BMessage(M_BEG), 'B'));
	item->SetTarget(be_app);
	fMenu->AddItem(item=new BMenuItem("Intermediate", new BMessage(M_INT), 'I'));
	item->SetTarget(be_app);
	fMenu->AddItem(item = new BMenuItem("Expert", new BMessage(M_EXP), 'E'));
	item->SetTarget(be_app);
	fMenu->AddItem(item = new BMenuItem("Custom...", new BMessage(M_CUS)));
	item->SetTarget(be_app);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(item=new BMenuItem("Best Times...", new BMessage(M_BEST), 'T'));
	item->SetTarget(be_app);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(item = new BMenuItem("About Minesweeper...",
						new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(item = new BMenuItem("Quit Minesweeper",
						new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);

//+	SetMainMenu(fMenu);

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &dir_path, true) == B_OK) {
		settings_path = dir_path;
		settings_path.Append (settings_fn);
		fPrefs = new BFile(settings_path.Path(), O_RDWR);
		if (fPrefs->InitCheck() != B_NO_ERROR) {
			dir = new BDirectory(dir_path.Path());
			if (dir->InitCheck() == B_NO_ERROR)
				dir->CreateFile(settings_fn, fPrefs);
			delete dir;
		}
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&vers, sizeof(int32));
			if (vers == 1) {
				fPrefs->Read(&win_pos, sizeof(BPoint));
				fPrefs->Read(&width, sizeof(int32));
				fPrefs->Read(&height, sizeof(int32));
				fPrefs->Read(&bombs, sizeof(int32));
			}
		}
		else {
			delete fPrefs;
			fPrefs = NULL;
		}
	}
	NewWindow(width, height, bombs);
}

//--------------------------------------------------------------------

TMinesApp::~TMinesApp(void)
{
	if (fPrefs) {
		delete fPrefs;
		fPrefs = NULL;
	}
}

//--------------------------------------------------------------------

void TMinesApp::AboutRequested(void)
{
	BAlert		*myAlert;

	myAlert = new BAlert("", "...by Robert Polic", "Big Deal");
	myAlert->Go();
}

//--------------------------------------------------------------------

void TMinesApp::MessageReceived(BMessage* msg)
{
	int32		x, y, b;
	BRect		r;

	switch (msg->what) {
		case M_NEW:
			fWindow->Lock();
			fWindow->fView->NewGame();
			fWindow->Unlock();
			pause_menu->SetEnabled(false);
			pause_menu->SetMarked(false);
			break;

		case M_PAUSE:
			fWindow->Lock();
			fWindow->fView->Pause();
			fWindow->Unlock();
			break;

		case M_BEG:
			if ((width != 8) || (height != 8) || (bombs != 10))
				NewWindow(8, 8, 10);
			else {
				fWindow->Lock();
				fWindow->fView->NewGame();
				fWindow->Unlock();
			}
			pause_menu->SetEnabled(false);
			pause_menu->SetMarked(false);
			break;

		case M_INT:
			if ((width != 16) || (height != 16) || (bombs != 40))
				NewWindow(16, 16, 40);
			else {
				fWindow->Lock();
				fWindow->fView->NewGame();
				fWindow->Unlock();
			}
			pause_menu->SetEnabled(false);
			pause_menu->SetMarked(false);
			break;

		case M_EXP:
			if ((width != 30) || (height != 16) || (bombs != 99))
				NewWindow(30, 16, 99);
			else {
				fWindow->Lock();
				fWindow->fView->NewGame();
				fWindow->Unlock();
			}
			pause_menu->SetEnabled(false);
			pause_menu->SetMarked(false);
			break;

		case M_CUS:
			if (custom)
				custom->Activate(true);
			else {
				BScreen screen( fWindow );
				BRect screen_frame = screen.Frame();
				r = fWindow->Frame();
				r.left += (r.Width() - CUSTOM_WIND_WIDTH) / 2;
				if (r.left < 10)
					r.left = 10;
				r.right = r.left + CUSTOM_WIND_WIDTH;
				if (r.right > (screen_frame.right - 10)) {
					r.right = (screen_frame.right - 10);
					r.left = r.right - CUSTOM_WIND_WIDTH;
				}
				r.top += (r.Height() - CUSTOM_WIND_HEIGHT) / 2;
				if (r.top < 10)
					r.top = 10;
				r.bottom = r.top + CUSTOM_WIND_HEIGHT;
				if (r.bottom > (screen_frame.bottom - 10)) {
					r.bottom = (screen_frame.bottom - 10);
					r.top = r.bottom - CUSTOM_WIND_HEIGHT;
				}
				custom = new TCustomWindow(r, width, height, bombs);
				custom->Show();
			}
			break;

		case M_CUSTOM:
			msg->FindInt32("width", &x);
			msg->FindInt32("height", &y);
			msg->FindInt32("bombs", &b);
			NewWindow(x, y, b);
			break;

		case M_BEST:
			if (best_window)
				best_window->Activate(TRUE);
			else {
				BScreen screen( fWindow );
				BRect screen_frame = screen.Frame();
				r = fWindow->Frame();
				r.left += (r.Width() - BEST_WIND_WIDTH) / 2;
				if (r.left < 10)
					r.left = 10;
				r.right = r.left + BEST_WIND_WIDTH;
				if (r.right > (screen_frame.right - 10)) {
					r.right = (screen_frame.right - 10);
					r.left = r.right - BEST_WIND_WIDTH;
				}
				r.top += (r.Height() - BEST_WIND_HEIGHT) / 2;
				if (r.top < 10)
					r.top = 10;
				r.bottom = r.top + BEST_WIND_HEIGHT;
				if (r.bottom > (screen_frame.bottom - 10)) {
					r.bottom = (screen_frame.bottom - 10);
					r.top = r.bottom - BEST_WIND_HEIGHT;
				}
				best_window = new TBestWindow(r, "Best Times");
				best_window->Show();
			}
			break;

			case 'FAST':
				SetTime(msg);
				break;

		default:
			inherited::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TMinesApp::NewWindow(int32 x, int32 y, int32 count)
{
	int32			n;
	int32			vers = 0;
	BPoint			win_pos;
	BRect			r;
	BScreen			screen( B_MAIN_SCREEN_ID );
	BRect			screen_frame = screen.Frame();

	if (fWindow) {
		r = fWindow->Frame();
		win_pos = r.LeftTop();
		if (fWindow->Lock())
			fWindow->Close();
	}
	else {
		r.left = BROWSER_WIND;
		r.top = TITLE_BAR_HEIGHT;
		if (fPrefs) {
			fPrefs->Seek(0, 0);
			fPrefs->Read(&vers, sizeof(int32));
			if (vers == 1)
				fPrefs->Read(&win_pos, sizeof(BPoint));
			if (screen_frame.Contains(win_pos))
				r.OffsetTo(win_pos);
		}
		else
			fPrefs = NULL;
	}

	width = x;
	height = y;
	bombs = count;

	r.right = r.left + (2 * BORDER1) + (2 * BORDER2) +
					   (2 * BORDER3) + (TILE * width) - 1;
	r.bottom = r.top + (2 * BORDER1) + (3 * BORDER2) + (2 * BORDER3) +
					   (TILE * height) + (2 * HEADER_BORDER) + HEADER_HEIGHT - 1;
	if (r.right > screen_frame.right) {
		n = (int32)(r.right - (screen_frame.right - 10));
		if (r.left - n > 0) {
			r.left -= n;
			r.right -= n;
		}
		else {
			r.right -= (r.left - 10);
			r.left = 10;
		}
	}
	if (r.bottom > screen_frame.bottom) {
		n = (int32)(r.bottom - (screen_frame.bottom - 10));
		if (r.top - n > 0) {
			r.top -= n;
			r.bottom -= n;
		}
		else {
			r.bottom -= (r.top - 10);
			r.top = 10;
		}
	}

	fWindow = new TMinesWindow(r, "Minesweeper");
	fWindow->Show();
}

//--------------------------------------------------------------------

void TMinesApp::SetTime(BMessage *msg)
{
	const char	*name;
	int32		time;
	int32		type;
	BFile		*file;

	msg->FindString("name", &name);
	msg->FindInt32("type", &type);
	msg->FindInt32("time", &time);

	file = new BFile(settings_path.Path(), O_RDWR);
	if (file->InitCheck() == B_NO_ERROR) {
		switch (type) {
			case 1:
				file->WriteAttr("begin_time", B_INT16_TYPE, 0,
								&time, sizeof(time));
				file->WriteAttr("begin_name", B_STRING_TYPE, 0,
								name, 256);
				break;
			case 2:
				file->WriteAttr("intermediate_time", B_INT16_TYPE, 0,
								&time, sizeof(time));
				file->WriteAttr("intermediate_name", B_STRING_TYPE, 0,
								name, 256);
				break;
			case 3:
				file->WriteAttr("expert_time", B_INT16_TYPE, 0,
								&time, sizeof(time));
				file->WriteAttr("expert_name", B_STRING_TYPE, 0,
								name, 256);
				break;
		}
		if (best_window) {
			best_window->Lock();
			best_window->fView->Draw(best_window->Bounds());
			best_window->Unlock();
		}
	}
	delete file;
}


//====================================================================

TMinesWindow::TMinesWindow(BRect rect, char *title)
			 :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE |
													 B_NOT_ZOOMABLE)
{
	BMenu		*menu;
	BMenuBar	*menubar;
	BMenuItem	*item;
	BRect		r;

	fPaused = false;

	r.Set(0, 0, 10000, 15);
	menubar = new BMenuBar(r, "");
	menu = new BMenu("File");
	menu->AddItem(item = new BMenuItem("New Game", new BMessage(M_NEW), 'N'));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("About Minesweeper...",
						new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Quit Minesweeper",
						new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	menubar->AddItem(menu);

	menu = new BMenu("Game");
	menu->AddItem(item = pause_menu = new BMenuItem("Pause",
		new BMessage(M_PAUSE), 'P'));
	item->SetTarget(be_app);
	pause_menu->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Beginner", new BMessage(M_BEG), 'B'));
	item->SetTarget(be_app);
	menu->AddItem(item=new BMenuItem("Intermediate", new BMessage(M_INT), 'I'));
	item->SetTarget(be_app);
	menu->AddItem(item = new BMenuItem("Expert", new BMessage(M_EXP), 'E'));
	item->SetTarget(be_app);
	menu->AddItem(item = new BMenuItem("Custom...", new BMessage(M_CUS)));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item=new BMenuItem("Best Times...", new BMessage(M_BEST), 'T'));
	item->SetTarget(be_app);
	menubar->AddItem(menu);

	AddChild(menubar);

	r = Frame();
	r.OffsetTo(0, menubar->Frame().bottom + 1);
	ResizeBy(0, menubar->Frame().bottom + 1);
	fView = new TMinesView(r, "MinesView");
	AddChild(fView);
	SetPulseRate(100000);

	AddShortcut('N', B_COMMAND_KEY, new BMessage(M_NEW));
	AddShortcut('P', B_COMMAND_KEY, new BMessage(M_PAUSE));
	AddShortcut('B', B_COMMAND_KEY, new BMessage(M_BEG));
	AddShortcut('I', B_COMMAND_KEY, new BMessage(M_INT));
	AddShortcut('E', B_COMMAND_KEY, new BMessage(M_EXP));
	AddShortcut('T', B_COMMAND_KEY, new BMessage(M_BEST));
}

//--------------------------------------------------------------------

void TMinesWindow::MessageReceived(BMessage* theMessage)
{
	switch(theMessage->what) {
		case M_NEW:
		case M_PAUSE:
		case M_BEG:
		case M_INT:
		case M_EXP:
		case M_BEST:
			DetachCurrentMessage();
			be_app->PostMessage(theMessage);
			break;
		default:
			inherited::MessageReceived(theMessage);
			break;
	}
}

//--------------------------------------------------------------------

bool TMinesWindow::QuitRequested()
{
	int		ref;
	int32	vers = 1;
	BPoint	win_pos;
	BRect	r;

	r = Frame();
	win_pos = r.LeftTop();
	if (((TMinesApp*)be_app)->fPrefs) {
		((TMinesApp*)be_app)->fPrefs->Seek(0, 0);
		((TMinesApp*)be_app)->fPrefs->Write(&vers, sizeof(int32));
		((TMinesApp*)be_app)->fPrefs->Write(&win_pos, sizeof(BPoint));
		((TMinesApp*)be_app)->fPrefs->Write(&width, sizeof(int32));
		((TMinesApp*)be_app)->fPrefs->Write(&height, sizeof(int32));
		((TMinesApp*)be_app)->fPrefs->Write(&bombs, sizeof(int32));
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//--------------------------------------------------------------------

void TMinesWindow::WindowActivated(bool active)
{
	if ((active) && (fPaused)) {
		fPaused = false;
		fView->Pause();
	}
	else if ((!active) && (fView->fGameState != G_PAUSED)) {
		fPaused = true;
		fView->Pause();
	}
}


//====================================================================

TMinesView::TMinesView(BRect rect, char *title)
		   :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW |
											 B_FRAME_EVENTS |
											 B_FULL_UPDATE_ON_RESIZE |
											 B_PULSE_NEEDED)
{
	fBombs = bombs;
	fTime = 0;
	fBoard = NULL;

	NewGame();
}

//--------------------------------------------------------------------

TMinesView::~TMinesView()
{
	int32	loop;

	for (loop = T0; loop <= T_BOMB_WRONG; loop++)
		delete fTiles[loop];
	for (loop = L0; loop <= LD; loop++)
		delete fLEDs[loop];
	for (loop = F_UP; loop <= F_LOST; loop++)
		delete fFaces[loop];
}

//--------------------------------------------------------------------

void TMinesView::AttachedToWindow()
{
	uchar		*bits;
	BRect		r;
	rgb_color	c;

	c.red = c.green = c.blue = 184;
	SetViewColor(c);

	r.Set(0, 0, ((TILE_WIDTH + 7) & 0xfff8) - 1, TILE_HEIGHT);
	bits = TILE_0;
	fTiles[T0] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T0]->SetBits((char*)bits, fTiles[T0]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_1;
	fTiles[T1] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T1]->SetBits((char*)bits, fTiles[T1]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_2;
	fTiles[T2] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T2]->SetBits((char*)bits, fTiles[T2]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_3;
	fTiles[T3] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T3]->SetBits((char*)bits, fTiles[T3]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_4;
	fTiles[T4] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T4]->SetBits((char*)bits, fTiles[T4]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_5;
	fTiles[T5] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T5]->SetBits((char*)bits, fTiles[T5]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_6;
	fTiles[T6] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T6]->SetBits((char*)bits, fTiles[T6]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_7;
	fTiles[T7] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T7]->SetBits((char*)bits, fTiles[T7]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = TILE_8;
	fTiles[T8] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T8]->SetBits((char*)bits, fTiles[T8]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = HIDE_BLANK;
	fTiles[T_BLANK] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T_BLANK]->SetBits((char*)bits, fTiles[T_BLANK]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = HIDE_FLAG;
	fTiles[T_FLAG] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T_FLAG]->SetBits((char*)bits, fTiles[T_FLAG]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = HIDE_GUESS;
	fTiles[T_GUESS] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T_GUESS]->SetBits((char*)bits, fTiles[T_GUESS]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = BOMB_OK;
	fTiles[T_BOMB_OK] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T_BOMB_OK]->SetBits((char*)bits, fTiles[T_BOMB_OK]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = BOMB_DEAD;
	fTiles[T_BOMB_DEAD] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T_BOMB_DEAD]->SetBits((char*)bits, fTiles[T_BOMB_DEAD]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = BOMB_WRONG;
	fTiles[T_BOMB_WRONG] = new BBitmap(r, B_COLOR_8_BIT);
	fTiles[T_BOMB_WRONG]->SetBits((char*)bits, fTiles[T_BOMB_WRONG]->BitsLength(), 0, B_COLOR_8_BIT);

	r.Set(0, 0, ((LED_WIDTH + 7) & 0xfff8) - 1, LED_HEIGHT);
	bits = LED_0;
	fLEDs[L0] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L0]->SetBits((char*)bits, fLEDs[L0]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_1;
	fLEDs[L1] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L1]->SetBits((char*)bits, fLEDs[L1]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_2;
	fLEDs[L2] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L2]->SetBits((char*)bits, fLEDs[L2]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_3;
	fLEDs[L3] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L3]->SetBits((char*)bits, fLEDs[L3]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_4;
	fLEDs[L4] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L4]->SetBits((char*)bits, fLEDs[L4]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_5;
	fLEDs[L5] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L5]->SetBits((char*)bits, fLEDs[L5]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_6;
	fLEDs[L6] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L6]->SetBits((char*)bits, fLEDs[L6]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_7;
	fLEDs[L7] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L7]->SetBits((char*)bits, fLEDs[L7]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_8;
	fLEDs[L8] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L8]->SetBits((char*)bits, fLEDs[L8]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_9;
	fLEDs[L9] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[L9]->SetBits((char*)bits, fLEDs[L9]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_DASH;
	fLEDs[LDASH] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LDASH]->SetBits((char*)bits, fLEDs[LDASH]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_P;
	fLEDs[LP] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LP]->SetBits((char*)bits, fLEDs[LP]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_A;
	fLEDs[LA] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LA]->SetBits((char*)bits, fLEDs[LA]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_U;
	fLEDs[LU] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LU]->SetBits((char*)bits, fLEDs[LU]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_5;
	fLEDs[LS] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LS]->SetBits((char*)bits, fLEDs[LS]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_E;
	fLEDs[LE] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LE]->SetBits((char*)bits, fLEDs[LE]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = LED_0;
	fLEDs[LD] = new BBitmap(r, B_COLOR_8_BIT);
	fLEDs[LD]->SetBits((char*)bits, fLEDs[LD]->BitsLength(), 0, B_COLOR_8_BIT);

	r.Set(0, 0, ((FACE_WIDTH + 7) & 0xfff8) - 1, FACE_HEIGHT);
	bits = FACE_UP;
	fFaces[F_UP] = new BBitmap(r, B_COLOR_8_BIT);
	fFaces[F_UP]->SetBits((char*)bits, fFaces[F_UP]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = FACE_DOWN;
	fFaces[F_DOWN] = new BBitmap(r, B_COLOR_8_BIT);
	fFaces[F_DOWN]->SetBits((char*)bits, fFaces[F_DOWN]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = FACE_GUESS;
	fFaces[F_GUESS] = new BBitmap(r, B_COLOR_8_BIT);
	fFaces[F_GUESS]->SetBits((char*)bits, fFaces[F_GUESS]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = FACE_WON;
	fFaces[F_WON] = new BBitmap(r, B_COLOR_8_BIT);
	fFaces[F_WON]->SetBits((char*)bits, fFaces[F_WON]->BitsLength(), 0, B_COLOR_8_BIT);
	bits = FACE_LOST;
	fFaces[F_LOST] = new BBitmap(r, B_COLOR_8_BIT);
	fFaces[F_LOST]->SetBits((char*)bits, fFaces[F_LOST]->BitsLength(), 0, B_COLOR_8_BIT);

	MakeFocus(true);
}

//--------------------------------------------------------------------

void TMinesView::Draw(BRect where)
{
	int32	loop;
	float	temp;
	BRect	r;
	BRect	header;

	// Window shading
	r = Bounds();
	for (loop = 0; loop < BORDER1; loop++) {
		SetHighColor(WHITE);
		StrokeLine(BPoint(r.left, r.top), BPoint(r.right - 1, r.top));
		StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
		SetHighColor(DARK_GRAY);
		StrokeLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom));
		StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));
		r.InsetBy(1, 1);
	}
	r.InsetBy(BORDER2, BORDER2);
	header = r;
	header.bottom = header.top + (2 * HEADER_BORDER) + HEADER_HEIGHT - 1;
	for (loop = 0; loop < HEADER_BORDER; loop++) {
		SetHighColor(DARK_GRAY);
		StrokeLine(BPoint(header.left, header.top),
				   BPoint(header.right - 1, header.top));
		StrokeLine(BPoint(header.left, header.top),
				   BPoint(header.left, header.bottom));
		SetHighColor(WHITE);
		StrokeLine(BPoint(header.right, header.top),
				   BPoint(header.right, header.bottom));
		StrokeLine(BPoint(header.left + 1, header.bottom),
				   BPoint(header.right, header.bottom));
		header.InsetBy(1, 1);
	}
	DrawFace();

	temp = header.right;
	header.top += 4;
	header.bottom -= 4;
	header.left += 4;
	header.right = header.left + LED_SIZE + 2;
	SetHighColor(DARK_GRAY);
	StrokeRect(header);
	DrawBombs();

	header.right = temp - 4;
	header.left = header.right - (LED_SIZE + 2);
	StrokeRect(header);
	DrawTime();

	r.top += HEADER_HEIGHT + (2 * HEADER_BORDER) + BORDER2;
	for (loop = 0; loop < BORDER3; loop++) {
		SetHighColor(DARK_GRAY);
		StrokeLine(BPoint(r.left, r.top), BPoint(r.right - 1, r.top));
		StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
		SetHighColor(WHITE);
		StrokeLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom));
		StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));
		r.InsetBy(1, 1);
	}
	r.left = (int)(where.left - (BORDER1 + BORDER2 + BORDER3)) / (TILE_WIDTH + 1);
	if (r.left < 0)
		r.left = 0;
	r.right = (int)(where.right - (BORDER1 + BORDER2 + BORDER3)) / (TILE_WIDTH + 1);
	if (r.right > (width - 1))
		r.right = width - 1;
	r.top = (int)(where.top - (BORDER1 + BORDER2 + (2 * HEADER_BORDER) +
			HEADER_HEIGHT + BORDER2 + BORDER3)) / (TILE_HEIGHT + 1);
	if (r.top < 0)
		r.top = 0;
	r.bottom = (int)(where.bottom - (BORDER1 + BORDER2 + (2 * HEADER_BORDER) +
				HEADER_HEIGHT + BORDER2 + BORDER3)) / (TILE_HEIGHT + 1);
	if (r.bottom > (height - 1))
		r.bottom = height - 1;
	if ((r.right >= r.left) && (r.bottom >= r.top))
		DrawBoard(r);
}

//--------------------------------------------------------------------

void TMinesView::KeyDown(const char *key, int32 count)
{
	if ((fGameState == G_FINISHED) && (key[0] == ' ')) {
		pause_menu->SetEnabled(false);
		pause_menu->SetMarked(false);
		NewGame();
	}
}

//--------------------------------------------------------------------

void TMinesView::MouseDown(BPoint thePoint)
{
	int32		x;
	int32		y;
	BMenuItem	*m_item;
	BPoint		where;
	BRect		r;

	r = Bounds();
	r.left = (r.Width() - FACE_WIDTH) / 2;
	r.right = r.left + FACE_WIDTH;
	r.top = BORDER1 + BORDER2 + HEADER_BORDER + 4;
	r.bottom = r.top + FACE_HEIGHT;
	if ((r.Contains(thePoint)) &&
		(Window()->CurrentMessage()->FindInt32("buttons") == 1)) {
		TrackFace(r);
		return;
	}

	x = (int)(thePoint.x - (BORDER1 + BORDER2 + BORDER3)) / (TILE_WIDTH + 1);
	if ((thePoint.x < (BORDER1 + BORDER2 + BORDER3)) || (x > (width - 1)))
		x = -1;
	if ((x < 0) || (x > (width - 1)))
		goto app_menu;
	y = (int)(thePoint.y - (BORDER1 + BORDER2 + (2 * HEADER_BORDER) +
		HEADER_HEIGHT + BORDER2 + BORDER3)) / (TILE_HEIGHT + 1);
	if ((thePoint.y < (BORDER1 + BORDER2 + (2 * HEADER_BORDER) + HEADER_HEIGHT +
		BORDER2 + BORDER3)) || (y > (height - 1)))
		y = -1;
	if ((y < 0) || (y > (height - 1)))
		goto app_menu;
	if ((fGameState == G_STARTED) || (fGameState == G_NEW)) {
		TrackTile(x, y, thePoint, Window()->CurrentMessage()->FindInt32("buttons"));
		return;
	}

app_menu:
	if (Window()->CurrentMessage()->FindInt32("buttons") > 1) {
		where = thePoint;
		ConvertToScreen(&where);
		m_item = ((TMinesApp*)be_app)->fMenu->Go(where, true);
		return;
	}

}

//--------------------------------------------------------------------

void TMinesView::Pulse()
{
	int32	seconds;

	if ((fGameState == G_STARTED) && (fTime < 999)) {
		seconds = (int32)((system_time() - fStartTime) / 1000000);
		if (fTime != seconds) {
			fTime = seconds;
			if (fTime > 999)
				fTime = 999;
			DrawTime();
		}
	}
}

//--------------------------------------------------------------------

void TMinesView::DrawBoard(BRect board)
{
	int32	x;
	int32	y;
	int32	left;
	int32	top;
	BRect	r;
	BRect	sr;

	sr.Set(0, 0, TILE_WIDTH, TILE_HEIGHT);
	left = BORDER1 + BORDER2 + BORDER3;
	top = BORDER1 + BORDER2 + (HEADER_BORDER * 2) +
		  HEADER_HEIGHT + BORDER2 + BORDER3;

	if (board.left < 0)
		board.left = 0;
	if (board.right >= width)
		board.right = (width - 1);
	if (board.top < 0)
		board.top = 0;
	if (board.bottom >= height)
		board.bottom = (height - 1);

	for (y = (int32)board.top; y <= (int32)board.bottom; y++) {
		for (x = (int32)board.left; x <= (int32)board.right; x++) {
			r.Set(left + (x * (TILE_WIDTH + 1)),
				  top + (y * (TILE_HEIGHT + 1)),
				  left + ((x + 1) * (TILE_WIDTH + 1)) - 1,
				  top + ((y + 1) * (TILE_HEIGHT + 1)) - 1);
			if (fGameState == G_PAUSED)
				DrawBitmapAsync(fTiles[T_BLANK], sr, r);
			else
				DrawBitmapAsync(fTiles[fBoard[y * width + x].state], sr, r);
		}
	}
}

//--------------------------------------------------------------------

void TMinesView::DrawBombs()
{
	bool	neg = false;
	int32	loop;
	int32	index;
	int32	bombs;
	BRect	r;
	BRect	sr;

	bombs = fBombs;
	if (bombs < 0) {
		neg = true;
		bombs *= -1;
	}
	r = Bounds();
	r.left += BORDER1 + BORDER2 + HEADER_BORDER + 4 + LED_BORDER;
	r.right = r.left + LED_WIDTH;
	r.top = BORDER1 + BORDER2 + HEADER_BORDER + 4 + LED_BORDER;
	r.bottom = r.top + LED_HEIGHT;
	sr.Set(0, 0, LED_WIDTH, LED_HEIGHT);
	if (fGameState == G_PAUSED) {
		DrawBitmapAsync(fLEDs[LP], sr, r);
		r.left = r.right + 1;
		r.right = r.left + LED_WIDTH;
		DrawBitmapAsync(fLEDs[LA], sr, r);
		r.left = r.right + 1;
		r.right = r.left + LED_WIDTH;
		DrawBitmapAsync(fLEDs[LU], sr, r);
	}
	else {
		for (loop = 100; loop >= 1; loop /= 10) {
			index = bombs / loop;
			bombs -= (index * loop);
			if ((neg) && (loop == 100))
				DrawBitmapAsync(fLEDs[LDASH], sr, r);
			else
				DrawBitmapAsync(fLEDs[index], sr, r);
			r.left = r.right + 1;
			r.right = r.left + LED_WIDTH;
		}
	}
}

//--------------------------------------------------------------------

void TMinesView::DrawFace()
{
	BRect	r;
	BRect	sr;

	r = Bounds();
	r.left = (r.Width() - FACE_WIDTH) / 2;
	r.right = r.left + FACE_WIDTH;
	r.top = BORDER1 + BORDER2 + HEADER_BORDER + 4;
	r.bottom = r.top + FACE_HEIGHT;

	sr.Set(0, 0, FACE_WIDTH, FACE_HEIGHT);
	DrawBitmapAsync(fFaces[fFaceState], sr, r);
}

//--------------------------------------------------------------------

void TMinesView::DrawTime()
{
	int32	loop;
	int32	index;
	int32	time;
	BRect	r;
	BRect	sr;

	time = fTime;
	r = Bounds();
	r.left = r.right - (BORDER1 + BORDER2 + HEADER_BORDER + 4 +
						LED_BORDER + LED_SIZE);
	r.right = r.left + LED_WIDTH;
	r.top = BORDER1 + BORDER2 + HEADER_BORDER + 4 + LED_BORDER;
	r.bottom = r.top + LED_HEIGHT;
	sr.Set(0, 0, LED_WIDTH, LED_HEIGHT);
	if (fGameState == G_PAUSED) {
		DrawBitmapAsync(fLEDs[LS], sr, r);
		r.left = r.right + 1;
		r.right = r.left + LED_WIDTH;
		DrawBitmapAsync(fLEDs[LE], sr, r);
		r.left = r.right + 1;
		r.right = r.left + LED_WIDTH;
		DrawBitmapAsync(fLEDs[LD], sr, r);
	}
	else {
		for (loop = 100; loop >= 1; loop /= 10) {
			index = time / loop;
			time -= (index * loop);
			DrawBitmapAsync(fLEDs[index], sr, r);
			r.left = r.right + 1;
			r.right = r.left + LED_WIDTH;
		}
	}
}

//--------------------------------------------------------------------

int32 TMinesView::Bombs(int32 x, int32 y)
{
	if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) 
		return 0;
	else if (fBoard[y * width + x].flags == F_FREE)
		return 0;
	else
		return 1;
}

//--------------------------------------------------------------------

void TMinesView::MoveIt(int32 x, int32 y)
{
	bool		done = false;
	int32		count;
	int32		flag;
	int32		index = 0;
	position	*pos;

	if (fBoard[y * width + x].state != T_BLANK)
		return;

	pos = (position *)malloc(sizeof(position) * width * height);
	do {
		count = 
		// check upper left
			Bombs(x - 1, y - 1) +
		// check upper
			Bombs(x, y - 1) +
		// check upper right
			Bombs(x + 1, y - 1) +
		// check left
			Bombs(x - 1, y) +
		// check right
			Bombs(x + 1, y) +
		// check bottom left
			Bombs(x - 1, y + 1) +
		// check bottom
			Bombs(x, y + 1) +
		// check bottom right
			Bombs(x + 1, y + 1);

		fBoard[y * width + x].state = count;
		DrawBoard(BRect(x, y, x, y));
		if (!count) {
			pos[index].x = x;
			pos[index].y = y;
			pos[index].flags = 0;
		}
		else if (!index)
			break;
		else {
			index--;
			x = pos[index].x;
			y = pos[index].y;
		}

		for (;;) {
			flag = pos[index].flags;
			if (!(flag & TOP_LEFT) && (y) && (x) &&
				(fBoard[(y - 1) * width + (x - 1)].state == T_BLANK)) {
				pos[index].flags |= TOP_LEFT;
				y--;
				x--;
				index++;
				break;
			}
			else if (!(flag & TOP) && (y) &&
				(fBoard[(y - 1) * width + x].state == T_BLANK)) {
				pos[index].flags |= TOP;
				y--;
				index++;
				break;
			}
			else if (!(flag & TOP_RIGHT) && (y) && (x < (width - 1)) &&
				(fBoard[(y - 1) * width + (x + 1)].state == T_BLANK)) {
				pos[index].flags |= TOP_RIGHT;
				y--;
				x++;
				index++;
				break;
			}
			else if (!(flag & LEFT) && (x) &&
				(fBoard[y * width + (x - 1)].state == T_BLANK)) {
				pos[index].flags |= LEFT;
				x--;
				index++;
				break;
			}
			else if (!(flag & RIGHT) && (x < (width - 1)) &&
				(fBoard[y * width + (x + 1)].state == T_BLANK)) {
				pos[index].flags |= RIGHT;
				x++;
				index++;
				break;
			}
			else if (!(flag & BOTTOM_LEFT) && (y < (height - 1)) && (x) &&
				(fBoard[(y + 1) * width + (x - 1)].state == T_BLANK)) {
				pos[index].flags |= BOTTOM_LEFT;
				y++;
				x--;
				index++;
				break;
			} else if (!(flag & BOTTOM) && (y < (height - 1)) &&
				(fBoard[(y + 1) * width + x].state == T_BLANK)) {
				pos[index].flags |= BOTTOM;
				y++;
				index++;
				break;
			}
			else if (!(flag & BOTTOM_RIGHT) && (y < (height - 1)) && (x < (width - 1)) &&
				(fBoard[(y + 1) * width + (x + 1)].state == T_BLANK)) {
				pos[index].flags |= BOTTOM_RIGHT;
				y++;
				x++;
				index++;
				break;
			}
			else if (index) {
				index--;
				x = pos[index].x;
				y = pos[index].y;
			}
			else {
				done = true;
				break;
			}
		}
	} while (!done);
	free(pos);
}

//--------------------------------------------------------------------

void TMinesView::NewGame()
{
	int32	x;
	int32	y;
	int32	range;
	int32	n;
	BRect	r;

	fGameState = G_NEW;
	fFaceState = F_UP;
	DrawFace();

	if (!fBoard)
		fBoard = (tile *)malloc((width * height) * sizeof(tile));

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (fBoard[y * width + x].state != T_BLANK) {
				fBoard[y * width + x].state = T_BLANK;
				r.Set(x, y, x, y);
				DrawBoard(r);
			}
			fBoard[y * width + x].flags = F_FREE;
		}
	}

	range = width * height;
	for (x = 0; x < bombs; x++) {
		do {
			n = rand();
			if(n <= RAND_MAX-((RAND_MAX+1)%(uint32)range))
				n = n%range;
		} while ((n >= range) || (fBoard[n].flags == F_BOMB));
		fBoard[n].flags = F_BOMB;
	}

	fTime = 0;
	DrawTime();
	fBombs = bombs;
	DrawBombs();
}

//--------------------------------------------------------------------

void TMinesView::Pause()
{
	int32	x, y;
	BRect	r;

	if (fGameState == G_PAUSED) {
		pause_menu->SetMarked(false);
		fGameState = G_STARTED;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if (fBoard[y * width + x].state != T_BLANK) {
					r.Set(x, y, x, y);
					DrawBoard(r);
				}
			}
		}
		fStartTime += (system_time() - fPauseTime);
		DrawBombs();
		DrawTime();
	}
	else if (fGameState == G_STARTED) {
		pause_menu->SetMarked(true);
		fGameState = G_PAUSED;
		fPauseTime = system_time();
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				if (fBoard[y * width + x].state != T_BLANK) {
					r.Set(x, y, x, y);
					DrawBoard(r);
				}
			}
		}
		DrawBombs();
		DrawTime();
	}
}

//--------------------------------------------------------------------

void TMinesView::ShowGroup(int32 old_x, int32 old_y,
						   int32 new_x, int32 new_y, int32* state)
{
	int32	x, y;
	int32	temp_state[9];
	BRect	r;
	BRect	r_old;
	BRect	r_new;
	BRect	temp;

	r.Set(0, 0, width - 1, height - 1);
	r_old.Set(old_x - 1, old_y - 1, old_x + 1, old_y + 1);
	r_new.Set(new_x - 1, new_y - 1, new_x + 1, new_y + 1);

	// restore non-intersecting areas
	for (y = old_y - 1; y <= old_y + 1; y++) {
		for (x = old_x - 1; x <= old_x + 1; x++) {
			if (r.Contains(BPoint(x, y))) {
				if (!r_new.Contains(BPoint(x, y))) {
					fBoard[y * width + x].state =
						state[(y - (old_y - 1)) * 3 + (x - (old_x - 1))];
					if (fBoard[y * width + x].state == T_BLANK) {
						temp.Set(x, y, x, y);
						DrawBoard(temp);
					}
				}
				else if ((new_x != -1) && (new_y != -1))
					temp_state[(y - (new_y - 1)) * 3 + (x - (new_x - 1))] =
						 state[(y - (old_y - 1)) * 3 + (x - (old_x - 1))];
			}
		}
	}

	// show new areas
	for (y = new_y - 1; y <= new_y + 1; y++) {
		for (x = new_x - 1; x <= new_x + 1; x++) {
			if ((r.Contains(BPoint(x, y))) && (!r_old.Contains(BPoint(x, y)))) {
				temp_state[(y - (new_y - 1)) * 3 + (x - (new_x - 1))] =
							fBoard[y * width + x].state;
				if (fBoard[y * width + x].state == T_BLANK) {
					fBoard[y * width + x].state = T0;
					temp.Set(x, y, x, y);
					DrawBoard(temp);
				}
			}
		}
	}

	for (x = 0; x < 9; x++)
		state[x] = temp_state[x];
}

//--------------------------------------------------------------------

void TMinesView::TrackFace(BRect r)
{
	bool		in = true;
	int32		old_state;
	uint32		buttons;
	BPoint		point;
	BRect		r1;

	old_state = fFaceState;
	fFaceState = F_DOWN;
	DrawFace();
	do {
		GetMouse(&point, &buttons);
		if ((r.Contains(point)) && (!in)) {
			fFaceState = F_DOWN;
			DrawFace();
			in = true;
		}
		else if ((!r.Contains(point)) && (in)) {
			fFaceState = old_state;
			DrawFace();
			in = false;
		}
		snooze(10000);
	} while (buttons);

	if (in) {
		if (fGameState != G_NEW) {
			pause_menu->SetEnabled(false);
			pause_menu->SetMarked(false);
		}
		NewGame();
	}
}

//--------------------------------------------------------------------

void TMinesView::TrackTile(int32 orig_x, int32 orig_y,
							BPoint where, uint32 orig_buttons)
{
	char		name[256];
	bool		in = true;
	bool		group = false;
	int32		old_state[9];
	int32		x, y;
	int32		last_x, last_y;
	int32		button = T0;
	int32		loop;
	int32		start = 4;
	int32		end = 4;
	int32		n;
	int32		range;
	uint32		buttons;
	BPoint		point;
	BRect		r;
	TFastWindow	*fast;
	
	old_state[4] = fBoard[orig_y * width + orig_x].state;
	if (orig_buttons == B_SECONDARY_MOUSE_BUTTON) {
		if (old_state[4] == T_FLAG) {
			button = T_GUESS;
			fBombs++;
			DrawBombs();
		}
		else if (old_state[4] == T_GUESS)
			button = T_BLANK;
		else if (old_state[4] == T_BLANK) {
			button = T_FLAG;
			fBombs--;
			DrawBombs();
		}
		if (button != T0) {
			fBoard[orig_y * width + orig_x].state = button;
			r.Set(orig_x, orig_y, orig_x, orig_y);
			DrawBoard(r);
		}
		return;
	}
	else if (orig_buttons > B_SECONDARY_MOUSE_BUTTON) {
		ShowGroup(-2, -2, orig_x, orig_y, &old_state[0]);
		group = true;
	}

	fFaceState = F_GUESS;
	DrawFace();

	if ((!group) && ((old_state[4] == T_BLANK) || (old_state[4] == T_GUESS))) {
		fBoard[orig_y * width + orig_x].state = T0;
		r.Set(orig_x, orig_y, orig_x, orig_y);
		DrawBoard(r);
	}
	last_x = orig_x;
	last_y = orig_y;
	point = where;

	do {
		x = (int)(point.x - (BORDER1 + BORDER2 + BORDER3)) / (TILE_WIDTH + 1);
		if ((point.x < (BORDER1 + BORDER2 + BORDER3)) || (x > (width - 1)))
			x = -2;
		y = (int)(point.y - (BORDER1 + BORDER2 + (2 * HEADER_BORDER) +
			HEADER_HEIGHT + BORDER2 + BORDER3)) / (TILE_HEIGHT + 1);
		if ((point.y < (BORDER1 + BORDER2 + (2 * HEADER_BORDER) + HEADER_HEIGHT +
			BORDER2 + BORDER3)) || (y > (height - 1)))
			y = -2;
		if ((x == -2) || (y == -2)) {
			if (in) {
				in = false;
				if (group)
					ShowGroup(last_x, last_y, x, y, &old_state[0]);
				else if (fBoard[last_y * width + last_x].state != old_state[4]) {
					fBoard[last_y * width + last_x].state = old_state[4];
					r.Set(last_x, last_y, last_x, last_y);
					DrawBoard(r);
				}
			}
		}
		else if ((x != last_x) || (y != last_y)) {
			if (group)
				ShowGroup(last_x, last_y, x, y, &old_state[0]);
			else if (in) {
				if (fBoard[last_y * width + last_x].state != old_state[4]) {
					fBoard[last_y * width + last_x].state = old_state[4];
					r.Set(last_x, last_y, last_x, last_y);
					DrawBoard(r);
				}
			}
			if (!group) {
				old_state[4] = fBoard[y * width + x].state;
				if ((old_state[4] == T_BLANK) || (old_state[4] == T_GUESS) ||
						((old_state[4] == T_FLAG) && (button == T_FLAG))) {
					fBoard[y * width + x].state = T0;
					r.Set(x, y, x, y);
					DrawBoard(r);
				}
			}
			in = true;
		}
		last_x = x;
		last_y = y;
		Pulse();
		snooze(10000);
		GetMouse(&point, &buttons);
		if ((buttons > B_SECONDARY_MOUSE_BUTTON) && (!group)) {
			if (in)
				fBoard[y * width + x].state = old_state[4];
			ShowGroup(-2, -2, x, y, &old_state[0]);
			group = true;
		}
	} while (buttons);

	if (in) {
		if (group) {
			n = 0;
			for (loop = 0; loop < 9; loop++)
				if (old_state[loop] == T_FLAG)
					n++;
			if ((old_state[4] < T1) || (old_state[4] > T8) ||
				(old_state[4] != n)) {
				ShowGroup(x, y, -2, -2, &old_state[0]);
				goto done;
			}
			x--;
			y--;
			start = 0;
			end = 8;
		}

		last_x = x;
		last_y = y;
		for (loop = start; loop <= end; loop++) {
			if ((last_x >= 0) && (last_x < width) &&
				(last_y >= 0) && (last_y < height)) {
				if (old_state[loop] == T_GUESS)
					old_state[loop] = T_BLANK;
				fBoard[last_y * width + last_x].state = old_state[loop];
			}
			if ((loop) && !((loop + 1) % 3)) {
				last_x -= 2;
				last_y++;
			}
			else
				last_x++;
		}

		last_x = x;
		last_y = y;
		for (loop = start; loop <= end; loop++) {
			x = last_x;
			y = last_y;
			if ((x >= 0) && (x < width) && (y >= 0) && (y < height)) {
				if ((old_state[loop] == T_BLANK) ||
					(old_state[loop] == T_GUESS)) {
					if (fGameState == G_NEW) {
						if (fBoard[y * width + x].flags == F_BOMB) {
							range = width * height;
							do {
								n = rand();
								if(n <= RAND_MAX-((RAND_MAX+1)%(uint32)range))
									n = n%range;
							} while ((n == (y * width + x)) || (n >= range) ||
									(fBoard[n].flags == F_BOMB));
							fBoard[n].flags = F_BOMB;
							fBoard[y * width + x].flags = F_FREE;
						}
					}
					if (fBoard[y * width + x].flags == F_BOMB) {
						fBoard[y * width + x].state = T_BOMB_DEAD;
						r.Set(x, y, x, y);
						DrawBoard(r);
						fGameState = G_FINISHED;
						fFaceState = F_LOST;
					}
					else if (fBoard[y * width + x].state == T_BLANK) {
						if (fGameState == G_NEW) {
							fGameState = G_STARTED;
							pause_menu->SetEnabled(true);
							fStartTime = system_time();
						}
						MoveIt(x, y);
						x = 0;
						do {
							if (((fBoard[x].flags == F_FREE) &&
								 (fBoard[x].state != T_BLANK) &&
								 (fBoard[x].state != T_FLAG) &&
								 (fBoard[x].state != T_GUESS)) ||
								 (fBoard[x].flags == F_BOMB))
								x++;
							else
								break;
						} while (x != width * height);
						if ((x == width * height) && (fGameState != G_FINISHED)) {
							fGameState = G_FINISHED;
							fFaceState = F_WON;
							DrawFace();
							pause_menu->SetEnabled(false);
							for (y = 0; y < height; y++) {
								for (x = 0; x < width; x++) {
									if ((fBoard[y * width + x].flags == F_BOMB) &&
										(fBoard[y * width + x].state != T_FLAG)) {
										fBoard[y * width + x].state = T_FLAG;
										r.Set(x, y, x, y);
										DrawBoard(r);
									}
								}
							}
							if (fBombs != 0) {
								fBombs = 0;
								DrawBombs();
							}
						}
					}
				}
			}
			if ((loop) && !((loop + 1) % 3)) {
				last_x -= 2;
				last_y++;
			}
			else
				last_x++;
		}

		if (fFaceState == F_LOST) {
			DrawFace();
			pause_menu->SetEnabled(false);
			for (y = 0; y < height; y++) {
				for (x = 0; x < width; x++) {
					if (fBoard[y * width + x].flags == F_BOMB) {
						if ((fBoard[y * width + x].state == T_BLANK) ||
							(fBoard[y * width + x].state == T_GUESS)) {
							fBoard[y * width + x].state = T_BOMB_OK;
							r.Set(x, y, x, y);
							DrawBoard(r);
						}
					}
					else if ((fBoard[y * width + x].state == T_FLAG) &&
							 (fBoard[y * width + x].flags != F_BOMB)) {
						fBoard[y * width + x].state = T_BOMB_WRONG;
						r.Set(x, y, x, y);
						DrawBoard(r);
					}
				}
			}
		}
		else if (fFaceState == F_WON) {
			n = 0;
			if ((width == 8) && (height == 8) && (bombs == 10) &&
											(fTime < GetTime(name, 1)))
				n = 1;
			else if ((width == 16) && (height == 16) && (bombs == 40) &&
											(fTime < GetTime(name, 2)))
				n = 2;
			else if ((width == 30) && (height == 16) && (bombs == 99) &&
											(fTime < GetTime(name, 3)))
				n = 3;
			if (n) {
				BScreen screen( Window() );
				BRect screen_frame = screen.Frame();

				r = Window()->Frame();
				r.left += (r.Width() - FAST_WIND_WIDTH) / 2;
				if (r.left < 10)
					r.left = 10;
				r.right = r.left + FAST_WIND_WIDTH;
				if (r.right > (screen_frame.right - 10)) {
					r.right = (screen_frame.right - 10);
					r.left = r.right - FAST_WIND_WIDTH;
				}
				r.top += (r.Height() - FAST_WIND_HEIGHT) / 2;
				if (r.top < 10)
					r.top = 10;
				r.bottom = r.top + FAST_WIND_HEIGHT;
				if (r.bottom > (screen_frame.bottom - 10)) {
					r.bottom = (screen_frame.bottom - 10);
					r.top = r.bottom - FAST_WIND_HEIGHT;
				}
				fast = new TFastWindow(r, "Fast Times", n, name, fTime);
				fast->Show();
				be_app->SetCursor(B_HAND_CURSOR);
			}
		}
	}

done:;
	if (fFaceState == F_GUESS) {
		fFaceState = F_UP;
		DrawFace();
	}
}

//--------------------------------------------------------------------

int32 GetTime(char* name, int32 type)
{
	int32		time = 999;
	BFile		*file;

	strcpy(name, FAST_DEFAULT_NAME);
	file = new BFile(settings_path.Path(), O_RDWR);
	if (file->InitCheck() == B_NO_ERROR) {
		switch (type) {
			case 1:
				file->ReadAttr("begin_time", B_INT16_TYPE, 0,
									  &time, sizeof(time));
				file->ReadAttr("begin_name", B_STRING_TYPE, 0,
									  name, 256);
				break;
			case 2:
				file->ReadAttr("intermediate_time", B_INT16_TYPE, 0,
									  &time, sizeof(time));
				file->ReadAttr("intermediate_name", B_STRING_TYPE, 0,
									  name, 256);
				break;
			case 3:
				file->ReadAttr("expert_time", B_INT16_TYPE, 0,
									  &time, sizeof(time));
				file->ReadAttr("expert_name", B_STRING_TYPE, 0,
									  name, 256);
				break;
		}
	}
	delete file;
	return time;
}
