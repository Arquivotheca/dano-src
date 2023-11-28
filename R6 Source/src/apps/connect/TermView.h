
#include <byteorder.h>

struct BTermView: BView {
	typedef uchar	usmall;
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
		friend		struct BTermView::Line;
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
		friend		struct BTermView;
	};
	struct ELine;
	struct Line {
		struct segment {
			uchar	background,
				foreground,
				effects;
			usmall	end;
		};
		uint		get( BTermView *, uint);
		void		set( BTermView *, uint, uint),
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
		void		compact( BTermView *),
				dump( );
		uchar		*chars;
		segment		*segments;
		uint		nsegment;
		enum {
				bPERB		= 8 * sizeof(char),		// number of bits in a 'char'
				UNDERLINED	= 0x01,
				REVERSED	= 0x02,
				BOLD		= 0x04,
				BLINKING	= 0x08,
				CURSOR		= 0x10,
				BLACK		= 1,
				RED		= 2,
				GREEN		= 3,
				YELLOW		= 4,
				BLUE		= 5,
				MAGENTA		= 6,
				CYAN		= 7,
				WHITE		= 8
		};
	};
	class ELine {
			ELine( );
		uint	pad( uint, uint);
		Line	*line;
		uint	dirty,
			lowdirt,
			cells[(1<< (8 * sizeof(usmall)))-1],
			ncell;
		friend	struct BTermView::Line;
		friend	struct BTermView::Blinker;
		friend	struct BTermView;
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
		uint8	encoding;
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
		changefont( const char *, uint = 0, uint8 = -1),
		setup( ),
		prefhandler( BMessage *),
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
		pasteselection( ),
		copyclipboard( ),
		pasteclipboard( ),
		purge( ),
		scroll( uint),
		reclaim( );
	uint	convertattr( uint),
		nexttab( uint);
	TextPos	nextword( TextPos),
		prevword( TextPos);
	bool	queryclipboard( ),
		inrange( uint),
		active;
	Line	linetab[2000];
	ELine	eline;
	uint	background,
		foreground,
		cursorbg,
		cursorfg,
		selbg,
		selfg,
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
	friend	struct BTermView::Line;
	friend	struct BTermView::Blinker;
	friend	struct BTermView::Mouser;
	friend	struct BTermView::SB;
	friend	struct BTermView::Boss;
	friend	struct BTermView::Control;
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
