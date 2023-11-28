

#include	<Screen.h>
#include	<Bitmap.h>
#include	<Button.h>
#include	<string.h>
#include	<stdlib.h>
#include	"rico.h"
#include	"RNode.h"
#include	"buttons.h"


rgb_color BitmapButton::gray = {
	232, 232, 232
};
BRect	InfoButton::rect( 0, 0, 34, 34);
BRect	ArrowButton::rect( 0, 0, 39, 18);


BitmapButton::BitmapButton( BPoint org, BBitmap *e, BBitmap *d, BMessage *m):
BButton( bsize( e), 0, "", m)
{

	MoveTo( org);
	ebm = e;
	dbm = d;
}


BitmapButton::~BitmapButton( )
{

	delete ebm;
	if (dbm)
		delete dbm;
}


void
BitmapButton::Draw( BRect r)
{

	BButton::Draw( r);
	MovePenTo( 5, 5);
	if ((IsEnabled( ))
	or (not dbm))
		DrawBitmap( ebm);
	else
		DrawBitmap( dbm);
	if (Value( )) {
		r = ebm->Bounds( );
		r.OffsetBy( 5, 5);
		InvertRect( r);
	}
	else if (IsFocus( )) {
		BRect b = ebm->Bounds( );
		b.OffsetBy( 5, 5);
		b.InsetBy( 5, 5);
		b.OffsetBy( 0, 4);
		SetHighColor( 0, 0, 229);
		StrokeLine( b.LeftBottom( ), b.RightBottom( ));
		SetHighColor( 255, 255, 255);
		b.OffsetBy( 0, 1);
		StrokeLine( b.LeftBottom( ), b.RightBottom( ));
	}
}


BRect
BitmapButton::bsize( BBitmap *b)
{
	BRect	r;

	r = b->Bounds( );
	r.right += 9;
	r.bottom += 9;
	return (r);
}


InfoButton::InfoButton( BPoint org, int mesg):
BitmapButton( org, new BM, 0, new BMessage( mesg))
{

}


InfoButton::BM::BM( ):
BBitmap( rect, B_COLOR_8_BIT, TRUE)
{

	AddChild( new V( this));
}


void
InfoButton::BM::smooth( )
{
	BScreen	s;

	uint xmax = BytesPerRow( );
	uint ymax = Bounds( ).IntegerHeight( ) + 1;
	uchar *p = (uchar *) malloc( ymax*xmax);
	for (uint i=0; i<ymax*xmax; ++i)
		p[i] = s.IndexForColor( gray);
	for (uint y=1; y<ymax-1; ++y)
		for (uint x=1; x<xmax-1; ++x) {
			double v = 0;
			v += .5 * s.ColorForIndex( ((uchar *)Bits( ))[(y-1)*xmax+x-1]).red;
			v += 1. * s.ColorForIndex( ((uchar *)Bits( ))[(y-1)*xmax+x+0]).red;
			v += .5 * s.ColorForIndex( ((uchar *)Bits( ))[(y-1)*xmax+x+1]).red;
			v += 1. * s.ColorForIndex( ((uchar *)Bits( ))[(y+0)*xmax+x-1]).red;
			v += 1. * s.ColorForIndex( ((uchar *)Bits( ))[(y+0)*xmax+x+0]).red;
			v += 1. * s.ColorForIndex( ((uchar *)Bits( ))[(y+0)*xmax+x+1]).red;
			v += .5 * s.ColorForIndex( ((uchar *)Bits( ))[(y+1)*xmax+x-1]).red;
			v += 1. * s.ColorForIndex( ((uchar *)Bits( ))[(y+1)*xmax+x+0]).red;
			v += .5 * s.ColorForIndex( ((uchar *)Bits( ))[(y+1)*xmax+x+1]).red;
			v /= 7;
			v += .75 * (v-30);
			v = min( 255, max( v, 0));
			v = v/255 * gray.red;
			p[y*xmax+x] = s.IndexForColor( v, v, v);
		}
	memcpy( Bits( ), p, ymax*xmax);
	free( p);
}


InfoButton::BM::V::V( BM *p):
BView( rect, 0, 0, 0)
{

	bm = p;
}


void InfoButton::BM::V::AttachedToWindow( )
{

	draw1( );
	Sync( );
	bm->smooth( );
	draw2( );
	Sync( );
}


void
InfoButton::BM::V::draw1( )
{

	SetPenSize( 3);
	SetHighColor( 0, 0, 0);
	StrokeArc( BPoint( 17, 17), 13, 13, 0, 360);
}


void
InfoButton::BM::V::draw2( )
{

	SetFont( be_fixed_font);
	SetFontSize( 24);
	MovePenTo( 10, 23);
	DrawString( "i");
}


ArrowButton::ArrowButton( BPoint org, bool flipped, int mesg):
BitmapButton( org, new BM( flipped, TRUE), new BM( flipped, FALSE), new BMessage( mesg))
{

}


ArrowButton::BM::BM( bool flipped, bool enabled):
BBitmap( rect, B_COLOR_8_BIT, TRUE)
{

	AddChild( new V( flipped, enabled));
}


ArrowButton::BM::V::V( bool f, bool e):
BView( rect, 0, 0, 0)
{

	flipped = f;
	enabled = e;
}


void
ArrowButton::BM::V::AttachedToWindow( )
{
	const float a[][2] = {
		 0,  0,
		14,  7,
		 4,  7,
		 4, 10,
		 0, 10,
		 0,  0,
	};
	BPoint	b[2*nel( a)];

	SetHighColor( gray);
	FillRect( rect);
	SetHighColor( 0, 0, 0);
	for (uint i=0; i<nel( a); ++i) {
		b[i+0*nel( a)].x = 20 + a[i][0];
		b[i+1*nel( a)].x = 20 - a[i][0];
		float y = a[i][1];
		if (flipped)
			y = 12 - y;
		y += 3;
		b[i+0*nel( a)].y = y;
		b[i+1*nel( a)].y = y;
	}
	unless (enabled)
		SetHighColor( 168, 168, 168);
	FillPolygon( b, nel( b));
	Sync( );
}
