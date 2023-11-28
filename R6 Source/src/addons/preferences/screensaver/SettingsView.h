#if ! defined SETTINGSVIEW_INCLUDED
#define SETTINGSVIEW_INCLUDED 1

#include <View.h>
#include "ModuleRoster.h"

class CMonitorControl;
class BButton;
class BTextControl;
class BCheckBox;
class BTextView;
class BMessage;
class BSlider;
class TimeSlider;
class BBox;
class BListView;
class BScreenSaver;
class ModuleListView;
class ModuleListItem;
class BFilePanel;
class ModulePreviewView;

class SettingsView : public BView, public RosterClient
{
	BCheckBox			*dofade;
	BStringView			*timefade;
	TimeSlider			*fade;
	BStringView			*runtext;
	ModuleListView		*list;
	BView				*blankpreview;
	BView				*mockup;
	BBox				*box;
	ModuleRoster		*roster;
	BButton				*test;
	BButton				*add;
	ModuleListItem		*currentmod;
	BMessage			metadata;
	char				modulename[B_PATH_NAME_LENGTH];
	BFilePanel			*filepanel;
	ModulePreviewView	*preview;

public:

			SettingsView(BRect frame, const char *name, ModuleRoster *rost);
			~SettingsView();
	void	MessageReceived(BMessage *msg);
	void	AllAttached(void);
	void	CheckDependencies();
	void	SaveState();
	void	AttachedToWindow();
	void	AddMod(ModuleListItem *mod);
	void	SelectMod(int32 index, bool programmatic);

	void	AddMod(const BPath &path);
	void	RemoveMod(const BPath &path);
	void	ModulesChanged(const BMessage &msg);
};

#endif
