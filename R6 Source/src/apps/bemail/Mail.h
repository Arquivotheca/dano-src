//--------------------------------------------------------------------
//	
//	Mail.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef MAIL_H
#define MAIL_H

#include <Application.h>
#include <Font.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MessageFilter.h>
#include <Point.h>
#include <Rect.h>
#include <Window.h>
#include <PopUpMenu.h>

#define MAX_DICTIONARIES	8
#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			457
#define WIND_HEIGHT			400
#define RIGHT_BOUNDARY 		8191
#define SEPERATOR_MARGIN	  7
#define	VIEW_COLOR			216
#define FONT_SIZE			 10.0
#define QUOTE				">"

enum	MESSAGES			{REFS_RECEIVED = 64, LIST_INVOKED, WINDOW_CLOSED,
							 CHANGE_FONT, RESET_BUTTONS, PREFS_CHANGED};

enum	TEXT				{SUBJECT_FIELD = REFS_RECEIVED + 64, TO_FIELD,
							 ENCLOSE_FIELD, CC_FIELD, BCC_FIELD, NAME_FIELD};

enum	MENUS	/* app */	{M_NEW = SUBJECT_FIELD + 64, M_PREFS, M_EDIT_SIGNATURE,
								M_FONT, M_STYLE, M_SIZE, M_BEGINNER, M_EXPERT,
				/* file */	 M_WRAP_TEXT, M_REPLY, M_REPLY_ALL, M_FORWARD, M_RESEND, M_COPY_TO_NEW,
								M_HEADER, M_RAW, M_SEND_NOW, M_SAVE_AS_DRAFT,
								M_SAVE, M_PRINT_SETUP, M_PRINT, M_DELETE,
								M_DELETE_PREV, M_DELETE_NEXT,
								M_CLOSE_READ, M_CLOSE_SAVED, M_CLOSE_SAME,
								M_CLOSE_CUSTOM, M_STATUS,
								M_OPEN_MAIL_BOX, M_OPEN_MAIL_FOLDER,
				/* edit */	 M_SELECT, M_QUOTE, M_REMOVE_QUOTE, M_CHECK_SPELLING,
								M_SIGNATURE, M_RANDOM_SIG, M_SIG_MENU, M_FIND, M_FIND_AGAIN,
				/* encls */	 M_ADD, M_REMOVE,
							 M_TO_MENU, M_CC_MENU, M_BCC_MENU,
				/* nav */	M_NEXTMSG, M_PREVMSG};

enum	USER_LEVEL			 {L_BEGINNER = 0, L_EXPERT};

enum	WINDOW_TYPES		 {MAIL_WINDOW = 0, PREFS_WINDOW, SIG_WINDOW};


class	TMailWindow;
class	THeaderView;
class	TEnclosuresView;
class	TContentView;
class	TMenu;
class	TPrefsWindow;
class	TSignatureWindow;
class	BMenuItem;
class	BmapButton;

class	BFile;
class	BFilePanel;
class	ButtonBar;
class	BMenuBar;
class	Words;
class	BMailMessage;


//====================================================================

class TMailApp : public BApplication {

public:

					TMailApp();
					~TMailApp();
	virtual void	AboutRequested();
	virtual void	ArgvReceived(int32, char**);
	virtual void	MessageReceived(BMessage*);
	virtual bool	QuitRequested();
	virtual void	ReadyToRun();
	virtual void	RefsReceived(BMessage*);
	TMailWindow*	FindWindow(const entry_ref&);
	void			FontChange();
	TMailWindow*	NewWindow(const entry_ref *rec = NULL, const char *to = NULL,
							  bool resend = false,BMessenger *msg = NULL);

	BFont			fFont;
	
private:
			void	ClearPrintSettings();
	
	BList				fWindowList;
	int32				fWindowCount;
	BFile				*fPrefs;
	TPrefsWindow		*fPrefsWindow;
	TSignatureWindow	*fSigWindow;
	BMessenger			*trackerMessenger;	// Talks to tracker window that
	bool				fPrevBBPref;		// this was launched from.
};

//--------------------------------------------------------------------

class TMailWindow : public BWindow {
public:

					TMailWindow(BRect, const char *, const entry_ref *, const char *,
						const BFont *font, bool, BMessenger*);
	virtual			~TMailWindow();
	virtual void	FrameResized(float width, float height);
	virtual void	MenusBeginning();
	virtual void	MessageReceived(BMessage*);
	virtual bool	QuitRequested();
	virtual void	Show();
	virtual void	Zoom(BPoint, float, float);
	virtual	void	WindowActivated(bool state);

	void			SetTo(const char *mailTo, const char *subject, const char *ccTo = NULL,
						const char *bccTo = NULL, const BString *body = NULL,
						BMessage *enclosures = NULL);
	void			AddSignature(BMailMessage*);
	void			Forward(entry_ref*);
	void			Print();
	void			PrintSetup();
	void			Reply(entry_ref*, TMailWindow*, bool);
	void			CopyMessage( entry_ref *ref, TMailWindow *src );
	status_t		Send(bool);
	status_t		SaveAsDraft( void );
	status_t		OpenMessage(entry_ref*);
	entry_ref*		GetMailFile() const;
	bool			GetTrackerWindowFile(entry_ref*, bool dir) const;
	void			SetCurrentMessageRead();
	void			SetTrackerSelectionToCurrent();
	TMailWindow*	FrontmostWindow();
	void			UpdateViews( void );
	
protected:
	void			AddEnclosure(BMessage *msg);
	void			BuildButtonBar( void );

private:

	entry_ref		*fRef;			// Reference to currently displayed file
	int32			fFieldState;
	BFile			*fFile;
	BFilePanel		*fPanel;
	BMenuBar		*fMenuBar;
	BMenuItem		*fAdd;
	BMenuItem		*fCut;
	BMenuItem		*fCopy;
	BMenuItem		*fHeader;
	BMenuItem		*fPaste;
	BMenuItem		*fPrint;
	BMenuItem		*fPrintSetup;
	BMenuItem		*fQuote;
	BMenuItem		*fRaw;
	BMenuItem		*fRemove;
	BMenuItem		*fRemoveQuote;
	BMenuItem		*fSendNow;
	BMenuItem		*fSendLater;
	BMenuItem		*fUndo;
	BMenuItem		*nextMsg;
	BMenuItem		*prevMsg;
	BMenuItem		*deleteNext;
	BMenuItem		*fSpelling;
	BMenu			*saveAddrMenu;
	ButtonBar		*fButtonBar;
	BmapButton		*fSendButton;
	BmapButton		*fSaveButton;
	BmapButton		*fPrintButton;
	BmapButton		*fSigButton;
	BRect			fZoom;
	TContentView	*fContentView;
	THeaderView		*fHeaderView;
	TEnclosuresView	*fEnclosuresView;
	TMenu			*fSignature;
	BMessenger		*trackerMessenger;		// Talks to tracker window that
											// this was launched from.
	static  BList	sWindowList;
	bool			fSigAdded;
	bool			fIncoming;
	bool			fReplying;
	bool			fResending;
	bool			fSent;
	bool			fDraft;
	bool			fChanged;
};

//====================================================================

class TMenu: public BPopUpMenu {
public:
					TMenu(const char*, const char*, int32, bool popup=false);
					~TMenu();
					
	virtual BPoint 	ScreenLocation(void);
	virtual void	AttachedToWindow();
	void			BuildMenu();

private:

	char			*fAttribute;
	char			*fPredicate;
	bool			fPopup;
	int32			fMessage;
};

//====================================================================

int32	header_len(BFile*);
extern Words *gWords[MAX_DICTIONARIES], *gExactWords[MAX_DICTIONARIES];
extern int32 gUserDict;
extern BFile *gUserDictFile;
extern int32 gDictCount;
#endif
