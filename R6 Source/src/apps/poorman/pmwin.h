/* emacs: -*- C++ -*- */
/*
 * pmwin.h
 * Copyright (c) 1996 Be, Inc.  All Rights Reserved 
 *
 */
#ifndef _PMWIN_H
#define _PMWIN_H

#include <Window.h>
#include <Button.h>
#include <StringView.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <Box.h>
#include <ScrollView.h>
#include "pmlog.h"
#include "web_server.h"
#include "pmprefs.h"

class PMGetHits;
class BFilePanel;

class BackView : public BBox {
        public:
                BackView(BRect);
                void FrameResized(float, float);
                BTextView *log;
};

class PMWindow : public BWindow {
public:
        BStringView *htext;
        BStringView *stext;
        BStringView *dtext;
        BScrollView *box;
        BTextView *log;
    	BackView *background;
        BMenuBar *menuBar;
        WebServer *server;
        int quitting;
        bool isZoomed;
        bool logging;
        PMPrefs *settings;
        BMessenger *messenger;

        PMGetHits *gethits;
        BFilePanel*     filepanel;
        PMWindow(PMPrefs *prefs, char *dirname, bool hide=false);
		~PMWindow();
        void ReadSettings(void);
        void SaveSettings(void);

        bool SetMaxConnections(const int16 num);
        int16 GetMaxConnections(void) const;

        void RefsReceived(BMessage *message);
        void SetDirName(const char *dir);
        void StartServer(void);
        void StopServer(void);
        void UpdateMenus(void);
        void SaveConsole(void);
        void SaveConsoleSelection(void);
        char *NewDirectory(void);
        void AddToLog(const char *msg);
        virtual void MessageReceived(BMessage* message);
        virtual bool QuitRequested(void);
        virtual void Zoom(BPoint leftTop, float width, float height);

private:
        float lastWidth;
        float lastHeight;
        BFilePanel *saveLogPanel;
        BFilePanel *saveSelPanel;
        int16   max_connections;
};

class PMGetHits : public GetHits {
        PMWindow *win;
public:
        PMGetHits(PMWindow *win) { this->win = win; }
        void Hit(int hitcount);
};

// Returns true if the value actually changed
inline bool PMWindow::SetMaxConnections(const int16 num) {
        bool result = false;

        if (num != max_connections) {
                result = true;
        }
        max_connections = num;
        settings->SetInt16("max_connections", max_connections);
        return result;
}
inline int16 PMWindow::GetMaxConnections(void) const {
        return max_connections;
}

static const int32 SAVE_CONSOLE = 'SvCn';
static const int32 SAVE_SELECTION = 'ScSl';

#endif /* _PMWIN_H */
