#if ! defined MODULERUNNER_INCLUDED
#define MODULERUNNER_INCLUDED

#include <Application.h>
#include <Path.h>
#include <String.h>

class Activity;
class BWindow;

#define MAXACT 4

class ModuleRunner : public BApplication
{
	Activity	*act[MAXACT];
	int32		delay[MAXACT];
	int32		currentact;
	int32		max;
	bool		override;
	bool		testmode;
	BPath		addon;
	bool		crash_protection;
	bool		locked;
	bool		unlocking;
	bool		unlocked;
	BWindow		*lockwin;
	
#if SCREENSAVER_USE_BINDER
	BString fModuleName;
	
	status_t GetModuleNameFromBinder(const char** moduleName);
#endif

public:
					ModuleRunner();
	virtual bool	QuitRequested();
	virtual void	ReadyToRun();
	virtual void	ArgvReceived(int32 argc, char **argv);
	virtual void	RefsReceived(BMessage *msg);
	virtual void	MessageReceived(BMessage *msg);
	void			LoadPrefs();
};

#endif
