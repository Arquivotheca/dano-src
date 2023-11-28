//  FileAndPrintPanel.cpp
//
//	russ 1/15/99
// 
//	(c) 1997-99 Be, Inc. All rights reserved


#include <stdio.h>
#include <string.h>
#include <unistd.h> //crypt
#include <ctype.h>
#include <stdlib.h>

#include <Alert.h>
#include <Box.h>
#include <MessageFilter.h>
#include <Screen.h>
#include <TextControl.h>
#include <CheckBox.h>
#include <netconfig.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <Entry.h>
#include <Button.h>

#include <interface_misc.h>

#include "FileAndPrintApp.h"
#include "FileAndPrintPanel.h"
#include "Resource.h"
#include "smbdes.h"

// Becuase these const are being used for login panel also,
// used kFP* to make unique.
const char* kFPNoMatchStr = "Passwords don't match. Try again.";
const char* kFPEmptyPwdStr = "Empty password! Are you sure?";
const char* kFPOKStr = "OK";
const char* kFPCancelStr = "Cancel";
const char* kFPDoneStr = "Done";
const char* kEnableCifs = "File and Printer Sharing";
const char* kFPConfirmPwdStr = "Confirm password:";
const char* kFPPwdStr = "Remote password:";
const char* kFPUserName = "Remote user name:";
const char* kWorkgroupName = "Workgroup name:";
const char* khideShares = "Show hidden mounts";

const int32 kFPPanelWidth = 280;
const int32 kFPPanelHeight = 195;

static void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

FileAndPrintPanel::FileAndPrintPanel()
	:BWindow(BRect(0,0,kFPPanelWidth, kFPPanelHeight),
		"World O' Networking Setup",  B_TITLED_WINDOW,
		 B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	
	strcpy(m_WorkgroupName, "WORKGROUP");
	strcpy(m_FPUserName, "guest");
	strcpy(m_Password, "");
		
	GetNetworkSettings();
	
    CreateFileAndPrintUI();
    
    Update(kFillControls);

	CenterWindowOnScreen(this);
	Show();	

	Lock();
    m_workgroup->MakeFocus(true);
	Unlock();
}

FileAndPrintPanel::~FileAndPrintPanel()
{
}

bool
FileAndPrintPanel::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
FileAndPrintPanel::MessageReceived(BMessage *msg) 
{
	status_t status;

	switch (msg->what) {
		case MSG_PASSWORD_TEXT:
		case MSG_CONFIRM_PSWD_TEXT:
			break;
		
		//case MSG_UPDATE_CONTROLS:
		//	Update(kFillControls);
		//	break;

		case MSG_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
		
		case MSG_DONE:
			if (B_CONTROL_ON  == m_cifsenable->Value()){
				status = check_password();
				if (status >= B_NO_ERROR) {
					Update(kEmptyControls);
					SaveNetworkSettings();
					RestartNetworking();			
					PostMessage(B_QUIT_REQUESTED);
				}
				m_pass1->MakeFocus(TRUE);
			}
			else{
				Update(kEmptyControls);
					SaveNetworkSettings();
					RestartNetworking();			
					PostMessage(B_QUIT_REQUESTED);
			}
			break;
		
		case MSG_CIFSENABLE:	
		   	if (B_CONTROL_ON  == m_cifsenable->Value()){
			   	m_workgroup->SetEnabled(true);
				m_hideShares->SetEnabled(true);
	       		m_user->SetEnabled(true);	
				m_pass1->SetEnabled(true);
				m_pass2->SetEnabled(true);	
			 }
			 else{
				m_workgroup->SetEnabled(false);
				m_hideShares->SetEnabled(false);
	       		m_user->SetEnabled(false);	
				m_pass1->SetEnabled(false);
				m_pass2->SetEnabled(false);	
			 }
		break;
		
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

int
FileAndPrintPanel::check_password(void)
{
	BAlert *alert;

	if (strcmp(m_pass1->Text(), m_pass2->Text()) != 0) {
		alert = new BAlert("", kFPNoMatchStr, kFPOKStr);
		alert->Go();
		return (B_ERROR);
	}
	if (*m_pass1->Text() == 0) {
		alert = new BAlert("", kFPEmptyPwdStr, kFPCancelStr, kFPOKStr);
		if (alert->Go() == 0) {
			return (B_ERROR);
		}
		
	}
	return (B_NO_ERROR);
}

void 
FileAndPrintPanel::CreateFileAndPrintUI()
{
	BTextView *tv;
		
	BRect rect(Bounds());
	rect.InsetBy(-1.0, -1.0);
	m_background = new BBox(rect);
	m_background->SetFont(be_plain_font);
	AddChild(m_background);
	
	rect.left = 10;
	rect.right = Bounds().Width() - 11;
    rect.top = 12; rect.bottom = 13;
	m_cifsenable = new BCheckBox(rect, "", kEnableCifs, 
		new BMessage(MSG_CIFSENABLE));
	m_background->AddChild(m_cifsenable);
	
	rect.top = m_cifsenable->Frame().bottom + 8;
	rect.bottom = rect.top + m_cifsenable->Frame().Height();
	m_hideShares = new BCheckBox(rect, "", khideShares, 
		new BMessage(MSG_HIDESHARES));
	m_background->AddChild(m_hideShares);

	rect.top = m_hideShares->Frame().bottom + 8;
	rect.bottom = rect.top + m_hideShares->Frame().Height();
	m_workgroup = new BTextControl(rect, "", kWorkgroupName, "", new BMessage(MSG_UPDATE_CONTROLS));
    m_workgroup->SetDivider(m_background->StringWidth(kFPConfirmPwdStr)+10);
    m_workgroup->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
    m_background->AddChild(m_workgroup);
	
    rect.top = m_workgroup->Frame().bottom + 5;
	rect.bottom = rect.top + m_workgroup->Frame().Height();
	m_user = new BTextControl(rect, "", kFPUserName, "", new BMessage(MSG_UPDATE_CONTROLS));
    m_user->SetDivider(m_background->StringWidth(kFPConfirmPwdStr)+10);
    m_user->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
    m_background->AddChild(m_user);
      
	rect.top = m_user->Frame().bottom + 5;
	rect.bottom = rect.top + m_user->Frame().Height();
    m_pass1 = new BTextControl(rect, "", kFPPwdStr, "", new BMessage(MSG_PASSWORD_TEXT));
    m_pass1->SetDivider(m_background->StringWidth(kFPConfirmPwdStr)+10);
    m_pass1->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)m_pass1->ChildAt(0))->HideTyping(true);
    m_background->AddChild(m_pass1);
    
	rect.top = m_pass1->Frame().bottom + 5;
	rect.bottom = rect.top + m_pass1->Frame().Height();
	m_pass2 = new BTextControl(rect, "", kFPConfirmPwdStr, "", new BMessage(MSG_CONFIRM_PSWD_TEXT));
	m_pass2->SetDivider(m_background->StringWidth(kFPConfirmPwdStr)+10);
	m_pass2->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)m_pass2->ChildAt(0))->HideTyping(true);
    m_background->AddChild(m_pass2);

	rect.right = Bounds().Width() - 10;
	rect.left = rect.right - 75;
	rect.top = m_pass2->Frame().bottom + 10;
	rect.bottom = rect.top + 1;
	m_okbut = new BButton(rect, "", kFPDoneStr, new BMessage(MSG_DONE));
	
	rect.right = rect.left - 10;
	rect.left = rect.right - 75;
    m_cancelbut = new BButton(rect, "", kFPCancelStr, 
							new BMessage(MSG_CANCEL));
	m_background->AddChild(m_cancelbut);
    
	m_okbut->MakeDefault(true);
	m_background->AddChild(m_okbut);

}

void
FileAndPrintPanel::Update(int16 direction)
{		
    // Load up the controls
    if (direction == kFillControls){
		m_cifsenable->SetValue(true);
		m_hideShares->SetValue(m_hideSharesFlag);
    	m_workgroup->SetText(m_WorkgroupName);
    	m_user->SetText(m_FPUserName);
	}
    else{      
		SetPassword(m_pass1->Text());        
        strcpy(m_WorkgroupName, m_workgroup->Text()); 
		strcpy(m_FPUserName, m_user->Text());
    }
}

void
FileAndPrintPanel::SetPassword(const char *pass)
{
	// Encrypt and store password
	unsigned char p14[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char p21[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		
	char UPpass[64];
	
	if (pass == NULL) {
		return;
	}

	if (*pass == '\0') {
		memset(m_Password, 0, kMaxStrLen);
		return;
	}
	
	strncpy(UPpass, pass, 64);
	strncpy(m_ClearTextPass, pass, kMaxStrLen);
		
	int len = strlen(UPpass);
	for(int i = 0; i <= len; i++) {
		UPpass[i] = toupper(UPpass[i]);
	}
	if (strlen(UPpass) <= 14) {
		memcpy(p14, UPpass, strlen(UPpass));
	} else {
		memcpy(p14, UPpass, 14);
	}
	
	E_P16(p14, p21);
	
	sprintf(m_Password, "%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|",
		p21[0],p21[1],p21[2],p21[3],p21[4],p21[5],p21[6],p21[7],p21[8],p21[9],
		p21[10],p21[11],p21[12],p21[13],p21[14],p21[15],p21[16],p21[17],p21[18],p21[19],
		p21[20],p21[21]);
	
	return;
}

void
FileAndPrintPanel::GetNetworkSettings()
{
 	char buf[kMaxStrLen];
	
	strcpy(buf, "");
	m_hideSharesFlag = false;
	netconfig_find("ms_sharing", "MSSHOWHIDDENSHARES", buf, kMaxStrLen);
	if (strcmp(buf, "1") == 0)
		m_hideSharesFlag = true;
		
	netconfig_find("ms_sharing", "MSWORKGROUP", m_WorkgroupName, kMaxStrLen);
	netconfig_find("ms_sharing", "MSUSERNAME", m_FPUserName, kMaxStrLen);
	netconfig_find("ms_sharing", "MSPASSWORD", m_Password, kMaxStrLen);	
}

void
FileAndPrintPanel::SaveNetworkSettings()
{
	char buf[512];
	char sysbuf[512];
	char* c;
	app_info info;
	BPath path;	
	FILE *fsrc;	
		
	be_app->GetAppInfo(&info);
	BEntry entry(&info.ref);
	entry.GetPath(&path);
	strcpy(buf, path.Path());
	c = strrchr(buf, '/');
	
	// XXX alfred - working around multiuser
	setuid(0);
	setgid(0);
	
	if (B_CONTROL_ON  == m_cifsenable->Value()){
		strcpy(c, "/Netscript.CIFS");
		// Make sure to have the Netscript.CIFS in same directory as this binary
		fsrc = fopen(buf, "r");
		if (fsrc == NULL){
			BAlert *alert;		
     		alert = new BAlert("", "Problems opening Netscript.CIFS. Netscript.CIFS\
									must be in the same directory as this program.", "OK");
     		alert->Go();
            exit(0);
		}  
		fclose(fsrc);
			    
		system("mv /boot/beos/system/boot/Netscript /boot/beos/system/boot/Netscript.old");
        sprintf(sysbuf, "cp %s /boot/beos/system/boot/Netscript", buf);
		system(sysbuf);

		strcpy(c, "/cifs.zip       ");
		sprintf(sysbuf, "unzip -o -qq %s -d /boot", buf);
		system(sysbuf);

#ifdef INSTALL_SAMBA
		// Adjust the samba workgroup
		sprintf(sysbuf, "sed --expression=\"s/workgroup *= *[a-zA-Z]*/workgroup=%s/\" /boot/beos/system/servers/samba/lib/smb.conf > /tmp/smb.conf.swap", m_WorkgroupName);
		system(sysbuf);
		system("mv /tmp/smb.conf.swap /boot/beos/system/servers/samba/lib/smb.conf");
		
		
		// Do a "smbpasswd -a username password"
		sprintf(buf, "./_installscript %s %s\0", m_FPUserName, m_ClearTextPass);
		system(buf);
#endif				
		

	}
	else{
		strcpy(c, "/Netscript.GENERIC");
		sprintf(sysbuf, "cp %s /boot/beos/system/boot/Netscript", buf);
		system(sysbuf);
	}
	
	// Write section if doesn't exits
	netconfig_set("ms_sharing", NULL, NULL);
	sprintf(buf, "%i", m_hideShares->Value());
	netconfig_set("ms_sharing", "MSSHOWHIDDENSHARES", buf);
	netconfig_set("ms_sharing", "MSWORKGROUP", m_WorkgroupName);
	netconfig_set("ms_sharing", "MSUSERNAME", m_FPUserName);
	netconfig_set("ms_sharing", "MSPASSWORD", m_Password);
	
}

int 
FileAndPrintPanel::GetNextFieldStr(char* instring, char* outstring)
{                                  
  char* ptr;
  char* ptr1;
  char* ptr2;    
   
   ptr  = instring;
   
   if((ptr1 = strchr(ptr, ' ')) == NULL){
       strcpy(outstring, instring);
       return (0);
   }
     
   *ptr1 = '\0';                         
   ptr2 = ++ptr1;
       
   for(int n = NC_MAXVALLEN; n >= 0; n--){  //Trim trailing blanks
          ptr1--;
          if (*ptr1 == ' ') 
             *ptr1 = '\0';
          else
             break;     
    }       
             
   strcpy(outstring, ptr);  	
   strcpy(instring, ptr2);	      	      
       
   return (1);                            
}

static int
roster_quit(char *sig)
{
	app_info info;
	if (be_roster->GetAppInfo(sig, &info) < B_NO_ERROR) {
		/*
		 * Already quit
		 */
		return (1);
	}
	// ??? just make the BMessenger on the stack!
	BMessenger *app = new BMessenger((char*)0, info.team);
	app->SendMessage(B_QUIT_REQUESTED);

	int32 i;
	for (i = 0; i < 10; i++) {
		if (!be_roster->IsRunning(sig)) {
			break;
		}
		sleep(1);
	}
	//
	// it's possible the net_server is wedged so bad that the above doesn't work.
	// here we get medevial on it
	//
	status_t rv = B_NO_ERROR;	
	if ((i >= 10) && (info.team >= 0)) {
		rv = kill_team(info.team);
		sleep(5);
	}	
	delete app;
	return (rv == B_NO_ERROR);
}

void
FileAndPrintPanel::RestartNetworking()
{
 	BAlert* alert;

	if (1/*m_Dirty*/){
			alert = new BAlert("restart",
				"File and print sharing changes require a networking restart. Restart networking?", 
					   "Cancel", "Restart", NULL, B_WIDTH_FROM_WIDEST);
			if ((alert->Go() != 1))
						return;
						
			// Run Netscript /////////////////////////////
			BPath script_path;
			find_directory (B_BEOS_BOOT_DIRECTORY, &script_path);
			script_path.Append ("Netscript");
			if (roster_quit("application/x-vnd.Be-NETS")) {
				char *argv[3];
				argv[0] = "/bin/sh";
				argv[1] = (char *)script_path.Path();
				argv[2] = NULL;
				resume_thread(load_image(2, (const char**)argv,
			 								(const char**)environ));
			} else {
				BAlert *alert = new BAlert("done", "Can't restart networking", "OK",
						   NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go();
			}
			/////////////////////////////////////////////////////
	}
}
