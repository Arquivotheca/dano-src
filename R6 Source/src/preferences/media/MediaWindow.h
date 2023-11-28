#if !defined(MEDIA_WINDOW_H)
#define MEDIA_WINDOW_H

#include "DynamicScrollView.h"
#include "MediaListView.h"
#include "MediaViews.h"

#include <MediaNode.h>
#include <Window.h>

class BScrollBar;
class BParameterWeb;
class BMediaRoster;
class BScrollView;

#define MIN_WIDTH 605
#define MIN_HEIGHT 402

#define RESTART_SERVER 'resr'
#define SELECTION_CHANGED 'slct'
#define SHOW_VOLUME 'volu'
const uint32 INIT_SERVICES = 'init';

class TStatusWindow;
class MediaWindow : public BWindow {

public:
					MediaWindow(const BRect area, const char * name);
					~MediaWindow();

	virtual	void 	MessageReceived(BMessage * message);
	virtual	bool 	QuitRequested();
	void			FrameMoved(BPoint location);
	void			FrameResized(float width, float height);
	
	void			ClearDynoTarget();

	void 			SetVideoInputView(const dormant_node_info &info);
	void 			SetVideoOutputView(const dormant_node_info &info);
	void 			SetAudioInputView(const dormant_node_info &info);
	void 			SetAudioOutputView(const dormant_node_info &info);	
	void 			SetMixerView(const dormant_node_info &info);
	
	int32		ServicesRestart();
private:
	status_t	Init();
	status_t	RestartServer();
	void		ShowStatusWindow();
	void		HideStatusWindow();
	void		KillRestartThread();
	
	void		SetupListView();
	void		SetupAudioView();
	void		SetupVideoView();

	void		SetCurrentView();

	bool mInited;
	BMediaRoster *mRoster;
	BView * mBackground;
	BDynamicScrollView * mDyno;
	BStringView * mTitle;
	MediaListView * mList;
	BView * mAudio;
	BView * mVideo;
	BView * mCurrent;

	thread_id		fThreadID;
	TStatusWindow*	fStatusWindow;
	bool			fInitWhenDoneRestarting;
	bool			fServerIsDead;
};

#endif
