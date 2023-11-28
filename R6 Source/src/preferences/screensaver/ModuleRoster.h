#if ! defined MODULEROSTER_INCLUDED
#define MODULEROSTER_INCLUDED 1

#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <List.h>
#include <Path.h>

class BDirectory;

class ModuleRoster : public BLooper
{
	BList		modules;
	BMessenger	view;
	BMessage	addnotif;
	BMessage	remnotif;
	BMessage	modulesnotif;

public:
			ModuleRoster();
			~ModuleRoster();
	void	RefsReceived(BMessage *msg);
	void	MessageReceived(BMessage *msg);
	void	ModuleList(BDirectory *dir, BList *modules);
	void	ModuleListDiffs();
	void	ModulesChangedNotification();

	static bool	IsModule(BPath path, BMessage *data);
	static void	Install(BMessage *msg);
};

#endif
