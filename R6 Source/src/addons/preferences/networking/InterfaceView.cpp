//  InterfaceView.cpp
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#include "InterfaceView.h"

#include "Resource.h"
#include "netconfig.h"
#include "net_settings.h" 
#include "Settings.h"
#include "NetDevScan.h"
#include "NetworkingCore.h"
#include "PPAddOn.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Alert.h>
#include <Box.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>
#include <TextControl.h>
#include <NetDevice.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <Roster.h>
#include <Button.h>
#include <Path.h>
#include <StringView.h>

#include <image.h>

const char* kDomainStr = "Domain name:";
const char* kHostName = "Host name:";

const char* kBtnConfig = "Config...";
const char* kBtnDone = "Done";
const char* kBtnCancel = "Cancel";
const char* kRadioBtnDHCP = "Obtain settings automatically (DHCP)";
const char* kRadioBtnSettings = "Specify settings";
const char* kInterfaceName = "Network Interface:";
const char* kGatewayStr = "Gateway:";
const char* kPrimaryDNSStr = "Primary DNS:";
const char* kSecondaryDNSStr = "Secondary DNS:";
const char* kIPAddress = "IP address:";
const char* kSubnetMask = "Subnet mask:";

const char* kstrTitle = "Network";
const char* kBtnRstNetworkStr = "Restart Networking";
const char* kBtnRevertStr = "Revert";
const char* kBtnSaveStr = "Save";

const char* kDefaultIPAddressStr = "192.168.0.1";
const char* kDefaultSubnetStr = "255.255.255.0";
const char* kDefaultGatewayStr = "";

// ********************************************************************************

InterfaceView::InterfaceView(PPAddOn *adn, NetworkingCore *nc, int32 intf, bool defaultify)
 : BView(BRect(0, 0, kPanelViewWidth, kPanelViewHeight), "interface", B_FOLLOW_NONE, B_WILL_DRAW)
{
	addon = adn;

	core = nc;

	settings = (InterfaceSettings *)core->settings.interfacelist.ItemAt(intf);
	if(defaultify)
		RestoreDefaults();

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BuildUI();
}

InterfaceView::~InterfaceView()
{
	core->settings.Unsubscribe(this);
printf("InterfaceView destructor called\n");
}

void InterfaceView::RestoreDefaults()
{
	settings->m_Configured = false;
	settings->m_Enabled = true;
	strcpy(settings->m_Router, kDefaultGatewayStr);
	strcpy(settings->m_IPAddress, kDefaultIPAddressStr);
	strcpy(settings->m_SubnetMask, kDefaultSubnetStr);
	strcpy(settings->m_UseDHCP, "0");
	strcpy(settings->m_HostName, "");
	strcpy(settings->m_DNSDomain, "");
	strcpy(settings->m_DNSPrimary, "");
	strcpy(settings->m_DNSSecondary, "");
}

void InterfaceView::AttachedToWindow()
{
    BView::AttachedToWindow(); 

	core->settings.Subscribe(this);	// automatically loads settings

	m_HostName->SetTarget(this);
	m_EnabledCtl->SetTarget(this);
	m_UseDHCP->SetTarget(this);
	m_UseSettings->SetTarget(this);
	m_IPAddress->SetTarget(this);
	m_DomainName->SetTarget(this);
	m_SubnetMask->SetTarget(this);
	m_PrimaryDNSName->SetTarget(this);
	m_SecondaryDNSName->SetTarget(this);
	m_GatewayName->SetTarget(this);
	m_RemoveButton->SetTarget(this);
	m_PreferredButton->SetTarget(this);
	if(m_ConfigButton)
		m_ConfigButton->SetTarget(this);

	m_RestartButton->SetTarget(core);
	m_SaveButton->SetTarget(core);
	m_RevertButton->SetTarget(core);

//	Window()->SetDefaultButton(m_DoneBtn);
}

void InterfaceView::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		case MSG_CONFIG :
			{								// sent from Config button.
			m_ConfigButton->SetEnabled(false);

			// ISA: So isa cards can configure themselves the addon gets called
			// the addon writes to the network file. In the future the device
			// manager will take care of this, and the network device will publish
			// themselves.			
			core->fNetDevScanList->ConfigNetDev(fInterfaceName->Text(), settings->m_InterfaceName,
						this, core->settings.m_hncw, false);
			}
			break;

		case MSG_REMOVE :
			if(settings->m_Jumpered || ! IsCardPresent())
			{
				core->settings.Unsubscribe(this);
				core->settings.interfacelist.RemoveItem(settings);
				delete settings;
				addon->RemovePanel(addon);
			}
			else
			{
				RestoreDefaults();
				LoadSettings();
			}
		    core->settings.SetDirty(this, true);
			break;

		case MSG_PREFERRED :
			core->settings.m_Preferred = settings;
			CheckEnable();
			// TODO: switch textview<->stringview
		    core->settings.SetDirty(this, true);
			break;

		case MSG_ENABLE :
			CheckEnable();
		    core->settings.SetDirty(this, true);
			break;

		case MSG_DHCP :
			CheckDHCP();
		    core->settings.SetDirty(this, true);
			break;
			
		case MSG_DIRTY :
			settings->m_Configured = true;
			DisplayState();
		    core->settings.SetDirty(this, true);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

void InterfaceView::CheckDHCP()
{
	if(m_UseSettings->Value())
	{
		m_SubnetMask->SetEnabled(true);
		m_IPAddress->SetEnabled(true);

		if(core->settings.m_Preferred == settings)
		{
			m_GatewayName->SetEnabled(true);
			m_PrimaryDNSName->SetEnabled(true);
			m_SecondaryDNSName->SetEnabled(true);
			m_DomainName->SetEnabled(true);
		}
		else
		{
			m_GatewayName->SetEnabled(false);
			m_PrimaryDNSName->SetEnabled(false);
			m_SecondaryDNSName->SetEnabled(false);
			m_DomainName->SetEnabled(false);
		}
	}
	else
	{
		m_SubnetMask->SetEnabled(false);
		m_IPAddress->SetEnabled(false);
		m_GatewayName->SetEnabled(false);
		m_PrimaryDNSName->SetEnabled(false);
		m_SecondaryDNSName->SetEnabled(false);
		m_DomainName->SetEnabled(false);
	}
}

void InterfaceView::CheckEnable()
{
	Window()->BeginViewTransaction();
	if(m_EnabledCtl->Value())
	{
		m_HostName->SetEnabled(true);
		m_UseDHCP->SetEnabled(true);
		m_UseSettings->SetEnabled(true);

		CheckDHCP();

		m_StatusLabel->SetDrawingMode(B_OP_COPY);
		m_StatusLabel->Invalidate();
		m_RemoveButton->SetEnabled(true);

		m_PreferredButton->SetEnabled((core->settings.m_Preferred == settings || core->settings.interfacelist.CountItems() == 1) ? false : true);
		if(m_ConfigButton)
			m_ConfigButton->SetEnabled(true);
	}
	else
	{
		m_HostName->SetEnabled(false);
		m_UseDHCP->SetEnabled(false);
		m_UseSettings->SetEnabled(false);
		m_SubnetMask->SetEnabled(false);
		m_IPAddress->SetEnabled(false);
		m_PrimaryDNSName->SetEnabled(false);
		m_SecondaryDNSName->SetEnabled(false);
		m_DomainName->SetEnabled(false);
		m_GatewayName->SetEnabled(false);
		m_StatusLabel->SetDrawingMode(B_OP_BLEND);
		m_StatusLabel->Invalidate();
		m_PreferredButton->SetEnabled(false);
		m_RemoveButton->SetEnabled(false);
		if(m_ConfigButton)
			m_ConfigButton->SetEnabled(false);
	}

	DisplayState();
	Window()->EndViewTransaction();
}

bool InterfaceView::IsCardPresent() 
{
	char	*NetDevScanName;
	bool	present = false;

	// Loop through list of physical interfaces in machine.
	// Check if interface is already in list, if so, do not
	// add.
	for(int32 i = 0; i < core->fNetDevScanList->CountItems(); i++)
	{
		TNetDevScanItem *item = (TNetDevScanItem *)core->fNetDevScanList->ItemAt(i);
		NetDevScanName = item->LinkName();
		if(!NetDevScanName)
			continue;

		if(strcmp(NetDevScanName, settings->m_LinkName) == 0)
		{
			present = true;
			break;
		}
	}

	return present;
}

void InterfaceView::DisplayState()
{
	char	status[32];

	strcpy(status, "Status: ");
	strcpy(status + 8, "Not configured");
	if(settings->m_Configured)
	{
		strcpy(status + 8, "Not present");
		if(IsCardPresent())
		{
			if(m_EnabledCtl->Value())
				strcpy(status + 8, "Ready");
			else
				strcpy(status + 8, "Disabled");
		}
	}

	m_StatusLabel->SetText(status);
}

void InterfaceView::Done(status_t /*status*/)
{
	if(LockLooper())
	{
		m_ConfigButton->SetEnabled(true);
		UnlockLooper();
	}
}

void InterfaceView::CanRevert(bool flag)
{
	m_RevertButton->SetEnabled(flag);
}

void InterfaceView::CanSave(bool flag)
{
	m_SaveButton->SetEnabled(flag);
}

bool InterfaceView::LockSubscriber()
{
	return LockLooper();
}

void InterfaceView::UnlockSubscriber()
{
	UnlockLooper();
}

void InterfaceView::LoadSettings()
{
	m_DomainName->SetText(settings->m_DNSDomain);
	m_HostName->SetText(settings->m_HostName);
	m_PrimaryDNSName->SetText(settings->m_DNSPrimary);
	m_SecondaryDNSName->SetText(settings->m_DNSSecondary); 	
	fInterfaceName->SetText(settings->m_PrettyName);
	m_GatewayName->SetText(settings->m_Router);			
	m_IPAddress->SetText(settings->m_IPAddress);
	m_SubnetMask->SetText(settings->m_SubnetMask);
	m_EnabledCtl->SetValue(settings->m_Enabled);

	// Set UseDHCP radio button
	if(atol(settings->m_UseDHCP) == 1)
		m_UseDHCP->SetValue(1);
	else
		m_UseSettings->SetValue(1);				

	CheckEnable();
}

void InterfaceView::UnloadSettings()
{
	// update the settings file with any new entries
   	strcpy(settings->m_DNSDomain, m_DomainName->Text());
   	strcpy(settings->m_HostName, m_HostName->Text());
   	strcpy(settings->m_DNSPrimary, m_PrimaryDNSName->Text());
   	strcpy(settings->m_DNSSecondary, m_SecondaryDNSName->Text());
	strcpy(settings->m_Router, m_GatewayName->Text());	
	strcpy(settings->m_IPAddress, m_IPAddress->Text());
	strcpy(settings->m_SubnetMask, m_SubnetMask->Text());	
	settings->m_Enabled = m_EnabledCtl->Value();

	char buf[NC_MAXVALLEN];
	sprintf(buf, "%ld", m_UseDHCP->Value());
	strcpy(settings->m_UseDHCP, buf);
}


const int32 kTextControlGap = 5;

void InterfaceView::BuildUI()
{
	// interface name
	fInterfaceName = new BStringView(BRect(0, 0, kPanelViewWidth, 24), "", "");	// filled in later
	BFont	f(be_bold_font);
	f.SetSize(18);
	fInterfaceName->SetFont(&f);
	AddChild(fInterfaceName);

	// interface box
	BBox *box = new BBox(BRect(0, 40, kPanelViewWidth, kPanelViewHeight - 35));
	AddChild(box);
	m_EnabledCtl = new BCheckBox(BRect(0, 0, StringWidth("Interface enabled") + 25, 20), "", "Interface enabled", new BMessage(MSG_ENABLE),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	box->SetLabel(m_EnabledCtl);


	// Hostname
    m_HostName = new BTextControl(BRect(25, 20, 150, 40), "", kHostName, "", new BMessage(MSG_DIRTY));
	((BTextView *)(m_HostName->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_HostName) - 1);
	box->AddChild(m_HostName);

	// status
	m_StatusLabel = new BStringView(BRect(210, 20, 370, 40), B_EMPTY_STRING, "");
	box->AddChild(m_StatusLabel);

	// DHCP radios
	m_UseDHCP = new BRadioButton(BRect(20, 60, 300, 75), "", kRadioBtnDHCP, new BMessage(MSG_DHCP),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	box->AddChild(m_UseDHCP);

	m_UseSettings = new BRadioButton(BRect(20, 80, 300, 95), "", kRadioBtnSettings, new BMessage(MSG_DHCP),
			B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	box->AddChild(m_UseSettings);


	// IP address
    m_IPAddress = new BTextControl(BRect(20, 105, 180, 120), "", kIPAddress, "", new BMessage(MSG_DIRTY));
	m_IPAddress->SetDivider(80);	
	m_IPAddress->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);						  
	((BTextView *)(m_IPAddress->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_IPAddress) - 1);
	box->AddChild(m_IPAddress);

	// mask
    m_SubnetMask = new BTextControl(BRect(210, 105, 370, 120), "", kSubnetMask, "", new BMessage(MSG_DIRTY));
	m_SubnetMask->SetDivider(80);	
	m_SubnetMask->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);						  
	((BTextView *)(m_SubnetMask->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_SubnetMask) - 1);
	box->AddChild(m_SubnetMask);

	// Domain Name
	m_DomainName = new BTextControl(BRect(20, 130, 180, 145), "", kDomainStr, "", new BMessage(MSG_DIRTY));
	m_DomainName->SetDivider(80);	
	m_DomainName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	((BTextView *)(m_DomainName->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_DNSDomain) - 1);
	box->AddChild(m_DomainName);

	// gateway
    m_GatewayName = new BTextControl(BRect(20, 150, 180, 165), "", kGatewayStr, "", new BMessage(MSG_DIRTY));
	m_GatewayName->SetDivider(80);	
	m_GatewayName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);						  
	((BTextView *)(m_GatewayName->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_Router) - 1);
	box->AddChild(m_GatewayName);

	// Primary DNS
	m_PrimaryDNSName = new BTextControl(BRect(210, 130, 370, 145), "", kPrimaryDNSStr, "", new BMessage(MSG_DIRTY));
	m_PrimaryDNSName->SetDivider(80);
	m_PrimaryDNSName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);							  
	((BTextView *)(m_PrimaryDNSName->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_DNSPrimary) - 1);
	box->AddChild(m_PrimaryDNSName);

	// Secondary DNS
	m_SecondaryDNSName = new BTextControl(BRect(210, 150, 370, 165), "", kSecondaryDNSStr, "", new BMessage(MSG_DIRTY));
	m_SecondaryDNSName->SetDivider(80);
	m_SecondaryDNSName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	((BTextView *)(m_SecondaryDNSName->ChildAt(0)))->SetMaxBytes(sizeof(settings->m_DNSSecondary) - 1);
	box->AddChild(m_SecondaryDNSName);

	// remove button
	BRect r(box->Bounds());
	m_RemoveButton = new BButton(BRect(r.right - 85, r.bottom - 30, r.right - 10, r.bottom - 10), "remove", (settings->m_Jumpered || ! IsCardPresent()) ? "Remove" : "Unconfigure", new BMessage(MSG_REMOVE));
	box->AddChild(m_RemoveButton);

	// config button
	if(settings->m_Jumpered)
	{
		m_ConfigButton = new BButton(BRect(r.right - 210, r.bottom - 30, r.right - 95, r.bottom - 10), "config", "Card Settings" B_UTF8_ELLIPSIS, new BMessage(MSG_CONFIG));
		box->AddChild(m_ConfigButton);
	}
	else
		m_ConfigButton = 0;

	m_PreferredButton = new BButton(BRect(r.left + 10, r.bottom - 30, r.left + 100, r.bottom - 10), "preferred", "Make preferred", new BMessage(MSG_PREFERRED));
	box->AddChild(m_PreferredButton);

	// bottom buttons
	m_RestartButton = new BButton(BRect(0, kPanelViewHeight - 25, 120, kPanelViewHeight - 5), "restart", "Restart Networking", new BMessage(MSG_RESTARTNETWORKING));
	AddChild(m_RestartButton);

	m_RevertButton = new BButton(BRect(130, kPanelViewHeight - 25, 205, kPanelViewHeight - 5), "revert", "Revert", new BMessage(MSG_REVERT));
	AddChild(m_RevertButton);

	m_SaveButton = new BButton(BRect(kPanelViewWidth - 75, kPanelViewHeight - 25, kPanelViewWidth, kPanelViewHeight - 5), "restart", "Save", new BMessage(MSG_SAVE));
	AddChild(m_SaveButton);
}

#if 0
bool NetworkPanel::QuitRequested()
{
	//	updates the settings file
	fSettingsPanel->Update(kEmptyControls);
	fMiscellaneousPanel->Update(kEmptyControls);

	if(TryToSave(true))
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
	return false;
}

void NetworkPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_REVERT:
			core->settings.ReloadData();			
			break;

		case MSG_SAVE:		
			fSettingsPanel->Update(kEmptyControls);
			fMiscellaneousPanel->Update(kEmptyControls);
			TryToSave(false);		
		break;
		
		case MSG_FILLCONTROLS:
			fSettingsPanel->Update(kFillControls);
			fMiscellaneousPanel->Update(kFillControls);
		break;
		
		case MSG_EMPTYCONTROLS:
			fSettingsPanel->Update(kEmptyControls);
			fMiscellaneousPanel->Update(kEmptyControls);
		break;

		// sent from DUN when closed
		case 'dun!':
			fSettingsPanel->Update(kFillControls);
			break;
					
		default:
			BWindow::MessageReceived(message);
		break;
	}
}
#endif