//******************************************************************************
//
//	File:		Window.cpp
//
//	Description:	BWindow class.
//			Application Windows objects.
//
//	Written by:	Peter Potrebic & Benoit Schillings
//
//	Copyright 1992-97, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#define _CACHE_			1
#define _USE_RUNNER_	1

#include <Beep.h>
#include <Debug.h>
#include <StreamIO.h>
#include <StopWatch.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <AppDefsPrivate.h>
#include <OS.h>
#include <messages.h>
#include <token.h>

#include <Directory.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <Application.h>
#include <InterfaceDefs.h>
#include <Alert.h>
#include <Button.h>
#include <Dragger.h>
#include <List.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuWindow.h>
#include <MessageQueue.h>
#include <Picture.h>
#include <ScrollBar.h>
#include <Screen.h>
#include <TextView.h>
#include <ToolTip.h>
#include <View.h>
#include <Window.h>

#include <message_util.h>
#include <MessageBody.h>
#include <interface_misc.h>
#include <MenuPrivate.h>
#include <session.h>
#include <Roster.h>
#include <Autolock.h>
#include <TraverseViews.h>
#include <archive_defs.h>
#include <encoding_tables.h>
#include "SaveScreen.h"
#include <PropertyInfo.h>
#include <input_server_private.h>
#if DEBUG
#include <roster_private.h>
#endif

bool substituteCursorHack = false;
unsigned char B_HAND_CURSOR_SUBSTITUTE[268];
unsigned char B_I_BEAM_CURSOR_SUBSTITUTE[268];

#include "TCPIPSocket.h"

#define DEFAULT_PULSE_RATE 500000

// This magic is from MenuWindow.cpp.
const window_feel _PRIVATE_MENU_WINDOW_FEEL_ = (window_feel)1025;

/* macro to convert a uchar* utf8_string into a uint16* uni_string,
   one character at a time. Move the pointer. You can check terminaison on
   the string by testing : if (string[0] == 0).
   WARNING : you need to use EXPLICIT POINTERS for both str and unistr. */
#define convert_to_unicode(str, uni_str) \
{\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
        *uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		int   val;\
		val = ((str[0]&7)<<18) | ((str[1]&63)<<12) | ((str[2]&63)<<6) | (str[3]&63);\
		uni_str[0] = 0xd7c0+(val>>10);\
		uni_str[1] = 0xdc00+(val&0x3ff);\
		uni_str += 2;\
		str += 4;\
	}\
}

/* macro to convert a ushort* uni_string into a char* or uchar* utf8_string,
   one character at a time. Move the pointer. You can check terminaison on
   the uni_string by testing : if (uni_string[0] == 0)
   WARNING : you need to use EXPLICIT POINTERS for both str and unistr. */
#define convert_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

/*---------------------------------------------------------------*/

struct Event {
	uint32			next;
	int16			eventType;
	int16			transit;
	void *			meaningless;
	message_header	header;
	uint8			theRest[1];
};

enum {
	epEvents			= 0x0000FFFF,
	epWindowResized		= 0x00010000,
	epWindowMoved 		= 0x00020000,
	epViewsMoveSized	= 0x00040000,
	epBlockClient		= 0x00080000
};

class _CEventPort_ {

	public:

							_CEventPort_(
								int32 *atomic,
								uint32 *nextBunch,
								uint32 eventBase,
								sem_id clientBlock);
							~_CEventPort_();

		void				ProcessPending();
		BMessage *			GenerateMessage();
		int32				Messages();

	private:
	
		int32 *				m_atomic;
		sem_id				m_clientBlock;
		uint32 *			m_nextBunch;
		uint32				m_eventBase;
		int32				m_eventsQueued;
		bool				m_pending;
};

/*---------------------------------------------------------------*/

_CEventPort_::_CEventPort_(
	int32 *atomic,
	uint32 *nextBunch,
	uint32 eventBase,
	sem_id clientBlock)
{
	m_atomic = atomic;
	m_eventBase = eventBase;
	m_clientBlock = clientBlock;
	m_nextBunch = nextBunch;
	m_eventsQueued = 0;
	m_pending = false;
};

_CEventPort_::~_CEventPort_()
{
};

int32 _CEventPort_::Messages()
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	return m_eventsQueued;
};

void _CEventPort_::ProcessPending()
{
	uint32 oldValue;
	status_t err;
	
	if (m_eventsQueued) {
		m_pending = true;
		return;
	};
	
	m_pending = false;
	oldValue = atomic_and(m_atomic,0);
	while (oldValue & epBlockClient) {
		do {
			err = acquire_sem(m_clientBlock);
		} while (err == B_INTERRUPTED);
		oldValue = atomic_and(m_atomic,0);
	};
	m_eventsQueued = oldValue & epEvents;
};

BMessage * _CEventPort_::GenerateMessage()
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	if (!m_eventsQueued) return NULL;

	Event *msg = (Event *)(*m_nextBunch + m_eventBase);
	m_nextBunch = &msg->next;
	m_eventsQueued--;

	BMessage *bm = new BMessage();
	bm->Unflatten((char*)&msg->header);
	
	return bm;
};

/*---------------------------------------------------------------*/

struct _cmd_key_ {
	ulong		key;
	ulong		mod;

	bool		targetFocused;
	BHandler	*scut_target;

	// one of the following 3 fields should be set
	BMenuItem	*item;
	BMessage	*msg;
};

namespace BPrivate {

/*---------------------------------------------------------------*/

struct win_tip_info {
	BToolTip* fTip;
	BView* fShower;
	BRect fRegion;		// These are in fShower's coordinate space
	BPoint fCursor;
};

/*---------------------------------------------------------------*/

struct win_pulse_state {
	win_pulse_state()
		: fRate(DEFAULT_PULSE_RATE), fNext(B_INFINITE_TIMEOUT),
		  fNeeded(false), fEnabled(false)
	{
	}
	
	bigtime_t fRate;
	bigtime_t fNext;
	bool fNeeded;
	bool fEnabled;
};

/*---------------------------------------------------------------*/

}	// namespace BPrivate

#define kIsShown 0
const uint32 kStackStringSize = 64;

/*---------------------------------------------------------------*/

BWindow::BWindow(BRect bound, const char* name, window_type type, uint32 flags,
	uint32 workspaces)
		: BLooper(name, B_DISPLAY_PRIORITY), accelList(4)
{
	AssertLocked();
	
	window_look theLook;
	window_feel	theFeel;
	decompose_type(type, &theLook, &theFeel);

	InitData(bound, name, theLook, theFeel, flags, workspaces);
}

/*---------------------------------------------------------------*/

BWindow::BWindow(
	BRect		bound, 
	const char	*name, 
	window_look look, 
	window_feel	feel,
	uint32 		flags,
	uint32 		workspaces)
		: BLooper(name, B_DISPLAY_PRIORITY), accelList(4)
{
	AssertLocked();
	InitData(bound, name, look, feel, flags, workspaces);
}

/*---------------------------------------------------------------*/

BWindow::BWindow(BMessage *data)
	: BLooper(data), accelList(4)
{
	BRect 		frame;
	const char 	*name;
	window_type	wtype;
	uint32		flags;
	long		wspaces;
	window_look	wlook;
	window_feel	wfeel;

	AssertLocked();
	data->FindRect(S_FRAME, &frame);

	/*
	 Warning - the following code is in place to work around a bug - 
	 in PR1 & PR2 the window code was incorrectly saving/restoring the
	 window title. It was placing the title in S_NAME, but BHandler was
	 using the same entry for the handler's name! But was fixed in R3,
	 but need to ensure that older archives will at least find some sort
	 of name.
	*/
	if (data->FindString(S_TITLE, &name) != B_OK) {
//+		PRINT(("old title"));
		data->FindString(S_NAME, &name);
	}

	data->FindInt32(S_FLAGS, (int32 *)&flags);
	
	status_t err = data->FindInt32(S_WLOOK, (int32 *)&wlook);
	if (err == B_NO_ERROR)
		err = data->FindInt32(S_WFEEL, (int32 *)&wfeel);
	
	if (err != B_NO_ERROR) {
		data->FindInt32(S_TYPE, (int32 *)&wtype);
		decompose_type(wtype, &wlook, &wfeel);
	}

	data->FindInt32(S_WORKSPACES, &wspaces);

	// ??? throw an exception if there's an error
	InitData(frame, name, wlook, wfeel, flags, wspaces);

	float	f;
	if (data->FindFloat(S_ZOOM_LIMITS, &f) == B_OK) {
		fMaxZoomH = f;
		data->FindFloat(S_ZOOM_LIMITS, 1, &fMaxZoomV);
	}

	if (data->FindFloat(S_SIZE_LIMITS, &f) == B_OK) {
		fMinWindH = f;
		data->FindFloat(S_SIZE_LIMITS, 1, &fMinWindV);
		data->FindFloat(S_SIZE_LIMITS, 2, &fMaxWindH);
		data->FindFloat(S_SIZE_LIMITS, 3, &fMaxWindV);
		SetSizeLimits(fMinWindH, fMaxWindH, fMinWindV, fMaxWindV);
	}

	bigtime_t	bt;
	if (data->FindInt64(S_PULSE_RATE, &bt) == B_OK) {
		fPulseState = new win_pulse_state;
		fPulseState->fRate = bt;
	}

	UnarchiveChildren(data);
}

/*---------------------------------------------------------------*/

status_t BWindow::Archive(BMessage *data, bool deep) const
{
	/*
	 ???
	 The shortcuts aren't archived now because there is no way to also
	 archive the 'targets' of those messages.
	*/

	inherited::Archive(data);

	data->AddRect(S_FRAME, Frame());
	data->AddString(S_TITLE, Title());

	data->AddInt32(S_WLOOK, fLook);
	data->AddInt32(S_WFEEL, fFeel);

	window_type theType = compose_type(fLook, fFeel);
	if (theType != B_UNTYPED_WINDOW)
		data->AddInt32(S_TYPE, theType);

	if (fFlags)
		data->AddInt32(S_FLAGS, fFlags);

	data->AddInt32(S_WORKSPACES, Workspaces());

	if ((fMaxZoomH != 0) || (fMaxZoomV != 0)) {
		data->AddFloat(S_ZOOM_LIMITS, fMaxZoomH);
		data->AddFloat(S_ZOOM_LIMITS, fMaxZoomV);
	}

	if ((fMaxWindH != 0) || (fMaxWindV != 0)
		|| (fMinWindH != 0) || (fMinWindV != 0)) {
			data->AddFloat(S_SIZE_LIMITS, fMinWindH);
			data->AddFloat(S_SIZE_LIMITS, fMinWindV);
			data->AddFloat(S_SIZE_LIMITS, fMaxWindH);
			data->AddFloat(S_SIZE_LIMITS, fMaxWindV);
	}

	if (fPulseState != NULL && fPulseState->fRate != DEFAULT_PULSE_RATE) {
		data->AddInt64(S_PULSE_RATE, fPulseState->fRate);
	}

	// now archive all the child views
	if (deep)
		ArchiveChildren(data, deep);
	return 0;
}

/*---------------------------------------------------------------*/

status_t BWindow::UnarchiveChildren(BMessage *data)
{
	// MUST pass 'this'. Can't call BView::AddChild to add a top_level view
	return top_view->UnarchiveChildren(data, this);
}

/*---------------------------------------------------------------*/

status_t BWindow::ArchiveChildren(BMessage *data, bool deep) const
{
	return top_view->ArchiveChildren(data, deep);
}

/*---------------------------------------------------------------*/

BArchivable *BWindow::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BWindow"))
		return NULL;
	return new BWindow(data);
}

/*-------------------------------------------------------------*/

void 
BWindow::PreInitData(
	const char	*name, 
	window_look	look,
	window_feel	feel,
	uint32		flags)
{
	fTitle = NULL;
	SetLocalTitle(name);
	server_token = NO_TOKEN;
	fInUpdate = false;
	f_active = 2;
	
	// All windows start out as hidden
	fShowLevel = kIsShown + 1;
	
	fFlags = flags;
	
	send_port = B_BAD_PORT_ID;
	receive_port = B_BAD_PORT_ID;
	
	top_view = NULL;
	fFocus = NULL;
	fLastMouseMovedView = NULL;
	
	a_session = NULL;

	fKeyMenuBar = NULL;
	fDefaultButton = NULL;
#if _R5_COMPATIBLE_
	fKeyIntercept = NULL;
#endif
	top_view_token = NO_TOKEN;
	fPulseState = NULL;
	
	fWaitingForMenu = false;
	fOffscreen = false;
	fIsNavigating = false;
	fIsFilePanel = false;
	fNoQuitShortcut = false;
	
	fMenuSem = B_NO_MORE_SEMS;

	fMaxZoomH = 0;
	fMaxZoomV = 0;
	fMinWindH = 0;
	fMinWindV = 0;
	fMaxWindH = 0;
	fMaxWindV = 0;
	
	fFrame.Set(0, 0, 0, 0);
	
	fLook = look;
	fCurDrawViewState = NULL;

	fFeel = feel;
	fLastViewToken = -1;
	fEventPort = NULL;
	
	fTipInfo = NULL;
	fWindowColor = make_color(255, 255, 255);
}

/*-------------------------------------------------------------*/

void 
BWindow::InitData(
	BRect		bound, 
	const char	*name, 
	window_look	look,
	window_feel	feel,
	uint32		flags, 
	uint32		workspaces)
{
	PreInitData(name, look, feel, flags);
	
	AppSession	*init_session;
	long		name_length;
	BRect		view_bound;
	BRect		window_bound;
	const char	*fake_name;
	bool		remote=false;

	fake_name = name ? name : "";
//	PRINT(("window(sem=%d)\n", fLockSem));

	window_bound = bound;

	// All and only B_MODAL_WINDOWs are MODAL
	if (IsModal())
		be_app->SetCursor(B_HAND_CURSOR);		//reset the cursor for modal windows !!

	{
	_BAppServerLink_ link;
	
#ifdef REMOTE_DISPLAY
	char *asName = getenv("APP_SERVER_NAME");
	if (asName && (strncasecmp(asName,"tcp:",4) == 0)) {
		char *p,*d,host[80];
		int32 r;
		int16 port = 666;
		d = host;
		p = asName+4;
		while (*p && (*p != ':')) *d++ = *p++;
		*d = 0;
		if (*p == ':') port = atoi(p+1);

		int32 buffer[2];
		buffer[0] = RMT_WINDOW_SOCKET;
		buffer[1] = Team();
		TCPIPSocket *socket = new TCPIPSocket();
		if (!socket->Open(host,port)) printf("open failed!\n");
		r = socket->Send(buffer,8);
		init_session = new AppSession(socket);
		remote = true;
	} else
#endif
	{
		init_session = new AppSession(be_app->fServerTo, fake_name);
		init_session->swrite_l(GR_CREATE_WINDOW);
		init_session->swrite_l(_find_cur_team_id_());
	}

	name_length = strlen(fake_name) + 1;

	char		buf[32+32];
	if (name) {
		strncpy(buf, name, 32);
		buf[32] = 0;
		strcat(buf, "w_rcv_port");	// should be 31 chars max
	} else {
		strcpy(buf, "WindowRcvPort"); // should be 63 chars max
	}
//	receive_port = create_port(100, buf);

	view_bound.top = 0;
	view_bound.left = 0;
	view_bound.bottom = (bound.bottom - bound.top);
	view_bound.right = (bound.right - bound.left);

	// pass NULL for view_name to hide it from FindView()
	top_view = new BView(view_bound, NULL, B_FOLLOW_ALL, 0);
	top_view->set_owner(this);
	set_focus(NULL, false);

	// Are we using some initial workspace?
	ulong wspace;
	if ((workspaces == B_CURRENT_WORKSPACE) && be_app->IsLaunching() &&
		((wspace = be_app->InitialWorkspace()) != B_CURRENT_WORKSPACE)) {
		workspaces = wspace;
	}

	init_session->swrite_rect(&window_bound);	
	init_session->swrite_l(fLook);
	init_session->swrite_l(fFeel);
	init_session->swrite_l(fFlags);
	init_session->swrite_l(_get_object_token_(this));
	init_session->swrite_l(-1);
	init_session->swrite_l(fMsgPort);
	init_session->swrite_l(workspaces);
	init_session->swrite_l(name_length);
	init_session->swrite  (name_length, (void *)fake_name);

	init_session->flush();

	sem_id clientBlock;
	uint32 firstEvent,atomicVar;

	init_session->sread_rect(&fCurrentFrame);
	init_session->sread(4, &server_token);
	init_session->sread(4, &send_port);
	init_session->sread(4, &top_view_token);
	init_session->sread(4, &clientBlock);
	init_session->sread(4, &firstEvent);
	init_session->sread(4, &atomicVar);

	if (!remote) {
		fEventPort = new _CEventPort_(
			(int32*)be_app->rw_offs_to_ptr(atomicVar),
			(uint32*)be_app->ro_offs_to_ptr(firstEvent),
			(uint32)be_app->ro_offs_to_ptr(0),
			clientBlock);
	};

	a_session = init_session;

	}
	
#if _CACHE_
	fCurDrawViewState = new _view_attr_();
#else
	fCurDrawViewState = NULL;
#endif

	fParent = NULL;

	// If the window is user closable then add the alt-w shortcut
	if (!((fFlags & B_NOT_CLOSABLE) || IsModal()))
		AddShortcut('W', B_COMMAND_KEY, new BMessage(B_CLOSE_REQUESTED));
		
	if (!(fFlags & B_NOT_ZOOMABLE)) 
		AddShortcut('Z', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage(B_ZOOM));

	if (!(fFlags & B_NOT_MINIMIZABLE))
		AddShortcut('M', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage(B_MINIMIZE));
	
	// add Cut/Copy/Paste/Select All key shortcuts. 
	// They work even without menus.
	// By passing NULL they will get sent to the focused view by default.
	AddShortcut('C', B_COMMAND_KEY, new BMessage(B_COPY), (BView *)NULL);
	AddShortcut('X', B_COMMAND_KEY, new BMessage(B_CUT), (BView *)NULL);
	AddShortcut('V', B_COMMAND_KEY, new BMessage(B_PASTE), (BView *)NULL);
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), (BView *)NULL);
}

/*-------------------------------------------------------------*/

BWindow::BWindow(BRect frame, color_space depth, uint32 bitmapFlags, int32 rowBytes)
	: BLooper("bm"), accelList(5)
{
	PreInitData("", B_NO_BORDER_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | _BITMAP_WINDOW_FLAG_);
	fOffscreen = true;
	
	AppSession	*init_session;
	long		name_length;
	BRect		view_bound;

	AssertLocked();

	_BAppServerLink_ link;

#ifdef REMOTE_DISPLAY
	bool remote=false;
	char *asName = getenv("APP_SERVER_NAME");
	if (asName && (strncasecmp(asName,"tcp:",4) == 0)) {
		char *p,*d,host[80];
		int32 r;
		int16 port = 666;
		d = host;
		p = asName+4;
		while (*p && (*p != ':')) *d++ = *p++;
		*d = 0;
		if (*p == ':') port = atoi(p+1);

		int32 buffer[2];
		buffer[0] = RMT_OFFSCREEN_WINDOW_SOCKET;
		buffer[1] = Team();
		TCPIPSocket *socket = new TCPIPSocket();
		if (!socket->Open(host,port)) printf("open failed!\n");
		r = socket->Send(buffer,8);
		init_session = new AppSession(socket);
		remote = true;
	} else
#endif
	{
		init_session = new AppSession(be_app->fServerTo);
		init_session->swrite_l(GR_CREATE_OFF_WINDOW);
		init_session->swrite_l(_find_cur_team_id_());
	}

	view_bound.top = 0;
	view_bound.left = 0;
	view_bound.bottom = (frame.bottom - frame.top);
	view_bound.right = (frame.right - frame.left);

	// pass NULL for view_name to hide it from FindView()
	top_view = new BView(view_bound, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	top_view->set_owner(this); //??? How do we handle this?
	set_focus(NULL, false);

	name_length = strlen(fTitle) + 1;

	init_session->swrite_rect(&frame);
	init_session->swrite_l(depth);
	init_session->swrite_l(fFlags);
	init_session->swrite_l(bitmapFlags);
	init_session->swrite_l(rowBytes);
	init_session->swrite_l(_get_object_token_(this));
	init_session->swrite_l(-1);
	init_session->swrite_l(fMsgPort);
	init_session->swrite_l(name_length);
	init_session->swrite  (name_length, fTitle);
	init_session->flush();

	init_session->sread(4, &server_token);
	init_session->sread(4, &send_port);

	a_session = init_session;

	/*
	 Notice the BLooper::Run() isn't called here, because we don't
	 want the client side task for the window. It isn't needed.
	*/

	/*
	 A bitmap window will never call Run() (which does the initial
	 Unlock. So let's do the Unlock here.
	*/

	Unlock();
}

/*-------------------------------------------------------------*/

void	BWindow::GetSizeLimits(float *min_h, float *max_h,
	float *min_v, float *max_v)
{
	*min_v = fMinWindV;
	*min_h = fMinWindH;

	if (fMaxWindV == 0.0)
		*max_v = 32768.0;
	else
		*max_v = fMaxWindV;

	if (fMaxWindH == 0.0)
		*max_h = 32768.0;
	else
		*max_h = fMaxWindH;
}

/*-------------------------------------------------------------*/

void	BWindow::SetSizeLimits(float min_h, float max_h,
	float min_v, float max_v)
{
	long	bid;	

	fMinWindH = floor(min_h + 0.5);
	fMinWindV = floor(min_v + 0.5);
	fMaxWindH = floor(max_h + 0.5);
	fMaxWindV = floor(max_v + 0.5);

	if (Lock()) {
		a_session->swrite_l(GR_WINDOW_LIMITS);
		a_session->swrite_l(floor(min_h+0.5));
		a_session->swrite_l(floor(max_h+0.5));
		a_session->swrite_l(floor(min_v+0.5));
		a_session->swrite_l(floor(max_v+0.5));
		
		Flush();
		a_session->sread(sizeof(long), &bid);
		Unlock();
	}
}

/*-------------------------------------------------------------*/

BWindow::~BWindow()
{
	int		i;
	int		count;

//	PRINT(("BWindow::~BWindow(sem=%d)\n", fLockSem));
	if (fMenuSem != B_NO_MORE_SEMS) {
		/*
		 We must be in the middle of menu tracking. When the menu
		 tracking code is finished it will release and DELETE the
		 semaphore. This will cause the following acquire_sem to
		 return and then the Close will continue.
		*/

		long c = fOwnerCount;
		ASSERT(c);
//+		SERIAL_PRINT(("unlock all (%s), tid=%d\n",
//+			Title(), find_thread(NULL)));
		for (long i = 0; i < c; i++)
			Unlock();

		BMenu	*mv;
		if (CurrentFocus() && (mv = dynamic_cast<BMenu *>(CurrentFocus())) != 0) {
			// a menu is currently focused, so ALT down causes an exit.
			mv->fState = EXIT_TRACKING;
		}

		status_t	err;
		do {
			err = acquire_sem(fMenuSem);
		} while (err == B_INTERRUPTED);

		fMenuSem = B_NO_MORE_SEMS;
	}

	if (!Lock()) {
		SERIAL_PRINT(("Lock failed 1\n"));
		return;
	}

	delete fPulseState;
	fPulseState = NULL;

	// Must interate through all the top "real" views and remove from window
	// This must be done before trying to delete the views so that when
	// DettachedFromWindow is called the view is still fully constructed.
	BView *child;
	while ((child = top_view->ChildAt(0)) != 0) {
		child->RemoveSelf();
		delete child;
	}

	// the top_view isn't a real view, so we can't call RemoveSelf(). But we
	// need to set the owner to NULL so the the runtime debug code in
	// BView::~BView doesn't notice an owner.
	top_view->set_owner(NULL);
	delete top_view;

//	if (a_session->send_port != B_BAD_PORT_ID) {
		a_session->swrite_l(GR_CLOSE_WINDOW);
		a_session->swrite_l(server_token);
		a_session->flush();
//		a_session->sread(4,&server_token);
		a_session->sclose();
//	}
	delete a_session;

	_cmd_key_	*a;
	count = accelList.CountItems();
	for(i = 0; i < count; i++) {
		a = (_cmd_key_ *)accelList.ItemAt(i);
		ASSERT((a));
		if (a->msg)
			delete(a->msg);
		free(a);
	}

	if (fTitle) {
		free(fTitle);
		fTitle = NULL;
	}

	delete fTipInfo;
	fTipInfo = NULL;
	
//+	if (fMsgBuffer) {
//+		free(fMsgBuffer);
//+		fMsgBuffer = NULL;
//+	}

#if _CACHE_
	delete fCurDrawViewState;
	fCurDrawViewState = NULL;
#endif

	delete (fEventPort);
}

/*-------------------------------------------------------------*/

void	BWindow::Quit()
{
	if (!IsLocked()) {
		fprintf(stderr,"ERROR - you must Lock a window before calling Quit(), team=%ld, window=%s\n",
			_find_cur_team_id_(), Title() ? Title() : "no-name");
	}

	if (!Lock()) {
/*		debugger("Lock on window in BWindow::Close failed.");*/
		return;
	}

	if (fRunCalled) 
	{ 
		while( fShowLevel <= kIsShown )
			Hide();
	}

	if( (Flags()&B_QUIT_ON_WINDOW_CLOSE) != 0 ) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	inherited::Quit();
}

/*-------------------------------------------------------------*/

void	BWindow::BitmapClose()
{
	delete this;
}

/*-------------------------------------------------------------*/

void	BWindow::Activate(bool bringFront)
{
	if (Lock()) {
		if (fShowLevel > kIsShown) {
//			??? The browser hits this print when you hide the dock window
//			PRINT(("a Hidden window was being activated\n"));
			Unlock();
			return;
		}

		if (bringFront)
			a_session->swrite_l(GR_SELECT_WINDOW);
		else
			a_session->swrite_l(GR_SEND_TO_BACK);

		a_session->swrite_l(server_token);
		Flush();
		Unlock();
	}
}

/*-------------------------------------------------------------*/

BMenuBar *BWindow::KeyMenuBar() const
{
	return fKeyMenuBar;
}

/*-------------------------------------------------------------*/

void	BWindow::SetKeyMenuBar(BMenuBar *bar)
{
	fKeyMenuBar = bar;
}

/*-------------------------------------------------------------*/

static BMenuItem *find_item_with_shortcut(BMenu *menu, ulong key, ulong mod)
{
	long		i = 0;
	BMenuItem	*item;

	while ((item = menu->ItemAt(i++)) != 0) {
		ulong	im;
		char	is = item->Shortcut(&im);
		if (item->Submenu()) {
			item = find_item_with_shortcut(item->Submenu(), key, mod);
			if (item)
				return item;
		} else if (item->IsEnabled() && is && (tolower(is) == tolower(key))
			&& (im == mod)) {
				return item;
		}
	}
	return NULL;
}

/*-------------------------------------------------------------*/

void	BWindow::do_key_up(BMessage *an_event, BHandler *handler)
{
//+	PRINT_OBJECT((*an_event));

	BView		*target_view = NULL;
	bool		is_non_view;

	// some specific target was set
	target_view = handler ? cast_as(handler, BView) : NULL;
	is_non_view = (target_view == NULL);

	if (is_non_view) {
		if (handler)
			handler->MessageReceived(an_event);
		else
			MessageReceived(an_event);
	} else {
		int32		i, encoding, length, tmp_length;
		uint32		dummyType;
		char		*tmp_string;
		ushort		tmp_uni[2];
		ushort		*table, *uni_str;
		char		stackString[kStackStringSize + 1];
		char		*string = stackString;
		bool		mallocedString = false;

		ASSERT(target_view);

		an_event->GetInfo("byte", &dummyType, &length);
		if (length > (int32)kStackStringSize) {
			string = (char *)malloc(length + 1);
			mallocedString = true;
		}

		for (i = 0; i < length; i++)
			an_event->FindInt8("byte", i, (int8 *)&string[i]);
		string[length] = '\0';

		encoding = target_view->font_encoding();
		if (encoding == B_UNICODE_UTF8) {
			i = 0;
			while (i < length) {
				int32	numBytes = 1;
				uchar	bytes[5];
				bytes[0] = string[i];
				while ((string[i + numBytes] & 0xC0) == 0x80) {
					bytes[numBytes] = string[i + numBytes];
					numBytes++;
				}
				bytes[numBytes] = '\0';
				// This is a gross hack because BControl didn't used to
				// implement KeyUp().
#if _R5_COMPATIBLE_
				if (fKeyIntercept) fKeyIntercept->BControl::KeyUp((char *)bytes, numBytes);
				else
#endif
					target_view->KeyUp((char *)bytes, numBytes);
				i += numBytes;
			}
		}
		else {
			const char	*anotherStr = string;
			char		tmp_stackString[kStackStringSize + 1];
			tmp_string = tmp_stackString;

			if (mallocedString)
				tmp_string = (char*)malloc(length+1);

			tmp_length = 0;
			table = b_symbol_set_table[encoding];

			while (anotherStr[0] != 0) {
				if ((anotherStr[0] & 0x80) == 0)
					tmp_string[tmp_length++] = *anotherStr++;
				else {
					uni_str = tmp_uni;
					convert_to_unicode(anotherStr, uni_str);
					for (i=128; i<256; i++)
						if (table[i] == tmp_uni[0]) {
							tmp_string[tmp_length++] = i;
							break;
						}
				}
			}
			tmp_string[tmp_length] = 0;
			for (int32 i = 0; i < tmp_length; i++) {
				uchar bytes[2];
				bytes[0] = tmp_string[i];
				bytes[1] = '\0';
				// This is a gross hack because BControl didn't used to
				// implement KeyUp().
#if _R5_COMPATIBLE_
				if (fKeyIntercept) fKeyIntercept->BControl::KeyUp((char *)bytes, 1);
				else
#endif
					target_view->KeyUp((char *)bytes, 1);
			}			
			if (mallocedString)
				free(tmp_string);
		}
		Flush();
		if (mallocedString)
			free(string);
//+		PRINT(("KEY_UP (%d), view=%s (%s)\n", find_thread(NULL),
//+			target_view->Name(), class_name(target_view)));
//+		target_view->KeyUp();
	}
}

/*-------------------------------------------------------------*/

void	BWindow::do_key_down(BMessage *an_event, BHandler *handler)
{
	ulong		mod;
	uchar 		ch;
	ulong		raw;
	ulong 		menu_ch;

	if (an_event->FindInt8("byte", 0, (int8 *)&ch) != B_NO_ERROR)
		return;

	// ignore shift_lock and num_lock
	an_event->FindInt32("modifiers", (long*) &mod);
	mod = mod & 
		(B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY |
		 B_OPTION_KEY | B_MENU_KEY);
	an_event->FindInt32("raw_char", (long*) &menu_ch);

//+	PRINT(("char=0x%x (%c), mod=0x%x (B_ESACPE=%x)\n", ch, ch, mod, B_ESCAPE));

	if ((mod == B_COMMAND_KEY) && (ch == B_ESCAPE)) {
		BMenuWindow	*mw = dynamic_cast<BMenuWindow *>(this);
		BMenu		*mv;
		if (mw) {
			// This window is a menu, so want to exit menu tracking.
			mw->fMenu->fState = EXIT_TRACKING;
		} else if (CurrentFocus() && (mv = dynamic_cast<BMenu *>(CurrentFocus())) != 0) {
			// a menu is currently focused, so exit.
			mv->fState = EXIT_TRACKING;
		} else if (fKeyMenuBar) {
			fKeyMenuBar->StartMenuBar(0);
		}
		return;
	} else if ((menu_ch == B_TAB) && (mod & B_COMMAND_KEY)) {
		kb_navigate();
		return;
	} else if ((menu_ch == B_TAB) && (mod & B_CONTROL_KEY)) {
		BMessage	msg('TASK');
		BMessenger	target("application/x-vnd.Be-TSKB", -1, NULL);

		bool repeated_key = an_event->HasInt32("be:key_repeat");
		if (!repeated_key && target.IsValid()) {
			an_event->FindInt32("key", (int32 *)&menu_ch);
			msg.AddInt32("key", (int32)menu_ch);
			msg.AddInt32("modifiers", (int32)mod);
			msg.AddInt32("team", Team());
			msg.AddInt32("token", server_token);
			msg.SetWhen(system_time());
			
			target.SendMessage(&msg);
		}
		return;
	} else if (mod & B_COMMAND_KEY) {

//+		PRINT_OBJECT((*an_event));
		// slow down auto-repeating shortcuts by skipping every other event
		int32	repeats = an_event->FindInt32("be:key_repeat");
		if ((repeats % 2) == 1) {
//+			PRINT(("Repeat: %d\n", repeats));
			return;
		}

#if _SUPPORTS_FEATURE_SCREEN_DUMP
#if __POWERPC__
		// MacOS-style print screen = Command + Shift + 3
		if ((mod == (B_COMMAND_KEY | B_SHIFT_KEY)) && (ch == '#')) {
			_save_screen_to_file_();	// spawns a thread
			return;
		}
#endif
#endif

//+		an_event->FindInt32("key", (long*) &raw);
//+		PRINT(("MENU char: 0x%x (%c), raw=0x%x, map: 0x%x (%c), (mods = 0x%x)\n",
//+			ch, ch, raw, menu_ch, menu_ch, mod));
//+		PRINT(("class=%s\n", class_name(this)));

		if ((menu_ch == 'q') && (mod == B_COMMAND_KEY) && (!IsModal()) && 
			(dynamic_cast<BMenuWindow *>(this) == NULL) && (!fNoQuitShortcut)) {
			// ALT-Q (don't do this for modal windows)
			BMessage msg(B_QUIT_REQUESTED);
			msg.AddBool("shortcut", true);
			(void) be_app->PostMessage(&msg);
			// If app is too busy, thus filling up its port this attempted
			// Quit will be lost. That's seems reasonable.
			return;
		}

		MenusBeginning();
		_cmd_key_	*a = FindShortcut(menu_ch, mod);
		
		if (a) {
			if (a->item) {
				BMenu *menu = a->item->Menu();
				ASSERT((menu));
				menu->InvokeItem(a->item, true);
			} else {
				BMessage msg(*a->msg);
				msg.SetWhen(CurrentMessage()->When());
				
				/*
				NOTE - if the message is supposed to go to the focused
				view, but there isn't a focused view, then the message
				gets sent to the window.
				*/

				BHandler *target = a->scut_target;
				if (a->targetFocused)
					target = NULL;
//+				PRINT(("target = %x, (%s)\n",
//+					target, target ? class_name(target) : "null"));

				(void) PostMessage(&msg, target);
				/*
				 If the window's port is full the window is poorly
				 designing, using up all of a resource. In this case the
				 cmd-shortcut will be lost/ignored.
				*/
			}
			MenusEnded();
			return;
		}
		MenusEnded();

#if DEBUG
		if ((tolower(menu_ch) == 's') &&
			(mod == (B_COMMAND_KEY | B_SHIFT_KEY | B_CONTROL_KEY))) {
			BMessage	msg(CMD_SAVE_MIME_TABLE);
			_send_to_roster_(&msg, NULL, true);
		}
#endif

		return;
	} else if ((((mod & B_OPTION_KEY) != 0) &&
				((ch == B_TAB) || (ch == B_RETURN)))) {
		/*
		 The <Option> key is used to override when doing navigation.
	 	*/
		kb_navigate();
		return;
	}

	an_event->FindInt32("key", (long*) &raw);
#if _SUPPORTS_FEATURE_SCREEN_DUMP
	if ((ch == B_FUNCTION_KEY) && (raw == B_PRINT_KEY)) {
		_save_screen_to_file_();	// spawns a thread
		return;
	}
#endif

//+	PRINT(("char: 0x%x (%c)", ch, ch));

	BView		*target_view = NULL;
	bool		is_non_view;

	// some specific target was set
	target_view = cast_as(handler, BView);
	is_non_view = (target_view == NULL);

	if (is_non_view) {
		ASSERT(handler);
		handler->MessageReceived(an_event);
	} else {
		int32		i, encoding, length, tmp_length;
		uint32		dummyType;
		char		*tmp_string;
		ushort		tmp_uni[2];
		ushort		*table, *uni_str;
		char		stackString[kStackStringSize + 1];
		char		*string = stackString;
		bool		mallocedString = false;

		ASSERT(target_view);

		an_event->GetInfo("byte", &dummyType, &length);
		if (length > (int32)kStackStringSize) {
			string = (char *)malloc(length + 1);
			mallocedString = true;
		}

		for (i = 0; i < length; i++)
			an_event->FindInt8("byte", i, (int8 *)&string[i]);
		string[length] = '\0';

		encoding = target_view->font_encoding();
		if (encoding == B_UNICODE_UTF8) {
			i = 0;
			while (i < length) {
				int32	numBytes = 1;
				uchar	bytes[5];
				bytes[0] = string[i];
				while ((string[i + numBytes] & 0xC0) == 0x80) {
					bytes[numBytes] = string[i + numBytes];
					numBytes++;
				}
				bytes[numBytes] = '\0';
				target_view->KeyDown((char *)bytes, numBytes);
				i += numBytes;
			}
		}
		else {
			const char	*anotherStr = string;
			char		tmp_stackString[kStackStringSize + 1];
			tmp_string = tmp_stackString;

			if (mallocedString)
				tmp_string = (char*)malloc(length+1);

			tmp_length = 0;
			table = b_symbol_set_table[encoding];

			while (anotherStr[0] != 0) {
				if ((anotherStr[0] & 0x80) == 0)
					tmp_string[tmp_length++] = *anotherStr++;
				else {
					uni_str = tmp_uni;
					convert_to_unicode(anotherStr, uni_str);
					for (i=128; i<256; i++)
						if (table[i] == tmp_uni[0]) {
							tmp_string[tmp_length++] = i;
							break;
						}
				}
			}
			tmp_string[tmp_length] = 0;
			for (int32 i = 0; i < tmp_length; i++) {
				uchar bytes[2];
				bytes[0] = tmp_string[i];
				bytes[1] = '\0';
#if _R5_COMPATIBLE_
				if (fKeyIntercept) fKeyIntercept->BControl::KeyDown((char *)bytes, 1);
				else
#endif
					target_view->KeyDown((char *)bytes, 1);
			}	
			if (mallocedString)		
				free(tmp_string);
		}
		Flush();
		if (mallocedString)
			free(string);
	}
}

/*-------------------------------------------------------------*/

void BWindow::kb_navigate()
{
	BMessage *msg = CurrentMessage();
	ASSERT(msg->what == B_KEY_DOWN);

	uchar ch = 0;
	if (msg->FindInt8("byte", (int8 *)&ch) != B_NO_ERROR)
		return;

	switch (ch) {
		case B_TAB:
			{
			ulong mods;
			msg->FindInt32("modifiers", (long*) &mods);
			// command key means to navigate between 'control' groups
			bool groups = (mods & B_COMMAND_KEY) != 0;
			if (mods & B_SHIFT_KEY)
				navigate_to_next(B_BACKWARD, groups);
			else
				navigate_to_next(B_FORWARD, groups);
			break;
			}
		default:
			break;
	}
}

/*------------------------------------------------------------*/

void	BWindow::navigate_to_next(int32 dir, bool switch_group)
{
	uint32  flags=0;
	BView*  next;
	BView*  prev;
	BView*  stop;
	
	if (dir != B_BACKWARD)
	  flags |= B_NAVIGATE_NEXT;
	  
	if (switch_group) flags |= B_NAVIGATE_GROUP;  
	
#if _SUPPORTS_EXCEPTION_HANDLING
	try {
#endif
		fIsNavigating = true;
		prev = CurrentFocus();
		stop = prev;
		
		while (true) {
			next = NextNavigableView(prev, flags);
			if (!next) break;
			
			next->MakeFocus(true);
			
			if (next->IsFocus()) {
				next->SetExplicitFocus();
				break;
			}	
			
			if (!stop)	{
				stop = next;
			}	
			else if (stop == next)	{
				break;
			}
		
			prev = next; // Start over, further this time	
		}
		
#if _SUPPORTS_EXCEPTION_HANDLING
	} catch(...) {
		fIsNavigating = false;
		throw;
	}
#endif

	fIsNavigating = false;
}

/*-------------------------------------------------------------*/

BView *BWindow::LastMouseMovedView() const
{
	return fLastMouseMovedView;
}

/*-------------------------------------------------------------*/

void	BWindow::do_mouse_moved(BMessage *an_event, BView *target)
{
	BPoint		loc;
	int32		but;
	BMessage	*dragging = NULL;
	status_t	err;
	int32		lockCount=0;

	// Remember who is moving.
	fLastMouseMovedView = target;
	
	do {
		Unlock();
		lockCount++;
	} while (IsLocked());

	bigtime_t when = an_event->When();
	if (an_event->HasInt32("_msg_what_")) {
		uint32 dmWhat = (uint32)an_event->FindInt32("_msg_what_");
		area_id messageArea = an_event->FindInt32("_msg_data_");
		bigtime_t dragWhen;
		bool hasWhen = (an_event->FindInt64("_msg_when_", (int64*)&dragWhen) == B_OK);
			
		if (be_app->fDraggedMessage->serverArea == messageArea) {
			/*	This is the same message as last time, so try to use
				the already existing area. */
			do {
				err = acquire_sem(be_app->fDraggedMessage->sem);
			} while (err == B_INTERRUPTED);
			if (be_app->fDraggedMessage->serverArea == messageArea) {
				/*	Cool.  The message we saw last time is still around, and
					we've just read-locked the area, so nobody will touch it
					while we're working with the message. */
				dragging = _reconstruct_msg_(dmWhat, be_app->fDraggedMessage->areaBase);
				if (dragging && hasWhen) dragging->SetWhen(dragWhen);
			} else {
				/*	Oops.  Somebody has either removed or changed the area
					that we already had cloned for this message.  That means
					at the very least that the old message is no longer being
					dragged, so we can forget about it. */
				release_sem(be_app->fDraggedMessage->sem);
				dragging = NULL;
			};
		} else {
			if (be_app->fDraggedMessage->timestamp < when) {
				/*	We need to try to clone the area.  First acquire a write
					lock. */
				do {
					err = acquire_sem_etc(be_app->fDraggedMessage->sem,100000,0,0);
				} while (err == B_INTERRUPTED);
				if (be_app->fDraggedMessage->serverArea == messageArea) {
					/*	How convenient!  Someone has already mapped the
						message into our address space for us.  Don't look
						the proverbial gift area in the mouth... just use
						it.  First drop down into a read lock. */
					release_sem_etc(be_app->fDraggedMessage->sem,
						99999,B_DO_NOT_RESCHEDULE);
					dragging = _reconstruct_msg_(dmWhat, be_app->fDraggedMessage->areaBase);
					if (dragging && hasWhen) dragging->SetWhen(dragWhen);
				} else {
					uint32 ptr;
					area_id newArea =
						clone_area("draggedMessage",(void**)(&ptr),
								   B_ANY_ADDRESS,B_READ_AREA,messageArea);
					if (newArea < 0) {
						/*	There was an error cloning the area.  Either
							we're out of memory, or the server area doesn't
							exist any more, which means that the drag is
							done, so just ignore the message drag. */
						release_sem_etc(be_app->fDraggedMessage->sem,
							100000,B_DO_NOT_RESCHEDULE);
						dragging = NULL;
					} else {
						/*	We have the app's dragmessage area write locked,
							and we've just cloned the message the server is
							dragging.  We're ready to update the app's notion
							of the message being dragged, but first we have
							to remove any old area that's still around from
							a previous drag. */
						if (be_app->fDraggedMessage->area != B_BAD_VALUE)
							delete_area(be_app->fDraggedMessage->area);
						be_app->fDraggedMessage->serverArea = messageArea;
						be_app->fDraggedMessage->area = newArea;
						be_app->fDraggedMessage->areaBase = ptr;
						be_app->fDraggedMessage->timestamp = when;
						
						/*	Done updating the area.  Now drop down into a
							read lock and set up the dragged message as
							normal. */
						release_sem_etc(be_app->fDraggedMessage->sem,
							99999,B_DO_NOT_RESCHEDULE);
						dragging = _reconstruct_msg_(dmWhat, be_app->fDraggedMessage->areaBase);
						if (dragging && hasWhen) dragging->SetWhen(dragWhen);
					};
				};
			} else {
				/*	The timestamp on the message status is _after_ the data
					given us by this message, which means that whatever
					message it was, it's gone, so don't bother locking
					anything or trying to get the message. */
				dragging = NULL;
			};
		};
	} else {
		/*	If the timestamp on this message is after that of the last
			dragmessage, we can delete it, since that means that drag is
			no longer going on. */
		if ((be_app->fDraggedMessage->area != B_BAD_VALUE) &&
			(be_app->fDraggedMessage->timestamp < when)) {
			/* We can try to erase it */
			do {
				err = acquire_sem_etc(be_app->fDraggedMessage->sem,100000,0,0);
			} while (err == B_INTERRUPTED);
			/* Check again now that we're locked */
			if ((be_app->fDraggedMessage->area != B_BAD_VALUE) &&
				(be_app->fDraggedMessage->timestamp < when)) {
				delete_area(be_app->fDraggedMessage->area);
				be_app->fDraggedMessage->serverArea = B_BAD_VALUE;
				be_app->fDraggedMessage->area = B_BAD_VALUE;
				be_app->fDraggedMessage->areaBase = (uint32) NULL;
				be_app->fDraggedMessage->timestamp = when;
			};
			release_sem_etc(be_app->fDraggedMessage->sem,
				100000,B_DO_NOT_RESCHEDULE);
		};
	};

	while (lockCount--) Lock();

	an_event->FindPoint("be:view_where", &loc);
	an_event->FindInt32("buttons", &but);
	
	// Now that we are locked back up, check to make sure that
	// the last mouse moved field is still the same.  It could change,
	// for example, if the view was removed from this window while
	// it was unlocked.
	if (fLastMouseMovedView == target)
		target->MouseMoved(loc, an_event->FindInt32("be:transit"), dragging);

	// Similar thing with the tool tip.  Note that we do this check
	// again because it is completely reasonable to remove a view
	// in the MouseMoved() callback.  If the view has changed, or a
	// button is pressed, we just want to hide the current tool tip
	// rather than giving the view a chance to show it.
	if (but || (fLastMouseMovedView != target))
		KillTip();
	else
		MoveTipCursor(target, loc);
	
	if (dragging) {
		delete dragging;
		release_sem(be_app->fDraggedMessage->sem);
	};

	/* Why do we need this here? --geh */
	Flush();
}


/*-------------------------------------------------------------*/

void	BWindow::do_mouse_down(BMessage *an_event, BView *target)
{
	BPoint	loc;

	ASSERT(target);

	/*
	 We have a synchronization issue here between the time the app_server
	 created this event and determined the target, and now. During that
	 span of time the target view might have been removed from the window.
	 In that case we should just ignore the event.
	*/
	if (target->Window() != this)
		return;

//+	PRINT(("focus = %s\n", fFocus ? class_name(fFocus) : "<none>"));

	/*
	 Need some special code here to handle the case where the window
	 is in menu tracking mode. If that's the case, and the mouse down
	 isn't in the menu then we need some special code here to cause
	 menu tracking to exit. Otherwise the menu tracking thread might
	 not ever see the mouse down - so it won't exit.
	*/
	BMenuBar *mb;;

	if (fFocus && (mb = dynamic_cast<BMenuBar *>(fFocus)) != 0 && (target != mb)) {
		// cause menu tracking to exit
		if (mb->fTracking) {
			// do all this drawing here, otherwise the window won't redraw
			// until the mouse goes up.
			mb->SelectItem(NULL);
			if (mb->IsStickyMode()) {
				mb->SetStickyMode(false);
				mb->RedrawAfterSticky(mb->Bounds());
			}
			mb->RestoreFocus();
			mb->fState = EXIT_TRACKING;
		}
	}

	an_event->FindPoint("be:view_where", &loc);

	ASSERT(target);

	target->MouseDown(loc);
	Flush();
}

/*-------------------------------------------------------------*/

#define ufPostDraw 0x00000001

typedef	struct {
	int32	view_token;
	uint32	flags;
	BRect	current_bound;
	BRect	update_rect;
} update_info;


/*-------------------------------------------------------------*/

void	BWindow::do_draw_views()
{
	long			view_count,i,result;
	uint32			view_list,notShared;
	update_info		*infoBase = NULL;
	update_info		*infos;
	BView			*view;
	BRect			update_rect;

	DEBUG_ONLY(bool ok =) Lock();
	ASSERT(ok);

	a_session->swrite_l(E_START_DRAW_WIND);
	a_session->flush();
	a_session->sread(4,&view_count);
	notShared = view_count & 0x80000000;
	view_count &= 0x7FFFFFFF;
	
	if (!view_count) {
		a_session->sread(4,&view_list);
		Unlock();
		return;
	}

	fLastViewToken = -1;
	fInUpdate = true;

	if (notShared) {
		infoBase = infos = (update_info *)malloc(sizeof(update_info)*view_count);
		a_session->sread(sizeof(update_info)*view_count,infos);
	} else {
		a_session->sread(4,&view_list);
		infos = (update_info *)be_app->ro_offs_to_ptr(view_list);
	}

	for (i = 0; i < view_count; i++) {

		result = gDefaultTokens->GetToken(infos->view_token, HANDLER_TOKEN_TYPE,
										  (void **)&view);
		a_session->swrite_l(E_START_DRAW_VIEW);

		if (result == B_OK) {
			update_rect = infos->update_rect;

			#if _CACHE_
				ASSERT(fCurDrawViewState);
				if (view->fPermanentState)
					memcpy(fCurDrawViewState, view->fPermanentState, sizeof(_view_attr_));
				else
					fCurDrawViewState->set_default_values();
				view->fState = fCurDrawViewState;
			#endif

			view->fCachedBounds = infos->current_bound;
			ASSERT(view->Window() == this);
			ASSERT(IsLocked());
			if (infos->flags & ufPostDraw)
				view->DrawAfterChildren(update_rect);
			else
				view->Draw(update_rect);

#ifdef DIM_BACKGROUND_WINDOWS				
			// Hack added by alan:
			if (!IsActive())
			{
				if (fFeel != _PRIVATE_MENU_WINDOW_FEEL_ && fFeel != window_feel(1024))
				{
					view->PushState();
					view->SetDrawingMode(B_OP_ALPHA);
					rgb_color c(ui_color(B_UI_PANEL_BACKGROUND_COLOR));
					c.alpha = 128;
					view->SetHighColor(c);
					view->FillRect(view->Bounds());
					view->PopState();
				}
			}
#endif
			#if _CACHE_
				view->fState = view->fPermanentState;
			#endif

			view->fCachedBounds.top = 1;
			view->fCachedBounds.bottom = -1;
		}
		infos++;
	}
	a_session->swrite_l(E_END_DRAW_WIND);
	a_session->flush();

	fLastViewToken = -1;
	fInUpdate = false;

	if (notShared)
		free(infoBase);

	Unlock();
}

/*-------------------------------------------------------------*/

void	BWindow::Zoom()
{
	BRect		current;
	BScreen		screen(this);
	BRect		sFrame = screen.Frame();
	float		hSlop = 4.0;
	float		vSlop = 5.0;

	BRect zoomto(0,0,0,0);

	if (fLook == B_TITLED_WINDOW_LOOK) {
		hSlop = 8.0;
		vSlop = 9.0;
	}

	// Hardcoded top-left position for current decor
	switch (Look()) {
		case B_DOCUMENT_WINDOW_LOOK:
		case B_TITLED_WINDOW_LOOK:
			zoomto.left = 5;
			zoomto.top = 24;
			break;
		case B_FLOATING_WINDOW_LOOK:
			zoomto.left = 3;
			zoomto.top = 18;
			break;
		case B_MODAL_WINDOW_LOOK:
			zoomto.left = 5;
			zoomto.top = 5;
			break;
		case B_BORDERED_WINDOW_LOOK:
			zoomto.left = 1;
			zoomto.top = 1;
			break;
		case B_NO_BORDER_WINDOW_LOOK:
		default:
			// leave as 0,0
			break;
	}

	current = Frame();

	const uint32 flags = Flags();
	
	if( (flags&B_NOT_H_RESIZABLE) == 0 ) {
		// if horizontally resizable, find new zoom X dimenensions...
		if (fMaxWindH) {
			if ((fMaxZoomH) && (fMaxZoomH < fMaxWindH))
				zoomto.right = zoomto.left + fMaxZoomH;
			else
				zoomto.right = zoomto.left + fMaxWindH;
		} else if (fMaxZoomH) {
			zoomto.right = zoomto.left + fMaxZoomH;
		} else {
			zoomto.right = sFrame.Width();
		}

		// make sure that window isn't wider than screen.
		if (zoomto.right > sFrame.Width() - hSlop)
			zoomto.right = sFrame.Width() - hSlop;
			
	} else {
		// ...otherwise, keep current X dimensions.
		zoomto.left = current.left;
		zoomto.right = current.right;
	}

	if( (flags&B_NOT_V_RESIZABLE) == 0 ) {
		// if vertically resizable, find new zoom Y dimenensions...
		if (fMaxWindV) {
			if ((fMaxZoomV) && (fMaxZoomV < fMaxWindV))
				zoomto.bottom = zoomto.top + fMaxZoomV;
			else
				zoomto.bottom = zoomto.top + fMaxWindV;
		} else if (fMaxZoomV) {
			zoomto.bottom = zoomto.top + fMaxZoomV;
		} else {
			zoomto.bottom = sFrame.Height();
		}
	
		// make sure that window isn't taller than screen.
		if (zoomto.bottom > sFrame.Height() - vSlop)
			zoomto.bottom = sFrame.Height() - vSlop;
			
	} else {
		// ...otherwise, keep current Y dimensions.
		zoomto.top = current.top;
		zoomto.bottom = current.bottom;
	}


	// if the current window size is less than slop pixels from max...
	if ((abs((int)(current.Width() - zoomto.Width())) < 5.0) &&
		(abs((int)(current.Height() - zoomto.Height())) < 5.0)) {
		// ...then revert to old position/size (if there is one)
		if (((int)fFrame.Width()) && ((int)fFrame.Height())) {
			BRect tmp = fFrame;
			// only move the window if we have to
			Zoom(BPoint(tmp.left, tmp.top), tmp.Width(), tmp.Height());
			fFrame = tmp;
		}
	} else {
		// ...size to max
		BRect tmp = current;
		
		if( (flags&B_NOT_H_RESIZABLE) == 0 ) {
			// if horizontally resizable, adjust final zoom X dimensions to
			// fit on screen.
			if (current.left < zoomto.left)
				current.left = zoomto.left;
			if (current.left + zoomto.Width() < (sFrame.Width() - hSlop)) {
				zoomto.right = current.left + zoomto.Width();
				zoomto.left = current.left;
			} else {
				zoomto.left = (sFrame.Width() - hSlop) - zoomto.Width();
				zoomto.right = sFrame.Width() - hSlop;
			}
		}

		if( (flags&B_NOT_V_RESIZABLE) == 0 ) {
			// if vertically resizable, adjust final zoom Y dimensions to
			// fit on screen.
			if (current.top < zoomto.top)
				current.top = zoomto.top;
			if (current.top + zoomto.Height() < (sFrame.Height() - vSlop)) {
				zoomto.bottom = current.top + zoomto.Height();
				zoomto.top = current.top;
			} else {
				zoomto.top = (sFrame.Height() - vSlop) - zoomto.Height();
				zoomto.bottom = sFrame.Height() - vSlop;
			}
		}
		Zoom(BPoint(zoomto.left, zoomto.top), zoomto.Width(), zoomto.Height());
		fFrame = tmp;
	}
}

/*-------------------------------------------------------------*/

void	BWindow::handle_activate(BMessage *an_event)
{
	bool act;
	DEBUG_ONLY(status_t err = )an_event->FindBool("active", &act);
	ASSERT(err == B_OK);
	f_active = act;

//+	PRINT(("Activate(%s, %d)\n", Title() ? Title() : "<<null>>", act));
	//if (f_active && !(fFlags & _MENU_WINDOW_FLAG_)) {
	if ((f_active) && (dynamic_cast<BMenuWindow *>(this) == NULL)) {
		// tell the roster that a window is being activated. Pass
		// the 'application' thread to the roster.
		if (be_roster->UpdateActiveApp(be_app->Team(), BMessenger(this))) {
			// 'assert' the application cursor
//			be_app->SetAppCursor();
		}
	}

	if ((!fOffscreen) && (fFocus != NULL) && (fFocus->f_type & B_INPUT_METHOD_AWARE)) {
		BMessage reply;
		BMessage command((f_active != 0) ? IS_FOCUS_IM_AWARE_VIEW : IS_UNFOCUS_IM_AWARE_VIEW);
		command.AddMessenger(IS_VIEW, BMessenger(fFocus));

		_control_input_server_(&command, &reply);
	}

	WindowActivated(f_active);
	top_view->do_activate(f_active);
	
#ifdef DIM_BACKGROUND_WINDOWS
	UpdateIfNeeded();
#endif
}

/*-------------------------------------------------------------*/

void BWindow::WindowActivated(bool)
{
}		

/*-------------------------------------------------------------*/

void	BWindow::FrameMoved(BPoint)
{
}		

/*-------------------------------------------------------------*/

void	BWindow::WorkspacesChanged(uint32, uint32)
{
}		

/*-------------------------------------------------------------*/

void	BWindow::WorkspaceActivated(int32, bool)
{
}		

/*-------------------------------------------------------------*/

void	BWindow::FrameResized(float, float)		
{
}

/*-------------------------------------------------------------*/

void	BWindow::ScreenChanged(BRect, color_space )
{
/* That needs to be done by the server, or it will be impossible
   to drag window across workspaces */
/*	BRect	windowFrame;
	float	width;
	float	height;

	if (Lock()) {
		width = screen_size.Width();
		height = screen_size.Height();
		windowFrame = Frame();
		if (windowFrame.top > (height - 8))
			height -= 16;
		else height = windowFrame.top;

		if (windowFrame.left > (width - 8))
			width -= 16;
		else
			width = windowFrame.left;

		MoveTo(width, height);
		Unlock();
	}*/
}

/*-------------------------------------------------------------*/

BMessage	*BWindow::extract_drop(BMessage *an_event, BHandler **target)
{
	BPoint 		pt;
	BPoint 		offset;
	BPoint		screen_pt;
	long		flat_size;
	char		*flat_buffer;
	BMessage	*dropped_msg;
	BView		*t = NULL;

	a_session->swrite_l(GET_DROP);
	a_session->flush();
	
	a_session->sread(4, &flat_size);
	flat_buffer = (char *)malloc(flat_size);
	a_session->sread(flat_size, flat_buffer);
	dropped_msg = new BMessage();
	dropped_msg->Unflatten(flat_buffer);
	free((char *)flat_buffer);

	/*
	  Dropping a message is different from SendMessage in that it
	  really a 'different' message in the drop case. The data is
	  compressed and re-hydrated into a new message when the drop
	  occurs. Also, the app_server doesn't know about reply's and
	  BMessengers. For this reason setting the 'reply_to' field is
	  a little clumsier.
	*/
	BMessenger	mm;
	dropped_msg->FindMessenger("_reply_messenger_", &mm);
	_set_message_reply_(dropped_msg, mm);
	dropped_msg->RemoveName("_reply_messenger_");

	an_event->FindPoint("where", &pt);
	an_event->FindPoint("offset", &offset);

	t = FindView(pt);
	
	// I think that the 'where' field should be in screen coords. Otherwise,
	// if the message propagates up the view chain, the value will be
	// in local coords of some child view.
	screen_pt = pt;
	ConvertToScreen(&screen_pt);
	dropped_msg->AddPoint("_drop_point_", screen_pt);
	dropped_msg->AddPoint("_drop_offset_", offset);

	if (t) {
		*target = t;
	} else {
		// if the msg wasn't dropped on any view, then give window
		// a crack at the msg.
		*target = this;
	}

	return dropped_msg;
}

/*-------------------------------------------------------------*/

bool	BWindow::NeedsUpdate() const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	long	result;
	
	if (t->Lock()) {
		a_session->swrite_l(GR_NEED_UPDATE);
		a_session->flush();
		a_session->sread(4, &result);
		t->Unlock();
	}
	else
		return(0);

	return(result);
}

/*-------------------------------------------------------------*/

void	BWindow::UpdateIfNeeded()
{
	if (find_thread(NULL) != Thread()) {
/*
		BMessage msg(_UPDATE_IF_NEEDED_);
		sem_id ack = create_sem(0,"tmpsem");
		msg.AddInt32("ack",(int32)ack);

		int32 lockCount=0;
		while (IsLocked()) {
			Unlock();
			lockCount++;
		};

		while (PostMessage(&msg,this) == B_WOULD_BLOCK) snooze(50000);
		acquire_sem(ack);		

		while (lockCount--) Lock();
*/
		return;
	};

	do_draw_views();
}

/*---------------------------------------------------------------*/

bool BWindow::find_token_and_handler(BMessage *msg, int32 *tok, BHandler **hand)
{
	bool		pref = _use_preferred_target_(msg);
	long		token = _get_message_target_(msg);
	BHandler	*handler = NULL;

	if (pref) {
		handler = PreferredHandler();
		if (!handler)
			handler = this;
	} else {
		// if token == NO_TOKEN then the message is for window
		if (token != NO_TOKEN)
			gDefaultTokens->GetToken(token, HANDLER_TOKEN_TYPE, (void **) &handler);
	}

	*hand = handler;
	*tok = token;

	return (pref);
}

/*---------------------------------------------------------------*/

BMessage * BWindow::ReadMessageFromPort(bigtime_t tout)
{
	if (fEventPort) {
		BMessage *msg=fEventPort->GenerateMessage();
		if (msg) return msg;
	};
	return BLooper::ReadMessageFromPort(tout);
};

int32 BWindow::MessagesWaiting()
{
	int32 pcount = port_count(fMsgPort);
	if (pcount < 0) pcount = 0;
	return pcount+ (fEventPort?fEventPort->Messages():0);
};

void	BWindow::task_looper()
{
	BMessage  	*msg;
	loop_state	loop;

	// The looper is locked before calling this virtual. Let's Unlock here.
	AssertLocked();
	{
		// just the right place to send the client thread id to the server...
		a_session->swrite_l(GR_LOOPER_THREAD);
		a_session->swrite_l(find_thread(NULL));
		a_session->flush();
	}
	Unlock();

	if (IsLocked())
		debugger("shouldn't be locked anymore");

	while(1) {
again:
		loop.reset();
		Lock();
		ReadyToLoop(&loop);
		Unlock();
		
		bigtime_t nextTime = MessageQueue()->NextTime();
		
		if (nextTime > loop.next_loop_time) {
			nextTime = loop.next_loop_time;
		}
		if (nextTime != B_INFINITE_TIMEOUT) {
			bigtime_t sysTime = system_time();
			if (nextTime > sysTime) nextTime -= sysTime;
			else nextTime = 0;
		}
		
		msg = ReadMessageFromPort(nextTime);
		if (msg) {
			MessageQueue()->AddMessage(msg);
		}

		int32 pcount = MessagesWaiting();
		while(pcount > 0) {
			pcount--;
			msg = ReadMessageFromPort(0);
			if (msg) {
				MessageQueue()->AddMessage(msg);
			}
		}

		if (fInUpdate)
			goto again;

		if (MessageQueue()->NextTime() > next_pulse()) {
			// We are past the next pulse time, with no pulse in
			// site.  Send another one to prime the pump.
			BMessenger(this).SendMessage(BMessage(B_PULSE));
		}
		
		while(MessageQueue()->NextTime() <= system_time() && !fInUpdate) {
			msg = MessageQueue()->NextMessage();

			if (msg) {
				BHandler	*handler = NULL;

				fLastMessage = msg;

#if _SUPPORTS_EXCEPTION_HANDLING
				try
#endif
				{
					long token;
					bool main_lock;
					bool pref;

					main_lock = Lock();
					ASSERT(main_lock);

					ASSERT(!fInUpdate);

					SetCurrentBeep("");

					pref = find_token_and_handler(msg, &token, &handler);

#if xDEBUG
//+					if (!(msg->what == B_PULSE || msg->what == B_MOUSE_MOVED))
					if (msg->what == B_MOUSE_MOVED) {
						bigtime_t	when = msg->When();
						PRINT(("Window(%s): %.4s, when=%Ld\n",
							Title() ? Title() : "NULL", &msg->what, when));
					}
#endif

					// if there was a token then check to make sure handler
					// is still valid
					if ((token == NO_TOKEN) || fHandlers.HasItem(handler)) {
						ASSERT(!handler || is_kind_of(handler, BHandler));

						bool skip = false;

						if (msg->what == _MESSAGE_DROPPED_) {
							BHandler *target = NULL;
							BMessage *drop_msg = extract_drop(msg, &target);
							if (drop_msg) {
								fLastMessage = drop_msg;
								delete msg;
								msg = drop_msg;
								handler = target;
								// save the original 'target' of the dropped
								// message
								_set_message_target_(msg,
									_get_object_token_(handler));
							} else
								skip = true;
						}

						if (!skip) {
							/*
							 Now, for some set of messages the target is
							 calculated/dynamic. Handle this calculation here.
							*/
							handler = determine_target(msg, handler, pref);
							if (!handler)
								skip = true;
						}

						if (msg->HasSpecifiers()) {
							int32	s;
							msg->GetCurrentSpecifier(&s, NULL, NULL, NULL);
							if (s >= 0) {
								BHandler *tmp_h;
								tmp_h = handler ? handler : this;
								handler = resolve_specifier(tmp_h, msg);
								if (!handler)
									skip = true;
							}
						}

						if (!skip) {
							handler = top_level_filter(msg, handler);
							if (!handler)
								skip = true;
						}

						if (!skip) {
							ASSERT(handler->Looper() == this);
							DispatchMessage(msg, handler);
						}
					}

					if( *CurrentBeep() != '\0' ) {
						system_beep(CurrentBeep());
						SetCurrentBeep("");
					}
					
					Unlock();

				}
#if _SUPPORTS_EXCEPTION_HANDLING
				catch (...) {
					// unhandled exception...
					const char *title = Title();
					char *buf = (char *) malloc (100 + strlen(title));
					sprintf(buf, "Unexpected error in window \"%s\". Should an attempt be made to continue running?",
						title);
					BAlert *alert = new BAlert("", buf, "Continue", "Close",
						NULL, B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
					long r = alert->Go();
					if (r == 1) {
						fTerminating = true;
						delete this;
					} else {
						// unlock the window
						long c = CountLocks();
						while (c--) {
							Unlock();
						}
					}
				}
#endif
				delete fLastMessage;
				fLastMessage = 0;

			}
			Flush();

			if (MessagesWaiting() > 0) {
//+				if (msg = ReadMessageFromPort())
//+					MessageQueue()->AddMessage(msg);
				goto again;
			}
		}
	}
}

/*-------------------------------------------------------------*/

void BWindow::DequeueAll()
{
	//** This assertion causes problems with Wagner.  Why is it
	//** needed?
	//** ASSERT(find_thread(NULL) == Thread());
	ASSERT(IsLocked());

	// If we're already in an update we can't continue. Else we'd deadlock!
	if (fInUpdate) return;

	BMessage	*msg;
	// read all queued messages. If we're in the middle of an update
	// we have to keep reading until the update is finished!
	while((MessagesWaiting() > 0) || fInUpdate) {
		if ((msg = ReadMessageFromPort()) != 0) {
			if (msg->what == _UPDATE_) {
				do_draw_views();
				delete msg;
			} else if (msg->what == _EVENTS_PENDING_) {
				if (fEventPort) fEventPort->ProcessPending();
				delete msg;
			} else MessageQueue()->AddMessage(msg);
		};
	}
}

/*-------------------------------------------------------------*/

BMessage *BWindow::ConvertToMessage(void */*raw*/, int32 /*code*/)
{
	// No longer used.
	return NULL;
}

/*-------------------------------------------------------------*/

void	BWindow::view_builder(BView *a_view)
{
	int32	received_token;
	BView	*a_child;

	BScrollBar *scroller = dynamic_cast<BScrollBar *>(a_view);

//+	PRINT(("view_builder - window=%s, view=%s\n",
//+		this->Title(), a_view->Name() ? a_view->Name() : "null"));
	if (scroller) {
		float min, max;
		a_session->swrite_l(GR_ADD_SCROLLBAR);
		scroller->GetRange(&min, &max);
		a_session->swrite_l((int32) min);
		a_session->swrite_l((int32) max);
		a_session->swrite_s((short) scroller->Orientation());
	} else {
		a_session->swrite_l(GR_ADD_VIEW);
	}

	// adding a view that wants pulsing, so enabling pulsing.
	if (a_view->f_type & B_PULSE_NEEDED)
		enable_pulsing(true);

	a_session->swrite_rect(&(a_view->fCachedBounds));
	a_session->swrite_l(a_view->f_type);
	a_session->swrite_l(0);
	a_session->swrite_l(_get_object_token_(a_view));

	// now that view belongs invalidate the cached bounds
	// this is used by do_view_draws() and BView::Bounds()
	a_view->fCachedBounds.bottom = -1;
	a_view->fCachedBounds.top = 1;
	a_view->fStateLevel = 0;

	if (a_view->parent == 0)
		a_session->swrite_l(server_token);
	else
		a_session->swrite_l(a_view->parent->server_token);

	Flush();

	a_session->sread(4, &received_token);
	a_view->server_token = received_token;

	a_view->set_cached_state();

	a_child = a_view->first_child;

	while(a_child) {
		if (a_child->server_token == -1)
			view_builder(a_child);
		a_child = a_child->next_sibling;
	}
}


/*-------------------------------------------------------------*/

void	BWindow::attach_builder(BView *a_view)
{
	if (a_view->fState->view_ui_color ||
			a_view->fState->low_ui_color ||
			a_view->fState->high_ui_color) {
		BMessage settings;
		get_ui_settings(&settings);
		a_view->refresh_colors(settings);
	}
	
	a_view->AttachedToWindow();
	a_view->attached = true;

	BView	*a_child = a_view->first_child;
	
	while(a_child) {
		if (!a_child->attached)
			attach_builder(a_child);
		a_child = a_child->next_sibling;
	}

	a_view->AllAttached();
}

/*-------------------------------------------------------------*/

void	BWindow::detach_builder(BView *a_view)
{
	a_view->DetachedFromWindow();
	a_view->attached = false;
	a_view->fStateLevel = 0;
	
	BView	*a_child = a_view->first_child;
	
	while(a_child) {
		detach_builder(a_child);
		a_child = a_child->next_sibling;
	}

	a_view->AllDetached();

	if (a_view == CurrentFocus())
		set_focus(NULL, !a_view->fNoISInteraction);
	if (a_view == fLastMouseMovedView)
		fLastMouseMovedView = NULL;
	if (a_view == fDefaultButton)
		fDefaultButton = NULL;
#if _R5_COMPATIBLE_
	if (a_view == fKeyIntercept)
		fKeyIntercept = NULL;
#endif
	if (fTipInfo && a_view == fTipInfo->fShower)
		fTipInfo->fShower = NULL;
}

/*-------------------------------------------------------------*/

status_t BWindow::AddChild(BWindow *window)
{
#if DEBUG
	if (!IsLocked())
		TRACE();
#endif

	if (window->fParent) {
		debugger("AddChild failed - the window already belonged to another window.\n");
		return B_ERROR;
	}

	status_t result;
	int32 tok;
	if ((result = _safe_get_server_token_(window, &tok)) != B_OK)
		return result;

	result = B_BAD_VALUE;
	if (Lock()) {
		a_session->swrite_l(GR_ADD_WINDOW_CHILD);
		a_session->swrite_l(server_token);
		a_session->swrite_l(tok);
		Flush();
		a_session->sread(4, &result);
		if (result == B_OK) {
			window->fParent = this;
		}
		Unlock();
	}

	return result;
}

BWindow *BWindow::Parent()
{
#if DEBUG
	if (!IsLocked())
		TRACE();
#endif
	return fParent;
}

bool BWindow::RemoveChild(BWindow *window)
{
#if DEBUG
	if (!IsLocked())
		TRACE();
#endif

	if (window->fParent != this) {
		return false;
	}

	bool result = false;
	if (Lock()) {
		result = window->RemoveSelf();
		Unlock();
	}

	return result;
}
 
bool BWindow::RemoveSelf()
{
#if DEBUG
	if (!IsLocked())
		TRACE();
#endif

	if (fParent == NULL) {
		return false;
	}

	long result = B_ERROR;
	if (Lock()) {
		a_session->swrite_l(GR_REMOVE_WINDOW_CHILD);
		a_session->swrite_l(server_token);
		Flush();
		a_session->sread(4, &result);
		if (result == B_OK) {
			fParent = NULL;
		}
		Unlock();
	}

	return (result == B_OK);
}


/*-------------------------------------------------------------*/

void	BWindow::AddChild(BView *child, BView *before)
{
#if DEBUG
//+	if (!IsLocked() && (find_thread(NULL) != Thread()))
//+		TRACE();
#endif
	if (child->RealParent()) {
		debugger("AddChild failed - the view already belonged to someone else.\n");
		return;
	}

	if (Lock()) {
		child->top_level_view = true;
		child->set_owner(this);
		view_builder(child);
		// top_view isn't a 'real' view -- doesn't have server_token
		top_view->AddChild(child, before);
		attach_builder(child);
		Unlock();
	}
}

bool BWindow::RemoveChild(BView *a_view)
{
#if DEBUG
	if (!IsLocked())
		TRACE();
#endif
	bool result = false;
	if (Lock()) {
		result = top_view->RemoveChild(a_view);
		Unlock();
	}

	return result;
}

/*-------------------------------------------------------------*/

int32	BWindow::CountChildren() const
{
	return top_view->CountChildren();
}

/*-------------------------------------------------------------*/

BView*	BWindow::ChildAt(int32 index) const
{
	return top_view->ChildAt(index);
}

/*----------------------------------------------------------------*/

void	BWindow::movesize(uint32 opcode, float h, float v)
{
	a_session->swrite_l(opcode);
	a_session->swrite_l(server_token);
	a_session->swrite_l(floor(h+0.5));
	a_session->swrite_l(floor(v+0.5));
	Flush();
	fFrame = Frame();	/* save new bounds for zoom */
}	

/*----------------------------------------------------------------*/

void	BWindow::MoveBy(float dh, float dv)
{
	if (Lock()) {
		fCurrentFrame.OffsetBy(dh,dv);
		movesize(GR_MOVE_WINDOW, dh, dv);
		Unlock();
	};
}

/*----------------------------------------------------------------*/

void	BWindow::ResizeBy(float dh, float dv)
{
	if (Lock()) {
		fCurrentFrame.right += dh;
		fCurrentFrame.bottom += dv;
		movesize(GR_SIZE_WINDOW, dh, dv);
		Unlock();
	};
}

/*----------------------------------------------------------------*/

void	BWindow::MoveTo(BPoint pt)
{
	if (Lock()) {
		fCurrentFrame.OffsetTo(pt);
		movesize(GR_MOVETO_WINDOW, pt.x, pt.y);
		Unlock();
	};
}

/*----------------------------------------------------------------*/

void	BWindow::MoveTo(float x, float y)
{
	MoveTo(BPoint(x, y));
}

/*----------------------------------------------------------------*/

void	BWindow::ResizeTo(float h, float v)
{
	if (Lock()) {
		fCurrentFrame.right = fCurrentFrame.left + h;
		fCurrentFrame.bottom = fCurrentFrame.top + v;
		movesize(GR_SIZETO_WINDOW, h, v);
		Unlock();
	};
}

/*----------------------------------------------------------------*/

bool	BWindow::IsMinimized() const
{
	bool	min = false;
	BWindow *self = const_cast<BWindow *>(this);
	if (self->Lock()) {
		int32	token = server_token;
		self->Unlock();
		window_info	*winfo = get_window_info(token);
		min = winfo->is_mini;
		free(winfo);
	}
	return min;
}

/*-------------------------------------------------------------*/

void	BWindow::Minimize(bool minimize)
{
	PRINT(("BWindow::Minimize(%d)\n", minimize));
	if (Lock()) {
		if (minimize) {
			system_beep("Window Minimized");
			a_session->swrite_l(GR_MINIMIZE);
		} else {
			system_beep("Window Restored");
			a_session->swrite_l(GR_MAXIMIZE);
		}
		a_session->swrite_l(server_token);
		Flush();
		Unlock();
	}
}

/*-------------------------------------------------------------*/

void	BWindow::Zoom(BPoint rec_pos, float rec_width, float rec_height)
{
	BRect		current;

	current = Frame();
	if ((current.Width() != rec_width) || (current.Height() != rec_height))
		ResizeTo(rec_width, rec_height);
	if ((current.left != rec_pos.x) || (current.top != rec_pos.y))
		MoveTo(rec_pos);
}

/*-------------------------------------------------------------*/

void	BWindow::SetZoomLimits(float max_h, float max_v)
{
	fMaxZoomH = max_h;
	fMaxZoomV = max_v;
}

/*----------------------------------------------------------------*/

void	BWindow::EnableUpdates()
{
	if (Lock()) {
		a_session->swrite_l(GR_UPDATE_ON);
		Flush();
		Unlock();
	}
}

void	BWindow::DisableUpdates()
{
	if (Lock()) {
		a_session->swrite_l(GR_UPDATE_OFF);
		Flush();
		Unlock();
	}
}

/*----------------------------------------------------------------*/

void	BWindow::BeginViewTransaction()
{
	if (Lock()) {
		a_session->swrite_l(GR_OPEN_VIEW_TRANSACTION);
		Unlock();
	};
};

void	BWindow::EndViewTransaction()
{
	if (Lock()) {
		a_session->swrite_l(GR_CLOSE_VIEW_TRANSACTION);
		Unlock();
	};
};

/*----------------------------------------------------------------*/

status_t BWindow::ClipWindowToPicture(BPicture *picture, BPoint offset, uint32 flags)
{
	if (Lock()) {
		a_session->swrite_l(GR_SET_WINDOW_PICTURE);
		a_session->swrite_l(IKAccess::PictureToken(picture));
		a_session->swrite(sizeof(BPoint),&offset);
		a_session->swrite_l(flags);
		a_session->swrite_l(GR_SYNC);
		a_session->flush();
		a_session->sread(4, &flags);
		Unlock();
	};
	return B_OK;
}

/*----------------------------------------------------------------*/

bigtime_t	BWindow::PulseRate() const
{
	return fPulseState ? fPulseState->fRate : DEFAULT_PULSE_RATE;
}

/*----------------------------------------------------------------*/

void	BWindow::SetPulseRate(bigtime_t rate)
{
	if (rate <= 0) {
		rate = 0;
	} else if (rate < 100000) {
		rate = 100000;
	} else {
		rate = (rate / 50000) * 50000;
	}
	
	if (fPulseState == NULL) {
		if (rate == DEFAULT_PULSE_RATE)
			return;
		fPulseState = new win_pulse_state;
	}
	
	fPulseState->fRate = rate;
//+	printf("SetPulseRate (t=%s, r=%Ld, need=%d)\n",
//+		Title(), rate, fPulseState->fNeeded);
	if (rate == 0) {
		enable_pulsing(false);
	} else if (fPulseState->fEnabled) {
		// reset pulsing, so that next pulse occurs at the selected
		// time from now
		enable_pulsing(false);
		enable_pulsing(true);
	} else if (fPulseState->fNeeded) {
		// there are views that currently want pulsing and now that
		// the PulseRate is being set to some > 0 value get
		// things going again
		enable_pulsing(true);
	}
}

/*----------------------------------------------------------------*/

void	BWindow::Hide()
{
	if (Lock()) {
		fShowLevel++;
		if( (fFlags&B_NOT_CLOSABLE) == 0 && fShowLevel == kIsShown+1 ) {
			system_beep("Window Close");
		}
		a_session->swrite_l(GR_HIDE);
		a_session->swrite_l(server_token);
		Flush();
		Unlock();
	}
}

/*----------------------------------------------------------------*/

void	BWindow::Show()
{
	if (!fRunCalled) {
		AssertLocked();

		char		thread_name[B_OS_NAME_LENGTH];
		const char	*name = Title();
		if (!name)
			name = "";

		// Call Run to get the thread going!
		thread_id	task_id = Run();
		
		if (task_id < B_OK)
			return;

		strcpy(thread_name, "w>");
		strncat(thread_name, name, B_OS_NAME_LENGTH - 3);
		rename_thread(task_id, thread_name);
	}

	if (Lock()) {
		fShowLevel--;
		if( (fFlags&B_NOT_CLOSABLE) == 0 && fShowLevel == kIsShown ) {
			system_beep("Window Open");
		}
		if (f_active == 2) {
			f_active = 1;
		}
		a_session->swrite_l(GR_SHOW);
		a_session->swrite_l(server_token);
		Flush();
		Unlock();
	}
	
	return;
}	

/*----------------------------------------------------------------*/

bool	BWindow::IsHidden() const
{
	if (!IsLocked())
		fprintf(stderr,"calling IsHidden without locking the window\n");
	return(fShowLevel > kIsShown);
}

/*----------------------------------------------------------------*/
// Bounds will return the window bound in the local
// coordinate system. The returned values are always in screen pixels

BRect	BWindow::Bounds() const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	BRect r;
	if (t->Lock()) {
		r = fCurrentFrame;
/*		
		a_session->swrite_l(GR_WGET_BOUND);
		Flush();
		a_session->sread_rect(&r);
*/		
		t->Unlock();
	}
	r.OffsetTo(BPoint(0, 0));
	return r;
}

/*----------------------------------------------------------------*/
// Frame will return the position of the window in the global
// coordinate system. The returned values are always in screen pixels

BRect	BWindow::Frame() const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	BRect r;
	if (t->Lock()) {
		r = fCurrentFrame;
/*		
		a_session->swrite_l(GR_WGET_BOX);
		Flush();
		a_session->sread_rect(&r);
*/		
		t->Unlock();
	}
	return r;
}

/*----------------------------------------------------------------*/

const char*BWindow::Title() const
{
	return fTitle;
}

/*----------------------------------------------------------------*/

void	BWindow::SetLocalTitle(const char *new_title)
{
	if (fTitle) {
		free(fTitle);
		fTitle = NULL;
	}

	if (new_title) {
		fTitle = strdup(new_title);
	}
	SetName(new_title);
}

/*----------------------------------------------------------------*/

void	BWindow::SetTitle(const char* new_title)
{
	long		len;
	char		thread_name[B_OS_NAME_LENGTH];
	const char	*fake_title;

	/*
	 Need a little 'smarts' here because we want to allow NULL names
	 at the API level. However, the app_server doesn't support that
	 so we need to fake it out.
	*/

	fake_title = new_title ? new_title : "";
	
	if (Lock()) {
		strcpy(thread_name, "w>");
		strncat(thread_name, fake_title, B_OS_NAME_LENGTH - 3);
		rename_thread(Thread(), thread_name);
		a_session->swrite_l(GR_WSET_TITLE);
		len = strlen(fake_title);
		a_session->swrite_l(len);
		a_session->swrite(len + 1, (void *) fake_title);
		a_session->flush();
		

		// now update the local name. NULL is legal name in this case.
		SetLocalTitle(new_title);

		Unlock();
	}
}

/*----------------------------------------------------------------*/
// IsFront will return true if the window is the true front window
// Being the true front meaning that there are no window in front
// of it even some created by other sessions.
// Only floating windows can be in front and not be seen.

bool	BWindow::IsFront() const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	char	result;

	if (t->Lock()) {
		a_session->swrite_l(GR_WIS_FRONT);
		Flush();
		a_session->sread(1, &result);
		t->Unlock();
		return(result);
	} else
		return(false);
}

/*----------------------------------------------------------------*/

int32	BWindow::get_server_token() const
{
	return(server_token);
}

/*----------------------------------------------------------------*/

status_t BWindow::SetWindowAlignment(window_alignment mode, int32 h,
	int32 hOffset, int32 width, int32 widthOffset, int32 v, int32 vOffset,
	int32 height, int32 heightOffset)
{
	int32		result;
	
	if (Lock()) {
		a_session->swrite_l(GR_SET_WINDOW_ALIGNMENT);
		a_session->swrite_l(server_token);
		a_session->swrite_l(mode);
		a_session->swrite_l(h);
		a_session->swrite_l(hOffset);
		a_session->swrite_l(width);
		a_session->swrite_l(widthOffset);
		a_session->swrite_l(v);
		a_session->swrite_l(vOffset);
		a_session->swrite_l(height);
		a_session->swrite_l(heightOffset);
		Flush();
		a_session->sread(4, &result);
		Unlock();
		return(result);
	} else
		return(B_ERROR);
}

/*----------------------------------------------------------------*/

status_t
BWindow::GetWindowAlignment(
	window_alignment	*mode,
	int32				*h, 
	int32				*hOffset, 
	int32				*width, 
	int32				*widthOffset, 
	int32				*v,
	int32				*vOffset, 
	int32				*height, 
	int32				*heightOffset) const
{
	BWindow *self = const_cast<BWindow *>(this);	// to get non-const "this" pointer		

	if (self->Lock()) {
		window_alignment	theMode = B_BYTE_ALIGNMENT;
		int32				theH = 0;
		int32				theHOffset = 0;
		int32 				theWidth = 0;
		int32				theWidthOffset = 0;
		int32				theV = 0;
		int32				theVOffset = 0;
		int32				theHeight = 0;
		int32				theHeightOffset = 0;
		status_t			result = B_ERROR;

		a_session->swrite_l(GR_GET_WINDOW_ALIGNMENT);
		a_session->swrite_l(server_token);
		Flush();

		a_session->sread(4, &theMode);
		if (mode != NULL)		
			*mode = theMode;

		a_session->sread(4, &theH);
		if (h != NULL)
			*h = theH;

		a_session->sread(4, &theHOffset);
		if (hOffset != NULL)
			*hOffset = theHOffset;

		a_session->sread(4, &theWidth);
		if (width != NULL)
			*width = theWidth;

		a_session->sread(4, &theWidthOffset);
		if (widthOffset != NULL)
			*widthOffset = theWidthOffset;

		a_session->sread(4, &theV);
		if (v != NULL)
			*v = theV;

		a_session->sread(4, &theVOffset);
		if (vOffset != NULL)
			*vOffset = theVOffset;

		a_session->sread(4, &theHeight);
		if (height != NULL)
			*height = theHeight;

		a_session->sread(4, &theHeightOffset);
		if (heightOffset != NULL)
			*heightOffset = theHeightOffset;

		a_session->sread(4, &result);

		self->Unlock();
		return (result);
	}

	return (B_ERROR);
}

/*----------------------------------------------------------------*/

bool	BWindow::IsActive() const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	char	result;

	if (t->Lock()) {
		a_session->swrite_l(GR_IS_ACTIVE);
		Flush();
		a_session->sread(1, &result);
		t->f_active = result;
		t->Unlock();
		return(result);
	} else
		return(false);
}

/*----------------------------------------------------------------*/

void	BWindow::SetWindowColor(rgb_color color)
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	if (t->Lock()) {
		if (fWindowColor != color) {
			fWindowColor = color;
			BRect r(Bounds());
			fLastViewToken = top_view_token;
			a_session->swrite_l(GR_PICK_VIEW);
			a_session->swrite_l(top_view_token);
			a_session->swrite_l(GR_SET_VIEW_COLOR);
			a_session->swrite(sizeof(rgb_color), &color);
			a_session->swrite_l(GR_INVAL);
			a_session->swrite_rect(&r);
			Flush();
		}
		t->Unlock();
	}
}

rgb_color BWindow::WindowColor() const
{
	rgb_color col;
	
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	if (t->Lock()) {
		col = fWindowColor;
		t->Unlock();
	} else {
		col = make_color(255, 255, 255);
	}
	
	return col;
}

/*----------------------------------------------------------------*/

void	BWindow::Flush() const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	if (t->Lock()) {	
		a_session->flush();
		t->Unlock();
	}
}

/*----------------------------------------------------------------*/

void	BWindow::Sync() const
{
	BWindow *self = const_cast<BWindow *>(this);

	if (self->Lock()) {
		a_session->full_sync();
		self->Unlock();
	}
}

/*----------------------------------------------------------------*/

void	BWindow::SetCurrentBeep(const char* name)
{
	fEventBeep = name;
}

void	BWindow::set_async_beep(const char* name)
{
	if( fFlags&B_ASYNCHRONOUS_CONTROLS ) SetCurrentBeep(name);
	else system_beep(name);
}

/*----------------------------------------------------------------*/

const char*	BWindow::CurrentBeep() const
{
	return fEventBeep.String();
}

/*----------------------------------------------------------------*/

BView	*BWindow::FindView(const char *name) const
{
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer
	// pjp
	// Need to explicitly Lock and Unlock here.
	if (t->Lock()) {
		BView *result = top_view->FindView(name);
		t->Unlock();
		return(result);
	}
	else
		return(NULL);	
}

/*----------------------------------------------------------------*/

BView	*BWindow::FindView(BPoint loc) const
{
	long		view_client_token;
	BView		*the_view;
	status_t	result;
	BWindow *t = (BWindow *) this;	// to get non-const "this" pointer

	if (t->Lock()) {	
		a_session->swrite_l(GR_FIND_VIEW);
		a_session->swrite_coo(&loc.x);
		a_session->swrite_coo(&loc.y);
		a_session->flush();

		a_session->sread(4, &view_client_token);
	
		result = gDefaultTokens->GetToken(view_client_token, HANDLER_TOKEN_TYPE,
										  (void **)&the_view);
								  
		t->Unlock();

		if (result >= B_OK)
			return(the_view);
		else
			return(NULL);
	} else
		return(NULL);
}

/*---------------------------------------------------------------*/

void	BWindow::set_focus(BView *f, bool notify_input_server)
{	
	if ((notify_input_server) && (!fOffscreen) && (f_active == 1) && (f != fFocus)) {
		if ((f != NULL) && (f->f_type & B_INPUT_METHOD_AWARE)) {
			BMessage reply;
			BMessage command(IS_FOCUS_IM_AWARE_VIEW);
			command.AddMessenger(IS_VIEW, BMessenger(f));

			_control_input_server_(&command, &reply);
		}
		else {
			if ((fFocus != NULL) && (fFocus->f_type & B_INPUT_METHOD_AWARE)) {
				BMessage reply;
				BMessage command(IS_UNFOCUS_IM_AWARE_VIEW);
				command.AddMessenger(IS_VIEW, BMessenger(fFocus));
	
				_control_input_server_(&command, &reply);
			}
		}
	}

	// ** dependency here with BLooper::SetPreferredHandler() **
	// set fFocus first, as BLooper::SetPreferredHandler() will
	// ALWAYS assign a BWindow's preferred handler to be its focus
	// (regardless of what was passed into it)
	fFocus = f;
	SetPreferredHandler(f);
}

/*---------------------------------------------------------------*/

BView	*BWindow::CurrentFocus() const
{
	return fFocus;
}

/*---------------------------------------------------------------*/

void	BWindow::ClearExplicitFocus()
{
	if (fFocus)
		fFocus->SetExplicitFocus(false);
}

/*----------------------------------------------------------------*/

void	BWindow::start_drag(BMessage *msg, int32 token, BPoint offset,
							int32 bitmap_token, drawing_mode dragMode, BHandler *reply_to)
{
	if (!msg) return;

	be_app->send_drag(msg, token, offset, bitmap_token, dragMode, BMessenger(reply_to));
}

/*---------------------------------------------------------------*/

void	BWindow::start_drag(BMessage *msg, int32 token, BPoint offset,
	BRect track_rect, BHandler *reply_to)
{
	if (!msg) return;

	be_app->send_drag(msg, token, offset, track_rect, BMessenger(reply_to));
}

/*---------------------------------------------------------------*/

_cmd_key_ *BWindow::allocShortcut(uint32 key, uint32 mods)
{
	_cmd_key_	*a;

	RemoveShortcut(key, mods);
	a = (_cmd_key_ *)calloc(1, sizeof(_cmd_key_));
	
	if(!a) {
		debugger("Error allocating _cmd_key_.\n");
		return NULL;
	}
	
	a->key = key;
	a->mod = mods | B_COMMAND_KEY;	// add B_COMMAND_KEY to ensure it is set

	a->targetFocused = false;
	a->scut_target = NULL;

	a->item = NULL;
	a->msg = NULL;

	return a;
}

/*---------------------------------------------------------------*/

void BWindow::AddShortcut(uint32 key, uint32 mods, BMenuItem *item)
{
	_cmd_key_	*a = allocShortcut(key, mods);
	a->item = item;
	accelList.AddItem(a);
}

/*---------------------------------------------------------------*/

void BWindow::AddShortcut(uint32 key, uint32 mods, BMessage *msg)
{
	_cmd_key_	*a = FindShortcut(key, mods);
	if (a && a->msg == msg)
		return;

	a = allocShortcut(key, mods);
	a->msg = msg;
	a->scut_target = this;
	accelList.AddItem(a);
}

/*---------------------------------------------------------------*/

void BWindow::AddShortcut(uint32 key, uint32 mods, BMessage *msg,
	BHandler *target)
{
	_cmd_key_	*a = FindShortcut(key, mods);
	if (a && a->msg == msg)
		return;

	a = allocShortcut(key, mods);
	a->msg = msg;
	a->scut_target = target;
	a->targetFocused = (target == NULL);
	accelList.AddItem(a);
}

/*---------------------------------------------------------------*/

_cmd_key_ *BWindow::FindShortcut(uint32 key, uint32 mods)
{
	_cmd_key_	*a;
	long		i;

	for(i=0; ; i++) {
		a = (_cmd_key_ *)accelList.ItemAt(i);
		if(!a)
			break;

		if ((tolower(a->key) == tolower(key)) && a->mod == mods) {
			return a;
		}
	}
	return NULL;
}

/*---------------------------------------------------------------*/

void BWindow::RemoveShortcut(uint32 key, ulong mods)
{
	_cmd_key_	*a;
	long		i;

	if ((key == 'q' || key == 'Q') && mods == B_COMMAND_KEY) {
		// The Command-Q shortcut is special because it needs to go
		// to the application, which the normal shortcut mechanism
		// is stupidly unable to do.  So we kind-of make it look like
		// a normal shortcut here.
		fNoQuitShortcut = true;
	} else {
		for(i=0; ; i++) {
			a = (_cmd_key_ *)accelList.ItemAt(i);
			if(!a)
				break;
	
			if ((tolower(a->key) == tolower(key)) && a->mod == mods) {
				accelList.RemoveItem(i);
				if (a->msg)
					delete a->msg;
				free(a);
				return;
			}
		}
	}
}

/*---------------------------------------------------------------*/

BButton *BWindow::DefaultButton() const
{
	return fDefaultButton;
}

/*---------------------------------------------------------------*/

void BWindow::SetDefaultButton(BButton *button)
{
	/*
	 You might notice that this method and it's cousin, BButton::MakeDefault
	 are rather complicated. We're just setting up a default button, why
	 does it have to be so complicated? Well, we wanted to be able to
	 set up the default button by calling a method on either the Window
	 or the Button. We didn't want to be Button centric or Window centric.
	 To support that notion or to have these methods also be virtual meant
	 that they are complicated.
	*/
	
	BButton *prev = fDefaultButton;

	if (prev == button)
		return;

	fDefaultButton = button;

	if (prev) {
		// if the button still thinks it is the default the un-default it
		if (prev->IsDefault())
			prev->MakeDefault(false);
		prev->Invalidate();
	}
	if (fDefaultButton) {
		// if the new default button doesn't know it is the default then...
		if (!fDefaultButton->IsDefault())
			fDefaultButton->MakeDefault(true);
		fDefaultButton->Invalidate();
	}
}

/*---------------------------------------------------------------*/

void BWindow::MenusBeginning()
{
}

/*---------------------------------------------------------------*/

void BWindow::MenusEnded()
{
//+	PRINT(("DoneShowing\n"));
}

/*---------------------------------------------------------------*/

BHandler *BWindow::determine_target(BMessage *msg, BHandler *target, bool pref)
{
	// if a handler was specified then use it
	if ((target) && (!pref))
		return target;

	switch (msg->what) {
		case B_PULSE:
			target = this;
			break;
		case B_MOUSE_MOVED:
			status_t	error;
			long		token;

			error = msg->FindInt32("_view_token_", &token);
			if (!error) {
//printf("token %ld\n", token);
				target = NULL;
				error = gDefaultTokens->GetToken(token, HANDLER_TOKEN_TYPE,
												 (void **)&target);
				if (error < B_OK) 
					target = NULL;
				else {
					// ensure that the view still belongs to this window
					// ??? race condition here. If it doesn't belong then
					// it could get deleted at any moment
					if (target && (target->Looper() != this))
						target = NULL;
				}
			}

			break;

		case B_KEY_UP:
			{
			// default behavior -> send KeyDown event to focused view
			target = CurrentFocus();
			
			uchar ch = 0;
			if (msg->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR) {
				// default buttons override the target_view view.
				if ( fDefaultButton && (ch == B_ENTER) && 
					 fDefaultButton->IsEnabled() ) {
					target = fDefaultButton;
				}
			}
			break;
			}
			
		case B_KEY_DOWN:
			{
			// default behavior -> send KeyDown event to focused view
			target = CurrentFocus();
			
			uchar ch = 0;
			if (msg->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR) {
				// default buttons override the target_view view.
				if ( fDefaultButton && (ch == B_ENTER) && 
					 fDefaultButton->IsEnabled() ) {
					// skip auto-repeating enter keys
					int32	repeats = msg->FindInt32("be:key_repeat");
					if (repeats != 0)
						return NULL;
					target = fDefaultButton;
				}
			}
			break;
			}
	}

	if (!target)
		target = this;

//+	PRINT(("what=%.4s, target = %s (%s), (filters=%x)\n",
//+		(char *) &(msg->what), target->Name() ? target->Name() : "",
//+		class_name(target), target->FilterList()));
	return target;
}

/*---------------------------------------------------------------*/

void	BWindow::DispatchMessage(BMessage *an_event, BHandler *handler)
{
	
	ASSERT(an_event);
	ASSERT(handler);

//+	if ((an_event->what != B_PULSE) && (an_event->what != B_MOUSE_MOVED))
//+		PRINT(("what=%.4s, target = %s (%s), (filters=%x)\n",
//+			(char *) &(an_event->what), handler->Name() ? handler->Name() : "",
//+			class_name(handler), handler->FilterList()));

	if (!an_event->IsSystem()) {
		handler->MessageReceived(an_event);
	} else {
		switch(an_event->what) {

			case B_PULSE :
//+				PRINT(("Pulse (%s)\n", Title()));
//+				atomic_add(&pulse_queued, -1);
				if (handler == this) {
					if (f_active != 2) {
						if (!fInUpdate) {
//+							PRINT(("handle_tick (%s)\n", Title()));
							top_view->handle_tick();
							Flush();
						}
					}
					
					// If we are passed our pulse time, bump to next and send
					// a message for it.
					const bigtime_t pulseTime = next_pulse();
					const bigtime_t sysTime = system_time();
					if (pulseTime <= sysTime) {
						// Bump to next time.
						fPulseState->fNext += fPulseState->fRate;
						// If next time is before now, jump to right now.
						if (fPulseState->fNext < sysTime)
							fPulseState->fNext = sysTime;
						BMessenger(this).SendMessageAtTime(BMessage(B_PULSE),
														   fPulseState->fNext);
					}
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_UI_SETTINGS_CHANGED:
				if (handler == this) {
					BMessage upd;
					status_t result = an_event->FindMessage("be:settings", &upd);
					if (result == B_OK && !upd.IsEmpty()) {
						DisableUpdates();
						UISettingsChanged(&upd, 0);
						top_view->handle_ui_settings(&upd, 0);
						EnableUpdates();
					}
				} else {
					handler->MessageReceived(an_event);
				}
				break;
		
			case B_MOUSE_UP :
				{
				KillTip();
				set_async_beep("Mouse Up");
				BView *v = dynamic_cast<BView *>(handler);
				if (v) {
					BPoint loc;
					an_event->FindPoint("be:view_where", &loc);
					v->MouseUp(loc);
				} else {
					handler->MessageReceived(an_event);
				}

				break;
				}

			case B_MOUSE_DOWN :
				{
				fWaitingForMenu = false;
				ASSERT(f_active != 2);
				
				KillTip();
				set_async_beep("Mouse Down");
				BView *v = dynamic_cast<BView *>(handler);
				if (v) {
					if (v != CurrentFocus())
						ClearExplicitFocus();
					do_mouse_down(an_event, v);
				} else {
					handler->MessageReceived(an_event);
				}

				break;
				}

			case B_MOUSE_MOVED :
				if (BView *v = cast_as(handler, BView)) {
					do_mouse_moved(an_event, v);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_REQUEST_TOOL_INFO:
				SendToolTipInfo();
				break;

			case B_KEY_UP :	
				KillTip();
				set_async_beep("Key Up");
				do_key_up(an_event, handler);
				break;

			case B_KEY_DOWN :	
				KillTip();
				set_async_beep("Key Down");
				fWaitingForMenu = false;
				ASSERT(f_active != 2);
				do_key_down(an_event, handler);
				break;

			case B_INVALIDATE :
				if (BView *v = cast_as(handler, BView)) {
					BRect area;
					if (an_event->FindRect("be:area", &area) == B_OK) {
						v->Invalidate(area);
					} else {
						v->Invalidate();
					}
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_VALUE_CHANGED :	
				{
				do_value_change(an_event, handler);
				break;
				}

			case B_WINDOW_ACTIVATED :
				// Do not perform activation event for menus.
				HideTip();
				if( fFeel != _PRIVATE_MENU_WINDOW_FEEL_ ) {
					bool act;
					if( an_event->FindBool("active", &act) != B_OK ) act = false;
					if( act ) set_async_beep("Window Activated");
				}
				if (handler == this) {
					fWaitingForMenu = false;
					handle_activate(an_event);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_WINDOW_MOVED :	
				HideTip();
				if (handler == this) {
					BPoint pt;
					an_event->FindPoint("where", &pt);
					fCurrentFrame.OffsetTo(pt);
					FrameMoved(pt);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_WINDOW_RESIZED :
				HideTip();
				if (handler == this) {
					long h, w;
					an_event->FindInt32("width", &w);
					an_event->FindInt32("height", &h);
					fCurrentFrame.right = fCurrentFrame.left + w;
					fCurrentFrame.bottom = fCurrentFrame.top + h;
					FrameResized(w, h);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_MINIMIZE :
				bool b;
				if( an_event->FindBool("minimize", &b) != B_OK ) b = true;
				if (handler == this) {
					Minimize(b);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_ZOOM:
				HideTip();
				set_async_beep("Window Zoomed");
				if (handler == this) {
					Zoom();
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_SCREEN_CHANGED :
				HideTip();
				if (handler == this) {
					BRect r;
					color_space mode;
					an_event->FindRect("frame", &r);
				  	an_event->FindInt32("mode", (long*) &mode);
					ScreenChanged(r, mode);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case _MENUS_DONE_ :
				MenusEnded();
				break;

			case _MENU_EVENT_ :
				// private message
				ASSERT(f_active != 2);
				do_menu_event(an_event);
				break;

			case _EVENTS_PENDING_ :
				if (fEventPort) fEventPort->ProcessPending();
				break;

			case _UPDATE_:
				do_draw_views();
				break;

			case _UPDATE_IF_NEEDED_:
				do_draw_views();
				delete_sem(an_event->FindInt32("ack"));
				break;

			case _QUIT_ :
				// private message
				fTerminating = true;
				delete this;		// doesn't return;
				TRESPASS();
				break;

			case B_VIEW_RESIZED :
				{
				BView *v = dynamic_cast<BView *>(handler);
				if (v) {
					long h, w;
					an_event->FindInt32("width", &w);
					an_event->FindInt32("height", &h);
					v->FrameResized(w, h);
				} else {
					handler->MessageReceived(an_event);
				}
				break;
				}

			case B_VIEW_MOVED :
				{
				BView *v = dynamic_cast<BView *>(handler);
				if (v) {
					BPoint pt;
					an_event->FindPoint("where", &pt);
					v->FrameMoved(pt);
				} else {
					handler->MessageReceived(an_event);
				}
				break;
				}

			case B_WORKSPACES_CHANGED :
				HideTip();
				if (handler == this) {
					long o, n;
					an_event->FindInt32("old", &o);
					an_event->FindInt32("new", &n);
					WorkspacesChanged(o, n);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_WORKSPACE_ACTIVATED :
				HideTip();
				if (handler == this) {
					long space;
					bool act;
					an_event->FindInt32("workspace", &space);
					an_event->FindBool("active", &act);
					WorkspaceActivated(space, act);
				} else {
					handler->MessageReceived(an_event);
				}
				break;

			case B_MOUSE_WHEEL_CHANGED :
				KillTip();
				inherited::DispatchMessage(an_event, handler);
				break;
				
			default:
				inherited::DispatchMessage(an_event, handler);
				break;

		}
	}
}

/*------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_SCRIPTING
enum {
	PI_BASIC,
	PI_VIEW_WILDCARD,
	PI_MENU_BAR
};

	/*
	 A generic BWindow supports the following:
	 	GET/SET		"Frame"			DIRECT form only
//+	 	EXECUTE		"MoveTo"		DIRECT form only
//+	 	EXECUTE		"MoveBy"		DIRECT form only
	 	GET/SET		"Title"			DIRECT form only
	 	GET/SET		"Workspaces"	DIRECT form only
	 	GET/SET		"Look"			DIRECT form only
	 	GET/SET		"Feel"			DIRECT form only
	 	GET/SET		"Flags"			DIRECT form only
	 	GET/SET		"Hidden"		DIRECT form only
	 	GET/SET		"Active"		DIRECT form only
	 	GET/SET		"Minimize"		DIRECT form only
	 	---			"View"			INDEX, NAME	(actually passed on to top_view)
	 	---			"MenuBar"		pass to KeyMenuBar
	*/

static value_info	value_list[] = {
	{	"MoveTo", 'WDMT', B_COMMAND_KIND,
			"'data' is a BPoint with the new location", 0, {} },
	{	"MoveBy", 'WDMB', B_COMMAND_KIND,
			"'data' is a BPoint with the x and y offsets", 0, {} },
	{	"ResizeTo", 'WDRT', B_COMMAND_KIND,
			"'data' is a BPoint with the new x and y sizes", 0, {} },
	{	"ResizeBy", 'WDRB', B_COMMAND_KIND,
			"'data' is a BPoint with the x and y resize delta", 0, {} },
	{0, 0, B_COMMAND_KIND, NULL, 0, {} }
};

static property_info	prop_list[] = {
	{"Frame",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_RECT_TYPE },
		{},
		{}
	},
	{"Title",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_STRING_TYPE },
		{},
		{}
	},
	{"Workspaces",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"View",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_INT32_TYPE },
		{},
		{}	
	},
	{"Look",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Feel",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Flags",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Hidden",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"Active",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"Minimize",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_BASIC,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"View",
		{},			// allows any command
		{},			// allows any specifier form
		0, PI_VIEW_WILDCARD,
		{},
		{},
		{}
	},
	{"MenuBar",		// pass commands to the KeyMenuBar
		{},
		{B_DIRECT_SPECIFIER},
		0, PI_MENU_BAR,
		{},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

/*------------------------------------------------------------*/

BHandler *BWindow::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BHandler	*target = NULL;

//+	PRINT(("BWindow::Resolve: msg->what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, prop));
//+	PRINT(("	class = %s\n", class_name(this)));

	BPropertyInfo	pi(prop_list);
	int32			i;
	uint32			user_data;
	status_t		err = B_OK;
	BMessage		error_msg(B_MESSAGE_NOT_UNDERSTOOD);
	
	if (strcmp(prop, "View") == 0) {
		// don't POP the specifier. Let the top_view handle this
		// same specifier!
//+		PRINT(("passing on to top view\n"));
		target = top_view;
	} else if ((i = pi.FindMatch(msg, index, spec, form, prop)) >= 0) {
		user_data = prop_list[i].extra_data;
		switch (user_data) {
			case PI_BASIC:
//+				PRINT(("passing to self\n"));
				target = this;
				break;
			case PI_VIEW_WILDCARD:
				// don't POP the specifier. Let the top_view handle this
				// same specifier!
//+				PRINT(("passing on to top view\n"));
				target = top_view;
				break;
			case PI_MENU_BAR: {
				BMenuBar	*mb = KeyMenuBar();
				if (mb) {
					msg->PopSpecifier();
					target = mb;
				} else {
					err = B_NAME_NOT_FOUND;
					error_msg.AddString("message",
						"This window doesn't have a main MenuBar");
				}
				break;
			}
			default:
				ASSERT_WITH_MESSAGE(0, "Someone changed prop_list");
				break;
		}
	}

	if (err) {
		error_msg.AddInt32("error", err);
		msg->SendReply(&error_msg);
	} else if (!target) {
		target = inherited::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------*/

void BWindow::MessageReceived(BMessage *msg)
{
//+	PRINT(("what = %.4s\n", (char *) &(msg->what)));
	bool 		handled = false;
	BMessage	reply(B_REPLY);
	status_t	err;

	switch (msg->what) {
		case B_KEY_DOWN:
			kb_navigate();
			handled = true;
			break;
#if _SUPPORTS_FEATURE_SCRIPTING
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage	specifier;
			int32		form;
			const char	*prop;
			int32		cur;
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (err)
				break;
			if ((strcmp(prop, "Frame") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddRect("result", Frame());
					handled = true;
				} else {
					BRect rect;
					err = msg->FindRect("data", &rect);
					if (err == B_OK) {
						MoveTo(rect.LeftTop());
						ResizeTo(rect.Width(), rect.Height());
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Title") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					if (Title())
						reply.AddString("result", Title());
					else {
						err = B_NAME_NOT_FOUND;
						reply.AddInt32("error", err);
					}
					handled = true;
				} else {
					const char *title;
					err = msg->FindString("data", &title);
					if (err == B_OK) {
						SetTitle(title);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Workspaces") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddInt32("result", Workspaces());
					handled = true;
				} else {
					uint32	ws;
					err = msg->FindInt32("data", (int32 *) &ws);
					if (err == B_OK) {
						SetWorkspaces(ws);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Look") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddInt32("result", Look());
					handled = true;
				} else {
					window_look	wl;
					err = msg->FindInt32("data", (int32 *) &wl);
					if (err == B_OK) {
						err = SetLook(wl);
						reply.AddInt32("error", err);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Feel") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddInt32("result", Feel());
					handled = true;
				} else {
					window_feel feel;
					err = msg->FindInt32("data", (int32 *) &feel);
					if (err == B_OK) {
						err = SetFeel(feel);
						reply.AddInt32("error", err);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Flags") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddInt32("result", Flags());
					handled = true;
				} else {
					uint32 flags;
					err = msg->FindInt32("data", (int32 *) &flags);
					if (err == B_OK) {
						err = SetFlags(flags);
						reply.AddInt32("error", err);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Hidden") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddBool("result", IsHidden());
					handled = true;
				} else {
					bool	hide;
					err = msg->FindBool("data", &hide);
					if (err == B_OK) {
						// Don't allow nesting of calls to Hide/Show
						// to simplify the scripting model.
						if (hide) {
							if (!IsHidden())
								Hide();
						} else {
							if (IsHidden())
								Show();
						}
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Active") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddBool("result", IsActive());
					handled = true;
				} else {
					bool activate;
					err = msg->FindBool("data", &activate);
					if (err == B_OK) {
						if (activate != IsActive())
							Activate(activate);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if ((strcmp(prop, "Minimize") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddBool("result", IsMinimized());
					handled = true;
				} else {
					bool min;
					err = msg->FindBool("data", &min);
					if (err == B_OK) {
						Minimize(min);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
			break;
		}
#endif
		// what are the following two?
		// we should get rid of them and have corresponding scripting
		// calls instead
		case 'WDMB':
		{
			BPoint		pt;
			err = msg->FindPoint("data", &pt);
			if (err == B_OK) {
				MoveBy(pt.x, pt.y);
				handled = true;
			}
			break;
		}
		case 'WDMT':
		{
			BPoint		pt;
			err = msg->FindPoint("data", &pt);
			if (err == B_OK) {
				MoveTo(pt);
				handled = true;
			}
			break;
		}
		case 'WDRB':
		{
			BPoint		pt;
			err = msg->FindPoint("data", &pt);
			if (err == B_OK) {
				ResizeBy(pt.x, pt.y);
				handled = true;
			}
			break;
		}
		case 'WDRT':
		{
			BPoint		pt;
			err = msg->FindPoint("data", &pt);
			if (err == B_OK) {
				ResizeTo(pt.x, pt.y);
				handled = true;
			}
			break;
		}
	}

	if (handled)
		msg->SendReply(&reply);
	else
		inherited::MessageReceived(msg);
}

/*-------------------------------------------------------------*/

status_t	BWindow::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-window");
	BPropertyInfo	pi(prop_list, value_list);
	data->AddFlat("messages", &pi);
	return inherited::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*-------------------------------------------------------------*/

void	BWindow::do_menu_event(BMessage *an_event)
{
	ulong 		buttons;
	long		altState;
	bool		hasAlt;

	an_event->FindInt32("buttons", (long*) &buttons);
	if (buttons)
		return;

	/*
	 field			comment
	 -------		-------------------------------------------------------
	 buttons		state of the mouse buttons
	 alt			if field exists then alt key is going down (1) or up (0)
	 menu			Microsoft's new Menu Key.
	*/

	hasAlt = an_event->HasInt32("alt");
	an_event->FindInt32("alt", &altState);

	if (altState) {		// Alt key down
		return;
	}

	long menu;
	an_event->FindInt32("menu", &menu);
	if (menu || fWaitingForMenu) {
		fWaitingForMenu = false;
		if (fKeyMenuBar) {
			fKeyMenuBar->StartMenuBar(0);
		}
	}
}

/*-------------------------------------------------------------*/

void	BWindow::do_value_change(BMessage *an_event, BHandler *handler)
{
	long	new_v;

	ASSERT(handler);
	BScrollBar *sbar = dynamic_cast<BScrollBar *>(handler);

	if (sbar != 0) {	
		an_event->FindInt32("value", &new_v);

		if (sbar->fValue != new_v) {
			sbar->fValue = new_v;
			sbar->ValueChanged(new_v);
			Flush();
		}
	} else {
		handler->MessageReceived(an_event);
	}
}

/*---------------------------------------------------------------*/

BPoint BWindow::ConvertToScreen(BPoint pt) const
{
	return top_view->ConvertToScreen(pt);
}

/*---------------------------------------------------------------*/

void	BWindow::ConvertToScreen(BPoint *pt) const
{
	top_view->ConvertToScreen(pt);
}

/*---------------------------------------------------------------*/

BPoint	BWindow::ConvertFromScreen(BPoint pt) const
{
	return top_view->ConvertFromScreen(pt);
}

/*---------------------------------------------------------------*/

void	BWindow::ConvertFromScreen(BPoint *pt) const
{
	top_view->ConvertFromScreen(pt);
}

/*---------------------------------------------------------------*/

BRect	BWindow::ConvertToScreen(BRect r) const
{
	return top_view->ConvertToScreen(r);
}

/*---------------------------------------------------------------*/

void	BWindow::ConvertToScreen(BRect *r) const
{
	top_view->ConvertToScreen(r);
}

/*---------------------------------------------------------------*/

BRect	BWindow::ConvertFromScreen(BRect r) const
{
	return top_view->ConvertFromScreen(r);
}

/*---------------------------------------------------------------*/

void	BWindow::ConvertFromScreen(BRect *r) const
{
	top_view->ConvertFromScreen(r);
}

/*---------------------------------------------------------------*/

void _set_menu_sem_(BWindow *w, sem_id sem)
{
	// this is a friend function to BWindow. This is used to allow
	// the MenuBar to set the fMenuSem field of the corresponding
	// window. Used to synchronize the closing of windows while in
	// the middle of menu tracking.
	w->fMenuSem = sem;
}

/*---------------------------------------------------------------*/

uint32	BWindow::Workspaces() const
{
	BWindow	*t = (BWindow *) this;
	ulong	result;

	if (t->Lock()) {
		a_session->swrite_l(GR_WORKSPACE);
		a_session->swrite_l(server_token);
		Flush();
		a_session->sread(4, &result);
		t->Unlock();
		return(result);
	} else
		return(0);
}

/*---------------------------------------------------------------*/

void BWindow::SetWorkspaces(uint32 space)
{
	if (Lock()) {
		a_session->swrite_l(GR_SET_WS);
		a_session->swrite_l(space);
		a_session->swrite_l(server_token);
		Flush();
		Unlock();
	}
}

/*---------------------------------------------------------------*/

bool BWindow::InUpdate()
{
	return fInUpdate;
}

/*---------------------------------------------------------------*/

bigtime_t BWindow::next_pulse() const
{
	return
		(fPulseState && fPulseState->fEnabled && fPulseState->fRate > 0)
		? fPulseState->fNext : B_INFINITE_TIMEOUT;
}

/*---------------------------------------------------------------*/

void BWindow::enable_pulsing(bool enable)
{
//+	printf("enable_pulsing(e=%d, o=%d)\n",
//+			enable, fPulseState ? fPulseState->fEnabled : false);
	if (enable && (fPulseState == NULL || !fPulseState->fEnabled)) {
		if (!fPulseState)
			fPulseState = new win_pulse_state;
		fPulseState->fNeeded = true;

		if (fPulseState->fRate > 0) {
			BMessenger me(this);
			MessageQueue()->RemoveMessages(B_PULSE, &me);
			fPulseState->fNext = system_time()+fPulseState->fRate;
			fPulseState->fEnabled = true;
			me.SendMessageAtTime(BMessage(B_PULSE), fPulseState->fNext);
		}
	} else if (!enable && fPulseState && fPulseState->fEnabled) {
//+		PRINT(("Stopping runner (%s) (rate=%Ld)\n", Title(), fPulseState->fRate));
		fPulseState->fEnabled = false;
	}
}

/*---------------------------------------------------------------*/

void	BWindow::AddFloater(BWindow *)
{
	// this function was never implemented
}


/*---------------------------------------------------------------*/

void	BWindow::RemoveFloater(BWindow *)
{
	// this function was never implemented
}

/*---------------------------------------------------------------*/

status_t	_safe_get_server_token_(const BLooper *l, int32 *token)
{
	// This function takes a BLooper, instead of a BWindow for a reason.
	// Look below for the use of dynamic_cast

	BAutolock	auto_lock(BLooper::sLooperListLock);

	if (!auto_lock.IsLocked()) {
		// in practice this should never happen.
		return B_ERROR;
	}
	
	if (!BLooper::IsLooperValid(l)) {
		// The looper is no longer valid
		return B_BAD_VALUE;
	}

	const BWindow *w;
	if ((w = dynamic_cast<const BWindow*>(l)) == NULL) {
		// We've got a looper, but it isn't a window...
		// must be that old window died, and a new looper (not a window)
		// was created in the same location. Tricky race condition and this
		// at least tries to deal with it.
		return B_BAD_VALUE;
	}
	// As always (when not using BMessengers) we now know that we have
	// a BWindow, but not necessarily the window that the caller expected.
		
	// now it is safe to deref the pointer
	*token = w->server_token;

	return B_OK;
}

status_t
BWindow::AddToSubset(BWindow *window)
{
	status_t	result;
	int32		tok;

	if ((result = _safe_get_server_token_(window, &tok)) != B_OK)
		return result;

	if (Lock()) {
		a_session->swrite_l(GR_ADD_SUBWINDOW);
		a_session->swrite_l(server_token);
		a_session->swrite_l(tok);
		Flush();
		a_session->sread(4, &result);

		Unlock();
	} else {
		result = B_BAD_VALUE;
	}

	return (result);		
}

/*---------------------------------------------------------------*/

status_t
BWindow::RemoveFromSubset(BWindow *window)
{
	status_t	result = B_ERROR;
	int32		tok;

	if ((result = _safe_get_server_token_(window, &tok)) != B_OK)
		return result;

	if (Lock()) {
		a_session->swrite_l(GR_REMOVE_SUBWINDOW);
		a_session->swrite_l(server_token);
		a_session->swrite_l(tok);
		Flush();
		a_session->sread(4, &result);

		Unlock();
	} else {
		result = B_BAD_VALUE;
	}
	
	return (result);
}

/*---------------------------------------------------------------*/

status_t
BWindow::SetFlags(uint32 flags)
{
	status_t error = B_ERROR;

	if (Lock()) {
		a_session->swrite_l(GR_SET_WINDOW_FLAGS);
		a_session->swrite_l(server_token);
		a_session->swrite_l(0xFFFFFFFF);	// no design
		a_session->swrite_l(0xFFFFFFFF);	// no behavior
		a_session->swrite_l(flags);			// set flags
		Flush();
	
		fFlags = flags;
		error = B_NO_ERROR;

		Unlock();
	}

	return (error);
}

/*---------------------------------------------------------------*/

uint32	
BWindow::Flags() const
{
	return (fFlags);
/*
	uint32 flags = 0;
	uint32 type = 0;
	BWindow *w = const_cast<BWindow*>(this);
	if (w->Lock()) {
		a_session->swrite_l(GR_GET_WINDOW_FLAGS);
		a_session->swrite_l(server_token);
		Flush();
		a_session->sread(4, &flags);
		a_session->sread(4, &type);
		w->Unlock();
	}
	return flags;
*/
}

/*---------------------------------------------------------------*/

status_t	
BWindow::SetType(window_type type)
{
	status_t	error = B_NO_ERROR;
	window_look	theLook;
	window_feel	theFeel;

	decompose_type(type, &theLook, &theFeel);

	error = SetLook(theLook);
	if (error == B_NO_ERROR)
		error = SetFeel(theFeel);	

	return (error);
/*
	status_t	error = B_BAD_VALUE;
	if (Lock()) {
		fType = type;
		a_session->swrite_l(GR_SET_WINDOW_FLAGS);
		a_session->swrite_l(server_token);
		a_session->swrite_l(0);				// don't change the flags
		a_session->swrite_l(0);   			// mask is 0
		a_session->swrite_l((int32) type);
		Flush();
		Unlock();
	}
	return error;
*/
}

/*---------------------------------------------------------------*/

window_type	
BWindow::Type() const
{
	return (compose_type(fLook, fFeel));

/*
	uint32 flags = 0;
	BWindow *w = const_cast<BWindow*>(this);
	if (w->Lock()) {
		a_session->swrite_l(GR_GET_WINDOW_FLAGS);
		a_session->swrite_l(server_token);
		Flush();
		a_session->sread(4, &flags);
		a_session->sread(4, (int32 *) &fType);
		w->Unlock();
	}
	return (window_type) fType;
*/
}

/*---------------------------------------------------------------*/

status_t
BWindow::SetLook(
	window_look	look)
{
	status_t error = B_ERROR;

	if (Lock()) {
		a_session->swrite_l(GR_SET_WINDOW_FLAGS);
		a_session->swrite_l(server_token);
		a_session->swrite_l(look);			// set look
		a_session->swrite_l(0xFFFFFFFF);	// no feel
		a_session->swrite_l(0xFFFFFFFF);	// no flags
		Flush();
	
		fLook = look;
		error = B_NO_ERROR;

		Unlock();
	}

	return (error);
}

/*---------------------------------------------------------------*/

window_look	
BWindow::Look() const
{
	return (fLook);
}

/*---------------------------------------------------------------*/

status_t
BWindow::SetFeel(
	window_feel	feel)
{
	status_t error = B_ERROR;

	if (Lock()) {
		a_session->swrite_l(GR_SET_WINDOW_FLAGS);
		a_session->swrite_l(server_token);
		a_session->swrite_l(0xFFFFFFFF);	// no look
		a_session->swrite_l(feel);			// set feel
		a_session->swrite_l(0xFFFFFFFF);	// no flags
		Flush();
	
		fFeel = feel;
		error = B_NO_ERROR;

		Unlock();
	}

	return (error);
}

/*---------------------------------------------------------------*/

window_feel
BWindow::Feel() const
{
	return (fFeel);
}

/*---------------------------------------------------------------*/

bool
BWindow::IsModal() const
{
	return ( (fFeel == B_MODAL_SUBSET_WINDOW_FEEL) ||
			 (fFeel == B_MODAL_APP_WINDOW_FEEL) || 
			 (fFeel == B_MODAL_ALL_WINDOW_FEEL) );
}

/*---------------------------------------------------------------*/

bool
BWindow::IsFloating() const
{
	return ( (fFeel == B_FLOATING_SUBSET_WINDOW_FEEL) ||
			 (fFeel == B_FLOATING_APP_WINDOW_FEEL) || 
			 (fFeel == B_FLOATING_ALL_WINDOW_FEEL) );
}

/*---------------------------------------------------------------*/

status_t
BWindow::GetFrameState(BMessage* into) const
{
	status_t result = B_ERROR;
	
	BWindow *w = const_cast<BWindow*>(this);
	if (w->Lock()) {
		a_session->swrite_l(GR_WGET_DECOR_STATE);
		Flush();
		int32 len;
		a_session->sread(4, &len);
		if (len > 0) {
			void* data = malloc(len);
			if (data) {
				a_session->sread(len, data);
				result = into->Unflatten((const char*)data);
				free(data);
			} else {
				a_session->drain(len);
				result = B_NO_MEMORY;
			}
		} else {
			into->MakeEmpty();
		}
		w->Unlock();
	}
	
	return result;
}

/*---------------------------------------------------------------*/

status_t
BWindow::UpdateFrameState(const BMessage& changes)
{
	BMallocIO io;
	status_t result = changes.Flatten(&io);
	if (result < B_OK) return result;
	
	result = B_ERROR;
	
	if (Lock()) {
		a_session->swrite_l(GR_WSET_DECOR_STATE);
		a_session->swrite_l(io.BufferLength());
		a_session->swrite(io.BufferLength(), io.Buffer());
		Flush();
	
		result = B_NO_ERROR;

		Unlock();
	}

	return (result);
}

/*---------------------------------------------------------------*/


status_t	BWindow::SendBehind(const BWindow *window)
{
	status_t	error;
	int32		tok;

	if ((error = _safe_get_server_token_(window, &tok)) != B_OK)
		return error;

	if (Lock()) {
		a_session->swrite_l(GR_SEND_BEHIND);
		a_session->swrite_l(server_token);

		// tok could of course be bad/dead by the time the app_server
		// gets it, but that could happen in any case.
		a_session->swrite_l(tok);
		Flush();
		a_session->sread(4, &error);
		Unlock();
	} else {
		error = B_BAD_VALUE;
	}

	return error;
}

/*---------------------------------------------------------------*/

window_type  BWindow::WindowType() const
{
	return Type();
}

/*---------------------------------------------------------------*/

window_type
BWindow::compose_type(
	window_look	look, 
	window_feel	feel) const
{
	window_type result = B_UNTYPED_WINDOW;

	switch (look) {
		case B_BORDERED_WINDOW_LOOK:
			if (feel == B_NORMAL_WINDOW_FEEL)
				result = B_BORDERED_WINDOW;			
			break;

		case B_TITLED_WINDOW_LOOK:
			if (feel == B_NORMAL_WINDOW_FEEL)
				result = B_TITLED_WINDOW;
			break;
	
		case B_DOCUMENT_WINDOW_LOOK:
			if (feel == B_NORMAL_WINDOW_FEEL)
				result = B_DOCUMENT_WINDOW;
			break;

		case B_MODAL_WINDOW_LOOK:
			if (feel == B_MODAL_APP_WINDOW_FEEL)
				result = B_MODAL_WINDOW;
			break;

		case B_FLOATING_WINDOW_LOOK:
			if (feel == B_FLOATING_APP_WINDOW_FEEL)
				result = B_FLOATING_WINDOW;
			break;

		default:
			break;
	}

	return (result);
}

/*---------------------------------------------------------------*/

void
BWindow::decompose_type(
	window_type	type, 
	window_look	*look,
	window_feel	*feel) const
{
	switch (type) {
		case B_TITLED_WINDOW:
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_MODAL_WINDOW:
			*look = B_MODAL_WINDOW_LOOK;
			*feel = B_MODAL_APP_WINDOW_FEEL;
			break;
	
		case B_DOCUMENT_WINDOW:
			*look = B_DOCUMENT_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_BORDERED_WINDOW:
			*look = B_BORDERED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_FLOATING_WINDOW:
			*look = B_FLOATING_WINDOW_LOOK;
			*feel = B_FLOATING_APP_WINDOW_FEEL;
			break;	

		case B_UNTYPED_WINDOW:
		default:
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
	}	
}

/*---------------------------------------------------------------*/

void
BWindow::SetIsFilePanel(
	bool	panel)
{
	fIsFilePanel = panel;
}

/*---------------------------------------------------------------*/

bool
BWindow::IsFilePanel() const
{
	return (fIsFilePanel);
}

/*---------------------------------------------------------------*/

status_t BWindow::SendToolTipInfo(const BView* who)
{
	if (!fTipInfo || !fTipInfo->fShower) return B_NO_INIT;
	
	// skip if not requested view or window is no longer active
	if ((who && fTipInfo->fShower != who) || !IsActive()) return B_OK;
	
	win_tip_info& wi = *fTipInfo;
	
	// retrieve tip information.
	status_t err;
	BToolTipInfo* info = wi.fTip->NewToolTipInfo();
	err = wi.fShower->GetToolTipInfo(wi.fCursor, &wi.fRegion, info);
	
	if (err == B_OK && wi.fRegion.Contains(wi.fCursor)) {
		wi.fTip->SetToolTipInfo(BMessenger(this),
								wi.fShower->ConvertToScreen(wi.fRegion),
								info);
	} else {
		delete info;
		HideTip();
	}
	
	return err;
}

void BWindow::MoveTipCursor(BView* v, BPoint view_loc)
{
	if (!IsActive() || !v) return;
	
	BView* orig_view = v;
	
	// find the view under the cursor that wants to show a tip.
	const BPoint screen_loc(v->ConvertToScreen(view_loc));
	while (v) {
		status_t err;
		BRect region;
		
		if (fTipInfo && fTipInfo->fShower == v) {
			// Point must be in this view's coordinate space.
			BPoint my_loc;
			if (v == orig_view) my_loc = view_loc;
			else my_loc = v->ConvertFromScreen(screen_loc);
			
			if (fTipInfo->fRegion.Contains(my_loc)) {
				// This view is currently displaying a tool tip -- have the tip handle
				// any cursor movement.
				BPoint last_loc = fTipInfo->fCursor;
				fTipInfo->fCursor = my_loc;
				fTipInfo->fTip->CursorMoved(BMessenger(this), screen_loc, my_loc-last_loc);
				return;
			}
		}
		
		err = v->GetToolTipInfo(screen_loc, &region);
		if (err == B_OK) {
			// Point must be in this view's coordinate space.
			BPoint my_loc;
			if (v == orig_view) my_loc = view_loc;
			else my_loc = v->ConvertFromScreen(screen_loc);
			
			if (region.Contains(my_loc)) {
				if (fTipInfo && fTipInfo->fShower) {
					// currently displaying some other tool tip -- hide it.
					HideTip();
				}
				if (!fTipInfo) fTipInfo = new win_tip_info;
				if (fTipInfo) {
					fTipInfo->fTip = BToolTip::Default();
					fTipInfo->fRegion = region;
					fTipInfo->fCursor = my_loc;
					fTipInfo->fShower = v;
					fTipInfo->fTip->ShowTip(BMessenger(this));
					return;
				}
			}
		}
	
		v = v->Parent();
	}
	
	if (fTipInfo && fTipInfo->fShower) {
		// make sure any currently display tool tip is now hidden.
		HideTip();
	}
}

void BWindow::HideTip()
{
	if (fTipInfo) {
		fTipInfo->fTip->HideTip(BMessenger(this));
		fTipInfo->fRegion = BRect();
		fTipInfo->fShower = NULL;
	}
}

void BWindow::KillTip()
{
	if (fTipInfo) {
		fTipInfo->fTip->KillTip(BMessenger(this));
		fTipInfo->fRegion = BRect();
		fTipInfo->fShower = NULL;
	}
}

/*---------------------------------------------------------------*/

BWindow::BWindow() {}
BWindow::BWindow(BWindow &)
	:	BLooper()
	{}
BWindow &BWindow::operator=(BWindow &) { return *this; }

/*----------------------------------------------------------------*/

status_t BWindow::Perform(perform_code d, void *arg)
{
	return inherited::Perform(d, arg);
}

/* ---------------------------------------------------------------- */
bool BWindow::QuitRequested()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BLooper::QuitRequested();
}

thread_id BWindow::Run()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BLooper::Run();
}

/*----------------------------------------------------------------*/

status_t BWindow::UISettingsChanged(const BMessage*, uint32)
{
	return B_OK;
}

/*----------------------------------------------------------------*/

#if _R5_COMPATIBLE_
extern "C" {
	_EXPORT status_t
	#if __GNUC__
	_ReservedWindow1__7BWindow
	#elif __MWERKS__
	_ReservedWindow1__7BWindowFv
	#endif
	(BWindow* This, const BMessage* changes, uint32 flags)
	{
		return This->BWindow::UISettingsChanged(changes, flags);
	}
}
#endif

/*----------------------------------------------------------------*/

BView* BWindow::NextNavigableView(BView* currentFocus, uint32 flags)
{
	int32		dir;
	bool		found;
	BView		*view;
	BView		*next = NULL;
	BView		*first_miss = NULL;
	bool		switch_group = (flags & B_NAVIGATE_GROUP);
	
	if (flags & B_NAVIGATE_NEXT)
	  dir = B_FORWARD;
	else
	  dir = B_BACKWARD;
	    
	view = currentFocus;
	BTraverseViews	t(this, view);

	do {
		first_miss = view;
		found = false;
		
		if (switch_group) {
			// 'first' indicates if we're looking for the first GROUP view
			bool	first = true;
			if (dir == B_BACKWARD) {
				// need to treat going backwards a little different.
				if (!view || (view->Flags() & B_NAVIGABLE_JUMP))
					first = true;
				else
					first = false;
			}
			// find the next/prev control group
			while ((view = ((dir == B_FORWARD) ? t.Next() : t.Previous())) != 0) {
				ulong flags = view->Flags();
				// view must be NAVIGABLE_GROUP and visible
				if ((flags & B_NAVIGABLE_JUMP) && !view->IsHidden()) {
	//+				PRINT(("found control group\n"));
					if (first) {
						next = view;
						break;
					} else {
						first = true;
					}
				}
			}
			if (next) {
				// want to start the search from 'next'
				t.Reset(this, next, true, false);
			} else {
				// didn't find any control groups... so start with first view
				// in the window - pretend the window is one big group
				t.Reset(this, NULL);
			}
			dir = B_FORWARD;
		}
	
		while ((view = ((dir == B_FORWARD) ? t.Next() : t.Previous())) != 0) {
			ulong flags = view->Flags();
			// view must be NAVIGABLE and visible
			if ((flags & B_NAVIGABLE) && !view->IsHidden()) {
				next = view;
				break;
			}
		}
	
		if (next) {
	//+		PRINT(("navigating to view \"%s\" (class=%s)\n",
	//+			next->Name() ? next->Name() : "null", class_name(next)));
			//next->MakeFocus(true);
			found = true;
		}
		
		view = next;
		
	} while (view && !found && view != first_miss);
	
	return view;	
}

/*----------------------------------------------------------------*/

#if _R5_COMPATIBLE_
extern "C" {
	_EXPORT BView*
	#if __GNUC__
	_ReservedWindow2__7BWindow
	#elif __MWERKS__
	_ReservedWindow2__7BWindowFv
	#endif
	(BWindow* This, BView* currentFocus, uint32 flags)
	{
		return This->BWindow::NextNavigableView(currentFocus, flags);
	}
}
#endif

/*----------------------------------------------------------------*/

void BWindow::_ReservedWindow3() {}
void BWindow::_ReservedWindow4() {}
void BWindow::_ReservedWindow5() {}
void BWindow::_ReservedWindow6() {}
void BWindow::_ReservedWindow7() {}
void BWindow::_ReservedWindow8() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
