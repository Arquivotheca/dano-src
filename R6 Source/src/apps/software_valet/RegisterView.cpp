// RegisterView.cpp

#include "RegisterView.h"
#include "RegInfoView.h"
#include "PackageDB.h"
#include "SimpleListView.h"
#include "RegisterThread.h"

#include "Util.h"
#include "LabelView.h"

#include "SettingsManager.h"
#include "PackageItem.h"
#include "ValetMessage.h"

#include <ctype.h>
#include <CheckBox.h>
#include <StatusBar.h>
#include <Button.h>
#include <ClassInfo.h>
#include <Control.h>
#include <String.h>
#include <Window.h>


#include "MyDebug.h"

extern SettingsManager	*gSettings;

void	EnableView(BView *parent, bool enable);


// pkgItem busy!, don't uninstall!
RegisterView::RegisterView(BRect b, BMessage &incurPkgs, BMessenger _updt,
		bool showSerial)
	:	BView(b,"registerview",B_FOLLOW_ALL,B_WILL_DRAW),
		curPkgs(incurPkgs),
		regWhen(M_REGNOW),
		needSerialID(showSerial),
		saveReg(false),
		updt(_updt),
		regThread(NULL)
{
	SetViewColor(light_gray_background);
	curReg = gSettings->reg;	// copy in the registration info
	
	type_code type;
	curPkgs.GetInfo("items",&type,&registerCount);
}

void RegisterView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	BRect r = Bounds();
	r.InsetBy(8,8);

	r.bottom = r.top + 16;
	BRect tr = r;
	//tr.left += 44;	// for icon
	
	char *buf = new char[256];
	
	if (registerCount == 1) {
		sprintf(buf,"Send the following registration information:");
	}
	else {
		sprintf(buf,"The following registration information will be sent for the\
 selected packages:");
	}
	AddChild(new LabelView(tr,buf));
 	
 	r.top = r.bottom + 12;
 	BView *v;
 	
 	
 	const char *sid;
 	if (needSerialID)
	 	sid = curPkgs.FindString("sid");
	else
		sid = NULL;
		
 	// view will resize
 	v = new RegInfoView(r,&curReg,&curReg,sid);
	AddChild(v);
	r = v->Frame();
	
	r.top = r.bottom + 4;
	r.left += 24;
	r.bottom = r.top + 18;
	/***
	BRadioButton *btn;
	btn = new BRadioButton(r,B_EMPTY_STRING,
						"Register now (connecting if necessary)",
						new BMessage(M_REGNOW),
						B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(btn);
	btn->SetFont(be_plain_font);
	btn->SetTarget(this);	
	btn->SetValue(B_CONTROL_ON);
	
	r.OffsetBy(0,18);
	
	btn = new BRadioButton(r,B_EMPTY_STRING,
						"Register at next server connection",
						new BMessage(M_REGLATER),
						B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(btn);
	btn->SetFont(be_plain_font);
	btn->SetTarget(this);
	btn->SetEnabled(false);
	***/
	const char *nstr = curReg.FindString("name");
	saveReg = !(nstr && *nstr);
	
	BCheckBox *cb = new BCheckBox(r,B_EMPTY_STRING,
						"Save this registration information",
						new BMessage(M_SAVEREG),
						B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(cb);
	cb->SetTarget(this);
	cb->SetValue(saveReg ? B_CONTROL_ON : B_CONTROL_OFF);
	cb->SetFont(be_plain_font);
	
	r.OffsetBy(-20,24);
	
	r.bottom = r.top + 60;
	
	BStatusBar *bar = new BStatusBar(r,"status");
	bar->SetBarHeight(8.0);
	AddChild(bar);
	bar->Hide();
	
	r = Bounds();
	r.InsetBy(8,12);
	r.top = r.bottom - 20;
	r.left = r.right - 75;
	BButton *b = new BButton(r,"regok","Register",new BMessage(M_REGOK),
						B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(b);
	b->SetFont(be_plain_font);
	b->SetTarget(this);
	b->MakeDefault(TRUE);

	r.right = r.left - 20;
	r.left = r.right - 75;
	b = new BButton(r,B_EMPTY_STRING,"Not Now",new BMessage(B_QUIT_REQUESTED),
						B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(b);
	b->SetFont(be_plain_font);
}

bool	validEmail(const char *str);
int		validChars(const char *str);

bool RegisterView::ValidateRegInfo(BMessage *msg)
{
	// name, requireed, two chars
	// email, opt, xxx@xxx
	const char *str;
	bool vEmail = true;
	bool vName = true;
	bool vSerial = true;
	
	msg->FindString("name",&str);
	if (!str || (*str == '\0') || (validChars(str) < 2)) {
		// name is invalid
		vName = false;
	}
	msg->FindString("email",&str);
	if (str && *str) {
		if (!validEmail(str)) {
			// email is invalid
			vEmail = false;
		}
	}
	if (needSerialID) {
		// check to see if the serial is not found or blank - note that we don't actually
		// check to see if it is a valid serial number -- the server does the real check
	 	if (msg->FindString("sid", &str) == B_OK) {
			if (*str == '\0') {
				// a new serial number was entered by the user, and it was blank
				vSerial = false;
			}	
		} else if (curPkgs.FindString("sid", &str) != B_OK || *str == '\0') {
			// no new serial number was entered, and either there was not an old number
			// or the old number was blank
			vSerial = false;
		}
	}
	if (vEmail && vName && vSerial) {
		return true;
	} else {
		BString errMsg;
		errMsg << "There are problems with the registration information you have provided.\n\n";
		if (!vName) {
			errMsg << "- You must enter a valid name\n";
		}
		if (!vEmail) {
			errMsg << "- You must enter a valid email address\n";
		}
		if (!vSerial) {
			errMsg << "- You must enter a valid serial number\n";
		}
		doError(errMsg.String(), NULL, "Continue");
		return false;
	}
	
}

bool validEmail(const char *str)
{
	const char *c = str;
	// count to '@'
	while (*c && *c != '@') c++;
	if (*c != '@') return false;
	
	if (!(c - str)) return false;
	c++;	// skip @
	str = c;
	while(*c && *c != '.') c++;
	if (*c != '.') return false;
		
	if (!(c - str)) return false;
	c++;	// skip .
	str = c;
	while(*c) c++;
	if (!(c - str)) return false;
		
	return true;
}

int	validChars(const char *str)
{
	int res = 0;
	const char *c = str;
	while (*c) {
		if (! (isspace(*c) || ispunct(*c)))
			res++;
		c++;
	}
	return res;
}

void RegisterView::MakeEditable(bool state)
{
	EnableView(this,state);
	BControl *c = cast_as(FindView("regok"),BControl);
	c->SetEnabled(true);
	if (state) {
		c->SetLabel("OK");
		c->Message()->RemoveName("cancel");
	}
	else {
		c->SetLabel("Cancel");
		c->Message()->AddBool("cancel",true);
	}
	BStatusBar *bar = (BStatusBar *)FindView("status");
	if (bar) {
		if (state)
			bar->Hide();
		else {
			bar->SetMaxValue(3.0 * registerCount);
			bar->Show();
		}
	}
}

void RegisterView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_SAVEREG:
			saveReg = !saveReg;
			break;
		//case M_REGLATER:
		//	regWhen = M_REGLATER;
		//	break;
		//case M_REGNOW:
		//	regWhen = M_REGNOW;
		//	break;
		case M_REGOK: {
			if (msg->HasBool("cancel")) {
				PRINT(("cancel it!\n"));
				if (regThread) {
					PRINT(("interrupting net\n"));
				
					regThread->htconn.Interrupt();
					regThread = NULL;
				}
				return;
			}
			
			// make sure any text gets added to the message
			BTextView *tv = cast_as(Window()->CurrentFocus(),BTextView);
			if (tv) {
				tv->MakeFocus(FALSE);
				Looper()->DetachCurrentMessage();
				Looper()->PostMessage(msg,this);
				break;
			}
			
			if (!ValidateRegInfo(&curReg))
				break;
			
			const char *sid = curReg.FindString("sid");
			if (sid) {
				// get user entered serial number (if any)
				// only for 1 selected package (no multiple selection)
				curPkgs.RemoveName("sid");
				curPkgs.AddString("sid",sid);
			}
			// clear out sid from the reg information before saving
			curReg.RemoveName("sid");
			
			// save the registration data preference
			if (saveReg) {
				PRINT(("saving registration info\n"));
				gSettings->reg = curReg;
				gSettings->SaveSettings();
			}
			
			MakeEditable(false);
			
			// spawn a thread to do the network stuff
			BMessenger tmp(this);
			regThread = new RegisterThread(&curPkgs, &curReg, tmp);
			regThread->Run();
			break;
		}
		case F_REG_CONNECT: {
			BStatusBar *bar = (BStatusBar *)FindView("status");
			if (bar) bar->Update(0.5,"Trying to connect...");
			break;
		}
		case F_REG_SENDING: {
			BStatusBar *bar = (BStatusBar *)FindView("status");
			if (bar) bar->Update(1.5,"Sending registration info...");
			break;
		}
		case F_REG_DONE: {
			BStatusBar *bar = (BStatusBar *)FindView("status");
			if (bar) bar->Update(1.0,"Registration complete");
		
			msg->what = 'PDis';
			updt.SendMessage(msg);
			break;
		}
		case F_REG_ALLDONE: {
			regThread = NULL;
			
			BStatusBar *bar = (BStatusBar *)FindView("status");
			if (bar) bar->Update(INT_MAX,"Registration complete");
			snooze(1000*600);
			Looper()->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case F_REG_ERROR: {
			// XXX: take this out
//			printf("register error status = %d\n", msg->FindInt32("status"));
			regThread = NULL;
			
			BStatusBar *bar = (BStatusBar *)FindView("status");
			
			const char *err = msg->FindString("message");
			if (!err) err = "Error sending registration";
			
			if (bar) bar->Update(INT_MAX,err);
			doError(err);
			
			if (msg->FindInt32("status") == -2 && registerCount == 1) {
				// bad serial number, let them retype
				MakeEditable(true);	
			}
			else {
				Looper()->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default: {
			BView::MessageReceived(msg);
			break;
		}
	}
}

void	EnableView(BView *parent, bool enable)
{
	BView	*v;
	int i = 0;
	while(v = parent->ChildAt(i++)) {
		BControl	*c;
		c = cast_as(v,BControl);
		if (c)	c->SetEnabled(enable);
		else EnableView(v,enable);
	}
}
