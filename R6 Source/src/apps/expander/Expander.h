//--------------------------------------------------------------------
//	
//	Expander.h
//
//	Written by: Robert Chinn
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef FAT_BITS_H
#define FAT_BITS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Debug.h>

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef	_FILE_PANEL_H
#include <FilePanel.h>
#endif
#include <FindDirectory.h>

#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>

#include <PopUpMenu.h>
#include <Screen.h>

#include "StatusWindow.h"
#include "PrefsWindow.h"
#include "XpandoMatic.h"
#include "XpandoWindow.h"
#include "utils.h"

#define msg_toggle_status 'stat'

enum {
	kDestFromSrc = 99,
	kDefaultDest,			// not used
	kCustomDest,
	kNoDest
};

//====================================================================

class TExpanderApp : public BApplication {
public:
					TExpanderApp(void);
					~TExpanderApp(void);

			void 	InitApp();
									
			void	DoAbout(void);
	virtual void	ArgvReceived(long argc, char** argv);
	virtual void	MessageReceived(BMessage*);
	virtual bool	QuitRequested(void);
	virtual void	RefsReceived(BMessage *theMessage);
			void	ReadyToRun();
			
			TXpandoWindow *FindFrontWindow(void);

			TXpandoWindow *CheckForDupRef(entry_ref *ref);
			void	NewRef(entry_ref *ref);
			void	ProcessRef(entry_ref *ref);
			
			BPoint NextWindowLoc();
			void	NewXpandoWindow(entry_ref *ref);
			
			void	InitStatus(void);
			void	ShowStatus(char *msg);
			void	HideStatus(void);
			bool 	StatusIsShowing(void);
			void 	ToggleStatusWind(void);
			
			void	GetPrefs(void);
			void	SetPrefs(void);
			void	UpdatePrefs(BMessage *msg);
			void	ShowPreferences(void);
			
			bool	AutoExtractRun() const;
			bool 	QuitWhenDone() const;
			short	DestSetting() const;
			void	DestRef(entry_ref *ref);
			bool	OpenDest() const;
			bool	ShowContents() const;
			void	SetWindowLoc(BPoint p);
			
private:
	bool					fAutoExtract;
	bool					fQuitWhenDone;
	short					fDestSetting;
	entry_ref			fDestRef;
	bool					fOpenDest;
	bool					fShowContents;
	
	TExpanderStatusWindow	*fStatusWind;
	bool					fStatusIsShowing;
	
	bool					fGotRefs;
	bool					fAutoExtractRun;
	
	BPoint				fWindowLoc;
	float 				fHOffset;

	typedef  BApplication inherited;
};

#endif














