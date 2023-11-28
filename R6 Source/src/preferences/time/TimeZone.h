#ifndef _TIMEZONE_H_
#define _TIMEZONE_H_

#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <View.h>

class BMenuField;

const int32 msg_select_city = 'city';
const int32 msg_set_city = 'setc';
const int32 msg_select_region = 'rgn ';
const int32 msg_change_city = 'chng';

class TCityItem : public BStringItem {
public:
						TCityItem(char *city, char *path);
						~TCityItem();
	
		char*			Path();
private:
		char 			fPath[B_PATH_NAME_LENGTH];
};

class TCityList : public BListView {
public:
						TCityList(BRect r, const char* region,
							const char* city);
						~TCityList();
		
		void			AttachedToWindow();
		void			KeyDown(const char *bytes, int32 numBytes);
		void			MouseDown(BPoint where);

		void			BuildList(const char* region, bool show_options);
		void			EmptyList();
		
const	char*			CityAt(int32 index) const;
		char*			CityPathAt(int32 index) const;
		int32			IndexForCity(const char* city) const;
		
		bool			SetCity(int32 index);
		bool			SetCity(const char* city);
		void			SelectCity();
		TCityItem*		CurrentCity();
		int32			CurrentCityIndex();
		
private:
		bool			fInitd;
		int32			fCityIndex;
		char			fRegionPath[B_PATH_NAME_LENGTH];
		
		char				fKeyBuffer[32];
		bigtime_t			fLastTime;
};

class TRegionItem : public BMenuItem {
public:
						TRegionItem(char *name, char *path, BMessage *msg);
						~TRegionItem();
	
		char* 			Path();
private:
		char 			fPath[B_PATH_NAME_LENGTH];
};

class TRegionMenu : public BPopUpMenu {
public:
						TRegionMenu(char* region);
						~TRegionMenu();
			
		void			BuildList();
const	char*			RegionAt(int32 index) const;
		char*			RegionPathAt(int32 index) const;
		int32			IndexForRegion(const char* region) const;
		bool			SetRegion(char* region);
		TRegionItem*	CurrentRegion();
		
		char*			LongestRegionName()	;
private:
		char			fLongestRegionName[B_FILE_NAME_LENGTH];
};

class TTimeDisplay : public BView {
public:
						TTimeDisplay(BRect frame, const char* label,
							const char* city, const char* path );
						~TTimeDisplay();
				
		void			AttachedToWindow();		
		void			Draw(BRect);
		void			MessageReceived(BMessage*);
		void			Pulse();
		
		void			SetCity(const char* city, const char* path);
		const char*		City() const;
		const char*		Path() const;
		
		struct tm		LocalTime();
private:
		char			fPath[B_PATH_NAME_LENGTH];
		char			fCity[B_FILE_NAME_LENGTH];
		
		BView*			fOSView;
		BBitmap*		fOSBits;
};

class TTimeZone : public BView {
public:
						TTimeZone(BRect frame);
						~TTimeZone();
	
		void			AttachedToWindow();
		void			FrameResized(float new_width, float new_height);
		void 			MessageReceived(BMessage *msg);
	
		void			OffsetSystemClock(struct tm*, struct tm*);
		
		bool			AddRegionList(char* region);
		void			AddCityList(const char* region, const char* city);
		void			AddSelectedCity();
		
		bool			CurrentSystemSettings(char* region, char* city);
		
		void			NewRegionSelection(int32 index);
		void			NewCitySelection(int32 index);
				
		void			City(char* city, char* path);
private:
		bool			fInitd;
		int32			fLastCityIndex;
		
		BMenuField* 	fRegionPopup;
		TRegionMenu*	fRegionMenu;
		BScrollView* 	fCityScroller;
		TCityList*		fCityList;
		
		TTimeDisplay*	fCurrentCity;
		TTimeDisplay*	fSelectedCity;
		BButton*		fSetTZBtn;
};

#endif /* _TIMEZONE_H_ */
