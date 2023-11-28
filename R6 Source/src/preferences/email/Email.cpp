//--------------------------------------------------------------------
//	
//	Email.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------


#include "Email.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mail.h>
#include <Roster.h>
#include <Screen.h>

bool	is_dirty = false;

typedef struct
{
	bool enabled;
	char default_domain[256];
} mail_prefs_data;

status_t get_mail_prefs_data(mail_prefs_data *data);
status_t set_mail_prefs_data(mail_prefs_data *data, bool save);

//====================================================================

int main(void)
{	
	TEmailApp	*myApp;

	myApp = new TEmailApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TEmailApp::TEmailApp(void)
		  :BApplication("application/x-vnd.Be-mprf")
{
	BDirectory		dir;
	BEntry			entry;
	BPath			path;
	BPoint			pos;
	BRect			r;

	r.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT + 26);
	pos = r.LeftTop();

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry("Email_data", &entry) == B_NO_ERROR) {
		fPrefs = new BFile(&entry, O_RDWR);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&pos, sizeof(BPoint));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(pos))
				r.OffsetTo(pos);
		}
	}
	else {
		fPrefs = new BFile();
		if (dir.CreateFile("Email_data", fPrefs) != B_NO_ERROR) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}
	fWindow = new TEmailWindow(r, "E-mail");
	fWindow->Show();
}

//--------------------------------------------------------------------

TEmailApp::~TEmailApp(void)
{
	if (fPrefs)
		delete fPrefs;
}

//--------------------------------------------------------------------

void TEmailApp::AboutRequested(void)
{
	(new BAlert("", "...by Robert Polic", "Big Deal"))->Go();
}


//====================================================================

TEmailWindow::TEmailWindow(BRect rect, char *title)
			 :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE |
													B_NOT_ZOOMABLE)
{
	BRect		r;

	r = Frame();
	r.OffsetTo(0, 0);
	fView = new TEmailView(r, "EmailView");
	Lock();
	AddChild(fView);
	Unlock();
}

//--------------------------------------------------------------------

void TEmailWindow::MessageReceived(BMessage* msg)
{
	char				error_str[256];
	char				host[B_MAX_HOST_NAME_LENGTH];
	mail_pop_account	account;
	status_t			result;

	switch(msg->what) {
		case M_CHECK_BUTTON:
			if (is_dirty) {
				get_pop_account(&account);
				get_smtp_host(host);
				fView->Save(false);
			}
			result = check_for_mail(NULL);
			if (is_dirty) {
				set_pop_account(&account, 0, false);
				set_smtp_host(host, false);
			}
			if (result != B_NO_ERROR)
				if ((new BAlert("", "The mail_daemon is not running.  Shall I start it?", "No", "Start"))->Go())
					be_roster->Launch("application/x-vnd.Be-POST");
			if (result != B_MAIL_NO_DAEMON) {
				if ((result = send_queued_mail()) != B_NO_ERROR) {
					sprintf(error_str, "Error sending mail (%s).", strerror(result));
					(new BAlert("", error_str, "OK"))->Go();
				}
			}
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TEmailWindow::QuitRequested(void)
{
	BPoint		pos;
	BRect		r;
	status_t	result;

	if (is_dirty) {
		result = (new BAlert("", "Save changes before quitting?",
							"Cancel", "Quit", "Save"))->Go();
		if (result == 2)
			fView->Save(true);
		else if (result == 0)
			return false;
	}

	r = Frame();
	pos = r.LeftTop();
	if (((TEmailApp*)be_app)->fPrefs) {
		((TEmailApp*)be_app)->fPrefs->Seek(0, 0);
		((TEmailApp*)be_app)->fPrefs->Write(&pos, sizeof(BPoint));
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//====================================================================

TEmailView::TEmailView(BRect rect, char *title)
		   :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW)
{
	mail_prefs_data data;

	passdirty = false;

	fNever = false;
	get_pop_account(&fAccount);
	get_pop_account(&fSavedAccount);
	get_smtp_host(fHost);
	get_smtp_host(fSavedHost);
	get_mail_notification(&fNotify);
	get_mail_notification(&fSavedNotify);
	
	get_mail_prefs_data(&data);
	strcpy(fDefaultDomain, data.default_domain);
	fEnabled = data.enabled;
		
	strcpy(fSavedDefaultDomain, fDefaultDomain);	
	fSavedEnabled = fEnabled;
}

//--------------------------------------------------------------------

void TEmailView::AttachedToWindow(void)
{
	BFont		font = *be_plain_font;
	BRect		r;
	rgb_color	c = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(c);

	font.SetSize(10);
	SetFont(&font);

	r.left = 10; r.right = 200;
	r.top = 39; r.bottom = 41;
	fPOPName = new TTextControl(r, POP_NAME, sizeof(fAccount.pop_name),
								fAccount.pop_name, M_DIRTY, M_POP_NAME);
	fPOPName->SetTarget(this);
	fPOPName->SetFlags(B_WILL_DRAW | B_NAVIGABLE_JUMP | B_NAVIGABLE);
	AddChild(fPOPName);
	fPOPName->SetDivider(StringWidth("Default domain:") + 5);
	fPOPName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	r.top = fPOPName->Frame().bottom + 5;
	r.bottom = r.top + 1;
	fPOPPassword = new BTextControl(r, "", POP_PSWD, fAccount.pop_password, new BMessage(M_POP_PSWD));
	fPOPPassword->SetTarget(this);
	AddChild(fPOPPassword);
	((BTextView *)fPOPPassword->ChildAt(0))->SetMaxBytes(sizeof(fAccount.pop_password) - 1);
	((BTextView *)fPOPPassword->ChildAt(0))->HideTyping(true);
	fPOPPassword->SetText(fAccount.pop_password);
	fPOPPassword->SetDivider(StringWidth("Default domain:") + 5);
	fPOPPassword->SetModificationMessage(new BMessage(M_POP_PSWD));
	fPOPPassword->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	r.top = fPOPPassword->Frame().bottom + 5;
	r.bottom = r.top + 1;
	fPOPHost = new TTextControl(r, POP_HOST_TEXT, sizeof(fAccount.pop_host),
		fAccount.pop_host, M_DIRTY, M_POP_HOST);
	fPOPHost->SetTarget(this);
	AddChild(fPOPHost);
	fPOPHost->SetDivider(StringWidth("Default domain:") + 5);
	fPOPHost->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	r.top = fPOPHost->Frame().bottom + 5;
	r.bottom = r.top + 1;
	fSMTPHost = new TTextControl(r, SMTP_HOST_TEXT, sizeof(fHost),
		fHost, M_DIRTY, M_SMTP_HOST);
	fSMTPHost->SetTarget(this);
	AddChild(fSMTPHost);
	fSMTPHost->SetDivider(StringWidth("Default domain:") + 5);
	fSMTPHost->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	
	MakeDayMenu();
	MakeIntervalMenu();
	MakeFromMenu();
	MakeToMenu();

	r.left = 10; r.right = 200;
	r.top = 185; r.bottom = 186;
	fRealName = new TTextControl(r, USER_NAME, sizeof(fAccount.real_name),
								fAccount.real_name, M_DIRTY, M_USER_NAME);
	fRealName->SetTarget(this);
	fRealName->SetFlags(B_WILL_DRAW | B_NAVIGABLE_JUMP | B_NAVIGABLE);
	AddChild(fRealName);
	fRealName->SetDivider(StringWidth("Default domain:") + 5);
	fRealName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	r.top = fRealName->Frame().bottom + 5;
	r.bottom = r.top + 1;
	fReplyTo = new TTextControl(r, REPLY_TO, sizeof(fAccount.reply_to),
		fAccount.reply_to, M_DIRTY, M_REPLY_TO);
	fReplyTo->SetTarget(this);
	AddChild(fReplyTo);
	fReplyTo->SetDivider(StringWidth("Default domain:") + 5);
	fReplyTo->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	r.top = fReplyTo->Frame().bottom + 5;
	r.bottom = r.top + 1;
	fDefaultDomainFld = new TTextControl(r, "Default domain:", sizeof(fDefaultDomain),
		fDefaultDomain, M_DIRTY, M_SMTP_HOST);
	fDefaultDomainFld->SetTarget(this);
	AddChild(fDefaultDomainFld);
	fDefaultDomainFld->SetDivider(StringWidth("Default domain:") + 5);
	fDefaultDomainFld->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	//	notification
	r.Set(NOTIFY_ALERT_H, NOTIFY_ALERT_V,
		  NOTIFY_ALERT_H + NOTIFY_WIDTH, NOTIFY_ALERT_V + NOTIFY_HEIGHT);
	fAlert = new BCheckBox(r, "", NOTIFY_ALERT_TEXT, new BMessage(M_ALERT),
		B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	if (fNotify.alert)
		fAlert->SetValue(1);
	fAlert->SetFont(&font);
	fAlert->SetTarget(this);
	AddChild(fAlert);

	r.Set(NOTIFY_BEEP_H, NOTIFY_BEEP_V,
		  NOTIFY_BEEP_H + NOTIFY_WIDTH, NOTIFY_BEEP_V + NOTIFY_HEIGHT);
	fBeep = new BCheckBox(r, "", NOTIFY_BEEP_TEXT, new BMessage(M_BEEP));
	if (fNotify.beep)
		fBeep->SetValue(1);
	fBeep->SetFont(&font);
	fBeep->SetTarget(this);
	AddChild(fBeep);

	r.Set(ENABLE_H, ENABLE_V,
		  ENABLE_H + ENABLE_WIDTH, ENABLE_V + ENABLE_HEIGHT);
	r.OffsetBy(0, 25);
	fEnable = new BCheckBox(r, "", ENABLE_TEXT, new BMessage(M_ENABLE),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	if (fEnabled)
		fEnable->SetValue(1);
	fEnable->SetFont(&font);
	fEnable->SetTarget(this);
	AddChild(fEnable);

	//	buttons
	r.Set(CHECK_BUTTON_H, CHECK_BUTTON_V,
		  CHECK_BUTTON_H + CHECK_BUTTON_WIDTH, CHECK_BUTTON_V + BUTTON_HEIGHT);
	r.OffsetBy(0, 26);
	fCheck = new BButton(r, "", CHECK_BUTTON, new BMessage(M_CHECK_BUTTON));
	fCheck->SetTarget(Window());
	AddChild(fCheck);

	r.Set(REVERT_BUTTON_H, REVERT_BUTTON_V,
		  REVERT_BUTTON_H + BUTTON_WIDTH, REVERT_BUTTON_V + BUTTON_HEIGHT);
	r.OffsetBy(0, 26);
	fRevert = new BButton(r, "", REVERT_BUTTON, new BMessage(M_REVERT_BUTTON));
	fRevert->SetEnabled(false);
	fRevert->SetTarget(this);
	AddChild(fRevert);

	r.Set(SAVE_BUTTON_H, SAVE_BUTTON_V,
		  SAVE_BUTTON_H + BUTTON_WIDTH, SAVE_BUTTON_V + BUTTON_HEIGHT);
	r.OffsetBy(0, 26);
	fSave = new BButton(r, "", SAVE_BUTTON, new BMessage(M_SAVE_BUTTON));
	fSave->SetEnabled(false);
	fSave->SetTarget(this);
	AddChild(fSave);
	fSave->MakeDefault(true);

	SetDrawingMode(B_OP_OVER);
	fPOPName->BTextControl::MakeFocus(true);
}

//--------------------------------------------------------------------

void TEmailView::Draw(BRect)
{
	BFont		font = *be_bold_font;
	BRect		r;

	// Window shading
	r = Frame();
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));
	StrokeLine(BPoint(LINE1_H, LINE1_V + 1),
			   BPoint(r.right - 4, LINE1_V + 1));
	StrokeLine(BPoint(LINE2_H, LINE2_V + 1),
			   BPoint(r.right - 4, LINE2_V + 1));
	StrokeLine(BPoint(LINE3_H, LINE3_V + 1),
			   BPoint(r.right - 4, LINE3_V + 1));
	StrokeLine(BPoint(LINE4_H, LINE4_V + 1 + 26),
			   BPoint(r.right - 4, LINE4_V + 1 + 26));

	StrokeLine(BPoint(LINE7_H + 1, LINE7_V), BPoint(LINE7_H + 1, LINE7_B));
	StrokeLine(BPoint(LINE8_H + 1, LINE8_V), BPoint(LINE8_H + 1, LINE8_B + 26));

	StrokeLine(BPoint(CHECK_BUTTON_H + CHECK_BUTTON_WIDTH + 10 + 1, CHECK_BUTTON_V + 27), 
			   BPoint(CHECK_BUTTON_H + CHECK_BUTTON_WIDTH + 10 + 1, CHECK_BUTTON_V + 25 + fCheck->Frame().Height()));

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.left + 1, r.bottom), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(LINE1_H, LINE1_V), BPoint(r.right - 4, LINE1_V));
	StrokeLine(BPoint(LINE2_H, LINE2_V), BPoint(r.right - 4, LINE2_V));
	StrokeLine(BPoint(LINE3_H, LINE3_V), BPoint(r.right - 4, LINE3_V));
	StrokeLine(BPoint(LINE4_H, LINE4_V + 26), BPoint(r.right - 4, LINE4_V + 26));

	StrokeLine(BPoint(LINE7_H, LINE7_V), BPoint(LINE7_H, LINE7_B));
	StrokeLine(BPoint(LINE8_H, LINE8_V), BPoint(LINE8_H, LINE8_B + 26));

	StrokeLine(BPoint(CHECK_BUTTON_H + CHECK_BUTTON_WIDTH + 10, CHECK_BUTTON_V + 27), 
			   BPoint(CHECK_BUTTON_H + CHECK_BUTTON_WIDTH + 10, CHECK_BUTTON_V + 25 + fCheck->Frame().Height()));

	SetHighColor(0, 0, 0);
	SetLowColor(ViewColor());
	font.SetSize(12);
	SetFont(&font);
	MovePenTo(TEXT1_H, TEXT1_V);	DrawString(TEXT1);
	MovePenTo(TEXT2_H, TEXT2_V);	DrawString(TEXT2);
	MovePenTo(TEXT3_H, TEXT3_V);	DrawString(TEXT3);
	MovePenTo(TEXT4_H, TEXT4_V);	DrawString(TEXT4);
}

//--------------------------------------------------------------------

void TEmailView::MessageReceived(BMessage* msg)
{
	bool		enabled = false;
	int32		index;
	BMenuItem	*item;
	mail_prefs_data data;

	switch (msg->what) {
		case M_SAVE_BUTTON:
			Save(true);
			break;

		case M_REVERT_BUTTON:
			fPOPName->SetText(fSavedAccount.pop_name);
			fPOPPassword->SetModificationMessage(0);
			fPOPPassword->SetText(fSavedAccount.pop_password);
			fPOPPassword->SetModificationMessage(new BMessage(M_POP_PSWD));
			fPOPHost->SetText(fSavedAccount.pop_host);
			fSMTPHost->SetText(fSavedHost);
			fDefaultDomainFld->SetText(fSavedDefaultDomain);

			RevertMenus(&fSavedAccount);

			fAlert->SetValue(fSavedNotify.alert);
			fBeep->SetValue(fSavedNotify.beep);

			fRealName->SetText(fSavedAccount.real_name);
			fReplyTo->SetText(fSavedAccount.reply_to);

			fSave->SetEnabled(false);
			fRevert->SetEnabled(false);

			fEnable->SetValue(fSavedEnabled);

			get_pop_account(&fAccount);
			get_smtp_host(fHost);
			get_mail_notification(&fNotify);
			
			//get_default_domain(fDefaultDomain);
			get_mail_prefs_data(&data);
	    	strcpy(fDefaultDomain, data.default_domain);
			passdirty = false;
			break;

		case M_POP_PSWD:
			passdirty = true;
			break;

		case M_POP_NAME:
		case M_POP_HOST:
		case M_SMTP_HOST:
		case M_DIRTY:
		case M_ALERT:
		case M_BEEP:
		case M_USER_NAME:
		case M_REPLY_TO:
			break;

		case M_ENABLE:
			fEnabled = fEnable->Value();
			break;

		case M_NEVER:
			fAccount.days = B_CHECK_NEVER;
			break;
		case M_WEEKDAYS:
			fAccount.days = B_CHECK_WEEKDAYS;
			break;
		case M_EVERY:
			fAccount.days = B_CHECK_DAILY;
			break;
		case M_CONTINUOUSLY:
			fAccount.days = B_CHECK_CONTINUOUSLY;
			break;

		case M_MINUTE:
			fAccount.interval = 1 * 60;
			break;
		case M_FIVE:
			fAccount.interval = 5 * 60;
			break;
		case M_TEN:
			fAccount.interval = 10 * 60;
			break;
		case M_FIFTEEN:
			fAccount.interval = 15 * 60;
			break;
		case M_THIRTY:
			fAccount.interval = 30 * 60;
			break;
		case M_HOUR:
			fAccount.interval = 60 * 60;
			break;

		case M_TIME:
			index = msg->FindInt32("index") * 60 * 60;
			msg->FindPointer("source", (void **)&item);
			if (item->Menu() == fStart1) {
				if (index == 0) {
					if ((fAccount.begin_time != 12 * 60 * 60) &&
						(fAccount.begin_time != 0)) {
						fStart2Items[0]->SetLabel("Midnight");
						fStart2Items[1]->SetLabel("Noon");
						if (fAccount.begin_time < 12 * 60 * 60)
							fStart2Items[0]->SetMarked(true);
						else
							fStart2Items[1]->SetMarked(true);
					}
				}
				else if ((fAccount.begin_time == 12 * 60 * 60) ||
						 (fAccount.begin_time == 0)) {
					fStart2Items[0]->SetLabel("AM");
					fStart2Items[1]->SetLabel("PM");
					if (fAccount.begin_time < 12 * 60 * 60)
						fStart2Items[0]->SetMarked(true);
					else
						fStart2Items[1]->SetMarked(true);
				}
				if (fAccount.begin_time > 12 * 60 * 60) {
					index += 12 * 60 * 60;
					if (index == 24 * 60 * 60)
						index = 0;
				}
				fAccount.begin_time = index;
			}
			else {
				if (index == 0) {
					if ((fAccount.end_time != 12 * 60 * 60) &&
						(fAccount.end_time != 0)) {
						fEnd2Items[0]->SetLabel("Midnight");
						fEnd2Items[1]->SetLabel("Noon");
						if (fAccount.end_time < 12 * 60 * 60)
							fEnd2Items[0]->SetMarked(true);
						else
							fEnd2Items[1]->SetMarked(true);
					}
				}
				else if ((fAccount.end_time == 12 * 60 * 60) ||
						 (fAccount.end_time == 0)) {
					fEnd2Items[0]->SetLabel("AM");
					fEnd2Items[1]->SetLabel("PM");
					if (fAccount.end_time < 12 * 60 * 60)
						fEnd2Items[0]->SetMarked(true);
					else
						fEnd2Items[1]->SetMarked(true);
				}
				if (fAccount.end_time >= 12 * 60 * 60) {
					index += 12 * 60 * 60;
					if (index == 24 * 60 * 60)
						index = 0;
				}
				fAccount.end_time = index;
			}
			break;

		case M_AM_PM:
			if (msg->FindInt32("index"))
				index = 12 * 60 * 60;
			else index = -12 * 60 * 60;
			msg->FindPointer("source", (void **)&item);
			if (item->Menu() == fStart2) {
				if ((fAccount.begin_time + index >= 0) &&
					(fAccount.begin_time + index < 24 * 60 * 60))
					fAccount.begin_time += index;
					if (fAccount.begin_time == 24 * 60 * 60)
						fAccount.begin_time = 0;
			}
			else {
				if ((fAccount.end_time + index >= 0) &&
					(fAccount.end_time + index < 24 * 60 * 60))
					fAccount.end_time += index;
					if (fAccount.end_time == 24 * 60 * 60)
						fAccount.end_time = 0;
			}
			break;
	}
	CheckSave();

	if ((msg->what == M_WEEKDAYS) || (msg->what == M_EVERY) || (msg->what == M_CONTINUOUSLY))
		enabled = true;
	else if (msg->what != M_NEVER)
		return;

	fNever = !enabled;
	if (fMenu2->IsEnabled() != enabled)
		fMenu2->SetEnabled(enabled);
	if (msg->what == M_CONTINUOUSLY)
		enabled = false;
	if (fMenu3->IsEnabled() != enabled)
		fMenu3->SetEnabled(enabled);
	if (fMenu4->IsEnabled() != enabled)
		fMenu4->SetEnabled(enabled);
	if (fMenu5->IsEnabled() != enabled)
		fMenu5->SetEnabled(enabled);
	if (fMenu6->IsEnabled() != enabled)
		fMenu6->SetEnabled(enabled);
}

//--------------------------------------------------------------------

void TEmailView::CheckSave()
{
	if ((strcmp(fSavedAccount.pop_name, fPOPName->Text())) ||
		(strcmp(fSavedAccount.pop_password, fPOPPassword->Text()) || passdirty) ||
		(strcmp(fSavedAccount.pop_host, fPOPHost->Text())) ||
		(strcmp(fSavedAccount.real_name, fRealName->Text())) ||
		(strcmp(fSavedAccount.reply_to, fReplyTo->Text())) ||
		(strcmp(fHost, fSMTPHost->Text())) ||
		(strcmp(fDefaultDomain, fDefaultDomainFld->Text())) ||
		(fSavedNotify.alert != fAlert->Value()) ||
		(fSavedNotify.beep != fBeep->Value()) ||
		(fAccount.days != fSavedAccount.days) ||
		(fAccount.interval != fSavedAccount.interval) ||
		(fAccount.begin_time != fSavedAccount.begin_time) ||
		(fAccount.end_time != fSavedAccount.end_time) ||
		(fEnabled != fSavedEnabled))
		is_dirty = true;
	else
		is_dirty = false;

	fSave->SetEnabled(is_dirty);
	fRevert->SetEnabled(is_dirty);
}

//--------------------------------------------------------------------

void TEmailView::MakeDayMenu()
{
	int			loop;
	BFont		font = *be_plain_font;
	BRect		r;

	fDays = new BPopUpMenu("");
	fDays->AddItem(fDayItems[0] = new BMenuItem("Never", new BMessage(M_NEVER)));
	if (fAccount.days == B_CHECK_NEVER) {
		fDayItems[0]->SetMarked(true);
		fNever = true;
	}
	fDays->AddItem(fDayItems[1] = new BMenuItem("Weekdays", new BMessage(M_WEEKDAYS)));
	if (fAccount.days == B_CHECK_WEEKDAYS)
		fDayItems[1]->SetMarked(true);

	fDays->AddItem(fDayItems[2] = new BMenuItem("Everyday", new BMessage(M_EVERY)));
	if (fAccount.days == B_CHECK_DAILY)
		fDayItems[2]->SetMarked(true);

	fDays->AddItem(fDayItems[3] = new BMenuItem("Continuously", new BMessage(M_CONTINUOUSLY)));
	if (fAccount.days == B_CHECK_CONTINUOUSLY)
		fDayItems[3]->SetMarked(true);

	font.SetSize(10);
	r.Set(MENU1_H, MENU1_V, MENU1_H + 150, MENU1_V + MENU_HEIGHT);
	fMenu1 = new BMenuField(r, "", MENU1_TEXT, fDays, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	fMenu1->SetFont(&font);
	fMenu1->SetDivider(StringWidth(MENU1_TEXT) + 7);
	fMenu1->MenuBar()->SetFont(&font);
	fMenu1->SetFont(&font);
	AddChild(fMenu1);

	for (loop = 0; loop < 4; loop++)
		fDayItems[loop]->SetTarget(this);
}

//--------------------------------------------------------------------

void TEmailView::MakeIntervalMenu()
{
	int			loop;
	BFont		font = *be_plain_font;
	BRect		r;

	fTime = new BPopUpMenu("");
	fTime->AddItem(fTimeItems[0] = new BMenuItem("Minute", new BMessage(M_MINUTE)));
	if (fAccount.interval == 1 * 60)
		fTimeItems[0]->SetMarked(true);

	fTime->AddItem(fTimeItems[1] = new BMenuItem(" 5 Minutes", new BMessage(M_FIVE)));
	if (fAccount.interval == 5 * 60)
		fTimeItems[1]->SetMarked(true);

	fTime->AddItem(fTimeItems[2] = new BMenuItem("10 Minutes", new BMessage(M_TEN)));
	if (fAccount.interval == 10 * 60)
		fTimeItems[2]->SetMarked(true);

	fTime->AddItem(fTimeItems[3] = new BMenuItem("15 Minutes", new BMessage(M_FIFTEEN)));
	if (fAccount.interval == 15 * 60)
		fTimeItems[3]->SetMarked(true);

	fTime->AddItem(fTimeItems[4] = new BMenuItem("30 Minutes", new BMessage(M_THIRTY)));
	if (fAccount.interval == 30 * 60)
		fTimeItems[4]->SetMarked(true);

	fTime->AddItem(fTimeItems[5] = new BMenuItem("Hour", new BMessage(M_HOUR)));
	if (fAccount.interval == 60 * 60)
		fTimeItems[5]->SetMarked(true);

	font.SetSize(10);
	r.Set(MENU1_H + (font.StringWidth(MENU1_TEXT) - font.StringWidth(MENU2_TEXT)),
		  MENU2_V, MENU1_H + 150, MENU2_V + MENU_HEIGHT);
	fMenu2 = new BMenuField(r, "", MENU2_TEXT, fTime);
	fMenu2->SetFont(&font);
	fMenu2->SetDivider(StringWidth(MENU2_TEXT) + 7);
	fMenu2->MenuBar()->SetFont(&font);
	fTime->SetFont(&font);
	if (fNever)
		fMenu2->SetEnabled(false);
	AddChild(fMenu2);

	for (loop = 0; loop < 6; loop++)
		fTimeItems[loop]->SetTarget(this);
}

//--------------------------------------------------------------------

void TEmailView::MakeFromMenu()
{
	char		time[10];
	int32		loop;
	int32		secs;
	BFont		font = *be_plain_font;
	BRect		r;

	fStart1 = new BPopUpMenu("");
	secs = fAccount.begin_time / (60 * 60);
	if (secs >= 12)
		secs -= 12;
	for (loop = 0; loop <= 11; loop++) {
		if (loop)
			sprintf(time, "%2ld:00", loop);
		else
			sprintf(time, "12:00");
		fStart1->AddItem(fStart1Items[loop] = new BMenuItem(time, new BMessage(M_TIME)));
		if (loop == secs)
			fStart1Items[loop]->SetMarked(true);
		fStart1Items[loop]->SetTarget(this);
	}

	font.SetSize(10);
	r.Set(MENU1_H + (font.StringWidth(MENU1_TEXT) - font.StringWidth(MENU3_TEXT)),
		  MENU3_V, MENU3_H + 85, MENU3_V + MENU_HEIGHT);
	fMenu3 = new BMenuField(r, "", MENU3_TEXT, fStart1);
	fMenu3->SetFont(&font);
	fMenu3->SetDivider(StringWidth(MENU3_TEXT) + 7);
	fMenu3->MenuBar()->SetFont(&font);
	fStart1->SetFont(&font);
	if ((fNever) || (fAccount.days == B_CHECK_CONTINUOUSLY))
		fMenu3->SetEnabled(false);
	AddChild(fMenu3);

	fStart2 = new BPopUpMenu("");
	fStart2->AddItem(fStart2Items[0] = new BMenuItem("AM", new BMessage(M_AM_PM)));
	if (fAccount.begin_time < 12 * 60 * 60)
		fStart2Items[0]->SetMarked(true);
	fStart2Items[0]->SetTarget(this);
	fStart2->AddItem(fStart2Items[1] = new BMenuItem("PM", new BMessage(M_AM_PM)));
	if (fAccount.begin_time >= 12 * 60 * 60)
		fStart2Items[1]->SetMarked(true);
	fStart2Items[1]->SetTarget(this);
	if ((secs == 12) || (secs == 0)) {
		fStart2Items[0]->SetLabel("Midnight");
		fStart2Items[1]->SetLabel("Noon");
	}
	r.Set(MENU3_H + 85, MENU3_V,
		  MENU3_H + 85 + 60, MENU3_V + MENU_HEIGHT);
	fMenu4 = new BMenuField(r, "", "", fStart2);
	fMenu4->SetDivider(0.0);
	fMenu4->SetFont(&font);
	fMenu4->MenuBar()->SetFont(&font);
	fStart2->SetFont(&font);
	if ((fNever) || (fAccount.days == B_CHECK_CONTINUOUSLY))
		fMenu4->SetEnabled(false);
	AddChild(fMenu4);
}

//--------------------------------------------------------------------

void TEmailView::MakeToMenu()
{
	char		time[10];
	int32		loop;
	int32		secs;
	BFont		font = *be_plain_font;
	BRect		r;

	fEnd1 = new BPopUpMenu("");
	secs = fAccount.end_time / (60 * 60);
	if (secs >= 12)
		secs -= 12;
	for (loop = 0; loop <= 11; loop++) {
		if (loop)
			sprintf(time, "%2ld:00", loop);
		else
			sprintf(time, "12:00");
		fEnd1->AddItem(fEnd1Items[loop] = new BMenuItem(time, new BMessage(M_TIME)));
		if (loop == secs)
			fEnd1Items[loop]->SetMarked(true);
		fEnd1Items[loop]->SetTarget(this);
	}

	font.SetSize(10);
	r.Set(MENU1_H + (font.StringWidth(MENU1_TEXT) - font.StringWidth(MENU4_TEXT)),
		  MENU4_V, MENU4_H + 77, MENU4_V + MENU_HEIGHT);
	fMenu5 = new BMenuField(r, "", MENU4_TEXT, fEnd1);
	fMenu5->SetFont(&font);
	fMenu5->SetDivider(StringWidth(MENU4_TEXT) + 7);
	fMenu5->MenuBar()->SetFont(&font);
	fEnd1->SetFont(&font);
	if ((fNever) || (fAccount.days == B_CHECK_CONTINUOUSLY))
		fMenu5->SetEnabled(false);
	AddChild(fMenu5);

	fEnd2 = new BPopUpMenu("");
	fEnd2->AddItem(fEnd2Items[0] = new BMenuItem("AM", new BMessage(M_AM_PM)));
	if (fAccount.end_time < 12 * 60 * 60)
		fEnd2Items[0]->SetMarked(true);
	fEnd2Items[0]->SetTarget(this);
	fEnd2->AddItem(fEnd2Items[1] = new BMenuItem("PM", new BMessage(M_AM_PM)));
	if (fAccount.end_time >= 12 * 60 * 60)
		fEnd2Items[1]->SetMarked(true);
	fEnd2Items[1]->SetTarget(this);
	if ((secs == 12) || (secs == 0)) {
		fEnd2Items[0]->SetLabel("Midnight");
		fEnd2Items[1]->SetLabel("Noon");
	}
	r.Set(MENU4_H + 77, MENU4_V,
		  MENU4_H + 77 + 60, MENU4_V + MENU_HEIGHT);
	fMenu6 = new BMenuField(r, "", "", fEnd2);
	fMenu6->SetDivider(0.0);
	fMenu6->SetFont(&font);
	fMenu6->MenuBar()->SetFont(&font);
	fEnd2->SetFont(&font);
	if ((fNever) || (fAccount.days == B_CHECK_CONTINUOUSLY))
		fMenu6->SetEnabled(false);
	AddChild(fMenu6);
}

//--------------------------------------------------------------------

void TEmailView::RevertMenus(mail_pop_account *account)
{
	bool	enable = true;
	int32	secs;

	if (account->days == B_CHECK_NEVER) {
		fDayItems[0]->SetMarked(true);
		fNever = true;
		enable = false;
	}
	if (account->days == B_CHECK_WEEKDAYS)
		fDayItems[1]->SetMarked(true);
	if (account->days == B_CHECK_DAILY)
		fDayItems[2]->SetMarked(true);
	if (account->days == B_CHECK_CONTINUOUSLY)
		fDayItems[3]->SetMarked(true);

	if (account->interval == 1 * 60)
		fTimeItems[0]->SetMarked(true);
	if (account->interval == 5 * 60)
		fTimeItems[1]->SetMarked(true);
	if (account->interval == 10 * 60)
		fTimeItems[2]->SetMarked(true);
	if (account->interval == 15 * 60)
		fTimeItems[3]->SetMarked(true);
	if (account->interval == 30 * 60)
		fTimeItems[4]->SetMarked(true);
	if (account->interval == 60 * 60)
		fTimeItems[5]->SetMarked(true);

	secs = account->begin_time / (60 * 60);
	if (secs >= 12)
		secs -= 12;
	fStart1Items[secs]->SetMarked(true);
	if ((secs == 12) || (secs == 0)) {
		if ((fAccount.begin_time != 0) && (fAccount.begin_time != 12 * 60 * 60)) {
			fStart2Items[0]->SetLabel("Midnight");
			fStart2Items[1]->SetLabel("Noon");
		}
	}
	else if ((fAccount.begin_time == 0) || (fAccount.begin_time == 12 * 60 * 60)) {
		fStart2Items[0]->SetLabel("AM");
		fStart2Items[1]->SetLabel("PM");
	}
	if (account->begin_time < 12 * 60 * 60)
		fStart2Items[0]->SetMarked(true);
	else
		fStart2Items[1]->SetMarked(true);

	secs = account->end_time / (60 * 60);
	if (secs >= 12)
		secs -= 12;
	fEnd1Items[secs]->SetMarked(true);
	if ((secs == 12) || (secs == 0)) {
		if ((fAccount.end_time != 0) && (fAccount.end_time != 12 * 60 * 60)) {
			fEnd2Items[0]->SetLabel("Midnight");
			fEnd2Items[1]->SetLabel("Noon");
		}
	}
	else if ((fAccount.end_time == 0) || (fAccount.end_time == 12 * 60 * 60)) {
		fEnd2Items[0]->SetLabel("AM");
		fEnd2Items[1]->SetLabel("PM");
	}
	if (account->end_time < 12 * 60 * 60)
		fEnd2Items[0]->SetMarked(true);
	else
		fEnd2Items[1]->SetMarked(true);

	fMenu2->SetEnabled(enable);
	if (account->days == B_CHECK_CONTINUOUSLY)
		enable = false;
	fMenu3->SetEnabled(enable);
	fMenu4->SetEnabled(enable);
	fMenu5->SetEnabled(enable);
	fMenu6->SetEnabled(enable);
}

//--------------------------------------------------------------------

void TEmailView::Save(bool for_real)
{
	BMessenger	*msg;
	mail_prefs_data data;

	msg = new BMessenger("application/x-vnd.Be-POST", -1, NULL);
	if (!msg->IsValid()) {
		if ((new BAlert("", "The mail_daemon is not running.  Shall I start it?", "No", "Start"))->Go())
			be_roster->Launch("application/x-vnd.Be-POST");
		Window()->UpdateIfNeeded();
	}
	delete msg;

	if (is_dirty) {
		strcpy(fAccount.pop_name, fPOPName->Text());
		strcpy(fAccount.pop_password, fPOPPassword->Text());
		strcpy(fAccount.pop_host, fPOPHost->Text());
		strcpy(fAccount.real_name, fRealName->Text());
		strcpy(fAccount.reply_to, fReplyTo->Text());
		strcpy(fHost, fSMTPHost->Text());
		strcpy(fDefaultDomain, fDefaultDomainFld->Text());
		fNotify.alert = fAlert->Value();
		fNotify.beep = fBeep->Value();

		set_pop_account(&fAccount, 0, for_real);
		set_smtp_host(fHost, for_real);
		set_mail_notification(&fNotify, for_real);
		
		//set_default_domain(fDefaultDomain, for_real);
		strcpy(data.default_domain, fDefaultDomain);
		data.enabled = fEnabled;
		set_mail_prefs_data(&data, for_real);

        // This seems very odd? 
		if (for_real) {
			fSave->SetEnabled(false);
			fRevert->SetEnabled(false);
			get_pop_account(&fSavedAccount);
			get_smtp_host(fSavedHost);
			get_mail_notification(&fSavedNotify);
			
			strcpy(fSavedDefaultDomain, fDefaultDomain);
						
			fSavedEnabled = fEnabled;
			is_dirty = false;
			passdirty = false;
		}
	}
}


//====================================================================

TTextControl::TTextControl(BRect rect, char *label, int32 length,
						   char *text, int32 mod_msg, int32 msg)
			 :BTextControl(rect, "", label, text, new BMessage(msg))
{
	SetModificationMessage(new BMessage(mod_msg));

	fMessage = msg;
	strcpy(fLabel, label);
	fLength = length;
}

//--------------------------------------------------------------------

void TTextControl::AttachedToWindow(void)
{
	BFont		font = *be_plain_font;
	BTextView	*text;

	BTextControl::AttachedToWindow();

	SetDivider(StringWidth(fLabel) + 7);
	text = (BTextView *)ChildAt(0);
	text->SetFont(&font);
	text->SetMaxBytes(fLength - 1);
}
