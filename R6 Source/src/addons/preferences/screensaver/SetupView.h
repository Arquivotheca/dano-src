#if ! defined SETUPVIEW_INCLUDED
#define SETUPVIEW_INCLUDED 1

#include <View.h>

class BTabView;
class ModuleRoster;
class SettingsView;

class SetupView : public BView
{
	BTabView		*tabView;
	ModuleRoster	*roster;
	SettingsView	*set;

public:
			SetupView();
			~SetupView();
	void	AttachedToWindow();
	void	DetachedFromWindow();
};

#endif
