//
// pmprefs.h
//
// Define preference object for Poor Man.
//

#ifndef __PMPREFS_H__
#define __PMPREFS_H__

#include <File.h>
#include <StorageDefs.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <FilePanel.h>
#include <CheckBox.h>
#include <TextView.h>
#include <TextControl.h>
#include <string.h>
#include <Message.h>
#include <Window.h>
#include "web_server.h"

class PMPrefs;
class BSlider;

class PMPrefsWindow : public BWindow {
	public:
					PMPrefsWindow(BPoint where, PMPrefs *prefs);
					~PMPrefsWindow();
	virtual bool	QuitRequested(void);
	virtual void	MessageReceived(BMessage *message);

	private:
		BView		*CreateLogPanel(BRect frame, BView *tab_view, PMPrefs *prefs);
		BView		*CreateSitePanel(BRect frame, BView *tab_view, PMPrefs *prefs);
		BView		*CreateAdvPanel(BRect frame, BView *tab_view, PMPrefs *prefs);
		int16		CheckMaxConnections(int16 max_connections);

		void		CreateLogFile(entry_ref *dir, const char *name);
		void		RefsReceived(BMessage *msg);
		void		AskToRestartServer(void);
		void		SaveSettings(void);

		PMPrefs		*prefs;
		BTextControl *log_path;
		BTextControl *web_dir;
		BTextControl *index_name;
		BSlider		*max_connections;
		BCheckBox	*checkbox_dir_list;
		BCheckBox	*log_to_file;
		BCheckBox	*log_to_console;
		BFilePanel	*logpanel;
		BFilePanel	*filepanel;
		BMessenger	*message_me_baby;
		bool		changedMaxConnections;
		bool		PrefsChanged;
};

class PMPrefs : public BMessage {
	public:
					PMPrefs(char *filename);
					~PMPrefs();
		status_t	InitCheck(void);
	
		status_t	SetBool(const char *name, bool b);
		status_t	SetInt16(const char *name, int16 i);
		status_t	SetInt32(const char *name, int32 i);
		status_t	SetFloat(const char *name, float f);
		status_t	SetString(const char *name, const char *s);
		status_t	SetPoint(const char *name, BPoint p);
		status_t	SetRect(const char *name, BRect r);

		void		WriteSettingsFile(void);
		PMPrefsWindow *ShowWindow(BPoint where);
		void		WindowClosed(void);
	
	private:
		BPath		path;
		status_t	status;
		PMPrefsWindow *prefsWindow;
};


inline status_t PMPrefs::InitCheck(void) {
	return status;
}

// Messages that indicate file panel type

static const uint32 MSG_LOG_FILE_SELECT			= 'FPlf';
static const uint32 MSG_WEB_DIR_SELECT			= 'FPwd';

// Message constants for preference controls

static const uint32 PREFS_CTL_LOG_CONSOLE		= 'PRlc';
static const uint32 PREFS_CTL_LOG_FILE			= 'PRlf';
static const uint32 PREFS_CTL_LOG_FILE_SELECT	= 'PRls';

static const uint32 PREFS_CTL_WEB_DIR_SELECT	= 'PRds';
static const uint32 PREFS_CTL_DIR_LIST			= 'PRdl';

static const uint32 PREFS_CTL_WEB_DIRECTORY		= 'PRdr';
static const uint32 PREFS_CTL_INDEX_NAME		= 'PRin';
static const uint32 PREFS_CTL_LOG_NAME			= 'PRna';

static const uint32 PREFS_CTL_MAX_CONNECTIONS	= 'PRmc';

static const uint32 PREFS_CTL_OK				= 'PRok';
static const uint32 PREFS_CTL_CANCEL			= 'PRcn';

#endif
