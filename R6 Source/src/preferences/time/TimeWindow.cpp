#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <priv_syscalls.h>

#include <Application.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <Window.h>

#include "color_defs.h"
#include "dt_utils.h"
#include "TimeWindow.h"
#include "DateTime.h"

const char* const kWindowTitle = "Time & Date";
const char* const kTimeSettingsFilename = "Time_settings";

const uint32 msg_update_day = 'uday';
const uint32 msg_set_time = 'time';
const uint32 msg_set_sdate = 'dats';
const uint32 msg_set_local_time = 'locl';
const uint32 msg_set_gmt_time = 'gmt ';
const uint32 msg_show_config = 'cnfg';

// ************************************************************************** //

void
write_rtc_setting(bool is_local)
{
	char setting_file[MAXPATHLEN+1];
	int fd;

	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, setting_file,
		MAXPATHLEN);
	strcat(setting_file, "/" RTC_SETTINGS_FILE);

	fd = open(setting_file, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		return; // XXX make settings directory? What to do here.
	}
	
	write(fd, (is_local ? "local\n": "gmt  \n"), 6);
	close(fd);
	
	_kset_tzfilename_(NULL, 0, !is_local);
}

bool
get_rtc_setting(void)
{
	char setting_file[MAXPATHLEN+1];
	char c = 0;
	int fd;

	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, setting_file,
		MAXPATHLEN);
	strcat(setting_file, "/" RTC_SETTINGS_FILE);

	fd = open(setting_file, O_RDONLY);
	if (fd < 0) {
		write_rtc_setting(true);	
		return true;
	}
	
	read(fd, &c, 1);
	close(fd);
	if (c == 'g')
		return false;
	else
		return true;
}

// **************************************************************************** //

const int32 kMonthCount = 12;
const char* const kMonthList[] = {
	"January", "February","March","April","May","June", \
	"July","August","September","October","November","December"
};

TDateSelector::TDateSelector(BRect frame)
	: TListSelector(frame, "", NULL)
{
	fKeyBuffer[0] = fKeyBuffer[1] = fKeyBuffer[2] = 0;
}

TDateSelector::~TDateSelector()
{
}

void 
TDateSelector::KeyDown(const char *bytes, int32 numBytes)
{
	if (!HandleKeyDown())
		return;
	
	//	do a simple type ahead
	short tlist = TargetList();
	if ( tlist == 0  && isalpha(bytes[0])) {		//	month
		if (fLastTime > system_time()) {
			short len = strlen(fKeyBuffer);
			if (len < 9) {
				fKeyBuffer[len] = tolower(bytes[0]);
				fKeyBuffer[len+1] = 0;
			}
		} else {	// start over
			fKeyBuffer[0] = toupper(bytes[0]);
			fKeyBuffer[1] = 0;
		}

		SetDate(fKeyBuffer);
		fLastTime = system_time() + 500000;
		return;
	} else if (tlist == 1 && isdigit(bytes[0])){	// 	day
		if (fLastTime > system_time()) {
			fKeyBuffer[0] = fKeyBuffer[1];			
			fKeyBuffer[1] = bytes[0];
		} else {	// start over
			fKeyBuffer[0] = '0';
			fKeyBuffer[1] = bytes[0];
		}
		fKeyBuffer[2] = 0;

		SetDate(fKeyBuffer);
		fLastTime = system_time() + 500000;
		return;
	} else if (tlist == 2 && isdigit(bytes[0])) {	// 	year
		if (fLastTime > system_time()) {
			fKeyBuffer[2] = fKeyBuffer[3];			
			fKeyBuffer[3] = bytes[0];
		} else {
			strcpy(fKeyBuffer, SelectedText());
			//add the first key
			fKeyBuffer[3] = bytes[0];
		}
		fKeyBuffer[4] = 0;

		SetDate(fKeyBuffer);
		fLastTime = system_time() + 500000;
		return;
	}


	TListSelector::KeyDown(bytes, numBytes);
}
	
void
TDateSelector::MouseDown(BPoint loc)
{
	Window()->PostMessage('date', Parent());
	TListSelector::MouseDown(loc);
}

void
TDateSelector::SetTargetList(short s)
{
	fLastTime = 0;
	fKeyBuffer[0] = 0;
	TListSelector::SetTargetList(s);
}

void
TDateSelector::NextTargetList()
{
	fLastTime = 0;
	fKeyBuffer[0] = 0;
	TListSelector::NextTargetList();
}

void
TDateSelector::PreviousTargetList()
{
	fLastTime = 0;
	fKeyBuffer[0] = 0;
	TListSelector::PreviousTargetList();
}

void
TDateSelector::SetDate(const char* date)
{
	if (TargetList() == 2 || TargetList() == 1) {
		SetSelection(atoi(date));
	} else {
		bool done = false, found=false;
		char month[10];
		short i=-1;
		short len=strlen(date);
		while (!done) {
			strncpy(month, date, len);
			for (i=0 ; i<kMonthCount ; i++) {
				if (strncmp(kMonthList[i], month, len) == 0) {
					found = true;
					done = true;
					break;
				}
			}
			len--;
			if (len == 0)
				done = true;
		}
		
		if (found)
			SetSelection(i);
	}
	Invoke();		
}

// **************************************************************************** //

TTimeSelector::TTimeSelector(BRect frame)
	: TListSelector(frame, "", NULL)
{
	fKeyBuffer[0] = fKeyBuffer[1] = fKeyBuffer[2] = 0;
	fLastTime = 0;
}

TTimeSelector::~TTimeSelector()
{
}

void 
TTimeSelector::KeyDown(const char *bytes, int32 numBytes)
{
	if (!HandleKeyDown())
		return;
	
	//	do a simple type ahead
	if ( (TargetList() >= 0 && TargetList() <= 2 && isdigit(bytes[0])) ) {
		if (fLastTime < system_time()) {
			//add the first key
			fKeyBuffer[0] = '0';
			fKeyBuffer[1] = bytes[0];
		} else {
			fKeyBuffer[0] = fKeyBuffer[1];			
			fKeyBuffer[1] = bytes[0];
		}
		
		fKeyBuffer[2] = 0;
		SetTime(fKeyBuffer);
		fLastTime = system_time() + 500000;
		return;
	} else if ( TargetList() == 3
				&& (bytes[0] == 'A' || bytes[0] == 'a'
				|| bytes[0] == 'P' || bytes[0] == 'p')) {

		fKeyBuffer[0] = toupper(bytes[0]);
		fKeyBuffer[1] = 'M';
		fKeyBuffer[2] = 0;

		SetTime(fKeyBuffer);
		return;
	}


	TListSelector::KeyDown(bytes, numBytes);
}
	
void
TTimeSelector::MouseDown(BPoint loc)
{
	Window()->PostMessage('time', Parent());
	TListSelector::MouseDown(loc);
}

void
TTimeSelector::SetTargetList(short s)
{
	fLastTime = 0;
	fKeyBuffer[0] = 0;
	TListSelector::SetTargetList(s);
}

void
TTimeSelector::NextTargetList()
{
	fLastTime = 0;
	fKeyBuffer[0] = 0;
	TListSelector::NextTargetList();
}

void
TTimeSelector::PreviousTargetList()
{
	fLastTime = 0;
	fKeyBuffer[0] = 0;
	TListSelector::PreviousTargetList();
}

void
TTimeSelector::SetTime(const char* time)
{
	if (TargetList() == 3) {
		if (strcmp("AM", time) == 0)
			SetSelection(0);
		else
			SetSelection(1);
	} else {
		int32 num = atoi(time);
		if (TargetList() == 0)
			num--;					//	hour is 1-12 not 0 based
		SetSelection(num);
	}
	Invoke();		
}

// **************************************************************************** //

TDTView::TDTView(BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED)
{
//	SetFont(be_plain_font);
	
	fFirstTime = false;
	
	BRect r(10, 12, Bounds().Width()/2 - 14, 30);
	fDateSelector = new TDateSelector( r );

	// build and add  lists for month / day / year
	TSelectionList* list = new TSelectionList(NULL, new BMessage('mont'));
	for (int32 i=0 ; i<kMonthCount ; i++)
		list->AddItem(kMonthList[i]);	
	list->SetDelimiter("/");
	fDateSelector->AddList(list);
	fDateSelector->AddList(1, 31, "/", new BMessage('day '));	
	fDateSelector->AddList(1965, 2010, NULL, new BMessage('year'));

	fDateSelector->SetAlignment(B_ALIGN_RIGHT, 0);
	fDateSelector->SetAlignment(B_ALIGN_CENTER, 1);
	fDateSelector->SetAlignment(B_ALIGN_CENTER, 2);
	
	fDateSelector->SetTargetList(0);
	AddChild(fDateSelector);

	BRect d_rect(fDateSelector->Frame());	
	BRect b_rect(fDateSelector->BevelFrame());

	time_t curr_time = time(NULL);
	short start_day, day_count;
	_MonthInfo( &curr_time, &start_day, &day_count);
	
	int32 day_width = ((int32)b_rect.Width() - (2 * 6)) / 7;
	int32 day_height = day_width;

	fCalendar = new TCalendar(
		BPoint(d_rect.left + b_rect.left + 11, d_rect.bottom+13),
	 	day_width, day_height, 2, true,
		curr_time, day_count, start_day, B_ALIGN_CENTER,
		new BMessage(msg_update_day));
	AddChild(fCalendar);
	fCalendar->SetTarget(NULL, Window());
		
	// 	resize it here, we have enough information to determine
	//	the optimal width and height
	//	all other controls are based on the date selector and calender
	//	locations
	
	ResizeToPreferred();

	//	pass in some dimension, call resizetopreferred later
	//	and set its location accordingly
	r.Set(0, 12, Bounds().Width() - 28, 30);
	fTimeSelector = new TTimeSelector( r );

	char str[3];
	list = new TSelectionList(NULL, new BMessage('hour'));
	for (int32 i=1 ; i <= 12 ; i++) {
		sprintf(str, "%02ld", i);
		list->AddItem(str);
	}
	list->SetDelimiter(":");
	fTimeSelector->AddList(list);	
	list = new TSelectionList(NULL, new BMessage('min '));
	for (int32 i=0 ; i <= 59 ; i++) {
		sprintf(str, "%02ld", i);
		list->AddItem(str);
	}	
	list->SetDelimiter(":");
	fTimeSelector->AddList(list);
	list = new TSelectionList(NULL, new BMessage('secs'));
	for (int32 i=0 ; i <= 59 ; i++) {
		sprintf(str, "%02ld", i);
		list->AddItem(str);
	}	
	fTimeSelector->AddList(list);	
	list = new TSelectionList(NULL, new BMessage('invl'));
	list->AddItem("AM");	
	list->AddItem("PM");	
	fTimeSelector->AddList(list);

	fTimeSelector->SetAlignment(B_ALIGN_RIGHT, 0);
	fTimeSelector->SetAlignment(B_ALIGN_RIGHT, 1);
	fTimeSelector->SetAlignment(B_ALIGN_RIGHT, 2);
	fTimeSelector->SetAlignment(B_ALIGN_RIGHT, 3);
	
	fTimeSelector->SetTargetList(0);
	AddChild(fTimeSelector);
	
	int32 right = (int32)fDateSelector->Frame().right + 13 + 2;
	int32 distance = (int32)Bounds().Width() - right;
	int32 x = (distance/2) - ((int32)fTimeSelector->Bounds().Width()/2);
	fTimeSelector->MoveTo(right + x, fTimeSelector->Frame().top);

	//	add local/gmt radio buttons
	int32 analogClockTop=0;
#if SHOW_GMT
	bool isLocal = get_rtc_setting();

	r.left = fDateSelector->Frame().right + 13 + 2 + 5;
	r.right = r.left + StringWidth("Clock set to:") + 5;
	r.bottom = Bounds().Height() - 12 - FontHeight(this, true) + 2 - 5;
	r.top = r.bottom - FontHeight(this, true) - 2;
	fGMTLabel = new BStringView(r, "text", "Clock set to:",
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fGMTLabel->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fGMTLabel);	

	r.left = r.right + 5; r.right = Bounds().Width() - 5;
	r.top -= 2;  r.bottom = r.top + FontHeight(this, true);
	fLocalTimeBtn = new BRadioButton(r, "local", "Local time",
		new BMessage(msg_set_local_time), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fLocalTimeBtn->SetValue(isLocal);
	AddChild(fLocalTimeBtn);

	r.top = r.bottom + 5;
	r.bottom = r.top + FontHeight(this, true) + 2;
	fGMTBtn = new BRadioButton(r, "gmt", "GMT",
		new BMessage(msg_set_gmt_time), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fGMTBtn->SetValue(!isLocal);
	AddChild(fGMTBtn);
	
	analogClockTop = ((int32)fLocalTimeBtn->Frame().top
		- (int32)fTimeSelector->Frame().bottom)/2;
	analogClockTop -= 84/2;
	analogClockTop += (int32)fTimeSelector->Frame().bottom;
#else
	analogClockTop = (int32)fTimeSelector->Frame().bottom + 15;
#endif
	//	if the gmt controls do not exist then
	//		the analog clock should be 15 below the time selector
	//	else it should be centered between the time selector
	//		and the gmt controls
	// 	the analog clock is not navigable so its add'd order does not matter

	int32 analogClockLeft = 0;
	analogClockLeft = (int32)fTimeSelector->Bounds().Width()/2 - 84/2;
	analogClockLeft += (int32)fTimeSelector->Frame().left;
	
	fAnalogClock = new TAnalogClock(
		BPoint(analogClockLeft, analogClockTop),
		"Clock", 30, 30, 20, 41, true);
	AddChild(fAnalogClock);
}

void
TDTView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (!fFirstTime) {
#if SHOW_GMT
		fGMTBtn->SetTarget(this);
		fLocalTimeBtn->SetTarget(this);
#endif
	
		UpdateControls(true);
		
		fDateSelector->SetTarget(this);
		fCalendar->SetTarget(this);
		fTimeSelector->SetTarget(this);

		fFirstTime = true;
	}
}

void
TDTView::DetachedFromWindow()
{
	fDateSelector->MakeSelected(false);
	fTimeSelector->MakeSelected(false);
}

void
TDTView::Draw(BRect r)
{
	BView::Draw(r);
	
	SetLowColor(ViewColor());
	SetHighColor(kDarkGray);
	StrokeLine(	BPoint(fDateSelector->Frame().right+13, 7),
				BPoint(fDateSelector->Frame().right+13, Bounds().Height() - 5));
	SetHighColor(kWhite);
	StrokeLine(	BPoint(fDateSelector->Frame().right+14, 7),
				BPoint(fDateSelector->Frame().right+14, Bounds().Height() - 5));
}

void
TDTView::MessageReceived(BMessage* m)
{
	int32 month, day, year;
	int32 hour, minute, second;
	bool interval;
	
	switch(m->what) {
#if SHOW_GMT
		case msg_set_gmt_time:	
		case msg_set_local_time:
			{
				bool isLocal = (m->what == msg_set_local_time);

				write_rtc_setting(isLocal);			
				tzset();
			}
			
			fDateSelector->MakeSelected(false);
			fTimeSelector->MakeSelected(false);
			break;
#endif

		//	mousedown in analog clock
		case 'clok':
			fDateSelector->MakeSelected(false);
			fTimeSelector->MakeSelected(false);
			break;
		//	mousedown in time or date selectors
		case 'time':
			fDateSelector->MakeSelected(false);
			break;
		case 'date':
			fTimeSelector->MakeSelected(false);
			break;
		
		//	date change from date selector
		case 'mont':
			GetCurrentDate(&month, &day, &year);
			m->FindInt32("index", &month);
			UpdateSystemDate(month, day, year);
			UpdateCalendar(true);
			break;
		case 'day ':
			GetCurrentDate(&month, &day, &year);
			m->FindInt32("index", &day);
			UpdateSystemDate(month, day, year);
			UpdateCalendar(false);
			break;
		case 'year':
			GetCurrentDate(&month, &day, &year);
			m->FindInt32("index", &year);			
			UpdateSystemDate(month, day, year);
			UpdateCalendar(true);
			break;
			
		//	sent from the calendar	
		case msg_update_day:
			GetCurrentDate(&month, &day, &year);
			m->FindInt32("date", &day);
			UpdateSystemDate(month, day, year);
			fDateSelector->MakeSelected(false);
			fTimeSelector->MakeSelected(false);
			break;
	 	
		//	time change from time selector	
		case 'hour':
			GetCurrentTime(&hour, &minute, &second, &interval);
			m->FindInt32("index", &hour);	// hour list is 0 based, add 1 to it
			UpdateSystemTime(hour+1, minute, second, interval);
			break;
		case 'min ':
			GetCurrentTime(&hour, &minute, &second, &interval);
			m->FindInt32("index", &minute);
			UpdateSystemTime(hour, minute, second, interval);
			break;
		case 'secs':
			GetCurrentTime(&hour, &minute, &second, &interval);
			m->FindInt32("index", &second);
			UpdateSystemTime(hour, minute, second, interval);
			break;
		case 'invl':
			{
				int32 i;
				GetCurrentTime(&hour, &minute, &second, &interval);
				m->FindInt32("index", &i);
				interval = (i==1);
				UpdateSystemTime(hour, minute, second, interval);
			}
			break;
			
		case 'aclk':
			{
				GetCurrentTime(&hour, &minute, &second, &interval);
				m->FindInt32("hours", &hour);
				m->FindInt32("minutes", &minute);
	
				if (interval && hour > 12)
					hour -= 12;
				else if (!interval && hour == 12)
					hour = 0;
	
				UpdateSystemTime(hour, minute, second, interval);
			}
			break;
			
		default:
			BView::MessageReceived(m);
	}
}

void
TDTView::MouseDown(BPoint loc)
{
	//	deselect the date selector
	fDateSelector->MakeSelected(false);
	//	deselect the time selector
	fTimeSelector->MakeSelected(false);
	BView::MouseDown(loc);
}

void
TDTView::Pulse()
{
	UpdateControls();
}

void
TDTView::GetPreferredSize(float* width, float* height)
{
	//	 width
	//	12 border, left
	//	width of date selector
	//	12 gap
	// 	2 line
	//	stringwidth gmt/local string, 21 accounts for spacing
	//		and radio btn graphic
	//	12 border, right
	*width = 12 + fDateSelector->Bounds().Width() + 12 + 2
		+ 12 + StringWidth("Clock set to:") + 26 + StringWidth("Local time") + 12;

	//	height
	//	14 border, top
	//	height of date selector
	//	14 gap
	//	height of calendar
	//	14	border, bottom
	*height = 12 + fDateSelector->Bounds().Height() + 16
		+ fCalendar->Bounds().Height() + 12;
}

void
TDTView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void
TDTView::UpdateCalendar(bool changeDays)
{
	time_t t = time(NULL);
	struct tm currTime = *localtime(&t);
	time_t temp = time(NULL);
	short startDay, daysInMonth;
	_MonthInfo(&temp, &startDay, &daysInMonth);

	fCalendar->UpdateCalendar(temp, daysInMonth, startDay,
		currTime.tm_mday);
	//	the number of days in the month may have changed
	//	update the ceiling of the day counter in the date selector
	if (changeDays)
		fDateSelector->SetCeiling(daysInMonth, 1);
}

//	make all the time and date controls
//	display the sytem time and date
void
TDTView::UpdateControls(bool force)
{
	time_t t = time(NULL);
	struct tm currTime = *localtime(&t);
	bool calendarUpdated = false;

	//	get the current date and time
	//	update the date selector
	if (force || fDateSelector->TargetList() != 0) {
		if (currTime.tm_mon != (fDateSelector->Selection(0))) {
			fDateSelector->SetSelection(currTime.tm_mon, 0);
			UpdateCalendar(true);
			calendarUpdated = true;
		}
	}
	if (force || fDateSelector->TargetList() != 1) {
		if (currTime.tm_mday != (fDateSelector->Selection(1))) {
			fDateSelector->SetSelection(currTime.tm_mday, 1);
			if (!calendarUpdated) {
				UpdateCalendar(true);
				calendarUpdated = true;
			}
		}
	}
	if (force || fDateSelector->TargetList() != 2) {
		if (currTime.tm_year != (fDateSelector->Selection(2)-1900)) {
			fDateSelector->SetSelection(currTime.tm_year+1900, 2);
			if (!calendarUpdated) {
				UpdateCalendar(true);
				calendarUpdated = true;
			}
		}
	}
		
	//	update the time selector
	//	second
	if (force || fTimeSelector->TargetList() != 2) {
		if (currTime.tm_sec != (fTimeSelector->Selection(2)))
			fTimeSelector->SetSelection(currTime.tm_sec, 2);
	}
	//	minute
	if (force || fTimeSelector->TargetList() != 1) {
		if (currTime.tm_min != (fTimeSelector->Selection(1)))
			fTimeSelector->SetSelection(currTime.tm_min, 1);
	}
	//	hour
	if (force || fTimeSelector->TargetList() != 0) {
		int32 newHour = currTime.tm_hour;
		if (newHour > 12)
			newHour -= 12;
		else if (newHour == 0)
			newHour = 12;
		if (newHour != (fTimeSelector->Selection(0)+1))
			fTimeSelector->SetSelection(newHour-1, 0);
	}
	//	interval
	if (force || fTimeSelector->TargetList() != 3) {
		short interval;
		if (currTime.tm_hour < 12) interval = 0;
		else interval = 1;
		if (interval != fTimeSelector->Selection(3))
			fTimeSelector->SetSelection(interval, 3);
	}
	//	update the clock
	fAnalogClock->SetTime(currTime.tm_hour, currTime.tm_min,
		currTime.tm_sec);
}

void
TDTView::GetCurrentDate(int32* month, int32* day, int32* year)
{
	time_t now = time(NULL);
	struct tm lcltm = *localtime(&now);
	
	*month = lcltm.tm_mon;
	*day = lcltm.tm_mday;
	*year = lcltm.tm_year + 1900;
}

void
TDTView::GetCurrentTime(int32* hour, int32* minute, int32* second,
	bool* interval)
{
	time_t now = time(NULL);
	struct tm lcltm = *localtime(&now);

//printf("local time %i %i %i\n", lcltm.tm_hour, lcltm.tm_min, lcltm.tm_sec);	

	*interval = lcltm.tm_hour >= 12;
	*hour = lcltm.tm_hour;
	if (*hour > 12)
		*hour -= 12;
	*minute = lcltm.tm_min;
	*second = lcltm.tm_sec;

//printf("local time %i %i %i\n", *hour, *minute, *second);	
}

void
TDTView::UpdateSystemDate(int32 month, int32 day, int32 year)
{
	time_t	caltime, now;
	struct tm 	new_time, lcltm;

	now = time(NULL);
	lcltm = *localtime(&now);
	
	new_time = lcltm;
	new_time.tm_mon = month;
	new_time.tm_mday = day;
	new_time.tm_year = year - 1900;

	caltime = mktime(&new_time);
	
	stime(&caltime);
	
	Pulse();
}

void
TDTView::UpdateSystemTime(int32 hour, int32 minute, int32 second,
	bool interval)
{
	time_t	caltime, now;
	struct tm 	new_time, lcltm;

	now = time(NULL);
	lcltm = *localtime(&now);
	
	new_time = lcltm;
	new_time.tm_hour = hour;
	if (interval && hour != 12)
		new_time.tm_hour += 12;
	else if (!interval && hour == 12)
		new_time.tm_hour = 0;
	
	new_time.tm_min  = minute;
	new_time.tm_sec  = second;
	
	caltime = mktime(&new_time);
	
	stime(&caltime);
	
	Pulse();
}

// ************************************************************************** //

TConfigView::TConfigView(BRect frame)
	: BTabView(frame, "configure", B_WIDTH_FROM_WIDEST, B_FOLLOW_NONE)
{
	SetFont(be_plain_font);

	BRect r(ContainerView()->Bounds());
	fDTView = new TDTView( r, "Settings");
	fDTView->SetViewColor(ViewColor());
	AddTab(fDTView);
	
	fTimeZone = new TTimeZone(r);
	fTimeZone->SetViewColor(ViewColor());
	AddTab(fTimeZone);
	
}

TConfigView::~TConfigView()
{
}

void
TConfigView::AttachedToWindow()
{
	BTabView::AttachedToWindow();

	//	the dtview will resize itself based on the current font
	//	force the timezone and parent (then window) to follow suit
	float w = fDTView->Bounds().Width() - Bounds().Width() + 2;
	float h = fDTView->Bounds().Height() - Bounds().Height() + TabHeight() + 2;
	fTimeZone->ResizeBy(w, h);
	ResizeBy(w, h);
}

void 
TConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'aclk':
			fDTView->MessageReceived(msg);
			break;
		default:
			BTabView::MessageReceived(msg);
			break;
	}
}

//
//	here exhibit two bugs in TabView
//		TabHeight is off by 1 (too great)
//		The extra medgray lines are unnecessary
void
TConfigView::DrawBox(BRect selFrame)
{
	float t_height = TabHeight()-1;
	//
	//	add the frame, excluding the tabs
	//
	BeginLineArray(6);
	// 	left of tab
	if (selFrame.left > 0)
		AddLine(BPoint(0,t_height+1), BPoint(selFrame.left-1,t_height+1),kWhite);
	//	right of tab	
	AddLine(BPoint(selFrame.right+1,t_height+1),
		BPoint(Bounds().Width(),t_height+1),kWhite);

	// 	left side
	AddLine(BPoint(0,t_height+1), BPoint(0,Bounds().Height()), kWhite);
	// 	right side
//	AddLine(BPoint(Bounds().Width()-1,t_height+1),
//		BPoint(Bounds().Width()-1,Bounds().Height()-1), kMedGray);
	AddLine(BPoint(Bounds().Width(),t_height+1),
		BPoint(Bounds().Width(),Bounds().Height()), kDarkGray);
	// 	bottom
//	AddLine(BPoint(1,Bounds().Height()-1),
//		BPoint(Bounds().Width()-1,Bounds().Height()-1),kMedGray);
	AddLine(BPoint(1,Bounds().Height()),
		BPoint(Bounds().Width(),Bounds().Height()),kDarkGray);
		
	EndLineArray();
//	BTabView::DrawBox(selFrame);
}

// **************************************************************************** //

const int32 kWinWidth = 340;
const int32 kWinHeight = 217;

TTimeWindow::TTimeWindow()
	: BWindow(BRect(0, 0, kWinWidth, kWinHeight), kWindowTitle,
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE )
{
	BPath path;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
		BPoint loc;
				
		path.Append (kTimeSettingsFilename);
		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
			loc.x = -1; loc.y = -1;
		
			lseek (ref, 0, SEEK_SET);
			read(ref, &loc, sizeof(BPoint));
			close(ref);
			
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
				MoveTo(loc);
				goto NEXT;
			}			
		}
	}
	
	// 	if prefs dont yet exist or the window is not onscreen, center the window
	{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
	}
NEXT:			
	AddParts();
	SetPulseRate(1000000);
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
}

TTimeWindow::~TTimeWindow()
{
}

void
TTimeWindow::FrameResized(float w, float h)
{
	BWindow::FrameResized(w,h);
}

void
TTimeWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'aclk':
			fConfigView->MessageReceived(msg);
			break;
			
		case msg_show_config:
			{
				int32 tab=0;
				msg->FindInt32("config", &tab);
				if (fConfigView->Selection() != tab)
					fConfigView->Select(tab);
			}
			break;
			
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool 
TTimeWindow::QuitRequested()
{
	BPoint	win_pos;
	BPath	path;

	win_pos = Frame().LeftTop();
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		int	ref;
		path.Append (kTimeSettingsFilename);
		if ((ref = creat(path.Path(), 0777)) >= 0) {
			write(ref, &win_pos, sizeof(BPoint));
			close(ref);
		}
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
TTimeWindow::AddParts()
{
	BRect r(Bounds());
	r.InsetBy(-1, -1);
	BBox* bg = new BBox(r, "bg", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(bg);
	
	r.Set(1, 10, Bounds().Width()+1, Bounds().Height()+1);
	fConfigView = new TConfigView(r);
	bg->AddChild(fConfigView);
	
	//	the config view will resize itself to accomodate itself based
	//	on the current font and layout of its controls
	//	the window should be resized to make the controls visible accordingly
	int32 w = (int32)fConfigView->Bounds().Width() - (int32)r.Width();
	int32 h = (int32)fConfigView->Bounds().Height() - (int32)r.Height();

	ResizeBy(w,h);
}
