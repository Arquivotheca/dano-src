

#include	<unistd.h>
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<Application.h>
#include	<MenuBar.h>
#include	<MenuItem.h>
#include	<ScrollBar.h>
#include	<ScrollView.h>
#include	<Box.h>
#include	<StringView.h>
#include	<Alert.h>
#include	<Beep.h>
#include	"RNode.h"
#include	"Timer.h"
#include	"rico.h"

inline double sqrt(int a) { return sqrt((double) a); }

struct MI: BMenuItem {
	MI( char *label): BMenuItem( label, 0) {
		BMessage *m = new BMessage( 'menu');
		m->AddString( "", label);
		SetMessage( m);
	}
};
struct M: BMenu {
	M( ): BMenu( "Info") {
		AddItem( new MI( "Rules..."));
		AddItem( new MI( "About..."));
	}
};
struct MB: BMenuBar {
	MB( ): BMenuBar( BRect( ), 0) {
		AddItem( new M);
	}
};
struct A: BApplication, RNode {
		A( );
	void	ReadyToRun( ),
		MessageReceived( BMessage *);
};
struct W: BWindow, RNode {
	struct About: BAlert {
			About( );
	};
	struct V: BView {
			V( BRect);
	};
		W( RNode, BPoint);
	private:
	void	MessageReceived( BMessage *),
		WindowActivated( bool);
	RNode	app,
		stonecount,
		winprob,
		speed,
		scroller;
};
struct Rules: BWindow {
	struct Text: BView {
		struct T: BTextView, RNode {
				T( RNode, BRect);
			private:
			void	AttachedToWindow( ),
				MessageReceived( BMessage *),
				FrameResized( float, float),
				setscroller( );
		};
			Text( RNode, BRect);
	};
	struct SB: BScrollBar, RNode {
			SB( RNode, BRect);
		private:
		void	AttachedToWindow( ),
			MessageReceived( BMessage *),
			ValueChanged( float);
	};
		Rules( );
};
class Board {
	struct V {
		protected:
		virtual void	iestablish( BWindow *) = 0,
				festablish( BWindow *) = 0,
				armflash( double) = 0,
				handleinput( BMessage *) = 0,
				flash( ) = 0,
				ack( ) = 0;
	};
	class I: BHandler, RNode, virtual V {
	public:
			I( RNode);
		void	iestablish( BWindow *),
			MessageReceived( BMessage *),
			ack( );
	};
	class F: BHandler, RNode, virtual V {
	public:
		void	festablish( BWindow *),
			armflash( double),
			MessageReceived( BMessage *);
	};
	class B: public BView, RNode, virtual V {
	public:
		struct Hole {
			BRect	rect;
			uint	stones;
			float	theta;
			BPoint	center( );
		};
		enum {
			LC,
			H0, H1, H2, H3, H4, H5,
			RC,
			L0, L1, L2, L3, L4, L5,
			NHOLE,
			FLASHCOUNT = 5,
			FLASHDELAY = 80000
		};
			B( RNode, BPoint);
		void	AttachedToWindow( ),
			Draw( BRect),
			drawstones( uint),
			MessageReceived( BMessage *),
			MouseMoved( BPoint, uint32, const BMessage *),
			MouseDown( BPoint),
			handleinput( BMessage *),
			predict( char *),
			flash( ),
			setstones( uint, uint),
			checkhilite( ),
			reportstones( ),
			setflash( uint);
		BPoint	centerofmass( const Hole &);
		uint	point2hole( BPoint),
			searchdepth,
			hilite,
			flashcount,
			flashhole;
		Hole	hole[NHOLE];
		bool	humanturn;
		static	int stonedata[];
	};
	struct Obj: B, I, F {
			Obj( RNode, RNode, BPoint);
	};
};
struct Input: BLooper, RNode {
		Input( RNode);
	private:
	void	MessageReceived( BMessage *);
};
struct Depth: BBox {
	struct SB: BScrollBar, RNode {
	        	SB( RNode, RNode, BRect);
		private:
		void	AttachedToWindow( ),
			ValueChanged( float),
			MessageReceived( BMessage *);
		bool	enabled;
		uint	depth;
		RNode	label;
	};
		Depth( RNode, BPoint);
};
struct Alert: BAlert {
		Alert( char *);
		~Alert( );
};
struct Str: BStringView, RNode {
		Str( RNode, BRect);
	private:
	void	AttachedToWindow( ),
		MessageReceived( BMessage *);
};
struct BoxedString: BBox {
	struct S: BStringView, RNode {
			S( RNode, BRect, const BFont * = 0, uint = 0);
		private:
		void	AttachedToWindow( ),
			MessageReceived( BMessage *);
	};
		BoxedString( RNode n, BRect r, char *, const BFont * = 0, uint = 0);
};
struct Lab: BStringView {
		Lab( char *, BPoint);
	private:
	void	AttachedToWindow( );
};
/*
enum {
	SIGNATURE	= 'CALA'
};
*/
#define SIGNATURE "application/x-vnd.Be-CALA"

static const rgb_color background = {
	255, 220, 180
};


static uint	menuheight( );
static	int	distance( BPoint, BPoint);
static	int sqr( int);

int
main( )
{
	A	a;

	a.Run( );
	close( 1);
	return (0);
}


A::A( ):
BApplication( SIGNATURE)
{

	srand( find_thread( 0));
}


void
A::ReadyToRun( )
{
	RNode	n;

//+	BPopUpMenu *m = MainMenu( );
//+	BMenuItem *mi = new BMenuItem( "Show rules...", new BMessage( 'rule'));
//+	m->AddItem( mi, 1);
//+	mi->SetTarget( this);
	n.SetDestination( *this);
	new W( n, BPoint( 10, 40));
	Establish( 0, this, "A");
}


void
A::MessageReceived( BMessage *m)
{
	static struct {
		char	size;
		char	depth;
		char	hoty;
		char	hotx;
		char	data[64];
	} xcursor = {
		16, 1, 8, 8,
		0x60, 0x06,0x90, 0x09,0x88, 0x11,0x44, 0x22,0x22, 0x44,0x11, 0x88,0x08, 0x10,0x04, 0x20,
		0x04, 0x20,0x08, 0x10,0x11, 0x88,0x22, 0x44,0x44, 0x22,0x88, 0x11,0x90, 0x09,0x60, 0x06,
		0x00, 0x00,0x60, 0x06,0x70, 0x0e,0x38, 0x1c,0x1c, 0x38,0x0e, 0x70,0x07, 0xe0,0x03, 0xc0,
		0x03, 0xc0,0x07, 0xe0,0x0e, 0x70,0x1c, 0x38,0x38, 0x1c,0x70, 0x0e,0x60, 0x06,0x00, 0x00
	};
	static	int serialno;

	//fprintf( stderr, "A::MessageReceived: what='%.4s'\n", &m->what);
	switch (m->what) {
	case 'rule':
		new Rules;
		break;
	case 'humn':
		serialno = m->FindInt32( "");
		SetCursor( B_HAND_CURSOR);
		break;
	case 'mach': {
		uint sn = m->FindInt32( "");
		if (serialno < sn) {
			serialno = sn;
			SetCursor( &xcursor);
		}
		break;
	}
	case EOF:
		Quit( );
	}
}


W::About::About( ):
BAlert( 0, "\n\n", "Okay", 0, 0, B_WIDTH_AS_USUAL, B_INFO_ALERT)
{

	BTextView *t = TextView( );
	t->SetText( "By Rico Tudor.  Thanks to Warren Smith, Henry Cejtin, Igor Rivin.");
	Go( );
}


Rules::Rules( ):
BWindow( BRect( 100, 100, 600-1, 300-1), "Rules of Calah", B_DOCUMENT_WINDOW, 0)
{
	RNode	n1,
		n2;

	Run( );
	n1.SetDestination( n2);
	n2.SetDestination( n1);
	BRect r = Bounds( );
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.left = r.right+1 - B_V_SCROLL_BAR_WIDTH;
	AddChild( new SB( n1, r));
	r = Bounds( );
	r.right -= B_V_SCROLL_BAR_WIDTH;
	AddChild( new Text( n2, r));
	Show( );
}


Rules::Text::Text( RNode n, BRect r):
BView( r, 0, B_FOLLOW_ALL, B_WILL_DRAW)
{

	SetViewColor( background);
	r = Bounds( );
	r.InsetBy( 10, 10);
	AddChild( new T( n, r));
}


Rules::Text::T::T( RNode n, BRect r):
RNode( n), BTextView( r, 0, r, B_FOLLOW_ALL, B_WILL_DRAW)
{

}


void
Rules::Text::T::AttachedToWindow( )
{
	static char *RULES[] = {
		"BACKGROUND\n",
		"Calah is a member of the mancala family of games, which have been played in Africa for millennia.  This simplest variant is described by Slagle & Dixon (1969) \"Experiments with some programs that search game trees\", JACM V.16, No.2.\n",
		"\n",
		"BOARD DESCRIPTION\n",
		"The game is played with 36 stones on a board with 14 holes.  The holes are arranged in an oval track with two larger holes at each end: these are called the \"calahs\".  The left calah is assigned to the computer, while yours is to the right.\n",
		"\n",
		"OBJECT\n",
		"At the start of the game, both calahs are empty; the remaining holes each contain 3 stones.  You move first.  The goal is to collect as many stones as possible in your calah: stones in a calah remain there for the rest of the game.  The other stones can move from hole to hole.\n",
		"\n",
		"MOVEMENT\n",
		"Turns alternate between players, with the current player being allowed to move stones.  First, a non-empty hole is selected from the player's side of the board.  For you, these are the six holes along the bottom.  Then, the stones are removed and \"seeded\".  Seeding proceeds in a counter-clockwise direction, starting with the neighboring hole.  One stone is added to each hole, until all stones have been placed.  The opponent's calah is never seeded.\n",
		"\n",
		"The last hole seeded determines whether the player can make a free move or conduct a capture.\n",
		"\n",
		"FREE MOVE\n",
		"If the last hole seeded was the player's calah, the player moves again immediately.\n",
		"\n",
		"CAPTURES\n",
		"If the last seeded hole was empty, and was on the player's side of the board, and the corresponding hole on the opponent's side is non-empty, a capture occurs.  Stones from both holes are moved to the player's calah.\n",
		"\n",
		"END OF GAME\n",
		"The game ends when the current player has no move, i.e. there are no stones in the player's holes.  Any stones on the other side go into the opponent's calah.  The winner is the player whose calah has the most stones.\n"
	};
	BFont	f;

	BTextView::AttachedToWindow( );
	GetFontAndColor( 0, &f);
	f.SetSize( 12);
	SetFontAndColor( &f);
	MakeEditable( FALSE);
	MakeSelectable( FALSE);
	SetWordWrap( TRUE);
	SetViewColor( background);
	SetLowColor( background);
	for (uint i=0; i<nel( RULES); ++i)
		Insert( RULES[i]);
	ScrollTo( 0, 0);
	FrameResized( 0, 0);
	Establish( this, Window( ), "A::Rules::Text::T");
}


void
Rules::Text::T::MessageReceived( BMessage *m)
{

	unless (m->what == EOF)
		ScrollTo( 0, m->what);
}


void
Rules::Text::T::FrameResized( float, float)
{

	BRect r = Bounds( );
	unless ((r.Width( ) < 30)
	or (r.Height( ) < 30)) {
		r.OffsetBy( 0, -r.top);
		SetTextRect( r);
	}
	setscroller( );
}


void
Rules::Text::T::setscroller( )
{

	BRect b = Bounds( );
	float h = LineHeight( );
	uint l = CountLines( );
	BMessage *m = new BMessage;
	m->AddInt32( "v", b.top);
	if (l * h)
		m->AddFloat( "", b.Height( )/(l*h));
	m->AddInt32( "s", h);
	m->AddInt32( "S", b.Height( ));
	if (l*h < b.Height( ))
		m->AddInt32( "r", 0);
	else
		m->AddInt32( "r", l*h-b.Height( ));
	*this >> m;
}


Rules::SB::SB( RNode n, BRect r):
RNode( n), BScrollBar( r, 0, 0, 0, 100, B_VERTICAL)
{

}


void
Rules::SB::AttachedToWindow( )
{

	Establish( this, Window( ), "A::Rules::SB");
}


void
Rules::SB::MessageReceived( BMessage *m)
{

	unless (m->what == EOF) {
		SetSteps( m->FindInt32( "s"), m->FindInt32( "S"));
		SetProportion( m->FindFloat( ""));
		SetRange( 0, m->FindInt32( "r"));
		SetValue( m->FindInt32( "v"));
	}
}


void
Rules::SB::ValueChanged( float v)
{

	*this >> v;
}


W::W( RNode n, BPoint org):
app( n), BWindow( BRect( 0, 0, 620-1, 430-1), "Calah", B_TITLED_WINDOW, B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	RNode	n1,
		n2,
		n3,
		n4,
		n5,
		n6,
		n7;

	Run( );
	MoveTo( org);
	n1.SetDestination( n2);
	n2.SetDestination( n1);
	SetDestination( n3);
	n3.SetDestination( *this);
	stonecount.SetDestination( n4);
	winprob.SetDestination( n5);
	speed.SetDestination( n7);
	scroller.SetDestination( n6);
	n6.SetDestination( *this);
	new Input( n1);

	AddChild( new MB);
	BRect r = Bounds( );
	r.OffsetBy( 0, menuheight( ));
	V *v = new V( r);
	AddChild( v);
	v->AddChild( new Depth( n6, BPoint( 50, 240+1)));
	v->AddChild( new Board::Obj( n2, n3, BPoint( 0, 0)));
	v->AddChild( new BoxedString( n4, BRect( 225, 240, 225+110-1, 240+50-1), "Stone Count", 0, 21));
	v->AddChild( new BoxedString( n5, BRect( 370, 240, 370+200-1, 240+40-1), "Your winning prospects", 0, 14));
	v->AddChild( new BoxedString( n7, BRect( 370, 300, 370+200-1, 300+40-1), "Machine speed (nodes/sec)", 0, 14));
	Show( );
	Establish( 0, this, "W");
}


void
W::WindowActivated( bool active)
{

	BMessage *m = new BMessage( 'actv');
	m->AddBool( "", active);
	*this >> m;
}


void
W::MessageReceived( BMessage *m)
{
	char	s[20];
	static	int serialno;

	//fprintf( stderr, "W::MessageReceived: what=0x%X (%.4s)\n", m->what, &m->what);
	switch (m->what) {
	case EOF:
		Quit( );
		break;
	case 'dena':
		scroller >> 'enab';
		break;
	case 'ddis':
		scroller >> 'disa';
		break;
	case 'dnew':
		*this >> DetachCurrentMessage( );
		break;
	case 'humn':
		m = new BMessage( 'humn');
		m->AddInt32( "", ++serialno);
		app >> m;
		break;
	case 'mach':
		m = new BMessage( 'mach');
		m->AddInt32( "", ++serialno);
		BTimerSend( app, m, 700000);
		break;
	case 'tick':
		s[0] = 0;
		if (const char *p = m->FindString( ""))
			sprintf( s, "%.f", atof( p+5) / atof( p+16) * 1000000);
		m = new BMessage;
		m->AddString( "", s);
		speed >> m;
		break;
	case 'prob':
		winprob >> DetachCurrentMessage( );
		break;
	case 'menu':
		if (streq( m->FindString( ""), "Rules..."))
			new Rules;
		else
			new About;
		break;
	case 'ston':
		s[0] = 0;
		if (uint i = m->FindInt32( ""))
			sprintf( s, "%d", i);
		m = new BMessage;
		m->AddString( "", s);
		stonecount >> m;
	}
}


W::V::V( BRect r):
BView( r, 0, 0, B_WILL_DRAW)
{

	SetViewColor( background);
}


Board::Obj::Obj( RNode n1, RNode n2, BPoint org):
I( n1), B( n2, org)
{

}


Board::B::B( RNode n, BPoint org):
RNode( n), BView( BRect( 0, 0, 620-1, 230-1), 0, B_FOLLOW_ALL, B_WILL_DRAW)
{
	enum {
		X0 = 90,
		X1 = 8,
		X2 = 494,
		X3 = 72,
		Y0 = 10,
		Y1 = 150,
		I0 = 78,
		I1 = 116
	};

	MoveTo( org);
	int y2 = (Y0+Y1+I0-I1) / 2;
	hole[LC].rect = BRect( X1, y2, X1+I1, y2+I1);
	hole[RC].rect = BRect( X2, y2, X2+I1, y2+I1);
	hole[H0].rect = BRect( X0+0*X3, Y0, X0+0*X3+I0, Y0+I0);
	hole[H1].rect = BRect( X0+1*X3, Y0, X0+1*X3+I0, Y0+I0);
	hole[H2].rect = BRect( X0+2*X3, Y0, X0+2*X3+I0, Y0+I0);
	hole[H3].rect = BRect( X0+3*X3, Y0, X0+3*X3+I0, Y0+I0);
	hole[H4].rect = BRect( X0+4*X3, Y0, X0+4*X3+I0, Y0+I0);
	hole[H5].rect = BRect( X0+5*X3, Y0, X0+5*X3+I0, Y0+I0);
	hole[L0].rect = BRect( X0+0*X3, Y1, X0+0*X3+I0, Y1+I0);
	hole[L1].rect = BRect( X0+1*X3, Y1, X0+1*X3+I0, Y1+I0);
	hole[L2].rect = BRect( X0+2*X3, Y1, X0+2*X3+I0, Y1+I0);
	hole[L3].rect = BRect( X0+3*X3, Y1, X0+3*X3+I0, Y1+I0);
	hole[L4].rect = BRect( X0+4*X3, Y1, X0+4*X3+I0, Y1+I0);
	hole[L5].rect = BRect( X0+5*X3, Y1, X0+5*X3+I0, Y1+I0);
	for (uint i=0; i<NHOLE; ++i)
		hole[i].stones = 0;
	flashcount = 0;
	hilite = ~0;
	humanturn = FALSE;
	searchdepth = 0;
}


void
Board::B::AttachedToWindow( )
{

	SetViewColor( background);
	snooze( 500000);
	Establish( this, Window( ), "Board");
	iestablish( Window( ));
	festablish( Window( ));
	*this >> 'dena';
}


void
Board::B::Draw( BRect)
{

	SetHighColor( 0, 180, 0);
	for (uint i=0; i<NHOLE; ++i)
		FillEllipse( hole[i].rect);
	for (uint i=0; i<NHOLE; ++i)
		drawstones( i);
}


void
Board::B::drawstones( uint n)
{
	const float SDIAM = 14;

	Hole &h = hole[n];
	BPoint ch = h.center( );
	BPoint cm = centerofmass( h);
	for (uint i=0; i<h.stones; ++i) {
		float x0 = (stonedata[2*i+0]-cm.x) / 7.;
		float y0 = (stonedata[2*i+1]-cm.y) / 7.;
		float x = x0*cos( h.theta) - y0*sin( h.theta);
		float y = x0*sin( h.theta) + y0*cos( h.theta);
		BRect r( x, y, x+SDIAM, y+SDIAM);
		r.OffsetBy( ch);
		r.OffsetBy( -SDIAM/2, -SDIAM/2);
		if ((flashcount % 2)
		and (n == flashhole))
			SetHighColor( 255, 255, 255);
		else if (n == hilite)
			SetHighColor( 255, 180, 0);
		else
			SetHighColor( 255, 0, 0);
		FillEllipse( r);
		SetHighColor( 0, 0, 0);
		StrokeEllipse( r);
	}
}


BPoint
Board::B::centerofmass( const Hole &h)
{

	BPoint c( 0, 0);
	for (uint i=0; i<h.stones; ++i) {
		float x = stonedata[2*i+0];
		float y = stonedata[2*i+1];
		c.x += x;
		c.y += y;
	}
	c.x /= h.stones;
	c.y /= h.stones;
	return (c);
}


void
Board::B::MessageReceived( BMessage *m)
{
	BPoint	p;
	ulong	l;

	//fprintf( stderr, "Board::B::MessageReceived: what=0x%X\n", m->what);
	switch (m->what) {
	case 'actv':
		checkhilite( );
		reportstones( );
		break;
	case 'dnew':
		printf( "d %u\n", m->FindInt32( ""));
		fflush( stdout);
	}
}


void
Board::B::handleinput( BMessage *m)
{
	static char	IMOVES[]	= "dcba98";
	char	*p;

	//fprintf( stderr, "Board::B::handleinput: what=0x%X\n", m->what);
	switch (m->what) {
	case 0:
		if (p = (char *)m->FindString( ""))
			if ((strncmp( p, "Game over", 9) == 0)
			or (strncmp( p, "The game is a draw", 18) == 0)) {
				*this >> 'prob';
				*this >> 'tick';
				hilite = ~0;
				new Alert( p);
				ack( );
			}
			else if (strncmp( p, "Another game?", 13) == 0) {
				*this >> 'dena';
				ack( );
			}
			else if (strncmp( p, "tick", 4) == 0) {
				m->what = 'tick';
				*this >> Window( )->DetachCurrentMessage( );
				printf( "\n");
				fflush( stdout);
				ack( );
			}
			else if (strncmp( p, "I move ", 7) == 0)
				setflash( H0+strchr( IMOVES, p[7])-IMOVES);
			else if (strncmp( p, "Please ", 7) == 0) {
				humanturn = TRUE;
				*this >> 'humn';
				checkhilite( );
				ack( );
			}
			else if (strncmp( p, "You move ", 9) == 0) {
				humanturn = FALSE;
				*this >> 'mach';
				checkhilite( );
				setflash( L0+p[9]-'1');
			}
			else if (strncmp( p, "You've ", 7) == 0) {
				predict( p);
				ack( );
			}
			else if (*p == '!') {
				setstones( LC, atoi( p+1+0*3));
				setstones( H0, atoi( p+1+1*3));
				setstones( H1, atoi( p+1+2*3));
				setstones( H2, atoi( p+1+3*3));
				setstones( H3, atoi( p+1+4*3));
				setstones( H4, atoi( p+1+5*3));
				setstones( H5, atoi( p+1+6*3));
				setstones( RC, atoi( p+1+7*3));
				setstones( L0, atoi( p+1+8*3));
				setstones( L1, atoi( p+1+9*3));
				setstones( L2, atoi( p+1+10*3));
				setstones( L3, atoi( p+1+11*3));
				setstones( L4, atoi( p+1+12*3));
				setstones( L5, atoi( p+1+13*3));
				ack( );
			}
			else
				ack( );
		break;
	case EOF:
		Shutdown( );
	}
}


void
Board::B::predict( char *p)
{
	char	*s;

	BMessage *m = new BMessage( 'prob');
	float f = atof( p+12);
	if (f == 0)
		s = "Nonexistent";
	else if (f < 0.3392292)
		s = "Minuscule";
	else if (f < 0.3788793)
		s = "Feeble";
	else if (f < 0.456533)
		s = "Diminished";
	else if (f == .5)
		s = "Drawish";
	else if (f < 0.5598765)
		s = "Middling";
	else if (f < 0.6312018)
		s = "Encouraging";
	else if (f < 0.7043011)
		s = "Strong";
	else if (f < 1)
		s = "Overwhelming";
	else
		s = "Assured";
	m->AddString( "", s);
	*this >> m;
}


void
Board::B::setstones( uint n, uint s)
{

	Hole &h = hole[n];
	unless (h.stones == s) {
		h.stones = s;
		h.theta = rand( );
		SetHighColor( 0, 180, 0);
		FillEllipse( h.rect);
		drawstones( n);
		reportstones( );
	}
}


void
Board::B::MouseDown( BPoint p)
{

	uint h = point2hole( p);
	if ((L0 <= h && h < NHOLE)
	and (humanturn)
	and (hole[h].stones)) {
		*this >> 'ddis';
		printf( "%d\n", h-L0+1);
		fflush( stdout);
	}
	else
		beep( );
}


void
Board::B::MouseMoved( BPoint, uint32, const BMessage *)
{

	checkhilite( );
	reportstones( );
}


void
Board::B::checkhilite( )
{
	BPoint	p;
	ulong	l;

	GetMouse( &p, &l);
	if ((humanturn)
	and (Window( )->IsActive( ))) {
		uint h = point2hole( p);
		if (L0 <= h && h < NHOLE) {
			unless (h == hilite) {
				uint n = hilite;
				hilite = h;
				unless (n == ~0)
					drawstones( n);
				drawstones( hilite);
			}
		}
		else {
			unless (hilite == ~0) {
				uint n = hilite;
				hilite = ~0;
				drawstones( n);
			}
		}
	}
	else
		unless (hilite == ~0) {
			uint n = hilite;
			hilite = ~0;
			drawstones( n);
		}
}


void
Board::B::reportstones( )
{
	static	int last;
	BPoint	p;
	ulong	l;

	GetMouse( &p, &l);
	uint n = 0;
	if (Window( )->IsActive( )) {
		uint h = point2hole( p);
		unless (h == ~0)
			n = hole[h].stones;
	}
	unless (n == last) {
		BMessage *m = new BMessage( 'ston');
		m->AddInt32( "", n);
		*this >> m;
		last = n;
	}
}


uint
Board::B::point2hole( BPoint p)
{
	uint	cd	= ~0;
	uint	ci	= ~0;

	for (uint n=0; n<NHOLE; ++n) {
		uint d = distance( p, hole[n].center( ));
		if ((d < cd)
		and (d < hole[n].rect.IntegerWidth( )/2)) {
			cd = d;
			ci = n;
		}
	}
	return (ci);
}


void
Board::B::setflash( uint n)
{

	//fprintf( stderr, "Board::B::setflash: %d\n", n);
	flashcount = FLASHCOUNT;
	flashhole = n;
	armflash( FLASHDELAY);
}


void
Board::B::flash( )
{

	drawstones( flashhole);
	if (flashcount) {
		--flashcount;
		armflash( FLASHDELAY);
	}
	else
		ack( );
}


BPoint
Board::B::Hole::center( )
{

	return (BPoint( (rect.right+rect.left)/2, (rect.bottom+rect.top)/2));
}


Board::I::I( RNode n):
RNode( n)
{

}


void
Board::I::iestablish( BWindow *w)
{

	Establish( this, w, "Board::I");
}


void
Board::I::MessageReceived( BMessage *m)
{

	//fprintf( stderr, "Board::I::MessageReceived: what=0x%X, (%s)\n", m->what, m->FindString( ""));
	handleinput( m);
}


void
Board::I::ack( )
{

	*this >> 0;
}


void
Board::F::festablish( BWindow *w)
{

	SetDestination( *this);
	Establish( this, w, "Board::F");
}


void
Board::F::armflash( double t)
{

	BTimerSend( *this, new BMessage, t);
}


void
Board::F::MessageReceived( BMessage *m)
{

	//fprintf( stderr, "Board::F::MessageReceived: what=0x%X\n", m->what);
	unless (m->what == EOF)
		flash( );
}


Input::Input( RNode n):
RNode( n)
{

	Run( );
	Establish( 0, this, "Input");
	PostMessage( 0uL);
}


void
Input::MessageReceived( BMessage *m)
{
	char	line[1000];

	switch (m->what) {
	default:
		if (fgets( line, sizeof line, stdin)) {
			if (char *p = strchr( line, '\n'))
				*p = 0;
			BMessage *m = new BMessage;
			m->AddString( "", line);
			*this >> m;
			break;
		}
	case EOF:
		Quit( );
	}
}


Depth::Depth( RNode n, BPoint org):
BBox( BRect( 0, 0, 140-1, 160-1), 0)
{
	RNode	n2;

	BRect rect = Bounds( );
	MoveTo( org);
	n2.SetDestination( n2);
	SetLabel( "Search Depth");
	AddChild( new Str( n2, BRect( 20, 20, 50, 50)));
	BRect r = rect;
	r.top += 20;
	r.bottom -= 10;
	r.left += 70;
	r.right = r.left + B_V_SCROLL_BAR_WIDTH - 1;
	AddChild( new SB( n, n2, r));
	AddChild( new Lab( "(easy)", BPoint( 95, 33)));
	AddChild( new Lab( "(hard)", BPoint( 95, 130)));
	SetLowColor( background);
	SetViewColor( background);
};


Depth::SB::SB( RNode n1, RNode n2, BRect r):
RNode( n1), label( n2), BScrollBar( r, 0, 0, 0, 0, B_HORIZONTAL)
{

	depth = 0;
	enabled = FALSE;
}


void
Depth::SB::AttachedToWindow( )
{

	Establish( this, Window( ), "SB");
};


void
Depth::SB::ValueChanged( float v)
{
	char	s[20];

	if (enabled) {
		depth = v;
		BMessage *m = new BMessage( 'dnew');
		m->AddInt32( "", depth);
		*this >> m;
		m = new BMessage;
		sprintf( s, "%u", depth);
		m->AddString( "", s);
		label >> m;
	}
}


void
Depth::SB::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case 'enab':
		enabled = TRUE;
		SetRange( 0, 50);
		SetValue( depth);
		break;
	case 'disa':
		enabled = FALSE;
		SetRange( 0, 0);
	}
}


BoxedString::BoxedString( RNode n, BRect r, char *l, const BFont *f, uint fs):
BBox( r, 0)
{

	SetLowColor( background);
	SetViewColor( background);
	SetLabel( l);
	r.OffsetBy( -r.left, -r.top);
	r.InsetBy( 10, 10);
	r.OffsetBy( 5, 5);
	AddChild( new S( n, r, f, fs));
}


BoxedString::S::S( RNode n, BRect r, const BFont *f, uint fs):
BStringView( r, 0, 0), RNode( n)
{

	if (f) {
		SetFont( f);
	}
	if (fs) {
		SetFontSize( fs);
	}
}


void
BoxedString::S::AttachedToWindow( )
{

	SetLowColor( background);
	SetViewColor( background);
	SetAlignment( B_ALIGN_CENTER);
	Establish( this, Window( ), "BoxedString");
}


void
BoxedString::S::MessageReceived( BMessage *m)
{

	unless (m->what == EOF)
		SetText( m->FindString( ""));
}


Str::Str( RNode n, BRect r):
BStringView( r, 0, 0), RNode( n)
{

}


void
Str::AttachedToWindow( )
{

	SetLowColor( background);
	SetViewColor( background);
	SetAlignment( B_ALIGN_RIGHT);
	SetFontSize( 21);
	Establish( this, Window( ), "Str");
}


void
Str::MessageReceived( BMessage *m)
{

	unless (m->what == EOF)
		SetText( m->FindString( ""));
}


Lab::Lab( char *l, BPoint org):
BStringView( BRect( 0, 0, 100, 10), 0, l)
{

	MoveTo( org);
	SetLowColor( background);
	SetViewColor( background);
};


void
Lab::AttachedToWindow( )
{

	ResizeTo( StringWidth( Text( )), 10);
}


Alert::Alert( char *m):
BAlert( "", m, "Okay", 0, 0, B_WIDTH_AS_USUAL, B_EMPTY_ALERT)
{

	Go( );
}


Alert::~Alert( )
{

	printf( "y\n");
	fflush( stdout);
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


static int
distance( BPoint a, BPoint b)
{

	return (sqrt( sqr( a.x-b.x)+sqr( a.y-b.y)));
}


static int
sqr( int i)
{

	return (i * i);
}


int Board::B::stonedata[] = {
	455, 360,	440, 454,	532, 419,
	357, 370,	344, 467,	544, 317,
	516, 514,	463, 263,	365, 275,
	429, 551,	621, 378,	611, 474,
	266, 410,	276, 314,	336, 564,
	548, 220,	507, 608,	256, 508,
	631, 271,	597, 568,	465, 164,
	284, 220,	367, 167,	394, 641,
	700, 435,	706, 332,	186, 352,
	687, 533,	177, 453,	248, 605,
	634, 174,	194, 255,	551, 123,
	722, 237,	412,  82,	784, 391
};
