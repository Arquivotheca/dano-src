//
// BurnerWindow.h
//
//  by Nathan Schrenk (nschrenk@be.com)

#ifndef _BURNER_WINDOW_H_
#define _BURNER_WINDOW_H_

#include <Window.h>
#include <MenuItem.h>

class BHandler;
class BMenu;
class BMenuBar;
class BurnControlView;
class BurnerProject;
class CDDriver;
class CDPlayerView;
class TrackListView;
class TrackEditView;
struct app_info;

const int32 kDeviceSelectMessage	= 'dSEL';

class BurnerWindow : public BWindow
{
public:
	BurnerWindow(const char *title);
	virtual ~BurnerWindow();

	// messages that get sent when the track list changes
	enum track_message {
		TRACK_ADDED				= 'tADD',
		TRACK_DELETED			= 'tDEL',
		TRACK_MOVED				= 'tMOV',
		TRACK_SELECTION_CHANGED	= 'tSEL'
	};
	
	// the methods below can only be called when the window is locked
	BMenuBar	*MainMenu();

	void		AddTrackListener(BHandler *handler);
	status_t	RemoveTrackListener(BHandler *handler);
	void		SendTrackMessage(BMessage *msg);	// sends a message to all listeners

	status_t	SetToProject(BurnerProject *project);
	status_t	UnloadProject();
	BurnerProject	*Project();

	bool BurnInProgress();

	bool CanQuit();
	void SetCanQuit(bool okToQuit);

//	bool Enabled();
//	void SetEnabled(bool enabled);
		
protected:	
	virtual void Quit();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *msg);
	virtual void MenusBeginning();
	
private:
	status_t AutoPosition(app_info *info);
	CDDriver *GetSelectedDriver();
	void	RebuildDeviceMenu();
	
	BList			fListenerList;
	CDPlayerView	*playerView;
	TrackEditView	*fEditView;
	BurnControlView	*burnView;
	TrackListView	*tlView;
	BurnerProject	*fProject;
	BMenuBar		*fMainMenu;
	BMenu			*fProjectMenu;
	BMenu			*fDiscMenu;
	BMenu			*fColumnMenu;
//	BMenu			*fDriveMenu;
	BMenu			*fSettingsMenu;
	bool			fCanQuit;
	bool			fBurning;
	bool			fProjectLoaded;
//	bool			fEnabled;
};

//------------------------------------------------------------------------

class CDRMenuItem : public BMenuItem
{
public:
	CDRMenuItem(CDDriver *driver, BMessage *msg);
	virtual ~CDRMenuItem();
	CDDriver *Driver();
	
private:
	CDDriver *fDriver;
};

#endif // _BURNER_WINDOW_H_
