//--------------------------------------------------------------------
//	
//	Content.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef CONTENT_H
#define CONTENT_H

#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_attr.h>
#include <Point.h>
#include <Rect.h>

#define MESSAGE_TEXT		"Message:"
#define MESSAGE_TEXT_H		 16
#define MESSAGE_TEXT_V		 5
#define MESSAGE_FIELD_H		 59
#define MESSAGE_FIELD_V		 11

#define CONTENT_TYPE		"content-type: "
#define CONTENT_ENCODING	"content-transfer-encoding: "
#define CONTENT_DISPOSITION	"Content-Disposition: "
#define MIME_TEXT			"text/"
#define MIME_MULTIPART		"multipart/"

class	TMailWindow;
class	TScrollView;
class	TTextView;
class	BFile;
class 	BList;
class	BPopupMenu;

struct text_run_array;

typedef struct {
	bool		header;
	bool		raw;
	bool		quote;
	bool		incoming;
	bool		close;
	bool		mime;
	TTextView	*view;
	BFile		*file;
	BList		*enclosures;
	sem_id		*stop_sem;
} reader;

enum ENCLOSURE_TYPE		{TYPE_ENCLOSURE = 100, TYPE_BE_ENCLOSURE,
						 TYPE_URL, TYPE_MAILTO};

typedef struct {
	int32		type;
	char		*name;
	char		*content_type;
	char		*encoding;
	int32		text_start;
	int32		text_end;
	off_t		file_offset;
	off_t		file_length;
	bool		saved;
	bool		have_ref;
	entry_ref	ref;
	node_ref	node;
} hyper_text;

bool	get_semaphore(BWindow*, sem_id*);
bool	insert(reader*, char*, int32, bool);
bool	parse_header(char*, char*, off_t, char*, reader*, off_t*);
bool	strip_it(char*, int32, reader*);

class	TSavePanel;


//====================================================================

class TContentView : public BView {
public:

	TTextView		*fTextView;

					TContentView(BRect, bool, BFile*, BFont*); 
	virtual void	MessageReceived(BMessage*);
	void			FindString(const char*);
	void			Focus(bool);
	void			FrameResized(float, float);
	
private:

	typedef BView	inherited;

	bool			fFocus;
	bool			fIncoming;
	float			fOffset;
};

//====================================================================

enum {
	S_CLEAR_ERRORS = 1,
	S_SHOW_ERRORS = 2
};

class TTextView : public BTextView {
public:


					TTextView(BRect, BRect, bool, BFile*, TContentView*,BFont*);
					~TTextView();
	virtual	void	AttachedToWindow();
	virtual void	KeyDown(const char*, int32);
	virtual void	MakeFocus(bool);
	virtual void	MessageReceived(BMessage*);
	virtual void	MouseDown(BPoint);
	virtual void	MouseMoved(BPoint, uint32, const BMessage*);
	virtual void	InsertText( const char *text, int32 length, int32 offset, const text_run_array *runs );
	virtual void 	DeleteText(int32 start, int32 finish);
            
	void			ClearList();
	void			LoadMessage(BFile*, bool, bool, const char*);
	void			Open(hyper_text*);
	static status_t	Reader(reader*);
	status_t		Save(BMessage*,bool makeNewFile = true );
	void			SaveBeFile(BFile*, char*, ssize_t);
	void			StopLoad();
	void			AddAsContent(BMailMessage*, bool);
	void			CheckSpelling( int32 start, int32 end, int32 flags = S_CLEAR_ERRORS | S_SHOW_ERRORS );
	void			FindSpellBoundry( int32 length, int32 offset, int32 *start, int32 *end );
	void			EnableSpellCheck( bool enable );

	bool			fHeader;
	bool			fReady;

private:
	
	void 			ContentChanged( void );
	
	char			*fYankBuffer;
	int32			fLastPosition;
	BFile			*fFile;
	BFont			fFont;
	TContentView	*fParent;
	sem_id			fStopSem;
	thread_id		fThread;
	BList			*fEnclosures;
	BPopUpMenu		*fEnclosureMenu;
	BPopUpMenu		*fLinkMenu;
	TSavePanel		*fPanel;
	bool			fIncoming;
	bool			fSpellCheck;
	bool			fRaw;
	bool			fCursor;
};


//====================================================================

class TSavePanel : public BFilePanel {
public:

					TSavePanel(hyper_text*, TTextView*);
	virtual void	SendMessage(const BMessenger*, BMessage*);
	void			SetEnclosure(hyper_text*);

private:

	hyper_text		*fEnclosure;
	TTextView		*fView;
};

#endif
