//  GeneralView.h
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#if ! defined GENERALVIEW_INCLUDED
#define	GENERALVIEW_INCLUDED

#include "Settings.h"
#include <View.h>
#include <NetDevice.h>
#include <image.h>

class NetworkingCore;
class PPWindow;
class PPAddOn;
class TextControl;
class BCheckBox;
class BButton;

class GeneralView : public BView, public SettingsSubscriber
{
	PPAddOn			*addon;
	image_id		image;
	PPWindow		*window;

	BCheckBox		*m_FTPServer;
	BCheckBox		*m_TelnetServer;
	BTextControl	*m_UserName;
	BTextControl	*m_Password1;
	BTextControl	*m_Password2;
	BCheckBox		*m_IPForwarding;
	BCheckBox		*m_AppleTalk;
	BButton			*m_AddHWButton;
	BButton			*m_RestartButton;
	BButton			*m_SaveButton;
	BButton			*m_RevertButton;

	const char *GetUniqueInterfaceName();

public:
			GeneralView(PPAddOn *adn, image_id i, PPWindow *w);

	void	BuildNetCards();
	void	AttachedToWindow();
	void	MessageReceived(BMessage *message);
	void	CreateMiscUI();
	void	AddInterfaces();
	bool	IsInterfacePresent(char *name);
	bool	QuitRequested();

	// SettingsSubscriber interface
	bool	LockSubscriber();
	void	UnlockSubscriber();
	void	CanRevert(bool flag);
	void	CanSave(bool flag);
	void	LoadSettings();
	void	UnloadSettings();

	NetworkingCore		*core;
};

#endif	// GENERALVIEW_INCLUDED
