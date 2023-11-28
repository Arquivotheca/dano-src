
#include <byteorder.h>
#include <View.h>

struct BTermView: BView {
	struct TextPos {
			TextPos( );
			TextPos( uint);
			operator uint( );
		#if B_HOST_IS_LENDIAN
		ushort	col,line;
		#else
		ushort	line,col;
		#endif
	};
	struct Line;
	struct Blinker: RNodeHandler {
				Blinker( BTermView *);
		void		delay( );
		//private:
		void		MessageReceived( BMessage *);
		BTermView	*termview;
		bigtime_t	blinkrate,
				blinktime;
		bool		visible;
		friend		class Line;
	};
	struct Mouser: BHandler, RNode {
				Mouser( BTermView *);
		private:
		void		MessageReceived( BMessage *),
				trackselection( TextPos, BPoint);
		bool		selecting,
				shifted;
		TextPos		pivot;
		BPoint		pivotpoint;
		uint		clicks;
		BTermView	*termview;
	};
	struct SB: RNodeHandler {
				SB( BTermView *, RNode);
		private:
		void		MessageReceived( BMessage *);
		BTermView	*termview;
	};
	struct Boss: RNodeHandler {
				Boss( BTermView *, RNode);
		private:
		void		MessageReceived( BMessage *),
				setsize( uint, uint);
		BTermView	*termview;
		friend		class BTermView;
	};
	struct ELine;
	struct Line {
		struct segment {
			ushort	attr,
				end;
		};
		uint		ncell( BTermView *),
				get( BTermView *, uint);
		void		set( BTermView *, uint, uint),
				wrap( BTermView *),
				clear( BTermView *, uint = 0, uint = ~0),
				draw( BTermView *, uint, uint = 0, uint = ~0),
				shiftleft( BTermView *, uint),
				shiftright( BTermView *, uint);
		static uint	attribute( uint),
				background( uint),
				foreground( uint),
				effects( uint),
				character( uint),
				makeattribute( uint, uint, uint),
				makecode( uint, uint);
		//private:
				Line( );
		ELine		*expand( BTermView *, uint = ~0);
		void		sync( );
		void		compact( BTermView *);
		uchar		*chars;
		segment		*segments;
		ushort		nsegment,
				wrapcol;
		enum {
				bPERB		= bitsof( char),
				UNDERLINED	= 1 << 0,
				REVERSED	= 1 << 1,
				BOLD		= 1 << 2,
				BLINKING	= 1 << 3,
				CURSOR		= 1 << 4,
				FGVALID		= 1 << 5,
				BGVALID		= 1 << 6,
				FOREGROUND	= 10,
				BACKGROUND	= 13,
				WHITE		= 0,
				BLACK		= 1,
				RED		= 2,
				GREEN		= 3,
				YELLOW		= 4,
				BLUE		= 5,
				MAGENTA		= 6,
				CYAN		= 7,
		};
	};
	class ELine {
			ELine( );
		uint	pad( uint, uint);
		Line	*line;
		uint	dirty,
			lowdirt,
			cells[1000],
			ncell,
			wrapcol;
		friend	class Line;
		friend	class Blinker;
		friend	class BTermView;
	};
	struct Control: RNodeHandler {
				Control( BTermView *, RNode);
		void		sendwinch( );
		private:
		void		MessageReceived( BMessage *);
		BTermView	*termview;
		bigtime_t	winchrate,
				winchtime;
	};
	struct Pref {
		uint		ncol,
				nrow,
				tabwidth,
				fontsize;
		char		fontname[B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1];
		bigtime_t	blinkrate;
		rgb_color	background,
				foreground,
				cursorbg,
				cursorfg,
				selbg,
				selfg;
		uint8		encoding;
	};
		BTermView( RNode, RNode, RNode, RNode, RNode, BRect);
	private:
	void	MessageReceived( BMessage *),
		AttachedToWindow( ),
		WindowActivated( bool),
		process( BMessage *),
		Draw( BRect),
		FrameResized( float, float),
		MouseDown( BPoint),
		KeyDown( const char *, int32),
		keyin( const char [], uint = 0),
		kscroll( int),
		reportcursor( ),
		changefont( const char *, uint = 0),
		setup( ),
		prefhandler( BMessage *),
		reencode( uint),
		drawrange( TextPos, TextPos),
		drawline( uint, uint = 0, uint = ~0),
		setdeffont( const BFont *),
		setdefcolors( uint []),
		setcursor( bool = TRUE),
		newline( ),
		shiftup( uint, uint, uint),
		shiftdown( uint, uint, uint),
		setregion( uint, uint),
		clearlines( uint, uint),
		clearcols( uint = 0, uint = ~0),
		setselection( TextPos, TextPos),
		getselection( XString *),
		showselection( ),
		exportselection( BMessage *, char *),
		shiftsel( uint, uint, int),
		pasteselection( ),
		copyclipboard( ),
		pasteclipboard( ),
		gettext( XString *, uint, uint),
		getstring( XString *, TextPos, TextPos),
		purge( ),
		scroll( uint),
		reclaim( );
	uint	convertattr( uint),
		nexttab( uint);
	TextPos	nextword( TextPos),
		prevword( TextPos);
	bool	queryclipboard( ),
		find( BMessage *),
		findforw( Finder *, uint *),
		findback( Finder *, uint *),
		warn( char *),
		inrange( uint),
		active;
	Line	linetab[4000];
	ELine	eline;

	rgb_color bgColor, fgColor;
	rgb_color fgCursor, bgCursor;
	rgb_color fgSel, bgSel;

	uint
		baseline( BRect),
		lastline( BRect),
		curhome,
		curline,
		curcol,
		ncol,
		nrow,
		fill,
		attr,
		fsize,
		encoding,
		em,
		vee,
		ascent,
		xtrim,
		ytrim,
		tabwidth;
	TextPos	sels,
		sele;
	int	xcharadjust,
		ycharadjust;
	FILE	*log;
	void	parse( uchar *, uint);
	bool	escapesequence( );
	XString	x;
	Blinker	blinker;
	Mouser	mouser;
	BMessage*mqueued;
	uint	regbase,
		reglen,
		saved_attr,
		saved_col,
		saved_row;
	bool	application_mode,
		insert_mode,
		scrolllock;
	SB	scroller;
	Boss	boss;
	Control	control;
	RNode	data,
		req;
	friend	class Line;
	friend	class Blinker;
	friend	class Mouser;
	friend	class SB;
	friend	class Boss;
	friend	class Control;
};

inline
BTermView::TextPos::TextPos( )
{

	line = 0;
	col = 0;
}

inline
BTermView::TextPos::TextPos( uint l)
{

	line = l;
	col = 0;
}

inline
BTermView::TextPos::operator uint( )
{

	return (*(uint *)this);
}
