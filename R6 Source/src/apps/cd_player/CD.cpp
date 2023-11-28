//--------------------------------------------------------------------
//	
//	CD.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <scsi.h>
#include <OS.h>
#include <fcntl.h>
#include <fs_info.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>

#include "CD.h"
#include "CDBits.h"
#include <Debug.h>
#include "SaveWindow.h"
#include <scsiprobe_driver.h>
#include <IDE.h>
#include <ide_device_info.h>
#include <Screen.h>

#include <fcntl.h>

#define kWINDOW_TITLE	"CDPlayer"
#define kCDDA_TYPE		"audio/x-cdda"
#define kCDDA_ATTRIBUTE	"Audio:Track"

sem_id			cd_sem = 0;
bool			changed = false;
int32			track;
bool			mute = false;
int32			format = RAW;
int32			time_mode = TRACK_RE;
bool			quitting = false;
int32			window_count = 0;

status_t make_file(BEntry*, BFile*);


//====================================================================

int main()
{	
	TCDApplication *app = new TCDApplication();
	app->Run();

	delete app;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TCDApplication::TCDApplication(void)
		  :BApplication("application/x-vnd.Be-CDP!"),
		   fPlayTrack(false)
{
	char			error[128] = "There is no CD-ROM drive connected or \
									  the drivers for the CD-ROM drive are \
									  not installed.";
	char			device[B_OS_NAME_LENGTH];
	int				i = 0;
	int				count = 0;
	int32			cd_index = 0;
	BAlert			*myAlert;
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	BMenu			*subMenu;
	BMenuItem		*item;
	BPath			path;
	BPoint			win_pos;
	BRect			r;
	BVolume			vol;
	BVolumeRoster	roster;

//
// Setup index
//
	roster.GetBootVolume(&vol);
	fs_create_index(vol.Device(), CD_KEY, B_INT32_TYPE, 0);

//
// Create app sub-menu with list of all available CD-ROM drives...
//
	subMenu = new BMenu("New Player");
	fDeviceMenu = new BMenu("Device");
	fDeviceMenu->SetRadioMode(true);

	TryDir("/dev/disk", subMenu, fDeviceMenu, &count);

// If there are no drives, alert the user and leave...
	if (!count) {
		myAlert = new BAlert("", error, "Continue", "Quit");
		i = myAlert->Go();
		if (i == 1) {
			be_app->PostMessage(B_QUIT_REQUESTED);
			delete fDeviceMenu;
			return;
		}
		cd_index = -1;
		item = new BMenuItem("None", new BMessage(MENU_OOPS));
		item->SetEnabled(false);
		fDeviceMenu->AddItem(item);
		item = new BMenuItem("None", new BMessage(MENU_OOPS));
		item->SetEnabled(false);
		subMenu->AddItem(item);
	}

	fMenu = new BPopUpMenu(kWINDOW_TITLE, false, false);
	fMenu->AddItem(item = new BMenuItem("About CDPlayer...", new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(item = new BMenuItem(subMenu));
	item->SetTarget(be_app);
	fMenu->AddItem(item = new BMenuItem(fDeviceMenu));
	item->SetTarget(be_app);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(item = new BMenuItem("Quit CDPlayer", new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);

//+	SetMainMenu(fMenu);

//
// Set default window position.  If a pref file exists, read the desired
// window position along with all the other prefs...
//
	r.Set(BROWSER_WIND, TITLE_BAR_HEIGHT,
		  BROWSER_WIND + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);

	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	dir.SetTo(path.Path());
	if (dir.FindEntry("CDPlayer_data", &entry) == B_NO_ERROR) {
		file.SetTo(&entry, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			file.Read(&win_pos, sizeof(BPoint));
			file.Read(&mute, sizeof(bool));
			file.Read(&format, sizeof(int32));
			file.Read(&time_mode, sizeof(int32));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(win_pos))
				r.OffsetTo(win_pos);
		}
	}

	cd_sem = create_sem(1, "cd_sem");
	fWindow = new TCDWindow(r, kWINDOW_TITLE, cd_index);
	fWindow->Show();
}

//--------------------------------------------------------------------

void TCDApplication::TryDir(const char *directory, BMenu *new_menu, BMenu *sub_menu, int *count)
{
	const char			*name;
	int					fd;
	BPath				path;
	BDirectory			dir;
	BEntry				entry;
	BMenuItem			*item;
	device_geometry		geometry;

	dir.SetTo(directory);
	if (dir.InitCheck() == B_NO_ERROR) {
		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			entry.GetPath(&path);
			name = path.Path();
			if (entry.IsDirectory())
				TryDir(name, new_menu, sub_menu, count);
			else if ((strstr(name, "/raw")) && ((fd = open(name, O_RDWR)) >= 0)) {
				if (ioctl(fd, B_GET_GEOMETRY, &geometry) == B_NO_ERROR) {
					if (geometry.device_type == B_CD) {
						*count += 1;
						sub_menu->AddItem(item = new BMenuItem(name, new BMessage(MENU_ID)));
						item->SetTarget(be_app);
						new_menu->AddItem(item = new BMenuItem(name, new BMessage(MENU_NEW)));
						item->SetTarget(be_app);
					}
				}
				close(fd);
			}
		}
	}
}


//--------------------------------------------------------------------

TCDApplication::~TCDApplication(void)
{
	if (cd_sem)
		delete_sem(cd_sem);
}

//--------------------------------------------------------------------

void TCDApplication::AboutRequested(void)
{
	char				about[32] = "...by Robert Polic";
	BAlert				*myAlert;

	myAlert = new BAlert("",about,"Big Deal");
	myAlert->Go();
}

//--------------------------------------------------------------------

void TCDApplication::ArgvReceived(int32 argc, char **argv)
{
	BEntry entry;

	if (entry.SetTo(argv[1]) == B_NO_ERROR) {
		BMessage msg(B_REFS_RECEIVED);
		entry_ref ref;

		entry.GetRef(&ref);
		msg.AddRef("refs", &ref);
		RefsReceived(&msg);
	}
}

//--------------------------------------------------------------------

void TCDApplication::MessageReceived(BMessage* msg)
{
	int32		index;
	BDirectory	dir;
	BEntry		entry;
	BFile		file;
	BPath		path;
	BPoint		win_pos;
	BRect		r;

	switch (msg->what) {
		case MENU_NEW:
			r.Set(BROWSER_WIND, TITLE_BAR_HEIGHT,
				  BROWSER_WIND + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);

			find_directory(B_USER_SETTINGS_DIRECTORY, &path);
			dir.SetTo(path.Path());
			if (dir.FindEntry("CDPlayer_data", &entry) == B_NO_ERROR) {
				file.SetTo(&entry, O_RDONLY);
				if (file.InitCheck() == B_NO_ERROR) {
					file.Read(&win_pos, sizeof(BPoint));
					win_pos.x += (10 * window_count);
					win_pos.y += (10 * window_count);
					if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(win_pos))
						r.OffsetTo(win_pos);
				}
			}
			index = msg->FindInt32("index");
			fWindow = new TCDWindow(r, kWINDOW_TITLE, index);
			fWindow->Show();
			if ((msg->HasInt32("track")) && (fWindow->Lock())) {
				int32	track;

				msg->FindInt32("track", &track);
				fWindow->fView->PlayTrack(track);
				fWindow->Unlock();
			}
			break;

		case MENU_ID:
			index = msg->FindInt32("index");
			if (fWindow) {
				fWindow->Lock();
				fWindow->fView->ChangeDriver(index);
				fWindow->Unlock();
			}
			break;

		case B_SIMPLE_DATA:
			RefsReceived(msg);
			break;

		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

//--------------------------------------------------------------------

bool TCDApplication::QuitRequested(void)
{
	quitting = true;
	return true;
}

//--------------------------------------------------------------------

void TCDApplication::RefsReceived(BMessage *msg)
{
	int32		item = 0;
	entry_ref	ref;

	if (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		BEntry	entry(&ref);
		BFile	file;

		file.SetTo(&ref, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			BNodeInfo	node(&file);
			char		type[B_FILE_NAME_LENGTH];

			node.GetType(type);
			if (!strcmp(type, kCDDA_TYPE)) {
				attr_info	info;

				if (file.GetAttrInfo(kCDDA_ATTRIBUTE, &info) == B_NO_ERROR) {
					int32		index = 0;
					int32		track;
					BMenuItem	*item;
					fs_info		fs;

					file.ReadAttr(kCDDA_ATTRIBUTE, B_INT32_TYPE, 0, &track, info.size);
					fs_stat_dev(ref.device, &fs);
					while ((item = fDeviceMenu->ItemAt(index)) != NULL) {
						if (!strcmp(item->Label(), fs.device_name)) {
							item->SetMarked(true);
							FindPlayer(index, track);
							return;
						}
						index++;
					}

					if (file.GetAttrInfo(CD_KEY, &info) == B_NO_ERROR) {
						uint32	key;

						file.ReadAttr(CD_KEY, B_INT32_TYPE, 0, (int32 *)&key, info.size);
						FindDevice(key, track);
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------

void TCDApplication::FindDevice(uint32 key, int32 track)
{
	char			str[256];
	int32			fd;
	int32			index = 0;
	BEntry			entry;
	BFile			file;
	BMenuItem		*item;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;
	attr_info		info;

	while ((item = fDeviceMenu->ItemAt(index)) != NULL) {
		if ((fd = open(item->Label(), O_RDWR)) >= 0) {
			scsi_toc	TOC;

			if (ioctl(fd, B_SCSI_GET_TOC, &TOC) == B_NO_ERROR) {
				char		byte;
				uint32		new_key;

				byte = TOC.toc_data[4 + ((TOC.toc_data[3] - TOC.toc_data[2] + 1) * 8) + 5];
				new_key = 0;
				new_key = (byte / 10) << 20;
				new_key |= (byte % 10) << 16;
				byte = TOC.toc_data[4 + ((TOC.toc_data[3] - TOC.toc_data[2] + 1) * 8) + 6];
				new_key |= (byte / 10) << 12;
				new_key |= (byte % 10) << 8;
				byte = TOC.toc_data[4 + ((TOC.toc_data[3] - TOC.toc_data[2] + 1) * 8) + 7];
				new_key |= (byte / 10) << 4;
				new_key |= byte % 10;
				if (key == new_key) {
					FindPlayer(index, track);
					close(fd);
					return;
				}
			}
			close(fd);
		}
		index++;
	}

	sprintf(str, "%s=%d", CD_KEY, key);
	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate(str);
	query.Fetch();

	if (query.GetNextEntry(&entry) == B_NO_ERROR) {
		char	*error;
		char	*list;
		int32	len = 0;

		file.SetTo(&entry, O_RDWR);
		if (file.GetAttrInfo("CD:tracks", &info) == B_NO_ERROR) {
			list = (char *)malloc(info.size);
			file.ReadAttr("CD:tracks", B_STRING_TYPE, 0, list, info.size);
			while (list[len] != '\n') {
				len++;
			}
			list[len] = 0;
			error = (char *)malloc(len + 256);
			sprintf(error, "The audio CD \"%s\" could not be located.  Insert the CD and try again.", list);
			(new BAlert("", error, "OK"))->Go();
			free(list);
			free(error);
		}
		else
			(new BAlert("", "The audio CD for this track could not be located.  Insert the correct CD and try again.", "OK"))->Go();
	}
}

//--------------------------------------------------------------------

void TCDApplication::FindPlayer(int32 index, int32 track)
{
	int32		count = CountWindows();
	int32		loop;
	BMessage	msg(MENU_NEW);
	TCDWindow	*window;

	for (loop = 0; loop < count; loop++) {
		if (((window = dynamic_cast<TCDWindow *>(WindowAt(loop))) != NULL) &&
			(window->Lock())) {
			if (window->fView->fCDIndex == index) {
				window->fView->PlayTrack(track);
				window->Activate(true);
				window->Unlock();
				return;
			}
			window->Unlock();
		}
	}

	msg.AddInt32("index", index);
	msg.AddInt32("track", track);
	be_app->PostMessage(&msg);
}


//====================================================================

TCDWindow::TCDWindow(BRect rect, char *title, int32 index)
	          :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect			tempRect;
	TSliderView*	slider_view;

	SetPulseRate(500000);
	tempRect.Set(0, 0, WIND_WIDTH, WIND_HEIGHT);
	fView = new TCDView(tempRect, "CDView", index);

	tempRect.Set(SLIDER_BACK_LEFT,
				 SLIDER_BACK_TOP,
				 SLIDER_BACK_LEFT + SLIDER_BACK_WIDTH,
				 SLIDER_BACK_TOP + SLIDER_BACK_HEIGHT);
	slider_view = new TSliderView(tempRect, "SliderView", fView);
	AddChild(slider_view);

	AddChild(fView);
	window_count++;
}

//--------------------------------------------------------------------

TCDWindow::~TCDWindow(void)
{
	TCDApplication *app = dynamic_cast<TCDApplication *>(be_app);
	window_count--;
	if (app)
		app->fWindow = NULL;
}

//--------------------------------------------------------------------

void TCDWindow::MessageReceived(BMessage* theMessage)
{
	const char	*text;
	int32		length;
	int32		loop;
	track_info	*track_data;

	switch(theMessage->what) {
		case KILL_EDIT_SAVE:
		case KILL_EDIT_NOSAVE:
		case KILL_TITLE_SAVE:
			if (fView->fTextView) {
				fView->RemoveChild(fView->fTextView);
				if (theMessage->what != KILL_EDIT_NOSAVE) {
					track_data = (track_info*)(fView->fTitleList->ItemAt(0));
					if (track_data) {
						text = fView->fTextView->Text();
						length = fView->fTextView->TextLength();
						for (loop = 0; loop < length; loop++)
							track_data->title[loop] = *text++;
						track_data->title[loop] = 0;
						track_data->flags |= DIRTY;
						fView->FindRecord(fView->fKey, true);
						SetTitle(track_data->title);
					}
				}
				delete fView->fTextView;
				fView->fTextView = NULL;
			}
			else if ((fView->fTrackView) && (fView->fTrackView->fTextView)) {
				if (theMessage->what == KILL_EDIT_SAVE)
					fView->fTrackView->NextField(false);
				else if (theMessage->what == KILL_EDIT_NOSAVE)
					fView->fTrackView->KillEditField(false);
			}
			break;

		case NEXT_FIELD:
			if ((fView->fTrackView) && (fView->fTrackView->fTextView))
				fView->fTrackView->NextField(false);
			break;

		case PREV_FIELD:
			if ((fView->fTrackView) && (fView->fTrackView->fTextView))
				fView->fTrackView->NextField(true);
			break;

		case SAVE_DATA:
			fView->FindRecord(fView->fKey, true);
			break;

		case B_SIMPLE_DATA:
			be_app->PostMessage(theMessage);
			break;

		default:
			BWindow::MessageReceived(theMessage);
			break;
	}
}

//--------------------------------------------------------------------

bool TCDWindow::QuitRequested(void)
{
	BDirectory	dir;
	BEntry		entry;
	BFile		file;
	BPath		path;
	BPoint		win_pos;
	BRect		r;
	status_t	result;

	if (window_count == 1) {
		r = Frame();
		win_pos = r.LeftTop();

		find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
		dir.SetTo(path.Path());
		if (dir.FindEntry("CDPlayer_data", &entry) == B_NO_ERROR) {
			file.SetTo(&entry, O_RDWR);
			result = file.InitCheck();
		}
		else
			result = dir.CreateFile("CDPlayer_data", &file);
		if (result == B_NO_ERROR) {
			file.Write(&win_pos, sizeof(BPoint));
			file.Write(&mute, sizeof(bool));
			file.Write(&format, sizeof(int32));
			file.Write(&time_mode, sizeof(int32));
		}
	}

	if (fView->fSaveWindow) {
		if (fView->fSaveWindow->Lock())
			fView->fSaveWindow->Quit();
		fView->fSaveWindow = NULL;
	}
	if (window_count == 1)
		be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}

//--------------------------------------------------------------------

void TCDWindow::WindowActivated(bool active)
{
	BMenu		*sub_menu;
	BMenuItem	*item;

	if (active) {
		TCDApplication *app = dynamic_cast<TCDApplication *>(be_app);
		if (app) {
			app->fWindow = this;
			sub_menu = app->fMenu->SubmenuAt(3);
			if (sub_menu) {
				item = sub_menu->ItemAt(fView->fCDIndex);
				if (item)
					item->SetMarked(true);
			}
		}
	}
}

//====================================================================

TCDView::TCDView(BRect rect, char *title, int32 index)
	   :BView(rect, title, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED)
{
	fCDID = 0;
	fCDIndex = index;
}


//--------------------------------------------------------------------

TCDView::~TCDView(void)
{
	int32		i;
	track_info	*track_data;

	for (i = 0; i < 3; i++)
		delete fListOpenButton[i];
	for (i = 0; i < 2; i++)
		delete fListCloseButton[i];
	for (i = 0; i < 3; i++)
		delete fStopButton[i];
	for (i = 0; i < 3; i++)
		delete fPlayButton[i];
	for (i = 0; i < 2; i++)
		delete fPauseButton[i];
	for (i = 0; i < 3; i++)
		delete fPrevButton[i];
	for (i = 0; i < 3; i++)
		delete fNextButton[i];
	for (i = 0; i < 3; i++)
		delete fBackButton[i];
	for (i = 0; i < 3; i++)
		delete fForButton[i];
	for (i = 0; i < 3; i++)
		delete fEjectButton[i];
	for (i = 0; i < 3; i++)
		delete fSaveButton[i];
	for (i = 0; i < 3; i++)
		delete fModeButton[i];
	for (i = 0; i < 3; i++)
		delete fShuffleButton[i];
	for (i = 0; i < 3; i++)
		delete fRepeatButton[i];
	for (i = 0; i < 3; i++)
		delete fNormalButton[i];
	for (i = 0; i < 3; i++)
		delete fClockButton[i];
	delete fLogo;
	delete fSound;
	for (i = 0; i < 10; i++)
		delete fLEDs[i];
	delete fOffScreen;
	while (track_data = (track_info*)fTitleList->ItemAt(0)) {
		fTitleList->RemoveItem(track_data);
		free(track_data);
	}
	delete fTitleList;
}

//--------------------------------------------------------------------

void TCDView::AttachedToWindow(void)
{
	unsigned char*	bits;
	BRect			tempRect;
	rgb_color		c;

	fReady = false;
	tempRect.Set(0, 0, ((BUTTON_WIDTH + 7) & 0xfff8) - 1, BUTTON_HEIGHT);
	bits = list_open_button;
	fListOpenButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fListOpenButton[NORMAL_M]->SetBits((char*)bits, fListOpenButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = list_open_select_button;
	fListOpenButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fListOpenButton[SELECT_M]->SetBits((char*)bits, fListOpenButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = list_open_disable_button;
	fListOpenButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fListOpenButton[DISABLE_M]->SetBits((char*)bits, fListOpenButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = list_close_button;
	fListCloseButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fListCloseButton[NORMAL_M]->SetBits((char*)bits, fListCloseButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = list_close_select_button;
	fListCloseButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fListCloseButton[SELECT_M]->SetBits((char*)bits, fListCloseButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = stop_button;
	fStopButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fStopButton[NORMAL_M]->SetBits((char*)bits, fStopButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = stop_select_button;
	fStopButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fStopButton[SELECT_M]->SetBits((char*)bits, fStopButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = stop_disable_button;
	fStopButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fStopButton[DISABLE_M]->SetBits((char*)bits, fStopButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = play_button;
	fPlayButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPlayButton[NORMAL_M]->SetBits((char*)bits, fPlayButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = play_select_button;
	fPlayButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPlayButton[SELECT_M]->SetBits((char*)bits, fPlayButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = play_disable_button;
	fPlayButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPlayButton[DISABLE_M]->SetBits((char*)bits, fPlayButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = pause_button;
	fPauseButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPauseButton[NORMAL_M]->SetBits((char*)bits, fPauseButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = pause_select_button;
	fPauseButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPauseButton[SELECT_M]->SetBits((char*)bits, fPauseButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = prev_button;
	fPrevButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPrevButton[NORMAL_M]->SetBits((char*)bits, fPrevButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = prev_select_button;
	fPrevButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPrevButton[SELECT_M]->SetBits((char*)bits, fPrevButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = prev_disable_button;
	fPrevButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fPrevButton[DISABLE_M]->SetBits((char*)bits, fPrevButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = next_button;
	fNextButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fNextButton[NORMAL_M]->SetBits((char*)bits, fNextButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = next_select_button;
	fNextButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fNextButton[SELECT_M]->SetBits((char*)bits, fNextButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = next_disable_button;
	fNextButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fNextButton[DISABLE_M]->SetBits((char*)bits, fNextButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = back_button;
	fBackButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fBackButton[NORMAL_M]->SetBits((char*)bits, fBackButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = back_select_button;
	fBackButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fBackButton[SELECT_M]->SetBits((char*)bits, fBackButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = back_disable_button;
	fBackButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fBackButton[DISABLE_M]->SetBits((char*)bits, fBackButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = for_button;
	fForButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fForButton[NORMAL_M]->SetBits((char*)bits, fForButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = for_select_button;
	fForButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fForButton[SELECT_M]->SetBits((char*)bits, fForButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = for_disable_button;
	fForButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fForButton[DISABLE_M]->SetBits((char*)bits, fForButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = eject_button;
	fEjectButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fEjectButton[NORMAL_M]->SetBits((char*)bits, fEjectButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = eject_select_button;
	fEjectButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fEjectButton[SELECT_M]->SetBits((char*)bits, fEjectButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = eject_disable_button;
	fEjectButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fEjectButton[DISABLE_M]->SetBits((char*)bits, fEjectButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = save_button;
	fSaveButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fSaveButton[NORMAL_M]->SetBits((char*)bits, fSaveButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = save_select_button;
	fSaveButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fSaveButton[SELECT_M]->SetBits((char*)bits, fSaveButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = save_disable_button;
	fSaveButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fSaveButton[DISABLE_M]->SetBits((char*)bits, fSaveButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = mode_button;
	fModeButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fModeButton[NORMAL_M]->SetBits((char*)bits, fModeButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = mode_select_button;
	fModeButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fModeButton[SELECT_M]->SetBits((char*)bits, fModeButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = mode_disable_button;
	fModeButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fModeButton[DISABLE_M]->SetBits((char*)bits, fModeButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = shuffle_button;
	fShuffleButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fShuffleButton[NORMAL_M]->SetBits((char*)bits, fShuffleButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = shuffle_select_button;
	fShuffleButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fShuffleButton[SELECT_M]->SetBits((char*)bits, fShuffleButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = shuffle_disable_button;
	fShuffleButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fShuffleButton[DISABLE_M]->SetBits((char*)bits, fShuffleButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = repeat_button;
	fRepeatButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fRepeatButton[NORMAL_M]->SetBits((char*)bits, fRepeatButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = repeat_select_button;
	fRepeatButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fRepeatButton[SELECT_M]->SetBits((char*)bits, fRepeatButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = repeat_disable_button;
	fRepeatButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fRepeatButton[DISABLE_M]->SetBits((char*)bits, fRepeatButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = normal_button;
	fNormalButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fNormalButton[NORMAL_M]->SetBits((char*)bits, fNormalButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = normal_select_button;
	fNormalButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fNormalButton[SELECT_M]->SetBits((char*)bits, fNormalButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = normal_disable_button;
	fNormalButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fNormalButton[DISABLE_M]->SetBits((char*)bits, fNormalButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	tempRect.Set(0, 0, CLOCK_WIDTH, CLOCK_HEIGHT);
	bits = clock_button;
	fClockButton[NORMAL_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fClockButton[NORMAL_M]->SetBits((char*)bits, fClockButton[NORMAL_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = clock_select_button;
	fClockButton[SELECT_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fClockButton[SELECT_M]->SetBits((char*)bits, fClockButton[SELECT_M]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = clock_disable_button;
	fClockButton[DISABLE_M] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fClockButton[DISABLE_M]->SetBits((char*)bits, fClockButton[DISABLE_M]->BitsLength(), 0, B_COLOR_8_BIT);

	tempRect.Set(0, 0, LOGO_WIDTH, LOGO_HEIGHT);
	bits = logo;
	fLogo = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLogo->SetBits((char*)bits, fLogo->BitsLength(), 0, B_COLOR_8_BIT);

	tempRect.Set(0, 0, SOUND_WIDTH, SOUND_HEIGHT);
	bits = sound;
	fSound = new BBitmap(tempRect, B_COLOR_8_BIT);
	fSound->SetBits((char*)bits, fSound->BitsLength(), 0, B_COLOR_8_BIT);

	tempRect.Set(0, 0, LED_WIDTH, LED_HEIGHT);
	bits = led_0;
	fLEDs[0] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[0]->SetBits((char*)bits, fLEDs[0]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_1;
	fLEDs[1] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[1]->SetBits((char*)bits, fLEDs[1]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_2;
	fLEDs[2] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[2]->SetBits((char*)bits, fLEDs[2]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_3;
	fLEDs[3] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[3]->SetBits((char*)bits, fLEDs[3]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_4;
	fLEDs[4] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[4]->SetBits((char*)bits, fLEDs[4]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_5;
	fLEDs[5] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[5]->SetBits((char*)bits, fLEDs[5]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_6;
	fLEDs[6] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[6]->SetBits((char*)bits, fLEDs[6]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_7;
	fLEDs[7] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[7]->SetBits((char*)bits, fLEDs[7]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_8;
	fLEDs[8] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[8]->SetBits((char*)bits, fLEDs[8]->BitsLength(), 0, B_COLOR_8_BIT);

	bits = led_9;
	fLEDs[9] = new BBitmap(tempRect, B_COLOR_8_BIT);
	fLEDs[9]->SetBits((char*)bits, fLEDs[9]->BitsLength(), 0, B_COLOR_8_BIT);

	tempRect.Set(0, 0, TIME_GUAGE_R - TIME_GUAGE_L, TIME_GUAGE_B - (TIME_T + 4));
	fOffScreen = new BBitmap(tempRect, B_COLOR_8_BIT, true);
	fOffScreen->AddChild(fOffView = new BView(tempRect, "", B_FOLLOW_ALL, B_WILL_DRAW));

	fSaveWindow = NULL;
	fTextView = NULL;
	fTrackView = NULL;
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetFont(be_plain_font);
	SetFontSize(9);
	fTrack = 1;
	fState = DISABLED_S;
	fPlayMode = MODE_B;
	fRepeatMode = NORMAL_B;
	fTimeMode = time_mode;
	fMin = 0;
	fSec = 0;
	fPercent = 0.0;
	fTitleList = new BList();
	Pulse();
	fReady = true;
}

//--------------------------------------------------------------------

void TCDView::Draw(BRect where)
{
	int32		i = 0;
	BRect		r;

	// Window shading
	r = Frame();
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	SetHighColor(184, 184, 184);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));

	// Title box
	if (0) { //fTextView) {
		r.Set(TITLE_L + 6, TITLE_T + 8, TITLE_R - 4, TITLE_B - 4);
		if (r.Intersects(where)) {
			r.Set(where.left - (TITLE_L + 6),
				  where.top - (TITLE_T + 6),
				  where.right - (TITLE_R - 4),
				  where.bottom - (TITLE_B - 4));
			fTextView->FillRect(r, B_SOLID_LOW);
			fTextView->Draw(r);
		}
	}
	r.Set(TITLE_L, TITLE_T, TITLE_R, TITLE_B);
	if (r.Intersects(where)) {
		SetHighColor(128, 128, 128);
		StrokeLine(BPoint(TITLE_L, TITLE_T), BPoint(TITLE_L, TITLE_B));
		StrokeLine(BPoint(TITLE_L + 1, TITLE_T), BPoint(TITLE_L + 1, TITLE_B - 1));
		StrokeLine(BPoint(TITLE_L + 2, TITLE_T), BPoint(TITLE_L + 2, TITLE_B - 2));
		StrokeLine(BPoint(TITLE_L + 3, TITLE_T), BPoint(TITLE_R, TITLE_T));
		StrokeLine(BPoint(TITLE_L + 3, TITLE_T + 1), BPoint(TITLE_R - 1, TITLE_T + 1));
		StrokeLine(BPoint(TITLE_L + 3, TITLE_T + 2), BPoint(TITLE_R - 2, TITLE_T + 2));
		StrokeLine(BPoint(TITLE_L + 3, TITLE_B - 3), BPoint(TITLE_R - 3, TITLE_B - 3));
		StrokeLine(BPoint(TITLE_R - 3, TITLE_T + 3), BPoint(TITLE_R - 3, TITLE_B - 4));
		SetHighColor(255, 255, 255);
		StrokeLine(BPoint(TITLE_L + 1, TITLE_B), BPoint(TITLE_R, TITLE_B));
		StrokeLine(BPoint(TITLE_L + 2, TITLE_B - 1), BPoint(TITLE_R, TITLE_B - 1));
		StrokeLine(BPoint(TITLE_L + 3, TITLE_B - 2), BPoint(TITLE_R, TITLE_B - 2));
		StrokeLine(BPoint(TITLE_R, TITLE_T + 1), BPoint(TITLE_R, TITLE_B - 3));
		StrokeLine(BPoint(TITLE_R - 1, TITLE_T + 2), BPoint(TITLE_R - 1, TITLE_B - 3));
		StrokeLine(BPoint(TITLE_R - 2, TITLE_T + 3), BPoint(TITLE_R - 2, TITLE_B - 3));
		SetHighColor(192, 192, 192);
		StrokeLine(BPoint(TITLE_L + 3, TITLE_T + 3), BPoint(TITLE_L + 3, TITLE_B - 4));
		StrokeLine(BPoint(TITLE_L + 4, TITLE_T + 3), BPoint(TITLE_L + 4, TITLE_B - 4));
		StrokeLine(BPoint(TITLE_L + 5, TITLE_T + 3), BPoint(TITLE_L + 5, TITLE_B - 4));
		StrokeLine(BPoint(TITLE_L + 6, TITLE_T + 3), BPoint(TITLE_R - 4, TITLE_T + 3));
		StrokeLine(BPoint(TITLE_L + 6, TITLE_T + 4), BPoint(TITLE_R - 4, TITLE_T + 4));
		StrokeLine(BPoint(TITLE_L + 6, TITLE_T + 5), BPoint(TITLE_R - 4, TITLE_T + 5));
		SetHighColor(208, 208, 208);
		StrokeLine(BPoint(TITLE_L + 6, TITLE_T + 6), BPoint(TITLE_L + 6, TITLE_B - 4));
		StrokeLine(BPoint(TITLE_L + 7, TITLE_T + 6), BPoint(TITLE_R - 4, TITLE_T + 6));
		DrawTitle(false);
	}

	// Track box
	r.Set(TRACK_L, TRACK_T, TRACK_R, TRACK_B);
	if (r.Intersects(where)) {
		SetHighColor(128, 128, 128);
		StrokeLine(BPoint(TRACK_L, TRACK_T), BPoint(TRACK_R, TRACK_T));
		StrokeLine(BPoint(TRACK_L, TRACK_T + 1), BPoint(TRACK_R - 1, TRACK_T + 1));
		StrokeLine(BPoint(TRACK_L, TRACK_T + 2), BPoint(TRACK_R - 2, TRACK_T + 2));
		StrokeLine(BPoint(TRACK_L, TRACK_T + 3), BPoint(TRACK_L, TRACK_B));
		StrokeLine(BPoint(TRACK_L + 1, TRACK_T + 3), BPoint(TRACK_L + 1, TRACK_B - 1));
		StrokeLine(BPoint(TRACK_L + 2, TRACK_T + 3), BPoint(TRACK_L + 2, TRACK_B - 2));
		SetHighColor(255, 255, 255);
		StrokeLine(BPoint(TRACK_L + 1, TRACK_B), BPoint(TRACK_R, TRACK_B));
		StrokeLine(BPoint(TRACK_L + 2, TRACK_B - 1), BPoint(TRACK_R, TRACK_B - 1));
		StrokeLine(BPoint(TRACK_L + 3, TRACK_B - 2), BPoint(TRACK_R, TRACK_B - 2));
		StrokeLine(BPoint(TRACK_R, TRACK_T + 1), BPoint(TRACK_R, TRACK_B - 3));
		StrokeLine(BPoint(TRACK_R - 1, TRACK_T + 2), BPoint(TRACK_R - 1, TRACK_B - 3));
		StrokeLine(BPoint(TRACK_R - 2, TRACK_T + 3), BPoint(TRACK_R - 2, TRACK_B - 3));
		SetHighColor(0, 0, 0);
		r.Set(TRACK_L + 3, TRACK_T + 3, TRACK_R - 3, TRACK_B - 3);
		FillRect(r);
	}

	// Track
	MovePenTo(TRACK_TEXT_LEFT, TRACK_TEXT_TOP);
	SetDrawingMode(B_OP_OVER);
	SetHighColor(0, 203, 0);
	DrawString("track");
	SetDrawingMode(B_OP_COPY);
	DrawTrack(fTrack);

	// Time box
	r.Set(TIME_L, TIME_T, TIME_R, TIME_B);
	if (r.Intersects(where)) {
		SetHighColor(128, 128, 128);
		StrokeLine(BPoint(TIME_L, TIME_T), BPoint(TIME_R, TIME_T));
		StrokeLine(BPoint(TIME_L, TIME_T + 1), BPoint(TIME_R - 1, TIME_T + 1));
		StrokeLine(BPoint(TIME_L, TIME_T + 2), BPoint(TIME_R - 2, TIME_T + 2));
		StrokeLine(BPoint(TIME_L, TIME_T + 3), BPoint(TIME_L, TIME_B));
		StrokeLine(BPoint(TIME_L + 1, TIME_T + 3), BPoint(TIME_L + 1, TIME_B - 1));
		StrokeLine(BPoint(TIME_L + 2, TIME_T + 3), BPoint(TIME_L + 2, TIME_B - 2));
		SetHighColor(255, 255, 255);
		StrokeLine(BPoint(TIME_L + 1, TIME_B), BPoint(TIME_R, TIME_B));
		StrokeLine(BPoint(TIME_L + 2, TIME_B - 1), BPoint(TIME_R, TIME_B - 1));
		StrokeLine(BPoint(TIME_L + 3, TIME_B - 2), BPoint(TIME_R, TIME_B - 2));
		StrokeLine(BPoint(TIME_R, TIME_T + 1), BPoint(TIME_R, TIME_B - 3));
		StrokeLine(BPoint(TIME_R - 1, TIME_T + 2), BPoint(TIME_R - 1, TIME_B - 3));
		StrokeLine(BPoint(TIME_R - 2, TIME_T + 3), BPoint(TIME_R - 2, TIME_B - 3));
		SetHighColor(0, 0, 0);
		r.Set(TIME_L + 3, TIME_T + 3, TIME_R - 3, TIME_B - 3);
		FillRect(r);
	}

	// Time
	SetTime(fTimeMode);
	DrawTime();

	// Item list box
	r.Set(ITEM_L, ITEM_T, ITEM_R, ITEM_B);
	if (r.Intersects(where)) {
		SetHighColor(128, 128, 128);
		StrokeLine(BPoint(ITEM_L, ITEM_T), BPoint(ITEM_R, ITEM_T));
		StrokeLine(BPoint(ITEM_L, ITEM_T + 1), BPoint(ITEM_L, ITEM_B));
		SetHighColor(255, 255, 255);
		StrokeLine(BPoint(ITEM_L + 1, ITEM_B), BPoint(ITEM_R, ITEM_B));
		StrokeLine(BPoint(ITEM_R, ITEM_T + 1), BPoint(ITEM_R, ITEM_B - 1));
		SetHighColor(208, 208, 208);
		r.Set(ITEM_L + 1, ITEM_T + 1, ITEM_R - 1, ITEM_B - 1);
		FillRect(r);
		if (fCDID) {
			while ((fTOC.toc_data[4 + (i * 8) + 2] != 0xaa) && (i < 17)) {
				r.Set((i * 13) + ITEM_L + 1, ITEM_T + 1,
					  (i * 13) + 12 + ITEM_L + 1, ITEM_B - 1);
				if (r.Intersects(where)) {
					if (fTrack == fTOC.toc_data[4 + (i * 8) + 2])
						DrawListItems(fTrack, 0);
					else
						DrawListItems(0, fTOC.toc_data[4 + (i * 8) + 2]);
				}
				i++;
			}
		}
	}

	SetHighColor(0, 0, 0);

	r.Set(LOGO_H, LOGO_V, LOGO_H + LOGO_WIDTH, LOGO_V + LOGO_HEIGHT);
	if (r.Intersects(where))
		DrawBitmap(fLogo, BPoint(LOGO_H, LOGO_V));
	r.Set(SOUND_H, SOUND_V, SOUND_H + SOUND_WIDTH, SOUND_V + SOUND_HEIGHT);
	if (r.Intersects(where))
		DrawBitmap(fSound, BPoint(SOUND_H, SOUND_V));

	DrawButtons(where);
}

//--------------------------------------------------------------------

void TCDView::MouseDown(BPoint thePoint)
{
	bool				hit = true;
	bool				doit = false;
	int32				item = 0;
	int32				new_track;
	int32				old_track;
	int32				last_pos;
	int32				current_pos;
	int32				inc;
	uint32				buttons;
	BBitmap*			button_on;
	BBitmap*			button_off;
	BPoint				where;
	BRect				dr;
	BRect				sr;
	BRect				r;
	TSaveWindow			*wind;
	scsi_position		pos;
	scsi_play_position	audio;
	scsi_scan			scan;
	track_info			*track_data;

	if (Window()->CurrentMessage()->FindInt32("buttons") > 1) {
		where = thePoint;
		ConvertToScreen(&where);
		((TCDApplication*)be_app)->fMenu->Go(where, true);
		return;
	}

	r.Set(TITLE_L + 7, TITLE_T + 9, TITLE_R - 8, TITLE_B - 4);
	if ((r.Contains(thePoint)) && (fState != DISABLED_S)) {
		if (fTrackView)
			fTrackView->KillEditField(true);
		r.top -= 3;
		dr.Set(0, 0, r.Width(), r.Height());
		fTextView = new TEditTextView(r, dr, true);
		AddChild(fTextView);
		track_data = (track_info*)(fTitleList->ItemAt(0));
		if (track_data)
			fTextView->SetText(track_data->title);
		fTextView->SelectAll();
		fTextView->SetMaxBytes(sizeof(track_data->title) - 1);
		return;
	}

	if (fState == DISABLED_S) {
		dr.Set(EJECT_H, BUTTON_V, EJECT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			sr.Set(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
			item = EJECT_B;
			button_on = fEjectButton[SELECT_M];
			button_off = fEjectButton[NORMAL_M];
			goto track;
		}
	}
	else {
		dr.Set(CLOCK_H, CLOCK_V, CLOCK_H + CLOCK_WIDTH, CLOCK_V + CLOCK_HEIGHT);
		if (dr.Contains(thePoint)) {
			sr.Set(0, 0, CLOCK_WIDTH, CLOCK_HEIGHT);
			item = CLOCK_B;
			button_on = fClockButton[SELECT_M];
			button_off = fClockButton[NORMAL_M];
			goto track;
		}
		sr.Set(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
		dr.Set(STOP_H, BUTTON_V, STOP_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = STOP_B;
			button_on = fStopButton[SELECT_M];
			button_off = fStopButton[NORMAL_M];
			goto track;
		}
		dr.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = PLAY_B;
			if ((fState == PAUSED_S) || (fState == STOPPED_S)) {
				button_on = fPlayButton[SELECT_M];
				button_off = fPlayButton[NORMAL_M];
			}
			else {
				button_on = fPauseButton[SELECT_M];
				button_off = fPauseButton[NORMAL_M];
			}
			goto track;
		}
		dr.Set(PREV_H, BUTTON_V, PREV_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = PREV_B;
			button_on = fPrevButton[SELECT_M];
			button_off = fPrevButton[NORMAL_M];
			goto track;
		}
		dr.Set(NEXT_H, BUTTON_V, NEXT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			if (fTrack == fMaxTrack)
				return;
			item = NEXT_B;
			button_on = fNextButton[SELECT_M];
			button_off = fNextButton[NORMAL_M];
			goto track;
		}
		dr.Set(BACK_H, BUTTON_V, BACK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = BACK_B;
			button_on = fBackButton[SELECT_M];
			button_off = fBackButton[NORMAL_M];
			goto track;
		}
		dr.Set(FOR_H, BUTTON_V, FOR_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = FOR_B;
			button_on = fForButton[SELECT_M];
			button_off = fForButton[NORMAL_M];
			goto track;
		}
		dr.Set(EJECT_H, BUTTON_V, EJECT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = EJECT_B;
			button_on = fEjectButton[SELECT_M];
			button_off = fEjectButton[NORMAL_M];
			goto track;
		}
		dr.Set(SAVE_H, BUTTON_V, SAVE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = SAVE_B;
			button_on = fSaveButton[SELECT_M];
			button_off = fSaveButton[NORMAL_M];
			goto track;
		}
		dr.Set(MODE_H, BUTTON_V, MODE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = MODE_B;
			if (fPlayMode == MODE_B) {
				button_on = fModeButton[SELECT_M];
				button_off = fModeButton[NORMAL_M];
			}
			else {
				button_on = fShuffleButton[SELECT_M];
				button_off = fShuffleButton[NORMAL_M];
			}
			goto track;
		}
		dr.Set(NORMAL_H, BUTTON_V, NORMAL_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = NORMAL_B;
			if (fRepeatMode == NORMAL_B) {
				button_on = fNormalButton[SELECT_M];
				button_off = fNormalButton[NORMAL_M];
			}
			else {
				button_on = fRepeatButton[SELECT_M];
				button_off = fRepeatButton[NORMAL_M];
			}
			goto track;
		}
		dr.Set(BLANK_H, BUTTON_V, BLANK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		if (dr.Contains(thePoint)) {
			item = BLANK_B;
			if (fTrackView) {
				button_on = fListCloseButton[SELECT_M];
				button_off = fListCloseButton[NORMAL_M];
			}
			else {
				button_on = fListOpenButton[SELECT_M];
				button_off = fListOpenButton[NORMAL_M];
			}
			goto track;
		}
		dr.Set(ITEM_L + 1, ITEM_T + 1, ITEM_L + (13 * min_c(17, (int)fTotalTracks)), ITEM_B - 1);
		if (dr.Contains(thePoint)) {
			item = LISTITEM_B;
			goto track;
		}

track:;
		if (item) {
			if (item != LISTITEM_B) {
				DrawBitmap(button_on, sr, dr);
				if ((item == BACK_B) || (item == FOR_B)) {
					if (fState == STOPPED_S) {
						fState = PLAYING_S;
						PlayTrack(fTrack);
					}
					else if (fState == PAUSED_S) {
						r.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
						DrawBitmap(fPauseButton[NORMAL_M], sr, r);
						if (fCDID)
							ioctl(fCDID, B_SCSI_RESUME_AUDIO);
						fState = PLAYING_S;
					}
					if (item == BACK_B)
						scan.direction = -1;
					else
						scan.direction = 1;
					GetMouse(&where, &buttons);
					if (buttons > 1)
						scan.speed = 1;
					else
						scan.speed = 0;
					if (fCDID)
						ioctl(fCDID, B_SCSI_SCAN, &scan);
				}
			}
			else {
				new_track = (int)((thePoint.x - dr.left) / 13) + 1;
				old_track = fTrack;
				if (new_track != fTrack)
					DrawListItems(new_track, fTrack);
			}
			do {
				GetMouse(&where, &buttons);
				if (dr.Contains(where)) {
					if (item == LISTITEM_B) {
						if (new_track != ((int32)(where.x - dr.left) / 13) + 1) {
							DrawListItems(((int32)(where.x - dr.left) / 13) + 1, new_track);
							new_track = ((int32)(where.x - dr.left) / 13) + 1;
							hit = true;
						}
						else
						if (fTrack != old_track) {
							DrawListItems(new_track, fTrack);
							old_track = fTrack;
						}
					}
					else if ((item == BACK_B) || (item == FOR_B)) {
						doit = false;
						if ((buttons > 1) && (scan.speed != 1)) {
							doit = true;
							scan.speed = 1;
						}
						else if ((buttons == 1) && (scan.speed)) {
							doit = true;
							scan.speed = 0;
						}
						if (!hit) {
							hit = true;
							DrawBitmap(button_on, sr, dr);
							doit = true;
						}
						if ((fCDID) && (doit))
							ioctl(fCDID, B_SCSI_SCAN, &scan);
					}
					else
					if (!hit) {
						hit = true;
						DrawBitmap(button_on, sr, dr);
					}
				}
				else if (hit) {
					hit = false;
					if (item == LISTITEM_B) {
						if (new_track != fTrack) {
							DrawListItems(fTrack, new_track);
							new_track = fTrack;
						}
					}
					else if ((item == BACK_B) || (item == FOR_B)) {
						DrawBitmap(button_off, sr, dr);
						if (fCDID) {
							ioctl(fCDID, B_SCSI_GET_POSITION, &pos);
							audio.start_m = pos.position[9];
							audio.start_s = pos.position[10];
							audio.start_f = pos.position[11];
							audio.end_m = 99;
							audio.end_s = 59;
							audio.end_f = 71;
							ioctl(fCDID, B_SCSI_PLAY_POSITION, &audio);
						}
					}
					else
						DrawBitmap(button_off, sr, dr);
				}
				Status();
				if(buttons) {
					snooze(10000);
				}
			} while (buttons);

			if (hit) {
				if (item != LISTITEM_B)
					DrawBitmap(button_off, sr, dr);
				switch (item) {
					case CLOCK_B:
						if (fTimeMode == DISC_EL)
							SetTime(TRACK_RE);
						else
							SetTime(fTimeMode + 1);
						break;
						
					case STOP_B:
						if (fState != STOPPED_S) {
							if (fState != PAUSED_S) {
								dr.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
								DrawBitmap(fPlayButton[NORMAL_M], sr, dr);
							}
							if (fRepeatMode == REPEAT_B) {
								fRepeatMode = NORMAL_B;
								dr.Set(NORMAL_H, BUTTON_V,
									   NORMAL_H + BUTTON_WIDTH,
									   BUTTON_V + BUTTON_HEIGHT);
								DrawBitmap(fNormalButton[NORMAL_M], sr, dr);
							}
							if (fCDID)
								ioctl(fCDID, B_SCSI_STOP_AUDIO);
							fState = STOPPED_S;
							DrawTrack(fMinTrack);
						}
						break;

					case PLAY_B:
						if (fState == PAUSED_S) {
							DrawBitmap(fPauseButton[NORMAL_M], sr, dr);
							if (fCDID)
								ioctl(fCDID, B_SCSI_RESUME_AUDIO);
							fState = PLAYING_S;
						}
						else if (fState == PLAYING_S) {
							DrawBitmap(fPlayButton[NORMAL_M], sr, dr);
							if (fCDID)
								ioctl(fCDID, B_SCSI_PAUSE_AUDIO);
							fState = PAUSED_S;
						}
						else if (fState == STOPPED_S) {
							fState = PLAYING_S;
							PlayTrack(fTrack);
						}
						break;

					case PREV_B:
						if (fState != STOPPED_S) {
							// if we're more than 3 seconds into current track, start
							// this track over, else go to previous track.
							if (fCDID)
								ioctl(fCDID, B_SCSI_GET_POSITION, &pos);
							if ((pos.position[13]) || (pos.position[14] > 3) || (fTrack == fMinTrack))
								PlayTrack(fTrack);
							else
								PlayTrack(fTrack - 1);
						}
						else
							PlayTrack(fTrack - 1);
						break;

					case NEXT_B:
						PlayTrack(fTrack + 1);
						break;

					case BACK_B:
					case FOR_B:
						if (fCDID) {
							ioctl(fCDID, B_SCSI_GET_POSITION, &pos);
							audio.start_m = pos.position[9];
							audio.start_s = pos.position[10];
							audio.start_f = pos.position[11];
							audio.end_m = 99;
							audio.end_s = 59;
							audio.end_f = 71;
							ioctl(fCDID, B_SCSI_PLAY_POSITION, &audio);
						}
						break;

					case EJECT_B:
						acquire_sem(cd_sem);
						if(!fCDID) {
							BMenu		*sub_menu;
							BMenuItem	*item;
							sub_menu = ((TCDApplication *)be_app)->fMenu->SubmenuAt(3);
							if (sub_menu) {
								item = sub_menu->ItemAt(fCDIndex);
								if (item) {
									if ((fCDID = open(item->Label(), 0)) <= 0) {
										
										fCDID = 0;
									}
								}
							}
						}
						if (fCDID) {
							status_t media_status = B_NO_ERROR;
							ioctl(fCDID, B_GET_MEDIA_STATUS, &media_status,
							      sizeof(media_status));
							ioctl(fCDID, media_status == B_DEV_DOOR_OPEN ?
							      B_LOAD_MEDIA : B_EJECT_DEVICE);
							close(fCDID);
							fCDID = 0;
						}
						fState = DISABLED_S;
						release_sem(cd_sem);
						DisablePlayer();
						break;

					case SAVE_B:
						if (!fSaveWindow) {
							changed = false;
							{
								BScreen screen( Window() );
								BRect sframe = screen.Frame();
								r.left = (sframe.Width() - SAVE_WIDTH) / 2;
								r.right = r.left + SAVE_WIDTH;
								r.top = (sframe.Height() - SAVE_HEIGHT) / 2;
								r.bottom = r.top + SAVE_HEIGHT;

								fSaveWindow = new TSaveWindow(r, "Save Audio",
										 fCDID, &fTOC, fTrack, fTitleList);
							}
						}

						if (fSaveWindow->Lock()) {
							if (fSaveWindow->IsHidden())
								fSaveWindow->ShowTrack(fTrack);
							fSaveWindow->Activate(true);
							fSaveWindow->Unlock();
						}
						break;
						
					case MODE_B:
						if (fPlayMode == MODE_B) {
							fPlayMode = SHUFFLE_B;
							DrawBitmap(fShuffleButton[NORMAL_M], sr, dr);
						}
						else {
							fPlayMode = MODE_B;
							DrawBitmap(fModeButton[NORMAL_M], sr, dr);
						}
						break;

					case NORMAL_B:
						if (fRepeatMode == NORMAL_B) {
							fRepeatMode = REPEAT_B;
							DrawBitmap(fRepeatButton[NORMAL_M], sr, dr);
						}
						else {
							fRepeatMode = NORMAL_B;
							DrawBitmap(fNormalButton[NORMAL_M], sr, dr);
						}
						break;

					case LISTITEM_B:
						if (fState == STOPPED_S)
							fState = PLAYING_S;
						PlayTrack(new_track);
						break;

					case BLANK_B:
						if (fTrackView) {
							Window()->ResizeTo(WIND_WIDTH, WIND_HEIGHT);
							Window()->RemoveChild(fTrackView);
							delete fTrackView;
							fTrackView = NULL;
						}
						else {
							r.Set(0, WIND_HEIGHT + 1, WIND_WIDTH,
								  WIND_HEIGHT + HEADER + TRAILER +
								  (CELL_HEIGHT * fTotalTracks));
							fTrackView = new TTrackView(r, "TrackView", &fTOC, fTitleList, Window(), fCDID);
							Window()->AddChild(fTrackView);
							Window()->ResizeTo(WIND_WIDTH, r.bottom);
						}
						break;
				}
			}
		}
	}
}

//--------------------------------------------------------------------

void TCDView::Pulse(void)
{
	bool		gotone = false;
	char		byte;
	int32		start;
	int32		len;
	BMenu		*sub_menu;
	BMenuItem	*item;
	track_info	*track_data;
	track_info	*album_data;
	status_t	media_status;

	if (!fCDID) {
		acquire_sem(cd_sem);
		sub_menu = ((TCDApplication *)be_app)->fMenu->SubmenuAt(3);
		if (sub_menu) {
			item = sub_menu->ItemAt(fCDIndex);
			if (item) {
				if ((fCDID = open(item->Label(), 0)) <= 0) {
					
					fCDID = 0;
				}
				else {
					if(ioctl(fCDID, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status)) >= 0 &&
					   (media_status == B_DEV_NO_MEDIA ||
					    media_status == B_DEV_NOT_READY ||
					    media_status == B_DEV_DOOR_OPEN )) {
						close(fCDID);
						fCDID = 0;
						release_sem(cd_sem);
						goto done;
					}
					if (ioctl(fCDID, B_SCSI_GET_TOC, &fTOC) < B_NO_ERROR) {
						close(fCDID);
						fCDID = 0;
						release_sem(cd_sem);
						goto done;
					}
					fMinTrack = fTOC.toc_data[2];
					fMaxTrack = fTOC.toc_data[3];
					DrawTrack(fMinTrack);
					DrawListItems(true);
					DrawListItems(false);

					fTotalTracks = 0;
					album_data = (track_info*)malloc(sizeof(track_info));
					album_data->flags = 0;
					sprintf(album_data->title, "Audio CD");
					fTitleList->AddItem(album_data);
					while (fTotalTracks <= (fTOC.toc_data[3] - fTOC.toc_data[2])) {
						track_data = (track_info*)malloc(sizeof(track_info));
						track_data->flags = 0;
						sprintf(track_data->title, "Track %d", fTotalTracks + 1);
						fTitleList->AddItem(track_data);

						start = (fTOC.toc_data[4 + (fTotalTracks * 8) + 5] * 60) +
								(fTOC.toc_data[4 + (fTotalTracks * 8) + 6]);
						len = ((fTOC.toc_data[4 + ((fTotalTracks + 1) * 8) + 5] * 60) +
							   (fTOC.toc_data[4 + ((fTotalTracks + 1) * 8) + 6])) - start;
						track_data->length = len;
						fTotalTracks++;
					}
					gotone = true;
					byte = fTOC.toc_data[4 + (fTotalTracks * 8) + 5];
					len = byte * 60;
					fKey = 0;
					fKey = (byte / 10) << 20;
					fKey |= (byte % 10) << 16;
					byte = fTOC.toc_data[4 + (fTotalTracks * 8) + 6];
					len += byte;
					album_data->length = len;
					fKey |= (byte / 10) << 12;
					fKey |= (byte % 10) << 8;
					byte = fTOC.toc_data[4 + (fTotalTracks * 8) + 7];
					fKey |= (byte / 10) << 4;
					fKey |= byte % 10;
					FindRecord(fKey, false);
					DrawTitle(false);
				}
			}
		}
		release_sem(cd_sem);
	}
  done:
	if (fCDID) {
		Status();
		if (gotone) {
			track = fTrack;
			changed = true;
		}
	}
	else
	if (fState != DISABLED_S) {
		fState = DISABLED_S;
		DisablePlayer();
	}
}

//--------------------------------------------------------------------

void TCDView::ChangeDriver(int32 index)
{
	if (index != fCDIndex) {
		if (fCDID) {
			acquire_sem(cd_sem);
			close(fCDID);
			fCDID = 0;
			fKey = 0;
			DisablePlayer();
			release_sem(cd_sem);
		}
		fCDIndex = index;
		Pulse();
	}
}

//--------------------------------------------------------------------

void TCDView::DisablePlayer(void)
{
	char		byte;
	const char	*text;
	int32		length;
	int32		loop;
	track_info	*track_data;

	if (fTextView) {
		RemoveChild(fTextView);
		track_data = (track_info*)(fTitleList->ItemAt(0));
		text = fTextView->Text();
		length = fTextView->TextLength();
		for (loop = 0; loop < length; loop++)
			track_data->title[loop] = *text++;
		track_data->title[loop] = 0;
		track_data->flags |= DIRTY;
		delete fTextView;
		fTextView = NULL;
	}
	if (fTrackView) {
		Window()->ResizeTo(WIND_WIDTH, WIND_HEIGHT);
		Window()->RemoveChild(fTrackView);
		delete fTrackView;
		fTrackView = NULL;
	}
	DrawTitle(true);
	Window()->SetTitle(kWINDOW_TITLE);
	fTrack = 1;
	fMinTrack = 1;
	fMaxTrack = 1;
	DrawTrack(fTrack, false);
	DrawButtons(Window()->Bounds());
	DrawListItems(true);
	fMin = 0;
	fSec = 0;
	fPercent = 0.0;
	DrawTime();
	if (fSaveWindow)
		fSaveWindow->Hide();

	FindRecord(fKey, true);
	while (track_data = (track_info*)fTitleList->ItemAt(0)) {
		fTitleList->RemoveItem(track_data);
		free(track_data);
	}

}

//--------------------------------------------------------------------

void TCDView::DrawButtons(BRect where)
{
	BRect	dr;
	BRect	sr;

	sr.Set(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
	dr.Set(BLANK_H, BUTTON_V, BLANK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
	DrawBitmap(fListOpenButton[NORMAL_M], sr, dr);

	switch (fState) {
		case DISABLED_S:
			dr.Set(BLANK_H, BUTTON_V, BLANK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fListOpenButton[DISABLE_M], sr, dr);

			dr.Set(STOP_H, BUTTON_V, STOP_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fStopButton[DISABLE_M], sr, dr);

			dr.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fPlayButton[DISABLE_M], sr, dr);

			dr.Set(PREV_H, BUTTON_V, PREV_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fPrevButton[DISABLE_M], sr, dr);

			dr.Set(NEXT_H, BUTTON_V, NEXT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fNextButton[DISABLE_M], sr, dr);

			dr.Set(BACK_H, BUTTON_V, BACK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fBackButton[DISABLE_M], sr, dr);

			dr.Set(FOR_H, BUTTON_V, FOR_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fForButton[DISABLE_M], sr, dr);

			dr.Set(EJECT_H, BUTTON_V, EJECT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fEjectButton[NORMAL_M], sr, dr);

			dr.Set(SAVE_H, BUTTON_V, SAVE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fSaveButton[DISABLE_M], sr, dr);

			dr.Set(MODE_H, BUTTON_V, MODE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fPlayMode == MODE_B)
					DrawBitmap(fModeButton[DISABLE_M], sr, dr);
				else
					DrawBitmap(fShuffleButton[DISABLE_M], sr, dr);
			}

			dr.Set(NORMAL_H, BUTTON_V, NORMAL_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fRepeatMode == NORMAL_B)
					DrawBitmap(fNormalButton[DISABLE_M], sr, dr);
				else
					DrawBitmap(fRepeatButton[DISABLE_M], sr, dr);
			}

			dr.Set(CLOCK_H, CLOCK_V, CLOCK_H + CLOCK_WIDTH, CLOCK_V + CLOCK_HEIGHT);
			if (where.Intersects(dr)) {
				sr.Set(0, 0, CLOCK_WIDTH, CLOCK_HEIGHT);
				DrawBitmap(fClockButton[DISABLE_M], sr, dr);
			}
			break;

		case PLAYING_S:
		case STOPPED_S:
			dr.Set(BLANK_H, BUTTON_V, BLANK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fTrackView)
					DrawBitmap(fListCloseButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fListOpenButton[NORMAL_M], sr, dr);
			}

			dr.Set(STOP_H, BUTTON_V, STOP_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fStopButton[NORMAL_M], sr, dr);

			dr.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fState == STOPPED_S)
					DrawBitmap(fPlayButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fPauseButton[NORMAL_M], sr, dr);
			}

			dr.Set(PREV_H, BUTTON_V, PREV_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fPrevButton[NORMAL_M], sr, dr);

			dr.Set(NEXT_H, BUTTON_V, NEXT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fTrack == fMaxTrack)
					DrawBitmap(fNextButton[DISABLE_M], sr, dr);
				else
					DrawBitmap(fNextButton[NORMAL_M], sr, dr);
			}

			dr.Set(BACK_H, BUTTON_V, BACK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fBackButton[NORMAL_M], sr, dr);

			dr.Set(FOR_H, BUTTON_V, FOR_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fForButton[NORMAL_M], sr, dr);

			dr.Set(EJECT_H, BUTTON_V, EJECT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fEjectButton[NORMAL_M], sr, dr);

			dr.Set(SAVE_H, BUTTON_V, SAVE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fSaveButton[NORMAL_M], sr, dr);

			dr.Set(MODE_H, BUTTON_V, MODE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fPlayMode == MODE_B)
					DrawBitmap(fModeButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fShuffleButton[NORMAL_M], sr, dr);
			}

			dr.Set(NORMAL_H, BUTTON_V, NORMAL_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fRepeatMode == NORMAL_B)
					DrawBitmap(fNormalButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fRepeatButton[NORMAL_M], sr, dr);
			}

			dr.Set(CLOCK_H, CLOCK_V, CLOCK_H + CLOCK_WIDTH, CLOCK_V + CLOCK_HEIGHT);
			if (where.Intersects(dr)) {
				sr.Set(0, 0, CLOCK_WIDTH, CLOCK_HEIGHT);
				DrawBitmap(fClockButton[NORMAL_M], sr, dr);
			}
			break;

		case PAUSED_S:
			dr.Set(BLANK_H, BUTTON_V, BLANK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fTrackView)
					DrawBitmap(fListCloseButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fListOpenButton[NORMAL_M], sr, dr);
			}

			dr.Set(STOP_H, BUTTON_V, STOP_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fStopButton[NORMAL_M], sr, dr);

			dr.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fPlayButton[NORMAL_M], sr, dr);

			dr.Set(PREV_H, BUTTON_V, PREV_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fPrevButton[NORMAL_M], sr, dr);

			dr.Set(NEXT_H, BUTTON_V, NEXT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fTrack == fMaxTrack)
					DrawBitmap(fNextButton[DISABLE_M], sr, dr);
				else
					DrawBitmap(fNextButton[NORMAL_M], sr, dr);
			}

			dr.Set(BACK_H, BUTTON_V, BACK_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fBackButton[NORMAL_M], sr, dr);

			dr.Set(FOR_H, BUTTON_V, FOR_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fForButton[NORMAL_M], sr, dr);

			dr.Set(EJECT_H, BUTTON_V, EJECT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fEjectButton[NORMAL_M], sr, dr);

			dr.Set(SAVE_H, BUTTON_V, SAVE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr))
				DrawBitmap(fSaveButton[NORMAL_M], sr, dr);

			dr.Set(MODE_H, BUTTON_V, MODE_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fPlayMode == MODE_B)
					DrawBitmap(fModeButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fShuffleButton[NORMAL_M], sr, dr);
			}

			dr.Set(NORMAL_H, BUTTON_V, NORMAL_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
			if (where.Intersects(dr)) {
				if (fRepeatMode == NORMAL_B)
					DrawBitmap(fNormalButton[NORMAL_M], sr, dr);
				else
					DrawBitmap(fRepeatButton[NORMAL_M], sr, dr);
			}

			dr.Set(CLOCK_H, CLOCK_V, CLOCK_H + CLOCK_WIDTH, CLOCK_V + CLOCK_HEIGHT);
			if (where.Intersects(dr))	{
				sr.Set(0, 0, CLOCK_WIDTH, CLOCK_HEIGHT);
				DrawBitmap(fClockButton[NORMAL_M], sr, dr);
			}
			break;

		default:
			break;
	}
}

//--------------------------------------------------------------------

void TCDView::DrawListItems(int32 newtrack, int32 oldtrack)
{
	char	string[128];
	int32	i;
	float	width;
	BRect	r;

	SetDrawingMode(B_OP_OVER);

	if (oldtrack) {
		i = 0;
		while ((fTOC.toc_data[4 + (i * 8) + 2] != oldtrack) && (i < 17)) {
			i++;
		}
		if (i < 17) {
			SetHighColor(255, 255, 255);
			StrokeLine(BPoint((i * 13) + ITEM_L + 1, ITEM_T + 1),
					   BPoint((i * 13) + 11 + ITEM_L + 1, ITEM_T + 1));
			StrokeLine(BPoint((i * 13) + ITEM_L + 1, ITEM_T + 1),
					   BPoint((i * 13) + ITEM_L + 1, ITEM_B - 2));
			SetHighColor(128, 128, 128);
			StrokeLine(BPoint((i * 13) + ITEM_L + 1, ITEM_B - 1),
					   BPoint((i * 13) + 12 + ITEM_L + 1, ITEM_B - 1));
			StrokeLine(BPoint((i * 13) + 12 + ITEM_L + 1, ITEM_B - 1),
					   BPoint((i * 13) + 12 + ITEM_L + 1, ITEM_T + 1));
			SetHighColor(208, 208, 208);
			r.Set((i * 13) + ITEM_L + 2, ITEM_T + 2,
				  (i * 13) + 11 + ITEM_L + 1, ITEM_B - 2);
			FillRect(r);
			sprintf(string,"%d", fTOC.toc_data[4 + (i * 8) + 2]);
			SetHighColor(0, 0, 0);
			width = (11 - StringWidth(string)) / 2;
			MovePenTo(r.left + width, r.bottom - 1);
			DrawString(string);
		}
	}

	if (newtrack) {
		i = 0;
		while ((fTOC.toc_data[4 + (i * 8) + 2] != newtrack) && (i < 17)) {
			i++;
		}
		if (i < 17) {
			SetHighColor(136, 136, 136);
			r.Set((i * 13) + ITEM_L + 1, ITEM_T + 1,
				  (i * 13) + 12 + ITEM_L + 1, ITEM_B - 1);
			FillRect(r);
			r.InsetBy(1, 1);
			sprintf(string,"%d", fTOC.toc_data[4 + (i * 8) + 2]);
			SetHighColor(0, 0, 0);
			width = (11 - StringWidth(string)) / 2;
			MovePenTo(r.left + width, r.bottom - 1);
			DrawString(string);
		}
	}
	SetDrawingMode(B_OP_COPY);
}

//--------------------------------------------------------------------

void TCDView::DrawListItems(bool clear)
{
	int32	i = 0;
	BRect	r;

	if (clear) {
		SetHighColor(208, 208, 208);
		r.Set(ITEM_L + 1, ITEM_T + 1, ITEM_R - 1, ITEM_B - 1);
		FillRect(r);
	}
	else {
		while ((fTOC.toc_data[4 + (i * 8) + 2] != 0xaa) && (i < 17)) {
			if (fTrack == fTOC.toc_data[4 + (i * 8) + 2])
				DrawListItems(fTrack, 0);
			else
				DrawListItems(0, fTOC.toc_data[4 + (i * 8) + 2]);
			i++;
		}
	}
}

//--------------------------------------------------------------------

void TCDView::DrawTime(void)
{
	char	time[256];
	int32	arrow1;
	int32	arrow2;
	BRect	sr;
	BRect	dr;

	fOffScreen->Lock();
	fOffView->SetFont(be_plain_font);
	fOffView->SetFontSize(9);
	sr = fOffView->Bounds();
	fOffView->SetHighColor(0, 0, 0);
	fOffView->FillRect(sr);

	fOffView->SetDrawingMode(B_OP_OVER);
	fOffView->SetHighColor(0, 203, 0);
	sprintf(time, "%.2d:%.2d", fMin, fSec);
	fOffView->MovePenTo(0, (TIME_TOP - TIME_T) - 3);
	fOffView->DrawString(time);
	fOffView->SetDrawingMode(B_OP_COPY);

	sr.top += 15;
	sr.bottom -= 1;
	fOffView->SetHighColor(128, 128, 128);
	fOffView->FillRect(sr);
	sr.left += 7;
	sr.right -= 7;
	arrow1 = (int32)sr.left;
	arrow2 = (int32)sr.right;
	if (fPercent) {
		sr.right = sr.left + ((sr.right - sr.left) * fPercent);
		fOffView->SetHighColor(0, 178, 0);
		fOffView->FillRect(sr);
	}
	fOffView->SetHighColor(0, 0, 0);
	fOffView->StrokeLine(BPoint(arrow1, sr.bottom - 3), BPoint(arrow1 - 3, sr.bottom));
	fOffView->StrokeLine(BPoint(arrow1, sr.bottom - 3), BPoint(arrow1 + 3, sr.bottom));
	fOffView->StrokeLine(BPoint(arrow2, sr.bottom - 3), BPoint(arrow2 - 3, sr.bottom));
	fOffView->StrokeLine(BPoint(arrow2, sr.bottom - 3), BPoint(arrow2 + 3, sr.bottom));
	fOffView->SetHighColor(255, 255, 255);
	fOffView->StrokeLine(BPoint(arrow1, sr.bottom - 2), BPoint(arrow1 - 2, sr.bottom));
	fOffView->StrokeLine(BPoint(arrow2, sr.bottom - 2), BPoint(arrow2 - 2, sr.bottom));
	fOffView->SetHighColor(128, 128, 128);
	fOffView->StrokeLine(BPoint(arrow1 - 3, sr.bottom + 1), BPoint(arrow1 + 3, sr.bottom + 1));
	fOffView->StrokeLine(BPoint(arrow2 - 3, sr.bottom + 1), BPoint(arrow2 + 3, sr.bottom + 1));
	fOffView->StrokeLine(BPoint(arrow1 + 1, sr.bottom - 1), BPoint(arrow1 + 2, sr.bottom));
	fOffView->StrokeLine(BPoint(arrow2 + 1, sr.bottom - 1), BPoint(arrow2 + 2, sr.bottom));
	fOffView->SetHighColor(192, 192, 192);
	fOffView->StrokeLine(BPoint(arrow1, sr.bottom - 1), BPoint(arrow1, sr.bottom - 1));
	fOffView->StrokeLine(BPoint(arrow2, sr.bottom - 1), BPoint(arrow2, sr.bottom - 1));
	fOffView->StrokeLine(BPoint(arrow1 - 1, sr.bottom), BPoint(arrow1 + 1, sr.bottom));
	fOffView->StrokeLine(BPoint(arrow2 - 1, sr.bottom), BPoint(arrow2 + 1, sr.bottom));
	fOffView->Sync();
	DrawBitmap(fOffScreen, BPoint(TIME_LEFT, (TIME_TOP - TIME_T) + 1));
	fOffScreen->Unlock();
}

//--------------------------------------------------------------------

void TCDView::DrawTitle(bool clear, int32 item)
{
	char		*string;
	char		*src[1];
	char		*result[1];
	BFont		font;
	BRect		r;
	track_info	*track_data;

	r.Set(TITLE_L + 7, TITLE_T + 9, TITLE_R - 8, TITLE_B - 4);
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(r);
	if (!clear) {
		if ((fState == DISABLED_S) || (fState == STOPPED_S))
			item = 0;
		if ((track_data = (track_info*)(fTitleList->ItemAt(item))) == NULL)
			track_data = (track_info*)(fTitleList->ItemAt(0));
		if (track_data) {
			SetHighColor(0, 0, 0);
			MovePenTo(TITLE_L + 8, TITLE_B - 7);
			SetDrawingMode(B_OP_OVER);
			SetFontSize(12);
			SetFont(be_bold_font);
			GetFont(&font);

			src[0] = track_data->title;
			string = (char *)malloc(strlen(track_data->title) + 16);
			result[0] = string;
			font.GetTruncatedStrings((const char **)src, 1, B_TRUNCATE_MIDDLE, r.Width() - 2, result);

			DrawString(string);
			free(string);
			SetFont(be_plain_font);
			SetDrawingMode(B_OP_COPY);
			SetFontSize(9);
		}
	}
}

//--------------------------------------------------------------------

void TCDView::DrawTrack(int32 track, bool update)
{
	BRect		dr;
	BRect		sr;

	sr.Set(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
	dr.Set(NEXT_H, BUTTON_V, NEXT_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
	if (track >= fMaxTrack)
		DrawBitmap(fNextButton[DISABLE_M], sr, dr);
//	if ((fTrack >= fMaxTrack) && (track < fMaxTrack))
	else
		DrawBitmap(fNextButton[NORMAL_M], sr, dr);

	dr.Set(PREV_H, BUTTON_V, PREV_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
	if (track < fMinTrack)
		DrawBitmap(fPrevButton[DISABLE_M], sr, dr);
//	if ((fTrack <= fMinTrack) && (track > fMinTrack))
	else
		DrawBitmap(fPrevButton[NORMAL_M], sr, dr);

	DrawListItems(track, fTrack);
	(track < 0) ? fTrack = 0 : fTrack = track;
	if (fTrack > 99)
		fTrack = 0;
	DrawBitmap(fLEDs[fTrack / 10], BPoint(TRACK_LED1_LEFT, TRACK_LED_TOP));
	DrawBitmap(fLEDs[fTrack - ((fTrack / 10) * 10)], BPoint(TRACK_LED2_LEFT, TRACK_LED_TOP));

	if (update)
		DrawTitle(false, fTrack);
}

//--------------------------------------------------------------------

void TCDView::FindRecord(uint32 key, bool update)
{
	bool				have_file = false;
	char				str[256];
	char				title[B_FILE_NAME_LENGTH];
	char				*list;
	int32				loop = 0;
	int32				index = 0;
	int32				offset;
	BEntry				entry;
	BFile				file;
	BQuery				query;
	BVolume				vol;
	BVolumeRoster		volume;
	attr_info			info;
	track_info			*track_data;

	sprintf(str, "%s=%d", CD_KEY, key);
	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate(str);
	query.Fetch();

	if (query.GetNextEntry(&entry) == B_NO_ERROR)
		file.SetTo(&entry, O_RDWR);
	else if ((update) && (make_file(&entry, &file) == B_NO_ERROR)) {
		fs_create_index(vol.Device(), CD_KEY, B_INT32_TYPE, 0);
		file.WriteAttr(CD_KEY, B_INT32_TYPE, 0, &key, sizeof(int32));
		if (track_data = (track_info *)fTitleList->ItemAt(0))
			track_data->flags |= DIRTY;
	}
	else
		return;

	if (update) {
		if (!IsDirty())
			return;
		if ((track_data = (track_info *)fTitleList->ItemAt(0)) &&
						(track_data->flags & DIRTY)) {
			strcpy(title, track_data->title);
			while (title[index]) {
				if (title[index] == '/')
					title[index] = '\\';
				index++;
			}
			strcpy(str, title);
			index = 1;
			while (entry.Rename(title) != B_NO_ERROR) {
				sprintf(title, "%s:%d", str, index++);
			}
		}

		fs_create_index(vol.Device(), "CD:tracks", B_STRING_TYPE, 0);
		file.SetSize(0);
		list = (char *)malloc(1);
		list[0] = 0;
	}
	else if (file.GetAttrInfo("CD:tracks", &info) == B_NO_ERROR) {
		list = (char *)malloc(info.size);
		file.ReadAttr("CD:tracks", B_STRING_TYPE, 0, list, info.size);
	}
	else
		return;

	while (track_data = (track_info *)fTitleList->ItemAt(loop)) {
		if (update) {
			track_data->flags &= ~DIRTY;
			list = (char *)realloc(list, strlen(list) + strlen(track_data->title) + 2);
			strcpy(&list[strlen(list)], track_data->title);
			list[strlen(list) + 1] = 0;
			list[strlen(list)] = '\n';

			file.Write(track_data->title, strlen(track_data->title));
			sprintf(str, "\t%.2d:%.2d\n", track_data->length / 60,
									   	track_data->length % 60);
			file.Write(str, strlen(str));
		}
		else {
			offset = index;
			while (list[index] != '\n') {
				index++;
			}
			list[index] = 0;
			strcpy(track_data->title, &list[offset]);
			index++;
			track_data->flags = 0;
			if (!loop)
				Window()->SetTitle(track_data->title);
		}
		loop++;
	}
	if (update)
		file.WriteAttr("CD:tracks", B_STRING_TYPE, 0, list, strlen(list) + 1);
	free(list);
}


//--------------------------------------------------------------------

bool TCDView::IsDirty(void)
{
	int32				loop = 0;
	track_info			*track_data;

	while (track_data = (track_info *)fTitleList->ItemAt(loop++)) {
		if (track_data->flags & DIRTY)
			return true;
	}
	return false;
}

//--------------------------------------------------------------------

void TCDView::PlayTrack(int32 play_track)
{
	BRect				dr;
	BRect				sr;
	scsi_play_track		track;

	if (play_track < fMinTrack)
		play_track = fMinTrack;
	track.start_track = play_track;
	track.start_index = 1;
	track.end_track = fMaxTrack;
	track.end_index = 1;
	if (play_track != fTrack) {
		int32	saved_state = fState;

		fState = PLAYING_S;
		DrawTrack(play_track);
		fState = saved_state;
	}
	else
		DrawTitle(false, play_track);
	if (fState != STOPPED_S) {
		sr.Set(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
		dr.Set(PLAY_H, BUTTON_V, PLAY_H + BUTTON_WIDTH, BUTTON_V + BUTTON_HEIGHT);
		DrawBitmap(fPauseButton[NORMAL_M], sr, dr);
	}
	if (fCDID) {
		if (ioctl(fCDID, B_SCSI_PLAY_TRACK, &track) != B_NO_ERROR) {
			track.end_index = 0;
			ioctl(fCDID, B_SCSI_PLAY_TRACK, &track);
		}
	}
}


//--------------------------------------------------------------------

void TCDView::SetTime(int32 time)
{
	BRect	r;

	fTimeMode = time;
	time_mode = time;
	SetHighColor(0, 0, 0);
	r.Set(TIME_TEXT1_LEFT, TIME_T + 4, TIME_TEXT_RIGHT, TIME_B - 4);
	FillRect(r);
	MovePenTo(TIME_TEXT1_LEFT, TIME_TEXT1_TOP);
	SetDrawingMode(B_OP_OVER);
	SetHighColor(0, 203, 0);
	switch (fTimeMode) {
		case TRACK_RE:
		case DISC_RE:
			DrawString("remaining:");
			break;

		case TRACK_EL:
		case DISC_EL:
			DrawString("elapsed:");
			break;
	}

	MovePenTo(TIME_TEXT2_LEFT, TIME_TEXT2_TOP);
	switch (fTimeMode) {
		case TRACK_RE:
		case TRACK_EL:
			DrawString("total trk:");
			break;

		case DISC_RE:
		case DISC_EL:
			DrawString("total disc:");
			break;
	}
	SetDrawingMode(B_OP_COPY);
	if (fCDID)
		Status();
}

//--------------------------------------------------------------------

void TCDView::Status(void)
{
	int32				state;
	int32				i = 0;
	int32				length;
	int32				position;
	int32				min;
	int32				sec;
	float				percent;
	scsi_position		pos;

	if (fCDID) {
		if (ioctl(fCDID, B_SCSI_GET_POSITION, &pos) != B_NO_ERROR) {
			acquire_sem(cd_sem);
			fCDID = 0;
			fState = DISABLED_S;
			release_sem(cd_sem);
			DisablePlayer();
		}

		if ((!pos.position[1]) || (pos.position[1] >= 0x13) ||
		   ((pos.position[1] == 0x12) && (!pos.position[6]))) {
			if (fRepeatMode == REPEAT_B)
				PlayTrack(fMinTrack);
			else {
				state = STOPPED_S;
				if ((fState != STOPPED_S) && (fTrack != fMinTrack)) {
					int32	saved_state = fState;

					fState = state;
					DrawTrack(fMinTrack);
					fState = saved_state;
				}
			}
		}
		else if (pos.position[1] == 0x11)
			state = PLAYING_S;
		else
			state = PAUSED_S;

		if (fReady) {
			if ((state != STOPPED_S) && (fState != DISABLED_S)
				&& (fTrack != pos.position[6])) {
				int32	saved_state = fState;

				fState = state;
				DrawTrack(pos.position[6]);
				fState = saved_state;
			}

			switch (fTimeMode) {
				case TRACK_EL:
				case TRACK_RE:
					// find current track in TOC
					while (fTOC.toc_data[4 + (i * 8) + 2] != fTrack) {
						i++;
					}
					length = ((fTOC.toc_data[4 + ((i + 1) * 8) + 5] * 60) +
							   fTOC.toc_data[4 + ((i + 1) * 8) + 6]) -
							 ((fTOC.toc_data[4 + (i * 8) + 5] * 60) +
							   fTOC.toc_data[4 + (i * 8) + 6]);
					if (fState == STOPPED_S)
						position = 0;
					else
						position = (pos.position[13] * 60) + pos.position[14];
					break;

				case DISC_EL:
				case DISC_RE:
					// find last track in TOC
					while (fTOC.toc_data[4 + (i * 8) + 2] != fMaxTrack) {
						i++;
					}
					i++;
					length = (fTOC.toc_data[4 + (i * 8) + 5] * 60) +
							  fTOC.toc_data[4 + (i * 8) + 6];
					if (fState == STOPPED_S)
						position = 0;
					else
						position = (pos.position[9] * 60) + pos.position[10];
					break;
			}

			switch (fTimeMode) {
				case TRACK_EL:
				case DISC_EL:
					min = position / 60;
					sec = position % 60;
					break;

				case TRACK_RE:
				case DISC_RE:
					min = (length - position) / 60;
					sec = (length - position) % 60;
					break;
			}
			percent = (float)position / (float)length;

			if ((min != fMin) || (sec != fSec) || (percent != fPercent)) {
				fMin = min;
				fSec = sec;
				fPercent = percent;
				DrawTime();
			}
		}

		if (state != fState) {
			fState = state;
			if (fReady)
				DrawButtons(Window()->Bounds());
		}
	}
}


//====================================================================

TSliderView::TSliderView(BRect rect, char *title, TCDView *view)
	   :BView(rect, title, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED)
{
	fCDView = view;
}

//--------------------------------------------------------------------

TSliderView::~TSliderView(void)
{
	delete fOffScreen;
	delete fSlider;
}

//--------------------------------------------------------------------

void TSliderView::AttachedToWindow(void)
{
	uchar*	bits;
	BRect	r;

	fReady = false;
	fValue = 1.00;

	r.Set(0, 0, ((SLIDER_WIDTH + 7) & 0xfff8) - 1, SLIDER_HEIGHT);
	bits = slider;
	fSlider = new BBitmap(r, B_COLOR_8_BIT);
	fSlider->SetBits((char*)bits, fSlider->BitsLength(), 0, B_COLOR_8_BIT);

	r = Frame();
	r.OffsetTo(0, 0);
	fOffScreen = new BBitmap(r, B_COLOR_8_BIT, true);
	fOffScreen->AddChild(fOffView = new BView(r, "", B_FOLLOW_NONE, B_WILL_DRAW));

	Pulse();
	fReady = true;
}

//--------------------------------------------------------------------

void TSliderView::Draw(BRect where)
{
	DrawSlider();
}

//--------------------------------------------------------------------

void TSliderView::MouseDown(BPoint thePoint)
{
	uint32	buttons;
	float	old_value;
	float	temp;
	BPoint	where;
	BRect	r;

	if (Window()->CurrentMessage()->FindInt32("buttons") > 1) {
		where = thePoint;
		ConvertToScreen(&where);
		((TCDApplication*)be_app)->fMenu->Go(where, true);
		return;
	}

	r.left = ((SLIDER_BACK_WIDTH - SLIDER_WIDTH - 4.0) * fValue) + 2.0;
	r.top = 0.0;
	r.right = r.left + SLIDER_WIDTH;
	r.bottom = SLIDER_HEIGHT;

	if (!r.Contains(thePoint)) {
		temp = (thePoint.x / (SLIDER_BACK_WIDTH - SLIDER_WIDTH - 4.0)) - ((SLIDER_WIDTH / 2.0) / SLIDER_BACK_WIDTH);
		if (temp < 0.00)
			temp = 0.00;
		if (temp > 1.00)
			temp = 1.00;
		if (temp != fValue)
			SetValue(temp);
	}

	old_value = fValue;
	do {
		GetMouse(&where, &buttons);
		temp = old_value + ((where.x - thePoint.x) / (SLIDER_BACK_WIDTH - SLIDER_WIDTH - 2.0));
		if (temp < 0.00)
			temp = 0.00;
		if (temp > 1.00)
			temp = 1.00;
		if (temp != fValue) {
			SetValue(temp);
		}
		if (fCDView->fCDID)
			fCDView->Status();
	} while (buttons);
}


//--------------------------------------------------------------------

void TSliderView::Pulse(void)
{
	float			value;
	scsi_volume		volume;

	if (fCDView->fCDID) {
		if (ioctl(fCDView->fCDID, B_SCSI_GET_VOLUME, &volume) == B_NO_ERROR) {
			value = volume.port0_volume / 255.0;
			if (value != fValue)
				SetValue(value);
		}
	}
}

//--------------------------------------------------------------------

void TSliderView::DrawSlider(void)
{
	BRect	sr;
	BRect	dr;

	fOffScreen->Lock();
	sr = fOffView->Bounds();

	// Slider background
	fOffView->SetHighColor(216, 216, 216);
	fOffView->FillRect(sr);
	fOffView->SetHighColor(176, 176, 176);
	fOffView->StrokeLine(BPoint(0, 5), BPoint(0, 5));
	fOffView->StrokeLine(BPoint(0, 10), BPoint(0, 10));
	fOffView->StrokeLine(BPoint(SLIDER_BACK_WIDTH, 5), BPoint(SLIDER_BACK_WIDTH, 5));
	fOffView->SetHighColor(255, 255, 255);
	fOffView->StrokeLine(BPoint(1, 10), BPoint(SLIDER_BACK_WIDTH, 10));
	fOffView->StrokeLine(BPoint(SLIDER_BACK_WIDTH, 9), BPoint(SLIDER_BACK_WIDTH, 6));
	fOffView->SetHighColor(144, 144, 144);
	fOffView->StrokeLine(BPoint(0, 6), BPoint (0, 9));
	fOffView->StrokeLine(BPoint(1, 5), BPoint(SLIDER_BACK_WIDTH - 1, 5));
	fOffView->SetHighColor(0, 0, 0);
	fOffView->StrokeLine(BPoint(1, 6), BPoint(SLIDER_BACK_WIDTH - 1, 6));
	fOffView->StrokeLine(BPoint(1, 7), BPoint(SLIDER_BACK_WIDTH - 1, 7));
	fOffView->StrokeLine(BPoint(1, 8), BPoint(1, 9));
	fOffView->StrokeLine(BPoint(SLIDER_BACK_WIDTH - 1, 8), BPoint(SLIDER_BACK_WIDTH - 1, 9));
	fOffView->SetHighColor(64, 64, 64);
	sr.Set(2, 8, SLIDER_BACK_WIDTH - 2, 9);
	fOffView->FillRect(sr);

	// Slider
	sr.Set(0, 0, SLIDER_WIDTH, SLIDER_HEIGHT);
	dr.left = ((SLIDER_BACK_WIDTH - SLIDER_WIDTH - 4.0) * fValue) + 2.0;
	dr.top = 0;
	dr.right = dr.left + SLIDER_WIDTH;
	dr.bottom = dr.top + SLIDER_HEIGHT;
	//fOffView->DrawBitmap(fSlider, BPoint(((SLIDER_BACK_WIDTH - SLIDER_WIDTH - 4.0) * fValue) + 2.0, 0));
	fOffView->DrawBitmap(fSlider, sr, dr);
	fOffView->SetHighColor(0, 0, 0);
	fOffView->Sync();
	DrawBitmap(fOffScreen, BPoint(0, 0));
	fOffScreen->Unlock();
}

//--------------------------------------------------------------------

void TSliderView::SetValue(float value)
{
	scsi_volume			volume;

	fValue = value;
	if (fReady)
		DrawSlider();

	if (fCDView->fCDID) {
		volume.port0_volume = (uchar)(255 * fValue);
		volume.port1_volume = (uchar)(255 * fValue);
		volume.flags = B_SCSI_PORT0_VOLUME | B_SCSI_PORT1_VOLUME;
		ioctl(fCDView->fCDID, B_SCSI_SET_VOLUME, &volume);
	}

}

//--------------------------------------------------------------------

float TSliderView::Value(void)
{
	return (fValue);
}


/*=================================================================*/

status_t make_file(BEntry *item, BFile *file)
{
	char		name[B_FILE_NAME_LENGTH];
	int32		index = 0;
	status_t	result;
	BBitmap		*bitmap;
	BDirectory	dir;
	BEntry		entry;
	BMessage	msg;
	BMimeType	mime;
	BMimeType	text;
	BNodeInfo	*node;
	BPath		path;

	if ((result = find_directory(B_USER_DIRECTORY, &path)) != B_NO_ERROR) {
		//printf("find_directory failed: %x\n", result);
		return result;
	}
	dir.SetTo(path.Path());
	if ((result = dir.InitCheck()) != B_NO_ERROR) {
		//printf("error setting user directory: %x\n", result);
		return result;
	}
	if ((result = dir.FindEntry("cd", &entry)) != B_NO_ERROR) {
		//printf("creating cd: %x\n", result);
		if ((result = dir.CreateDirectory("cd", &dir)) != B_NO_ERROR) {
			//printf("error creating cd directory: %x\n", result);
			return result;
		}
	}
	else {
		dir.SetTo(&entry);
		if ((result = dir.InitCheck()) != B_NO_ERROR) {
			//printf("error setting cd directory: %x\n", result);
			return result;
		}
	}
		
	while (1) {
		sprintf(name, "cd_%d", index++);
		if (dir.CreateFile(name, file, true) == B_NO_ERROR) {
			node = new BNodeInfo(file);
			node->SetType("text/x-cd_tracks");
			delete node;
			dir.FindEntry(name, item);
			break;
		}
	}
	//printf("created: %s/cd/%s\n", path.Path(), name);

	mime.SetType("text/x-cd_tracks");
	if (!mime.IsInstalled()) {
		text.SetType("text/plain");
		if (text.IsInstalled()) {
			text.GetPreferredApp(name);
			mime.SetPreferredApp(name);
			text.SetType(name);
			bitmap = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_COLOR_8_BIT);
			text.GetIconForType("text/plain", bitmap, B_MINI_ICON);
			mime.SetIcon(bitmap, B_MINI_ICON);
			delete bitmap;

			bitmap = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_COLOR_8_BIT);
			text.GetIconForType("text/plain", bitmap, B_LARGE_ICON);
			mime.SetIcon(bitmap, B_LARGE_ICON);
			delete bitmap;
		}
		else
			mime.SetPreferredApp("application/x-vnd.Be-STEE");
		mime.Install();
	}
	if ((mime.GetAttrInfo(&msg) != B_NO_ERROR) || (!msg.HasString("attr:public_name"))) {
		mime.SetShortDescription("CD Tracks");
		mime.SetLongDescription("CD Tracks");

		msg.AddString("attr:public_name", "Tracks");
		msg.AddString("attr:name", "CD:tracks");
		msg.AddInt32("attr:type", B_STRING_TYPE);
		msg.AddBool("attr:viewable", true);
		msg.AddBool("attr:editable", false);
		msg.AddInt32("attr:width", 200);
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT);
		msg.AddBool("attr:extra", false);
		mime.SetAttrInfo(&msg);
	}
	return B_NO_ERROR;
}

