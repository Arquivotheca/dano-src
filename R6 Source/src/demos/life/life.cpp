

#include	<Application.h>
#include	<Button.h>
#include	<ScrollBar.h>
#include	<StringView.h>
#include	<Box.h>
#include	<MenuBar.h>
#include	<PopUpMenu.h>
#include	<MenuItem.h>
#include	<WindowScreen.h>
#include	<Bitmap.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<time.h>
#include	"RNode.h"
#include	"rico.h"


#define	SPACE	B_8_BIT_1024x768
#define	YTEXT	20
#define	YLEN	(748+2)
#define	XLEN	(1024/8+2)
#define	NLOOPER	8

#define	PRIMORDIAL_SOUP	"primordial soup"
#define	GLIDER_ATTACK	"glider attack"
#define	RAIN_FOREST	"rain forest"
#define	SHOCK_WAVE	"shock wave"
#define	BE_LOGO		"Be logo"


static uint	universe[2][YLEN][XLEN];

static const rgb_color gray = {
	220, 220, 220
};


struct A: BApplication, RNode {
		A( );
	private:
	void	ReadyToRun( ),
		MessageReceived( BMessage *);
	bool	QuitRequested( );
};

struct Slider {
	struct setup {
		RNode	node;
		uint	vmin,
			vmax,
			vinit,
			what;
		char	*label;
		BPoint	base;
	};
	struct SB: BScrollBar, RNode, setup {
			SB( setup);
		private:
		void	AttachedToWindow( ),
			ValueChanged( float);
	};
	struct S: BStringView, RNode {
			S( setup);
		private:
		void	AttachedToWindow( ),
			MessageReceived( BMessage *);
	};
	struct Obj: BBox {
			Obj( setup);
	};
};

struct Sel {
	struct setup {
		RNode	node;
		uint	what;
		char	*label,
			**strings;
		BPoint	base;
	};
	struct P: BPopUpMenu, setup {
			P( BHandler *, setup);
		protected:
		void	AttachedToWindow( );
		BHandler*handler;
	};
	struct M: BMenuBar, RNode {
			M( setup);
		protected:
		void	AttachedToWindow( ),
			MessageReceived( BMessage *);
	};
	struct S: BStringView {
		S( char *);
	};
	struct Obj: BView {
		Obj( setup);
	};
};

struct C {
	struct B: BButton, RNode {
			B( RNode, BPoint);
		private:
		void	AttachedToWindow( ),
			MessageReceived( BMessage *);
	};
	struct V {
		protected:
		virtual void	wshandler( BMessage *) = 0,
				wthandler( BMessage *) = 0;
		RNode		node1,
				node2;
	};
	struct WS: BHandler, virtual V {
		protected:
		void	wsinit( BWindow *);
		private:
		void	MessageReceived( BMessage *);
	};
	struct WT: BHandler, virtual V {
		protected:
		void	wtinit( BWindow *);
		private:
		void	MessageReceived( BMessage *);
	};
	struct WV: BView {
			WV( Slider::setup, Sel::setup, BRect);
	};
	struct W: BWindow, RNode, virtual V {
		protected:
			W( RNode);
		void	winit( );
		private:
		void	MessageReceived( BMessage *),
			wshandler( BMessage *),
			wthandler( BMessage *);
		uint	nlooper;
		char	*pattern;
	};
	struct Obj: W, WS, WT {
			Obj( RNode);
	};
};

struct T {
	struct V {
		protected:
		virtual void	lhandler( BMessage *) = 0;
		RNode		node;
	};
	struct HL: BHandler, virtual V {
		protected:
		void	hlinit( BLooper *);
		private:
		void	MessageReceived( BMessage *);
	};
	struct W: BWindowScreen, RNode, virtual V {
		protected:
		void	winit( );
			W( RNode, RNode, uint, char *, status_t *);
		private:
		void	ScreenConnected( bool),
			MessageReceived( BMessage *),
			lhandler( BMessage *);
		bool	connected;
		sem_id	sem;
		uint	nlooper;
		char	*pattern;
		RNode	cwn;
	};
	struct Obj: W, HL {
		Obj( RNode, RNode, uint, char *, status_t *);
	};
};

struct S {
	struct V {
		protected:
		virtual void	lhandler( BMessage *) = 0;
		virtual void	idraw( char *, void *) = 0;
		uint		bperr;
		RNode		node;
	};
	struct HL: BHandler, virtual V {
		protected:
		void	hlinit( BLooper *),
			MessageReceived( BMessage *);
	};
	struct M: BLooper, RNode, virtual V {
		protected:
			M( RNode, sem_id, uint, char *);
		void	minit( );
		private:
		void	MessageReceived( BMessage *),
			uinit( char *),
			uload( char *),
			setcell( uint, uint),
			lhandler( BMessage *),
			requestgen( bool),
			showinfo( );
		void	*buffer;
		bool	done,
			connected;
		uint	nlooper,
			gen,
			mcount,
			v,
			vhist[100];
		float	times[64],
			bestrate;
		sem_id	sem;
		RNode	nodes[NLOOPER];
	};
	class I: BBitmap, virtual V {
		struct View: BView {
				View( BRect);
			void	draw( char *, void *, void *, uint);
			private:
			void	AttachedToWindow( );
		};
		protected:
			I( );
		private:
		void	idraw( char *, void *);
		View	*view;
	};
	struct Obj: M, HL, I {
		Obj( RNode, sem_id, uint, char *);
	};
};

struct L: BLooper, RNode {
			L( uint, uint, RNode);
	private:
	void		MessageReceived( BMessage *),
			display( uint, void *, uint, uint);
	uint		generate( void *, void *, uint);
	const uint	id,
			height;
	void		*buffer;
	uint		bperr,
			gen;
};

int
main( )
{
	A	a;

	return (0);
}


A::A( ):
BApplication("application/x-vnd.Be-LIFE")
{

	Run( );
}


void
A::ReadyToRun( )
{
	RNode	n;

	new C::Obj( n);
	SetDestination( n);
	n.SetDestination( *this);
	Establish( 0, this, "A");
}


void
A::MessageReceived( BMessage *m)
{

	if (m->what == EOF)
		Quit( );
}


bool
A::QuitRequested( )
{

	return (TRUE);
}


C::B::B( RNode n, BPoint p):
RNode( n), BButton( BRect( ), "", "Start", new BMessage( 'go'))
{

	MoveTo( p);
	SetFont( be_bold_font);
	ResizeTo( 70, 30);
}


void
C::B::AttachedToWindow( )
{

	SetTarget( this);
}


void
C::B::MessageReceived( BMessage *m)
{

	*this >> Window( )->DetachCurrentMessage( );
}


void
C::WS::wsinit( BWindow *w)
{

	node1.Establish( this, w, "C::WS");
}


void
C::WS::MessageReceived( BMessage *m)
{

	wshandler( m);
}


void
C::WT::wtinit( BWindow *w)
{

	node2.Establish( this, w, "C::WT");
}


void
C::WT::MessageReceived( BMessage *m)
{

	wthandler( m);
}


C::WV::WV( Slider::setup slider, Sel::setup sel, BRect r):
BView( r, 0, 0, B_WILL_DRAW)
{

	SetViewColor( gray);
	AddChild( new B( slider.node, BPoint( 62, 145)));
	AddChild( new Slider::Obj( slider));
	AddChild( new Sel::Obj( sel));
}


C::W::W( RNode n):
RNode( n), BWindow( BRect( 0, 0, 190, 190), "Life Configuration", B_TITLED_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{

	Run( );
	nlooper = 2;
	pattern = PRIMORDIAL_SOUP;
	Slider::setup slider;
	slider.vmin = 1;
	slider.vmax = NLOOPER;
	slider.vinit = nlooper;
	slider.label = "Thread Count";
	slider.what = 'thrd';
	slider.base = BPoint( 10, 10);
	slider.node.SetDestination( node1);
	Sel::setup sel;
	char *strings[] = {
		PRIMORDIAL_SOUP,
		GLIDER_ATTACK,
		RAIN_FOREST,
		SHOCK_WAVE,
		BE_LOGO,
		0
	};
	sel.what = 'patt';
	sel.label = "Scenario:";
	sel.strings = strings;
	sel.base = BPoint( 15, 100);
	sel.node.SetDestination( node1);
	AddChild( new WV( slider, sel, Bounds( )));
	MoveBy( 100, 100);
	Show( );
}


void
C::W::winit( )
{

	Establish( 0, this, "C::W");
}


void
C::W::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		node2.Shutdown( );
		break;
	case 'acti':
		Activate( TRUE);
	}
}


void
C::W::wshandler( BMessage *m)
{
	status_t   error;
	
	switch (m->what) {
	case 'go':
		{
			RNode n1;
			RNode n2;
			n1.SetDestination( *this);
			n2.SetDestination( node2);
			node2.SetDestination( n2);
			new T::Obj( n1, n2, nlooper, pattern, &error);
		}
		break;
	case 'thrd':
		nlooper = m->FindInt32( "");
		break;
	case 'patt':
		DetachCurrentMessage( );
		pattern = (char *) m->FindString( "");
		break;
	case EOF:
		Quit( );
	}
}


void
C::W::wthandler( BMessage *m)
{

	switch (m->what) {
	case EOF:
		Quit( );
	}
}


C::Obj::Obj( RNode n):
W( n)
{

	winit( );
	wsinit( this);
	wtinit( this);
}


void
T::HL::hlinit( BLooper *l)
{

	node.Establish( this, l, "T::HL");
}


void
T::HL::MessageReceived( BMessage *m)
{

	lhandler( m);
}


T::W::W( RNode n1, RNode n2, uint l, char *p, status_t *error):
BWindowScreen( "", SPACE, error), cwn( n1), RNode( n2)
{

	Run( );
	connected = FALSE;
	sem = create_sem( 0, "life");
	nlooper = l;
	pattern = p;
	Show( );
}


void
T::W::winit( )
{

	RNode n;
	new S::Obj( n, sem, nlooper, pattern);
	n.SetDestination( node);
	node.SetDestination( n);
	Establish( 0, this, "T::W");
}


void
T::W::ScreenConnected( bool active)
{

	if (active) {
		graphics_card_info *ci = CardInfo( );
		cwn >> 'acti';
		BMessage *m = new BMessage( 'buff');
		m->AddInt32( "buff", (long)ci->frame_buffer);
		m->AddInt32( "bperr", ci->bytes_per_row);
		node >> m;
	}
	else {
		node >> 'stop';
		acquire_sem( sem);
	}
	connected = active;
}


void
T::W::MessageReceived( BMessage *message)
{
	int8		key_code;

	switch(message->what) {
	case EOF:
		Quit( );
		break;
				
	case B_KEY_DOWN :
		if (message->FindInt8("byte", &key_code) != B_OK)
			break;
		if (key_code == B_ESCAPE)
			PostMessage(B_QUIT_REQUESTED);
		break;
	}
}


void
T::W::lhandler( BMessage *m)
{

	switch (m->what) {
	case EOF:
		Quit( );
		break;
	}
}


T::Obj::Obj( RNode n1, RNode n2, uint l, char *p, status_t *error):
W( n1, n2, l, p, error)
{

	winit( );
	hlinit( this);
}


S::Obj::Obj( RNode n, sem_id s, uint l, char *p):
M( n, s, l, p)
{

	minit( );
	hlinit( this);
}


void
S::HL::hlinit( BLooper *l)
{

	node.Establish( this, l, "S::HL");
}


void
S::HL::MessageReceived( BMessage *m)
{

	lhandler( m);
}


S::M::M( RNode n, sem_id s, uint l, char *p):
RNode( n)
{

	nlooper = l;
	mcount = 0;
	done = FALSE;
	connected = FALSE;
	gen = 0;
	bestrate = 0;
	sem = s;
	Run( );
	srand( time( (time_t *)0));
	uinit( p);
}


void
S::M::minit( )
{

	for (uint i=0; i<nlooper; ++i) {
		RNode n;
		new L( i, (YLEN-2)/nlooper, n);
		n.SetDestination( node);
		nodes[i].SetDestination( n);
	}
	Establish( 0, this, "S::M");
}


void
S::M::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		for (uint i=0; i<nlooper; ++i)
			nodes[i].Shutdown( );
		break;
	case 'buff':
		buffer = (void *) m->FindInt32( "buff");
		bperr = m->FindInt32( "bperr");
		snooze( 1000000);
		memset( buffer, 0, (YLEN-2+YTEXT)*bperr);
		snooze( 1000000);
		connected = TRUE;
		done = FALSE;
		requestgen( TRUE);
		break;
	case 'stop':
		if (done)
			release_sem( sem);
		else
			connected = FALSE;
	}
}


void
S::M::uinit( char *pattern)
{

	if (streq( pattern, PRIMORDIAL_SOUP)) {
		for (uint y=0; y<YLEN; ++y)
			for (uint x=0; x<(XLEN-2)*8; ++x)
				if ((rand( )>>4)%100 < 40)
					setcell( x, y);
	}
	else if (streq( pattern, GLIDER_ATTACK)) {
		for (uint ycoord=0; ycoord<550; ycoord+=20)
			for (uint xcoord=0; xcoord<(XLEN-2)*8-20; xcoord+=20) {
				uint x = xcoord + (rand( )>>4)%16;
				uint y = ycoord + (rand( )>>4)%16;
				setcell( x+0, y+2);
				setcell( x+1, y+2);
				setcell( x+2, y+2);
				if (ycoord/20 % 2)
					setcell( x+2, y+1);
				else
					setcell( x+0, y+1);
				setcell( x+1, y+0);
			}
		for (uint ycoord=600; ycoord<700; ycoord+=5)
			for (uint xcoord=0; xcoord<(XLEN-2)*8-5; xcoord+=5) {
				uint x = xcoord + (rand( )>>4)%2;
				uint y = ycoord + (rand( )>>4)%2;
				setcell( x+0, y+0);
				setcell( x+1, y+0);
				setcell( x+1, y+1);
				setcell( x+0, y+1);
			}
	}
	else if (streq( pattern, RAIN_FOREST)) {
		for (uint ycoord=0; ycoord<YLEN-2-3; ycoord+=5)
			for (uint xcoord=0; xcoord<(XLEN-2)*8; xcoord+=4) {
				uint x = xcoord + (rand( )>>4)%1;
				uint y = ycoord + (rand( )>>4)%2;
				setcell( x+0, y+0);
				setcell( x+1, y+0);
				setcell( x+1, y+1);
				setcell( x+0, y+1);
			}
		for (uint ycoord=YLEN*2/5; ycoord<YLEN*3/5; ycoord+=1)
			if ((rand( )>>4)%100 < 10)
				setcell( 0, ycoord);
	}
	else if (streq( pattern, SHOCK_WAVE)) {
		for (uint ycoord=50; ycoord<51; ycoord+=20)
			for (uint xcoord=450; xcoord<451; xcoord+=20) {
				uint x = xcoord + (rand( )>>4)%16;
				uint y = ycoord + (rand( )>>4)%16;
				setcell( x+0, y+2);
				setcell( x+1, y+2);
				setcell( x+2, y+2);
				setcell( x+2, y+1);
				setcell( x+1, y+0);
			}
		for (uint ycoord=100; ycoord<YLEN-2-100; ycoord+=4)
			for (uint xcoord=(rand( )>>4)%2; xcoord<(XLEN-2)*8-4; xcoord+=4) {
				uint x = xcoord + (rand( )>>4)%1;
				uint y = ycoord + (rand( )>>4)%1;
				setcell( x+0, y+0);
				setcell( x+1, y+0);
				setcell( x+1, y+1);
				setcell( x+0, y+1);
			}
	}
	else if (streq( pattern, BE_LOGO)) {
		for (uint y=0; y<548; y+=4)
			for (uint x=0; x<704; x+=3) {
				extern uint BeLogo[];
				uint i = y*704 + x;
				if (BeLogo[i/32] & 1<<31-i%32) {
					setcell( 150+x+0, 130+y+0);
					setcell( 150+x+1, 130+y+0);
					setcell( 150+x+1, 130+y+1);
					setcell( 150+x+0, 130+y+1);
				}
			}
		for (uint y=0; y<YLEN-2; y+=1)
			for (uint x=0; x<(XLEN-2)*8; x+=1)
				unless ((50 < x && x < (XLEN-2)*8-50)
				and (20 < y && y < (YLEN-2)-20))
					if ((rand( )>>4)%100 < 40)
						setcell( x, y);
	}
}


void
S::M::uload( char *file)
{
	uint	x,
		y;
	char	line[100];

	if (FILE *f = fopen( file, "r")) {
		while (fgets( line, sizeof line, f))
			unless (line[0] == '#')
				if (sscanf( line, "%d %d", &x, &y) == 2) {
					x += (XLEN-2)*8*1000;
					y += (YLEN-2) * 1000;
					setcell( x%((XLEN-2)*8), y%(YLEN-2));
				}
		fclose( f);
	}
}


void
S::M::setcell( uint x, uint y)
{

	uint w = x / 8;
	uint o = x % 8;
	universe[0][1+y][1+w] |= 0x10000000 >> o*4;
}


void
S::M::lhandler( BMessage *m)
{

	switch (m->what) {
	case EOF:
		delete_sem( sem);
		Quit( );
		break;
	default:
		v += m->what;
		if (++mcount == nlooper) {
			showinfo( );
			uint i = 0;
			loop {
				if (i == nel( vhist)) {
					vhist[gen%nel( vhist)] = v;
					if (connected)
						requestgen( FALSE);
					else
						release_sem( sem);
					break;
				}
				if (vhist[i] == v) {
					char s[80];
					sprintf( s, "Stability at generation %d", gen);
					idraw( s, buffer);
					done = TRUE;
					break;
				}
				++i;
			}
		}
	}
}


void
S::M::requestgen( bool buff)
{

	uint i = gen % 2;
	memcpy( universe[i][0], universe[i][YLEN-2], sizeof universe[i][0]);
	memcpy( universe[i][YLEN-1], universe[i][1], sizeof universe[i][0]);
	v = 0;
	for (uint i=0; i<nlooper; ++i) {
		BMessage *m = new BMessage( 'gen');
		if (buff) {
			m->what = 'buff';
			m->AddInt32( "buff", (long)buffer);
			m->AddInt32( "bperr", bperr);
		}
		nodes[i] >> m;
	}
	mcount = 0;
	++gen;
}


void
S::M::showinfo( )
{
	char	s1[80],
		s2[80];

	double t = system_time( );
	unless (gen % 10) {
		sprintf( s1, "Generation: %d", gen);
		unless (gen <= nel( times)) {
			float r = (YLEN-2)*(XLEN-2)*8*nel( times) / (t-times[gen%nel( times)]);
			sprintf( s2, "        Rate: %.2f Mc/s", r);
			strcat( s1, s2);
			bestrate = max_c( bestrate, r);
			sprintf( s2, "        Machine utilization: %.3f", r/bestrate);
			strcat( s1, s2);
		}
		idraw( s1, buffer);
	}
	times[gen%nel( times)] = t;
}


S::I::View::View( BRect r):
BView( r, 0, 0, B_WILL_DRAW)
{

}


void
S::I::View::AttachedToWindow( )
{

	SetHighColor( 255, 255, 0);
	SetLowColor( 0, 0, 0);
	SetFont( be_plain_font);
	SetFontSize( 20);
}


void
S::I::View::draw( char *s, void *bits, void *buffer, uint bperr)
{

	memset( bits, 0, (XLEN-2)*8*YTEXT);
	DrawString( s, BPoint( 30, 15));
	Sync( );
	uchar *p = (uchar *)buffer + bperr*(YLEN-2);
	for (uint y=0; y<YTEXT; ++y) {
		memcpy( p, (uchar *)bits+(XLEN-2)*8*y, (XLEN-2)*8);
		p += bperr;
	}
}


S::I::I( ):
BBitmap( BRect( 0, 0, (XLEN-2)*8-1, YTEXT-1), B_COLOR_8_BIT, TRUE)
{

	view = new View( Bounds( ));
	AddChild( view);
}


void
S::I::idraw( char *s, void *buffer)
{

	Lock( );
	view->draw( s, Bits( ), buffer, bperr);
	Unlock( );
}


L::L( uint i, uint h, RNode n):
RNode( n), id( i), height( h)
{

	gen = 0;
	Run( );
	Establish( 0, this, "L");
}


void
L::MessageReceived( BMessage *m)
{

	switch (m->what) {
	case EOF:
		Quit( );
		break;
	case 'buff':
		buffer = (void *) m->FindInt32( "buff");
		bperr = m->FindInt32( "bperr");
	case 'gen':
		uint y = 1 + height*id;
		uint src = gen % 2;
		uint dst = (gen+1) % 2;
		display( src, buffer, y, height);
		*this >> generate( universe[dst][y], universe[src][y], height);
		++gen;
	}
}


uint
L::generate( void *dst, void *src, uint ycount)
{

	uint x11111111 = 0x11111111;
	uint xCCCCCCCC = 0xCCCCCCCC;
	uint x33333333 = 0x33333333;
	uint x22222222 = 0x22222222;
	uint v = 0;
	uint *cp = (uint *) src;
	uint *rp = (uint *) dst;
	cp[-XLEN] = cp[-2];
	cp[-1] = cp[-XLEN+1];
	cp[0] = cp[XLEN-2];
	cp[XLEN-1] = cp[1];
	for (uint y=0; y<ycount; ++y) {
		cp[XLEN] = cp[XLEN+XLEN-2];
		cp[XLEN+XLEN-1] = cp[XLEN+1];
		++cp;
		++rp;
		for (uint x=1; x<XLEN-1; ++x) {
			uint z = 0;
			z += cp[-XLEN] + (cp[-XLEN]<<4) + (cp[-XLEN]>>4) + (cp[-XLEN-1]<<28) + (cp[-XLEN+1]>>28);
			z += cp[XLEN] + (cp[XLEN]<<4) + (cp[XLEN]>>4) + (cp[XLEN-1]<<28) + (cp[XLEN+1]>>28);
			z += (cp[-1]<<28) + (cp[1]>>28) + (cp[0]<<4) + (cp[0]>>4);
			z = (z|cp[0]) ^ x33333333;
			z = (z&xCCCCCCCC)>>2 | z&x33333333;
			z = (z&x22222222)>>1 | z&x11111111;
			z ^= x11111111;
			++cp;
			*rp++ = z;
			v = v*1021 + z + (z>>15) + 67;
		}
		++cp;
		++rp;
	}
	return (v);
}


#if __POWERPC__
void
L::display( uint i, void *buffer, uint ybase, uint ycount)
{
	uint	d[2];

	uint x40404040 = 0x40404040;
	uint x3F3F3F3F = 0x3F3F3F3F;
	double *p = (double *)buffer + (ybase-1)*bperr/8;
	for (uint y=0; y<ycount; ++y) {
		uint *up = universe[i][ybase+y] + 1;
		for (uint x=1; x<XLEN-1; ++x) {
			uint v = *up++;
			uint z;
			z = (v&0x10000000)>>4 | (v&0x01000000)>>8 | (v&0x00100000)>>12 | (v&0x00010000)>>16;
			d[0] = x40404040-z & x3F3F3F3F;
			z = (v&0x00001000)<<12 | (v&0x00000100)<<8 | (v&0x00000010)<<4 | (v&0x00000001)<<0;
			d[1] = x40404040-z & x3F3F3F3F;
			*p++ = * (double *) d;
		}
		p = (double *) ((uchar *)p + bperr - (XLEN-2)*8);
	}
}
#else
void
L::display( uint i, void *buffer, uint ybase, uint ycount)
{
	uint	d[2];

	uint x40404040 = 0x40404040;
	uint x3F3F3F3F = 0x3F3F3F3F;
	int32 *p = (int32 *)buffer + (ybase-1)*bperr/4;
	for (uint y=0; y<ycount; ++y) {
		uint *up = universe[i][ybase+y] + 1;
		for (uint x=1; x<XLEN-1; ++x) {
			uint v = *up++;
			uint z;
			z = (v&0x10000000)>>28 | (v&0x01000000)>>16 | (v&0x00100000)>>4 | (v&0x00010000)<<8;
			*p++ = x40404040-z & x3F3F3F3F;
			z = (v&0x00001000)>>12 | (v&0x00000100) | (v&0x00000010)<<12 | (v&0x00000001)<<24;
			*p++ = x40404040-z & x3F3F3F3F;
		}
		p = (int32 *) ((uchar *)p + bperr - (XLEN-2)*8);
	}
}
#endif

Slider::SB::SB( setup s):
BScrollBar( BRect( ), 0, 0, 0, 100, B_HORIZONTAL), setup( s)
{

	ResizeTo( 80, B_H_SCROLL_BAR_HEIGHT);
	MoveTo( 70, 25);
	SetDestination( node);
	SetRange( vmin, vmax);
}


void
Slider::SB::AttachedToWindow( )
{

	SetValue( vinit);
}


void
Slider::SB::ValueChanged( float f)
{

	BMessage *m = new BMessage( what);
	m->AddInt32( "", (int)f);
	*this >> m;
}


Slider::S::S( setup s):
RNode( s.node), BStringView( BRect( ), 0, "")
{

	ResizeTo( 20, 21);
	MoveTo( 30, 25);
	SetLowColor( gray);
	SetViewColor( gray);
	SetFont( be_plain_font);
	SetFontSize( 21);
}


void
Slider::S::AttachedToWindow( )
{

	Establish( this, Window( ), "Slider::S");
}


void
Slider::S::MessageReceived( BMessage *m)
{

	unless (m->what == EOF) {
		char t[20];
		sprintf( t, "%d", m->FindInt32( ""));
		SetText( t);
		*this >> Window( )->DetachCurrentMessage( );
	}
}


Slider::Obj::Obj( setup s):
BBox( BRect( ))
{

	ResizeTo( 170, 60);
	MoveTo( s.base);
	SetViewColor( gray);
	SetLowColor( gray);
	SetLabel( s.label);
	AddChild( new S( s));
	AddChild( new SB( s));
}




Sel::P::P( BHandler *h, setup s):
BPopUpMenu( s.strings[0]), setup( s)
{

	handler = h;
	for (uint i=0; strings[i]; ++i) {
		BMessage *m = new BMessage( what);
		m->AddString( "", strings[i]);
		AddItem( new BMenuItem( strings[i], m));
	}
}


void
Sel::P::AttachedToWindow( )
{

	BPopUpMenu::AttachedToWindow( );
	SetTargetForItems( handler);
}


Sel::M::M( setup s):
RNode( s.node), BMenuBar( BRect( ), 0, B_FOLLOW_ALL, B_ITEMS_IN_COLUMN, FALSE)
{

	ResizeTo( 100, 20);
	MoveTo( 52, 2);
	AddItem( new P( this, s));
}


void
Sel::M::AttachedToWindow( )
{

	BMenuBar::AttachedToWindow( );
	SetTargetForItems( this);
}


void
Sel::M::MessageReceived( BMessage *m)
{

	*this >> Window( )->DetachCurrentMessage( );
}


Sel::S::S( char *l):
BStringView( BRect( ), 0, l)
{

	ResizeTo( 49, 16);
	MoveTo( 0, 2);
	SetLowColor( gray);
	SetViewColor( gray);
}


Sel::Obj::Obj( setup s):
BView( BRect( ), 0, 0, B_WILL_DRAW)
{

	ResizeTo( 160, 24);
	MoveTo( s.base);
	SetViewColor( gray);
	AddChild( new S( s.label));
	AddChild( new M( s));
}
