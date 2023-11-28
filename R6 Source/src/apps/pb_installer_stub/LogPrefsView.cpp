/*
	
	LogPrefsView.cpp
	
	Copyright 1997 StarCode Software, All Rights Reserved.
	
*/


#include "LogPrefsView.h"
#include "LogEntry.h"
#include "SettingsManager.h"
#include "Util.h"

#include <StringView.h>
#include <CheckBox.h>
#include <Control.h>

extern SettingsManager	*gSettings;


LogPrefsView::LogPrefsView(BRect rect)
	   	   : ResizeView(rect, "Logging", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
	// Check for notification_freq
	// MakeEmailFile();
}


void LogPrefsView::AttachedToWindow()
{
	// Logging prefs stuff
	
	BRect valet_rect;
//	BRect email_rect;
	
	char	*cbNames[] = {//"SoftwareValet launch and quit",
						//"SoftwareValet Transcevier launch and quit",
						"Trolling for updates",
						"Installations (New packages)",
						//"Uninstallations",
						"Registrations",
						"Downloads (Full or updates/upgrades)"
						//"Updates found on BeDepot",
						//"New software on BeDepot"
						};
	int32	cbFlags[] = {
							//Log_Valet,
							//Log_Jaegar,
							Log_Trolling,
							Log_Installations,
							//Log_Uninstallations,
							Log_Registrations,
							Log_Downloads,
							//Log_Updates,
							//Log_BeDepot	
						};
						
	BRect bounds = Bounds();
	valet_rect = bounds;
	valet_rect.InsetBy(12,12);
	valet_rect.bottom = valet_rect.top + 14;
	
	//BStringView *sv =  new BStringView(valet_rect, B_EMPTY_STRING,
	//				"Customize logging and notification settings:");
	//AddChild(sv);
	
	
	// Email pop-up
	/***
	email_rect.Set(bounds.left+35, bounds.top + 40, 
					bounds.right, bounds.top + 60);
	BPopUpMenu *menu = new BPopUpMenu("<none selected>");;
	BMenuField *mf = new BMenuField(email_rect, "email_freq", "Notification frequency:", 
						menu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	menu->AddItem(new BMenuItem("Immediately",new BMessage(EMAIL_FREQ_IMM)));
	menu->AddItem(new BMenuItem("Hourly",new BMessage(EMAIL_FREQ_HOURLY)));
	menu->AddItem(new BMenuItem("Daily",new BMessage(EMAIL_FREQ_DAILY)));
	menu->AddItem(new BMenuItem("Never",new BMessage(EMAIL_FREQ_NEVER)));		
	menu->SetTargetForItems(this);
	// get from gSettings
	menu->ItemAt(0)->SetMarked(true);
	AddChild(mf);
	***/
	/****
	email_rect.Set(bounds.left+35, bounds.top + 40, 
					bounds.right, bounds.top + 60);
	BCheckBox *cb = new BCheckBox(email_rect, "email",
		"Create E-Mail for automated events",new BMessage(EMAIL_CONTROL));
	AddChild(cb);
	cb->SetValue(gSettings->data.FindInt32("notification/freq") == SettingsManager::EMAIL_ON);
	cb->SetTarget(this);

	valet_rect.Set(bounds.left + 12, bounds.top + 80,
					bounds.right - 12, bounds.top + 94);
	***/
	BStringView *sv;
	sv =  new BStringView(valet_rect, B_EMPTY_STRING,
			"Select which logged events to display:");
	AddChild(sv);
	
	valet_rect.Set(bounds.left + 85, bounds.top + 30, 
					bounds.right, bounds.top + 45);
					 
	int32 displayFlags = gSettings->data.FindInt32("logging/flags");
	for (int i = 0; i < nel(cbNames); i++) {
		BMessage *m = new BMessage(LOG_CONTROL);
		m->AddInt32("flag",cbFlags[i]);
		BCheckBox *valet_cb = new BCheckBox(valet_rect,B_EMPTY_STRING,
							cbNames[i],m);
		AddChild(valet_cb);
		valet_cb->SetTarget(this);
		valet_cb->SetValue(displayFlags & cbFlags[i]);
		valet_rect.OffsetBy(0,20);
	}
	////////////////////
	winHeight = valet_rect.bottom + 30;
}

void LogPrefsView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case LOG_CONTROL: {
			BControl *valet_cb;
			msg->FindPointer("source", (void **) &valet_cb);
			int32 flag = msg->FindInt32("flag");
			bool v = (valet_cb->Value() == B_CONTROL_ON);
			
			int32 theFlags = gSettings->data.FindInt32("logging/flags");
			if (v) theFlags |= flag;
			else theFlags &= ~flag;
			gSettings->data.ReplaceInt32("logging/flags",theFlags);
			break;
		}
		case EMAIL_CONTROL: {
			BControl *valet_cb;
			msg->FindPointer("source", (void **) &valet_cb);
			bool v = (valet_cb->Value() == B_CONTROL_ON);
			
			
			if (v)			
				gSettings->data.ReplaceInt32("notification/freq",SettingsManager::EMAIL_ON);
			else
				gSettings->data.ReplaceInt32("notification/freq",SettingsManager::EMAIL_OFF);
				
			break;
		}
		default:
			break;
	}
}

void LogPrefsView::Draw(BRect r)
{
	ResizeView::Draw(r);
}

/* To go into SetUpDefaults

	// is it better to just say something like:
	// Log_flags = 526; // first 9 bits set to 1
	
	setLog_Valet(true);
	setLog_Jaegar(true);
	setLog_Trolling(true);
	setLog_Installation(true);
	setLog_Uninstallation(true);
	setLog_Registration(true);
	setLog_Downloads(true);
	setLog_Updates(true);
	setLog_BeDepot(true);
	setnotification_freq(EMAIL_IMM_;
	
*/

