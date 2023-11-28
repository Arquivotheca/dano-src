#ifndef MAIN_H
#define MAIN_H

#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif

#define CMD_BACKGROUND_COLOR	'bkgc'
#define CMD_FONT_FACE			'ffac'
#define CMD_FONT_SIZE			'fsiz'
#define CMD_PERMANENT_TRIGGER	'trig'
#define CMD_ALLOW_STICKY		'stik'
#define CMD_SEPARATOR_STYLE		'seps'
#define CMD_ZSNAKE				'zsnk'

#define CMD_DEFAULTS			'rset'
#define CMD_REVERT				'rvrt'
#define CMD_APPLY				'aply'

#define CMD_DEFAULT_COLOR		'dftc'
#define CMD_COLOR_CHANGE		'cchg'
#define CMD_TMP_COLOR_CHANGE	'ctcc'
#define CMD_REVERT_COLOR		'crvt'

#define	CMD_WIN_STYLE_SHORTCUT	'wins'
#define	CMD_MAC_STYLE_SHORTCUT	'macs'

/*------------------------------------------------------------*/

class TWindow : public BWindow {
public:
					TWindow(BRect frame, const char *title);
					~TWindow();

virtual	void		MessageReceived(BMessage *msg);
virtual bool		QuitRequested();

private:
		void		CheckDirty();
		void		UpdateMenu(menu_info *minfo);
		void		InvalMenu(BMenu *menu);

		BMenuBar	*fMenuBar;
		BButton		*fRevert;
		BWindow		*fColorWindow;
		rgb_color	fRevertColor;
		menu_info	fMenuInfo;
		menu_info	fInitMenuInfo;

		BMenuItem	*fMacStyleItem;
		BMenuItem	*fWinStyleItem;
		BMenuItem	*fZSnakeItem;
		bool		fShowShortcutItems;
		bool		fInitMacStyleShortcut;
		bool		fInitWinStyleShortcut;
		bool		fInitZSnake;
		uchar		fInitCmdKey;
		uchar		fInitCtlKey;
		uchar		fInitRCmdKey;
		uchar		fInitROptKey;
};

/*------------------------------------------------------------*/

#endif
