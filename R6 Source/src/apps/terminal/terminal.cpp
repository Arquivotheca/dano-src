

#include	<Application.h>
#include	<Alert.h>
#include	<Bitmap.h>
#include	<Button.h>
#include	<CheckBox.h>
#include	<Clipboard.h>
#include	<ColorControl.h>
#include	<FilePanel.h>
#include	<FindDirectory.h>
#include	<MenuBar.h>
#include	<MenuItem.h>
#include	<Path.h>
#include	<PopUpMenu.h>
#include	<RadioButton.h>
#include	<Screen.h>
#include	<ScrollBar.h>
#include	<StringView.h>
#include	<TextControl.h>
#include	<NodeInfo.h>
#include	<Resources.h>
#include	<Roster.h>
#include	<TextView.h>
#include	"RNode.h"
#include	<sys/param.h>
#include	<unistd.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<string.h>
#include	"RNodeHandler.h"
#include	"Shell.h"
#include	<termio.h>
#include	"XString.h"
#include	"Termio.h"
#include	"rico.h"
#include	"reg.h"
#include	"Finder.h"
#include	"TermView.h"
#include	"Timer.h"
#include	"WindowScreen.h"
#include	<ctype.h>


struct geom {
	BRect	window,
		termview,
		scrollbar,
		border,
		menu;
};
struct Pref {
	uint		magic,
			version;
	BPoint		base;
	BTermView::Pref	termview;
	enum {
			MAGIC		= 0xF1F2F3F4,
			VERSION		= 2
	};
};
struct Border: BView {
		Border( RNode, BRect, BView *);
	private:
	void	AttachedToWindow( ),
		MessageReceived( BMessage *);
	RNode	node;
};
struct A: BApplication, RNode {
		A( );
	private:
	void	ArgvReceived( int32, char **),
		ReadyToRun( ),
		identify( ),
		kickmime( type_code, uint, icon_size),
		RefsReceived( BMessage *),
		MessageReceived( BMessage *),
		loadpref( const char *),
		loadcolor( BTermView::Pref, int),
		parsegeom( char *);
	char	*title,
		*preffile,
		**command;
	uint	id;
	BPoint	base;
	uint	ncol,
		nrow;
	bool	gflag,
		dfflag,
		dbflag,
		cfflag,
		cbflag,
		sfflag,
		sbflag;
	rgb_color
		dfcolor,
		dbcolor,
		cfcolor,
		cbcolor,
		sfcolor,
		sbcolor;
};
struct W {
	struct V {
		protected:
		virtual void	viewhandler( BMessage *)		= 0,
				lphandler( BMessage *)			= 0,
				sphandler( BMessage *)			= 0,
				tphandler( BMessage *)			= 0,
				fshandler( BMessage *)			= 0,
				fphandler( BMessage *)			= 0,
				findhandler( BMessage *)		= 0,
				lpstart( BLooper *, BPoint)		= 0,
				spstart( BLooper *, BPoint)		= 0,
				tpstart( BLooper *, BPoint)		= 0,
				fsstart( BLooper *, BPoint)		= 0,
				fpstart( BLooper *, BPoint)		= 0,
				findstart( BLooper *, BPoint, uint)	= 0;
		RNode		view,
				lp,
				sp,
				tp,
				fsp,
				fp,
				find;
	};
	struct View: public BHandler, public RNode, virtual V {
		protected:
			View( BLooper *);
		private:
		void	MessageReceived( BMessage *);
	};
	struct LPanel: public BHandler, virtual V {
		protected:
			LPanel( );
		private:
		void	lpstart( BLooper *, BPoint),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct SPanel: public BHandler, virtual V {
		protected:
			SPanel( );
		private:
		void	spstart( BLooper *, BPoint),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct TPref: public BHandler, virtual V {
		protected:
			TPref( );
		private:
		void	tpstart( BLooper *, BPoint),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct FSPref: public BHandler, virtual V {
		protected:
			FSPref( );
		private:
		void	fsstart( BLooper *, BPoint),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct FPanel: public BHandler, virtual V {
		protected:
			FPanel( );
		private:
		void	fpstart( BLooper *, BPoint),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct Find: public BHandler, virtual V {
		protected:
			Find( );
		private:
		void	findstart( BLooper *, BPoint, uint),
			MessageReceived( BMessage *);
		bool	active;
	};
	struct Win: public BWindow, public RNode, virtual V {
		protected:
			Win( RNode, geom, char *, char **);
		private:
		void	MessageReceived( BMessage *),
			DispatchMessage( BMessage *, BHandler *),
			WindowActivated( bool),
			Minimize( bool),
			gotonext( ),
			viewhandler( BMessage *),
			lphandler( BMessage *),
			sphandler( BMessage *),
			tphandler( BMessage *),
			fshandler( BMessage *),
			fphandler( BMessage *),
			findhandler( BMessage *),
			logstop( ),
			writesel( BMessage *),
			savepref( const char *, BMessage *, BPoint);
		BPoint	org( );
		FILE	*log;
		bool	minimized;
		RNode	edit,
			font,
			fenc,
			border;
	};
	struct Obj: Win, View, LPanel, SPanel, TPref, FPanel, FSPref, Find {
			Obj( RNode, geom, char *, char **);
#ifndef GCC_VIRTUAL_REPAIRED
		void	*operator new(size_t s){return (malloc( sizeof( Obj)));}
#endif
	};
};
struct SB: BScrollBar {
		SB( RNode, BView *, BRect);
	private:
	void	AttachedToWindow( ),
		ValueChanged( float),
		MessageReceived( BMessage *);
	bool	mousedown;
	RNode	timer,
		node;
};
struct WLP: BFilePanel, BHandler, RNode {
		WLP( RNode n, BPoint org): RNode( n), BFilePanel( B_SAVE_PANEL) {
			SetButtonLabel( B_DEFAULT_BUTTON, "Start Logging");
			Window( )->MoveTo( org);
			Window( )->SetTitle( "Log File");
			Show( );
			Establish( this, 0, "WLP");
		}
	void MessageReceived( BMessage *m) {
		switch (m->what) {
		case 'acti':
			Window( )->Show( );
			Window( )->Activate( );
		}
	}
	void SendMessage( const BMessenger *, BMessage *m) {
		entry_ref r;
		if (m->FindRef( "directory", 0, &r) == B_OK) {
			BPath path;
			BEntry( &r).GetPath( &path);
			path.Append( m->FindString( "name"));
			m = new BMessage( 'log');
			m->AddString( "file", path.Path( ));
			*this >> m;
		}
	}
};
struct WSP: BFilePanel, BHandler, RNode {
		WSP( RNode n, BPoint org): RNode( n), BFilePanel( B_SAVE_PANEL) {
			SetButtonLabel( B_DEFAULT_BUTTON, "Write");
			Window( )->MoveTo( org);
			Window( )->SetTitle( "Selection File");
			Show( );
			Establish( this, 0, "WSP");
		}
	void MessageReceived( BMessage *m) {
		switch (m->what) {
		case 'acti':
			Window( )->Show( );
			Window( )->Activate( );
		}
	}
	void SendMessage( const BMessenger *, BMessage *m) {
		entry_ref r;
		if (m->FindRef( "directory", 0, &r) == B_OK) {
			BPath path;
			BEntry( &r).GetPath( &path);
			path.Append( m->FindString( "name"));
			m = new BMessage( 'selW');
			m->AddString( "file", path.Path( ));
			*this >> m;
		}
	}
};
struct WFP: BFilePanel, BHandler, RNode {
		WFP( RNode n, BPoint org): RNode( n), BFilePanel( B_SAVE_PANEL) {
			Window( )->MoveTo( org);
			Window( )->SetTitle( "Settings File");
			Show( );
			Establish( this, 0, "WFP");
		}
	void MessageReceived( BMessage *m) {
		switch (m->what) {
		case 'acti':
			Window( )->Show( );
			Window( )->Activate( );
		}
	}
	void SendMessage( const BMessenger *, BMessage *m) {
		entry_ref r;
		if (m->FindRef( "directory", 0, &r) == B_OK) {
			BPath path;
			BEntry( &r).GetPath( &path);
			path.Append( m->FindString( "name"));
			m = new BMessage( 'SAVE');
			m->AddString( "file", path.Path( ));
			*this >> m;
		}
	}
};
struct WCP {
	struct S: BStringView {
			S( BRect, char *);
		private:
		void	AttachedToWindow( );
	};
	struct CC: BColorControl, BTermView::Pref, RNode {
				CC( RNode, RNode, BTermView::Pref *, BPoint);
		private:
		void		AttachedToWindow( ),
				SetValue( int32),
				MessageReceived( BMessage *);
		rgb_color	color;
		uint		key;
		RNode		view;
	};
	struct ColorPref: BWindow, RNode {
			ColorPref( RNode, char *, BPoint, BTermView::Pref *);
		private:
		void	MessageReceived( BMessage *);
	};
};
struct WTP {
	struct S: BScrollBar, RNode {
		S( RNode n1, RNode n2, BPoint p): RNode( n1), text( n2), BScrollBar( BRect( ), 0, 0, 1, 20, B_HORIZONTAL) {
			ResizeTo( 200, B_H_SCROLL_BAR_HEIGHT-1);
			MoveTo( p);
		}
		private:
		void AttachedToWindow( ) {
			Establish( this, Window( ), "WTP::S");
			*this >> 'tabs';
		}
		void ValueChanged( float v) {
			Invalidate( );
			BMessage *m = new BMessage( 'tabs');
			m->AddInt32( "", v);
			*this >> m;
		}
		void MessageReceived( BMessage *m) {
			switch (m->what) {
			case 'acti':
				Window( )->Activate( TRUE);
			case EOF:
				break;
			default:
				text >> m->what;
				unless (m->what == Value( ))
					SetValue( m->what);
			}
		}
		RNode	text;
	};
	struct T: BStringView, RNode {
		T( RNode n, BPoint p): RNode( n), BStringView( BRect( ), 0, 0) {
			ResizeTo( 30, 30);
			MoveTo( p);
			SetFontSize( 21);
		}
		private:
		void AttachedToWindow( ) {
			Establish( this, Window( ), "WTP::T");
		}
		void MessageReceived( BMessage *m) {
			unless (m->what == EOF) {
				char t[20];
				sprintf( t, "%d", m->what);
				SetText( t);
			}
		}
	};
	struct TabPref: BWindow {
		TabPref( RNode node, BPoint org): BWindow( BRect( 0, 0, 350, 50), "Tab Width", B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE) {
			RNode	n1,
				n2;
			MoveTo( org);
			Show( );
			n1.SetDestination( n2);
			AddChild( new S( node, n1, BPoint( 100, 20)));
			AddChild( new T( n2, BPoint( 40, 9)));
		}
	};
};
struct wfp {
	struct S: BScrollBar, RNode {
		S( RNode n1, RNode n2, BPoint p): RNode( n1), text( n2), BScrollBar( BRect( ), 0, 0, 3, 40, B_HORIZONTAL) {
			initialized = FALSE;
			delaying = FALSE;
			ResizeTo( 200, B_H_SCROLL_BAR_HEIGHT-1);
			MoveTo( p);
		}
		private:
		void AttachedToWindow( ) {
			delayer.SetDestination( *this);
			Establish( this, Window( ), "wfp::S");
			*this >> 'fsiz';
		}
		void ValueChanged( float v) {
			Invalidate( );
			unless (delaying) {
				BTimerSend( delayer, new BMessage( 'send'), 1000000);
				delaying = TRUE;
			}
			BMessage *m = new BMessage( 'fsiz');
			m->AddFloat( "", v);
			text >> m;
		}
		void MessageReceived( BMessage *m) {
			switch (m->what) {
			case 'acti':
				Window( )->Activate( TRUE);
				break;
			case 'send':
				m = new BMessage( 'fsiz');
				m->AddFloat( "", Value( ));
				*this >> m;
				delaying = FALSE;
			case EOF:
				break;
			default:
				unless (initialized) {
					SetValue( m->FindFloat( ""));
					initialized = TRUE;
				}
				text >> Looper( )->DetachCurrentMessage( );
			}
		}
		bool	initialized,
			delaying;
		RNode	delayer,
			text;
	};
	struct T: BStringView, RNode {
		T( RNode n, BPoint p): RNode( n), BStringView( BRect( ), 0, 0) {
			ResizeTo( 30, 30);
			MoveTo( p);
			SetFontSize( 21);
		}
		private:
		void AttachedToWindow( ) {
			Establish( this, Window( ), "wfp::T");
		}
		void MessageReceived( BMessage *m) {
			unless (m->what == EOF) {
				char t[20];
				sprintf( t, "%.1f", m->FindFloat( ""));
				SetText( t);
			}
		}
	};
	struct FontPref: BWindow {
		FontPref( RNode node, BPoint org): BWindow( BRect( 0, 0, 350, 50), "Font Size", B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE) {
			RNode	n1,
				n2;
			MoveTo( org);
			Show( );
			n1.SetDestination( n2);
			AddChild( new S( node, n1, BPoint( 100, 20)));
			AddChild( new T( n2, BPoint( 40, 9)));
		}
	};
};
struct WF: BWindow, RNode {
	struct H: BView {
			H( BPoint, uint);
		private:
		void	Draw( BRect);
		uint	len;
	};
			WF( RNode, BPoint);
	private:
	void		MessageReceived( BMessage *);
	bool		QuitRequested( );
	BRadioButton	*rb0,
			*rb1;
	BTextControl	*tc;
	BCheckBox	*cb0,
			*cb1,
			*cb2,
			*cb3;
};
struct EditMenu: BMenu, RNode {
		EditMenu( RNode);
	private:
	void	AttachedToWindow( );
	void	MessageReceived( BMessage *);
};
struct FontMenu: BMenu, RNode {
		FontMenu( RNode);
	private:
	void	AttachedToWindow( ),
		DetachedFromWindow( ),
		MessageReceived( BMessage *);
	char	*label( const char *);
	bool	initialized;
};
struct EncodingMenu: BMenu, RNode {
		EncodingMenu( RNode);
	private:
	void	AttachedToWindow( ),
		DetachedFromWindow( ),
		MessageReceived( BMessage *);
	bool	initialized;
};


static rgb_color
		colorconv( char *);
uint		narg( char **);
static geom	getgeom( BPoint);
static uint	menuheight( );
static void	inform( char *, ...),
		warn( char *, ...);

static char	APPSIG[]	= "application/x-vnd.Be-SHEL",
		PREFSIG[]	= "application/x-vnd.Be-pref";

bool commandIsMeta;
#define SC(C) (commandIsMeta ? 0 : (C))


int
main(int argc, char **argv)
{
	A	a;

	a.Run( );
	//RNode::DumpTable( );

	return (0);
}


A::A( ):
BApplication( APPSIG)
{
	static char	*cdef[]		= { "/bin/sh", "-login", 0 };
	char		s[99];
	char		*shell		= getenv("SHELL");

	preffile = 0;
	gflag = FALSE;
	dfflag = FALSE;
	dbflag = FALSE;
	cfflag = FALSE;
	cbflag = FALSE;
	sfflag = FALSE;
	sbflag = FALSE;
	be_clipboard->Lock( );
	identify( );
	be_clipboard->Unlock( );
	if (id == 1) {
		kickmime( 'ICON', 32, B_LARGE_ICON);
		kickmime( 'MICN', 16, B_MINI_ICON);
	}
	sprintf( s, "Terminal %d", id);
	title = strdup( s);

	if (shell && (access(shell, X_OK)== 0)) {
		cdef[0] = shell;
	}
	command = cdef;

	base = BPoint( -10+id*16, 9+id*16);
	nrow = 24;
	ncol = 80;
}


void
A::ArgvReceived( int32, char **av)
{

	while (*++av) {
		unless (**av == '-') {
			if (char **c = (char **) malloc( sizeof( *c)*(narg( av)+1))) {
				uint i = 0;
				loop {
					unless (*av) {
						c[i] = 0;
						command = c;
						break;
					}
					unless (c[i++] = strdup( *av++))
						break;
				}
			}
			break;
		}
		if (streq( *av, "-meta"))
			commandIsMeta = true;
		else if ((streq( *av, "-t"))
		or (streq( *av, "-title"))) {
			if (av[1])
				title = strdup( *++av);
		}
		else if (streq( *av, "-p")) {
			if (av[1])
				preffile = strdup( *++av);
		}
		else if (streq( *av, "-geom")) {
			if (av[1]) {
				parsegeom( *++av);
				gflag = TRUE;
			}
		}
		else if (streq( *av, "-fg")) {
			if (av[1]) {
				dfcolor = colorconv( *++av);
				dfflag = TRUE;
			}
		}
		else if (streq( *av, "-bg")) {
			if (av[1]) {
				dbcolor = colorconv( *++av);
				dbflag = TRUE;
			}
		}
		else if (streq( *av, "-selfg")) {
			if (av[1]) {
				sfcolor = colorconv( *++av);
				sfflag = TRUE;
			}
		}
		else if (streq( *av, "-selbg")) {
			if (av[1]) {
				sbcolor = colorconv( *++av);
				sbflag = TRUE;
			}
		}
		else if (streq( *av, "-curfg")) {
			if (av[1]) {
				cfcolor = colorconv( *++av);
				cfflag = TRUE;
			}
		}
		else if (streq( *av, "-curbg")) {
			if (av[1]) {
				cbcolor = colorconv( *++av);
				cbflag = TRUE;
			}
		}
	}
}


void
A::ReadyToRun( )
{
	BTermView::Pref	t;
	system_info	si;
	RNode		n1;

	get_system_info( &si);
	unless ((si.used_sems+120*2 < si.max_sems)
	and (si.used_ports+22*2 < si.max_ports)
	and (si.used_threads+10*2 < si.max_threads)
	and (si.used_teams+2*2 < si.max_teams)) {
		char MESG[] = "There are insufficient system resources to run this copy of Terminal.";
		BAlert *a = new BAlert( "", MESG, "Oh, well.", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		a->Go( );
		Quit( );
		return;
	}
	geom g = getgeom( BPoint( 80*7, 34*11));
	g.window.OffsetBy( base.x, base.y);
	SetDestination( n1);
	n1.SetDestination( *this);
	new W::Obj( n1, g, title, command);
	Establish( 0, this, "A");

	if (preffile)
		loadpref( preffile);
	else {
		BPath def;
		find_directory( B_USER_SETTINGS_DIRECTORY, &def);
		def.Append( "Terminal");
		BEntry e( def.Path( ));
		if (e.Exists( ))
			loadpref( def.Path( ));
	}

	t.foreground = dfcolor;
	t.background = dbcolor;
	t.cursorfg = cfcolor;
	t.cursorbg = cbcolor;
	t.selfg = sfcolor;
	t.selbg = sbcolor;
	if (dfflag)
		loadcolor( t, 'df');
	if (dbflag)
		loadcolor( t, 'db');
	if (cfflag)
		loadcolor( t, 'cf');
	if (cbflag)
		loadcolor( t, 'cb');
	if (sfflag)
		loadcolor( t, 'sf');
	if (sbflag)
		loadcolor( t, 'sb');

	if (gflag) {
		BMessage *m = new BMessage( 'tprf');
		t.ncol = ncol;
		t.nrow = nrow;
		m->AddInt32( "BTermView", 'size');
		m->AddData( "", B_UINT8_TYPE, &t, sizeof t);
		*this >> m;
	}

	*this >> 'sync';
}


void
A::identify( )
{
	const char	PNAME[]	= "TerminalNames";
	team_id		r[64];

	id = 0;
	port_id p = find_port( PNAME);
	if (p < B_NO_ERROR) {
		p = create_port( 1, PNAME);
		if (p < B_NO_ERROR)
			return;
		set_port_owner( p, B_SYSTEM_TEAM);
		memset( r, 0, sizeof r);
	}
	else {
		long _;
		read_port( p, &_, r, sizeof r);
	}
	for (uint i=0; i<nel( r); ++i) {
		team_info ti;
		if ((not r[i])
		or (get_team_info( r[i], &ti) < B_NO_ERROR)) {
			r[i] = Team( );
			id = i + 1;
			break;
		}
	}
	write_port( p, 0, r, sizeof r);
}


void
A::kickmime( type_code t, uint n, icon_size is)
{
	BResources	*r;
	const void	*d;
	size_t		dsize;

	BBitmap b( BRect( 0, 0, n-1, n-1), B_CMAP8);
	BMimeType m( PREFSIG);
	if ((m.GetIcon( &b, is) < 0)
	and (r = AppResources( ))
	and (d = r->LoadResource( t, 2, &dsize))
	and (dsize == n*n)) {
		memcpy( b.Bits( ), d, dsize);
		m.SetIcon( &b, is);
	}
}


void
A::RefsReceived( BMessage *m)
{
	uint32		type;
	int32		count, i;
	entry_ref	ref;
	BEntry		entry;
	BPath       path;

	if (m->GetInfo("refs", &type, &count) != B_NO_ERROR)
		return;
	for(i=0; i < count; i++) {
		if (m->FindRef("refs", i, &ref) != B_NO_ERROR)
			continue;
		if (entry.SetTo(&ref) != B_NO_ERROR)
			continue;
		if (entry.GetPath(&path) < 0)
			continue;
		preffile = strdup( path.Path( ));
		break;
	}
}


void
A::loadpref( const char *file)
{
	Pref	pref;

	if (file) {
		int fd = open( file, O_RDONLY);
		if (fd < 0)
			warn( "Cannot open %s (%s).\n", file, strerror( errno));
		else {
			unless ((read( fd, &pref, sizeof pref) == sizeof pref)
			and (pref.magic == Pref::MAGIC))
				warn( "%s is not a Terminal preference file.", file);
			else unless (pref.version == Pref::VERSION)
				warn( "Terminal preference file %s is out-of-date.", file);
			else {
				BMessage *m = new BMessage( 'pref');
				m->AddData( "", B_UINT8_TYPE, &pref, sizeof pref);
				*this >> m;
			}
			close( fd);
		}
	}
}


void
A::MessageReceived( BMessage *m)
{

	//printf( "A::MessageReceived: what=0x%X\n", m->what);
	switch (m->what) {
	case 'acti':
		*this >> 'acti';
		break;
	case 'new':
		{
			BEntry    pref_entry(preffile);

			if (pref_entry.InitCheck()== B_OK) {
				entry_ref pref_entry_ref;
				pref_entry.GetRef(&pref_entry_ref);
				be_roster->Launch(&pref_entry_ref);
			} else {
				be_roster->Launch( APPSIG);
			}
		} break;
	case 'newd':
		be_roster->Launch( APPSIG);
		break;
	case EOF:
		Quit( );
	}
}


void
A::parsegeom( char *s)
{
	uint	c,
		r;
	int	x,
		y;

	if (sscanf( s, "%ux%u+%d+%d", &c, &r, &x, &y) == 4) {
		ncol = c;
		nrow = r;
		base.x = x;
		base.y = y;
	}
	else if (sscanf( s, "%ux%u", &c, &r) == 2) {
		ncol = c;
		nrow = r;
	}
	else if (sscanf( s, "+%d+%d", &x, &y) == 2) {
		base.x = x;
		base.y = y;
	}
}


void
A::loadcolor( BTermView::Pref t, int key)
{

	BMessage *m = new BMessage( 'tprf');
	m->AddInt32( "BTermView", key);
	m->AddData( "", B_UINT8_TYPE, &t, sizeof t);
	*this >> m;
}


W::Obj::Obj( RNode n, geom g, char *t, char **c):
Win( n, g, t, c), View( this)
{

	View &v = *this;
	v.Establish( &v, this, "W::View");
}


W::Win::Win( RNode n1, geom g, char *t, char **c):
RNode( n1), BWindow( g.window, t, B_DOCUMENT_WINDOW, 0)
{
	RNode	n2,
		n3,
		n4,
		n5,
		n6,
		n7,
		n8,
		n9,
		n10,
		n11,
		n13,
		n20,
		n21,
		n22,
		n23,
		n24,
		n25,
		n26;

	log = 0;
	minimized = FALSE;

	Run( );

	n2.SetDestination( n4);
	n3.SetDestination( n6);
	n4.SetDestination( n8);
	n5.SetDestination( n2);
	n6.SetDestination( n9);
	n7.SetDestination( n3);
	n8.SetDestination( n5);
	n9.SetDestination( n7);
	n10.SetDestination( n11);
	n11.SetDestination( n23);
	n13.SetDestination( view);
	n20.SetDestination( n21);
	n21.SetDestination( n20);
	view.SetDestination( n13);
	n23.SetDestination( n24);
	n24.SetDestination( n10);
	n25.SetDestination( n13);
	n26.SetDestination( n13);
	edit.SetDestination( n25);
	font.SetDestination( n22);
	fenc.SetDestination( n26);

	new BTermio( n4, n5, n6, n7, n10, n23);
	new BShell::Obj( c, n8, n9, n11);
	AddChild( new SB( n21, 0, g.scrollbar));
	AddChild( new Border( border, g.border, new BTermView( n2, n3, n20, n13, n24, g.termview)));

	BMenuBar *mb = new BMenuBar( g.menu, "");
	BMenu *m6 = new BMenu( "Terminal");
	BMenu *m7 = new EditMenu( n25);
	BMenu *m8 = new BMenu( "Settings");
	BMenu *m1 = new BMenu( "Window Size");
	BMenu *m4 = new FontMenu( n22);
	BMenu *m5 = new EncodingMenu( n26);
	m6->AddItem( new BMenuItem( "Switch Terminals", new BMessage( 'next'), SC('G')));
	m6->AddItem( new BMenuItem( "Start New Terminal", new BMessage( 'new'), SC('N')));
	m6->AddItem( new BMenuItem( "Start New Terminal - default", new BMessage( 'newd'), SC('N'), B_SHIFT_KEY));
	m6->AddItem( new BMenuItem( "Log to File...", new BMessage( 'log')));
	m7->AddItem( new BMenuItem( "Copy", new BMessage( 'copy'), SC('C')));
	m7->AddItem( new BMenuItem( "Paste", new BMessage( 'pste'), SC('V')));
	m7->AddSeparatorItem( );
	m7->AddItem( new BMenuItem( "Select All", new BMessage( 'sela'), SC('A')));
	m7->AddItem( new BMenuItem( "Write Selection...", new BMessage( 'selw')));
	m7->AddItem( new BMenuItem( "Clear All", new BMessage( 'clrh'), SC('L')));
	m7->AddSeparatorItem( );
	m7->AddItem( new BMenuItem( "Find...", new BMessage( 'find'), SC( 'F')));
	m7->AddItem( new BMenuItem( "Find Backward", new BMessage( 'fnd-'), SC( '[')));
	m7->AddItem( new BMenuItem( "Find Forward", new BMessage( 'fnd+'), SC( ']')));
	m8->AddItem( m1);
	m8->AddItem( m4);
	m8->AddItem( new BMenuItem( "Font Size...", new BMessage( 'fsiz')));
	m8->AddItem( m5);
	m8->AddItem( new BMenuItem( "Tab Width...", new BMessage( 'tabs')));
	m8->AddItem( new BMenuItem( "Color...", new BMessage( 'colr')));
	m8->AddSeparatorItem( );
	m8->AddItem( new BMenuItem( "Save as Defaults", new BMessage( 'save')));
	m8->AddItem( new BMenuItem( "Save as Settings File...", new BMessage( 'Save')));
	m1->AddItem( new BMenuItem( "80x24", new BMessage( 80*24)));
	m1->AddItem( new BMenuItem( "80x25", new BMessage( 80*25)));
	m1->AddItem( new BMenuItem( "80x40", new BMessage( 80*40)));
	m1->AddItem( new BMenuItem( "132x24", new BMessage( 132*24)));
	m1->AddItem( new BMenuItem( "132x25", new BMessage( 132*25)));
	mb->AddItem( m6);
	mb->AddItem( m7);
	mb->AddItem( m8);
	AddChild( mb);

	SetSizeLimits( 20, 1e9, 55, 1e9);
	Establish( this, this, "W");
}


void
W::Win::WindowActivated( bool b)
{

	if (b)
		minimized = FALSE;
}


void
W::Win::Minimize( bool b)
{

	minimized = b;
	BWindow::Minimize( b);
}


void
W::Win::DispatchMessage( BMessage *m, BHandler *h)
{

  if (m->what == B_KEY_DOWN && commandIsMeta) {
	int32 mods = 0;
	m->FindInt32("modifiers", &mods);
	if (mods & B_COMMAND_KEY) {
	  // map COMMAND- into ESC-
	  mods &= ~(B_COMMAND_KEY | B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY);
	  m->ReplaceInt32("modifiers", mods);
	  uint32 t;
	  int32 n = 0;
	  int8 c = 033;		// ESC
	  m->GetInfo("byte", &t, &n);
	  for (int32 i = 0; i < n; i++) {
		int8 cx;
		m->FindInt8("byte", i, &cx);
		m->ReplaceInt8("byte", i, c);
		c = cx;
	  }
	  if (mods & B_CONTROL_KEY)	// Hack around the strange behavior of 
		c &= 0x1f;				// keyboard() in servers/app/server.cpp
	  m->AddInt8("byte", c);
	}
  }

  BWindow::DispatchMessage(m, h);
}


void
W::Win::MessageReceived( BMessage *m)
{
	uint	w;

	//printf( "W::Win::MessageReceived: what='%.4s'\n", &m->what);
	switch (m->what) {
	case 'pref':
		Pref *pref;
		ssize_t _;
		m->FindData( "", B_UINT8_TYPE, (const void **)&pref, &_);
		m = new BMessage( 'pref');
		m->AddData( "", B_UINT8_TYPE, &pref->termview, sizeof pref->termview);
		m->AddInt32( "BTermView", 'init');
		view >> m;
		unless (pref->base == BPoint( 0, 0))
			MoveTo( pref->base);
		break;
	case 'next':
		gotonext( );
		break;
	case 'acti':
		if (minimized || IsHidden() || !(Workspaces() & (1 << current_workspace())))
			gotonext( );
		else {
			Activate();
			if (focus_follows_mouse())
				set_mouse_position(Frame().left + 16.0, Frame().top - 19.0);
		}
		break;
	case 'new':
		*this >> 'new';
		break;
	case 'newd':
		*this >> 'newd';
		break;
	case 'log':
		if (log)
			logstop( );
		else
			lpstart( this, org( ));
		break;
	case 'tabs':
		tpstart( this, org( ));
		break;
	case 'fsiz':
		fsstart( this, org( ));
		break;
	case 'colr':
	case 'save':
		w = m->what;
		m = new BMessage( 'pref');
		m->AddInt32( "BTermView", 'get');
		m->AddInt32( "terminal", w);
		view >> m;
		break;
	case 'Save':
		fpstart( this, org( ));
		break;
	case 'find':
	case 'fnd-':
	case 'fnd+':
		findstart( this, org( ), m->what);
		break;
        case B_GET_PROPERTY:
        case B_SET_PROPERTY:
                BWindow::MessageReceived( m);
                break;
	case 'tprf':
		m->what = 'pref';
	default:
		view >> DetachCurrentMessage( );
	}
}


void
W::Win::gotonext( )
{
	BList	tl;

	be_roster->GetAppList( APPSIG, &tl);
	const uint n = tl.CountItems( );
	unless (n < 2)
		for (uint i=0; i<n; ++i)
			if (Team( ) == (team_id)tl.ItemAt( i)) {
				app_info a;
				team_id t = (team_id) tl.ItemAt( i? i-1: n-1);
				if (be_roster->GetRunningAppInfo( t, &a) == B_NO_ERROR) {
					BMessenger *mr = new BMessenger( (char *) 0, t);
					if (mr->IsValid( ))
						mr->SendMessage( 'acti');
					delete mr;
				}
				break;
			}
}


void
W::Win::viewhandler( BMessage *m)
{
	geom	g;
	bool	b;
	char	*s;

	//printf( "W::Win::viewhandler: what='%.4s'\n", &m->what);
	switch (m->what) {
	case EOF:
		Quit( );
		break;
	case 'selw':
		if (m->FindBool( "sel?", &b) == B_OK)
			if (b)
				spstart( this, org( ));
			else
				warn( "There is no selection.");
		break;
	case 'selW':
		writesel( m);
		break;
	case 'size':
		g = getgeom( m->FindPoint( ""));
		ResizeTo( g.window.right, g.window.bottom);
		break;
	case 'edit':
		edit >> DetachCurrentMessage( );
		break;
	case 'fnam':
		font >> DetachCurrentMessage( );
		break;
	case 'fenc':
		fenc >> DetachCurrentMessage( );
		break;
	case 'pref':
		switch (m->FindInt32( "terminal")) {
		case 'colr':
			char s[99];
			long n;
			sprintf( s, "Colors for %s", Title( ));
			const void *p;
			m->FindData( "", B_UINT8_TYPE, &p, &n);
			new WCP::ColorPref( view, s, org( ), (BTermView::Pref *)p);
			break;
		case 'save':
		{
			BPath path;
			find_directory( B_USER_SETTINGS_DIRECTORY, &path, true);
			path.Append( "Terminal");
			savepref( path.Path( ), m, BPoint( 0, 0));
			break;
		}
		case 'SAVE':
			savepref( m->FindString( "file"), m, Frame( ).LeftTop( ));
		}
		break;
	case 'sync':
		Show( );
		break;
	case 'tabs':
		tp >> m->FindInt32( "");
		break;
	case 'fsiz':
		fsp >> DetachCurrentMessage( );
		break;
	case 'warn':
		m->FindString( "", (const char **)&s);
		warn( "%s", s);
		break;
	default:
		border >> DetachCurrentMessage( );
	}
}


void
W::Win::lphandler( BMessage *m)
{
	BMenuBar	*mb;
	BMenuItem	*mi;
	const char	*file;

	if ((mb = KeyMenuBar( ))
	and (mi = mb->FindItem( "Terminal"))
	and (mi = mi->Submenu( )->FindItem( 'log'))
	and (m->FindString( "file", &file) == B_OK))
		if (log = fopen( file, "w")) {
			m->AddPointer( "", log);
			view >> m;
			mi->SetLabel( "Stop Logging...");
		}
		else
			warn( "Cannot write to log file (%s).", strerror( errno));
}


void
W::Win::sphandler( BMessage *m)
{

	view >> m;
}


void
W::Win::tphandler( BMessage *m)
{

	view >> m;
}


void
W::Win::fshandler( BMessage *m)
{

	view >> m;
}


void
W::Win::fphandler( BMessage *m)
{

	m->what = 'pref';
	m->AddInt32( "BTermView", 'get');
	m->AddInt32( "terminal", 'SAVE');
	view >> m;
}


void
W::Win::findhandler( BMessage *m)
{

	view >> m;
}


void
W::Win::logstop( )
{
	BMenuBar	*mb;
	BMenuItem	*mi;
	const char	*file;

	if ((mb = KeyMenuBar( ))
	and (mi = mb->FindItem( "Terminal"))
	and (mi = mi->Submenu( )->FindItem( 'log'))) {
		inform( "%d bytes logged.", ftell( log));
		view >> 'log0';
		log = 0;
		mi->SetLabel( "Log to File...");
	}
}


void
W::Win::writesel( BMessage *m)
{
	char	*file,
		*sel;

	if ((m->FindString( "file", (const char **)&file) == B_OK)
	and (m->FindString( "sel", (const char **)&sel) == B_OK)) {
		int fd = creat( file, 0666);
		if (fd < 0)
			warn( "Cannot create file (%s).", strerror( errno));
		else {
			int n = strlen( sel);
			int i = write( fd, sel, n);
			if (i < 0)
				warn( "Write error (%s).", strerror( errno));
			else unless (i == n)
				warn( "Hmm, wrote only %d of %d bytes.");
			else
				inform( "%d bytes written.", n);
			close( fd);
		}
	}
}


void
W::Win::savepref( const char *path, BMessage *m, BPoint base)
{

	int fd = creat( path, 0666);
	if (fd < 0)
		warn( "Cannot create %s.", path);
	else {
		long n;
		const void *p;
		m->FindData( "", B_UINT8_TYPE, &p, &n);
		if (p) {
			Pref pref;
			pref.magic = Pref::MAGIC;
			pref.version = Pref::VERSION;
			pref.base = base;
			pref.termview = *(BTermView::Pref *)p;
			write( fd, &pref, sizeof pref);
		}
		close( fd);
		BNode bn( path);
		BNodeInfo ni( &bn);
		ni.SetType( PREFSIG);
	}
}


BPoint
W::Win::org( )
{

	BPoint s = BScreen( this).Frame( ).RightBottom( ) - BPoint( 30, 30);
	BPoint o = Frame( ).LeftTop( ) + BPoint( 70, 60);
	o.x = min( o.x, s.x);
	o.y = min( o.y, s.y);
	return (o);
}


W::View::View( BLooper *l):
RNode( view)
{

}


void
W::View::MessageReceived( BMessage *m)
{

	//printf( "W::View::MessageReceived: what=0x%08X'\n", m->what);
	viewhandler( m);
}


W::LPanel::LPanel( )
{

	active = FALSE;
}


void
W::LPanel::lpstart( BLooper *l, BPoint org)
{

	if (active)
		lp >> 'acti';
	else {
		RNode n;
		lp = RNode( );
		lp.SetDestination( n);
		n.SetDestination( lp);
		new WLP( n, org);
		lp.Establish( this, l, "W::LPanel");
		active = TRUE;
	}
}


void
W::LPanel::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		lphandler( Looper( )->DetachCurrentMessage( ));
	}
}


W::SPanel::SPanel( )
{

	active = FALSE;
}


void
W::SPanel::spstart( BLooper *l, BPoint org)
{

	if (active)
		sp >> 'acti';
	else {
		RNode n;
		sp = RNode( );
		sp.SetDestination( n);
		n.SetDestination( sp);
		new WSP( n, org);
		sp.Establish( this, l, "W::SPanel");
		active = TRUE;
	}
}


void
W::SPanel::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		sphandler( Looper( )->DetachCurrentMessage( ));
	}
}


W::TPref::TPref( )
{

	active = FALSE;
}


void
W::TPref::tpstart( BLooper *l, BPoint org)
{

	if (active)
		tp >> 'acti';
	else {
		RNode n;
		tp = RNode( );
		tp.SetDestination( n);
		n.SetDestination( tp);
		new WTP::TabPref( n, org);
		tp.Establish( this, l, "W::TPref");
		active = TRUE;
	}
}


void
W::TPref::MessageReceived( BMessage *m)
{

	//printf( "W::TPref::MessageReceived: what='%.4s'\n", &m->what);
	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		tphandler( Looper( )->DetachCurrentMessage( ));
	}
}


W::FSPref::FSPref( )
{

	active = FALSE;
}


void
W::FSPref::fsstart( BLooper *l, BPoint org)
{

	if (active)
		fsp >> 'acti';
	else {
		RNode n;
		fsp = RNode( );
		fsp.SetDestination( n);
		n.SetDestination( fsp);
		new wfp::FontPref( n, org);
		fsp.Establish( this, l, "W::TPref");
		active = TRUE;
	}
}


void
W::FSPref::MessageReceived( BMessage *m)
{

	//printf( "W::FSPref::MessageReceived: what='%.4s'\n", &m->what);
	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		fshandler( Looper( )->DetachCurrentMessage( ));
	}
}


W::FPanel::FPanel( )
{

	active = FALSE;
}


void
W::FPanel::fpstart( BLooper *l, BPoint org)
{

	if (active)
		fp >> 'acti';
	else {
		RNode n;
		fp = RNode( );
		fp.SetDestination( n);
		n.SetDestination( fp);
		new WFP( n, org);
		fp.Establish( this, l, "W::FPanel");
		active = TRUE;
	}
}


void
W::FPanel::MessageReceived( BMessage *m)
{

	//printf( "W::FPanel::MessageReceived: what='%.4s'\n", &m->what);
	switch (m->what) {
	case EOF:
		active = FALSE;
		break;
	default:
		fphandler( Looper( )->DetachCurrentMessage( ));
	}
}


W::Find::Find( )
{

	active = FALSE;
}


void
W::Find::findstart( BLooper *l, BPoint org, uint com)
{

	unless (active) {
		RNode n;
		find.SetDestination( n);
		n.SetDestination( find);
		new WF( n, org);
		find.Establish( this, l, "W::Find");
		active = TRUE;
	}
	find >> com;
}


void
W::Find::MessageReceived( BMessage *m)
{

	findhandler( Looper( )->DetachCurrentMessage( ));
}


SB::SB( RNode n, BView *v, BRect r):
node( n), BScrollBar( r, 0, v, 0, 99, B_VERTICAL)
{

	mousedown = FALSE;
}


void
SB::AttachedToWindow( )
{

	timer.SetDestination( node);
	node.Establish( this, Looper( ), "SB");
}


void
SB::ValueChanged( float v)
{
	BMessage	*m;

	if ((m = Window( )->CurrentMessage( ))
	and (m->what == B_VALUE_CHANGED)
	and (not mousedown)) {
		BTimerSend( timer, new BMessage( 't'), 500000);
		mousedown = TRUE;
		node >> 'd';
	}
	m = new BMessage( 'v');
	m->AddInt32( "", v);
	node >> m;
}


void
SB::MessageReceived( BMessage *m)
{
	ulong	buttons;
	BPoint	pos;

	switch (m->what) {
	case 's&l':
		SetSteps( m->FindInt32( "s"), m->FindInt32( "l"));
		break;
	case 'v&r':
		unless (mousedown) {
			SetRange( 0, m->FindInt32( "r"));
			SetValue( m->FindInt32( "v"));
			SetProportion( m->FindFloat( ""));
		}
		break;
	case 't':
		GetMouse( &pos, &buttons, FALSE);
		if (buttons)
			BTimerSend( timer, new BMessage( 't'), 500000);
		else {
			mousedown = FALSE;
			node >> 'u';
		}
	}
}


WF::WF( RNode n, BPoint org):
RNode( n), BWindow( BRect( 0, 0, 1, 1), "Find", B_TITLED_WINDOW, B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{

	BRect r( 0, 0, 210, 225);
	BView *v = new BView( r, 0, 0, B_WILL_DRAW);
	v->SetViewColor( 216, 216, 216);
	rb0 = new BRadioButton( BRect( 10, 10+0*25,  80-1, 10+1*25-1), 0, "Use Text:", new BMessage( 't'), 0, B_WILL_DRAW|B_NAVIGABLE|B_NAVIGABLE_JUMP);
	rb1 = new BRadioButton( BRect( 10, 10+1*25, 250-1, 10+2*25-1), 0, "Use Selection", new BMessage( 's'), 0, B_WILL_DRAW|B_NAVIGABLE|B_NAVIGABLE_JUMP);
	tc = new BTextControl( BRect( 80, 10+0*25-1, 210-10-1, 10+1*25-1-1), 0, 0, 0, 0);
	cb0 = new BCheckBox( BRect( 10, 70+0*25, 210-10, 70+1*25-1), 0, "Search Forward", 0);
	cb1 = new BCheckBox( BRect( 10, 70+1*25, 210-10, 70+2*25-1), 0, "Match Case", 0);
	cb2 = new BCheckBox( BRect( 10, 70+2*25, 210-10, 70+3*25-1), 0, "Match Full Word", 0);
	cb3 = new BCheckBox( BRect( 10, 70+3*25, 210-10, 70+4*25-1), 0, "Use Regular Expression", 0);
	BButton *b = new BButton( BRect( 210/2-35, 225-35, 210/2+35, 225-10), 0, "Find", new BMessage( 'fnd'));
	v->AddChild( tc);
	v->AddChild( rb0);
	v->AddChild( rb1);
	v->AddChild( new H( BPoint( 5, 70-10), 210-5-5));
	v->AddChild( cb0);
	v->AddChild( cb1);
	v->AddChild( cb2);
	v->AddChild( cb3);
	v->AddChild( b);
	AddChild( v);
	rb0->SetValue( TRUE);
	tc->TextView( )->SetFontAndColor( be_fixed_font);
	b->MakeDefault( TRUE);
	AddShortcut( '[', 0, new BMessage( 'fnd-'));
	AddShortcut( ']', 0, new BMessage( 'fnd+'));
	ResizeTo( r.Width( ), r.Height( ));
	MoveTo( org);
	Run( );
	Establish( this, this, "WF");
}


void
WF::MessageReceived( BMessage *m)
{
	bool	f;

	switch (m->what) {
	case 'find':
		if (IsHidden( ))
			Show( );
		Activate( TRUE);
		tc->MakeFocus( tc->IsEnabled( ));
		return;
	case 't':
		cb3->SetEnabled( TRUE);
		tc->SetFlags( tc->Flags( ) | B_NAVIGABLE);
		tc->SetEnabled( TRUE);
		tc->MakeFocus( tc->IsEnabled( ));
		return;
	case 's':
		cb3->SetEnabled( FALSE);
		tc->SetFlags( tc->Flags( ) & ~B_NAVIGABLE);
		tc->SetEnabled( FALSE);
		tc->MakeFocus( tc->IsEnabled( ));
		return;
	case 'fnd-':
		f = FALSE;
		break;
	case 'fnd+':
		f = TRUE;
		break;
	case 'fnd':
		f = cb0->Value( );
		break;
	default:
		BWindow::MessageReceived( m);
		return;
	}
	m = new BMessage( 'find');
	if (rb0->Value( ))
		m->AddString( "pattern", tc->Text( ));
	m->AddBool( "forw", f);
	m->AddBool( "fold", not cb1->Value( ));
	m->AddBool( "word", cb2->Value( ));
	m->AddBool( "regex", cb3->IsEnabled( )? cb3->Value( ): FALSE);
	*this >> m;
	unless (IsHidden( ))
		Hide( );
}


bool
WF::QuitRequested( )
{

	Hide( );
	return (FALSE);
}


WF::H::H( BPoint o, uint l):
BView( BRect( o.x, o.y, o.x+l, o.y+1), 0, 0, B_WILL_DRAW)
{

	len = l;
}


void
WF::H::Draw( BRect r)
{

	SetHighColor( 184, 184, 184);
	StrokeLine( BPoint( 0, 0), BPoint( len, 0));
	SetHighColor( 239, 239, 239);
	StrokeLine( BPoint( 0, 1), BPoint( len+1, 1));
}


EditMenu::EditMenu( RNode n):
RNode( n), BMenu( "Edit")
{

}


void
EditMenu::AttachedToWindow( )
{

	BMenu::AttachedToWindow( );
	Establish( this, Looper( ), "EditMenu");
	*this >> 'edit';
}


void
EditMenu::MessageReceived( BMessage *m)
{

}


FontMenu::FontMenu( RNode n):
BMenu( "Font"), RNode( n)
{

	initialized = FALSE;
}


void
FontMenu::AttachedToWindow( )
{
	font_family	ff;
	font_style	fs;
	BFont		f;
	uint32		flags;
	char		s[B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1];

	SetRadioMode( TRUE);
	unless (initialized)
		for (uint i=0; i < count_font_families( ); ++i)
			if ((get_font_family( i, &ff, &flags) == B_OK)
			and (flags & B_IS_FIXED))
				for (uint j=0; j < count_font_styles( ff); ++j)
					if (get_font_style( ff, j, &fs, &flags) == B_OK) {
						strcat( strcat( strcpy( s, ff), "/"), fs);
						BMessage *m = new BMessage( 'fnam');
						m->AddString( "", s);
						BMenuItem *mi = new BMenuItem( label( s), m);
						AddItem( mi);
					}
	BMenu::AttachedToWindow( );
	initialized = TRUE;
	Establish( this, Window( ), "FontMenu");
}


void
FontMenu::DetachedFromWindow( )
{

	Establish( 0);
}


void
FontMenu::MessageReceived( BMessage *m)
{
	const char	*s;

	//printf( "FontMenu::MessageReceived: what='%.4s'\n", &m->what);
	if ((m->what == 'fnam')
	and (s = m->FindString( ""))
	and (s = label( s)))
		for (uint i=0; i<CountItems( ); ++i)
			if (BMenuItem *mi = ItemAt( i))
				if (streq( s, mi->Label( )))
					mi->SetMarked( TRUE);
}


char	*
FontMenu::label( const char *fn)
{
	static char	s[B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1];

	strcpy( s, fn);
	if (char *p = strchr( s, '/'))
		if (streq( p+1, "Regular"))
			*p = 0;
		else
			*p = ' ';
	return (s);
}


EncodingMenu::EncodingMenu( RNode n):
BMenu( "Font Encoding"), RNode( n)
{

	initialized = FALSE;
}


void
EncodingMenu::AttachedToWindow( )
{
	font_family	ff;
	font_style	fs;
	BFont		f;
	uint32		flags;
	char		s[B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1];
	static struct {
		char	*s;
		int8	e;
	} itab[] = {
		"UTF8",		B_UNICODE_UTF8,
		"ISO Latin 1",	B_ISO_8859_1,
		"ISO Latin 2",	B_ISO_8859_2,
		"ISO Latin 3",	B_ISO_8859_3,
		"ISO Latin 4",	B_ISO_8859_4,
		"ISO Latin 5",	B_ISO_8859_5,
		"ISO Latin 6",	B_ISO_8859_6,
		"ISO Latin 7",	B_ISO_8859_7,
		"ISO Latin 8",	B_ISO_8859_8,
		"ISO Latin 9",	B_ISO_8859_9,
		"ISO Latin 10",	B_ISO_8859_10,
		"Macintosh",	B_MACINTOSH_ROMAN
	};

	SetRadioMode( TRUE);
	unless (initialized)
		for (uint i=0; i<nel( itab); ++i) {
			BMessage *m = new BMessage( 'fenc');
			m->AddInt8( "", itab[i].e);
			BMenuItem *mi = new BMenuItem( itab[i].s, m);
			AddItem( mi);
		}
	BMenu::AttachedToWindow( );
	initialized = TRUE;
	Establish( this, Window( ), "EncodingMenu");
}


void
EncodingMenu::DetachedFromWindow( )
{

	Establish( 0);
}


void
EncodingMenu::MessageReceived( BMessage *m)
{
	BMenuItem	*mi;
	int8		u,
			v;

	if ((m->what == 'fenc')
	and (m->FindInt8( "", &v) == B_OK))
		for (uint i=0; i<CountItems( ); ++i)
			if ((mi = ItemAt( i))
			and (mi->Message( )->FindInt8( "", &u) == B_OK)
			and (u == v))
				mi->SetMarked( TRUE);
}


Border::Border( RNode n, BRect r, BView *v):
node( n), BView( r, 0, B_FOLLOW_ALL, B_WILL_DRAW)
{

	AddChild( v);
}


void
Border::AttachedToWindow( )
{

	node.SetDestination( node);
	node.Establish( this, Looper( ), "Border");
}


void
Border::MessageReceived( BMessage *m)
{
	if (m->what == 'bCol')
	{
		rgb_color* bg;
		ssize_t size;
		status_t err = m->FindData("bg", B_RGB_COLOR_TYPE, (const void**)(&bg), &size);
		if (!err)
		{
			SetViewColor(*bg);
			Invalidate();
		}
	}
	else BView::MessageReceived(m);
}


WCP::CC::CC( RNode n1, RNode n2, BTermView::Pref *pref, BPoint org):
RNode( n1), view( n2), BColorControl( org, B_CELLS_32x8, 12, ""), BTermView::Pref( *pref)
{

	key = 'db';
	color = background;
	BColorControl::SetValue( color);
}


void
WCP::CC::AttachedToWindow( )
{

	BColorControl::AttachedToWindow( );
	Establish( this, Window( ), "WCP::CC");
}


void
WCP::CC::SetValue( int32 v)
{
	BColorControl::SetValue( v);
	rgb_color c = ValueAsColor( );
	unless (c == color) {
		switch (key) {
		case 'df':
			foreground = c;
			break;
		case 'db':
			background = c;
			break;
		case 'cf':
			cursorfg = c;
			break;
		case 'cb':
			cursorbg = c;
			break;
		case 'sf':
			selfg = c;
			break;
		case 'sb':
			selbg = c;
		}
		BMessage *m = new BMessage( 'pref');
		m->AddInt32( "BTermView", key);
		m->AddData( "", B_UINT8_TYPE, (BTermView::Pref *)this, sizeof( BTermView::Pref));
		view >> m;
		color = c;
	}
}


void
WCP::CC::MessageReceived( BMessage *m)
{

	switch (m->what) {
	default:
		BColorControl::MessageReceived( m);
		return;
	case 'db':
		color = background;
		break;
	case 'df':
		color = foreground;
		break;
	case 'cb':
		color = cursorbg;
		break;
	case 'cf':
		color = cursorfg;
		break;
	case 'sb':
		color = selbg;
		break;
	case 'sf':
		color = selfg;
	}
	key = m->what;
	BColorControl::SetValue( color);
}


WCP::ColorPref::ColorPref( RNode n, char *t, BPoint org, BTermView::Pref *p):
BWindow( BRect( 0, 0, 480, 150), t, B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_WILL_ACCEPT_FIRST_CLICK)
{
	const struct {
		char	*label;
		uint	key;
	} s[] = {
		"text background", 'db',
		"text foreground", 'df',
		"cursor background", 'cb',
		"cursor foreground", 'cf',
		"selection background", 'sb',
		"selection foreground", 'sf'
	};

	BView *v = new BView( Bounds( ), 0, 0, 0);
	Run( );
	MoveTo( org);
	v->AddChild( new CC( *this, n, p, BPoint( 10, 40)));
	BPopUpMenu *m = new BPopUpMenu( "");
	for (uint i=0; i<nel( s); ++i) {
		BMenuItem *mi = new BMenuItem( s[i].label, new BMessage( s[i].key));
		m->AddItem( mi);
		unless (i)
			mi->SetMarked( TRUE);
	}
	BMenuBar *mb = new BMenuBar( BRect( 85, 10, 200, 150), 0, 0, B_ITEMS_IN_COLUMN);
	mb->AddItem( m);
	v->AddChild( mb);
	v->AddChild( new S( BRect( 10, 10, 85, 24), "Set color of"));
	v->SetViewColor( 216, 216, 216);
	AddChild( v);
	Show( );
	SetDestination( *this);
}


void
WCP::ColorPref::MessageReceived( BMessage *m)
{

	//printf( "WCP::ColorPref::MessageReceived: what=0x%08X'\n", m->what);
	*this >> DetachCurrentMessage( );
}


WCP::S::S( BRect r, char *s):
BStringView( r, 0, s)
{

}


void
WCP::S::AttachedToWindow( )
{

	BStringView::AttachedToWindow( );
}


static geom
getgeom( BPoint p)
{
	geom	g;

	uint h = menuheight( );
	g.termview = BRect( 0, 0, p.x-1, p.y-1);
	g.termview.OffsetBy( 3, 3);
	g.border = g.termview;
	g.border.InsetBy( -3, -3);
	g.border.OffsetBy( 0, h);
	g.scrollbar.left = g.border.right + 1;
	g.scrollbar.top = g.border.top - 1;
	g.scrollbar.right = g.scrollbar.left + B_V_SCROLL_BAR_WIDTH;
	g.scrollbar.bottom = g.border.bottom - B_H_SCROLL_BAR_HEIGHT + 1;
	g.menu.left = g.border.left;
	g.menu.top = g.border.top - h;
	g.menu.right = g.scrollbar.right;
	g.menu.bottom = g.border.top - 1;
	g.window = g.menu;
	--g.window.right;
	g.window.bottom = g.border.bottom;
	return (g);
}


static uint
menuheight( )
{
	static uint	h;

	unless (h) {
		BWindow *w = new BWindow( BRect( ), 0, B_TITLED_WINDOW, 0);
		BMenuBar *z = new BMenuBar( BRect( ), 0);
		z->AddItem( new BMenuItem( "", 0));
		w->Lock( );
		w->AddChild( z);
		h = z->Bounds( ).bottom + 1;
		w->Unlock( );
		w->Quit( );
	}
	return (h);
}


static void
inform( char *mesg, ...)
{
	va_list	ap;
	char	s[999];

	va_start( ap, mesg);
	vsprintf( s, mesg, ap);
	BAlert *a = new BAlert( "", s, "Okay", 0, 0, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	a->Go( );
}


static void
warn( char *mesg, ...)
{
	va_list	ap;
	char	s[999];

	va_start( ap, mesg);
	vsprintf( s, mesg, ap);
	BAlert *a = new BAlert( "", s, "Okay", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	a->Go( );
}


uint
narg( char **av)
{

	uint n = 0;
	while (*av++)
		++n;
	return (n);
}


static void
squish( uchar *s)
{

	uchar *d = s;
	while (uint c = *s++)
		unless (c == ' ')
			*d++ = isupper( c)? tolower( c): c;
}


static rgb_color
colorconv( char *s)
{
	rgb_color	c;
	uint		r,
			g,
			b;
	char		line[100],
			s2[sizeof line];

	memset( &c, 0, sizeof c);
	if (*s == '#') {
		r = 0;
		g = 0;
		b = 0;
		sscanf( s+1, "%2x%2x%2x", &r, &g, &b);
		c.red = r;
		c.green = g;
		c.blue = b;
	}
	else if (FILE *f = fopen( "/etc/rgb.txt", "r")) {
		squish( (uchar *)s);
		while (fgets( line, sizeof line, f)) {
			if (sscanf( line, "%d %d %d %[^\n]", &r, &g, &b, s2) == 4) {
				squish( (uchar *)s2);
				if (streq( s, s2)) {
					c.red = r;
					c.green = g;
					c.blue = b;
					break;
				}
			}
		}
		fclose( f);
	}
	return (c);
}
