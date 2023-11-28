#ifndef TIME_ZONE
#define TIME_ZONE

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <FindDirectory.h>
#include <MenuField.h>
#include <ScrollView.h>
#include <TextView.h>
#include <Window.h>

#include "color_defs.h"
#include "dt_utils.h"
#include "TimeWindow.h"
#include "TimeZone.h"

#include <priv_syscalls.h>

// **************************************************************************** //

TTimeZone::TTimeZone(BRect frame) :
    BView(frame, "Time Zone", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fInitd=false;
	fLastCityIndex = -1;
	
	fCityScroller = NULL;
	fCityList = NULL;
	fSetTZBtn = NULL;
}

TTimeZone::~TTimeZone()
{
}

void
TTimeZone::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (!fInitd) {
		char region[B_PATH_NAME_LENGTH];
		char city[B_PATH_NAME_LENGTH];
	
		CurrentSystemSettings(region, city);
	 	if (AddRegionList(region)) {
			AddCityList(region, city);
			AddSelectedCity();
		}

		fRegionMenu->SetTargetForItems(this);
		fCityList->SetTarget(this);
		fSetTZBtn->SetTarget(this);
		fInitd = true;
	}
}

void
TTimeZone::FrameResized(float w, float h)
{
	BView::FrameResized(w, h);
}

void
TTimeZone::MessageReceived(BMessage *msg)
{
	int32 index;
	char city[B_FILE_NAME_LENGTH], path[B_PATH_NAME_LENGTH];
	struct tm currentTime;
	struct tm selectedTime;
	
	switch (msg->what) {
		case msg_select_region:		// msg from menufield
			msg->FindInt32("index", &index);
			NewRegionSelection(index);
			break;
		
		case msg_select_city:		// selection msg from listview
			msg->FindInt32("index", &index);
			NewCitySelection(index);
			break;

		case msg_set_city:			//	msg from set button
			currentTime = fCurrentCity->LocalTime();
			selectedTime = fSelectedCity->LocalTime();			

			fCurrentCity->SetCity(fSelectedCity->City(), fSelectedCity->Path());
			SetSystemTimeZone((char*)fSelectedCity->Path());

			{
				bool isLocal = true;
				//	only do an offset on Intel when GMT (not local time) is set
#if SHOW_GMT
				isLocal = get_rtc_setting();
#endif
				if (isLocal)
					OffsetSystemClock(&currentTime, &selectedTime);
			}
			fSetTZBtn->SetEnabled(false);
			break;
		case msg_change_city:		//	invoke msg from listview
			msg->FindInt32("index", &index);
			if (index < 0)
				return;
			
			//	get the time in the current and selected zones	
			currentTime = fCurrentCity->LocalTime();
			selectedTime = fSelectedCity->LocalTime();			
			
			fCityList->SetCity(index);
			//	get the new city and its path
			City(city, path);
			
			//	set the selected to the new city
			fSelectedCity->SetCity(city, path);
			
			//	don't set the current if it is already set
			if (strcmp(city, fCurrentCity->City()) != 0) {
				fCurrentCity->SetCity(city, path);
				SetSystemTimeZone(path);
				OffsetSystemClock(&currentTime, &selectedTime);
				fSetTZBtn->SetEnabled(false);
			}			
			
			break;
	}
}

void
TTimeZone::OffsetSystemClock(struct tm* curr, struct tm* sel)
{
	system_info sinfo;
	get_system_info(&sinfo);

	//	don't offset if this is a bebox
	if (sinfo.platform_type != B_BEBOX_PLATFORM) {
		short hour = sel->tm_hour - curr->tm_hour;
		short minute = sel->tm_min - curr->tm_min;
		short second = sel->tm_sec - curr->tm_sec;
	
		time_t	now, caltime;
		struct tm 	new_time, lcltm;
	
		now = time(NULL);
		lcltm = *localtime(&now);
		
		new_time = lcltm;
	
		new_time.tm_hour += hour;
		new_time.tm_min  += minute;
		new_time.tm_sec  += second;
		
		caltime = mktime(&new_time);
		
		stime(&caltime);
	}
}

bool
TTimeZone::AddRegionList(char* region)
{
	fRegionMenu = new TRegionMenu(region);
	
	if (!fRegionMenu) {
		BRect r(Bounds());
	
		BTextView* errView = new BTextView(r, "", r, B_FOLLOW_ALL, B_WILL_DRAW);
		errView->SetViewColor(kViewGray);
		errView->MakeEditable(false);
		errView->MakeSelectable(false);
		errView->SetAlignment(B_ALIGN_CENTER);

		errView->SetText("Timezone database not found");

		AddChild(errView);
		
		return false;
	} else {
		// Need to accomodate width of maximum string, which may be a region or city.
		BRect r(8, 8, 8+10+StringWidth("Menlo Park, Calif")+15 , 29);
		fRegionPopup = new BMenuField( r, "region menu", "Region: ",
			fRegionMenu, true);
		AddChild(fRegionPopup);
		
		fRegionPopup->SetAlignment(B_ALIGN_RIGHT);
		fRegionPopup->SetDivider(0);
		
		return true;
	}
}

void
TTimeZone::AddCityList( const char* region, const char* city)
{
	BRect r(11, fRegionPopup->Frame().bottom + 4,
			fRegionPopup->Frame().right - B_V_SCROLL_BAR_WIDTH - 3,
			Bounds().Height() - 10);

	if (fCityList) {
		fCityList->RemoveSelf();
		delete fCityList;
	}
	//	remove the scroller, toss
	if (fCityScroller) {
		fCityScroller->RemoveSelf();
		delete fCityScroller;
	}

	fCityList = new TCityList( r, region, city);
	fCityList->SetSelectionMessage(new BMessage(msg_select_city));
	fCityList->SetInvocationMessage(new BMessage(msg_change_city));

	fCityScroller = new BScrollView( "scroller", fCityList,
		B_FOLLOW_ALL, true, false, true);

	AddChild(fCityScroller);
	fCityList->SetTarget(this);
}

void
TTimeZone::AddSelectedCity()
{
	float h = FontHeight(this, true);
	char city[B_PATH_NAME_LENGTH], path[B_PATH_NAME_LENGTH];
	BRect r(Bounds());
	
	City(city, path);
	
	r.right -= 8;  r.left = r.right - 75;		// B_WIDTH_AS_USUAL
	r.bottom -= 11; r.top = r.bottom -= 20;
	fSetTZBtn = new BButton(r, "set", "Set", new BMessage(msg_set_city),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(fSetTZBtn);
	fSetTZBtn->SetEnabled(false);	// both current and selected will be the same

	r.left = fCityScroller->Frame().right + 10;
	r.right = Bounds().Width() - 10;
	r.bottom = r.top - h - h - 16;
	r.top = r.bottom - h - h - 10;
	fSelectedCity = new TTimeDisplay(r, "Time in:", city, path);
	AddChild(fSelectedCity);

	r.bottom = r.top - 10;
	r.top = r.bottom - h - h - 7;
	fCurrentCity = new TTimeDisplay(r, "Current time zone:", city, path);
	AddChild(fCurrentCity);
}

const char* const kNA = "America";
const char* const kMenlo = "/Menlo_Park__Calif";

//	returns region and city paths for current tz
bool
TTimeZone::CurrentSystemSettings( char *region, char *city)
{
	if (!city || !region)
		return false;
		
	int ret;
	if ((ret = _kget_tzfilename_(city)) != 0) {
		char tzdirname[B_PATH_NAME_LENGTH];
		find_directory(B_BEOS_ETC_DIRECTORY, -1, false, tzdirname, MAXPATHLEN);
		strcat(tzdirname, "/timezones/");

		sprintf(region, "%s%s", tzdirname, kNA);
		
		struct stat s;
		if (stat( region, &s) == -1)
			return false;
		
		sprintf(city, "%s%s", region, kMenlo);
		if (stat( city, &s) == -1)
			return false;
		
		SetSystemTimeZone(city);
	} else
		_DirectoryNameFromPath(city, region);
	
	return true;
}

void
TTimeZone::NewRegionSelection(int32 index)
{
	const char* region = fRegionMenu->RegionPathAt(index);
	
	if (region) {
		//	rebuild everything so that the listview does not look lame
		//	remove the set button so that it can
		//	be added back later to retain the tab order
		if (fSetTZBtn)
			fSetTZBtn->RemoveSelf();
		
		AddCityList(region, NULL);
		
		AddChild(fSetTZBtn);

		//	if the currently selected region has been selected again
		//	select the city
		char r[B_PATH_NAME_LENGTH], c[B_PATH_NAME_LENGTH];
		CurrentSystemSettings(r, c);
		if (strcmp(r, region) == 0) {
			fCityList->SetCity(fLastCityIndex);
			if (!fSetTZBtn->IsEnabled())
				fSetTZBtn->SetEnabled(true);
			return;
		} 
	}
	if (fSetTZBtn->IsEnabled())
		fSetTZBtn->SetEnabled(false);
}

//	sent from listview
void
TTimeZone::NewCitySelection(int32 index)
{
	//	maintain this selection so that it can reselected
	//	when the region is selected again.
	fLastCityIndex = index;
	fCityList->SetCity(index);

	char city[B_PATH_NAME_LENGTH], path[B_PATH_NAME_LENGTH];
	City(city, path);
	
	fSelectedCity->SetCity(city, path);
	if (strcmp(city, fCurrentCity->City()) != 0)
		fSetTZBtn->SetEnabled(true);
	else
		fSetTZBtn->SetEnabled(false);
}

void
TTimeZone::City(char* city, char* path)
{
	TCityItem* ci = fCityList->CurrentCity();
	if (ci){
		strcpy(path, ci->Path());
		strcpy(city, ci->Text());
	}
}

// **************************************************************************** //

TTimeDisplay::TTimeDisplay(BRect frame, const char* label,
	const char* city, const char* path)
	: BView( frame, label, B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	strcpy(fPath, path);
	strcpy(fCity, city);
	
	fOSView = new BView(Bounds(), "", B_FOLLOW_NONE, 0);
	fOSBits = new BBitmap( Bounds(), B_RGB32, true);
	fOSBits->AddChild(fOSView);
}

TTimeDisplay::~TTimeDisplay()
{
	delete fOSBits;
}

void 
TTimeDisplay::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		fOSBits->Lock();
		fOSView->SetViewColor(ViewColor());
		fOSBits->Unlock();
	}
}

void 
TTimeDisplay::Draw(BRect)
{
	if (!fOSBits->Lock())
		return;
		
	fOSView->PushState();
	
	int32 h = (int32)FontHeight(this, true);
	
	fOSView->SetLowColor(fOSView->ViewColor());

	fOSView->SetHighColor(ViewColor());		
	fOSView->FillRect(Bounds());
	 
	fOSView->SetHighColor(0,0,0,255);
	//	draw the label
	fOSView->MovePenTo(3, h+2);
	fOSView->DrawString(Name());
	
	//	draw the city
	fOSView->MovePenTo(3, h+h+5);
	fOSView->DrawString(fCity);	

	struct tm lcltm = LocalTime();
	short hour, minute, second;
	bool interval;
	
	//	12 AM is 0
	//	12 PM is 12
	interval = lcltm.tm_hour >= 12;
	hour = lcltm.tm_hour;
	if (hour > 12)
		hour -= 12;
	else if (hour == 0)
		hour = 12;
	minute = lcltm.tm_min;
	second = lcltm.tm_sec;
	
	// this could probably be done with strftime
	char str[9];
	if (!interval)
		sprintf(str, "%02i:%02i %s", hour, minute, "AM");
	else
		sprintf(str, "%02i:%02i %s", hour, minute, "PM");
	
	fOSView->SetLowColor(ViewColor());	
	fOSView->SetHighColor(0,0,0,255);
	fOSView->MovePenTo(Bounds().Width() - StringWidth(str), h+h+5);
	fOSView->DrawString(str);

	fOSView->PopState();
	fOSView->Sync();
	fOSBits->Unlock();

	DrawBitmap(fOSBits);
}

void 
TTimeDisplay::MessageReceived(BMessage* m)
{
	switch(m->what) {
		default:
			BView::MessageReceived(m);
	}
}

void 
TTimeDisplay::Pulse()
{
	Draw(Bounds());
}

void
TTimeDisplay::SetCity(const char* city, const char* path)
{
	if (fCity && strcmp(fCity, city) == 0)
		return;
		
	strcpy(fCity, city);

	if (fPath && strcmp(fPath, path) == 0)
		return;
		
	strcpy(fPath, path);

	Draw(Bounds());
}

const char*
TTimeDisplay::City() const
{
	return fCity;
}

const char*
TTimeDisplay::Path() const
{
	return fPath;
}

struct tm
TTimeDisplay::LocalTime()
{
	SetTimeZone(fPath);
	
	time_t now = time(NULL);
	struct tm s = *localtime(&now);

	return s;
}

// **************************************************************************** //

TRegionItem::TRegionItem(char *itemName, char *path, BMessage *msg)
 	: BMenuItem(itemName, msg)
{
	strcpy(fPath, path);
}

TRegionItem::~TRegionItem()
{
}

char*
TRegionItem::Path()
{
	return fPath;
}

// **************************************************************************** //

TRegionMenu::TRegionMenu(char* region)
	: BPopUpMenu("region list")
{
	fLongestRegionName[0] = 0;
	BuildList();
	SetRegion(region);
}

TRegionMenu::~TRegionMenu()
{
}

void
TRegionMenu::BuildList()
{
	char tzdirname[B_PATH_NAME_LENGTH];
	find_directory(B_BEOS_ETC_DIRECTORY, -1, false, tzdirname, MAXPATHLEN);
	strcat(tzdirname, "/timezones/");

	DIR *dir = opendir(tzdirname);
	if (dir == NULL) {
		fprintf(stderr, "timezone directory is %s\n", tzdirname);
		perror("open of timezone directory failed");
		return;
	}

	struct dirent  	*dirent;
	char			region_path[B_PATH_NAME_LENGTH];

	while ((dirent = readdir(dir)) != 0) {
		int			ret;
		struct stat	statbuf;
		
		//	etc stuff is posix style
		//	GMT-4 is actually GMT plus 4 hours
		//	thus potentially confusing, so we skip it
		if (strcmp(dirent->d_name, ".") == 0  ||
			strcmp(dirent->d_name, "..") == 0 ||
			strcmp(dirent->d_name, "Etc") == 0) {
			continue;
		}
		
		strcpy(region_path, tzdirname);
		strcat(region_path, dirent->d_name);

		ret = stat(region_path, &statbuf);

		if (ret < 0)
			continue;

		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(dirent->d_name, "Pacific") == 0
				|| strcmp(dirent->d_name, "Atlantic") == 0
				|| strcmp(dirent->d_name, "Indian") == 0) {
				
				strcat(dirent->d_name, " Ocean");
				
			} else {
			
				replace_underscores(dirent->d_name);
				
			}
			
			if (strlen(fLongestRegionName) < strlen(dirent->d_name))
				strcpy(fLongestRegionName, dirent->d_name);
			
			TRegionItem* item = new TRegionItem(dirent->d_name, region_path,
				new BMessage(msg_select_region));
			AddItem(item);
		}
	}

	closedir(dir);
}

const char*
TRegionMenu::RegionAt(int32 index) const
{
	TRegionItem* item = (TRegionItem*)ItemAt(index);
	return item->Label();
}

char*
TRegionMenu::RegionPathAt(int32 index) const
{
	TRegionItem* item = (TRegionItem*)ItemAt(index);
	return item->Path();
}

int32
TRegionMenu::IndexForRegion(const char* region) const
{
	bool region_found = false;
	int32 index = -1;

	for (index=0 ; index < CountItems() ; index++) {
		if (strcmp(region, RegionPathAt(index)) == 0) {
			region_found = true;
			break;
		}
	}

	if (region_found)
		return index;
	else
		return -1;
}

bool
TRegionMenu::SetRegion(char* region)
{
	int32 index = IndexForRegion(region);
		
	if (index >= 0) {
		TRegionItem* item = (TRegionItem*)ItemAt(index);
		item->SetMarked(true);
		
		return true;
	} else
		return false;
}

TRegionItem*
TRegionMenu::CurrentRegion()
{
	return (TRegionItem*)FindMarked();
}

char*
TRegionMenu::LongestRegionName()
{
	return fLongestRegionName;
}

// **************************************************************************** //

TCityItem::TCityItem(char *cityName, char *path)
	: BStringItem(cityName)
{
	strcpy(fPath, path);
}

TCityItem::~TCityItem()
{
}

char*
TCityItem::Path()
{
	return fPath;
}

// **************************************************************************** //

TCityList::TCityList(BRect r, const char* region, const char* city)
	: BListView( r, "city list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL)
{
	fRegionPath[0] = 0;
	BuildList(region, false);
	SetCity(city);
	strcpy(fRegionPath, region);
	fInitd = false;
}

TCityList::~TCityList()
{
	EmptyList();
}

void
TCityList::AttachedToWindow()
{
	BListView::AttachedToWindow();
	SelectCity();
}

//	this type-ahead makes a few assumptions and is not optimized
//	the search for a matching city is linear which is bad if the list
//		is long
void
TCityList::KeyDown(const char *bytes, int32 numBytes)
{
	if (isalpha(bytes[0])) {
		if (fLastTime > system_time()) {
			short len = strlen(fKeyBuffer);
			if (len < 31) {
				fKeyBuffer[len] = tolower(bytes[0]);
				fKeyBuffer[len+1] = 0;
			}
		} else {	// start over
			fKeyBuffer[0] = tolower(bytes[0]);
			fKeyBuffer[1] = 0;
		}

		if (fKeyBuffer || strlen(fKeyBuffer) > 0){
			bool done = false, found=false;
			char city[32], testcity[32];
			short index=-1;
			short len=strlen(fKeyBuffer);
			while (!done) {
				strncpy(city, fKeyBuffer, len);
				for (index=0 ; index<CountItems() ; index++) {
					//	get a city from the list
					strncpy(testcity,CityAt(index),31);
					short clen = strlen(testcity);
					for (short i=0 ; i<clen ; i++)
						testcity[i] = tolower(testcity[i]);
	
					if (strncmp(testcity, city, len) == 0) {
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
				SetCity(index);
		}			
	
		fLastTime = system_time() + 500000;
		return;
	}
	
	BListView::KeyDown(bytes, numBytes);
}

void
TCityList::MouseDown(BPoint where)
{
	BListView::MouseDown(where);
}

void
TCityList::BuildList(const char* region, bool)
{
	//	check to see if the current region matches the requested
	if (strcmp(fRegionPath, region) == 0)
		return;
	else
		strcpy(fRegionPath, region);
		
	DIR* dir = opendir(region);
	if (dir == NULL)
		return;
	
	if (CountItems() > 0)
		EmptyList();
		
	struct dirent *dirent;
	BList* city_list = new BList();
	while ((dirent = readdir(dir)) != 0) {
		char *str;
		char path[B_PATH_NAME_LENGTH];
		struct stat statbuf;
		int ret;
		
		strcpy(path, region);
		strcat(path, "/");
		strcat(path, dirent->d_name);

		ret = stat(path, &statbuf);
		if (ret < 0)
			continue;

		if (strcmp(dirent->d_name, ".") == 0  ||
			strcmp(dirent->d_name, "..") == 0  ||
			S_ISDIR(statbuf.st_mode)) {
			continue;
		}

		if ((str = strstr(dirent->d_name, "_IN")) != 0)
			strcpy(str, ", Indiana");
		else if ((str = strstr(dirent->d_name, "__Calif")) != 0) {
			strcpy(str, ", Calif");
		}

		replace_underscores(dirent->d_name);
		
		city_list->AddItem( new TCityItem(dirent->d_name, path) );
	}

	AddList(city_list);
}

void
TCityList::EmptyList()
{
	TCityItem* city_item=NULL;
	while ((city_item = (TCityItem *)RemoveItem((int32) 0)) != 0) {
		delete city_item;
	}
}

const char*
TCityList::CityAt(int32 index) const
{
	TCityItem* item = (TCityItem*)ItemAt(index);
	return item->Text();
}

char*
TCityList::CityPathAt(int32 index) const
{
	TCityItem* item = (TCityItem*)ItemAt(index);
	return item->Path();
}

int32
TCityList::IndexForCity(const char* city) const
{
	if (!city || strlen(city) <= 0)
		return -1;
		
	bool city_found = false;
	int32 index=-1;
	
	for (index=0 ; index < CountItems() ; index++) {
		if (strcmp(city, CityPathAt(index)) == 0 ) {
			city_found = true;
			break;
		}
	}

	if (city_found)
		return index;
	else
		return -1;
}

bool
TCityList::SetCity(int32 index)
{
	if (index >= 0 && index < CountItems()) {
		fCityIndex = index;
		Select(fCityIndex);
		ScrollToSelection();
		
		return true;
	} else
		return false;
}

//	set current city by the path (city param)
bool
TCityList::SetCity(const char* city)
{
	fCityIndex = IndexForCity(city);
	if (fCityIndex >= 0) {
		Select(fCityIndex);
		ScrollToSelection();
		return true;
	} else
		return false;
}

void
TCityList::SelectCity()
{
	Select(fCityIndex);
	ScrollToSelection();
}

TCityItem*
TCityList::CurrentCity()
{
	return (TCityItem*)ItemAt(fCityIndex);
}

int32
TCityList::CurrentCityIndex()
{
	return fCityIndex;
}

#endif
