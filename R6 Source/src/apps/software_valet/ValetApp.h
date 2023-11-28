// ValetApp.h

#ifndef _VALETAPP_H_
#define _VALETAPP_H_

#include <Application.h>
#include <Locker.h>

enum {
	M_DO_SHOP	=	'Shop',
	M_DO_INSTALL =	'Inst',
	M_DO_DOWNLOAD = 'Down',
	M_DO_MANAGE =	'Mana',
	M_DO_SETTINGS =	'Sett'
};

class ValetWindow;
class PackageItem;

class ValetApp : public BApplication
{
public:
	ValetApp();

	virtual bool	QuitRequested();
	virtual void	AboutRequested();
	virtual void	DispatchMessage(BMessage *msg, BHandler *hand);
	virtual void	RefsReceived(BMessage *msg);
	virtual void	ArgvReceived(int32 argc, char **argv);
	virtual void	MessageReceived(BMessage *msg);
	virtual void	ReadyToRun();
	//		void	FirstLaunch(BDirectory &);
	
	static entry_ref	appRef;
	
	BMessenger		valetWindMess;
	BMessenger		logWindMess;
	BMessenger		reportWindMess;
	ValetWindow		*valetWindow;
	
	BLooper			*downloadManager;

	static PackageItem 	*sBeOSPackageItem;  // == null if beos is registered; non-null otherwise

private:
	bool	fSomeWindowShowing;
	bool	fRefsUnprocessed;
	friend class ValetWindow;
};




status_t CheckLaunchListener(bool alwaysLaunch = false);


#endif

