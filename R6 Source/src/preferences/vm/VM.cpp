//*********************************************************************
//	
//	VM.cpp
//
//	Written by: Robert Polic, rehacked by rudeboy
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//*********************************************************************

#ifndef VM_H
#include "VM.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <interface_misc.h>

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Screen.h>
#include <TextView.h>

//******************************************************************

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

//*********************************************************************

int main()
{	
	TVMApp	myApp;
	myApp.Run();
	return B_NO_ERROR;
}

//*********************************************************************

TVMApp::TVMApp(void)
	:BApplication("application/x-vnd.Be-MEM$")
{
	setgid(0);
	setuid(0);
	new TVMWindow();
}

TVMApp::~TVMApp(void)
{
}

void
TVMApp::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BApplication::MessageReceived(m);
			break;
	}
}

void
TVMApp::AboutRequested(void)
{
	(new BAlert("", "Virtual Memory configuration", "OK"))->Go();
}


//*********************************************************************

const int32 kWindowWidth = 269;
const int32 kWindowHeight = 172;

const int32 msg_swap_size = 'swap';

const int32 kMegabytes = 1024 * 1024;
const int32 kBuffer = 16 * kMegabytes;

const char* const kChangesMsg = "Changes will take effect on restart";

const char* const kSwapFileName = "swap";
const char* const kKernelSettingsDirName = "kernel/drivers";
const char* const kKernelSettingsFileName = "virtual_memory";
const char* const kKernelSwapStr = "swap_size ";
const char* const kKernelEnabledStr = "vm on\n";

TVMWindow::TVMWindow()
	:BWindow(BRect(0, 0, kWindowWidth, kWindowHeight), "VirtualMemory",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	//	retrieve the current vm settings and memory information
	GetVMSettings();

	BRect r(Bounds());
	r.InsetBy(-1, -1);
	fBG = new BBox(r);
	AddChild(fBG);	
	
	//	add the main view (slider, stringviews)
	r.Set(12, 12, fBG->Bounds().Width()-12, fBG->Bounds().Height()-45);
	fVMView = new TVMView(r, fPhysicalMemory, fSwapFileSize, fSwapMin, fSwapMax);	
	fBG->AddChild(fVMView);
	
	//	add the pre configured buttons
	r.top = fVMView->Frame().bottom + 1;
	r.bottom = Bounds().Height()+1;
	r.left = 1; r.right = Bounds().Width()+1;
	fBtnBar = new TButtonBar(r, true, true, BB_BORDER_NONE);
	fBG->AddChild(fBtnBar);
	
	//	set the revert button to an initial disabled state
	CanRevert(false);
		
	// 	check the state of creating/having a swap file
	//		if there is not enough space for a swap file, note the minimum requirements
	//			for one
	//		if there is 0 free disk space note that condition as well
	//		if the user has made a change, note if different from the actual swap size
	if (fCurrent.swap < fSwapMin || fSwapSpaceAvailable < fSwapMin) {
		fBtnBar->DisableControls();
		fVMView->HideControls();
		char error_msg[1024];
		int32 t = fSwapMin/kMegabytes;
		sprintf(error_msg,"The swap file could not be created.\n"
			"\nFor the system to create a minimum swap file size of %ld"
			" MB you will need %ld MB of free disk space.", t, t+16);

		r = fVMView->Bounds();
		r.InsetBy(10,20);
		BRect rv(r);
		rv.OffsetTo(0,0);
		BTextView* msg = new BTextView(r, "", rv,
			B_FOLLOW_NONE, B_WILL_DRAW);
		msg->SetText(error_msg);
		msg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		msg->MakeEditable(false);
		msg->MakeSelectable(false);
		fVMView->AddChild(msg);
	} else if (fSwapMin == fSwapMax) {
		fVMView->ShowAlert(true, "Free disk space to increase VM size.");		
		fBtnBar->DisableControls();
		fVMView->DisableControls();
	} else {
		fVMView->SetCurrentSwapSize(fCurrent.swap/kMegabytes);	
		fVMView->ShowAlert((fCurrent.swap != fSwapFileSize), kChangesMsg);
	}
	
	//	get the current prefs for this panel
	GetPrefs();
	SetPulseRate(500000);
	Show();
	
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
	AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_revert));
	AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_defaults));
}

TVMWindow::~TVMWindow()
{
}

void
TVMWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case B_ABOUT_REQUESTED:
			be_app->MessageReceived(msg);
			break;
			
		case msg_revert:
			Revert();
			break;

		case msg_defaults:
			SetDefaults();
			break;

		case msg_swap_size:
			{
			// 	message received from the slider
			//	new size has been requested update the panel
			// 	creates the kernel settings file, sets the swap size
			int32 swap_size=0;
			msg->FindInt32("be:value",&swap_size);
			fCurrent.swap = swap_size * kMegabytes;
			SetKernelSetting(fCurrent.swap);
		
			fVMView->ShowAlert(fCurrent.swap != fSwapFileSize, kChangesMsg);
			CanRevert(fCurrent.swap != fOriginal.swap);
			}
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
TVMWindow::QuitRequested(void)
{
	//	save off the window location
	SetPrefs();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
TVMWindow::GetPrefs()
{
	BPath path;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
		BPoint loc;
				
		path.Append ("VM_data");
		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
			loc.x = -1; loc.y = -1;
		
			lseek (ref, 0, SEEK_SET);
			read(ref, &loc, sizeof(BPoint));
			close(ref);
			
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
				MoveTo(loc);
				return;
			}			
		} else {
			// go back up 1 level
			find_directory (B_USER_SETTINGS_DIRECTORY, &path);
			BDirectory dir;
			dir.SetTo(path.Path());
			BFile prefsFile;
			dir.CreateFile("VM_data", &prefsFile);
		}
	}
	
	// 	if prefs dont yet exist, simply center the window
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
}

void
TVMWindow::SetPrefs()
{
	BPath path;
	BPoint loc = Frame().LeftTop();

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		
		path.Append ("VM_data");
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {

			lseek (ref, 0, SEEK_SET);
			write (ref, &loc, sizeof (BPoint));
			close(ref);
			
		}
	}
}

void
TVMWindow::SetDefaults()
{
	// 	this is not really a default state
	// 	instead this is the auto selection calculation made by the kernel
	//	kills the kernel setting file
	fCurrent.swap = fSwapMin;
	fVMView->SetCurrentSwapSize(fCurrent.swap/kMegabytes);
	fVMView->ShowAlert(fCurrent.swap != fSwapFileSize, kChangesMsg);
	CanRevert(fCurrent.swap != fOriginal.swap);

	KillKernelSettingsFile();
}

//	sets the swap size requested to the original size requested
//	as found upon launch of panel
void
TVMWindow::Revert()
{
	if (fOriginal.swap != fCurrent.swap) {
		fCurrent.swap = fOriginal.swap;
		fVMView->SetCurrentSwapSize(fOriginal.swap/kMegabytes);
	}

	fVMView->ShowAlert(fCurrent.swap != fSwapFileSize, kChangesMsg);
	CanRevert(false);
}

//	tell the button bar to set the buttons enabled states
void
TVMWindow::CanRevert(bool state)
{
	fBtnBar->CanRevert(state);
}

void
TVMWindow::CanDefault(bool state)
{
	fBtnBar->CanDefault(state);
}

// 	returns swap file size based on physical RAM, in bytes
// 	from Cyril (nukernel/vm/vm_stu.c
static int64
SwapFileSize(int64 memsize)
{
	static int		swsz[][2] = {
		{ 0, 16 },
		{ 16, 48 },
		{ 32, 80 },
		{ 64, 128 },
		{ 128, 192 },
		{ 512, 640 },
		{ 4096, 4096 }
	};

	int64 mb = memsize >> 20;
	off_t heuristic = 0;
	for(int i=0; ; i++) {
		if (mb <= swsz[i][0]) {
			heuristic = (off_t)((float)(mb - swsz[i-1][0]) /
					(float)(swsz[i][0] - swsz[i-1][0]) *
					(float)(swsz[i][1] - swsz[i-1][1]) + swsz[i-1][1]);
			heuristic <<= 20;
			break;
		}
	}
	
	return heuristic;
}

static bool
SwapFileExists(uint64* size)
{
	BPath		path;
	
	*size = 0;
	
	if (find_directory(B_COMMON_VAR_DIRECTORY, &path) == B_OK) {
		BDirectory	dir;
		BEntry		swap;

		dir.SetTo(path.Path());
		dir.FindEntry(kSwapFileName, &swap);
		if (swap.Exists()) {
			struct stat	stats;
			
			swap.GetStat(&stats);
			*size = stats.st_size;
			
			return true;
		}
	}
	
	return false;
}

void
TVMWindow::GetVMSettings()
{
	system_info	info;

	// physical RAM available
	get_system_info(&info);
	fPhysicalMemory = B_PAGE_SIZE * info.max_pages;

	// free disk space on boot volume
	//	must have > 16 MB free on disk
	BVolume			vol;
	BVolumeRoster	volume;
	volume.GetBootVolume(&vol);
	fSwapSpaceAvailable = vol.FreeBytes() - kBuffer;
	
	//	default swap file size is the minimum based on the physical
	fSwapMin = SwapFileSize(fPhysicalMemory);
	int64 requestedSwapFileSize = fSwapMin; 

	if (SwapFileExists(&fSwapFileSize))	{
		fSwapMax = ((fSwapFileSize + fSwapSpaceAvailable) < fSwapMin) ? fSwapMin :
			(fSwapFileSize + fSwapSpaceAvailable);
// 		requestedSwapFileSize = fSwapFileSize;
		fSwapSpaceAvailable = fSwapMax;
	} else {
		fSwapMax = (fSwapSpaceAvailable < fSwapMin) ? fSwapMin : fSwapSpaceAvailable;
		//	if no swap file then assume size is auto size, from above
		//  swap space available is free space on disk
	}

	// 	if the kernel settings file exists
	//	this setting will override the current setting
	//	if the file does not exist KernelSetting will return 0
	//	if it does exist, the value will only be used if it is
	//	within the calculated min and max swap file sizes
	uint64 customSize = KernelSetting();
	if ((customSize >= fSwapMin) && (customSize <= fSwapMax)) {
		requestedSwapFileSize = customSize;
	}
	// 	else
	//		swap size will equal current swap file size or
	//		will equal auto size

	// the current requested swap file size
	fCurrent.swap = fOriginal.swap = requestedSwapFileSize;
}

//	looks for the common settings directory, creates it if not there
//	looks for the kernel_settings file, creates it if it does not exist (and param is true)
status_t
TVMWindow::KernelSettingsFile(BEntry* entry, bool create)
{
	status_t 	err;
	BPath 		path;

	err = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true);
	if (err == B_OK) {
		BDirectory 	dir;
		char npath[B_PATH_NAME_LENGTH];

		sprintf(npath, "%s/%s", path.Path(), kKernelSettingsDirName);
		create_directory(npath, 0777);

		dir.SetTo(npath);

		err = dir.FindEntry(kKernelSettingsFileName, entry);
		
		// didn't exist, request to create it
		if (create && err != B_OK) {
			BFile file;
			dir.CreateFile(kKernelSettingsFileName, &file);
			file.Write(kKernelEnabledStr, strlen(kKernelEnabledStr));
			dir.FindEntry(kKernelSettingsFileName, entry);
		}
		
		err = entry->InitCheck();
	}
	
	return err;
}

// updates the kernel_settings file with the requested swap file size
static char*
update_field(char *buffer, const char *label, char *value)
{
	char	*offset;
	short	buff_len;
	short	diff;
	short	len = 0;

	offset = strstr(buffer, label);
	buff_len = strlen(buffer);
	if (offset) {
		short index = (offset - buffer) + strlen(label);
		while (buffer[index + len] != '\n') {
			len++;
		}
		diff = strlen(value) - len;
		if (diff) {
			if (diff > 0)
				buffer = (char *)realloc(buffer, buff_len + 1 + diff);
			memmove(&buffer[index] + len + diff, &buffer[index] + len,
					buff_len - (index + len) + 1);
			if (diff < 0)
				buffer = (char *)realloc(buffer, buff_len + 1 + diff);
		}
		memcpy(&buffer[index], value, strlen(value));
	}
	else {
		len = strlen(label) + strlen(value) + 1;
		buffer = (char *)realloc(buffer, buff_len + 1 + len);
		len = buff_len;
		diff = strlen(label);
		memcpy(&buffer[len], label, diff);
		len += diff;
		diff = strlen(value);
		memcpy(&buffer[len], value, diff);
		buffer[len + diff] = '\n';
		buffer[len + diff + 1] = 0;
	}
	return buffer;
}

//	set the value in the kernel settings file
//	result of the user moving the slider
//	creates a file if one does not exist
void
TVMWindow::SetKernelSetting(int64 vm_size)
{
	BEntry entry;
	
	// find the kernel settings file, create if necessary
	if (KernelSettingsFile(&entry, true) == B_OK) {
		char		str[256];
		char		*settings=NULL;
		size_t		len;
		struct stat	stats;
		BFile		file(&entry, O_RDWR);

		file.GetStat(&stats);
		settings = (char *)malloc(stats.st_size + 1);
		settings[stats.st_size] = 0;
		file.Seek(0, 0);
		len = file.Read(settings, stats.st_size);
		
		sprintf(str, "%Ld", vm_size);
			
		settings = update_field(settings, kKernelSwapStr, str);
		file.Seek(0, 0);
		file.Write(settings, strlen(settings));
		file.SetSize(strlen(settings));
		free(settings);
	}
}

//	any time the user clicks the default button,
//	the kernel settings file will be deleted
//	the kernel will then use the heuristic for setting the swap size
void
TVMWindow::KillKernelSettingsFile()
{
	BEntry entry;
	
	if (KernelSettingsFile(&entry) != B_OK)
		return;
			
	entry.Remove();
}

//	get the current setting in the kernel settings file
//	returns 0 if file does not exist					
int64
TVMWindow::KernelSetting()
{
	int64 	vm_size=0;
	BEntry	entry;
	
	if (KernelSettingsFile(&entry) != B_OK)
		return vm_size;
		
	BFile file(&entry, O_RDWR);
	if (file.InitCheck() == B_NO_ERROR) {
		char		str[256];
		char		*settings=NULL;
		size_t		len;
		struct stat	stats;

		file.GetStat(&stats);
		settings = (char *)malloc(stats.st_size + 1);
		settings[stats.st_size] = 0;
		len = file.Read(settings, stats.st_size);
		if (len) {
			char	*offset=NULL;
			
			offset = strstr(settings, kKernelSwapStr);
			if (offset) {
				short index = 0;

				offset += strlen(kKernelSwapStr);
				while (*offset != '\n') {
					str[index++] = *offset++;
				}
				str[index] = 0;
				vm_size = strtoull(str, NULL, 0);
			}
		}
		free(settings);
	}

	return vm_size;
}

//*********************************************************************

const char* const kPhysicalMemStr = "Physical memory:";
const char* const kActualSizeStr = "Current swap file size:";
const char*	const kRequestedSizeStr = "Requested swap file size:";

uint _randseed;

static void
makerandom()
{
	time_t	t;
	time( &t);	
	_randseed = t<<6 | getpid( );
}

static int
randnum( )
{
	uint r=0,i;

	for (i=0; i<64; ++i) {
		_randseed = _randseed*0x41C64E6D + 12345;
		r >>= 1;
		r |= _randseed & 1<<32-1;
	}
	return r;
}

const int32 kExclamationMarkWidth = 12;
const int32 kExclamationMarkHeight = 11;

const unsigned char kExclamationMark [] = {
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0xfa,0xfa,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0xfa,0xfa,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0xfa,0x00,0x00,0xfa,0x00,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0xfa,0x00,0x00,0xfa,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0xfa,0xfa,0x00,0x00,0xfa,0xfa,0x00,0xff,0xff,
	0xff,0xff,0x00,0xfa,0xfa,0x00,0x00,0xfa,0xfa,0x00,0xff,0xff,
	0xff,0x00,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0x00,0xff,
	0xff,0x00,0xfa,0xfa,0xfa,0x00,0x00,0xfa,0xfa,0xfa,0x00,0xff,
	0x00,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

TVMView::TVMView(BRect frame, uint32 physicalMemory, int64 actualSize,
	int64 swapMin, int64 swapMax)
		:BBox(frame, "", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED)
{
	char memStr[64];
	
	SetFont(be_plain_font);
	
	int32 w = (int32)StringWidth("0000 MB") + 5;
	float h = FontHeight(this, true);
	
	//	Physical Mem
	BRect r(10, 11, Bounds().Width()-10-w, 11+h);	
	char mag = 'K';
	int32 size = physicalMemory / 1024;
	if (size > 8096) {
		size /= 1024;
		mag = 'M';
	}
	sprintf(memStr, "%s %ld %cB", kPhysicalMemStr, size, mag);
	fPhysicalMemFld = new BStringView(r, "physical mem", memStr);
	AddChild(fPhysicalMemFld);
		
	//	Actual swap file size
	r.top = r.bottom + 5; r.bottom = r.top + h;
	size = actualSize/1024/1024;
	sprintf(memStr, "%s %ld MB", kActualSizeStr, size);
	fActualSizeFld = new BStringView(r, "", memStr);
	AddChild(fActualSizeFld);
	
	int32 min = swapMin / kMegabytes;
	int32 max = swapMax / kMegabytes;
	makerandom();
	when = (randnum() % min);
	when = (when < 0) ? max + when : when;
	when = (when < min) ? min + when : when;
	errorMsg[0] = 0;
	r.top = r.bottom+2;  r.bottom = r.top + 16;
	r.right = Bounds().Width() - 10;
	fSlider = new TSlider(r, "slider", "", new BMessage(msg_swap_size),
		min, max);
	AddChild(fSlider);
	fSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);

	char minstr[8], maxstr[8];
	sprintf(minstr,"%ld MB", min);
	sprintf(maxstr,"%ld MB", max);
	fSlider->SetLimitLabels(minstr,maxstr);
	rgb_color c;
	c.green = c.red = 102; c.blue = 152;
	fSlider->UseFillColor(true, &c);
	
	//	alert message fld
	r.top = fSlider->Frame().bottom + 5;
	r.bottom = r.top + h;
	r.left += 36;
	fMsgFld = new BStringView(r, "msg", "a message");
	AddChild(fMsgFld);	
	
	fExclamationMark = new BBitmap(BRect(0, 0, kExclamationMarkWidth-1, kExclamationMarkHeight-1), B_COLOR_8_BIT);
	fExclamationMark->SetBits(kExclamationMark, (kExclamationMarkWidth*kExclamationMarkHeight), 0, B_COLOR_8_BIT);
	fShowAlert = false;
}

TVMView::~TVMView()
{
	delete fExclamationMark;
}

void
TVMView::Draw(BRect where)
{
	BBox::Draw(where);
	
	PushState();

	BRect exRect(	28, 						Bounds().Height()-12-kExclamationMarkHeight+1,
					28+kExclamationMarkWidth-1, Bounds().Height()-12);
	if (fShowAlert) {
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fExclamationMark, exRect);
	} else {
		//	blow away the triangle
		SetHighColor(ViewColor());
		FillRect(exRect);
	}
	
	PopState();
}

void
TVMView::Pulse()
{
	BView::Pulse();

	if(when == -1 && ptime < time(0) && strlen(errorMsg) > 0) {
		fMsgFld->SetText(errorMsg);
		errorMsg[0] = 0;
	}
}

const char* const filePath = "/boot/beos/etc/fortunes/one-liners";

static char*
GetLine(int32 v)
{
	char* line = (char*)malloc(1024);
	
	FILE* ff = fopen( filePath, "r");
	if (!ff) return NULL;
	
	struct stat s;
	stat(filePath, &s);
	uint n = s.st_size;	
	if (n / sizeof(uint)) {
		n = randnum() % n % v;

		int linecount=0;
		while(fgets(line, 1024, ff) != NULL) {
			if (line[0] == '%')
				continue;
			linecount++;
			if (linecount >= (int)n)
				break;				
		}
	}
	fclose(ff);
	return line;
}

//	changes the alert state and message
void
TVMView::ShowAlert(bool state, const char* msg)
{
	if (!state)
		//	state is false so clear the text
		//	if the state changed an invalidate will be called
		//	and the triangle will disappear
		fMsgFld->SetText("");
	else {
		if (when != -1 && fSlider->Value() == when) {
			char* line = GetLine(fSlider->Value());
			if (line){
				fMsgFld->SetText(line);
				if (line) free(line);
				ptime = time(0) + 1;
				strcpy(errorMsg, msg);
			} else {
				errorMsg[0] = 0;
				fMsgFld->SetText(msg);
			}
			when = -1;
		} else if (msg && strcmp(fMsgFld->Text(), msg) != 0)
			fMsgFld->SetText(msg);
	}		
		
	if (state != fShowAlert) {
		fShowAlert = state;
		Draw(Bounds());
	}	
}

void
TVMView::SetCurrentSwapSize(int32 s)
{
	fSlider->SetAndInvoke(s);
}

void
TVMView::DisableControls()
{
	fSlider->SetEnabled(false);
}

void
TVMView::HideControls()
{
	fPhysicalMemFld->Hide();
	fActualSizeFld->Hide();
	fSlider->Hide();
	fMsgFld->Hide();
}

//*********************************************************************

TSlider::TSlider(BRect frame, const char *name, const char *label,
	BMessage *message, int32 minValue, int32 maxValue)
	: BSlider(frame, name, label, message, minValue, maxValue, B_TRIANGLE_THUMB)
{
}

TSlider::~TSlider()
{
}

void
TSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();
}

const int32	kYGap = 4;
const int32 kHashHeight = 6;
const rgb_color kBlack = {0, 0, 0, 255 };
void
TSlider::DrawText()
{
	BView* osView = OffscreenView();
	
	rgb_color textcolor = (IsEnabled()) ? kBlack : shift_color(kBlack, 0.5);
	
	osView->SetHighColor(textcolor);
	osView->SetLowColor(ViewColor());

	font_height finfo;
	osView->GetFontHeight(&finfo);
	float textHeight = ceil(finfo.ascent + finfo.descent + finfo.leading);

	char str[64];
	sprintf(str, "%s %ld MB", kRequestedSizeStr, Value());
	float xoffset=0,offsetFromTop=0;
	if (Label()) {
		offsetFromTop = textHeight;
		osView->MovePenTo(2,offsetFromTop);
		osView->DrawString(str);
		offsetFromTop += kYGap;
	}
	
	offsetFromTop += kHashHeight;
	offsetFromTop += BarFrame().Height();
	offsetFromTop += kHashHeight;
		
	if (MinLimitLabel() && MaxLimitLabel()) {
		textHeight = ceil(finfo.ascent + finfo.descent);
		offsetFromTop += textHeight;
		osView->MovePenTo(2,offsetFromTop);
		osView->DrawString(MinLimitLabel());		

		xoffset = osView->StringWidth(MaxLimitLabel());
		osView->MovePenTo(Bounds().Width()-xoffset-2,offsetFromTop);
		osView->DrawString(MaxLimitLabel());		
	}
}

//	modified the arrow keys so that
//	left/right move the slider by 1
//	up/down move the slider by 16
void 
TSlider::KeyDown(const char *bytes, int32 n)
{
	if (!IsEnabled() || IsHidden())
		return;
	switch (bytes[0]) {
		case B_DOWN_ARROW:
			SetAndInvoke(Value() - 16);
			break;
		case B_LEFT_ARROW:
			SetAndInvoke(Value() - 1);
			break;
		case B_UP_ARROW:
			SetAndInvoke(Value() + 16);
			break;
		case B_RIGHT_ARROW:
			SetAndInvoke(Value() + 1);
			break;
		default:
			BSlider::KeyDown(bytes, n);
			break;
	}
}

void
TSlider::SetAndInvoke(int32 v)
{
	SetValue(v);
	Invoke(Message());
}

//*********************************************************************

const rgb_color kWhite = { 255, 255, 255, 255};

const int32 kButtonXLoc = 10;

//	this is a simple generic object that draws buttons and lines
//	based on UI conformance rules
TButtonBar::TButtonBar(BRect frame, bool defaultsBtn, bool revertBtn,
	bb_border_type borderType)
	: BView(frame, "button bar", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW),
	fBorderType(borderType),
	fHasDefaultsBtn(defaultsBtn), fHasRevertBtn(revertBtn),
	fHasOtherBtn(false)
{
	BRect r;
	
	r.bottom = Bounds().Height()-11;
	r.top = r.bottom - (FontHeight(this, true) + 11);
	r.left = kButtonXLoc;
	if (fHasDefaultsBtn) {
		r.right = r.left + 75;
		fDefaultsBtn = new BButton(r, "defaults", "Default",
			new BMessage(msg_defaults));
		AddChild(fDefaultsBtn);
	} else
		fDefaultsBtn=NULL;
	
	if (fHasRevertBtn) {
		if (fHasDefaultsBtn)
			r.left = fDefaultsBtn->Frame().right + 10;
		else
			r.left = kButtonXLoc;
		r.right = r.left + 75;
		fRevertBtn = new BButton(r, "revert", "Revert",
			new BMessage(msg_revert));
		AddChild(fRevertBtn);
	} else
		fRevertBtn=NULL;
		
	fOtherBtn=NULL;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


TButtonBar::~TButtonBar()
{
}

void TButtonBar::Draw(BRect)
{
	PushState();
	if (fHasOtherBtn) {
		BPoint top, bottom;
		
		top.x = fOtherBtn->Frame().right + 10;
		top.y = fOtherBtn->Frame().top;
		bottom.x = top.x;
		bottom.y = fOtherBtn->Frame().bottom;
		
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
		SetLowColor(ViewColor());
		StrokeLine(top, bottom);
		SetHighColor(kWhite);
		top.x++;
		bottom.x++;
		StrokeLine(top, bottom);
	}
	PopState();
}

void 
TButtonBar::AddButton(const char* title, BMessage* m)
{
	BRect r;

	r.bottom = Bounds().Height() - 10;
	r.top = r.bottom - (FontHeight(this, true) + 10);
	r.left = kButtonXLoc;
	r.right = r.left + StringWidth(title) + 20;
	fOtherBtn = new BButton(r, "other", title, m);
	
	int32 w = 22 + (int32)fOtherBtn->Bounds().Width();
	if (fHasDefaultsBtn) {
		RemoveChild(fDefaultsBtn);
		if (fHasRevertBtn)
			RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fDefaultsBtn)*/;
		AddChild(fDefaultsBtn);
		fDefaultsBtn->MoveBy(w, 0);
		if (fHasRevertBtn) {
			AddChild(fRevertBtn);
			fRevertBtn->MoveBy(w,0);
		}
	} else if (fHasRevertBtn) {
		RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fRevertBtn)*/;
		AddChild(fRevertBtn);
		fRevertBtn->MoveBy(w,0);
	} else
		AddChild(fOtherBtn);
		
	fHasOtherBtn = (fOtherBtn != NULL);
}

void
TButtonBar::CanRevert(bool state)
{
	fRevertBtn->SetEnabled(state);
}

void
TButtonBar::CanDefault(bool state)
{
	fDefaultsBtn->SetEnabled(state);
}

void
TButtonBar::DisableControls()
{
	fRevertBtn->SetEnabled(false);
	fDefaultsBtn->SetEnabled(false);
}
