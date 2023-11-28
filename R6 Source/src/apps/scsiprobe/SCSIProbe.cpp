//--------------------------------------------------------------------
//	
//	SCSIProbe.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Robert Polic, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <fcntl.h>
#include "scsi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Roster.h>
#include <Screen.h>

#include "SCSIProbe.h"
#include <scsiprobe_driver.h>

field_offsets	fOffsets;


//====================================================================

int main()
{	
	TSCSIApp	*myApp;

	myApp = new TSCSIApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TSCSIApp::TSCSIApp(void)
		 :BApplication("application/x-vnd.Be-SCSI")
{
	int32			drvr;
	int32			high;
	int32			vers;
	BDirectory		dir;
	BEntry			entry;
	BPath			path;
	BPoint			win_pos;
	BRect			r;
	TSCSIWindow		*wind;

	fPrefs = NULL;
	drvr = open(DRIVER, 0);
	if (drvr < 0) {
		(new BAlert("", ERROR3_STR, "OK"))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	vers = ioctl(drvr, B_SCSIPROBE_VERSION);
	if ((vers < 4) || (vers == B_ERROR)) {
		close(drvr);
		(new BAlert("", ERROR2_STR, "OK"))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	high = ioctl(drvr, B_SCSIPROBE_HIGHEST_PATH);
	if ((high < 0) || (high == 255)) {
		close(drvr);
		(new BAlert("", ERROR3_STR, "OK"))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	close(drvr);

	r.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);
	win_pos = r.LeftTop();

	fOffsets.path_id = PATH_ID_X1;
	fOffsets.path_sim = PATH_SIM_X1;
	fOffsets.path_sim_vers = PATH_SIM_VERS_X1;
	fOffsets.path_hba = PATH_HBA_X1;
	fOffsets.path_hba_vers = PATH_HBA_VERS_X1;
	fOffsets.path_family = PATH_FAMILY_X1;
	fOffsets.path_type = PATH_TYPE_X1;

	fOffsets.device_id = DEVICE_ID_X1;
	fOffsets.device_type = DEVICE_TYPE_X1;
	fOffsets.device_vendor = DEVICE_VENDOR_X1;
	fOffsets.device_product = DEVICE_PRODUCT_X1;
	fOffsets.device_vers = DEVICE_VERS_X1;

	fOffsets.lun_id = LUN_ID_X1;
	fOffsets.lun_type = LUN_TYPE_X1;
	fOffsets.lun_vendor = LUN_VENDOR_X1;
	fOffsets.lun_product = LUN_PRODUCT_X1;
	fOffsets.lun_vers = LUN_VERS_X1;

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry(SETTINGS_FILE, &entry) == B_NO_ERROR) {
		fPrefs = new BFile();
		fPrefs->SetTo(&entry, O_RDWR);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&win_pos, sizeof(BPoint));
			fPrefs->Read(&fOffsets, sizeof(field_offsets));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(win_pos))
				r.OffsetTo(win_pos);
		}
		else {
			delete fPrefs;
			fPrefs = NULL;
		}
	}
	else {
		fPrefs = new BFile();
		if (dir.CreateFile(SETTINGS_FILE, fPrefs) != B_NO_ERROR) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}

	wind = new TSCSIWindow(r, "SCSIProbe");
	wind->Show();
}

//--------------------------------------------------------------------

TSCSIApp::~TSCSIApp(void)
{
	if (fPrefs)
		delete fPrefs;
}


//====================================================================

TSCSIWindow::TSCSIWindow(BRect rect, char *title)
			  :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE |
													 B_NOT_ZOOMABLE)
{
	BRect		r;
	TSCSIView	*myView;

	r = Frame();
	r.OffsetTo(0, 0);
	myView = new TSCSIView(r, "SCSIView");
	AddChild(myView);

	SetPulseRate(100000);
}

//--------------------------------------------------------------------

bool TSCSIWindow::QuitRequested(void)
{
	BPoint	win_pos;
	BRect	r;

	r = Frame();
	win_pos = r.LeftTop();
	if (((TSCSIApp*)be_app)->fPrefs) {
		((TSCSIApp*)be_app)->fPrefs->Seek(0, 0);
		((TSCSIApp*)be_app)->fPrefs->Write(&win_pos, sizeof(BPoint));
		((TSCSIApp*)be_app)->fPrefs->Write(&fOffsets, sizeof(field_offsets));
	}

	while (find_thread("SCSIMount") != B_NAME_NOT_FOUND) {
		snooze(100000);
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//====================================================================

TSCSIView::TSCSIView(BRect rect, char *title)
		  :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	BBox			*box;
	BRect			r;
	rgb_color		c;

	fFirstUpdateDone = false;
	fReset = false;
	fSelected = I_PATH;

	c.red = c.green = c.blue = VIEW_COLOR;
	SetViewColor(c);

	r = Bounds();
	r.InsetBy(-1, -1);
	box = new BBox(r);
	AddChild(box);

	r.Set(PATH_BOX_X1, PATH_BOX_Y1, PATH_BOX_X2, PATH_BOX_Y2);
	fItems[I_PATH] = new TItem(r, "path", I_PATH, this);
	box->AddChild(fItems[I_PATH]);
	r.Set(DEVICE_BOX_X1, DEVICE_BOX_Y1, DEVICE_BOX_X2, DEVICE_BOX_Y2);
	fItems[I_DEVICE] = new TItem(r, "device", I_DEVICE, this);
	box->AddChild(fItems[I_DEVICE]);
	r.Set(LUN_BOX_X1, LUN_BOX_Y1, LUN_BOX_X2, LUN_BOX_Y2);
	fItems[I_LUN] = new TItem(r, "lun", I_LUN, this);
	box->AddChild(fItems[I_LUN]);

	r.Set(UPDATE_BUTTON_X1, UPDATE_BUTTON_Y1, UPDATE_BUTTON_X2, UPDATE_BUTTON_Y2);
	fUpdateButton = new TButton(r, "update_button", UPDATE_BUTTON_TEXT, M_UPDATE, this);
	box->AddChild(fUpdateButton);

	r.Set(LED_BUTTON_X1, LED_BUTTON_Y1, LED_BUTTON_X2, LED_BUTTON_Y2);
	fLEDButton = new TButton(r, "led_button", LED_BUTTON_TEXT, M_LED, this);
	box->AddChild(fLEDButton);
	fLEDButton->SetEnabled(false);

	r.Set(ABOUT_BUTTON_X1, ABOUT_BUTTON_Y1, ABOUT_BUTTON_X2, ABOUT_BUTTON_Y2);
	fAboutButton = new TButton(r, "about_button", ABOUT_BUTTON_TEXT, M_ABOUT, this);
	box->AddChild(fAboutButton);

	fItems[fSelected]->Select(true);
	MakeFocus(true);
}

//--------------------------------------------------------------------

void TSCSIView::AllAttached(void)
{
	int32								device;
	int32								loop;
	int32								path;
	BAppFileInfo						application;
	BFile								file;
	app_info							app;
	scsiprobe_extended_path_inquiry	info;

	fPath = -1;
	fDevice = -1;
	fLUN = -1;

	be_app->GetAppInfo(&app);
	file.SetTo(&app.ref, O_RDONLY);
	application.SetTo(&file);
	application.GetVersionInfo(&fVersion, B_SYSTEM_VERSION_KIND);

	device = open(DRIVER, 0);
	path = ioctl(device, B_SCSIPROBE_HIGHEST_PATH);
	fPlatform = ioctl(device, B_SCSIPROBE_PLATFORM);
	for (loop = 0; loop <= path; loop++) {
		info.path = loop;
		ioctl(device, B_SCSIPROBE_EXTENDED_PATH_INQUIRY, &info);
		fPathData[loop].host_id = info.data.cam_path.cam_initiator_id;
		if (info.data.cam_path.cam_hba_inquiry & 0x40)
			fPathData[loop].devices = 32;
		else if (info.data.cam_path.cam_hba_inquiry & 0x20)
			fPathData[loop].devices = 16;
		else
			fPathData[loop].devices = 8;
		strcpy(fPathData[loop].sim_vendor, info.data.cam_path.cam_sim_vid);
		strcpy(fPathData[loop].sim_vers, info.data.cam_sim_version);
		strcpy(fPathData[loop].hba_vendor, info.data.cam_path.cam_hba_vid);
		strcpy(fPathData[loop].hba_vers, info.data.cam_hba_version);
		strcpy(fPathData[loop].family, info.data.cam_controller_family);
		strcpy(fPathData[loop].type, info.data.cam_controller_type);
		fItems[I_PATH]->AddItem(&fPathData[loop]);
	}
	close(device);

	fItems[I_PATH]->SetSelectionMessage(new BMessage(M_PATH), this);
	fItems[I_DEVICE]->SetSelectionMessage(new BMessage(M_DEVICE), this);
	fItems[I_LUN]->SetSelectionMessage(new BMessage(M_LUN), this);
	fUpdateButton->SetTarget(this);
	fLEDButton->SetTarget(this);
	fAboutButton->SetTarget(this);

	fItems[I_DEVICE]->SetItemCount(fPathData[0].devices);
	fItems[I_PATH]->SelectItem(0);
	MakeFocus(true);
}

//--------------------------------------------------------------------

void TSCSIView::KeyDown(const char *key, int32 count)
{
	char		c;
	char		raw;
	BMessage	*msg;
	flash_data	flash;
	key_info	keys;

	c = key[0];
	msg = Window()->CurrentMessage();
	raw = msg->FindInt32("key");
	if ((raw == 0x50) && (fReset))
		c = 'b';	

	switch (c) {
		case B_TAB:
			fItems[fSelected]->Select(false);
			if (modifiers() & B_SHIFT_KEY) {
				fSelected -= 1;
				if (fSelected < 0)
					fSelected = I_END - 1;
			}
			else {
				fSelected += 1;
				if (fSelected == I_END)
					fSelected = 0;
			}
			fItems[fSelected]->Select(true);
			break;

		case B_UP_ARROW:
		case B_DOWN_ARROW:
			fItems[fSelected]->KeyDown(key, count);
			break;

		case 'f':
		case 'F':
			get_key_info(&keys);
			if (!(keys.key_states[raw>>3] & (1 << ((7 - raw) & 7))))
				break;
			flash.flag = 0;
			if ((!fReset) && (fLEDButton->IsEnabled())) {
				fLEDButton->SetValue(1);
				do {
					FlashLED(&flash);
					snooze(250000);
					get_key_info(&keys);
				} while (keys.key_states[raw>>3] & (1 << ((7 - raw) & 7)));
				fLEDButton->SetValue(0);
			}
			break;

		case 'b':
			if ((fReset) && (fPath >= 0)) {
				fUpdateButton->SetValue(1);
				Reset();
				fUpdateButton->SetValue(0);
			}
			break;

		case 'a':
		case 'A':
			fAboutButton->SetValue(1);
			snooze(100000);
			fAboutButton->SetValue(0);
			About();
			break;
			
		case 'u':
		case 'U':
		case B_RETURN:
			if (fPath >= 0) {
				fUpdateButton->SetValue(1);
				DeviceInquire(fPath);
				fUpdateButton->SetValue(0);
			}
			break;
	}
}

//--------------------------------------------------------------------

void TSCSIView::Draw(BRect where)
{
	BView::Draw(where);
	fFirstUpdateDone = true;
}

//--------------------------------------------------------------------

void TSCSIView::MessageReceived(BMessage *msg)
{
	int32		item;

	switch (msg->what) {
		case M_PATH:
			if (fFirstUpdateDone) {
				item = fItems[I_PATH]->CurrentSelection();
				if ((item >= 0) && (item != fPath)){
					fPath = item;
					DeviceInquire(fPath);
				}
			}
			else
				Window()->PostMessage(msg, this);
			break;
		case M_DEVICE:
			fDevice = fItems[I_DEVICE]->CurrentSelection();
			LUNInquire(fPath, fDevice);
			break;
		case M_LUN:
			fLUN = fItems[I_LUN]->CurrentSelection();
			break;

		case M_UPDATE:
			if (fPath >= 0) {
				if (fReset)
					Reset();
				else
					DeviceInquire(fPath);
			}
			break;

		case M_LED:
			break;

		case M_ABOUT:
			About();
			break;

		default:
			BView::MessageReceived(msg);
	}
	if ((fLUN < 0) || ((fDevice > 0) && (fSCSIData[fDevice].flags == 2)))
		fLEDButton->SetEnabled(false);
	else
		fLEDButton->SetEnabled(true);
}

//--------------------------------------------------------------------

void TSCSIView::Pulse(void)
{
	if (modifiers() & B_OPTION_KEY) {
		if (!fReset) {
			fReset = true;
			fUpdateButton->SetLabel(RESET_BUTTON_TEXT);
		}
	}
	else if (fReset) {
		fReset = false;
		fUpdateButton->SetLabel(UPDATE_BUTTON_TEXT);
	}
}

//--------------------------------------------------------------------

void TSCSIView::About(void)
{
	(new BAlert("", B_UTF8_ELLIPSIS"by Robert Polic", "OK"))->Go();
}

//--------------------------------------------------------------------

void TSCSIView::DeviceInquire(int32 path)
{
	uchar			type;
	int32			drvr;
	int32			loop;
	int32			old;
	scsi_data		data;

	old = fDevice;
	fDevice = -1;

	if ((drvr = open(DRIVER, 0)) < 0)
		return;
	for (loop = 0; loop < fPathData[path].devices; loop++) {
		if (loop != fPathData[path].host_id) {
			if (Inquire(drvr, path, loop, 0, &data)) {
				if (fDevice < 0)
					fDevice = loop;
				if (old == loop)
					fDevice = loop;
			}
		} else {
			data.flags = 2;
			strncpy(data.type, scsi_types[3], sizeof(data.type));
			strncpy(data.vendor, BE_VENDOR, sizeof(data.vendor));
			switch (fPlatform) {
				case B_BEBOX_PLATFORM:
					strncpy(data.product, BE_PRODUCT, sizeof(data.product));
					break;
				case B_MAC_PLATFORM:
					strncpy(data.product, MAC_PRODUCT, sizeof(data.product));
					break;
				case B_AT_CLONE_PLATFORM:
					strncpy(data.product, INTEL_PRODUCT, sizeof(data.product));
					break;
				default:
					sprintf(data.product, "UNKNOWN (%d)", fPlatform);
			}
			strncpy(data.version, fVersion.short_info, sizeof(data.version));
		}
		fItems[I_DEVICE]->UpdateItem(loop, &fSCSIData[loop], &data);
	}
	close(drvr);
	if (fDevice < 0)
		fDevice = fPathData[path].host_id;
	fItems[I_DEVICE]->SelectItem(fDevice);
	LUNInquire(path, fDevice);
}

//--------------------------------------------------------------------

void TSCSIView::FlashLED(flash_data *flash)
{
	int32				drvr;
	scsiprobe_inquiry	quiry;

	if (!fSCSIData[fDevice].flags)
		return;

	drvr = open(DRIVER, 0);
	if (drvr < 0)
		return;

	if (!flash->flag) {
		flash->device = fDevice;
		if (fSelected == I_PATH) {
			flash->lun = 0;
			flash->flag = I_DEVICE;
		}
		else if (fSelected == I_DEVICE) {
			flash->lun = 0;
			flash->flag = I_LUN;
		}
		else {
			flash->lun = fLUN;
			flash->flag = I_END;
		}
	}

	quiry.path = fPath;
	quiry.id = flash->device;
	quiry.lun = flash->lun;
	quiry.len = 5;
	ioctl(drvr, B_SCSIPROBE_INQUIRY, &quiry);
	close(drvr);

	if (flash->flag == I_DEVICE) {
		do {
			flash->device++;
			if (flash->device == fPathData[fPath].devices)
				flash->device = 0;
		} while (fSCSIData[flash->device].flags != 1);
	}
	else if (flash->flag == I_LUN) {
		flash->lun++;
		if (flash->lun == fItems[I_LUN]->CountItems())
			flash->lun = 0;
	}
}

//--------------------------------------------------------------------

bool TSCSIView::Inquire(int32 drvr, int32 path, int32 device, int32 lun, scsi_data *data)
{
	uchar				type;
	scsiprobe_inquiry	quiry;

	quiry.path = path;
	quiry.id = device;
	quiry.lun = lun;
	quiry.len = 5;
	if ((ioctl(drvr, B_SCSIPROBE_INQUIRY, &quiry) == B_NO_ERROR) &&
		((quiry.data[0] & 0xe0) != 0x60)) {
		data->flags = 1;
		type = quiry.data[0] & 0x1f;
		if (type > 9)
			sprintf(data->type, "%d", type);
		else
			strncpy(data->type, scsi_types[type], sizeof(data->type));
		if ((quiry.data[4] >= 31) && ((quiry.data[3] & 0xf) < 3)) {
			quiry.len = quiry.data[4] + 4 + 1;
			ioctl(drvr, B_SCSIPROBE_INQUIRY, &quiry);
			NiceString(&quiry.data[8], data->vendor, 8);
			NiceString(&quiry.data[16], data->product, 16);
			NiceString(&quiry.data[32], data->version, 4);
		}
		else {
			strcpy(data->vendor, "\xe2\x80\xa2");
			data->product[0] = 0;
			data->version[0] = 0;
		}
		return true;
	}
	else {
		data->flags = 0;
		data->type[0] = 0;
		data->vendor[0] = 0;
		data->product[0] = 0;
		data->version[0] = 0;
		return false;
	}
}

//--------------------------------------------------------------------

void TSCSIView::LUNInquire(int32 path, int32 device)
{
	uchar			type;
	int32			drvr;
	int32			loop;
	int32			old;
	scsi_data		data;

	old = fLUN;
	fLUN = -1;

	if (!fSCSIData[device].flags) {
		fItems[I_LUN]->SetItemCount(0);
		return;
	}
	if ((drvr = open(DRIVER, 0)) < 0)
		return;
	for (loop = 0; loop < 8; loop++) {
		if (Inquire(drvr, path, device, loop, &data)) {
			if (fLUN < 0)
				fLUN = loop;
			if (old == loop)
				fLUN = loop;
		}
		else {
			if (!loop)
				fItems[I_LUN]->SetItemCount(0);
			break;
		}
		fItems[I_LUN]->SetItemCount(loop + 1);
		fItems[I_LUN]->UpdateItem(loop, &fLUNData[loop], &data);
	}
	close(drvr);
	if (fLUN < 0)
		fLUN = 0;
	fItems[I_LUN]->SelectItem(fLUN);
}

//--------------------------------------------------------------------

void TSCSIView::NiceString(uchar *src, char *dest, int32 len)
{
	bool	start;
	int32	index;
	int32	loop;

	index = 0;
	start = false;
	// remove leading spaces
	for (loop = 0; loop < len; loop++) {
		if ((src[loop] > ' ') || ((src[loop] == ' ') && (start))) {
			dest[index++] = src[loop];
			start = true;
		}
	}
	// remove trailing spaces
	for (loop = index - 1; loop; --loop)
		if (dest[loop] != ' ') {
			dest[loop + 1] = 0;
			break;
		}
}

//--------------------------------------------------------------------

void TSCSIView::Reset(void)
{
	int				drvr;
	scsiprobe_reset	reset;

	drvr = open(DRIVER, 0);
	if (drvr < 0)
		return;
	reset.path = fPath;
	ioctl(drvr, B_SCSIPROBE_RESET, &reset);
	close(drvr);
	DeviceInquire(fPath);
}

//--------------------------------------------------------------------

void TSCSIView::Select(int32 item)
{
	if (fSelected == item)
		return;
	fItems[fSelected]->Select(false);
	fSelected = item;
	fItems[fSelected]->Select(true);
}

//--------------------------------------------------------------------

void TSCSIView::Update(bool all)
{
}


//====================================================================

TItem::TItem(BRect rect, char *title, int32 item, TSCSIView *parent)
	  :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	char			*label;
	BFont			font = be_bold_font;
	BRect			r;
	BRect			list;
	rgb_color		c;

	fSelected = false;
	fItem = item;
	fParent = parent;

	c.red = c.green = c.blue = VIEW_COLOR;
	SetViewColor(c);

	r = rect;
	r.OffsetTo(2, 0);
	r.right -= 2;
	switch (item) {
		case I_PATH:
			label = PATH_LABEL_TEXT;
			list.Set(PATH_LIST_X1, PATH_LIST_Y1, PATH_LIST_X2, PATH_LIST_Y2);
			break;
		case I_DEVICE:
			label = DEVICE_LABEL_TEXT;
			list.Set(DEVICE_LIST_X1, DEVICE_LIST_Y1, DEVICE_LIST_X2, DEVICE_LIST_Y2);
			break;
		case I_LUN:
			label = LUN_LABEL_TEXT;
			list.Set(LUN_LIST_X1, LUN_LIST_Y1, LUN_LIST_X2, LUN_LIST_Y2);
			break;
	}
	fBox = new BBox(r);
	font.SetSize(12.0);
	fBox->SetFont(&font);
	fBox->SetLabel(label);
	AddChild(fBox);

	list.right -= B_V_SCROLL_BAR_WIDTH;
	fList = new TList(list, item, fParent);
	fBox->AddChild(fScroll = new BScrollView("", fList, B_FOLLOW_ALL, B_WILL_DRAW,
										false, true, B_PLAIN_BORDER));

	font = *be_plain_font;
	font.SetSize(11.0);
	switch (item) {
		case I_PATH:
			r.Set(fOffsets.path_id, PATH_ID_Y1, fOffsets.path_sim, PATH_ID_Y2);
			fLabels[0] = new BStringView(r, "", PATH_ID_TEXT);
			fLabels[0]->SetFont(&font);
			fBox->AddChild(fLabels[0]);

			r.Set(fOffsets.path_sim, PATH_SIM_Y1, fOffsets.path_sim_vers, PATH_SIM_Y2);
			fLabels[1] = new BStringView(r, "", PATH_SIM_TEXT);
			fLabels[1]->SetFont(&font);
			fBox->AddChild(fLabels[1]);

			r.Set(fOffsets.path_sim_vers, PATH_SIM_VERS_Y1, fOffsets.path_hba, PATH_SIM_VERS_Y2);
			fLabels[2] = new BStringView(r, "", PATH_SIM_VERS_TEXT);
			fLabels[2]->SetFont(&font);
			fBox->AddChild(fLabels[2]);

			r.Set(fOffsets.path_hba, PATH_HBA_Y1, fOffsets.path_hba_vers, PATH_HBA_Y2);
			fLabels[3] = new BStringView(r, "", PATH_HBA_TEXT);
			fLabels[3]->SetFont(&font);
			fBox->AddChild(fLabels[3]);

			r.Set(fOffsets.path_hba_vers, PATH_HBA_VERS_Y1, fOffsets.path_family, PATH_HBA_VERS_Y2);
			fLabels[4] = new BStringView(r, "", PATH_HBA_VERS_TEXT);
			fLabels[4]->SetFont(&font);
			fBox->AddChild(fLabels[4]);

			r.Set(fOffsets.path_family, PATH_FAMILY_Y1, fOffsets.path_type, PATH_FAMILY_Y2);
			fLabels[5] = new BStringView(r, "", PATH_FAMILY_TEXT);
			fLabels[5]->SetFont(&font);
			fBox->AddChild(fLabels[5]);

			r.Set(fOffsets.path_type, PATH_TYPE_Y1, PATH_TYPE_X2, PATH_TYPE_Y2);
			fLabels[6] = new BStringView(r, "", PATH_TYPE_TEXT);
			fLabels[6]->SetFont(&font);
			fBox->AddChild(fLabels[6]);
			break;

		case I_DEVICE:
			r.Set(fOffsets.device_id, DEVICE_ID_Y1, fOffsets.device_type, DEVICE_ID_Y2);
			fLabels[0] = new BStringView(r, "", DEVICE_ID_TEXT);
			fLabels[0]->SetFont(&font);
			fBox->AddChild(fLabels[0]);

			r.Set(fOffsets.device_type, DEVICE_TYPE_Y1, fOffsets.device_vendor, DEVICE_TYPE_Y2);
			fLabels[1] = new BStringView(r, "", DEVICE_TYPE_TEXT);
			fLabels[1]->SetFont(&font);
			fBox->AddChild(fLabels[1]);

			r.Set(fOffsets.device_vendor, DEVICE_VENDOR_Y1, fOffsets.device_product, DEVICE_VENDOR_Y2);
			fLabels[2] = new BStringView(r, "", DEVICE_VENDOR_TEXT);
			fLabels[2]->SetFont(&font);
			fBox->AddChild(fLabels[2]);

			r.Set(fOffsets.device_product, DEVICE_PRODUCT_Y1, fOffsets.device_vers, DEVICE_PRODUCT_Y2);
			fLabels[3] = new BStringView(r, "", DEVICE_PRODUCT_TEXT);
			fLabels[3]->SetFont(&font);
			fBox->AddChild(fLabels[3]);

			r.Set(fOffsets.device_vers, DEVICE_VERS_Y1, DEVICE_VERS_X2, DEVICE_VERS_Y2);
			fLabels[4] = new BStringView(r, "", DEVICE_VERS_TEXT);
			fLabels[4]->SetFont(&font);
			fBox->AddChild(fLabels[4]);
			break;

		case I_LUN:
			r.Set(fOffsets.lun_id, LUN_ID_Y1, fOffsets.lun_type, LUN_ID_Y2);
			fLabels[0] = new BStringView(r, "", LUN_ID_TEXT);
			fLabels[0]->SetFont(&font);
			fBox->AddChild(fLabels[0]);

			r.Set(fOffsets.lun_type, LUN_TYPE_Y1, fOffsets.lun_vendor, LUN_TYPE_Y2);
			fLabels[1] = new BStringView(r, "", LUN_TYPE_TEXT);
			fLabels[1]->SetFont(&font);
			fBox->AddChild(fLabels[1]);

			r.Set(fOffsets.lun_vendor, LUN_VENDOR_Y1, fOffsets.lun_product, LUN_VENDOR_Y2);
			fLabels[2] = new BStringView(r, "", LUN_VENDOR_TEXT);
			fLabels[2]->SetFont(&font);
			fBox->AddChild(fLabels[2]);

			r.Set(fOffsets.lun_product, LUN_PRODUCT_Y1, fOffsets.lun_vers, LUN_PRODUCT_Y2);
			fLabels[3] = new BStringView(r, "", LUN_PRODUCT_TEXT);
			fLabels[3]->SetFont(&font);
			fBox->AddChild(fLabels[3]);

			r.Set(fOffsets.lun_vers, LUN_VERS_Y1, LUN_VERS_X2, LUN_VERS_Y2);
			fLabels[4] = new BStringView(r, "", LUN_VERS_TEXT);
			fLabels[4]->SetFont(&font);
			fBox->AddChild(fLabels[4]);
			break;
	}
}

//--------------------------------------------------------------------

void TItem::Draw(BRect where)
{
	BRect	r;

	if (fSelected) {
		fBox->SetHighColor(0, 0, 0);
		r = fScroll->Frame();
		r.left -= 3;
		r.top -= 3;
		r.right += 3;
		r.bottom += 3;
		fBox->StrokeRect(r);
		r.InsetBy(1, 1);
		fBox->StrokeRect(r);
	}
}


//--------------------------------------------------------------------

void TItem::KeyDown(const char *key, int32 count)
{
	char		c;

	c = key[0];
	switch(c) {
		case B_UP_ARROW:
			fList->Select(fList->CurrentSelection() - 1);
			break;
		case B_DOWN_ARROW:
			fList->Select(fList->CurrentSelection() + 1);
			break;
	}
}
//--------------------------------------------------------------------

void TItem::AddItem(void *data)
{
	fList->AddItem(new TListItem(data, fItem, fList->CountItems()));
}

//--------------------------------------------------------------------

int32 TItem::CountItems(void)
{
	return fList->CountItems();
}

//--------------------------------------------------------------------

int32 TItem::CurrentSelection(void)
{
	return fList->CurrentSelection();
}

//--------------------------------------------------------------------

void TItem::Select(bool selected)
{
	BRect	r;

	if (selected == fSelected)
		return;
	fSelected = selected;

	if (fSelected)
		fBox->SetHighColor(0, 0, 0);
	else
		fBox->SetHighColor(VIEW_COLOR, VIEW_COLOR, VIEW_COLOR);
	r = fScroll->Frame();
	r.left -= 3;
	r.top -= 3;
	r.right += 3;
	r.bottom += 3;
	fBox->StrokeRect(r);
	r.InsetBy(1, 1);
	fBox->StrokeRect(r);
	fBox->SetHighColor(0, 0, 0);
}

//--------------------------------------------------------------------

void TItem::SelectItem(int32 item)
{
	fList->Select(item);
}

//--------------------------------------------------------------------

void TItem::SetItemCount(int32 count)
{
	int32	items;
	int32	loop;

	items = fList->CountItems();
	if (items == count)
		return;
	if (items < count) {
		while (items < count) {
			fList->AddItem(new TListItem(NULL, fItem, fList->CountItems()));
			items++;
		}
	}
	else while (items > count) {
		items--;
		fList->RemoveItem(items);
	}
}

//--------------------------------------------------------------------

void TItem::UpdateItem(int32 item, scsi_data *dst, scsi_data *src)
{
	if (((TListItem*)fList->ItemAt(item))->UpdateItem(dst, src)) {
		fList->Draw(fList->ItemFrame(item));
		Sync();
	}
}

//--------------------------------------------------------------------

void TItem::SetSelectionMessage(BMessage *msg, BView *target)
{
	fList->SetSelectionMessage(msg);
	fList->SetTarget(this);
}


//====================================================================

TList::TList(BRect rect, int32 item, TSCSIView *parent)
	  :BListView(rect, "", B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL)
{
	BFont		font = be_plain_font;
	font_height	height;

	fItem = item;
	fParent = parent;
	font.SetSize(10.0);
	SetFont(&font);

	font.GetHeight(&height);
	fHeight = height.ascent + height.descent + height.leading;
}

//--------------------------------------------------------------------

void TList::Draw(BRect where)
{
	SetHighColor(255, 255, 255);
	FillRect(where);
	BListView::Draw(where);
}

//--------------------------------------------------------------------

void TList::MouseDown(BPoint where)
{
	int32	current;

	current = CurrentSelection();
	fParent->Select(fItem);
	BListView::MouseDown(where);
	if (CurrentSelection() < 0)
		Select(current);
	fParent->MakeFocus(true);
}


//====================================================================

TListItem::TListItem(void *data, int32 item, int32 num)
		 :BListItem()
{
	fData = data;
	fItem = item;
	fNum = num;
}

//--------------------------------------------------------------------

void TListItem::DrawItem(BView *view, BRect where, bool complete)
{
	char	str[16];
	BFont	font;

	view->GetFont(&font);
	if (IsSelected()) {
		view->SetHighColor(SELECT_COLOR, SELECT_COLOR, SELECT_COLOR);
		view->SetLowColor(SELECT_COLOR, SELECT_COLOR, SELECT_COLOR);
	}
	else {
		view->SetHighColor(255, 255, 255);
		view->SetLowColor(255, 255, 255);
	}
	view->FillRect(where);

	sprintf(str, "%d", fNum);
	view->SetHighColor(0, 0, 0);
	switch (fItem) {
		case I_PATH:
			view->MovePenTo(fOffsets.path_sim - PATH_LIST_X1 - font.StringWidth(str) - 2, where.bottom - 2);
			view->DrawString(str);

			view->MovePenTo(fOffsets.path_sim - PATH_LIST_X1 + 2, where.bottom - 2);
			FitString(view, &font, ((path_data *)fData)->sim_vendor,
									fOffsets.path_sim_vers - fOffsets.path_sim);

			view->MovePenTo(fOffsets.path_sim_vers - PATH_LIST_X1 + 2, where.bottom - 2);
			FitString(view, &font, ((path_data *)fData)->sim_vers,
									fOffsets.path_hba - fOffsets.path_sim_vers);

			view->MovePenTo(fOffsets.path_hba - PATH_LIST_X1 + 2, where.bottom - 2);
			FitString(view, &font, ((path_data *)fData)->hba_vendor,
									fOffsets.path_hba_vers - fOffsets.path_hba);

			view->MovePenTo(fOffsets.path_hba_vers - PATH_LIST_X1 + 2, where.bottom - 2);
			FitString(view, &font, ((path_data *)fData)->hba_vers,
									fOffsets.path_family - fOffsets.path_hba_vers);

			view->MovePenTo(fOffsets.path_family - PATH_LIST_X1 + 2, where.bottom - 2);
			FitString(view, &font, ((path_data *)fData)->family,
									fOffsets.path_type - fOffsets.path_family);

			view->MovePenTo(fOffsets.path_type - PATH_LIST_X1 + 2, where.bottom - 2);
			FitString(view, &font, ((path_data *)fData)->type,
									PATH_LIST_X2 - PATH_LIST_X1 - 2 - fOffsets.path_type);

			view->SetHighColor(LINE_COLOR, LINE_COLOR, LINE_COLOR);
			view->StrokeLine(BPoint(0, where.bottom),
							BPoint(PATH_LIST_X2 - PATH_LIST_X1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.path_sim - PATH_LIST_X1 - 1, where.top),
							BPoint(fOffsets.path_sim - PATH_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.path_sim_vers - PATH_LIST_X1 - 1, where.top),
							BPoint(fOffsets.path_sim_vers - PATH_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.path_hba - PATH_LIST_X1 - 1, where.top),
							BPoint(fOffsets.path_hba - PATH_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.path_hba_vers - PATH_LIST_X1 - 1, where.top),
							BPoint(fOffsets.path_hba_vers - PATH_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.path_family - PATH_LIST_X1 - 1, where.top),
							BPoint(fOffsets.path_family - PATH_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.path_type - PATH_LIST_X1 - 1, where.top),
							BPoint(fOffsets.path_type - PATH_LIST_X1 - 1, where.bottom));
			break;
		case I_DEVICE:
			if ((!fData) || ((fData) && (!((scsi_data *)fData)->flags)))
				view->SetHighColor(GRAY_COLOR, GRAY_COLOR, GRAY_COLOR);
			view->MovePenTo(fOffsets.device_type - DEVICE_LIST_X1 - font.StringWidth(str) - 2, where.bottom - 2);
			view->DrawString(str);

			if (fData) {
				view->MovePenTo(fOffsets.device_type - DEVICE_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->type,
										fOffsets.device_vendor - fOffsets.device_type);
	
				view->MovePenTo(fOffsets.device_vendor - DEVICE_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->vendor,
										fOffsets.device_product - fOffsets.device_vendor);
	
				view->MovePenTo(fOffsets.device_product - DEVICE_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->product,
										fOffsets.device_vers - fOffsets.device_product);
	
				view->MovePenTo(fOffsets.device_vers - DEVICE_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->version,
										DEVICE_LIST_X2 - DEVICE_LIST_X1 - 2 - fOffsets.device_vers);
			}

			view->SetHighColor(LINE_COLOR, LINE_COLOR, LINE_COLOR);
			view->StrokeLine(BPoint(0, where.bottom),
							BPoint(DEVICE_LIST_X2 - DEVICE_LIST_X1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.device_type - DEVICE_LIST_X1 - 1, where.top),
							BPoint(fOffsets.device_type - DEVICE_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.device_vendor - DEVICE_LIST_X1 - 1, where.top),
							BPoint(fOffsets.device_vendor - DEVICE_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.device_product - DEVICE_LIST_X1 - 1, where.top),
							BPoint(fOffsets.device_product - DEVICE_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.device_vers - DEVICE_LIST_X1 - 1, where.top),
							BPoint(fOffsets.device_vers - DEVICE_LIST_X1 - 1, where.bottom));
			break;
		case I_LUN:
			if (!fData)
				view->SetHighColor(GRAY_COLOR, GRAY_COLOR, GRAY_COLOR);
			view->MovePenTo(fOffsets.lun_type - LUN_LIST_X1 - font.StringWidth(str) - 2, where.bottom - 2);
			view->DrawString(str);

			if (fData) {
				view->MovePenTo(fOffsets.lun_type - LUN_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->type,
										fOffsets.lun_vendor - fOffsets.lun_type);
	
				view->MovePenTo(fOffsets.lun_vendor - LUN_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->vendor,
										fOffsets.lun_product - fOffsets.lun_vendor);
	
				view->MovePenTo(fOffsets.lun_product - LUN_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->product,
										fOffsets.lun_vers - fOffsets.lun_product);
	
				view->MovePenTo(fOffsets.lun_vers - LUN_LIST_X1 + 2, where.bottom - 2);
				FitString(view, &font, ((scsi_data *)fData)->version,
										LUN_LIST_X2 - LUN_LIST_X1 - 2 - fOffsets.lun_vers);
			}

			view->SetHighColor(LINE_COLOR, LINE_COLOR, LINE_COLOR);
			view->StrokeLine(BPoint(0, where.bottom),
							BPoint(LUN_LIST_X2 - LUN_LIST_X1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.lun_type - LUN_LIST_X1 - 1, where.top),
							BPoint(fOffsets.lun_type - LUN_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.lun_vendor - LUN_LIST_X1 - 1, where.top),
							BPoint(fOffsets.lun_vendor - LUN_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.lun_product - LUN_LIST_X1 - 1, where.top),
							BPoint(fOffsets.lun_product - LUN_LIST_X1 - 1, where.bottom));
			view->StrokeLine(BPoint(fOffsets.lun_vers - LUN_LIST_X1 - 1, where.top),
							BPoint(fOffsets.lun_vers - LUN_LIST_X1 - 1, where.bottom));
			break;
	}
}

//--------------------------------------------------------------------

void TListItem::FitString(BView *view, BFont *font, char *str, int32 width)
{
	char	*string;
	char	*src[1];
	char	*result[1];

	src[0] = str;
	string = (char *)malloc(strlen(str) + 16);
	result[0] = string;
	font->GetTruncatedStrings((const char **)src, 1, B_TRUNCATE_END, width - 4, result);
	
	view->DrawString(string);
	free(string);
}

//--------------------------------------------------------------------

bool TListItem::UpdateItem(scsi_data *dst, scsi_data *src)
{
	if (!fData)
		fData = dst;
	else {
		if ((dst->flags == src->flags) &&
			(!strcmp(dst->type, src->type)) &&
			(!strcmp(dst->vendor, src->vendor)) &&
			(!strcmp(dst->product, src->product)) &&
			(!strcmp(dst->version, src->version)))
		return false;
	}
	dst->flags = src->flags;
	strcpy(dst->type, src->type);
	strcpy(dst->vendor, src->vendor);
	strcpy(dst->product, src->product);
	strcpy(dst->version, src->version);
	return true;
}


//====================================================================

TButton::TButton(BRect rect, const char *name, const char *label,
				  int32 msg, TSCSIView *parent)
		:BButton(rect, name, label, new BMessage(msg))
{
	fMessage = msg;
	fLabel = (char *)malloc(strlen(label) + 1);
	strcpy(fLabel, label);
	fParent = parent;
	ResizeTo(BUTTON_WIDTH, BUTTON_HEIGHT);
}

//--------------------------------------------------------------------

TButton::~TButton(void)
{
	free(fLabel);
}

//--------------------------------------------------------------------

void TButton::MouseDown(BPoint where)
{
	bool		in = true;
	uint32		buttons;
	BPoint		point;
	flash_data	flash;

	if (!IsEnabled())
		return;

	if (fMessage == M_LED) {
		flash.flag = 0;
		SetValue(1);
		do {
			GetMouse(&point, &buttons);
			if (Bounds().Contains(point)) {
				if (!in) {
					in = true;
					SetValue(1);
				}
				fParent->FlashLED(&flash);
			}
			else if (in) {
				in = false;
				SetValue(0);
			}
			snooze(250000);
			GetMouse(&point, &buttons);
		} while (buttons);
		if (in)
			SetValue(0);
	}
	else
		BButton::MouseDown(where);
}

//--------------------------------------------------------------------

void TButton::Draw(BRect where)
{
	float		x;
	float		y;
	BFont		font;
	BRect		r;
	font_height	height;

	r = Bounds();
	r.right--;
	r.bottom--;
	if (!Value()) {
		SetHighColor(BUTTON_COLOR, BUTTON_COLOR, BUTTON_COLOR);
		FillRoundRect(r, 3, 3);
		SetHighColor(BUTTON_SHADOW, BUTTON_SHADOW, BUTTON_SHADOW);
		r.OffsetTo(1, 1);
		StrokeRect(r);
		SetHighColor(LINE_COLOR, LINE_COLOR, LINE_COLOR);
		r.OffsetTo(0, 0);
		StrokeRoundRect(r, 3, 3);
	
		SetLowColor(BUTTON_COLOR, BUTTON_COLOR, BUTTON_COLOR);
		if (IsEnabled())
			SetHighColor(0, 0, 0);
		else
			SetHighColor(LINE_COLOR, LINE_COLOR, LINE_COLOR);
	}
	else {
		SetHighColor(BUTTON_DOWN, BUTTON_DOWN, BUTTON_DOWN);
		FillRoundRect(r, 3, 3);
		SetHighColor(BUTTON_SHADOW_DOWN, BUTTON_SHADOW_DOWN, BUTTON_SHADOW_DOWN);
		StrokeLine(BPoint(r.left + 1, r.top + 1), BPoint(r.right - 2, r.top + 1));
		StrokeLine(BPoint(r.left + 1, r.top + 1), BPoint(r.left + 1, r.bottom - 2));
		SetHighColor(LINE_COLOR, LINE_COLOR, LINE_COLOR);
		r.OffsetTo(0, 0);
		StrokeRoundRect(r, 3, 3);

		SetLowColor(BUTTON_DOWN, BUTTON_DOWN, BUTTON_DOWN);
		SetHighColor(255, 255, 255);
	}
	GetFont(&font);
	font.GetHeight(&height);
	x = r.left + ((r.right - r.left - font.StringWidth(fLabel)) / 2) + 1;
	y = r.bottom - ((r.bottom - r.top - (height.ascent + height.descent +
			  				height.leading)) / 2) - 1;
	MovePenTo(x, y);
	DrawString(fLabel);
	if (IsEnabled()) {
		StrokeLine(BPoint(x, y + 1),
				   BPoint(x + font.StringWidth(&fLabel[0], 1) - 2, y + 1));
		if (Value())
			SetHighColor(BUTTON_DOWN, BUTTON_DOWN, BUTTON_DOWN);
		else
			SetHighColor(BUTTON_SHADOW, BUTTON_SHADOW, BUTTON_SHADOW);
		StrokeLine(BPoint(x, y + 2),
				   BPoint(x + font.StringWidth(&fLabel[0], 1) - 2, y + 2));
	}
}

//--------------------------------------------------------------------

void TButton::SetLabel(const char *label)
{
	free(fLabel);
	fLabel = (char *)malloc(strlen(label) + 1);
	strcpy(fLabel, label);
	BButton::SetLabel(label);
}


//====================================================================

static int32 MountThread(void*)
{
#if 0
	int32		ref;
	scsi_info	info;

	if ((ref = open("/dev/scsi", 0)) >= 0) {
		ioctl(ref, SCSI_SCAN_FOR_DEVICES, 0);
		info.mask = SCSI_DISK_MASK | SCSI_CD_MASK |
					SCSI_TAPE_MASK | SCSI_WORM_MASK;
		info.index = 0;
		while (ioctl(ref, SCSI_GET_IND_DEVICE, &info) == B_NO_ERROR) {
			mount_vol(info.name);
			info.index++;
		}
		close(ref);
	}
#endif
	return B_NO_ERROR;
}
