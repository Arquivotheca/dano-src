//
// PMPrefs
//
// Preferences for Poor Man.
//

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <Message.h>
#include <TabView.h>
#include <Alert.h>
#include <Box.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <Window.h>
#include <TextView.h>
#include <ScrollBar.h>
#include <StringView.h>
#include <ScrollView.h>
#include <View.h>
#include <Slider.h>
#include <Entry.h>
#include "pmprefs.h"
#include "pmwin.h"
#include "hardcoded.h"
#include "pmapp.h"
#include "web_server.h"

extern void
alert(
	  const char *format,
	  ...
	  );

// Strings for preference controls

static char *prefStr_OK = "Done";
static char *prefStr_Cancel = "Cancel";

static char *prefStr_ConsoleLogging = "Console Logging";
static char *prefStr_LogToConsole = "Log to Console";

static char *prefStr_FileLogging = "File Logging";
static char *prefStr_LogToFile = "Log to File";
static char *prefStr_LogFileName = "Log File Name:";
static char *prefStr_LogFileButton = "Create Log File";

static char *prefStr_WebLocation = "Web Site Location";
static char *prefStr_SiteDirectory = "Web Directory:";
static char *prefStr_SiteButton = "Select Web Directory";
static char *prefStr_IndexName = "Index File Name:";

static char *prefStr_SiteOptions = "Web Site Options";
static char *prefStr_SendDirectoryList = "Send Directory List If No Index";

static char *prefStr_ConnectionOptions = "Connection Options";
static char *prefStr_MaxConnections = "Max. Connections:";

static char *MAX_CONNECTS_BAD = "The maximum number of simultaneous connections must be between 1 and 200.";

//
// ConnectionsSlider
//
// The class that defines the slider for the max. connection count.
// Needs to override the UpdateText() function.
//
class ConnectionSlider : public BSlider {
	public:
							ConnectionSlider(BRect frame, int16 value);
		virtual char		*UpdateText(void) const;
	
	private:
		mutable char	text[30];
};

ConnectionSlider::ConnectionSlider(BRect frame, int16 value)
		: BSlider(frame, "max_connections", "Maximum Simultaneous Connections:",
					new BMessage(PREFS_CTL_MAX_CONNECTIONS), 1, 200) {
	//SetHashMarks(B_HASH_MARKS_BOTTOM);
	//SetHashMarkCount(20);
	SetLimitLabels("1", "200");
	SetValue(value);
}

char *ConnectionSlider::UpdateText(void) const {
	sprintf(text, "%d connections", Value());
	return text;
}


PMPrefs::PMPrefs(char *fn)
	:	BMessage('pref'),
		prefsWindow(0)
{
	BFile file;
	
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		return;
	}
	path.Append(fn);
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		status = Unflatten(&file);
	}
}


PMPrefsWindow *PMPrefs::ShowWindow(BPoint where) {
	if (!prefsWindow) {
		prefsWindow = new PMPrefsWindow(where, this);
	}
	else {
		prefsWindow->Lock();
		if (prefsWindow->IsHidden()) {
			prefsWindow->Show();
		}
		prefsWindow->Activate(true);
		prefsWindow->Unlock();
	}
	return prefsWindow;
}

void PMPrefs::WindowClosed(void) {
	prefsWindow = NULL;
}


PMPrefs::~PMPrefs() {
	WriteSettingsFile();
}


void PMPrefs::WriteSettingsFile(void) {
	BFile file;
	
	if (file.SetTo(path.Path(), B_WRITE_ONLY|B_CREATE_FILE) == B_OK) {
		Flatten(&file);
	}
}


status_t PMPrefs::SetBool(const char *name, bool b) {
	if (HasBool(name)) {
		return ReplaceBool(name, 0, b);
	}
	return AddBool(name, b);
}

status_t PMPrefs::SetInt32(const char *name, int32 i) {
	if (HasInt32(name)) {
		return ReplaceInt32(name, 0, i);
	}
	return AddInt32(name, i);
}

status_t PMPrefs::SetInt16(const char *name, int16 i) {
	if (HasInt16(name)) {
		return ReplaceInt16(name, 0, i);
	}
	return AddInt16(name, i);
}

status_t PMPrefs::SetFloat(const char *name, float f) {
	if (HasFloat(name)) {
		return ReplaceFloat(name, 0, f);
	}
	return AddFloat(name, f);
}

status_t PMPrefs::SetString(const char *name, const char *s) {
	if (HasString(name)) {
		return ReplaceString(name, 0, s);
	}
	return AddString(name, s);
}

status_t PMPrefs::SetPoint(const char *name, BPoint p) {
	if (HasPoint(name)) {
		return ReplacePoint(name, 0, p);
	}
	return AddPoint(name, p);
}

status_t PMPrefs::SetRect(const char *name, BRect r) {
	if (HasRect(name)) {
		return ReplaceRect(name, 0, r);
	}
	return AddRect(name, r);
}


// Preferences window - must only be
// used when the prefs are valid!

PMPrefsWindow::PMPrefsWindow(BPoint where, PMPrefs *prefs)
			: BWindow(BRect(where.x,where.y,where.x+380,
						where.y+280), "PoorMan Settings",
						B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE) {
	message_me_baby = new BMessenger(this);
	BRect tviewrect;
	BBox *box;

	box = new BBox(Bounds(), NULL, B_FOLLOW_LEFT|B_FOLLOW_TOP,
				B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP,
				B_NO_BORDER);
	AddChild(box);
	box->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	
	tviewrect = Bounds();
	tviewrect.InsetBy(0,8);
	tviewrect.bottom -= 30;
	BTabView *tab_view = new BTabView(tviewrect, "tab_view");
	tab_view->SetFont(be_plain_font);
	box->AddChild(tab_view);

	BRect tr;
	float lineheight;
	font_height fheight;
	
	tab_view->GetFontHeight(&fheight);
	lineheight = fheight.ascent+fheight.descent+fheight.leading;
	tr.bottom = box->Bounds().bottom - 5;
	tr.top = tr.bottom - (13+lineheight);
	tr.right = box->Bounds().right-3;
	tr.left = tr.right - box->StringWidth(prefStr_Cancel) - 12;
	BButton *ok_but;
	box->AddChild(ok_but = new BButton(tr, "btn_ok", prefStr_OK,
				new BMessage(PREFS_CTL_OK)));
	
	tr.right = tr.left - 8;
	tr.left = tr.right - box->StringWidth(prefStr_Cancel) - 12;
	box->AddChild(new BButton(tr, "btn_cancel", prefStr_Cancel,
				new BMessage(PREFS_CTL_CANCEL)), ok_but);

	logpanel = NULL;
	filepanel = NULL;
	changedMaxConnections = false;

	// Add the tabs to the tab view

	BView *internal_view = tab_view->ChildAt(0);
	BRect r = internal_view->Bounds();

	// Tab 1: Site
	tab_view->AddTab(CreateSitePanel(r, tab_view, prefs));

	// Tab 2: Logging

	tab_view->AddTab(CreateLogPanel(r, tab_view, prefs));

	// Tab 3: Advanced

	tab_view->AddTab(CreateAdvPanel(r, tab_view, prefs));

	tab_view->Select((int32)0);
	PrefsChanged = false;

	Show();
	this->prefs = prefs;
}

BView *PMPrefsWindow::CreateSitePanel(BRect r, BView *tab_view, PMPrefs *prefs) {
	BView *v;
	BRect tr;
	BRect bounds;
	BButton *btn;
	BRect boxrect;
	BBox *box;
	BCheckBox *tcheck;
	font_height fheight;
	float	lineheight;
	bool b;

	v = new BView(r, "Site", B_FOLLOW_NONE, B_WILL_DRAW);

	// Get the font height so we can size and position
	// controls nicely.

	tab_view->GetFontHeight(&fheight);
	lineheight = fheight.ascent+fheight.descent+fheight.leading;

	// Box

	boxrect = v->Bounds();
	boxrect.bottom /= 2;
	boxrect.bottom += 15;
	boxrect.InsetBy(5,5);
	box = new BBox(boxrect, "web_location");
	box->SetLabel(prefStr_WebLocation);

	bounds = box->Bounds();

	// Web directory

	tr.left = bounds.left + 10;
	tr.top = bounds.top + 20;
	tr.bottom = tr.top + lineheight;
	tr.right = bounds.right - 10;
	const char *str;
	prefs->FindString("web_directory", &str);
	web_dir = new BTextControl(tr, "web_dir", prefStr_SiteDirectory, str,
					new BMessage(PREFS_CTL_WEB_DIRECTORY));
	web_dir->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	web_dir->SetModificationMessage(new BMessage(PREFS_CTL_WEB_DIRECTORY));
	box->AddChild(web_dir);

	tr.right += 2;
	tr.top = web_dir->Frame().bottom + 4;
	tr.bottom = tr.top + lineheight + 10;
	tr.left = tr.right - 20 - v->StringWidth(prefStr_SiteButton);
	box->AddChild(btn = new BButton(tr, "select_web_dir", prefStr_SiteButton,
					new BMessage(PREFS_CTL_WEB_DIR_SELECT)));

	// Index file name

	tr.top = btn->Frame().bottom+15;
	tr.bottom = tr.top + lineheight;
	tr.left = 10;
	tr.right = bounds.right - 10;
	prefs->FindString("index_file_name", &str);
	index_name = new BTextControl(tr, "index_name", prefStr_IndexName, str,
					new BMessage(PREFS_CTL_INDEX_NAME));
	index_name->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	index_name->SetModificationMessage(new BMessage(PREFS_CTL_INDEX_NAME));
	web_dir->SetDivider(v->StringWidth(prefStr_IndexName)+4);
	index_name->SetDivider(v->StringWidth(prefStr_IndexName)+4);		// to line up pretty-like
	box->AddChild(index_name);
	v->AddChild(box);

	// Options box

	boxrect.top = boxrect.bottom + 10;
	boxrect.bottom = v->Bounds().bottom - 5;
	box = new BBox(boxrect, "site_options");
	box->SetLabel(prefStr_SiteOptions);

	tr.Set(10,20,20+box->StringWidth(prefStr_SendDirectoryList), 20);
	box->AddChild(checkbox_dir_list = new BCheckBox(tr, "checkbox_dir_list",
					prefStr_SendDirectoryList,
					new BMessage(PREFS_CTL_DIR_LIST)));
	prefs->FindBool("dir_list_flag", &b);
	checkbox_dir_list->SetValue(b);

	v->AddChild(box);
	v->SetViewColor(tab_view->ViewColor());
	
	return v;
}

BView *PMPrefsWindow::CreateAdvPanel(BRect r, BView *tab_view, PMPrefs *prefs) {
	BView *v;
	BBox *box;
	BRect boxrect;
	BRect bounds;
	BRect tr;
	BCheckBox *tcheck;
	BScrollView *scrollview;
	font_height fheight;
	BTextView *textview;
	float	lineheight;
	char *s;
	int16 n;
	char buf[17];
	bool b;
	
	v = new BView(r, "Advanced", B_FOLLOW_NONE, B_WILL_DRAW);
	v->SetViewColor(tab_view->ViewColor());

	// Get the font height so we can size and position
	// controls nicely.

	tab_view->GetFontHeight(&fheight);
	lineheight = fheight.ascent+fheight.descent+fheight.leading;

	// Connection settings

	boxrect = v->Bounds();
	boxrect.bottom /= 2;
	boxrect.bottom -= 5;
	boxrect.InsetBy(5,5);
	box = new BBox(boxrect, "connection_box");
	box->SetLabel(prefStr_ConnectionOptions);

	// Maximum # of connections

	if ((prefs->FindInt16((const char *) "max_connections", &n)) != B_OK) {
		n = BACKLOG_DEFAULT;
	}

	bounds = box->Bounds();

	tr.left = bounds.left + 10;
	tr.top = bounds.top + 20;
	tr.bottom = tr.top + 20;
	tr.right = bounds.right - 10;
	box->AddChild(max_connections = new ConnectionSlider(tr, n));

	v->AddChild(box);
	return v;
}

BView *PMPrefsWindow::CreateLogPanel(BRect r, BView *tab_view, PMPrefs *prefs) {
	BView *v;
	BBox *box;
	BRect boxrect;
	BRect tr;
	BScrollView *scrollview;
	font_height fheight;
	float	lineheight;
	bool b;

	v = new BView(r, "Logging", B_FOLLOW_NONE, B_WILL_DRAW);

	// Get the font height so we can size and position
	// controls nicely.

	tab_view->GetFontHeight(&fheight);
	lineheight = fheight.ascent+fheight.descent+fheight.leading;

	// Console log settings

	boxrect = v->Bounds();
	boxrect.bottom /= 2;
	boxrect.bottom -= 5;
	boxrect.InsetBy(5,5);
	box = new BBox(boxrect, "console_box");
	box->SetLabel(prefStr_ConsoleLogging);
	tr.Set(10,20,20+box->StringWidth(prefStr_LogToConsole), 20);
	box->AddChild(log_to_console = new BCheckBox(tr, "checkbox_log_console",
					prefStr_LogToConsole,
					new BMessage(PREFS_CTL_LOG_CONSOLE)));
	prefs->FindBool("log_console_flag", &b);
	log_to_console->SetValue(b);
	v->AddChild(box);

	// File log settings

	boxrect.top = boxrect.bottom + 10;
	boxrect.bottom = v->Bounds().bottom - 5;
	box = new BBox(boxrect, "file_box");
	box->SetLabel(prefStr_FileLogging);

	tr.Set(10,20,20+box->StringWidth(prefStr_LogToFile), 20);
	box->AddChild(log_to_file = new BCheckBox(tr, "checkbox_log_file",
					prefStr_LogToFile,
					new BMessage(PREFS_CTL_LOG_FILE)));
	prefs->FindBool("log_file_flag", &b);
	log_to_file->SetValue(b);


	tr.top = log_to_file->Frame().bottom + 5;
	tr.bottom = tr.top + lineheight;
	tr.right = box->Bounds().right -10;

	const char *str;
	prefs->FindString("log_path", &str);
	log_path = new BTextControl(tr, "log_path", prefStr_LogFileName, str,
					new BMessage(PREFS_CTL_LOG_NAME));
	log_path->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	log_path->SetDivider(v->StringWidth(prefStr_LogFileName)+4);
	log_path->SetModificationMessage(new BMessage(PREFS_CTL_LOG_NAME));
	box->AddChild(log_path);

	tr.bottom = box->Bounds().bottom - 10;
	tr.top = tr.bottom - (10+lineheight);
	tr.right = box->Bounds().right-8;
	tr.left = tr.right - box->StringWidth(prefStr_LogFileButton);
	box->AddChild(new BButton(tr, "select_file", prefStr_LogFileButton,
					new BMessage(PREFS_CTL_LOG_FILE_SELECT)));

	v->AddChild(box);

	v->SetViewColor(tab_view->ViewColor());
	return v;
}

void PMPrefsWindow::SaveSettings(void) {
	prefs->SetRect("setwindow_frame", Frame());
	prefs->SetString("log_path", log_path->Text());
	prefs->SetString("web_directory", web_dir->Text());
	prefs->SetString("index_file_name", index_name->Text());
	logger->SetLogPath(log_path->Text());
	prefs->SetBool("dir_list_flag", checkbox_dir_list->Value());
	prefs->SetBool("log_console_flag", log_to_console->Value());
	logger->LogToConsole(log_to_console->Value());
	prefs->SetInt16("max_connections", max_connections->Value());
	prefs->SetBool("log_file_flag", log_to_file->Value());
	logger->LogToFile(log_to_file->Value());

	BMessenger messenger(SIGNATURE);
	BMessage message(GET_PM_WINDOW);
	BMessage reply;
	PMWindow *window;
	window = NULL;
	if (messenger.SendMessage(&message, &reply, 1000000, 1000000) == B_OK) {
		int16 i;
		reply.FindPointer("window", (void **) &window);
		if (window) {
			window->SetDirName(web_dir->Text());
			window->server->SetDirListFlag(checkbox_dir_list->Value());
			window->server->SetIndexFileName(index_name->Text());
			changedMaxConnections = window->SetMaxConnections(max_connections->Value());
			if (changedMaxConnections && window->server->IsRunning()) {
				AskToRestartServer();
				changedMaxConnections = false;
			}
		}
	}
	PrefsChanged = false;
	prefs->WriteSettingsFile();
}

bool PMPrefsWindow::QuitRequested(void) {
	if (PrefsChanged) {
		BAlert *alert;
		alert = new BAlert("", "You've changed one or more preference options.  Would you "
							   "like to save your changes?", 
							   "Cancel", "Don't Save", "Save", B_WIDTH_AS_USUAL,
							   B_OFFSET_SPACING, B_WARNING_ALERT);
		int32	result;
		if ((result = alert->Go()) == 2) {
			SaveSettings();
		} else if (result == 0) {
			return false;
		}
	}

	if (logpanel) {
		delete(logpanel);
		logpanel = NULL;
	}
	if (filepanel){
		delete(filepanel);
		filepanel = NULL;
	}

	prefs->WindowClosed();
	return true;
}

//
// AskToRestartServer
//
// If the maximum number of connections has changed,
// call this function to let the user decide to
// restart the server.
//
void PMPrefsWindow::AskToRestartServer(void) {
	BAlert *alert;
	alert = new BAlert("", "Changing the maximum number of simultaneous connections "
						   "does not take effect until the server is restarted.  Would "
						   "you like to restart the server now?", 
						   "Don't Restart", "Restart Server", NULL, B_WIDTH_FROM_WIDEST,
						   B_IDEA_ALERT);
	if (alert->Go() == 1) {
		BMessenger messenger(SIGNATURE);
		BMessage windowmsg(GET_PM_WINDOW);
		BMessage reply;
		PMWindow *window;
		messenger.SendMessage(&windowmsg, &reply);
		reply.FindPointer("window", (void **) &window);

		if (window) {
			window->Lock();
			window->StopServer();
			window->StartServer();
			window->Unlock();
		}
	}
}

PMPrefsWindow::~PMPrefsWindow() {
	delete message_me_baby;
}

void PMPrefsWindow::CreateLogFile(entry_ref *dir, const char *name) {
	BEntry entry;
	BPath path;
	
	if (!entry.SetTo(dir)) {
		entry.GetPath(&path);
		path.Append(name);
		logger->SetLogPath((char *) path.Path());
	}
}


void PMPrefsWindow::RefsReceived(BMessage *msg) {
	entry_ref ref;
	BEntry *dir;
	BPath path;
	status_t status;
	PMWindow *window;

	status = msg->FindRef("refs", &ref);
	if (status < B_OK) {
		return;
	}
	dir = new BEntry(&ref);
	dir->GetPath(&path);
	delete dir;

	BMessenger messenger(SIGNATURE);
	BMessage message(GET_PM_WINDOW);
	BMessage reply;
	messenger.SendMessage(&message, &reply);
	reply.FindPointer("window", (void **) &window);
	window->SetDirName(path.Path());
	web_dir->SetText(path.Path());
	if (filepanel) {
		delete(filepanel);
		filepanel = NULL;
	}
	PrefsChanged = true;
}

void PMPrefsWindow::MessageReceived(BMessage *message) {
	BMessenger messenger(SIGNATURE);
	BMessage windowmsg(GET_PM_WINDOW);
	BMessage reply;
	PMWindow *window;
	messenger.SendMessage(&windowmsg, &reply);
	reply.FindPointer("window", (void **) &window);

	switch(message->what) {
		case PREFS_CTL_DIR_LIST:
		case PREFS_CTL_LOG_CONSOLE:
		case PREFS_CTL_MAX_CONNECTIONS:
		case PREFS_CTL_LOG_FILE:
		case PREFS_CTL_WEB_DIRECTORY:
		case PREFS_CTL_INDEX_NAME:
		case PREFS_CTL_LOG_NAME:
			PrefsChanged = true;		// Something changed
			break;

		case B_CANCEL:			// File panel canceled
			if (message->FindInt32("old_what") == MSG_WEB_DIR_SELECT) {
				if (!window->server->GetDirName()) {
					be_app->PostMessage(B_QUIT_REQUESTED);
				}
				delete(filepanel);
				filepanel = NULL;
			}
			break;

		case PREFS_CTL_LOG_FILE_SELECT:
			if (!logpanel) {
				logpanel = new BFilePanel(B_SAVE_PANEL, message_me_baby);
				logpanel->Window()->Lock();
				logpanel->Window()->SetTitle("Create Poor Man Log");
				logpanel->Window()->Unlock();
				logpanel->SetButtonLabel(B_DEFAULT_BUTTON, "Create");
				//logpanel->SetMessage(new BMessage(MSG_LOG_FILE_SELECT));
				
				char *path;
				if ((path = (char *) logger->LogPath())) {
					BEntry ent;
					BDirectory dir;
					
					if (ent.SetTo(path) == B_OK) {
						if (ent.GetParent(&dir) == B_OK) {
							BPath p;
							p.SetTo(&dir, NULL);
							logpanel->SetPanelDirectory(&dir);
						}
					}
				}
			}
			logpanel->Show();
			break;

		case PREFS_CTL_WEB_DIR_SELECT:
				if (!filepanel) {
					bool modal;
					if (message->FindBool("modal", &modal) != B_OK) {
						modal = false;
					}
					filepanel= new BFilePanel(B_OPEN_PANEL, message_me_baby, NULL,
											B_DIRECTORY_NODE, false, new BMessage(MSG_WEB_DIR_SELECT),
											NULL, modal);
					filepanel->Window()->Lock();
					filepanel->Window()->SetTitle(prefStr_SiteButton);
					filepanel->Window()->Unlock();
					filepanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
					
					const char *path;
					prefs->FindString("web_directory", &path);
					if (path) {
						entry_ref ref;
						
						get_ref_for_path(path, &ref);	// Get ref to the path
						filepanel->SetPanelDirectory(&ref);
					}
				}
				filepanel->Show();
				break;

		case MSG_WEB_DIR_SELECT:
			RefsReceived(message);
			break;

		case B_SAVE_REQUESTED:			// log file created
			{
				const char *name;
				entry_ref ref;
				BEntry entry;
				BPath path;

				message->FindString("name", &name);
				message->FindRef("directory", &ref);
				CreateLogFile(&ref, name);

				if (!entry.SetTo(&ref)) {
					entry.GetPath(&path);
					path.Append(name);
					log_path->SetText(path.Path());
				}
				logger->LogToFile(true);
				log_to_file->SetValue(1);
				PrefsChanged = true;
			}
			break;
		
		case PREFS_CTL_OK:				// Save Settings
			SaveSettings();
			prefs->WindowClosed();
			Quit();
			break;
		
		case PREFS_CTL_CANCEL:			// Cancel setting changes
			prefs->WindowClosed();
			Quit();
			break;
		default:
			BWindow::MessageReceived(message);
	}
}
