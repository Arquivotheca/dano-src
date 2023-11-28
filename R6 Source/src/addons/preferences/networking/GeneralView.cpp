//  GeneralView.cpp
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#include "GeneralView.h"
#include "InterfaceAddOn.h"
#include "NetworkingCore.h"
#include "Settings.h"
#include "Resource.h"
#include "NetDevScan.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Alert.h>
#include <Box.h>
#include <CheckBox.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Button.h>
#include <TextControl.h>

const char* kFTPServerStr = "FTP server";
const char* kTelnetServerStr = "Telnet server";
const char* kAppleTalkStr = "AppleTalk";
const char* kIPForwardingStr = "IP forwarding";
const char* kConfigurationsStr = "Configurations";

GeneralView::GeneralView(PPAddOn *adn, image_id img, PPWindow *w)
 : BView(BRect(0, 0, kPanelViewWidth, kPanelViewHeight), "general", B_FOLLOW_NONE, B_WILL_DRAW),
	addon(adn), image(img), window(w)
{	
	core = new NetworkingCore();

	CreateMiscUI();
	BuildNetCards();
}

void GeneralView::BuildNetCards()
{
	// Loop through list of physical interfaces in machine.
	int32 detected = core->fNetDevScanList->CountItems();
	int32 i;
	for(i = 0; i < detected; i++)
	{
		TNetDevScanItem *scan = (TNetDevScanItem *)core->fNetDevScanList->ItemAt(i);

		// For isa jumpered cards the cards are hard coded into
		// netdevscan, but not shown unless the user adds the
		// interface. 
		if(!scan->Display())
			continue; 

		const char *name = scan->LinkName();
		if(!name)
			continue;

		// Do not add interface if already exists.
		bool exist = false;
		int32 count = core->settings.interfacelist.CountItems();
		InterfaceSettings *item = 0;
		int32 t = 0;
		if(count)
		{
			for(t = 0; t < count; t++)
			{
				item = (InterfaceSettings *)core->settings.interfacelist.ItemAt(t);
				if(strcmp(item->m_LinkName, name) == 0)
				{
					exist = true;
					break;
				}
			}
		}

		if(! exist)
		{
			// initial values for autodetected cards (plug and play or PCI)
			item = new InterfaceSettings();
			strcpy(item->m_InterfaceName, GetUniqueInterfaceName());
			strcpy(item->m_AddOnName, scan->NetDevScanName());
			strcpy(item->m_LinkName, scan->LinkName());
			strcpy(item->m_PrettyName, scan->InterfaceName());	
			core->settings.interfacelist.AddItem(item);
			t = core->settings.interfacelist.IndexOf(item);
		}

		item->m_Jumpered = false;	// this was autodetected so it's not jumpered
		item->m_Added = true;
		addon->UseNewPanel(new InterfaceAddOn(image, window, core, t, !exist));
	}

   	// Loop through settings interfaces and add the undetected ones
   	int32 cnt = core->settings.interfacelist.CountItems();    			 				
   	for(i = 0; i < cnt; i++)
	{
   	    InterfaceSettings *item = (InterfaceSettings *)core->settings.interfacelist.ItemAt(i);
		if(! item->m_Added)
		{
			// is this a jumpered device?
			item->m_Jumpered = false;
			for(int32 j = 0; j < detected; j++)
			{
				TNetDevScanItem *scan = (TNetDevScanItem *)core->fNetDevScanList->ItemAt(j);
				if(strcmp(scan->LinkName(), item->m_LinkName) == 0)
				{
					item->m_Jumpered = ! scan->Display();
					break;
				}
			}

			item->m_Added = true;
			addon->UseNewPanel(new InterfaceAddOn(image, window, core, i, false));
		}
   	}
}

bool GeneralView::QuitRequested()
{
	bool	really;

	if((really = core->TryToSave(true)) == true)
		core->settings.Unsubscribe(this);

	return really;
}

void GeneralView::AttachedToWindow()
{
    BView::AttachedToWindow(); 

	core->settings.Subscribe(this);	// automatically loads settings

	m_FTPServer->SetTarget(this);
	m_TelnetServer->SetTarget(this);
	m_IPForwarding->SetTarget(this);
	m_AppleTalk->SetTarget(this);
	m_AddHWButton->SetTarget(this);
	m_UserName->SetTarget(this);
	m_Password1->SetTarget(this);
	m_Password2->SetTarget(this);
	m_RestartButton->SetTarget(core);
	m_SaveButton->SetTarget(core);
	m_RevertButton->SetTarget(core);
}

void GeneralView::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case MSG_DIRTY:
		    core->settings.SetDirty(this, true);
			break;	

		case 'adhw' :
			{
			// Add Jumpered device
			printf("TODO: add jumpered device\n");
			}
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void GeneralView::CreateMiscUI()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BBox *services = new BBox(BRect(0, 0, kPanelViewWidth, 105));
	services->SetLabel("Host Services");
	AddChild(services);

	m_FTPServer = new BCheckBox(BRect(20, 20, kPanelViewWidth / 2 - 10, 35), "", kFTPServerStr, new BMessage(MSG_DIRTY),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	services->AddChild(m_FTPServer);

	m_TelnetServer = new BCheckBox(BRect(20, 45, kPanelViewWidth / 2 - 10, 60), "", kTelnetServerStr, new BMessage(MSG_DIRTY),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	services->AddChild(m_TelnetServer);

	m_UserName = new BTextControl(BRect(kPanelViewWidth / 2 - 50, 20, kPanelViewWidth - 25, 40), "user", "User name:", "", new BMessage(MSG_DIRTY));
	m_UserName->SetDivider(100);
	((BTextView *)(m_UserName->ChildAt(0)))->SetMaxBytes(sizeof(core->settings.m_UserName) - 1);
	m_UserName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	m_UserName->SetModificationMessage(new BMessage(MSG_DIRTY));
	services->AddChild(m_UserName);

	m_Password1 = new BTextControl(BRect(kPanelViewWidth / 2 - 50, 45, kPanelViewWidth - 25, 60), "pass1", "Password:", "", new BMessage(MSG_DIRTY));
	m_Password1->SetDivider(100);
	m_Password1->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	((BTextView *)(m_Password1->ChildAt(0)))->SetMaxBytes(8);
	((BTextView *)(m_Password1->ChildAt(0)))->HideTyping(true);
	m_Password1->SetModificationMessage(new BMessage(MSG_DIRTY));
	services->AddChild(m_Password1);

	m_Password2 = new BTextControl(BRect(kPanelViewWidth / 2 - 50, 70, kPanelViewWidth - 25, 85), "pass2", "Confirm Password:", "", new BMessage(MSG_DIRTY));
	m_Password2->SetDivider(100);
	m_Password2->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	((BTextView *)(m_Password2->ChildAt(0)))->SetMaxBytes(8);
	((BTextView *)(m_Password2->ChildAt(0)))->HideTyping(true);
	m_Password2->SetModificationMessage(new BMessage(MSG_DIRTY));
	services->AddChild(m_Password2);

	BBox *other = new BBox(BRect(0, 125, kPanelViewWidth, 173));
	other->SetLabel("Other Services");
	AddChild(other);

	m_AppleTalk = new BCheckBox(BRect(20, 20, kPanelViewWidth / 2 - 10, 35), "", kAppleTalkStr, new BMessage(MSG_DIRTY),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	other->AddChild(m_AppleTalk);

	m_IPForwarding = new BCheckBox(BRect(kPanelViewWidth / 2 + 35, 20, kPanelViewWidth - 35, 35), "", kIPForwardingStr, new BMessage(MSG_DIRTY),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	other->AddChild(m_IPForwarding);

	m_AddHWButton = new BButton(BRect(kPanelViewWidth - 160, kPanelViewHeight - 70, kPanelViewWidth, kPanelViewHeight - 50), "add", "Add undetected hardware" B_UTF8_ELLIPSIS, new BMessage('adhw'));
	AddChild(m_AddHWButton);

	BView *line = new BView(BRect(0, kPanelViewHeight - 36, kPanelViewWidth, kPanelViewHeight - 35), B_EMPTY_STRING, B_FOLLOW_NONE, 0);
	line->SetViewColor(131, 131, 131);
	AddChild(line);

	m_RestartButton = new BButton(BRect(0, kPanelViewHeight - 25, 120, kPanelViewHeight - 5), "restart", "Restart Networking", new BMessage(MSG_RESTARTNETWORKING));
	AddChild(m_RestartButton);

	m_RevertButton = new BButton(BRect(130, kPanelViewHeight - 25, 205, kPanelViewHeight - 5), "revert", "Revert", new BMessage(MSG_REVERT));
	AddChild(m_RevertButton);

	m_SaveButton = new BButton(BRect(kPanelViewWidth - 75, kPanelViewHeight - 25, kPanelViewWidth, kPanelViewHeight - 5), "restart", "Save", new BMessage(MSG_SAVE));
	AddChild(m_SaveButton);
}

bool GeneralView::LockSubscriber()
{
	return LockLooper();
}

void GeneralView::UnlockSubscriber()
{
	UnlockLooper();
}

void GeneralView::CanRevert(bool flag)
{
	m_RevertButton->SetEnabled(flag);
}

void GeneralView::CanSave(bool flag)
{
	m_SaveButton->SetEnabled(flag);
}

void GeneralView::LoadSettings()
{
	m_FTPServer->SetMessage(0);
	m_TelnetServer->SetMessage(0);
	m_UserName->SetMessage(0);
	m_UserName->SetModificationMessage(0);
	m_AppleTalk->SetMessage(0);
	m_IPForwarding->SetMessage(0);

	m_FTPServer->SetValue(atol(core->settings.m_FTPEnabled));
	m_TelnetServer->SetValue(atol(core->settings.m_TelnetdEnabled));
	m_UserName->SetText(core->settings.m_UserName);
	m_AppleTalk->SetValue(atol(core->settings.m_AppleTalkEnabled));
	m_IPForwarding->SetValue(atol(core->settings.m_IPForward));

	m_FTPServer->SetMessage(new BMessage(MSG_DIRTY));
	m_TelnetServer->SetMessage(new BMessage(MSG_DIRTY));
	m_UserName->SetMessage(new BMessage(MSG_DIRTY));
	m_UserName->SetModificationMessage(new BMessage(MSG_DIRTY));
	m_AppleTalk->SetMessage(new BMessage(MSG_DIRTY));
	m_IPForwarding->SetMessage(new BMessage(MSG_DIRTY));
}

void GeneralView::UnloadSettings()
{
	char buf[NC_MAXVALLEN];

	sprintf(buf, "%ld", m_FTPServer->Value());
	strcpy(core->settings.m_FTPEnabled, buf);
   	sprintf(buf, "%ld", m_TelnetServer->Value());
   	strcpy(core->settings.m_TelnetdEnabled, buf);
   	sprintf(buf, "%ld", m_AppleTalk->Value());
   	strcpy(core->settings.m_AppleTalkEnabled, buf);
   	sprintf(buf, "%ld", m_IPForwarding->Value());
   	strcpy(core->settings.m_IPForward, buf); 
	strcpy(core->settings.m_UserName, m_UserName->Text());

	const char *pass1 = m_Password1->Text();
	const char *pass2 = m_Password2->Text();
	if(strcmp(pass1, pass2) == 0)
	{
		// TODO: encrypt the password
//		strcpy(core->settings.m_Password, pass1);
	}
}


// Function to get the next unique interface name
// Called when adding a new interface. Fills the
// m_InterfaceName with the unique name
const char *GeneralView::GetUniqueInterfaceName()
{
	static char name[32];

	int32 cnt = core->settings.interfacelist.CountItems();	
	if(cnt == 0)
		return "interface0";

	InterfaceSettings *intf;
	int32 j = 0;
	for(int32 i = 0; i < cnt; i++)
	{
		// Try a unique name for interface: interfaceX
		sprintf(name, "interface%ld", i);

		// interface1 interface0: unordered list so need 2
		// passes to make sure that the interface name is unique
		for(j = 0; j < cnt; j++)
		{
			intf = (InterfaceSettings *)core->settings.interfacelist.ItemAt(j);	
			if(strcmp(name, intf->m_InterfaceName) == 0)	
				break;	// Match found try interface(+1)				
		}

		// Interface item are out of order and if one is deleted it
		// should be reused, so if inner "j" loop goes through list
		// and interfaceX not in list break and use interfaceX
		if(j == (cnt + 1)) 
			break;
	} 

	return name;
}

#if 0
void GeneralView::AddInterfaces()
{
	int i, t;
	char* name;
	char dispbuf[kMaxStrLen];
	char interfaceStatus[kMaxStrLen];
		
  	// Loop through interfaces from settings.
  	core->settings.m_InterfaceItemList.ToHead();
  	int32 cnt = core->settings.m_InterfaceItemList.GetCount();    			 				
	InterfaceItem* pInterfaceItem;
	  	for(i = 0; i < cnt; i++){
	  	    pInterfaceItem = core->settings.m_InterfaceItemList.Get();
	
		// Add any interface that is not deleted
	  	    if (!pInterfaceItem->m_Deleted){
			strcpy(interfaceStatus, "Not present");
			if (IsInterfacePresent(pInterfaceItem->m_PrettyName))
				if (pInterfaceItem->m_Enabled)
					strcpy(interfaceStatus, "Ready");
				else
					strcpy(interfaceStatus, "Disabled");
					
	  	    	sprintf(dispbuf, "%s->%s->%s", pInterfaceItem->m_IPAddress,
											pInterfaceItem->m_PrettyName,
																interfaceStatus);  
	
	  	 		m_NetList->AddItem(
				new TInterfaceItem(dispbuf, pInterfaceItem->m_PrettyName));
					//gNetDevScanList->InterfaceName(pInterfaceItem->m_AddOnName))); 
				
			}
			core->settings.m_InterfaceItemList.ToNext();
	  	}
	
	// Loop through list of physical interfaces in machine.
	// Check if interface is already in list, if so, do not
	// add.
	bool exist;
	for (i = 0; i < gNetDevScanList->CountItems(); i++){
		
		// For isa jumpered cards the cards are hard coded into
		// netdevscan, but not shown unless the user adds the
		// interface. 
		if (!(gNetDevScanList->ItemAt(i))->Display())
			continue; 
	
		name = gNetDevScanList->InterfaceName(i);
		if (!name)
			continue;
		
		// Do not add interface if already exists.
		exist = false;
		for (t = 0; t <	m_NetList->CountItems(); t++){
			item = (TInterfaceItem*)m_NetList->ItemAt(t);
			if (item) {
				const char* iName = item->InterfaceName();
	
				if (iName && (strcmp(iName, name) == 0)) {					
					exist = true;
					break;
				}					
			}			
		}
		
		if (exist)				
			continue;				
		
		sprintf(dispbuf, "0.0.0.0->%s->Not configured", name);													
		m_NetList->AddItem(new TInterfaceItem(dispbuf, name, false));	
	}					
	
	SetSelectionByName(m_currSelectedItem);   		
}

bool GeneralView::IsInterfacePresent(char* name) 
{
	char* NetDevScanName;

	// Loop through list of physical interfaces in machine.
	// Check if interface is already in list, if so, do not
	// add.
	for (int32 i = 0; i < core->CountItems(); i++){
		NetDevScanName = core->InterfaceName(i);
		if (!NetDevScanName)
			continue;
		
		if (strcmp(NetDevScanName, name) == 0)
			return true;		
	 }

	return false;
}
#endif