//  InterfaceView.h
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#if ! defined INTERFACEVIEW_INCLUDED
#define INTERFACEVIEW_INCLUDED

#include "Settings.h"
#include <View.h>
#include <NetDevice.h>

class NetworkingCore;
class PPAddOn;

class InterfaceView : public BView, public BCallbackHandler, public SettingsSubscriber
{
public:
			InterfaceView(PPAddOn *adn, NetworkingCore *nc, int32 intf, bool defaultify);
			~InterfaceView();
	void	RestoreDefaults();
	void	AttachedToWindow();
	void	MessageReceived(BMessage *message);
	void	BuildUI();
	void	CheckDHCP();
	void	CheckEnable();
	void	DisplayState();
	bool	IsCardPresent();

	// BCallbackHandler interface
	void	Done(status_t status);

	// SettingsSubscriber interface
	bool	LockSubscriber();
	void	UnlockSubscriber();
	void	CanRevert(bool flag);
	void	CanSave(bool flag);
	void	LoadSettings();
	void	UnloadSettings();

	PPAddOn			*addon;
	NetworkingCore	*core;
	InterfaceSettings	*settings;
	BStringView		*fInterfaceName; // Pretty name.
	BTextControl	*m_HostName;
	BStringView		*m_StatusLabel;

	BCheckBox		*m_EnabledCtl;
	BRadioButton	*m_UseDHCP;
	BRadioButton	*m_UseSettings;
	BTextControl	*m_IPAddress;
	BTextControl	*m_SubnetMask;
	BTextControl	*m_GatewayName;
	BTextControl	*m_DomainName;
	BTextControl	*m_PrimaryDNSName;
	BTextControl	*m_SecondaryDNSName;

	BButton			*m_PreferredButton;
	BButton			*m_RemoveButton;
	BButton			*m_ConfigButton;
	BButton			*m_RestartButton;
	BButton			*m_SaveButton;
	BButton			*m_RevertButton;
};

#endif // INTERFACEVIEW_INCLUDED
