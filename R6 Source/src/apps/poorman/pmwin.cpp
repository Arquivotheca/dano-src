/*
 * Copyright (c) 1999 Be, Inc.  All Rights Reserved 
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fs_info.h>
#include <Button.h>
#include <Alert.h>
#include <MenuBar.h>
#include <Volume.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <NodeInfo.h>
#include <Message.h>
#include <FilePanel.h>
#include <InterfaceDefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Entry.h>
#include <File.h>
#include <errno.h>
#include "pmapp.h"
#include "pmwin.h"
#include "hardcoded.h"
#include "EasyMenus.h"
#include "pmprefs.h"

static char *default_html_path = "/boot/home/public_html";

void alert( const char *format, ...);

void add_to_log(char *msg);

// The menu bar

MenuItemTemplate fileItemSaveConsole = {
        "Save Console as" B_UTF8_ELLIPSIS,
        'S', 0,
        MENU_FILE_SAVE_CONSOLE
};
MenuItemTemplate fileItemSaveConsoleSel = {
        "Save Console Selection as" B_UTF8_ELLIPSIS,
        0, 0,
        MENU_FILE_SAVE_CONSOLE_SEL
};

MenuItemTemplate fileItemAbout = {
        "About " PROGRAM_NAME,
        0, 0,
        B_ABOUT_REQUESTED
};
MenuItemTemplate fileItemQuit = {
        "Quit",
        'Q', 0,
        B_QUIT_REQUESTED
};

MenuItemTemplate *fileItemList[] = {
        &fileItemSaveConsole,
        &fileItemSaveConsoleSel,
        EZMenuSeparator,
        &fileItemAbout,
        EZMenuSeparator,
        &fileItemQuit,
        NULL
};

MenuTemplate fileMenuTemplate = {
        "File",
        B_ITEMS_IN_COLUMN,
        fileItemList
};

// Edit menu - new in R4

#if 0
MenuItemTemplate editItemUndo = {
        "Undo",
        'Z', 0,
        B_UNDO
};
MenuItemTemplate editItemCut = {
        "Cut",
        'X', 0,
        B_CUT
};
#endif
MenuItemTemplate editItemCopy = {
        "Copy",
        'C', 0,
        B_COPY
};
#if 0
MenuItemTemplate editItemPaste = {
        "Paste",
        'V', 0,
        B_PASTE
};
#endif
MenuItemTemplate editItemSelectAll = {
        "Select All",
        'A', 0,
        B_SELECT_ALL
};
MenuItemTemplate editItemPrefs = {
        "Preferences" B_UTF8_ELLIPSIS,
        0, 0,
        MENU_EDIT_PREFS
};

MenuItemTemplate *editItemList[] = {
        //&editItemUndo,
        //EZMenuSeparator,
        //&editItemCut,
        &editItemCopy,
        //&editItemPaste,
        EZMenuSeparator,
        &editItemSelectAll,
        EZMenuSeparator,
        &editItemPrefs,
        NULL
};

MenuTemplate editMenuTemplate = {
        "Edit",
        B_ITEMS_IN_COLUMN,
        editItemList
};

// Control menu

MenuItemTemplate webItemStatus = {
        "Run Server",
        0, 0,
        MENU_WEB_SERVER_RUN
};
MenuItemTemplate webItemClearHits = {
        "Clear Hit Counter",
        0, 0,
        MENU_WEB_CLEAR_HITS
};
MenuItemTemplate webItemConsoleClear = {
        "Clear Console Log",
        0, 0,
        MENU_WEB_CLEAR_CONSOLE
};
MenuItemTemplate webItemFileClear = {
        "Clear Log File",
        0, 0,
        MENU_WEB_CLEAR_FILE
};

MenuItemTemplate *webMenuList[] = {
        &webItemStatus,         // turn it on and off
        &webItemClearHits,
        EZMenuSeparator,
        &webItemConsoleClear,
        &webItemFileClear,
        NULL
};
MenuTemplate webMenuTemplate = {
        "Controls",
        B_ITEMS_IN_COLUMN,
        webMenuList
};

MenuTemplate *mainMenuList[] = {
        &fileMenuTemplate,
        &editMenuTemplate,
        &webMenuTemplate,
        NULL
};

MenuBarTemplate mainMenuBar = {
        "menubar",
        B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,
        B_ITEMS_IN_ROW,
        true,
        mainMenuList
};

void
PMGetHits::Hit(
                           int hitcount
                           )
{
        char buf[50];

        sprintf(buf, HIT_TEXT, hitcount);
        win->Lock();
        win->htext->SetText(buf);
        win->Unlock();
}

void
PMWindow::SetDirName(const char *dirname)
{
        char buf[1024];

        if (dirname) {
                sprintf(buf, DIR_TEXT, dirname);
        }
        else {
                sprintf(buf, DIR_TEXT, "(none)");
                StopServer();                   // Stop the server for null names
        }
        Lock();
        dtext->SetText(buf);
        Unlock();
        server->SetDirName(dirname);
}


//
// Read the window settings from the settings file.
//
//      Type    Name                    Description
//      ----    ----                    -----------
//      string  web_directory   The directory containing the web site.
//      string  index_file_name Name of the index file.
//      bool    is_zoomed               TRUE if window is zoomed, FALSE if not.
//      float   last_width              Last width (before last zoom hit).
//      float   last_height             Last height (before last zoom hit).
//      rect    window_frame    Window frame rectangle.
//
//      string  log_path                Log file pathname.
//      bool    log_console_flag TRUE if console logging is on.
//      bool    log_file_flag   TRUE if file logging is on.
//      int16   max_connections Maximum number of connections allowed at once.
//      bool    dir_list_flag   TRUE if directory listings are on.
//
//      rect    setwindow_frame Settings window rectangle.
//
void PMWindow::ReadSettings(void) {
        const char *s;

        // Web directory

        s = settings->FindString("web_directory");
        if (!s) {
                s = default_html_path;
        }
        struct stat st;
        if (stat(s, &st) < 0) {
                s = NewDirectory();
        }
        SetDirName(s);

        // Index file name

        s = settings->FindString("index_file_name");
        if (!s) {
                s = "index.html";
        }
        server->SetIndexFileName(s);

        // Directory listing flag

        server->SetDirListFlag(settings->FindBool("dir_list_flag"));

        // Maximum connections

        if (settings->FindInt16("max_connections", &max_connections) != B_OK) {
                max_connections = BACKLOG_DEFAULT;
        }

        // Now we read the window configuration.  To facilitate
        // dealing with cases where the window is lost in space,
        // this is skipped if the option key is held down.

        if (!(modifiers() & B_OPTION_KEY)) {
                BRect frame;

                // Get zoom flag

                isZoomed = settings->FindBool("is_zoomed");     // Default is false
                lastWidth = settings->FindFloat("last_width");
                lastHeight = settings->FindFloat("last_height");
                frame = settings->FindRect("frame");
                if (frame.IsValid()) {
                        MoveTo(frame.left, frame.top);
                        ResizeTo(frame.Width(), frame.Height());
                }
                else {
                        frame = Frame();
                }
                if (!lastWidth) {
                        lastWidth = frame.Width();
                }
                if (!lastHeight) {
                        lastHeight = frame.Height();
                }
        }
        SaveSettings();                 // If the settings didn't exist, create it now
}

void PMWindow::SaveSettings(void) {
        if (server->GetDirName()) {
                settings->SetString("web_directory", server->GetDirName());
        }
        if (server->GetIndexFileName()) {
                settings->SetString("index_file_name", server->GetIndexFileName());
        }
        settings->SetBool("dir_list_flag", server->GetDirListFlag());

        // Only save window settings if option key isn't down

        if (!(modifiers() & B_OPTION_KEY)) {
                settings->SetBool("is_zoomed", isZoomed);
                settings->SetFloat("last_width", lastWidth);
                settings->SetFloat("last_height", lastHeight);
                settings->SetRect("frame", Frame());
        }
        logger->SaveSettings();
}


PMWindow::PMWindow(
                                   PMPrefs *prefs,
                                   char *dirname,
                                   bool hide
                                   )
        : BWindow(BRect(WIND_XLEFT, WIND_YTOP, WIND_XRIGHT, WIND_YBOTTOM),
                                WIND_TITLE, B_TITLED_WINDOW, 0)
{
        BRect rect;

        messenger = new BMessenger(this);
        settings = prefs;
        saveLogPanel = NULL;
        saveSelPanel = NULL;

        quitting = 0;
        isZoomed = true;
        logging = true;

        Lock();

        SetSizeLimits(WIND_XRIGHT-WIND_XLEFT, 1200,
                                  HTEXT_YBOTTOM+35, 1200);

        // Set up the menu bar

        rect.Set(0.0, 0.0, Bounds().right, 20.0);
        AddChild(menuBar = EZCreateMenuBar(&mainMenuBar, rect));
        fileItemAbout.itemPtr->SetTarget(be_app);

        rect = Bounds();
//+     rect.top = 20;
        rect.top = menuBar->Frame().bottom + 1;
        background = new BackView(rect);
        AddChild(background);
        background->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

        rect.Set(HTEXT_XLEFT, HTEXT_YTOP, HTEXT_XRIGHT, HTEXT_YBOTTOM);
        background->AddChild(htext = new BStringView(rect, 
                                                                                "", ZERO_HIT_TEXT,
                                                                                B_FOLLOW_RIGHT,
                                                                                B_WILL_DRAW));
        htext->SetAlignment(B_ALIGN_RIGHT);


        rect.Set(STEXT_XLEFT, STEXT_YTOP, STEXT_XRIGHT, STEXT_YBOTTOM);
        background->AddChild(stext = new BStringView(rect, 
                                                                                "", STOPPED_TEXT,
                                                                                B_FOLLOW_NONE,
                                                                                B_WILL_DRAW));


        rect.Set(DTEXT_XLEFT, DTEXT_YTOP, DTEXT_XRIGHT, DTEXT_YBOTTOM);
        background->AddChild(dtext = new BStringView(rect, 
                                                                                "", "/",
                                                                                B_FOLLOW_NONE,
                                                                                B_WILL_DRAW));

        rect = Frame();
        rect.OffsetTo(0, 0);
        rect.left += BOX_XLEFT;
        rect.right -= (B_V_SCROLL_BAR_WIDTH+BOX_XLEFT);
        rect.top = BOX_YTOP;
        rect.bottom -= (BOX_XLEFT+20);  // Bottom margin & menu bar adjust

        BRect textrect;
        textrect = rect;
        textrect.OffsetTo(0,0);
        textrect.InsetBy(1,1);
        log = new BTextView(rect, "log", textrect,
                                                 B_FOLLOW_ALL, B_WILL_DRAW|B_NAVIGABLE);
        log->MakeEditable(false);
        background->log = log;
        background->AddChild(box = new BScrollView("", log, B_FOLLOW_ALL, B_FRAME_EVENTS, false, true));

        gethits = new PMGetHits(this);

        htext->SetFont(be_plain_font);
        htext->SetFontSize(FONT_SIZE);
        htext->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
        htext->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

        dtext->SetFont(be_plain_font);
        dtext->SetFontSize(FONT_SIZE);
        dtext->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
        dtext->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

        stext->SetFont(be_plain_font);
        stext->SetFontSize(FONT_SIZE);
        stext->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
        stext->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

        logger = new PMLog(server, this);
        server = new WebServer();

        ReadSettings();
        StartServer();          // this was before ReadSettings()

        if (dirname) {
                SetDirName(dirname);            // -d flag overrides directory
        }

        server->SetCallback(gethits);

        UpdateMenus();
        Unlock();
        if (hide) {
                Minimize(true);
        }
        log->MakeFocus(true);

        // If we didn't start up, and are about to quit,
        // don't bother to show the window.

        if (!quitting) {
                Show();
        }
}

PMWindow::~PMWindow() {
	delete messenger;
}

void PMWindow::UpdateMenus(void) {
        webItemStatus.itemPtr->SetMarked(server->IsRunning());
}


//
// NewDirectory
//
// Help the user set up a new HTML directory.
//
char *PMWindow::NewDirectory(void) {
        BAlert *alrt;
        char *result = NULL;
        int btn;

        // See if the boot disk is read-only; if it is, the default
        // path won't work, so we'll force them to create one elsewhere
        // or quit.

        dev_t bootdev = dev_for_path("/boot");
        BVolume vol(bootdev);
        if (vol.IsReadOnly()) {
                alrt = new BAlert(NULL, PROGRAM_NAME " you have not yet selected the folder you want " PROGRAM_NAME \
                																" to publish on the Web.\n\n" \
                                                                                "Would you like to select a folder to publish?",  \
                                                                                "Cancel", "Select", NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING);
        }
        else {
                alrt = new BAlert(NULL, "You have not yet selected the folder you want " PROGRAM_NAME \
                																" to publish on the Web.  " PROGRAM_NAME \
                																" can create a " B_UTF8_OPEN_QUOTE "public_html" \
                																B_UTF8_CLOSE_QUOTE " folder in your home folder " \
                																"or you can select one of your own folders to publish.\n\n" \
                                                                                "Do you wish to select a folder to publish on the Web?", \
                                                                                "Cancel", "Select", "Default", B_WIDTH_AS_USUAL, B_OFFSET_SPACING);
        }

        btn = alrt->Go();
        if (!btn) {
                be_app->PostMessage(B_QUIT_REQUESTED);
                quitting = true;
        }
        else if (btn == 1) {
                BMessage *message = new BMessage(PREFS_CTL_WEB_DIR_SELECT);
                message->AddBool("modal", true);
                settings->ShowWindow(BPoint(50,50))->PostMessage(message);
        }
        else {
                if (create_directory(default_html_path, 0777) == B_OK) {
                        alert("A default Web directory has been created for you at " \
                                  B_UTF8_OPEN_QUOTE "/boot/home/public_html." B_UTF8_CLOSE_QUOTE \
                                  "  Make sure an HTML document " \
                                  "named " B_UTF8_OPEN_QUOTE "index.html" B_UTF8_CLOSE_QUOTE \
                                  " is in that directory.");
                }
                else {
                        alert("Unable to create the default Web directory.");
                }
                result = default_html_path;
        }
        return result;
}


void
alert(
          const char *format,
          ...
          )
{
        BAlert *alert;
        va_list args;
        char buf[1024];

        va_start(args, format);
        vsprintf(buf, format, args);
        va_end(args);
        alert = new BAlert("", buf, OK);
        alert->Go();
}

void PMWindow::StopServer(void) {
        server->Stop();
        webItemStatus.itemPtr->SetMarked(false);
        stext->SetText(STOPPED_TEXT);
}

void PMWindow::StartServer(void) {
        if (server->Start(max_connections)) {
                webItemStatus.itemPtr->SetMarked(true);
                stext->SetText(RUNNING_TEXT);
        }
        else {
                alert(CANT_START_SERVER);
        }
}


void PMWindow::SaveConsole(void) {
        if (!saveLogPanel) {
                saveLogPanel = new BFilePanel(B_SAVE_PANEL, messenger,
                                                                NULL, 0, false, new BMessage(SAVE_CONSOLE));
                saveLogPanel->Window()->Lock();
                saveLogPanel->Window()->SetTitle("Save Log Console");
                saveLogPanel->Window()->Unlock();
        }
        saveLogPanel->Show();
}

void PMWindow::SaveConsoleSelection(void) {
        if (!saveSelPanel) {
                saveSelPanel = new BFilePanel(B_SAVE_PANEL, messenger,
                                                                NULL, 0, false, new BMessage(SAVE_SELECTION));
                saveSelPanel->Window()->Lock();
                saveSelPanel->Window()->SetTitle("Save Log Console Selection");
                saveSelPanel->Window()->Unlock();
        }
        saveSelPanel->Show();
}

void 
PMWindow::MessageReceived(
                                                  BMessage *message
                                                  )
{
        switch (message->what) {
                case B_SELECT_ALL:
                        log->SelectAll();
                        break;

        case MENU_WEB_SERVER_RUN:
                if (!server->GetDirName()) {
                        StopServer();
                        alert(NEED_HTML_DIRECTORY);
                }
                else {
                        if (server->IsRunning()) {
                                StopServer();
                        } else {
                                StartServer();
                        }
                }
                break;

        case MENU_WEB_CLEAR_HITS:
                server->ClearHits();
                break;

        case MENU_FILE_SAVE_CONSOLE:
                SaveConsole();
                break;

        case MENU_FILE_SAVE_CONSOLE_SEL:
                SaveConsoleSelection();
                break;

                case SAVE_CONSOLE: {
                        BFile *file;
                        BEntry entry;
                        BPath path;
                        entry_ref dir;

                        message->FindRef("directory", &dir);

                        if (!entry.SetTo(&dir)) {
                                entry.GetPath(&path);
                                path.Append(message->FindString("name"));
                                entry.SetTo(path.Path());
                                file = new BFile(&entry, B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
                                if (file->InitCheck() == B_OK) {
                                        BNodeInfo nodeInfo(file);
                                        nodeInfo.SetType("text/plain");
                                        file->Write(log->Text(), log->TextLength());
                                }
                        }
                } break;

                case SAVE_SELECTION: {
                        BFile *file;
                        BEntry entry;
                        BPath path;
                        entry_ref dir;
                        int32 start, end;

                        message->FindRef("directory", &dir);

                        if (!entry.SetTo(&dir)) {
                                entry.GetPath(&path);
                                path.Append(message->FindString("name"));
                                entry.SetTo(path.Path());
                                file = new BFile(&entry, B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
                                if (file->InitCheck() == B_OK) {
                                        BNodeInfo nodeInfo(file);
                                        nodeInfo.SetType("text/plain");
                                        log->GetSelection(&start, &end);
                                        file->Write(log->Text()+start, end-start);
                                }
                        }
                } break;

        case MENU_WEB_CLEAR_CONSOLE:
                logger->ClearConsole();
                break;

        case MENU_WEB_CLEAR_FILE:
                logger->ClearFile();
                break;

        case MENU_EDIT_PREFS:
                {
                        BRect r;
                        if (settings->FindRect("setwindow_frame", &r) != B_OK) {
                                r = Frame();
                                r.OffsetBy(30.0,30.0);
                        }
                        settings->ShowWindow(r.LeftTop());
                }
                break;

        default:
                BWindow::MessageReceived(message);
                break;
        }
        UpdateMenus();
}


bool 
PMWindow::QuitRequested(void)
{
        char buf[1024];
        BRect frame;

        SaveSettings();         // Save all settings

        if (server->IsRunning()) {
                StopServer();
        }

        quitting=true;
        be_app->PostMessage(B_QUIT_REQUESTED);
        return true;
}

void PMWindow::Zoom(BPoint leftTop, float width, float height) {
        if (isZoomed) {
                BRect r = Bounds();
                isZoomed = false;
                lastWidth = r.right-r.left;
                lastHeight = r.bottom-r.top;
                ResizeTo(WIND_XRIGHT-WIND_XLEFT, HTEXT_YBOTTOM+35);
        }
        else {
                isZoomed = true;
                ResizeTo(lastWidth, lastHeight);
        }
}


BackView::BackView(BRect rect)
        : BBox(rect, "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS,
                B_PLAIN_BORDER)
{
}

void BackView::FrameResized(float x, float y) {
        BRect r;

        r = Bounds();
        r.left += BOX_XLEFT;
        r.right -= (B_V_SCROLL_BAR_WIDTH + BOX_XLEFT);
        r.top += BOX_YTOP;
        r.bottom -= 5;
        r.OffsetTo(0,0);
        log->SetTextRect(r);

        BBox::FrameResized(x, y);
}

