#if ! defined SCREENSAVERCONTROLLER_INCLUDED
#define SCREENSAVERCONTROLLER_INCLUDED

#if SCREENSAVER_USE_BINDER
#include <Binder.h>
#include <SmartArray.h>
#endif /* #if SCREENSAVER_USE_BINDER */
#include <Looper.h>
#include <Message.h>
#include <experimental/MultiLocker.h>
#include <Node.h>
#include <OS.h>
#include <Path.h>

class BMessageRunner;
class MonitorView;
class BWindow;

#if SCREENSAVER_USE_BINDER
class ScreenSaverController;

class Listener : public BinderListener {
public:
	Listener(ScreenSaverController* controller);
	virtual ~Listener(void);
	
	virtual status_t Overheard(binder_node node, uint32 observed,
		BString propertyName);
		
private:
	ScreenSaverController* m_controller;
};
#endif /* #if SCREENSAVER_USE_BINDER */

class ScreenSaverController : public BLooper
{
	volatile bigtime_t	last;
	volatile bigtime_t	lastwake;
	volatile bool		saving;
	volatile team_id	blanker_team;
	bigtime_t			start;
	BMessageRunner		*idlecheck;
	int32				fadetime;
	BPath				prefs_dir;
	BPath				prefs_name;
	node_ref			settingsref;
	int32				hotnow;
	int32				hotnever;
	BPoint				lastpos;
	BMultiLocker*		m_locker;

#if SCREENSAVER_USE_BINDER
	binder_node m_node;
	binder_node m_settings;

	atom<Listener> m_listener;
#endif /* #if SCREENSAVER_USE_BINDER */

public:
						ScreenSaverController();

	virtual	void		MessageReceived(BMessage *msg);
	virtual bool		QuitRequested();

	void				LoadPrefs();
	bool				ProcessInput(BMessage *message);
	int8				Corner();

#if SCREENSAVER_USE_BINDER
	status_t 			GetPrefsFromBinder(void);
#endif /* #if SCREENSAVER_USE_BINDER */
};

#endif
