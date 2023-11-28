#ifndef MAIN_H
#define MAIN_H

#include <Message.h>
#include <Window.h>

enum {
	CMD_CHANGE_COLOR		= 'chcl',
	CMD_DEFAULTS			= 'rset',
	CMD_REVERT				= 'rvrt',
};

/*------------------------------------------------------------*/
class BButton;

class ColorSelector;

class ColorWindow : public BWindow {
public:
						ColorWindow(BRect frame, const char *title);
						~ColorWindow();

virtual	void			MessageReceived(BMessage *msg);
virtual bool			QuitRequested();
virtual	status_t		UISettingsChanged(const BMessage* changes, uint32 flags);

private:
		void			CheckDirty();

		ColorSelector*	fSelector;
		
		BButton*		fDefault;
		BButton*		fRevert;
};

/*------------------------------------------------------------*/

#endif
