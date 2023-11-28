

#include	<Application.h>
#include	<Alert.h>
#include	<Bitmap.h>
#include	<Button.h>
#include	<Clipboard.h>
#include	<ScrollBar.h>
#include	<MenuBar.h>
#include	<MenuItem.h>
#include	<StringView.h>
#include	<Screen.h>
#include	<ColorControl.h>
#include	<FilePanel.h>
#include	<Box.h>
#include	<Directory.h>
#include	<FindDirectory.h>
#include	<TextView.h>
#include	"RNode.h"
#include	<Path.h>
#include	<NodeInfo.h>
#include	<PopUpMenu.h>
#include	<Roster.h>
#include	<Resources.h>
#include	<Mime.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<string.h>
#include 	<time.h>
#include	"ttydev.h"
#include	"RNodeHandler.h"
#include	"Shell.h"
#include	<termio.h>
#include	"XString.h"
#include	"Termio.h"
//#include	"rico.h"
#include	"TermView.h"
#include	"Timer.h"

/** prototypes **/
//static void get_current_time(char *);     

/** added by RB **/
//FILE * logfile;

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
	uint		baud,
			parity,
			data,
			stop,
			flow;
	char		dialcom[100],
			device[20];
	BTermView::Pref	termview;
	enum {
			MAGIC		= 0xF1F2F3F4,
			VERSION		= 1
	};
};
struct Border: BView {
		Border( RNode, BRect);
	private:
	void	AttachedToWindow( ),
		MessageReceived( BMessage *);
	RNode	node;
};
struct A: BApplication, RNode {
		A( );
	private:
	void	ReadyToRun( ),
		identify( ),
		kickmime( type_code, uint, icon_size),
		RefsReceived( BMessage *),
		MessageReceived( BMessage *);
	bool	loadpref( const char *);
	char	*title( ),
		*preffile;
	uint	id;
};
struct W {
	struct V {
		public:
		virtual void	viewhandler( BMessage *)	= 0;
		RNode		view;
	};
	class View: public BHandler, public RNode, public virtual V {
		public:
			View( BLooper *);
		void	MessageReceived( BMessage *);
	};
	class Win: public BWindow, public RNode, public virtual V {
		public:
				Win( RNode, geom, char *);
		void		MessageReceived( BMessage *),
				gotonext( ),
				sendparams( ),
				sendstr( char *s),
				viewhandler( BMessage *),
				updatemenus( BTermView::Pref * = 0),
				savepref( const char *, BMessage *, BPoint);
		char		dialcom[100],
				device[20],
				init[100],
				telno[100],
				login[20],
				passw[20];
		uint		baud,
				parity,
				data,
				stop,
				flow;
		RNode		edit,
				border;
		public:
		static char defaultdial[],
			defaultinit[];
	};
	struct Obj: public Win, public View {
			Obj( RNode, geom, char *);
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
struct S: BStringView {
		S( BRect, char *);
	private:
	void	AttachedToWindow( );
};
struct WFP {
	struct Obj: BFilePanel, RNode {
			Obj( RNode n): RNode( n), BFilePanel( B_SAVE_PANEL) {
				Show( );
			}
		void SendMessage( const BMessenger *, BMessage *m) {
			entry_ref r;
			if (m->FindRef( "refs", 0, &r) == B_OK) {
				BEntry e( &r);
				BPath p;

				e.GetPath(&p);
				m = new BMessage( 'SAVE');
				m->AddString( "file", p.Path());
				*this >> m;
			}
			else if (m->FindRef( "directory", 0, &r) == B_OK) {
				BEntry e( &r);
				BPath p;
				char path[MAXPATHLEN];

				e.GetPath(&p);
				strcpy(path, p.Path());
				strcat( strcat( path, "/"), m->FindString( "name"));
				m = new BMessage( 'SAVE');
				m->AddString( "file", path);
				*this >> m;
			}
		}
	};
};
struct WCP {
	struct BG: BView {
			BG( );
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
struct Dev: BMenuBar {
			Dev( BPoint, char *);
	static char	_defaultdev[];
	static char	*defaultdev();
};
struct T: BBox {
	struct T2: BTextView {
			T2( BRect, char *, uint);
		private:
		void	KeyDown( const char *, int32);
		uint	mesg;
	};
		T( BRect, char *, uint);
};
struct C: BWindow, RNode {
	struct BG: BView {
			BG( char *);
		private:
		void	AttachedToWindow( );
	};
		C( RNode, BPoint, char *, char *);
	private:
	void	MessageReceived( BMessage *);
	char	dev[20];
};
struct D: BWindow, RNode {
	struct BG: BView {
			BG( D *);
		private:
		void	AttachedToWindow( );
	};
		D( RNode, BPoint, const char *, const char *, const char *, const char *, const char *, const char *, const char *);
	private:
	void	MessageReceived( BMessage *);
	char	com[100],
		init[100],
		telno[100],
		login[20],
		passw[20],
		dev[20];
	friend	struct D::BG;
};
struct EditMenu: BMenu, RNode {
		EditMenu( RNode);
	private:
	void	AttachedToWindow( );
	void	MessageReceived( BMessage *);
};
struct FontMenu: BMenu {
		FontMenu( );
	private:
	void	AttachedToWindow( );
	bool	monospace( char *);
	bool	initialized;
};


char	*Dev::defaultdev()
{
	DIR *dd;
	struct dirent *d;

	if (_defaultdev[0]) {
		return (_defaultdev);
	}
	sprintf(_defaultdev, "/dev/ports/serial1");
//	fprintf(logfile, "/dev/ports/serial1");
	dd = opendir("/dev/ports");
	if (dd == NULL) {
		return (_defaultdev);
	}
	while (d = readdir(dd)) {
		if (d->d_name[0] == '.') {
			continue;
		}
		if (strncmp(d->d_name, "com", 3) == 0) {
			/*
			 * HACK HACK HACK 
			 * Don't let com3/com4 ports be default for BeBox
			 */
			continue;
		}
		strcpy(_defaultdev, d->d_name);
		break;
	}
	closedir(dd);
	return (_defaultdev);
}

char	Dev::_defaultdev[64],
	W::Win::defaultdial[]	= "connect/dial-o-rama",
	W::Win::defaultinit[]	= "ATE1Q0X4";

static geom	getgeom( BPoint);
static uint	menuheight( );
static void	warn( char *, ...);
static char	*strncopy( char *, const char *, uint);

static char	APPSIG[]	= "application/x-vnd.Be-TERM";
static char	PREFSIG[]	= "application/x-vnd.Be-prf2";
bool		commandIsMeta;


int
main( )
{
	static char *av[] = {
		"parrot", "none", 0
	};
	extern char	**Argv;
	extern int	Argc;
	A		a;
	BPath	path;
        char log_time[15];
        char full_log_path[30];

	find_directory (B_BEOS_BIN_DIRECTORY, &path);
	path.Append (av[0]);
	av[0] = const_cast<char *>(path.Path());

	Argc = (sizeof( av) / sizeof( (av)[0])) -1;  //nel( av) - 1;
	Argv = av;
        
        /** added my RB **/
//        get_current_time( log_time );
//        strcat (full_log_path, "./logs/");
//        strcat (full_log_path, log_time);

        // open file here
//printf("%s",full_log_path);
//        logfile = fopen(full_log_path, "w+");
//        if (logfile == NULL) 
//           printf ("unable to open log file!\n");
        
	a.Run( );
	//RNode::DumpTable( );

//        fclose(logfile);

	return (0);
}


A::A( ):
BApplication( APPSIG)
{

	preffile = 0;
	be_clipboard->Lock( );
	identify( );
	be_clipboard->Unlock( );
	if (id == 1) {
		kickmime( 'ICON', 32, B_LARGE_ICON);
		kickmime( 'MICN', 16, B_MINI_ICON);
	}
}


void
A::ReadyToRun( )
{
	system_info	si;
	RNode		n1;

	get_system_info( &si);
	if  (!(si.used_sems+167*3/2 < si.max_sems)  // unless (exp)
	&& (si.used_ports+14*3/2 < si.max_ports)
	&& (si.used_threads+8*3/2 < si.max_threads)
	&& (si.used_teams+2*3/2 < si.max_teams)) {
		char MESG[] = "There are insufficient system resources to run this copy of SerialConnect.";
		BAlert *a = new BAlert( "", MESG, "Oh, well.", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		a->Go( );
		Quit( );
		return;
	}
	geom g = getgeom( BPoint( 80*7, 34*11));
	g.window.OffsetBy( -10+id*16, 9+id*16);
	SetDestination( n1);
	n1.SetDestination( *this);
	new W::Obj( n1, g, title( ));
	Establish( 0, this, "A");
	BPath path;
	find_directory( B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append( "SerialConnect");
				
	loadpref( preffile? preffile: path.Path( ));
	*this >> 'sync';
}


void
A::identify( )
{
	const char	PNAME[]	= "ConnectNames";
	team_id		r[40];

	id = 0;
	port_id p = find_port( PNAME);
	if (p < B_NO_ERROR) {
		p = create_port( 1, PNAME);
		if (p < B_NO_ERROR)
			return;
		set_port_owner( p, 1);
		memset( r, 0, sizeof r);
	}
	else {
		long _;
		read_port( p, &_, r, sizeof r);
	}
	for (uint i=0; i< ((sizeof( r) / sizeof( (r)[0]))); ++i) {
		team_info ti;
		if ((! r[i])
		|| (get_team_info( r[i], &ti) < B_NO_ERROR)) {
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
	&& (r = AppResources( ))
	&& (d = r->LoadResource( t, 2, &dsize))
	&& (dsize == n*n)) {
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
	BPath		p;

	if (m->GetInfo("refs", &type, &count) != B_NO_ERROR)
		return;

	for(i=0; i < count; i++) {
		if (m->FindRef("refs", i, &ref) != B_NO_ERROR)
			continue;

		if (entry.SetTo(&ref) != B_NO_ERROR)
			continue;

		if (entry.GetPath(&p) < B_NO_ERROR)
			continue;

                preffile = strdup( p.Path( ));
                break;
	}
}


bool
A::loadpref( const char *path)
{
	Pref	pref;

	if (path) {
		int fd = open( path, O_RDONLY);
		if (fd < 0)
			return (FALSE);
		if  (!(read( fd, &pref, sizeof pref) == sizeof pref)  //unless
		&& (pref.magic == Pref::MAGIC))
			warn( "%s is not a SerialConnect preference file.", path);
		else if (!(pref.version == Pref::VERSION))
			warn( "SerialConnect preference file %s is out-of-date.", path);
		else {
			BMessage *m = new BMessage( 'pref');
			m->AddData( "", B_UINT8_TYPE, &pref, sizeof pref);
			*this >> m;
		}
		close( fd);
	}
	return (TRUE);
}


void
A::MessageReceived( BMessage *m)
{
	BWindow*	win;

	printf( "A::MessageReceived: what=0x%X\n", m->what);
	switch (m->what) {
	case 'acti':
		if (win = WindowAt(0))
			win->PostMessage('acti');
		break;
	case EOF:
		Quit( );
	}
}


char	*
A::title( )
{

	if (id) {
		char s[99];
		sprintf( s, "SerialConnect %d", id);
//		fprintf( logfile, "SerialConnect %d", id);
		if (char *t = strdup( s))
			return (t);
	}
	return ("SerialConnect");
}


W::Obj::Obj( RNode n, geom g, char *t):
Win( n, g, t), View( this)
{

	View &v = *this;
	v.Establish( &v, this, "W::View");
}


W::Win::Win( RNode n1, geom g, char *b):
RNode( n1), BWindow( g.window, b, B_DOCUMENT_WINDOW, 0)
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
		n23,
		n24,
		n25;
	BPath path;

	Run( );

	find_directory (B_BEOS_ETC_DIRECTORY, &path);
	path.Append (defaultdial);
	strncopy( dialcom, path.Path(), sizeof dialcom);
	strncopy( device, Dev::defaultdev(), sizeof device);
	strncopy( init, defaultinit, sizeof init);
	strcpy( telno, "");
	strcpy( login, "");
	strcpy( passw, "");
	baud = 19200;
	parity = 'parN';
	data = 'dat8';
	stop = 'sto1';
	flow = 'floH';

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
	edit.SetDestination( n25);

	new BTermio( n4, n5, n6, n7, n10, n23);
	new BShell::Obj( n8, n9, n11);
	AddChild( new BTermView( n2, n3, n20, n13, n24, g.termview));
	AddChild( new SB( n21, 0, g.scrollbar));
	AddChild( new Border( border, g.border));

	BMenuBar *mb = new BMenuBar( g.menu, "");
	BMenu *m06 = new BMenu( "Connections");
	BMenu *m07 = new EditMenu( n25);
	BMenu *m08 = new BMenu( "Settings");
	BMenu *m01 = new BMenu( "Window Size");
	BMenu *m02 = new BMenu( "Tab Width");
	BMenu *m04 = new FontMenu( );
	BMenu *m05 = new BMenu( "Font Size");
	BMenu *m03 = new BMenu( "Parity");
	BMenu *m09 = new BMenu( "Data Bits");
	BMenu *m11 = new BMenu( "Stop Bits");
	BMenu *m10 = new BMenu( "Baud Rate");
	BMenu *m12 = new BMenu( "Flow Control");
	m02->SetRadioMode( TRUE);
	m05->SetRadioMode( TRUE);
	m03->SetRadioMode( TRUE);
	m09->SetRadioMode( TRUE);
	m11->SetRadioMode( TRUE);
	m10->SetRadioMode( TRUE);
	m12->SetRadioMode( TRUE);
	m06->AddItem( new BMenuItem( "Connect Directly...", new BMessage( 'conn'), 'D'));
	m06->AddItem( new BMenuItem( "Connect via Modem...", new BMessage( 'dial'), 'M'));
	m06->AddItem( new BMenuItem( "Disconnect", new BMessage( 'disc'), 'Z'));
	m07->AddItem( new BMenuItem( "Copy", new BMessage( 'copy'), 'C'));
	m07->AddItem( new BMenuItem( "Paste", new BMessage( 'pste'), 'V'));
	m07->AddSeparatorItem( );
	m07->AddItem( new BMenuItem( "Select All", new BMessage( 'sela'), 'A'));
	m07->AddItem( new BMenuItem( "Clear All", new BMessage( 'clrh'), 'L'));
	m08->AddItem( m01);
	m08->AddItem( m04);
	m08->AddItem( m05);
	m08->AddItem( m02);
	m08->AddItem( new BMenuItem( "Color...", new BMessage( 'colr')));
	m08->AddSeparatorItem( );
	m08->AddItem( m03);
	m08->AddItem( m09);
	m08->AddItem( m11);
	m08->AddItem( m10);
	m08->AddItem( m12);
	m08->AddSeparatorItem( );
	m08->AddItem( new BMenuItem( "Save as Defaults", new BMessage( 'save')));
	m08->AddItem( new BMenuItem( "Save as Settings File...", new BMessage( 'Save')));
	m01->AddItem( new BMenuItem( "80x24", new BMessage( 80*24)));
	m01->AddItem( new BMenuItem( "80x25", new BMessage( 80*25)));
	m01->AddItem( new BMenuItem( "132x24", new BMessage( 132*24)));
	m01->AddItem( new BMenuItem( "132x25", new BMessage( 132*25)));
	m02->AddItem( new BMenuItem( "1", new BMessage( 'tab1')));
	m02->AddItem( new BMenuItem( "2", new BMessage( 'tab2')));
	m02->AddItem( new BMenuItem( "4", new BMessage( 'tab4')));
	m02->AddItem( new BMenuItem( "8", new BMessage( 'tab8')));
	m03->AddItem( new BMenuItem( "None", new BMessage( 'parN')));
	m03->AddItem( new BMenuItem( "Even", new BMessage( 'parE')));
	m03->AddItem( new BMenuItem( "Odd", new BMessage( 'parO')));
	m09->AddItem( new BMenuItem( "8", new BMessage( 'dat8')));
	m09->AddItem( new BMenuItem( "7", new BMessage( 'dat7')));
	m11->AddItem( new BMenuItem( "1", new BMessage( 'sto1')));
	m11->AddItem( new BMenuItem( "2", new BMessage( 'sto2')));
	m12->AddItem( new BMenuItem( "RTS/CTS", new BMessage( 'floH')));
	m12->AddItem( new BMenuItem( "None", new BMessage( 'floN')));

	char *const rates[] = { "115200", "57600", "38400", "19200", "9600", "4800", "2400", "1800", "1200", "600", "300", "200", "150", "134", "110", "75", "50" };
	for (uint i=0; i< (sizeof( rates) / sizeof( (rates)[0])); ++i) {  //uint i=0; i<nel( rates); ++i
		BMessage *m = new BMessage( 'baud');
		uint r = atoi( rates[i]);
		m->AddInt32( "", r);
		BMenuItem *mi = new BMenuItem( rates[i], m);
		m10->AddItem( mi);
		if (r == baud)
			mi->SetMarked( TRUE);
	}

	char *const sizes[] = { "5", "10", "15", "20", "30", "40", "60", "120" };
	for (uint i=0; i< (sizeof( sizes) / sizeof( (sizes)[0])); ++i) { // uint i=0; i<nel( sizes); ++i
		BMessage *m = new BMessage( 'fsiz');
		float s = atof( sizes[i]);
		m->AddFloat( "", s);
		BMenuItem *mi = new BMenuItem( sizes[i], m);
		m05->AddItem( mi);
		if (s == 15)
			mi->SetMarked( TRUE);
	}

	mb->AddItem( m06);
	mb->AddItem( m07);
	mb->AddItem( m08);
	AddChild( mb);

	updatemenus( );
	SetSizeLimits( 20, 1e9, 55, 1e9);
	Establish( this, this, "W");
}


void
W::Win::MessageReceived( BMessage *m)
{
	char	buf[1000];
	long	n;
	BPoint	org;

	printf( "W::Win::MessageReceived: what='%.4s'\n", &m->what);
	switch (m->what) {
	case 'baud':
		baud = m->FindInt32( "");
		sendparams( );
		break;
	case 'parN':
	case 'parE':
	case 'parO':
		parity = m->what;
		sendparams( );
		break;
	case 'dat8':
	case 'dat7':
		data = m->what;
		sendparams( );
		break;
	case 'sto1':
	case 'sto2':
		stop = m->what;
		sendparams( );
		break;
	case 'floN':
	case 'floH':
		flow = m->what;
		sendparams( );
		break;
	case 'pref':
		Pref *pref;
		ssize_t _;
		m->FindData( "", B_UINT8_TYPE, (const void **)&pref, &_);
		m = new BMessage( 'pref');
		m->AddData( "", B_UINT8_TYPE, &pref->termview, sizeof pref->termview);
		m->AddInt32( "BTermView", 'init');
		view >> m;
		if (! (pref->base == BPoint( 0, 0)))
			MoveTo( pref->base);
		strncopy( dialcom, pref->dialcom, sizeof dialcom);
		strncopy( device, pref->device, sizeof device);
		baud = pref->baud;
		parity = pref->parity;
		data = pref->data;
		stop = pref->stop;
		flow = pref->flow;
		updatemenus( &pref->termview);
		break;
	case 'next':
		gotonext( );
		break;
	case 'conn':
		org = Frame( ).LeftTop( ) + BPoint( 70, 60);
		{
			RNode n;
			n.SetDestination( *this);
			new C( n, org, "Connect Directly", device);
		}
		break;
	case 'dial':
		org = Frame( ).LeftTop( ) + BPoint( 70, 60);
		{
			RNode n;
			n.SetDestination( *this);
			new D( n, org, "Connect via Modem", dialcom, device, init, telno, login, passw);
		}
		break;
	case 'disc':
		m = new BMessage( 'data');
		m->AddData( "", B_UINT8_TYPE, "\1:z\n", 4);
		view >> m;
		break;
	case 'CONN':
		strncopy( device, m->FindString( "dev"), sizeof device);
		sendparams( );
		sprintf( buf, "\1:c%s\n", device);
//		fprintf( logfile, "\1:c%s\n", device);
		sendstr( buf);
		break;
	case 'DIAL':
		strncopy( dialcom, m->FindString( "com"), sizeof dialcom);
		strncopy( device, m->FindString( "dev"), sizeof device);
		strncopy( init, m->FindString( "init"), sizeof init);
		strncopy( telno, m->FindString( "telno"), sizeof telno);
		strncopy( login, m->FindString( "login"), sizeof login);
		strncopy( passw, m->FindString( "passw"), sizeof passw);
		sendparams( );
		sprintf( buf, "\1:d%s\1%s\1%s\1%s\1%s\1%s\n", device, dialcom, init, telno, login, passw);
//		fprintf( logfile, "\1:d%s\1%s\1%s\1%s\1%s\1%s\n", device, dialcom, init, telno, login, passw);
		sendstr( buf);
		break;
	case 'colr':
	case 'save': {
		uint w = m->what;
		m = new BMessage( 'pref');
		m->AddInt32( "BTermView", 'get');
		m->AddInt32( "connect", w);
		view >> m;
		break;
		}
	case 'Save':
		{
			RNode n;
			n.SetDestination( *this);
			new WFP::Obj( n);
		}
		break;
	case 'SAVE':
		DetachCurrentMessage( );
		m->what = 'pref';
		m->AddInt32( "BTermView", 'get');
		m->AddInt32( "connect", 'SAVE');
		view >> m;
		break;
	case 'fnam':
		if  (!(m->FindBool( "monospace"))) {
			const char MESG[] = "This is a proportional font, and will appear peculiar when used with SerialConnect.";
			BAlert *a = new BAlert( "", MESG, "Fascinating");
			a->Go( );
		}
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
	if (!(n < 2))
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
W::Win::sendparams( )
{

	switch (parity) {
	case 'parN':
		sendstr( "\1:N\n");
		break;
	case 'parE':
		sendstr( "\1:E\n");
		break;
	default:
		sendstr( "\1:O\n");
	}
	switch (data) {
	case 'dat8':
		sendstr( "\1:8\n");
		break;
	default:
		sendstr( "\1:7\n");
	}
	switch (stop) {
	case 'sto1':
		sendstr( "\1:1\n");
		break;
	default:
		sendstr( "\1:2\n");
	}
	switch (flow) {
	case 'floN':
		sendstr( "\1:n\n");
		break;
	default:
		sendstr( "\1:h\n");
	}
	char s[20];
	sprintf( s, "\1:b%d\n", baud);
//	fprintf( logfile, "\1:b%d\n", baud);
	sendstr( s);
}


void
W::Win::sendstr( char *s)
{

	BMessage *m = new BMessage( 'data');
	m->AddData( "", B_UINT8_TYPE, s, strlen( s));
//added RB 
printf ("string = %s\n");
	view >> m;
}


void
W::Win::viewhandler( BMessage *m)
{

	printf( "W::Win::viewhandler: what='%.4s'\n", &m->what);
	switch (m->what) {
	case EOF:
		Quit( );
		break;
	case 'size': 
	{
		geom g = getgeom( m->FindPoint( ""));
		ResizeTo( g.window.right, g.window.bottom);
		break;
	}
	case 'edit':
		edit >> DetachCurrentMessage( );
		break;
	case 'pref':
		switch (m->FindInt32( "connect")) {
		case 'colr':
		{
			char s[99];
			long n;
			sprintf( s, "Colors for %s", Title( ));
//			fprintf( logfile, "Colors for %s", Title( ));
			const void *p;
			m->FindData( "", B_UINT8_TYPE, &p, &n);
			BPoint org = Frame( ).LeftTop( ) + BPoint( 70, 60);
			new WCP::ColorPref( view, s, org, (BTermView::Pref *)p);
			break;
		}
		case 'save':
		{
			BPath path;
			find_directory( B_USER_SETTINGS_DIRECTORY, &path, true);
			path.Append( "SerialConnect");
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
	default:
		border >> m->what;
	}
}


void
W::Win::updatemenus( BTermView::Pref *t)
{
	BMenuBar	*mb;
	BMenu		*m1,
			*m2;
	BMenuItem	*i1,
			*i2,
			*i3;
	char		s[20];

	if ((mb = KeyMenuBar( ))
	&& (i1 = mb->FindItem( "Settings"))
	&& (m1 = i1->Submenu( ))) {
		if ((i2 = m1->FindItem( "Parity"))
		&& (m2 = i2->Submenu( ))
		&& (i3 = m2->FindItem( parity)))
			i3->SetMarked( TRUE);
		if ((i2 = m1->FindItem( "Data Bits"))
		&& (m2 = i2->Submenu( ))
		&& (i3 = m2->FindItem( data)))
			i3->SetMarked( TRUE);
		if ((i2 = m1->FindItem( "Stop Bits"))
		&& (m2 = i2->Submenu( ))
		&& (i3 = m2->FindItem( stop)))
			i3->SetMarked( TRUE);
		if ((i2 = m1->FindItem( "Flow Control"))
		&& (m2 = i2->Submenu( ))
		&& (i3 = m2->FindItem( flow)))
			i3->SetMarked( TRUE);
		sprintf( s, "%d", baud);
//		fprintf( logfile, "%d", baud);
		if ((i2 = m1->FindItem( "Baud Rate"))
		&& (m2 = i2->Submenu( ))
		&& (i3 = m2->FindItem( s)))
			i3->SetMarked( TRUE);
		if (t) {
			sprintf( s, "%d", t->fontsize);
//			fprintf( logfile, "%d", t->fontsize);
			if ((i2 = m1->FindItem( "Font Size"))
			&& (m2 = i2->Submenu( ))
			&& (i3 = m2->FindItem( s)))
				i3->SetMarked( TRUE);
			sprintf( s, "%d", t->tabwidth);
//			fprintf( logfile, "%d", t->tabwidth);
			if ((i2 = m1->FindItem( "Tab Width"))
			&& (m2 = i2->Submenu( ))
			&& (i3 = m2->FindItem( s)))
				i3->SetMarked( TRUE);
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
		long _;
		const void *p;
		m->FindData( "", B_UINT8_TYPE, &p, &_);
		if (p) {
			Pref pref;
			pref.magic = Pref::MAGIC;
			pref.version = Pref::VERSION;
			pref.base = base;
			strncopy( pref.dialcom, dialcom, sizeof pref.dialcom);
			strncopy( pref.device, device, sizeof pref.device);
			pref.baud = baud;
			pref.parity = parity;
			pref.data = data;
			pref.stop = stop;
			pref.flow = flow;
			pref.termview = *(BTermView::Pref *)p;
			write( fd, &pref, sizeof pref);
		}
		close( fd);
		BNode bn( path);
		BNodeInfo ni( &bn);
		ni.SetType( PREFSIG);
	}
}


W::View::View( BLooper *l):
RNode( view)
{

}


void
W::View::MessageReceived( BMessage *m)
{

	printf( "W::View::MessageReceived: what=0x%08X'\n", m->what);
	viewhandler( m);
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
	&& (m->what == B_VALUE_CHANGED)
	&& (! mousedown)) {
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
		if (!mousedown) {
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


FontMenu::FontMenu( ):
BMenu( "Font")
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
	if (!initialized)
		for (uint i=0; i < count_font_families( ); ++i)
			if (get_font_family( i, &ff, &flags) == B_OK)
				for (uint j=0; j < count_font_styles( ff); ++j)
					if (get_font_style( ff, j, &fs, &flags) == B_OK) {
						strcat( strcat( strcpy( s, ff), "/"), fs);
						if (monospace( s)) {
							BMessage *m = new BMessage( 'fnam');
							m->AddString( "", s);
							m->AddBool( "monospace", TRUE);
							strcpy( s, ff);
							if (!(strcmp( fs, "Regular") == 0))
								strcat( strcat( s, " "), fs);
							BMenuItem *mi = new BMenuItem( s, m);
							AddItem( mi);
						}
					}
	BMenu::AttachedToWindow( );
	initialized = TRUE;
}


bool
FontMenu::monospace( char *fn)
{
	char *const MONOFONTS[] = {
		"Courier10 BT/Roman",
		"Courier10 BT/Bold",
		"Courier10 BT/Italic",
		"Courier10 BT/Bold Italic"
	};

	for (uint i=0; i< (sizeof( MONOFONTS) / sizeof( (MONOFONTS)[0])); ++i) //uint i=0; i<nel( MONOFONTS); ++i
		if (strcmp( fn, MONOFONTS[i]) == 0 )
			return (TRUE);
	return (FALSE);
}


Border::Border( RNode n, BRect r):
node( n), BView( r, 0, B_FOLLOW_ALL, B_WILL_DRAW)
{

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

	const color_map *cm = BScreen( Window( )).ColorMap( );
	if (m->what < (sizeof( cm->color_list) / sizeof( (cm->color_list)[0]))) { // m->what < nel( cm->color_list)
		SetViewColor( cm->color_list[m->what]);
		Invalidate( );
	}
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
	BScreen	screen( Window( ));

	BColorControl::SetValue( v);
	rgb_color c = ValueAsColor( );
	if (!(screen.IndexForColor( c) == screen.IndexForColor( color))) {
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
BWindow( BRect( 0, 0, 480, 150), t, B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
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

	Run( );
	MoveTo( org);
	AddChild( new CC( *this, n, p, BPoint( 10, 40)));
	BPopUpMenu *m = new BPopUpMenu( "");
	for (uint i=0; i< (sizeof( s) / sizeof( (s)[0])); ++i) { //uint i=0; i<nel( s); ++i
		BMenuItem *mi = new BMenuItem( s[i].label, new BMessage( s[i].key));
		m->AddItem( mi);
		if (!i)
			mi->SetMarked( TRUE);
	}
	BMenuBar *mb = new BMenuBar( BRect( 85, 10, 200, 150), 0, 0, B_ITEMS_IN_COLUMN);
	mb->AddItem( m);
	AddChild( mb);
	AddChild( new S( BRect( 10, 10, 85, 24), "Set color of"));
	AddChild( new BG);
	Show( );
	SetDestination( *this);
}


void
WCP::ColorPref::MessageReceived( BMessage *m)
{

	printf( "WCP::ColorPref::MessageReceived: what=0x%08X'\n", m->what);
	*this >> DetachCurrentMessage( );
}


S::S( BRect r, char *s):
BStringView( r, 0, s)
{

}


void
S::AttachedToWindow( )
{

#if 1
	SetViewColor( 255, 255, 0);
#else
	SetViewColor( 216, 216, 216);
#endif
	BStringView::AttachedToWindow( );
	//SetFontName( "Emily");
}


WCP::BG::BG( ):
BView( BRect( ), 0, B_FOLLOW_ALL, B_WILL_DRAW)
{

}


void
WCP::BG::AttachedToWindow( )
{

	BRect r = Window( )->Bounds( );
	ResizeTo( r.Width( ), r.Height( ));
	SetViewColor( 216, 216, 216);
}


Dev::Dev( BPoint org, char *init):
BMenuBar( BRect( ), 0, 0, B_ITEMS_IN_COLUMN)
{
#if 1
	MoveTo( org);
	BPopUpMenu *menu = new BPopUpMenu( "");
	if (DIR *dd = opendir("/dev/ports")) {
		while (struct dirent *d = readdir( dd)) {
			if (d->d_name[0] == '.') {
				continue;
			}
			BMessage *m = new BMessage( 'cdev');
			m->AddString( "", d->d_name);
			BMenuItem *mi = new BMenuItem( d->d_name, m);
			menu->AddItem( mi);
			if (strcmp( d->d_name, init) == 0) //streq( d->d_name, init)
				mi->SetMarked( TRUE);
		}
		closedir(dd);
	}
	AddItem( menu);
#else
	static char *s[] = {
		"serial1",
		"serial2",
		"serial3",
		"serial4"
	};
	BPopUpMenu *menu = new BPopUpMenu( "");
	for (uint i=0; i<nel( s); ++i) {
		BMessage *m = new BMessage( 'cdev');
		m->AddString( "", s[i]);
		BMenuItem *mi = new BMenuItem( s[i], m);
		menu->AddItem( mi);
		if (streq( s[i], init))
			mi->SetMarked( TRUE);
	}
	AddItem( menu);
#endif
}


T::T( BRect r, char *init, uint m):
BBox( r)
{

	r = Bounds( );
	r.InsetBy( 2, 2);
	AddChild( new T2( r, init, m));
}


T::T2::T2( BRect r, char *init, uint m):
BTextView( r, "", BRect( 0, 0, 1000, 100), B_FOLLOW_NONE, B_WILL_DRAW)
{

	mesg = m;
	SetText( init);
}


void
T::T2::KeyDown( const char *bytes, int32 numBytes)
{
	BMessage	*m;

	switch (bytes[0]) {
	case '\n':
	case '\r':
		Window( )->PostMessage( mesg);
	case B_DOWN_ARROW:
	case B_UP_ARROW:
		break;
	default:
		BTextView::KeyDown( bytes, numBytes);
		m = new BMessage( mesg);
		m->AddString( "", Text( ));
		Window( )->PostMessage( m);
	}
}


C::C( RNode n, BPoint org, char *t, char *d):
RNode( n), BWindow( BRect( 0, 0, 160, 90), t, B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_WILL_ACCEPT_FIRST_CLICK)
{

	Run( );
	strncopy( dev, d, sizeof dev);
	MoveTo( org);
	AddChild( new BG( d));
	Show( );
}


void
C::MessageReceived( BMessage *m)
{

	printf( "C::MessageReceived: what=0x%X (%.4s)\n", m->what, &m->what);
	switch (m->what) {
	case 'cdev':
		strncopy( dev, m->FindString( ""), sizeof dev);
		break;
	case 'CONN':
		m = new BMessage( 'CONN');
		m->AddString( "dev", dev);
		*this >> m;
		Quit( );
		break;
	case 'r':
		new C( *this, Frame( ).LeftTop( ), (char *)Title( ), Dev::defaultdev());
		Quit( );
	}
}


C::BG::BG( char *dev):
BView( BRect( ), 0, B_FOLLOW_ALL, B_WILL_DRAW)
{

	AddChild( new Dev( BPoint( 88, 15+16-menuheight( )), dev));
	AddChild( new S( BRect( 20, 15, 355, 29), "Connect via"));
	BButton *cb = new BButton( BRect( 35, 50, 125, 0), "", "Connect", new BMessage( 'CONN'));
	cb->MakeDefault( true);
	AddChild( cb);
}


void
C::BG::AttachedToWindow( )
{

	BRect r = Window( )->Bounds( );
	ResizeTo( r.Width( ), r.Height( ));
	SetViewColor( 216, 216, 216);
}


D::D( RNode n, BPoint org, const char *s, const char *c, const char *d, const char *i, const char *t, const char *l, const char *p):
RNode( n), BWindow( BRect( 0, 0, 412, 210), s, B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_WILL_ACCEPT_FIRST_CLICK)
{

	Run( );
	strncopy( com, c, sizeof com);
	strncopy( dev, d, sizeof dev);
	strncopy( init, i, sizeof init);
	strncopy( telno, t, sizeof telno);
	strncopy( login, l, sizeof login);
	strncopy( passw, p, sizeof passw);
	MoveTo( org);
	AddChild( new BG( this));
	Show( );
}


void
D::MessageReceived( BMessage *m)
{
	BPath	path;

	printf( "D::MessageReceived: what=0x%X (%.4s)\n", m->what, &m->what);
	switch (m->what) {
	case 'cdev':
		strncopy( dev, m->FindString( ""), sizeof dev);
		break;
	case 'dcom':
		if (char *p = (char *)m->FindString( ""))
			strncopy( com, m->FindString( ""), sizeof com);
		break;
	case 'init':
		if (char *p = (char *)m->FindString( ""))
			strncopy( init, m->FindString( ""), sizeof init);
		break;
	case 'tele':
		if (char *p = (char *)m->FindString( ""))
			strncopy( telno, m->FindString( ""), sizeof telno);
		break;
	case 'logi':
		if (char *p = (char *)m->FindString( ""))
			strncopy( login, m->FindString( ""), sizeof login);
		break;
	case 'pass':
		if (char *p = (char *)m->FindString( ""))
			strncopy( passw, m->FindString( ""), sizeof passw);
		break;
	case 'DIAL':
		m = new BMessage( 'DIAL');
		m->AddString( "com", com);
		m->AddString( "dev", dev);
		m->AddString( "init", init);
		m->AddString( "telno", telno);
		m->AddString( "login", login);
		m->AddString( "passw", passw);
		*this >> m;
		Quit( );
		break;
	case 'r':
		find_directory (B_BEOS_ETC_DIRECTORY, &path);
		path.Append (W::Win::defaultdial);
		new D( *this, Frame( ).LeftTop( ), (char *)Title( ), path.Path(), Dev::defaultdev(), W::Win::defaultinit, "", "", "");
		Quit( );
	}
}


D::BG::BG( D *d):
BView( BRect( ), 0, B_FOLLOW_ALL, B_WILL_DRAW)
{

	const int y = 8;
	const int l1 = 120;
	const int r1 = 400;
	const int l2 = 15;
	const int t = 30;
	const int r2 = 115;
	const int b1 = 18;
	const int b2 = 16;
	AddChild( new T( BRect( l1, y+0*t, r1, y+0*t+b1), d->telno, 'tele'));
	AddChild( new S( BRect( l2, y+0*t, r2, y+0*t+b2), "Phone Number"));
	AddChild( new T( BRect( l1, y+1*t, r1, y+1*t+b1), d->login, 'logi'));
	AddChild( new S( BRect( l2, y+1*t, r2, y+1*t+b2), "User Name"));
	AddChild( new T( BRect( l1, y+2*t, r1, y+2*t+b1), d->passw, 'pass'));
	AddChild( new S( BRect( l2, y+2*t, r2, y+2*t+b2), "Password"));
	AddChild( new T( BRect( l1, y+3*t, r1, y+3*t+b1), d->com, 'dcom'));
	AddChild( new S( BRect( l2, y+3*t, r2, y+3*t+b2), "Dialing Shell Script"));
	AddChild( new T( BRect( l1, y+4*t, r1, y+4*t+b1), d->init, 'init'));
	AddChild( new S( BRect( l2, y+4*t, r2, y+4*t+b2), "Modem Init. String"));
	AddChild( new Dev( BPoint( 151, 175+16-menuheight( )), d->dev));
	AddChild( new S( BRect( l2, 175, 155, 189), "Modem is connected to"));
	AddChild( new BButton( BRect( 240, 170, 310, 0), "", "Defaults", new BMessage( 'r')));
	AddChild( new BButton( BRect( 330, 170, 400, 0), "", "Connect", new BMessage( 'DIAL')));
}


void
D::BG::AttachedToWindow( )
{

	BRect r = Window( )->Bounds( );
	ResizeTo( r.Width( ), r.Height( ));
	SetViewColor( 216, 216, 216);
}


static geom
getgeom( BPoint p)
{
	geom	g;

	uint h = menuheight( );
	g.termview = BRect( 0, 0, p.x-1, p.y-1);
	g.termview.OffsetBy( 3, h+3);
	g.border = g.termview;
	g.border.InsetBy( -3, -3);
	g.scrollbar.left = g.border.right + 1;
	g.scrollbar.top = g.border.top;
	g.scrollbar.right = g.scrollbar.left + B_V_SCROLL_BAR_WIDTH;
	g.scrollbar.bottom = g.border.bottom - B_H_SCROLL_BAR_HEIGHT;
	g.menu.left = g.border.left;
	g.menu.top = g.border.top - h;
	g.menu.right = g.scrollbar.right;
	g.menu.bottom = g.border.top - 1;
	g.window = g.menu;
	--g.window.right;
	g.window.bottom = g.border.bottom;
	return (g);
}


static char	*
strncopy( char *d, const char *s, uint n)
{

	if (n) {
		strncpy( d, s, n);
		d[n-1] = 0;
	}
	return (d);
}


static uint
menuheight( )
{
	static uint	h;

	if (!h) {
		BWindow *w = new BWindow( BRect( ), 0, B_TITLED_WINDOW, 0);
		BMenuBar *z = new BMenuBar( BRect( ), 0);
		z->AddItem( new BMenuItem( "", 0));
		w->Lock( );
		w->AddChild( z);
		h = (int32)z->Bounds( ).bottom + 1;
		w->Unlock( );
		w->Quit( );
	}
	return (h);
}


static void
warn( char *mesg, ...)
{
	va_list	ap;
	char	s[999];

	va_start( ap, mesg);
	vsprintf( s, mesg, ap);
//	fprintf( logfile, mesg, ap);
	BAlert *a = new BAlert( "", s, "Okay", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	a->Go( );
}

/** added by RB ***/

/**  get_current_time -- returns a string of the current time
     from the system clock using the time.h header **/
static void get_current_time(char * filename)
{
   time_t rawtime;
   struct tm * timeinfo;
   char * stringtime = NULL;
   char * tokenstring = NULL;

   time ( &rawtime );
   timeinfo = localtime ( &rawtime ); 
   stringtime = asctime (timeinfo);
 
   tokenstring = strtok (stringtime, " ");

   tokenstring = strtok (NULL, " ");
   strcpy (filename, tokenstring);
   tokenstring = strtok (NULL, " ");
   strcat (filename, tokenstring);
   strcat (filename, "_");
   tokenstring = strtok (NULL, " ");
   tokenstring = strtok (NULL, " ");
   strncat (filename, tokenstring, (strlen(tokenstring)-1) );
   strcat (filename, ".log");
   printf ("filename = %s\n", filename);
  
//   return (filename); 
} 

