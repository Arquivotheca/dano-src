//--------------------------------------------------------------------
//	
//	Email.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------



#ifndef EMAIL_H
#define EMAIL_H

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Directory.h>
#include <Entry.h>
#include <E-mail.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <Path.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <TextControl.h>
#include <View.h>
#include <Volume.h>
#include <Window.h>
#include <Box.h>

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			380
#define WIND_HEIGHT			300

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

#define LINE1_H				  4					// 1 hor line
#define LINE1_V				  5
#define LINE2_H				LINE1_H				// 2 hor line
#define LINE2_V				143
#define LINE3_H				LINE1_H				// 3 hor line
#define LINE3_V				170
#define LINE4_H				LINE1_H				// 4 hor line
#define LINE4_V				238

#define LINE7_H				(WIND_WIDTH / 2 - 1)
#define LINE7_V				(LINE1_V + 2)
#define LINE7_B				(LINE2_V - 1)
#define LINE8_H				LINE7_H
#define LINE8_V				(LINE3_V + 2)
#define LINE8_B				(LINE4_V - 1)

#define TEXT1_H				  4
#define TEXT1_V				(LINE1_V - 5)
#define TEXT1				"Account Info"
#define TEXT2_H				(LINE7_H + TEXT1_H)
#define TEXT2_V				TEXT1_V
#define TEXT2				"Mail Schedule"
#define TEXT3_H				TEXT1_H
#define TEXT3_V				(LINE3_V - 5)
#define TEXT3				"User Settings"
#define TEXT4_H				TEXT2_H
#define TEXT4_V				TEXT3_V
#define TEXT4				"Mail Notification"

#define TEXT_HEIGHT			 16
#define POP_NAME_H			  6
#define POP_NAME_V			(LINE1_V + 15)
#define POP_NAME_WIDTH		193
#define POP_NAME			"POP user name:"
#define POP_PSWD_H			 11
#define POP_PSWD_V			(POP_NAME_V + 25)
#define POP_PSWD_WIDTH		188
#define POP_PSWD			"POP password:"
#define POP_HOST_H			 32
#define POP_HOST_V			(POP_PSWD_V + 25)
#define POP_HOST_WIDTH		167
#define POP_HOST_TEXT		"POP host:"
#define SMTP_HOST_H			 25
#define SMTP_HOST_V			(POP_HOST_V + 25)
#define SMTP_HOST_WIDTH		174
#define SMTP_HOST_TEXT		"SMTP host:"

#define MENU1_H				(LINE7_H + 19)
#define MENU1_V				(POP_NAME_V - 2)
#define MENU1_TEXT			"Check mail:"
#define MENU2_H				(LINE7_H + 40)
#define MENU2_V				(POP_PSWD_V - 2)
#define MENU2_TEXT			"Every:"
#define MENU3_H				(LINE7_H + 44)
#define MENU3_V				(POP_HOST_V - 2)
#define MENU3_TEXT			"From:"
#define MENU4_H				(LINE7_H + 52)
#define MENU4_V				(SMTP_HOST_V - 2)
#define MENU4_TEXT			"To:"

#define MENU_OFFSET			 70
#define MENU_HEIGHT			 16

#define USER_NAME_H			 28
#define USER_NAME_V			(LINE3_V + 15)
#define USER_NAME_WIDTH		171
#define USER_NAME			"Real name:"
#define REPLY_TO_H			 31
#define REPLY_TO_V			(USER_NAME_V + 25)
#define REPLY_TO_WIDTH		168
#define REPLY_TO			"Reply to:"

#define NOTIFY_WIDTH		150
#define NOTIFY_HEIGHT		 16
#define NOTIFY_ALERT_H		(LINE7_H + 30)
#define NOTIFY_ALERT_V		(LINE3_V + 30)
#define NOTIFY_ALERT_TEXT	"Show status window"
#define NOTIFY_BEEP_H		NOTIFY_ALERT_H
#define NOTIFY_BEEP_V		(NOTIFY_ALERT_V + 25)
#define NOTIFY_BEEP_TEXT	"Beep when new mail arrives"

#define ENABLE_WIDTH		(StringWidth(ENABLE_TEXT) + 25)
#define ENABLE_HEIGHT		16
#define ENABLE_H			NOTIFY_BEEP_H
#define ENABLE_V			(NOTIFY_ALERT_V + 25)
#define ENABLE_TEXT			"Enable"
#define SAVE_BUTTON_H		(WIND_WIDTH - 10 - BUTTON_WIDTH)
#define SAVE_BUTTON_V		(WIND_HEIGHT - BUTTON_HEIGHT - 13)
#define SAVE_BUTTON			"Save"
#define REVERT_BUTTON_H		(SAVE_BUTTON_H - 10 - BUTTON_WIDTH)
#define REVERT_BUTTON_V		SAVE_BUTTON_V
#define REVERT_BUTTON		"Revert"
#define CHECK_BUTTON_H		10
#define CHECK_BUTTON_V		SAVE_BUTTON_V
#define CHECK_BUTTON_WIDTH	(BUTTON_WIDTH + 10)
#define CHECK_BUTTON		"Check Now"

#define VIEW_COLOR			216

enum	MESSAGES			{M_SAVE_BUTTON = 128, M_REVERT_BUTTON,
							 M_CHECK_BUTTON, M_POP_NAME , M_POP_PSWD,
							 M_POP_HOST, M_SMTP_HOST, M_NEVER, M_WEEKDAYS,
							 M_EVERY, M_CONTINUOUSLY, M_MINUTE, M_FIVE, M_TEN,
							 M_FIFTEEN, M_THIRTY, M_HOUR, M_TIME, M_AM_PM,
							 M_DIRTY, M_ALERT, M_BEEP,
							 M_USER_NAME, M_REPLY_TO, M_ENABLE};
class	TEmailWindow;
class	TEmailView;
class	TMenuBar;


//====================================================================

//--------------------------------------------------------------------

class TEmailView : public BView {

private:

bool				fNever;
char				fHost[B_MAX_HOST_NAME_LENGTH];
char				fSavedHost[B_MAX_HOST_NAME_LENGTH];
int32				fEnabled;
int32				fSavedEnabled;
BButton				*fSave;
BButton				*fRevert;
BButton				*fCheck;
BCheckBox			*fAlert;
BCheckBox			*fBeep;
BCheckBox			*fEnable;
BMenuField			*fMenu1;
BMenuField			*fMenu2;
BMenuField			*fMenu3;
BMenuField			*fMenu4;
BMenuField			*fMenu5;
BMenuField			*fMenu6;
BMenuItem			*fDayItems[4];
BMenuItem			*fTimeItems[6];
BMenuItem			*fStart1Items[12];
BMenuItem			*fStart2Items[2];
BMenuItem			*fEnd1Items[12];
BMenuItem			*fEnd2Items[2];
BPopUpMenu			*fDays;
BPopUpMenu			*fTime;
BPopUpMenu			*fStart1;
BPopUpMenu			*fStart2;
BPopUpMenu			*fEnd1;
BPopUpMenu			*fEnd2;
BTextControl		*fPOPName;
BTextControl	*fPOPPassword;
BTextControl		*fPOPHost;
BTextControl		*fSMTPHost;
BTextControl		*fRealName;
BTextControl		*fReplyTo;
mail_pop_account	fAccount;
mail_pop_account	fSavedAccount;
mail_notification	fNotify;
mail_notification	fSavedNotify;
char				fDefaultDomain[128];
char				fSavedDefaultDomain[128];
BTextControl*		fDefaultDomainFld;
bool				passdirty;
bool				is_dirty;

public:

				TEmailView(); 
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MessageReceived(BMessage*);
		void	CheckSave(void);
		void	MakeDayMenu(BBox *box);
		void	MakeIntervalMenu(BBox *box);
		void	MakeFromMenu(BBox *box);
		void	MakeToMenu(BBox *box);
		void	RevertMenus(mail_pop_account*);
		void	Save(bool);
};

#endif
