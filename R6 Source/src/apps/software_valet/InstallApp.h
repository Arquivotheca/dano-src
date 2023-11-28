// InstallApp.h
#ifndef _INSTALLAPP_H_
#define _INSTALLAPP_H_

#include "InstallerType.h"
#include <Application.h>
#include <Message.h>

#define ARGV	0
#define APP_SIGNATURE "application/x-scode-Inst"

class InstallWindow;
class SettingsManager;

class InstallApp : public BApplication
{
public:
					InstallApp(); // set up menus	
	virtual void	ReadyToRun();

#if (ARGV)
	virtual void	RefsReceived(BMessage *);
	virtual void	ArgvReceived(int32, char **);
#endif
			void	CheckSelfExtracting();
	
	virtual bool	QuitRequested();

	static	void	SetGlobals(entry_ref ref);
	static	void	FreeGlobals();	
	
	static entry_ref	appRef;
	InstallWindow		*mainWindow;
	char				appName[B_FILE_NAME_LENGTH];
	bool				selfExtracting;
	bool				appQuitting;
	
	char				*initialInstallPack;
};


#endif
