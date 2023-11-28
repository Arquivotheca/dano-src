//
// Burner.h
//
//  by Nathan Schrenk (nschrenk@be.com)

#ifndef _BURNER_H_
#define _BURNER_H_

#include <Application.h>

class BList;
class CDDriver;
class BurnerWindow;
class BFilePanel;

// message constants
const uint32 kOpenProjMessage		= 'oPRJ';
const uint32 kSaveProjMessage		= 'sPRJ';
const uint32 kSaveProjAsMessage		= 'sAPR';
const uint32 kCloseRequestedMessage	= 'cLOS';
const uint32 kWindowClosedMessage	= 'wINC';
const uint32 kQuitMessage			= 'qUIT';
const uint32 kBurnStarting			= 'bRNS';
const uint32 kBurnEnding			= 'bRNE';
const uint32 kScanForDrives			= 'sFDR';
const uint32 kDeviceListChanged		= 'dLCH';
const uint32 kAddAudioTrackMessage	= 'aATK';
const uint32 kAddDataTrackMessage	= 'aDTK';

// status code constants
const status_t kMustSave			= 1501;
//const status_t k

#define BURNER_PROJECT_MIME_TYPE "application/x-vnd.Be.Burner-project"

struct DriverInfo
{
	DriverInfo(CDDriver *_driver) {
		owner = NULL;
		driver = _driver;
	}
//	BLocker			lock;
	CDDriver		*driver;
	BurnerWindow	*owner;
};
class AboutWnd : public BWindow
{
	public:
						AboutWnd();
						~AboutWnd();
			void		Quit();
};


class BurnerApp : public BApplication
{
public:
			BurnerApp();
	virtual ~BurnerApp();

	// XXX: need some way to access/manage driver list
	int32		CountDrivers();
	CDDriver	*DriverAt(int32 index);
	status_t	LockDriver(CDDriver *driver, BurnerWindow *win);
	status_t	UnlockDriver(CDDriver *driver, BurnerWindow *win);
	
protected:

	virtual void	AboutRequested();
	virtual void	ArgvReceived(int32 argc, char **argv);
	virtual void	RefsReceived(BMessage *message);
	virtual void	MessageReceived(BMessage *message);
	virtual void	ReadyToRun();
	virtual bool	QuitRequested();
	virtual void	Quit();

	void			DisplayAddTrackWindow(BurnerWindow *window, bool data);
	
private:
	void	ScanForDrives(); // looper must be locked to call this
	
	BList	fDriverList;	// looper must be locked to safely manipulate this list
	BFilePanel	*fSavePanel;
	BFilePanel	*fOpenPanel;
	BurnerWindow *fBurnerWindow;
};

#endif // _BURNER_H_
