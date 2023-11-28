#if ! defined BLANKETAPP_INCLUDED
#define BLANKETAPP_INCLUDED 1

#include <Application.h>

#include "Blanket.h"

class SetupWindow;
class ModuleRoster;
class BMessage;

class BlanketApp : public BApplication
{
	SetupWindow		*setupwin;
	ModuleRoster	*roster;

public:
			BlanketApp();
	bool	QuitRequested();
	void	ReadyToRun();
};

#endif
