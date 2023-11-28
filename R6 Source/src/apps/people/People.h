//--------------------------------------------------------------------
//	
//	People.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _PEOPLE_H
#define _PEOPLE_H

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Box.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <Rect.h>
#include <Screen.h>
#include <TextControl.h>
#include <View.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			321
#define WIND_HEIGHT			340

#define TEXT_HEIGHT			 16
#define NAME_H				 10
#define NAME_V				 10
#define NAME_WIDTH			300
#define NAME_TEXT			"Name:"
#define NICKNAME_H			 10
#define NICKNAME_V			(NAME_V + 25)
#define NICKNAME_WIDTH		300
#define NICKNAME_TEXT		"Nickname:"
#define COMPANY_H			 10
#define COMPANY_V			(NICKNAME_V + 25)
#define COMPANY_WIDTH		300
#define COMPANY_TEXT		"Company:"
#define ADDRESS_H			 10
#define ADDRESS_V			(COMPANY_V + 25)
#define ADDRESS_WIDTH		300
#define ADDRESS_TEXT		"Address:"
#define CITY_H				 10
#define CITY_V				(ADDRESS_V + 25)
#define CITY_WIDTH			300
#define CITY_TEXT			"City:"
#define STATE_H				 10
#define STATE_V				(CITY_V + 25)
#define STATE_WIDTH			175
#define STATE_TEXT			"State:"
#define ZIP_H				(STATE_H + STATE_WIDTH)
#define ZIP_V				(CITY_V + 25)
#define ZIP_WIDTH			125
#define ZIP_TEXT			"Zip:"
#define COUNTRY_H			 10
#define COUNTRY_V			(ZIP_V + 25)
#define COUNTRY_WIDTH		300
#define COUNTRY_TEXT		"Country:"
#define HPHONE_H			 10
#define HPHONE_V			(COUNTRY_V + 25)
#define HPHONE_WIDTH		300
#define HPHONE_TEXT			"Home Phone:"
#define WPHONE_H			 10
#define WPHONE_V			(HPHONE_V + 25)
#define WPHONE_WIDTH		300
#define WPHONE_TEXT			"Work Phone:"
#define FAX_H				 10
#define FAX_V				(WPHONE_V + 25)
#define FAX_WIDTH			300
#define FAX_TEXT			"Fax:"
#define EMAIL_H				 10
#define EMAIL_V				(FAX_V + 25)
#define EMAIL_WIDTH			300
#define EMAIL_TEXT			"E-mail:"
#define URL_H				 10
#define URL_V				(EMAIL_V + 25)
#define URL_WIDTH			300
#define URL_TEXT			"URL:"
#define GROUP_H				 10
#define GROUP_V				(URL_V + 25)
#define GROUP_WIDTH			300
#define GROUP_TEXT			"Group:"

#define	B_PERSON_MIMETYPE	"application/x-person"
#define APP_SIG				"application/x-vnd.Be-PEPL"

#define P_NAME				"META:name"
#define P_NICKNAME			"META:nickname"
#define P_COMPANY			"META:company"
#define P_ADDRESS			"META:address"
#define P_CITY				"META:city"
#define P_STATE				"META:state"
#define P_ZIP				"META:zip"
#define P_COUNTRY			"META:country"
#define P_HPHONE			"META:hphone"
#define P_WPHONE			"META:wphone"
#define P_FAX				"META:fax"
#define P_EMAIL				"META:email"
#define P_URL				"META:url"
#define P_GROUP				"META:group"

enum	MESSAGES			{M_NEW = 128, M_SAVE, M_SAVE_AS, M_REVERT,
							 M_UNDO, M_SELECT, M_GROUP_MENU, M_DIRTY,
							 M_NAME, M_NICKNAME, M_COMPANY, M_ADDRESS,
							 M_CITY, M_STATE, M_ZIP, M_COUNTRY, M_HPHONE,
							 M_WPHONE, M_FAX, M_EMAIL, M_URL, M_GROUP,
					 C_EMAIL = 256, C_URL };	/* for clicks */

enum	FIELDS				{F_NAME = 0, F_NICKNAME, F_COMPANY, F_ADDRESS,
							 F_CITY, F_STATE, F_ZIP, F_COUNTRY, F_HPHONE,
							 F_WPHONE, F_FAX, F_EMAIL, F_URL, F_GROUP, F_END};


class	TPeopleWindow;
class	TPeopleView;
class	TTextControl;


//====================================================================

class TPeopleApp : public BApplication {

private:

bool			fHaveWindow;
BRect			fPosition;

public:

BFile			*fPrefs;

				TPeopleApp(void);
				~TPeopleApp(void);
virtual void	AboutRequested(void);
virtual void	ArgvReceived(int32, char**);
virtual void	MessageReceived(BMessage*);
virtual void	RefsReceived(BMessage*);
virtual void	ReadyToRun(void);
TPeopleWindow	*FindWindow(entry_ref);
TPeopleWindow	*NewWindow(entry_ref* = NULL);
};

//--------------------------------------------------------------------

class TPeopleWindow : public BWindow {

private:

BFilePanel		*fPanel;
BMenuItem		*fCopy;
BMenuItem		*fCut;
BMenuItem		*fPaste;
BMenuItem		*fRevert;
BMenuItem		*fSave;
BMenuItem		*fUndo;
TPeopleView		*fView;

public:

entry_ref		*fRef;

				TPeopleWindow(BRect, char*, entry_ref*);
				~TPeopleWindow(void);
virtual void	MenusBeginning(void);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested(void);
void			DefaultName(char*);
void			SetField(int32, const char*);
void			SaveAs(void);
};

//--------------------------------------------------------------------

class TPeopleView : public BView {

private:

BFile			*fFile;
BPopUpMenu		*fGroups;
TTextControl	*fField[F_END];
		int32 _mLastEnd;	/* for Group selection */

public:

				TPeopleView(BRect, char*, entry_ref*);
				~TPeopleView(void); 
virtual	void	AttachedToWindow(void);
virtual void	MessageReceived(BMessage*);
void			BuildGroupMenu(void);
bool			CheckSave(void);
const char*		GetField(int32);
void			NewFile(entry_ref*);
void			Save(void);
void			SetField(int32, const char*, bool);
bool			TextSelected(void);
};

//--------------------------------------------------------------------

class TTextControl : public BTextControl {

private:

char			*fLabel;
char			*fOriginal;
int32			fLength;

public:

				TTextControl(BRect, char*, int32, char*, int32, int32, int32 = 0);
				~TTextControl(void);
virtual void	AttachedToWindow(void);
bool			Changed(void);
void			Revert(void);
void			Update(void);

		void SetClickMessage(BMessage * message);
virtual		void Draw(BRect area);
virtual		void MouseDown(BPoint point);

private:
		BMessage * _mClickMessage;
};
#endif
