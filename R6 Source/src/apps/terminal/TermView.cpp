

#include	<Beep.h>
#include	<Clipboard.h>
#include	<Entry.h>
#include	<Path.h>
#include	<Screen.h>
#include	<UTF8.h>
#include	<Window.h>
#include	"rico.h"
#include	"RNode.h"
#include	"Timer.h"
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<termios.h>
#include	<errno.h>
#include	<unistd.h>
#include	"RNodeHandler.h"
#include	"XString.h"
#include	"reg.h"
#include	"Finder.h"
#include	"TermView.h"

extern bool		commandIsMeta;
static uchar		*memswap( uchar *, uint, uint);
static uint		utf8len( uchar *, uint),
			e2c( uint);
static void		xcopy( XString *, uchar *, uint);
static bool		boolvar( BMessage *, char *);


BTermView::BTermView( RNode n1, RNode n2, RNode n3, RNode n4, RNode n5, BRect r):
data( n1), req( n2), boss( this, n4), blinker( this), scroller( this, n3), mouser( this),
control( this, n5), BView( r, 0, B_FOLLOW_ALL, B_WILL_DRAW|B_FRAME_EVENTS)
{
	active = TRUE;
	ncol = 0;
	nrow = 0;
	curhome = 0;
	curline = 0;
	curcol = 0;
	em = 1;
	vee = 1;
	xcharadjust = 0;
	ycharadjust = 0;
	xtrim = 0;
	ytrim = 0;
	tabwidth = 8;
	log = 0;
	bgCursor.red = bgCursor.green = bgCursor.blue = 0;
	fgCursor.red = fgCursor.green = fgCursor.blue = 255;
	bgSel = bgCursor;
	fgSel = fgCursor;
	reglen = 0;
	application_mode = FALSE;
	insert_mode = FALSE;
	scrolllock = FALSE;
	saved_attr = 0;
	saved_col = 0;
	saved_row = 0;
	mqueued = 0;
}


void
BTermView::AttachedToWindow( )
{
	static uint DEFCOLORS[] = {
		0, 0, 0, 255, 255, 255
	};

	MakeFocus( );
	setdeffont( be_fixed_font);
	setdefcolors( DEFCOLORS);
	setup( );
	scroll( 0);
	data.Establish( this, Looper( ), "BTermView.data");
	blinker.node.SetDestination( blinker.node);
	blinker.node.Establish( &blinker, Looper( ), "BTermView::blinker");
	blinker.node >> 0;
	scroller.node.Establish( &scroller, Looper( ), "BTermView::scroller");
	boss.node.Establish( &boss, Looper( ), "BTermView::boss");
	control.node.Establish( &control, Looper( ), "BTermView::control");
	mouser.Establish( &mouser, Looper( ), "BTermView::mouser");
}


void
BTermView::setdeffont( const BFont *f)
{
	font_family	ff;
	font_style	fs;
	char		s[B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1];

	f->GetFamilyAndStyle( &ff, &fs);
	strcat( strcat( strcpy( s, ff), "/"), fs);
	encoding = f->Encoding( );
	SetFont( f);
	BMessage *m = new BMessage( 'fnam');
	m->AddString( "", s);
	boss.node >> m;
	m = new BMessage( 'fenc');
	m->AddInt8( "", encoding);
	boss.node >> m;
}


void
BTermView::setdefcolors( uint dc[])
{
	fgColor.red = dc[0];
	fgColor.green = dc[1];
	fgColor.blue = dc[2];
	bgColor.red = dc[3];
	bgColor.green = dc[4];
	bgColor.blue = dc[5];

	BMessage *m = new BMessage( 'bCol');
	m->AddData( "bg", B_RGB_COLOR_TYPE, &bgColor, sizeof( bgColor));
	boss.node >> m;

	fill = Line::makeattribute( 0, 0, 0);
	attr = fill;
	SetHighColor(fgColor);
	SetLowColor(bgColor);
	SetViewColor(bgColor);

	purge( );
	Invalidate( );
}


void
BTermView::WindowActivated( bool a)
{

	active = a;
	setcursor( );
	blinker.delay( );
	if ((eline.line)
	and (eline.dirty)) {
		eline.line->compact( this);
		drawline( eline.line-linetab);
	}
}


static int
is_meta(char ch)
{
    if (ch == '['  || ch == ']'  || ch == '*'  || ch == '"' || ch == '\'' ||
        ch == '?'  || ch == '^'  || ch == '\\' || ch == ' ' || ch == '\t' ||
        ch == '\n' || ch == '\r' || ch == '('  || ch == ')' || ch == '{'  ||
        ch == '}'  || ch == '!'  || ch == '$'  || ch == '%' || ch == '~'  ||
        ch == '`'  || ch == '@'  || ch == '#'  || ch == '&' || ch == '>'  ||
        ch == '<'  || ch == ';'  || ch == '|')
        return 1;

    return 0;
}


static char *
escape_meta_chars(const char *str)
{
    int len, num_meta = 0;
    const char *t; 
	char *tmp, *ptr;

    for(len=0, t=str; *t; t++) {
        if (is_meta(*t))
            num_meta++;
        else
            len++;
    }

    ptr = (char *)malloc(len + num_meta*2 + 1);
    if (ptr == NULL)
        return ptr;

    for(tmp=ptr; *str; str++) {
        if (is_meta(*str)) {
            *tmp++ = '\\';
            *tmp++ = *str;
        } else {
            *tmp++ = *str;
        }
    }

    *tmp = '\0';     /* null terminate */

    return ptr;
}


void
BTermView::MessageReceived( BMessage *m)
{

	//printf( "BTermView::MessageReceived: message=%d\n", m->what);
	switch (m->what) {
	case EOF:
		boss.node.Shutdown( );
		break;

	case B_MOUSE_WHEEL_CHANGED:
		{
			float yDelta;
			if (m->FindFloat("be:wheel_delta_y", &yDelta) == B_OK)
				kscroll((int)-(yDelta*4));
		break;
		}

	case B_MIME_DATA:
		if (m->HasData("text/plain", B_MIME_TYPE)) {
			int32	dataLen = 0;
			char	*text = NULL;
			
			m->FindData("text/plain", B_MIME_TYPE, 
						(const void **)&text, &dataLen);
			
			if (dataLen > 0)	
				keyin(text, dataLen);
			
			break;
		}
		
	case B_SIMPLE_DATA:
		{
			uint32		type;
			int32		count, i;
			entry_ref	ref;
			BEntry		entry;
			BPath		path;

			if (m->GetInfo("refs", &type, &count) != B_NO_ERROR)
				break;

			for(i=0; i < count; i++) {
				if (m->FindRef("refs", i, &ref) != B_NO_ERROR)
					continue;

				if (entry.SetTo(&ref) != B_NO_ERROR)
					continue;

                if (entry.GetPath(&path) < 0)
					continue;
				
				if (i > 0) {
					keyin(" ", 1);
				}

				char *str = escape_meta_chars(path.Path());
				keyin(str, strlen(str));
				free(str);
				str = NULL;
			}
		break;
		}

	default:
		if (scrolllock)
			mqueued = Window( )->DetachCurrentMessage( );
		else
			process( m);
	}
}


void
BTermView::process( BMessage *m)
{
	long	n;
	uchar	*p;

	setcursor( FALSE);
	m->FindData( "", B_UINT8_TYPE, (const void **)&p, &n);
	if (p) {
		parse( p, n);
		if (log)
			fwrite( p, n, 1, log);
	}
	setcursor( );
	if ((eline.line)
	and (eline.dirty)) {
		uint lo = eline.lowdirt;
		uint hi = eline.dirty;
		eline.line->compact( this);
		drawline( eline.line-linetab, lo, hi);
	}
	blinker.delay( );
	req >> 0;
}


bool
BTermView::inrange( uint l)
{

	return (l < nel( linetab));
}


void
BTermView::Draw( BRect r)
{

	uint l = baseline( r);
	uint e = lastline( r);
	uint lcol = r.left / em;
	uint rcol = (r.right+1+em-1) / em;
	while ((l < e)
	and (inrange( l))) {
		linetab[l].draw( this, l, lcol, rcol);
		++l;
	}
}


void
BTermView::drawrange( TextPos s, TextPos e)
{

	uint l = s.line;
	if (l < e.line) {
		drawline( l++, s.col);
		while (l < e.line)
			drawline( l++);
		drawline( l, 0, e.col);
	}
	else if (s.col < e.col)
		drawline( l, s.col, e.col);
}


void
BTermView::drawline( uint l, uint start, uint end)
{

	if (inrange( l)) {
		BRect r;
		r.left = start*em;
		r.top = l * vee;
		if (end == ~0)
			r.right = 9999;
		else
			r.right = (int)(end*em) - 1;
		r.bottom = r.top + vee - 1;
		FillRect( r, B_SOLID_LOW );
		linetab[l].draw( this, l, start, end);
	}
}


void
BTermView::setcursor( bool visible)
{

	if (inrange( curline)) {
		uint c = linetab[curline].get( this, curcol);
		if (visible)
			c = Line::makecode( Line::attribute( c)|Line::CURSOR, Line::character( c));
		else
			c = Line::makecode( Line::attribute( c)&~Line::CURSOR, Line::character( c));
		linetab[curline].set( this, curcol, c);
	}
}


uint
BTermView::baseline( BRect r)
{

	return (r.top / vee);
}


uint
BTermView::lastline( BRect r)
{

	return ((r.bottom+1+vee-1) / vee);
}


void
BTermView::MouseDown( BPoint)
{

	BMessage *m = Window( )->DetachCurrentMessage( );
	m->what = 'pres';
	mouser >> m;
}


void
BTermView::KeyDown( const char *bytes, int32 nbyte)
{

	BMessage *m = Window( )->CurrentMessage( );
	//printf( "BTermView::KeyDown: key=0x%X, modifiers=0x%X\n", m->FindInt32( "key"), m->FindInt32( "modifiers"));
	uint modifiers = m->FindInt32( "modifiers");
	switch (m->FindInt32( "key")) {
	case 0x02:
		keyin( "\33[11~");
		break;
	case 0x03:
		keyin( "\33[12~");
		break;
	case 0x04:
		keyin( "\33[13~");
		break;
	case 0x05:
		keyin( "\33[14~");
		break;
	case 0x06:
		keyin( "\33[15~");
		break;
	case 0x07:
		keyin( "\33[16~");
		break;
	case 0x08:
		keyin( "\33[17~");
		break;
	case 0x09:
		keyin( "\33[18~");
		break;
	case 0x0A:
		keyin( "\33[19~");
		break;
	case 0x0B:
		keyin( "\33[20~");
		break;
	case 0x0C:
		keyin( "\33[21~");
		break;
	case 0x0D:
		keyin( "\33[22~");
		break;
	case 0x37:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[1~");
		break;
	case 0x38:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[A");
		break;
	case 0x39:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[5~");
		break;
	case 0x48:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[D");
		break;
	case 0x49:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[G");
		break;
	case 0x4A:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[C");
		break;
	case 0x58:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[4~");
		break;
	case 0x59:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[B");
		break;
	case 0x5A:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[6~");
		break;
	case 0x64:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[2~");
		break;
	case 0x65:
		keyin (modifiers&(B_SHIFT_KEY|B_NUM_LOCK)? bytes: "\33[3~");
		break;
	case 0x1F:
		keyin( "\33[2~");
		break;
	case 0x20:
		keyin( "\33[1~");
		break;
	case 0x35:
		keyin( "\33[4~");
		break;
	case 0x21:
		if (modifiers & B_SHIFT_KEY)
			kscroll( nrow);
		else
			keyin( "\33[5~");
		break;
	case 0x36:
		if (modifiers & B_SHIFT_KEY)
			kscroll( -nrow);
		else
			keyin( "\33[6~");
		break;
	case 0x57:
		if (modifiers & B_SHIFT_KEY)
			kscroll( 1);
		else
			keyin( application_mode? "\33OA": "\33[A");
		break;
	case 0x62:
		if (modifiers & B_SHIFT_KEY)
			kscroll( -1);
		else
			keyin( application_mode? "\33OB": "\33[B");
		break;
	case 0x63:
		keyin( application_mode? "\33OC": "\33[C");
		break;
	case 0x61:
		keyin( application_mode? "\33OD": "\33[D");
		break;
	case 0x47:
	case 0x5b:
		keyin( "\r");
		break;
	case 0x4E:
		if (modifiers & B_CONTROL_KEY) {
			data >> 'sig2';
			break;
		}
	default:
		keyin( bytes, nbyte);
	}
}


void
BTermView::keyin( const char s[], uint n)
{

	BMessage *m = new BMessage;
	unless (n)
		n = strlen( s);
	m->AddData( "", B_UINT8_TYPE, s, n);
	data >> m;
}


void
BTermView::kscroll( int n)
{

	BRect r = Bounds( );
	scroll( max( 0, r.top-n*(int)vee));
}


void
BTermView::FrameResized( float w, float h)
{

	setup( );
}


void
BTermView::changefont( const char *name, uint size)
{
	font_family	ff;
	font_style	fs;
	BFont		f;
	char		*p;

	GetFont( &f);
	if ((name)
	and (p = strchr( name, '/'))) {
		uint i = p - name;
		memset( ff, 0, sizeof ff);
		strncpy( ff, name, i);
		strcpy( fs, p+1);
		f.SetFamilyAndStyle( ff, fs);
		BMessage *m = new BMessage( 'fnam');
		m->AddString( "", name);
		boss.node >> m;
	}
	if (size)
		f.SetSize( size);
	f.SetEncoding( encoding);
	BMessage *m = new BMessage( 'fenc');
	m->AddInt8( "", encoding);
	boss.node >> m;
	SetFont( &f);
	uint oldvee = vee;
	uint oldnrow = nrow;
	uint oldncol = ncol;
	setup( );
	boss.setsize( oldncol, oldnrow);
	scroll( Bounds( ).top*vee/oldvee);
	//ScrollTo( 0, -9999);
	Invalidate( );
}


void
BTermView::setup( )
{
	font_height	fh;
	BFont		f;
	uint		oldncol	= ncol,
			oldnrow	= nrow;

	GetFont( &f);
	fsize = f.Size( );
	f.GetHeight( &fh);
	ascent = ceil( fh.ascent);
	vee = max( 1, ascent+ceil( fh.descent+fh.leading));
	unless (fsize < 6)
		--ascent;
	em = max( 1, StringWidth( "m"));
	BRect r = Bounds( );
	int i = r.right+1 - r.left;
	ncol = i<0? 0: i/em;
	i = r.bottom+1 - r.top;
	nrow = i<0? 0: i/vee;
	nrow = min( nrow, nel( linetab));
	unless ((nrow == oldnrow)
	and (ncol == oldncol))
		control.sendwinch( );

	BMessage *m = new BMessage( 's&l');
	m->AddInt32( "s", vee);
	m->AddInt32( "l", nrow*vee);
	scroller.node >> m;

	int x = r.right+1 - r.left;
	int y = r.bottom+1 - r.top;
	if ((x % em)
	or (y % vee)) {
		int xnew = x + xtrim;
		int ynew = y + ytrim;
		xtrim = xnew % em;
		ytrim = ynew % vee;
		xnew -= xtrim;
		ynew -= ytrim;
		ResizeBy( xnew-(float)x, ynew-(float)y);
	}
}


void
BTermView::prefhandler( BMessage *m)
{
	font_family	ff;
	font_style	fs;
	BFont		f;
	long		n;
	uint		a[6];
	Pref		*p;

	if (m->FindInt32( "BTermView") == 'get') {
		Pref pref;
		pref.ncol = ncol;
		pref.nrow = nrow;
		pref.tabwidth = tabwidth;
		GetFont( &f);
		pref.fontsize = f.Size( );
		f.GetFamilyAndStyle( &ff, &fs);
		strcat( strcat( strcpy( pref.fontname, ff), "/"), fs);
		pref.encoding = encoding;
		pref.blinkrate = blinker.blinkrate;
		pref.background = bgColor;
		pref.foreground = fgColor;
		pref.cursorbg = bgCursor;
		pref.cursorfg = fgCursor;
		pref.selbg = bgSel;
		pref.selfg = fgSel;
		m->AddData( "", B_UINT8_TYPE, &pref, sizeof pref);
		boss.node >> Window( )->DetachCurrentMessage( );
	}
	else {
		m->FindData( "", B_UINT8_TYPE, (const void **)&p, &n);
		if (p)
			switch (m->FindInt32( "BTermView")) {
			case 'init':
				encoding = p->encoding;
				changefont( p->fontname, p->fontsize);
				boss.setsize( p->ncol, p->nrow);
				blinker.blinkrate = p->blinkrate;
				a[0] = p->foreground.red;
				a[1] = p->foreground.green;
				a[2] = p->foreground.blue;
				a[3] = p->background.red;
				a[4] = p->background.green;
				a[5] = p->background.blue;
				setdefcolors( a);
				fgCursor = p->cursorfg;
				bgCursor = p->cursorbg;
				fgSel = p->selfg;
				bgSel = p->selbg;
				tabwidth = p->tabwidth;
				break;
			case 'size':
				boss.setsize( p->ncol, p->nrow);
				break;
			case 'br':
				blinker.blinkrate = p->blinkrate;
				break;
			case 'df':
				a[0] = p->foreground.red;
				a[1] = p->foreground.green;
				a[2] = p->foreground.blue;
				a[3] = bgColor.red;
				a[4] = bgColor.green;
				a[5] = bgColor.blue;
				setdefcolors( a);
				break;
			case 'db':
				a[0] = fgColor.red;
				a[1] = fgColor.green;
				a[2] = fgColor.blue;
				a[3] = p->background.red;
				a[4] = p->background.green;
				a[5] = p->background.blue;
				setdefcolors( a);
				break;
			case 'cf':
				fgCursor = p->cursorfg;
				drawline( curline, curcol, curcol+1);
				break;
			case 'cb':
				bgCursor = p->cursorbg;
				drawline( curline, curcol, curcol+1);
				break;
			case 'sf':
				fgSel = p->selfg;
				Invalidate( );
				break;
			case 'sb':
				bgSel = p->selbg;
				Invalidate( );
			}
	}
}


void
BTermView::reencode( uint enew)
{
	uchar	ubuf[3*nel( eline.cells)],
		cbuf[nel( eline.cells)],
		*p;
	uint	n;

	purge( );
	uint oldc = e2c( encoding);
	uint newc = e2c( enew);
	for (uint l=0; inrange( l); ++l) {
		Line &line = linetab[l];
		if (line.nsegment) {
			if (encoding == B_UNICODE_UTF8) {
				p = line.chars;
				n = utf8len( p, line.segments[line.nsegment-1].end);
			}
			else {
				int32 sl = line.segments[line.nsegment-1].end;
				int32 dl = sizeof ubuf;
				int32 s = 0;
				unless (convert_to_utf8( oldc, (char *)line.chars, &sl, (char *)ubuf, &dl, &s) == B_OK)
					continue;
				p = ubuf;
				n = dl;
			}
			unless (enew == B_UNICODE_UTF8) {
				int32 sl = n;
				int32 dl = sizeof cbuf;
				int32 s = 0;
				unless (convert_from_utf8( newc, (char *)p, &sl, (char *)cbuf, &dl, &s) == B_OK)
					continue;
				p = cbuf;
				n = dl;
			}
			if (uchar *q = (uchar *)malloc( n)) {
				memcpy( q, p, n);
				free( line.chars);
				line.chars = q;
			}
		}
	}
}


void
BTermView::setselection( TextPos p1, TextPos p2)
{

	TextPos olds = sels;
	TextPos olde = sele;
	sels = min( p1, p2);
	sele = max( p1, p2);
	if (sels < olds)
		if (sele < olds) {
			drawrange( sels, sele);
			drawrange( olds, olde);
		}
		else {
			drawrange( sels, olds);
			if (sele < olde)
				drawrange( sele, olde);
			else
				drawrange( olde, sele);
		}
	else if (sels == olds)
		if (sele < olde)
			drawrange( sele, olde);
		else
			drawrange( olde, sele);
	else if (sels < olde) {
		drawrange( olds, sels);
		if (sele < olde)
			drawrange( sele, olde);
		else
			drawrange( olde, sele);
	}
	else {
		drawrange( olds, olde);
		drawrange( sels, sele);
	}
}


void
BTermView::getselection( XString *x)
{

	uint line = sels.line;
	uint col = sels.col;
	uint le = sele.line;
	uint ce = sele.col;
	uint nspace = 0;
	while (inrange( line)) {
		if ((line == le)
		and (col == ce))
			break;
		if (linetab[line].wrapcol)
			if (col < linetab[line].wrapcol) {
				uint c = Line::character( linetab[line].get( this, col++));
				if ((c < 0x0080)
				or (encoding != B_UNICODE_UTF8))
					x->putb( c);
				else if (c < 0x0800) {
					x->putb( 0xC0|c>>6);
					x->putb( 0x80|c&0x003F);
				}
				else {
					x->putb( 0xE0|c>>12);
					x->putb( 0x80|c>>6&0x003F);
					x->putb( 0x80|c&0x003F);
				}
			}
			else {
				col = 0;
				++line;
			}
		else
			if (col < nel( eline.cells)) {
				uint c = Line::character( linetab[line].get( this, col++));
				if (c == ' ')
					++nspace;
				else {
					while (nspace) {
						x->putb( ' ');
						--nspace;
					}
					if ((c < 0x0080)
					or (encoding != B_UNICODE_UTF8))
						x->putb( c);
					else if (c < 0x0800) {
						x->putb( 0xC0|c>>6);
						x->putb( 0x80|c&0x003F);
					}
					else {
						x->putb( 0xE0|c>>12);
						x->putb( 0x80|c>>6&0x003F);
						x->putb( 0x80|c&0x003F);
					}
				}
			}
			else {
				nspace = 0;
				x->putb( '\n');
				col = 0;
				++line;
			}
	}
	while (nspace) {
		x->putb( ' ');
		--nspace;
	}
}


void
BTermView::showselection( )
{

	BRect r = Bounds( );
	uint y = r.bottom+1 - r.top;
	uint yhalf = y/2 - y/2%vee;
	uint s = sels.line * vee;
	uint e = sele.line * vee;
	if (sele.col)
		e += vee;
	if (s < r.top) {
		if (e-s < y)
			if (r.top-s < y)
				scroll( s);
			else if (e-s < yhalf)
				if (s < yhalf)
					scroll( 0);
				else
					scroll( s-yhalf);
			else
				scroll( s);
		else
			if (r.top-s < y)
				scroll( s);
			else
				if (s < yhalf)
					scroll( 0);
				else
					scroll( s-yhalf);
	}
	else if (s+vee <= r.top+y) {
		unless (e < r.top+y)
			if (e-s < y)
				scroll( e-y);
			else if (r.top+yhalf < s)
				scroll( s-yhalf);
	}
	else {
		if (e-s < y)
			if (e < r.top+y+y)
				scroll( e-y);
			else if (e-s < yhalf)
				scroll( s-yhalf);
			else
				scroll( e-y);
		else
			scroll( s-yhalf);
	}
}


void
BTermView::exportselection( BMessage *m, char *label)
{
	XString	x;

	getselection( &x);
	x.putb( 0);
	m->AddString( label, (const char *)x.getm( x.count( )));
}


void
BTermView::shiftsel( uint start, uint end, int shift)
{

	if (sels == sele)
		return;
	TextPos s = TextPos( start);
	TextPos e = TextPos( end);
	if (sels < s)
		if (sele <= s)
			;
		else
			setselection( 0, 0);
	else if (sels < e)
		if (sele <= e)
			if (shift < 0) {
				uint i = sels.line - s.line;
				if (i < -shift)
					setselection( 0, 0);
				else {
					sels.line += shift;
					sele.line += shift;
				}
			}
			else {
				uint i = e.line - sele.line;
				if (i < shift)
					setselection( 0, 0);
				else {
					sels.line += shift;
					sele.line += shift;
				}
			}
		else
			setselection( 0, 0);
	else
		;
}


void
BTermView::pasteselection( )
{

	if (sels == sele)
		pasteclipboard( );
	else {
		XString x;
		getselection( &x);
		uint n = x.count( );
		keyin( (char *)x.getm( n), n);
	}
}


bool
BTermView::queryclipboard( )
{
	bool	b;

	be_clipboard->Lock( );
	BMessage *clip_msg = be_clipboard->Data();
	b = clip_msg->HasData("text/plain", B_MIME_TYPE);
	be_clipboard->Unlock( );
	return b;
}


void
BTermView::copyclipboard( )
{
	XString	x;

	getselection( &x);
	if (x.count( )) {
		x.putb( 0);
		be_clipboard->Lock( );
		be_clipboard->Clear( );
		BMessage *clip_msg = be_clipboard->Data();
		char *data = (char *)x.getm(x.count());
		clip_msg->AddData("text/plain", B_MIME_TYPE, data, strlen(data));
		be_clipboard->Commit( );
		be_clipboard->Unlock( );
	}
}


void
BTermView::pasteclipboard( )
{
	long	n;
	char	*p;

	be_clipboard->Lock( );
	BMessage *clip_msg = be_clipboard->Data();
	if (clip_msg->FindData("text/plain", B_MIME_TYPE, (const void **)&p, &n) == B_OK)
		keyin( p, n);
	be_clipboard->Unlock( );
}


bool
BTermView::find( BMessage *m)
{
	XString	x;
	TextPos	s,
		e;
	uchar	*a;

	bool fold = boolvar( m, "fold");
	if (m->FindString( "pattern", (const char **)&a) == B_OK) {
		unless (*a)
			return (warn( "No search string."));
		xcopy( &x, a, encoding);
	}
	else {
		switch (sele.line - sels.line) {
		case 0:
			unless (sels.col == sele.col)
				break;
			return (warn( "No selection."));
		case 1:
			unless (sele.col)
				break;
		default:
			return (warn( "Cannot search a multi-line selection."));
		}
		getstring( &x, sels, sele);
	}
	x = Finder::getfindstr( x, fold, boolvar( m, "regex"), boolvar( m, "word"));
	uint line = curline;
	uint col = curcol;
	unless (sels == sele) {
		line = sels.line;
		col = sels.col;
	}
	bool forw = boolvar( m, "forw");
	Finder f( (ushort *)x.getm( x.count( )), forw, fold, col);
	if (forw) {
		unless (findforw( &f, &line))
			return (FALSE);
	}
	else
		unless (findback( &f, &line))
			return (FALSE);
	uint l = line;
	uint c = 0;
	while ((linetab[l].wrapcol)
	and (c+linetab[l].wrapcol <= f.col))
		c += linetab[l++].wrapcol;
	s.line = l;
	s.col = f.col - c;

	l = line;
	c = 0;
	while ((linetab[l].wrapcol)
	and (c+linetab[l].wrapcol <= (f.col+f.len)))
		c += linetab[l++].wrapcol;
	e.line = l;
	e.col = (f.col+f.len) - c;

	setselection( s, e);
	showselection( );
	return (TRUE);
}


bool
BTermView::findforw( Finder *f, uint *line)
{

	uint l = *line;
	unless (l < curhome+nrow)
		l = 0;
	loop {
		unless (l)
			break;
		unless (linetab[l-1].wrapcol)
			break;
		f->col += linetab[--l].wrapcol;
	}
	uint lstart = l;
	loop {
		if ((linetab[l].wrapcol)
		and (inrange( l+1))
		and (l+1 < curhome+nrow))
			++l;
		else {
			gettext( &x, lstart, l+1);
			ushort *text = (ushort *) x.getm( x.count( ));
			if (f->search( text, lstart))
				break;
			++l;
			unless ((inrange( l))
			and (l < curhome+nrow))
				l = 0;
			lstart = l;
		}
	}
	switch (f->status) {
	case Finder::BADSYNTAX:
		return (warn( "Invalid regular expression."));
	case Finder::NOMEM:
		return (warn( "Out of mem."));
	case Finder::FAILED:
		return (warn( "Not found."));
	}
	*line = lstart;
	return (TRUE);
}


bool
BTermView::findback( Finder *f, uint *line)
{

	uint l = *line;
	until ((not l)
	or (inrange( l) && l<curhome+nrow))
		--l;
	uint lstart = l;
	while ((lstart)
	and (linetab[lstart-1].wrapcol))
		f->col += linetab[--lstart].wrapcol;
	loop {
		gettext( &x, lstart, l+1);
		ushort *text = (ushort *) x.getm( x.count( ));
		if (f->search( text, lstart))
			break;
		if (lstart)
			l = lstart - 1;
		else
			l = curhome + nrow;
		until ((not l)
		or (inrange( l) && l<curhome+nrow))
			--l;
		lstart = l;
		while ((lstart)
		and (linetab[lstart-1].wrapcol))
			--lstart;
	}
	switch (f->status) {
	case Finder::BADSYNTAX:
		return (warn( "Invalid regular expression."));
	case Finder::NOMEM:
		return (warn( "Out of mem."));
	case Finder::FAILED:
		return (warn( "Not found."));
	}
	*line = lstart;
	return (TRUE);
}


void
BTermView::gettext( XString *x, uint start, uint end)
{

	uint l = start;
	while ((l < end)
	and (inrange( l))) {
		Line &line = linetab[l];
		if (line.wrapcol)
			for (uint col=0; col<line.wrapcol; ++col)
				x->putw( Line::character( line.get( this, col)));
		else {
			uint nspace = 0;
			uint ncol = line.ncell( this);
			for (uint col=0; col<ncol; ++col) {
				uint c = Line::character( line.get( this, col));
				if (c == ' ')
					++nspace;
				else {
					while (nspace) {
						x->putw( ' ');
						--nspace;
					}
					x->putw( c);
				}
			}
		}
		++l;
	}
	x->putw( 0);
}


void
BTermView::getstring( XString *x, TextPos s, TextPos e)
{

	uint line = s.line;
	uint col = s.col;
	uint le = e.line;
	uint ce = e.col;
	uint nspace = 0;
	while (inrange( line)) {
		if ((line == le)
		and (col == ce))
			break;
		if (linetab[line].wrapcol)
			if (col < linetab[line].wrapcol)
				x->putw( Line::character( linetab[line].get( this, col++)));
			else {
				col = 0;
				++line;
			}
		else
			if (col < nel( eline.cells)) {
				uint w = Line::character( linetab[line].get( this, col++));
				if (w == ' ')
					++nspace;
				else {
					while (nspace) {
						x->putw( ' ');
						--nspace;
					}
					x->putw( w);
				}
			}
			else {
				nspace = 0;
				col = 0;
				++line;
			}
	}
	while (nspace) {
		x->putw( ' ');
		--nspace;
	}
	x->putw( 0);
}


BTermView::TextPos
BTermView::nextword( TextPos t)
{

	if (inrange( t.line)) {
		Line &line = linetab[t.line];
		until (Line::character( line.get( this, t.col)) == ' ')
			++t.col;
	}
	return (t);
}


BTermView::TextPos
BTermView::prevword( TextPos t)
{

	if (inrange( t.line)) {
		Line &line = linetab[t.line];
		while ((t.col)
		and (Line::character( line.get( this, t.col-1)) != ' '))
			--t.col;
	}
	return (t);
}


void
BTermView::parse( uchar *s, uint n)
{
	uint	i;

	x.putm( s, n);
	loop {
		unless ((curhome <= curline)
		and (curline-curhome < nrow))
			if (curline < nrow)
				curhome = 0;
			else
				curhome = curline + 1 - nrow;
		switch (uint c = x.getb( )) {
		case '\33':
			i = x.count( ) + 1;
			if (escapesequence( ))
				break;
			x.ungetm( i-x.count( ));
		case -1:
			scroll( curhome * vee);
			return;
		case '\b':
			if (curcol)
				--curcol;
			break;
		case '\t':
			curcol = nexttab( curcol);
			break;
		case '\n':
			newline( );
			break;
		case '\r':
			curcol = 0;
			break;
		case '\a':
			beep( );
			break;
		default:
			if ((inrange( curline))
			and (c >= ' ')) {
				if ((c >= 0x80)
				and (encoding == B_UNICODE_UTF8)) {
					if (c < 0xC0) {
						if (curcol)
							--curcol;
						uint code = linetab[curline].get( this, curcol);
						uint a = Line::attribute( code);
						c = Line::character( code)<<6&0xFFFF | c&0x3F;
						linetab[curline].set( this, curcol++, Line::makecode( a, c));
						break;
					}
					c &= 0x1F;
				}
				unless (curcol < ncol) {
					linetab[curline].wrap( this);
					newline( );
					curcol = 0;
				}
				if (insert_mode)
					linetab[curline].shiftright( this, 1);
				linetab[curline].set( this, curcol++, Line::makecode( attr, c));
				break;
			}
		}
	}
}


void
BTermView::scroll( uint v)
{
	uint	r;

	BMessage *m = new BMessage( 'v&r');
	m->AddInt32( "v", v);
	if (curline < curhome+nrow)
		r = (curhome+nrow-1) * vee;
	else
		r = curline * vee;
	m->AddInt32( "r", r);
	m->AddFloat( "", 1 / (r/(nrow*(float)vee)+1));
	scroller.node >> m;
}


bool
BTermView::escapesequence( )
{
	uint	args[10],
		row,
		col,
		n,
		i;
	int	terminated;

	uint weird = 0;
	switch (uint c = x.getb( )) {
	case -1:
		return (FALSE);
	case '[':
		memset( args, 0, sizeof args);
		n = 0;
		loop
			switch (c = x.getb( )) {
			case -1:
				return (FALSE);
			case ';':
				if (n < nel( args)-1)
					++n;
				break;
			case '?':
				weird = 1000;
				break;
			default:
				if ('0' <= c && c <= '9') {
					args[n] = args[n]*10 + c - '0';
					break;
				}
				return (TRUE);
			case 'A':
				i = max( 1, args[0]);
				if (curline < curhome+i)
					curline = curhome;
				else
					curline -= i;
				return (TRUE);
			case 'B':
				curline += max( 1, args[0]);
				if ((nrow)
				and (curline >= curhome+nrow))
					curline = curhome + nrow - 1;
				return (TRUE);
			case 'C':
				curcol += max( 1, args[0]);
				if ((ncol)
				and (curcol >= ncol))
					curcol = ncol - 1;
				return (TRUE);
			case 'D':
				i = max( 1, args[0]);
				if (curcol < i)
					curcol = 0;
				else
					curcol -= i;
				return (TRUE);
			case 'H':
				row = args[0];
				if (row)
					--row;
				unless (row < nrow)
					row = nrow? nrow-1: 0;
				col = args[1];
				if (col)
					--col;
				unless (col < ncol)
					col = ncol? ncol-1: 0;
				unless (curhome+row == curline)
					curline = curhome + row;
				curcol = col;
				return (TRUE);
			case 'K':
				switch (args[0]) {
				case 0:
					clearcols( curcol);
					break;
				case 1:
					clearcols( 0, curcol+1);
					break;
				case 2:
					clearcols( );
				}
				return (TRUE);
			case 'J':
				switch (args[0]) {
				case 0:
					clearcols( curcol);
					clearlines( curline+1, curhome+nrow);
					break;
				case 1:
					clearlines( curhome, curline);
					clearcols( 0, curcol+1);
					break;
				case 2:
					clearlines( curhome, curhome+nrow);
				}
				return (TRUE);
			case 'm':
				for (i=0; i<=n; ++i)
					attr = convertattr( args[i]);
				return (TRUE);
			case 'n':
				if (args[0] == 6)
					reportcursor( );
				return (TRUE);
			case 'L':
				unless (reglen) {
					shiftdown( curline, curhome+nrow, max( 1, args[0]));
					curcol = 0;
				}
				else if ((curhome+regbase <= curline)
				and (curline < curhome+regbase+reglen)) {
					shiftdown( curline, curhome+regbase+reglen, max( 1, args[0]));
					curcol = 0;
				}
				return (TRUE);
			case 'M':
				unless (reglen) {
					shiftup( curline, curhome+nrow, max( 1, args[0]));
					curcol = 0;
				}
				else if ((curhome+regbase <= curline)
				and (curline < curhome+regbase+reglen)) {
					shiftup( curline, curhome+regbase+reglen, max( 1, args[0]));
					curcol = 0;
				}
				return (TRUE);
			case '@':
				if (inrange( curline))
					linetab[curline].shiftright( this, max( 1, args[0]));
				return (TRUE);
			case 'P':
				if (inrange( curline))
					linetab[curline].shiftleft( this, max( 1, args[0]));
				return (TRUE);
			case 'S':
				shiftup( curhome, curhome+nrow, max( 1, args[0]));
				return (TRUE);
			case 'T':
				shiftdown( curhome, curhome+nrow, max( 1, args[0]));
				return (TRUE);
			case 'h':
				for (i=0; i<=n; ++i)
					switch (args[i]+weird) {
					case 4:
						insert_mode = TRUE;
						break;
					case 1000+1:
						application_mode = TRUE;
					}
				return (TRUE);
			case 'l':
				for (i=0; i<=n; ++i)
					switch (args[i]+weird) {
					case 4:
						insert_mode = FALSE;
						break;
					case 1000+1:
						application_mode = FALSE;
					}
				return (TRUE);
			case 'r':
				unless (weird)
					setregion( args[0], args[1]);
				return (TRUE);
			case 'd':
				row = args[0];
				if (row)
					--row;
				unless (row < nrow)
					row = nrow? nrow-1: 0;
				curline = curhome + row;
				return (TRUE);
			case 'G':
				col = args[0];
				if (col)
					--col;
				unless (col < ncol)
					col = ncol? ncol-1: 0;
				curcol = col;
				return (TRUE);
			case 'X':
				clearcols( curcol, curcol+max( 1, args[0]));
				return (TRUE);
			}
	case 'D':
		newline( );
		break;
	case 'M':
		if (reglen) {
			if (curline == curhome+regbase)
				shiftdown( curhome+regbase, curhome+regbase+reglen, 1);
			else if (curhome < curline)
				--curline;
		}
		else
			if (curhome < curline)
				--curline;
			else {
				curline = curhome;
				shiftdown( curhome, curhome+nrow, 1);
			}
		return (TRUE);
	case '7':
		saved_attr = attr;
		saved_col = curcol;
		saved_row = curline - curhome;
		return (TRUE);
	case '8':
		attr = saved_attr;
		curcol = saved_col;
		curline = curhome + saved_row;
		unless (inrange( curline))
			curline = nel( linetab) - 1;
		return (TRUE);

	/* xterm escape sequence for setting the title bar - goes like this:
	 * <^[>]0;some_string<^G>
	 *
	 * i have also seen a 1 and 2 in place of the 0;  i think they are for
	 * setting the X icon title, the window title, etc.  since we only
	 * support one window title notion, any digit can be used.
	 *
	 * i don't know if xterm allows you to have non-printable characters
	 * in your title bar, but we disallow it.  this may not accept things
	 * like japanese title bars.
	 */
	case ']':
		char tbuf[256];
		terminated = 0;
		
		if (!isdigit(c = x.getb()))
			return TRUE;
		if ((c = x.getb()) != ';')
			return TRUE;

		i = 0;
		while ((c = x.getb()) != -1) {
			if (c == '\a')
				terminated = 1;
			/* this also catches the \a from above */
			if (!isprint(c))
				break;
			tbuf[i++] = c;
			if (i == sizeof(tbuf))
				break;
		}
		tbuf[i] = '\0';

		if (terminated && i > 0 && Window()->Lock()) {
			Window()->SetTitle(tbuf);
			Window()->Unlock();
		}		
		return TRUE;

	/*
	 * BeOS special
	 */
	case '{':
		memset( args, 0, sizeof args);
		n = 0;
		loop
			switch (c = x.getb( )) {
			case -1:
				return (FALSE);
			case ';':
				if (n < nel( args)-1)
					++n;
				break;
			default:
				if ('0' <= c && c <= '9') {
					args[n] = args[n]*10 + c - '0';
					break;
				}
				return (TRUE);
			case 'b':
				blinker.blinkrate = args[0]/10. * 1000000;
				return (TRUE);
			case 'c':
				fgCursor.red = args[0]; fgCursor.green = args[1]; fgCursor.blue = args[2];
				bgCursor.red = args[3]; bgCursor.green = args[4]; bgCursor.blue = args[5];
				return (TRUE);
			case 'd':
				setdefcolors( args);
				return (TRUE);
			case 's':
				fgSel.red = args[0]; fgSel.green = args[1]; fgSel.blue = args[2];
				bgSel.red = args[3]; bgSel.green = args[4]; bgSel.blue = args[5];
				setselection( 0, 0);
				return (TRUE);
			case 'm':
				/* since args is memset to 0, this defaults to off */
				commandIsMeta = (args[0] == 1 ? true : false);
				return (TRUE);
			}
	}
	return (TRUE);
}


void
BTermView::newline( )
{

	if (reglen) {
		if (curline == curhome+regbase+reglen-1)
			shiftup( curhome+regbase, curhome+regbase+reglen, 1);
		else if (curline+1 < curhome+nrow)
			unless (inrange( ++curline))
				reclaim( );
	}
	else
		unless (inrange( ++curline))
			reclaim( );
}


uint
BTermView::convertattr( uint arg)
{

	uint bg = Line::background( attr);
	uint fg = Line::foreground( attr);
	uint ef = Line::effects( attr);
	switch (arg) {
	case 0:
		return (fill);
	case 1:
		ef |= Line::BOLD;
		break;
	case 4:
		ef |= Line::UNDERLINED;
		break;
	case 5:
		ef |= Line::BLINKING;
		break;
	case 7:
		ef |= Line::REVERSED;
		break;
	case 24:
		ef &= ~ Line::UNDERLINED;
		break;
	case 30:
		fg = Line::BLACK;
		ef |= Line::FGVALID;
		break;
	case 31:
		fg = Line::RED;
		ef |= Line::FGVALID;
		break;
	case 32:
		fg = Line::GREEN;
		ef |= Line::FGVALID;
		break;
	case 33:
		fg = Line::YELLOW;
		ef |= Line::FGVALID;
		break;
	case 34:
		fg = Line::BLUE;
		ef |= Line::FGVALID;
		break;
	case 35:
		fg = Line::MAGENTA;
		ef |= Line::FGVALID;
		break;
	case 36:
		fg = Line::CYAN;
		ef |= Line::FGVALID;
		break;
	case 37:
		fg = Line::WHITE;
		ef |= Line::FGVALID;
		break;
	case 40:
		bg = Line::BLACK;
		ef |= Line::BGVALID;
		break;
	case 41:
		bg = Line::RED;
		ef |= Line::BGVALID;
		break;
	case 42:
		bg = Line::GREEN;
		ef |= Line::BGVALID;
		break;
	case 43:
		bg = Line::YELLOW;
		ef |= Line::BGVALID;
		break;
	case 44:
		bg = Line::BLUE;
		ef |= Line::BGVALID;
		break;
	case 45:
		bg = Line::MAGENTA;
		ef |= Line::BGVALID;
		break;
	case 46:
		bg = Line::CYAN;
		ef |= Line::BGVALID;
		break;
	case 47:
		bg = Line::WHITE;
		ef |= Line::BGVALID;
		break;
	}
	return (Line::makeattribute( bg, fg, ef));
}


void
BTermView::reportcursor( )
{
	char	s[99];

	uint r = min( nrow, curline-curhome);
	uint c = min( ncol, curcol);
	sprintf( s, "\33[%d;%dR", r+1, c+1);
	BMessage *m = new BMessage;
	m->AddData( "", B_UINT8_TYPE, s, strlen( s));
	data >> m;
}


void
BTermView::shiftup( uint start, uint end, uint lcount)
{

	end = min( end, nel( linetab));
	if (start < end) {
		purge( );
		shiftsel( start, end, -lcount);
		uint l = start;
		uint a = min( lcount, end-start);
		uint b = end-start - a;
		memswap( (uchar *)&linetab[l], a*sizeof( *linetab), b*sizeof( *linetab));
		BRect src = Bounds( );
		src.top = (l+a) * vee;
		src.bottom = end*vee - 1.;
		BRect dst = src;
		dst.OffsetBy( 0, -(float)(a*vee));
		CopyBits( src, dst);
		clearlines( l+b, l+b+a);
	}
}


void
BTermView::shiftdown( uint start, uint end, uint lcount)
{

	end = min( end, nel( linetab));
	if (start < end) {
		purge( );
		shiftsel( start, end, lcount);
		uint l = start;
		uint a = min( lcount, end-start);
		uint b = end-start - a;
		memswap( (uchar *)&linetab[l], b*sizeof( *linetab), a*sizeof( *linetab));
		BRect src = Bounds( );
		src.top = l * vee;
		src.bottom = src.top + b*vee - 1.;
		BRect dst = src;
		dst.OffsetBy( 0, a*vee);
		CopyBits( src, dst);
		clearlines( l, l+a);
	}
}


void
BTermView::setregion( uint s, uint e)
{

	if (s)
		--s;
	unless (e)
		e = nrow;
	uint l = e - s;
	if (2 <= l && l <= nrow) {
		if (l == nrow)
			reglen = 0;
		else {
			regbase = s;
			reglen = l;
		}
		curline = curhome;
		curcol = 0;
	}
}


void
BTermView::purge( )
{

	if (eline.line) {
		if (eline.dirty) {
			eline.line->compact( this);
			drawline( eline.line-linetab);
		}
		eline.line = 0;
	}
}


void
BTermView::reclaim( )
{

	uint i = nel( linetab) / 2;
	unless (curline < i) {
		purge( );
		memswap( (uchar *)linetab, i*sizeof( *linetab), i*sizeof( *linetab));
		curline -= i;
		curhome -= min( curhome, i);
		clearlines( i, nel( linetab));
		shiftsel( 0, nel( linetab), -i);
	}
}


void
BTermView::clearlines( uint start, uint end)
{

	end = min( end, nel( linetab));
	if (start < end) {
		BRect r = Bounds( );
		r.top = start * vee;
		r.bottom = end*vee - 1;
		FillRect( r, B_SOLID_LOW );
		while (start < end)
			if (eline.line == linetab+start)
				linetab[start++].clear( this);
			else {
				Line &line = linetab[start++];;
				if (line.segments)
					free( line.segments);
				if (line.chars)
					free( line.chars);
				line.segments = 0;
				line.nsegment = 0;
				line.chars = 0;
				line.wrapcol = 0;
			}
	}
}


void
BTermView::clearcols( uint col, uint end)
{

	if (inrange( curline))
		linetab[curline].clear( this, col, end);
}


uint
BTermView::nexttab( uint col)
{

	col += tabwidth - col%tabwidth;
	unless (col < ncol)
		col = ncol? ncol-1: 0;
	return (col);
}


BTermView::Blinker::Blinker( BTermView *tv):
termview( tv)
{

	blinkrate = 1000000;
	blinktime = 0;
	visible = FALSE;
}


void
BTermView::Blinker::delay( )
{

	blinktime = system_time( ) + blinkrate;
	visible = TRUE;
}


void
BTermView::Blinker::MessageReceived( BMessage *m)
{

	bigtime_t now = system_time( );
	unless (now < blinktime)
		if (blinkrate) {
			blinktime = now + blinkrate;
			if (termview->active) {
				visible = not visible;
				termview->setcursor( visible);
				ELine &eline = termview->eline;
				if (eline.line) {
					uint lo = eline.lowdirt;
					uint hi = eline.dirty;
					eline.line->compact( termview);
					termview->drawline( eline.line-termview->linetab, lo, hi);
				}
			}
		}
		else
			blinktime = now + 1000000;
	BTimerSend( node, new BMessage, blinktime-now);
}


BTermView::SB::SB( BTermView *tv, RNode n):
RNodeHandler( n), termview( tv)
{

}


void
BTermView::SB::MessageReceived( BMessage *m)
{

	//printf( "BTermView::SB::MessageReceived: message=0x%X\n", m->what);
	switch (m->what) {
	case 'd':
		termview->scrolllock = TRUE;
		break;
	case 'u':
		termview->scrolllock = FALSE;
		if (termview->mqueued) {
			termview->process( termview->mqueued);
			delete termview->mqueued;
			termview->mqueued = 0;
		}
		termview->scroll( termview->Bounds( ).top);
		break;
	case 'v':
		termview->ScrollTo( 0, m->FindInt32( ""));
	}
}


BTermView::Boss::Boss( BTermView *tv, RNode n):
termview( tv), RNodeHandler( n)
{

}


void
BTermView::Boss::MessageReceived( BMessage *m)
{
	BFont	f;
	int8	e;
	uint	i;
	float	fs;

	//printf( "BTermView::Boss::MessageReceived: message='%.4s'\n", &m->what);
	switch (m->what) {
	case 80*24:
		setsize( 80, 24);
		break;
	case 80*25:
		setsize( 80, 25);
		break;
	case 80*40:
		setsize( 80, 40);
		break;
	case 132*24:
		setsize( 132, 24);
		break;
	case 132*25:
		setsize( 132, 25);
		break;
	case 'tabs':
		if (i = m->FindInt32( ""))
			termview->tabwidth = i;
		else
			m->AddInt32( "", termview->tabwidth);
		node >> Looper( )->DetachCurrentMessage( );
		break;
	case 'fnam':
		termview->changefont( m->FindString( ""));
		break;
	case 'fenc':
		m->FindInt8( "", &e);
		unless (e == termview->encoding) {
			termview->reencode( e);
			termview->encoding = e;
			termview->changefont( 0);
		}
		break;
	case 'fsiz':
		termview->GetFont( &f);
		fs = m->FindFloat( "");
		unless (fs)
			m->AddFloat( "", f.Size( ));
		else unless (fs == f.Size( ))
			termview->changefont( 0, fs);
		node >> Looper( )->DetachCurrentMessage( );
		break;
	case 'edit':
		m = new BMessage( 'edit');
		m->AddBool( "sel", termview->sels != termview->sele);
		m->AddBool( "clip", termview->queryclipboard( ));
		node >> m;
		break;
	case 'copy':
		termview->copyclipboard( );
		break;
	case 'pste':
		termview->pasteclipboard( );
		break;
	case 'sela':
		i = nel( termview->linetab);
		while ((i)
		and (not termview->linetab[i-1].nsegment))
			--i;
		termview->setselection( 0, i);
		break;
	case 'selw':
		m->AddBool( "sel?", termview->sels<termview->sele);
		node >> termview->Window( )->DetachCurrentMessage( );
		break;
	case 'selW':
		termview->exportselection( m, "sel");
		node >> termview->Window( )->DetachCurrentMessage( );
		break;
	case 'clrh':
		termview->clearlines( 0, nel( termview->linetab));
		termview->curline = 0;
		termview->curcol = 0;
		termview->scroll( 0);
		termview->setselection( 0, 0);
		break;
	case 'pref':
		termview->prefhandler( m);
		break;
	case 'log':
		m->FindPointer( "", (void **)&termview->log);
		break;
	case 'log0':
		fclose( termview->log);
		termview->log = 0;
		break;
	case 'find':
		termview->find( m);
		break;
	case 'sync':
		node >> termview->Window( )->DetachCurrentMessage( );
		break;
	case 'data':
		termview->data >> termview->Window( )->DetachCurrentMessage( );
	}
}


void
BTermView::Boss::setsize( uint nc, uint nr)
{

	BMessage *m = new BMessage( 'size');
	m->AddPoint( "", BPoint( nc*termview->em, nr*termview->vee));
	node >> m;
}


BTermView::Control::Control( BTermView *tv, RNode n):
termview( tv), RNodeHandler( n)
{

	winchrate = 1;
	if (char *p = getenv( "WINCHRATE"))
		winchrate = atof( p);
	winchtime = 0;
}


void
BTermView::Control::MessageReceived( BMessage *m)
{

	//printf( "BTermView::Control::MessageReceived: message=%d\n", m->what);
	switch (m->what) {
	case 'Gr&c':
		winsize w;
		w.ws_row = termview->nrow;
		w.ws_col = termview->ncol;
		w.ws_xpixel = w.ws_col * termview->em;
		w.ws_ypixel = w.ws_row * termview->vee;
		m = new BMessage( 'Gr&c');
		m->AddData( "", B_UINT8_TYPE, &w, sizeof w);
		node >> m;
		break;
	case 'pref':
		termview->boss.node >> termview->Window( )->DetachCurrentMessage( );
	}
}


void
BTermView::Control::sendwinch( )
{

	if (winchrate) {
		bigtime_t t = system_time( );
		if (winchtime < t) {
			bigtime_t i = 1000000 / winchrate;
			BTimerSend( node, new BMessage( 'sigW'), i);
			winchtime = t + i;
		}
	}
}


BTermView::Mouser::Mouser( BTermView *tv):
termview( tv)
{

	selecting = FALSE;
	SetDestination( *this);
}


void
BTermView::Mouser::MessageReceived( BMessage *m)
{
	BPoint	h;
	ulong	buttons;
	TextPos	t;

	termview->GetMouse( &h, &buttons);
	t.line = max( h.y, 0) / termview->vee;
	t.col = max( h.x, 0) / termview->em;
	unless (selecting) {
		uint modifiers = m->FindInt32( "modifiers");
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			shifted = FALSE;
			if ((modifiers & B_SHIFT_KEY)
			and (termview->sels != termview->sele)) {
				shifted = TRUE;
				if (t < (termview->sels+termview->sele)/2)
					pivot = termview->sele;
				else
					pivot = termview->sels;
			}
			else
				pivot = t;
			pivotpoint = h;
			clicks = m->FindInt32( "clicks");
			selecting = TRUE;
			BTimerSend( *this, new BMessage( 'tick'), 50000);
		}
		else if (buttons)
			termview->pasteselection( );
	}
	else {
		trackselection( t, h);
		if (buttons)
			BTimerSend( *this, new BMessage( 'tick'), 50000);
		else
			selecting = FALSE;
	}
}


void
BTermView::Mouser::trackselection( TextPos t, BPoint h)
{

	TextPos p = pivot;
	switch (clicks % 3) {
	case 1:
		if (t < p) {
			unless (shifted)
				++p.col;
		}
		else
			if (shifted)
				++t.col;
			else {
				BPoint d = pivotpoint - h;
				if (abs( d.x)+abs( d.y) < 2)
					t = p;
				else
					++t.col;
			}
		break;
	case 2:
		if (t < p) {
			t = termview->prevword( t);
			unless (shifted)
				p = termview->nextword( p);
		}
		else {
			t = termview->nextword( t);
			unless (shifted)
				p = termview->prevword( p);
		}
		break;
	default:
		if (t < p) {
			unless (shifted) {
				++p.line;
				p.col = 0;
			}
		}
		else {
			unless (shifted)
				p.col = 0;
			++t.line;
		}
		t.col = 0;
	}
	termview->setselection( p, t);

	BRect b = termview->Bounds( );
	if (h.y < b.top)
		termview->scroll( h.y);
	else if (h.y > b.bottom)
		termview->scroll( h.y-b.Height( ));
}


BTermView::Line::Line( )
{

	nsegment = 0;
	segments = 0;
	chars = 0;
	wrapcol = 0;
}


/*
 * (`end' is merely an efficiency hint)
 */
BTermView::ELine	*
BTermView::Line::expand( BTermView *v, uint end)
{

	ELine &eline = v->eline;
	if (eline.line) {
		if (eline.line == this)
			return (&eline);
		if (eline.dirty) {
			uint lo = eline.lowdirt;
			uint hi = eline.dirty;
			eline.line->compact( v);
			v->drawline( eline.line-v->linetab, lo, hi);
		}
	}
	eline.line = this;
	eline.lowdirt = 0;
	eline.dirty = 0;
	uint col = 0;
	uchar *cp = chars;
	for (uint i=0; i<nsegment; ++i) {
		unless (col < end) {
			eline.lowdirt = col;
			eline.dirty = ~0;
			break;
		}
		segment &s = segments[i];
		uint a = s.attr;
		while (col < s.end) {
			uint c = *cp++;
			if ((c >= 0x80)
			and (v->encoding == B_UNICODE_UTF8)) {
				if (c < 0xE0)
					c = c & 0x1F;
				else {
					c = c & 0x0F;
					c = c<<6 | *cp++&0x3F;
				}
				c = c<<6 | *cp++&0x3F;
			}
			eline.cells[col] = makecode( a, c);
			++col;
		}
	}
	eline.ncell = col;
	eline.wrapcol = wrapcol;
	return (&eline);
}


void
BTermView::Line::compact( BTermView *v)
{

	ELine &eline = v->eline;
	if (segments) {
		free( segments);
		segments = 0;
		nsegment = 0;
	}
	if (chars) {
		free( chars);
		chars = 0;
	}
	wrapcol = eline.wrapcol;
	uint cmax = eline.ncell;
	if (cmax) {
		segment stab[nel( eline.cells)];
		uint si = 0;
		uint ci = 0;
		uint lasta = attribute( eline.cells[ci]);
		while (++ci < cmax) {
			uint a = attribute( eline.cells[ci]);
			unless (a == lasta) {
				stab[si].attr = lasta;
				stab[si].end = ci;
				++si;
				lasta = a;
			}
		}
		stab[si].attr = lasta;
		stab[si].end = ci;
		++si;
		uchar ctab[3*nel( eline.cells)];
		uchar *cp = ctab;
		for (ci=0; ci<cmax; ++ci) {
			uint c = character( eline.cells[ci]);
			if ((c < 0x0080)
			or (v->encoding != B_UNICODE_UTF8))
				*cp++ = c;
			else if (c < 0x0800) {
				*cp++ = 0xC0 | c>>6;
				*cp++ = 0x80 | c&0x003F;
			}
			else {
				*cp++ = 0xE0 | c>>12;
				*cp++ = 0x80 | c>>6&0x003F;
				*cp++ = 0x80 | c&0x003F;
			}
		}
		if (chars = (uchar *)malloc( cp-ctab)) {
			memcpy( chars, ctab, cp-ctab);
			if (segments = (segment *)malloc( si*sizeof( *segments))) {
				memcpy( segments, stab, si*sizeof( *segments));
				nsegment = si;
			}
		}
	}
	eline.lowdirt = 0;
	eline.dirty = 0;
}


void
BTermView::Line::clear( BTermView *v, uint col, uint end)
{
	ELine	*e;

	if (end == ~0) {
		e = expand( v, col);
		if (col < e->ncell) {
			e->ncell = col;
			e->lowdirt = e->dirty? min( e->lowdirt, col): col;
			e->dirty = ~0;
		}
	}
	else {
		e = expand( v);
		end = min( end, e->ncell);
		for (uint i=col; i<end; ++i)
			e->cells[i] = makecode( v->fill, ' ');
		e->lowdirt = e->dirty? min( e->lowdirt, col): col;
		e->dirty = max( e->dirty, end);
	}
}


uint
BTermView::Line::ncell( BTermView *v)
{

	ELine &eline = v->eline;
	if ((eline.line)
	and (eline.line == this))
		return (eline.ncell);
	if (nsegment)
		return (segments[nsegment-1].end);
	return (0);
}


uint
BTermView::Line::get( BTermView *v, uint col)
{

	ELine *e = expand( v);
	if (col < e->ncell)
		return (e->cells[col]);
	return (v->fill | ' ');
}


void
BTermView::Line::set( BTermView *v, uint col, uint code)
{

	ELine *e = expand( v);
	if (col < nel( e->cells)) {
		until (col < e->ncell)
			e->cells[e->ncell++] = makecode( v->fill, ' ');
		e->cells[col] = code;
		if (col >= e->dirty) {
			unless (e->dirty)
				e->lowdirt = col;
			e->dirty = col + 1;
		}
		else if (col < e->lowdirt)
			e->lowdirt = col;
	}
}


void
BTermView::Line::wrap( BTermView *v)
{

	expand( v)->wrapcol = v->ncol;
}


void
BTermView::Line::draw( BTermView *v, uint l, uint start, uint end)
{
	static rgb_color ANSI_TABLE[] = {
		{255, 255, 255, 255},			// white
		{0, 0, 0, 255},						// black
		{255, 0, 0, 255},					// red
		{0, 255, 0, 255},					// green
		{255, 255, 0, 255},				// yellow
		{0, 0, 255, 255},					// blue
		{255, 0, 255, 255},				// magenta
		{0, 255, 255, 255}				// cyan
	};

	// save this, we may need it later
	rgb_color saveLow = v->LowColor();
	BPoint pl( 0+v->xcharadjust, v->ascent+l*v->vee+v->ycharadjust);
	v->MovePenTo( pl);
	rgb_color curfg = v->fgColor;
	uint ss = ~0;
	uint se = ~0;
	unless (l < v->sels.line)
		if (l == v->sels.line) {
			ss = v->sels.col;
			if (l == v->sele.line)
				se = v->sele.col;
		}
		else if (l < v->sele.line)
			ss = 0;
		else if (l == v->sele.line) {
			ss = 0;
			se = v->sele.col;
		}
	uint si = 0;
	uint col = 0;
	uchar *cp = chars;
	uint n = 0;
	loop {
		unless (n) {
			unless (si < nsegment) {
				if ((ss < ~0)
				and (col < se)) {
					BRect r;
					r.top = l * v->vee;
					r.right = se == ~0? 9999: se*v->em - 1;
					r.bottom = r.top + v->vee - 1;
					if (col < ss)
						r.left = ss * v->em;
					else if (col < se)
						r.left = col * v->em;
					curfg = v->bgSel;
					v->SetHighColor( v->bgSel );
					v->FillRect( r);
				}
				break;
			}
			n = segments[si].end - col;
			++si;
		}
		if (col < start) {
			uint i = min( n, start-col);
			pl.x += i * v->em;
			v->MovePenTo( pl);
			col += i;
			cp += v->encoding==B_UNICODE_UTF8? utf8len( cp, i): i;
			n -= i;
		}
		else if (col < end) {
			uint i = min( n, end-col);
			if (col < ss)
				i = min( i, ss-col);
			else if (col < se)
				i = min( i, se-col);
			segment &s = segments[si-1];

			rgb_color bg = v->bgColor;
			rgb_color fg = v->fgColor;

			// ANSI escape sequence color handling
			if (effects( s.attr) & BGVALID)
			{
				bg = ANSI_TABLE[background( s.attr )];
			}
			if (effects( s.attr) & FGVALID)
			{
				fg = ANSI_TABLE[foreground( s.attr )];
			}

			if (ss <= col && col < se) {
				bg = v->bgSel;
				fg = v->fgSel;
			}
			else if (effects( s.attr) & REVERSED) {
				rgb_color u = bg;
				bg = fg;
				fg = u;
			}
			if (effects( s.attr) & CURSOR) {
				if ((bg == v->bgCursor)
				and (fg == v->fgCursor)) {
					bg = v->fgCursor;
					fg = v->bgCursor;
				}
				else {
					bg = v->bgCursor;
					fg = v->fgCursor;
				}
			}
			unless (bg == v->bgColor) {
				BPoint a = pl;
				a.x -= v->xcharadjust;
				a.y -= v->ascent + v->ycharadjust;
				BRect r( a.x, a.y, a.x+i*v->em-1, a.y+v->vee-1);
				curfg = bg;
				v->SetHighColor( curfg );
				v->FillRect( r);
				// for that nice anti-aliased effect
				v->SetLowColor( curfg );
			}
			unless (fg == curfg) {
				curfg = fg;
				v->SetHighColor( curfg );
			}
			if (effects( s.attr) & UNDERLINED) {
				uint y = pl.y;
				uint n = v->fsize/24 + 1;
				y += n;
				unless (v->fsize < 18)
					++y;
				unless (n == 1)
					v->SetPenSize( n);
				v->StrokeLine( BPoint( pl.x, y), BPoint( pl.x+i*v->em-1, y));
				unless (n == 1)
					v->SetPenSize( 1);
				v->MovePenTo( pl);
			}
			uint cn = v->encoding==B_UNICODE_UTF8? utf8len( cp, i): i;
			if (effects( s.attr) & BOLD) {
				v->MovePenTo( pl.x+1, pl.y-1);
				v->DrawString( (char *)cp, cn);
				v->MovePenTo( pl);
				if (v->fsize < 24)
					v->SetDrawingMode( B_OP_BLEND);
				else
					v->SetDrawingMode( B_OP_OVER);
				v->DrawString( (char *)cp, cn);
				v->SetDrawingMode( B_OP_COPY);
			}
			else
				v->DrawString( (char *)cp, cn);
			pl.x += i * v->em;
			col += i;
			cp += cn;
			n -= i;
			unless (bg == v->bgColor)
			{
				// reset the low color
				v->SetLowColor( saveLow);
			}
		}
		else
			break;
	}
	unless (curfg == v->fgColor)
	{
		v->SetHighColor( v->fgColor );
	}
}


void
BTermView::Line::shiftleft( BTermView *v, uint ccount)
{

	ELine *e = expand( v);
	uint maxcol = e->pad( v->ncol, makecode( v->fill, ' '));
	if (v->curcol < maxcol) {
		uint a = min( ccount, maxcol-v->curcol);
		uint b = maxcol-v->curcol - a;
		memmove( e->cells+v->curcol, e->cells+v->curcol+a, b*sizeof( *e->cells));
		clear( v, v->curcol+b);
		e->lowdirt = min( e->lowdirt, v->curcol);
		e->dirty = ~0;
	}
}


void
BTermView::Line::shiftright( BTermView *v, uint ccount)
{

	ELine *e = expand( v);
	uint maxcol = e->pad( v->ncol, makecode( v->fill, ' '));
	if (v->curcol < maxcol) {
		uint a = min( ccount, maxcol-v->curcol);
		uint b = maxcol-v->curcol - a;
		memmove( e->cells+v->curcol+a, e->cells+v->curcol, b*sizeof( *e->cells));
		clear( v, maxcol);
		clear( v, v->curcol, v->curcol+a);
		e->dirty = ~0;
	}
}


uint
BTermView::Line::makecode( uint a, uint c)
{

	return (a<<bitsof( ushort) | c);
}


uint
BTermView::Line::makeattribute( uint bg, uint fg, uint ef)
{

	return (bg<<BACKGROUND | fg<<FOREGROUND | ef&~(~0<<FOREGROUND));
}


uint
BTermView::Line::character( uint code)
{

	return (code & ~(~0<<bitsof( ushort)));
}


uint
BTermView::Line::attribute( uint code)
{

	return (code >> bitsof( ushort));
}


uint
BTermView::Line::background( uint attr)
{

	return (attr>>BACKGROUND & 7);
}


uint
BTermView::Line::foreground( uint attr)
{

	return (attr>>FOREGROUND & 7);
}


uint
BTermView::Line::effects( uint attr)
{

	return (attr);
}


BTermView::ELine::ELine( )
{

	line = 0;
	dirty = 0;
	lowdirt = 0;
	ncell = 0;
}


uint
BTermView::ELine::pad( uint n, uint code)
{

	n = min( n, nel( cells));
	while (ncell < n)
		cells[ncell++] = code;
	return (n);
}


bool
BTermView::warn( char *mesg)
{

	BMessage *m = new BMessage( 'warn');
	m->AddString( "", mesg);
	boss.node >> m;
	return (FALSE);
}


static uchar	*
memswap( uchar mem[], uint len1, uint len2)
{
	static uchar	*tmp;
	static uint	tmplen;

	unless (tmp) {
		tmplen = len1;
		tmp = (uchar *) malloc( tmplen);
	}
	else if (tmplen < len1) {
		tmplen = len1;
		tmp = (uchar *) realloc( tmp, tmplen);
	}
	if (tmp) {
		memcpy( tmp, mem, len1);
		memmove( mem, mem+len1, len2);
		memcpy( mem+len2, tmp, len1);
	}
	return (mem);
}


uint
utf8len( uchar *p, uint n)
{

	uchar *p0 = p;
	while (n) {
		switch (*p >> 5) {
		case 6:
			p += 2;
			break;
		case 7:
			p += 3;
			break;
		default:
			p += 1;
		}
		--n;
	}
	return (p - p0);
}


static void
xcopy( XString *x, uchar *cp, uint encoding)
{

	if (encoding == B_UNICODE_UTF8)
		while (uint c = *cp++) {
			if (c >= 0x80) {
				if (c < 0xE0)
					c = c & 0x1F;
				else {
					c = c & 0x0F;
					c = c<<6 | *cp++&0x3F;
				}
				c = c<<6 | *cp++&0x3F;
			}
			x->putw( c);
		}
	else {
		int32 sl = strlen( (char *)cp);
		int32 dl = sl;
		uchar *d = (uchar *) malloc( dl);
		int32 st = 0;
		if (convert_from_utf8( e2c( encoding), (char *)cp, &sl, (char *)d, &dl, &st, 1) == B_OK)
			for (uint i=0; i<dl; ++i)
				x->putw( d[i]);
		free( d);
	}
	x->putw( 0);
}


static uint
e2c( uint e)
{

	switch (e) {
	case B_UNICODE_UTF8:
		return (B_UNICODE_CONVERSION);
	case B_ISO_8859_1:
		return (B_ISO1_CONVERSION);
	case B_ISO_8859_2:
		return (B_ISO2_CONVERSION);
	case B_ISO_8859_3:
		return (B_ISO3_CONVERSION);
	case B_ISO_8859_4:
		return (B_ISO4_CONVERSION);
	case B_ISO_8859_5:
		return (B_ISO5_CONVERSION);
	case B_ISO_8859_6:
		return (B_ISO6_CONVERSION);
	case B_ISO_8859_7:
		return (B_ISO7_CONVERSION);
	case B_ISO_8859_8:
		return (B_ISO8_CONVERSION);
	case B_ISO_8859_9:
		return (B_ISO9_CONVERSION);
	case B_ISO_8859_10:
		return (B_ISO10_CONVERSION);
	}
	return (B_MAC_ROMAN_CONVERSION);
}


static bool
boolvar( BMessage *m, char *name)
{
	bool	b;

	if (m->FindBool( name, &b) == B_OK)
		return (b);
	return (FALSE);
}
