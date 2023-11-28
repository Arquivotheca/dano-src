#ifndef _PLAYER_WINDOW_H
#define _PLAYER_WINDOW_H

#include <Entry.h>
#include <String.h>
#include <Window.h>
#include "URL.h"

namespace BPrivate {

class AttributeStreamNode;

}

using namespace BPrivate;

class VideoSource;
class VideoView;
class BTimeSource;
class MediaController;
class TransportView;
class BMenuBar;
class BFilePanel;
class InfoWindow;
class BMenu;

const uint32 M_LIVE = 'live';
const uint32 M_OPEN = 'open';
const uint32 M_TOGGLE_MINI_MODE = 'tglm';
const uint32 M_FILE_INFO = 'info';
const uint32 M_DOWNLOAD_ADDONS = 'dnld';
const uint32 M_RESIZE_TO_1_BY_1_SCALE = '1x1s';
const uint32 M_RESIZE_TO_2_BY_1_SCALE = '2x1s';
const uint32 M_RESIZE_TO_1_BY_2_SCALE = '1x2s';
const uint32 M_RESIZE_TO_FULL_SCREEN = 'full';
const uint32 M_NUDGE_VOLUME_UP = 'volu';
const uint32 M_NUDGE_VOLUME_DOWN = 'vold';
const uint32 M_TOGGLE_LOOP_MODE = 'loop';
const uint32 M_TOGGLE_DROP_FRAMES = 'tdrp';
const uint32 M_RUN_APP_SETTINGS = 'apps';
const uint32 M_DONE_PLAYING = 'done';
const uint32 M_OPEN_URL_PANEL = 'urlp';
const uint32 kOpenURL = 'ourl';
const uint32 kFileReady = 'frdy';

class PlayerWindow : public BWindow {
public:

	PlayerWindow(BPoint);

	MediaController *Controller() const;
	status_t OpenFile(const entry_ref&);
	status_t OpenURL(const URL&, const char *cookies);
	status_t FinishOpenFile();
	void CloseFile();

	void RestoreState(AttributeStreamNode *, bool keepWindowPos = false);
	void SaveState(AttributeStreamNode *);
	void PrefsChanged();

	void Play();
		// to support auto play

	void DontSaveState(bool dontsavecontrollerstate);
		// call this when opening one time with special
		// arguments on the command line

	enum PlayMode {
		kMini,
		kLarge,
		kFullScreen
	};
	
	void SetMode(PlayMode);
	void ToggleMiniMode();
	PlayMode Mode() const;

	void DownloadCodec(const char *type = 0);
	void BuildContextMenu(BMenu *);
	TransportView *GetTransportView() const;
	static float TransportHeightForMode(PlayMode mode);
		
protected:

	void FileOpened();
	
	void Set1By1Scale();
	void Set1By2Scale();
	void Set2By1Scale();
	void SetFullScreen();
	void RestoreFromFullScreen();

	virtual void FrameResized(float new_width, float new_height);
	
	void SetUpWindowMode(bool);
	void FixupViewLocations();
		// called from FrameResized - works around a problem where
		// views end up in the wrong spot when resizing while switching modes

	virtual void MessageReceived(BMessage *);
	virtual bool QuitRequested();
	virtual	void MenusBeginning();
	virtual void Zoom(BPoint rec_position, float rec_width, float rec_height);
	virtual	void WindowActivated(bool state);
	virtual void WorkspaceActivated(int32 , bool );

	float MinHeightForMode(PlayMode mode) const;
	float MaxHeightForMode(PlayMode mode, bool noVideo) const;
	
	float MinWidthForMode(PlayMode mode) const;
	float MaxWidthForMode(PlayMode mode, bool noFile) const;
	
	void SetUpMenu();
	void SetUpShortcuts();

	void UpdateFileMenu(BMenu *);
	void UpdateViewMenu(BMenu *);
	void UpdateSettingsMenu(BMenu *);

	BRect SizeForVideoRect(BRect);	
private:

	void RemoveVideoView();
	void ControllerReady();

	bool ShouldSaveState();
	
	BMenuBar *fMainMenu;
	BTimeSource *fTimeSource;
	VideoView *fVideoView;
	VideoSource *fVideoSource;
	BView *fBackground;
	MediaController *fController;
	TransportView *fTransportView;
	PlayMode fMode;
	PlayMode fFallBackMode;
	BRect fFallBackRect;
	BFilePanel *fFilePanel;
	bool fStateChanged;
	bool fDontSaveWindowState;
	bool fDontSaveControllerState;
	bool fKeepNonproportionalResize;
	entry_ref fRef;
	BString fFileName;
	URL fCurrentURL;
	bool fIsLocalFile;

	InfoWindow *fInfoWindow;
	typedef BWindow _inherited;

	friend class ShortcutDispatchFilter;
};

#endif
