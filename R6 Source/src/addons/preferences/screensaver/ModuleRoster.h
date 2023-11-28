#if ! defined MODULEROSTER_INCLUDED
#define MODULEROSTER_INCLUDED 1

#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <List.h>

class RosterClient
{
public:
	virtual void	AddMod(const BPath &path) = 0;
	virtual void	RemoveMod(const BPath &path) = 0;
	virtual void	ModulesChanged(const BMessage &msg) = 0;
};

class BDirectory;

class ModuleRoster : public BLooper
{
	BList			modules;
	RosterClient	*client;
	BWindow			*clientwindow;

public:
			ModuleRoster();
			~ModuleRoster();
	void	RefsReceived(BMessage *msg);
	void	MessageReceived(BMessage *msg);
	void	ModuleList(BDirectory *dir, BList *modules);
	void	ModuleListDiffs();
	void	ModulesChangedNotification();
	void	Init(RosterClient *cli, BWindow *cliwin);

	static bool	IsModule(BPath path, BMessage *data);
	static void	Install(BMessage *msg);
};

#endif
