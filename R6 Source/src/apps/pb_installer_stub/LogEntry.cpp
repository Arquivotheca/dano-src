#include "LogEntry.h"
#include "Log.h"
#include "SettingsManager.h"
#include <malloc.h>


extern SettingsManager *gSettings;

LogEntry::LogEntry(void)
{
	entry = NULL;
	SetHeight(80.0);
}

LogEntry::~LogEntry(void) 
{
	free(entry);
}

void LogEntry::SetEntry(BMessage *data) 
{
	entry = data;
}

void LogEntry::DrawItem(BView *owner, BRect bounds, bool complete)
{
	complete;
	owner->SetFont(be_plain_font,B_FONT_SIZE);
	
	int32 action;
	char *actionstr;
	const char *time;
	const char *pkgname;
	const char *strings;
	entry->FindString("timestamp", &time);
	action = entry->what;
	if (action > Log::LOG_VALET_QUIT) {
		entry->FindString("pkgname", &pkgname);
	}
	
	actionstr = actiontostr(action, (char*)pkgname);
		
	//render
if (actionstr) {	
	BRect timerect(BPoint(bounds.left+1, bounds.top+1), BPoint(bounds.left+144, bounds.bottom));
	BRect eventrect(BPoint(bounds.left+141, bounds.top+1), BPoint(bounds.right, bounds.bottom));
	const int nLines = 2;
	const float timefont = 12.0;
	const int logitem_left_border = 2;
	const int logitem_top_border = 12;
	const int action_left_border = 150;
	const int extra_left_border = 175;
	owner->SetLowColor(200,200,200);
	owner->FillRect(timerect, B_SOLID_LOW);
	//owner->SetLowColor(230,230,230);
	//owner->FillRect(eventrect, B_SOLID_LOW);
	
	owner->SetFontSize(timefont);
	//owner->FillRect(bounds,B_SOLID_LOW);	
	owner->SetHighColor(0,0,0);	
	owner->MovePenTo(bounds.left, bounds.top);
	owner->StrokeLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	owner->StrokeLine(BPoint(bounds.left+145, bounds.top), 
						BPoint(bounds.left+145, bounds.bottom), B_MIXED_COLORS);
	owner->MovePenTo(bounds.left + logitem_left_border+3,
					bounds.top + logitem_top_border+4);			
	//owner->SetHighColor(230,0,230);		
	owner->DrawString(time);
	owner->SetFontSize(10.0);
	owner->MovePenTo(bounds.left + action_left_border,
					bounds.top + logitem_top_border+2);
	owner->SetHighColor(255,0,0);
	owner->DrawString(actionstr);
	owner->SetHighColor(66,66,255);	
	
	int errorchk;
	for (int i = 0; i < nLines; i++) {
		errorchk = entry->FindString("strings", i, &strings);
		if (errorchk == B_NO_ERROR) {
			owner->MovePenTo(bounds.left + extra_left_border, 
								bounds.top + ((i+2) * logitem_top_border));
			if (i==0) owner->MovePenBy(0, 2);
			owner->DrawString(strings);
		}
	}
	/*
	if (IsSelected()) {
		owner->InvertRect(bounds);

	}
	*/
	}
}

char *LogEntry::actiontostr(int32 action, char *pkg)
{
	char *returnstr = new char[MSGMAX];
	switch(action) {
		case (Log::LOG_LISTENER_LAUNCH)
			:	sprintf(returnstr, "The SoftwareValet Transceiver was launched.");
				break;
		case(Log::LOG_LISTENER_QUIT)
			:	sprintf(returnstr, "The SoftwareValet Transceiver exited.");
				break;
		case(Log::LOG_TROLL_BEGIN)
			:	sprintf(returnstr, "The SoftwareValet Transceiver began searching for updates.");
				break;
		case(Log::LOG_TROLL_END)
			:	sprintf(returnstr, "The SoftwareValet Transceiver finished searching for updates.");
				break;
		case(Log::LOG_VALET_LAUNCH)
			:	sprintf(returnstr, "SoftwareValet was launched");
				break;
		case(Log::LOG_VALET_QUIT)
			:	sprintf(returnstr, "SoftwareValet exited.");
				break;
		case(Log::LOG_INSTALL)
			:	sprintf(returnstr, "Installed package %s.", pkg);
				break;
		case(Log::LOG_UNINSTALL)
			:	sprintf(returnstr, "Uninstalled package %s.", pkg);
				break;
		case(Log::LOG_REGISTER)
			:	sprintf(returnstr, "Registered package %s.", pkg);
				break;
		case(Log::LOG_DOWNLOAD)
			:	sprintf(returnstr, "Finished downloading %s.", pkg);
				break;
		case(Log::LOG_RESUME_DOWNLOAD)
			:	sprintf(returnstr, "Resumed downloading %s.", pkg);
				break;
		case(Log::LOG_UPDATE_FOUND)
			:	sprintf(returnstr, "SoftwareValet found an update for %s.", pkg);
				break;
		case (Log::LOG_JAEGAR_CONNECT)
			:	sprintf(returnstr, "A message from the SoftwareValet Transceiver:");
				break;
		case (Log::LOG_MANUAL_UPDATE)
			:	sprintf(returnstr, "Searched for an update for %s", pkg);
				break;
		default	:return(NULL);
	}
	return(returnstr);
}

bool LogEntry::DisplayOrNot(int32 action) 
{
	int32 flag =0;
	switch(action) {
		case(Log::LOG_LISTENER_QUIT):
		case (Log::LOG_LISTENER_LAUNCH):
		case(Log::LOG_JAEGAR_CONNECT)
			:	flag = Log_Jaegar;
				break;
		case Log::LOG_VALET_QUIT:
		case(Log::LOG_VALET_LAUNCH)
			:	flag = Log_Valet;
				break;
		case(Log::LOG_INSTALL)
			:	flag = Log_Installations;
				break;
		case(Log::LOG_UNINSTALL)
			:	flag = Log_Uninstallations;
				break;
		case(Log::LOG_REGISTER)
			:	flag = Log_Registrations;
				break;
		case(Log::LOG_RESUME_DOWNLOAD):
		case(Log::LOG_DOWNLOAD)
			:	flag = Log_Downloads;
				break;
		case(Log::LOG_UPDATE_FOUND):
		case(Log::LOG_MANUAL_UPDATE)
			:	flag = Log_Updates;
				break;
		case(Log::LOG_TROLL_BEGIN):
		case(Log::LOG_TROLL_END)
			:	flag = Log_Trolling;
				break;
		default	:
			flag = 0;
	}

	return gSettings->data.FindInt32("logging/flags") & flag;
}
