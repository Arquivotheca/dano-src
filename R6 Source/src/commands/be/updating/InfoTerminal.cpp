#include "InfoTerminal.h"
#include "Geometry.h"
#include <math.h>
#include <string.h>
#include <Bitmap.h>
#include <stdio.h>

inline float fmin( float x, float y );

inline float fmin( float x, float y )
{
	return x < y ? x : y;
}

inline float fmax( float x, float y );

inline float fmax( float x, float y )
{
	return x > y ? x : y;
}

inline uint8 add_pixel( uint16 x );

inline uint8 add_pixel( uint8 a, uint8 b )
{
	uint16 c = a+b;
	return c < 255 ? c : 255;
}

static const float kApproachRate = 0.08;
static const float kLimit = 1.0e-2;

InfoTerminal::InfoTerminal( BRect frame, const char *name, uint32 resizeMask, uint32 flags )
	: BRenderView( frame, B_RGB32, name, resizeMask, flags ),
	fLineBM( NULL ),
	fOptions( IT_SCALE_HEIGHT | IT_SCALE_COLOR | IT_FROM_CENTER | IT_BLUR | IT_LINEAR | IT_ADD_PIXELS ),
	fApproachRate( kApproachRate ),
	fBlurWidth( 6.0 ),
	fAlignment( B_ALIGN_LEFT )
{
	SetFrameRate( 30 );
	fHighBG.red = 50; fHighBG.green = 0; fHighBG.blue = 0; fHighBG.alpha = 255;
	fLowBG.red = 0; fLowBG.green = 0; fLowBG.blue = 0; fLowBG.alpha = 255;
	fTextColor.red = 250; fTextColor.green = 50; fTextColor.blue = 50; fTextColor.alpha = 255;
}

InfoTerminal::~InfoTerminal( void )
{
	if( fLineBM )
		delete fLineBM;
}

void InfoTerminal::SetText( const char *text )
{
	fTextLock.Lock();
	fText.SetTo( text );
	fTextLock.Unlock();
	Display();
}

void InfoTerminal::SetText( const char *text, int32 length )
{
	fTextLock.Lock();
	fText.SetTo( text, length );
	fTextLock.Unlock();
	Display();
}

void InfoTerminal::SetFont( const BFont &font )
{
	fTextLock.Lock();
	fRenderFont = font;
	fRenderFont.GetHeight( &fFontHeight );
	fBlurWidth = fRenderFont.Size();
	fTextLock.Unlock();
}

void InfoTerminal::SetAlignment( alignment flag )
{
	fAlignment = flag;
}

void InfoTerminal::SetColors( rgb_color bg_color, rgb_color text, float raster )
{
	
	fHighBG = bg_color;
	fLowBG.alpha = 255;
	fTextColor = text;
	
	uint8 *c1 = (uint8 *)&fLowBG, *c2 = (uint8 *)&bg_color;
	float c;
	
	for( int32 i=0; i<3; i++, c1++, c2++ )
	{
		c = float(*c2) * raster;
		c = c >= 0 ? c : 0;
		c = c < 256 ? c : 255;
		*c1 = uint8(c);
	}
}

void InfoTerminal::SetOptions( uint32 options )
{
	fOptions = options;
}

void InfoTerminal::SetApproachRate( float rate )
{
	fApproachRate = rate;
}

void InfoTerminal::Display( void )
{
	Init();
	Resume();
}

void InfoTerminal::Init( void )
{
	fInit = true;
}

void InfoTerminal::BitmapInit( BBitmap **bmap, BRect bounds, color_space depth )
{
	if( *bmap )
		delete *bmap;
	bounds.right += 1.0;
	*bmap = new BBitmap( bounds, depth, true );
	fBMview = new BView( bounds, "", B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW );
	(*bmap)->AddChild( fBMview );
	
	Init();
}

bool InfoTerminal::ShouldSuspend( void )
{
	return fComplete;
}

void InfoTerminal::ThreadInit( void )
{
	Init();
}

//status_t InfoTerminal::ThreadExit( void )

BRect InfoTerminal::Render( void )
{
	fTextLock.Lock();
	if( fInit )
	{
		fComplete = false;
		fCycle = 0;
		fLineNumber = 0;
		fLineScale = 0;
		fCharOffset = 0;
		fInit = false;
	}
	
	BBitmap *bmap = OffscreenBitmap();
	BRect bounds = bmap->Bounds();
	BRect lineRect;
	BView *view = OffscreenView();
	
	// If first pass, render entire background
	if( fCycle == 0 )
	{
		RenderBG( bounds );
		view->Sync();
	}
	else
	{
		float charHeight = fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading;
		float lineOffset = (charHeight * (fLineNumber-1))+5;
		
		if( lineOffset+charHeight > bounds.bottom )
			fComplete = true; // We can stop now; end of bitmap reached
		else
		{
			lineRect.Set( 0, lineOffset, bounds.right, lineOffset+charHeight );
			// replace active line with BG
			RenderBG( lineRect );
			view->Sync();
			
			BPoint offset;
			offset.y = lineOffset;
			
			switch( fAlignment )
			{
				case B_ALIGN_RIGHT:
					offset.x = bounds.Width()-5-fLineWidth;
					break;
				case B_ALIGN_CENTER:
					offset.x = (bounds.Width()-fLineWidth)/2;
					break;
				case B_ALIGN_LEFT:
				default:
					offset.x = 5;
					break;
			}
			
			RenderLine( offset, fLineScale, fLineBM );
		}
		//view->Sync();
	}
	
	// Has the previous line been completed?
	if( fLineScale == 0 )
	{
		// Setup next line
		fLineNumber++;
		fLineScale = 0.95;
		const char *next = fText.String()+fCharOffset;
		
		// Remove leading CR LFs
		while( *next && strchr( "\r\n", *next ) )
			next++;
		
		// Have we reached the end of the text yet?
		if( *next )
		{
			BString line;
			BString word;
			int32 length;
			int32 totalLength = 0;
			char c;
			
			// Find end of line
			float totalWidth = 0;
			while(true)
			{
				length = strcspn( next+totalLength, "\r\n \t" );
				c = next[totalLength+length];
				if( (c == '\r') || (c == '\n') || (c == '\0') )
				{
					totalLength += length;
					break;
				}
				length += 1;
				totalWidth += fRenderFont.StringWidth( next+totalLength, length );
				if( totalWidth < bounds.right )
					totalLength += length;
				else
					break;
			}
			
			fCharOffset = next - fText.String() + totalLength;
			line.SetTo( next, totalLength );
			fLineWidth = fRenderFont.StringWidth( line.String() );
			
			if( fLineBM )
				delete fLineBM;
			
			fLineBM = MakeLineBM( &line );
		}
		else
			fComplete = true; // We can stop now
	}
	else if( fLineScale < kLimit ) // Are we close enough to the limit?
		fLineScale = 0; // Render just one last time at full size
	else if( fOptions & IT_LINEAR )
		fLineScale -= fApproachRate;
	else
		fLineScale *= 1.0-fApproachRate;
	
	fTextLock.Unlock();
	
	if( fCycle++ == 0 )
		return BoundsCache();
	else
		return lineRect;
}

void InfoTerminal::RenderBG( BRect bounds )
{
	BView *view = OffscreenView();
	BPoint start, end;
	
	view->SetHighColor( fLowBG );
	view->FillRect( bounds );
	
	view->BeginLineArray( (bounds.bottom-bounds.top)/2 + 2 );
	start.x = bounds.left;
	end.x = bounds.right;
	end.y = start.y = floor(bounds.top) - float(int32( bounds.top ) & 1L );
	for( ; start.y <= bounds.bottom+1; start.y += 2, end.y += 2 )	
		view->AddLine( start, end, fHighBG );
	view->EndLineArray();
}

// This thing could use a we bit of optimizing
void InfoTerminal::RenderLine( BPoint offset, float scale, BBitmap *srcBM )
{
	float happy = fOptions & IT_FROM_CENTER ? (fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading)*0.5*scale : 0;
	float fhappy = floor(happy);
	
	BBitmap *dstBM = OffscreenBitmap();
	BRect dstRect( srcBM->Bounds() );
	BRect srcRect( dstRect );
	
	dstRect.top += happy;
	dstRect.bottom -= happy;
	dstRect.OffsetBy( offset );
	if( dstRect.right > BoundsCache().right )
		dstRect.right = BoundsCache().right;
	
	float fcolor[3];
	
	if( fOptions & IT_SCALE_COLOR )
	{
		if( fOptions & IT_SCALE_DOWN )
		{
			float bgc, delta;
			uint8 *textColor = (uint8 *)&fTextColor;
			uint8 *highBG = (uint8 *)&fHighBG;
			uint8 *lowBG = (uint8 *)&fLowBG;
			float curve = powf(1.0-scale, fOptions & IT_BLUR ? 0.3 : 0.5);
			
			for( uint8 i=0; i<3; i++ )
			{
				bgc = ((float(*highBG++)+float(*lowBG++))/2);
				delta = float(*textColor++)-bgc;
				fcolor[i] = bgc + (delta*curve);
			}
		}
		else
		{
			float colorGain = powf( 1.0/((1.0-scale)), 4 );
			fcolor[0] = float(fTextColor.red)*colorGain;
			fcolor[1] = float(fTextColor.green)*colorGain;
			fcolor[2] = float(fTextColor.blue)*colorGain;
		}
	}
	else
	{
		fcolor[0] = fTextColor.red;
		fcolor[1] = fTextColor.green;
		fcolor[2] = fTextColor.blue;
	}
	
	fcolor[0] = fmax( fmin( fcolor[0], 255.0 ), 0.0 )/255.0;
	fcolor[1] = fmax( fmin( fcolor[1], 255.0 ), 0.0 )/255.0;
	fcolor[2] = fmax( fmin( fcolor[2], 255.0 ), 0.0 )/255.0;
		
	float srcLevel;
	
	float textScale = fOptions & IT_SCALE_HEIGHT ? 1.0-scale : 1.0;
	
	BMatrix2D m( BMatrix2D().Translation( -offset.x, -offset.y-happy ) * BMatrix2D().Scale( 1.0, 1.0/(textScale) ) );
	
	int32 srcRowBytes = srcBM->BytesPerRow();
	int32 dstRowBytes = dstBM->BytesPerRow();
	BPoint dstPt;
	BPoint srcPt;
	
	uint8 *srcBase = (uint8 *)srcBM->Bits();
	uint8 *dstRow = ((uint8 *)dstBM->Bits()) + (int32(offset.y+fhappy)*dstRowBytes) + (int32(offset.x)*sizeof(rgb_color));
	rgb_color *dstPixel;
	uint8 *srcPixel;
	
	uint32 add = fOptions & IT_ADD_PIXELS;
	
	for( dstPt.y=dstRect.top; dstPt.y < dstRect.bottom; dstPt.y += 1.0, dstRow += dstRowBytes )
	{
		dstPixel = (rgb_color *)dstRow;
		for( dstPt.x=dstRect.left; dstPt.x < dstRect.right; dstPt.x += 1.0, dstPixel++ )
		{
			srcPt = m.Transform( dstPt );
			srcPt.ConstrainTo( srcRect );
			srcPixel = srcBase + int32(srcPt.y)*srcRowBytes + int32(srcPt.x);
				
			if( *srcPixel )
			{
				srcLevel = float(*srcPixel << 2);
				if( add )
				{
					dstPixel->blue = add_pixel( dstPixel->blue, uint8(fcolor[0]*srcLevel) );
					dstPixel->green = add_pixel( dstPixel->green, uint8(fcolor[1]*srcLevel) );
					dstPixel->red = add_pixel( dstPixel->red, uint8(fcolor[2]*srcLevel) );
				}
				else
				{
					dstPixel->blue = uint8(fcolor[0]*srcLevel);
					dstPixel->green = uint8(fcolor[1]*srcLevel);
					dstPixel->red = uint8(fcolor[2]*srcLevel);
				}
			}
		}
	}
	if( (fOptions & IT_BLUR) && (scale != 0) )
	//if( true )
	{
		//uint16 b1, b2, b3, b4, b5;
		int8 a1, a2, a3, a4, a5;
		float fuzz = fBlurWidth;
		float blurWidth = fuzz*scale*5;
		dstRect.left -= blurWidth;
		dstRect.right += blurWidth;
		
		if( dstRect.left < 0 )
			dstRect.left = 0;
		if( dstRect.right > BoundsCache().right )
			dstRect.right = BoundsCache().right;
		
		a1 = 4*int8(ceil(fuzz*scale));
		a2 = 4*int8(ceil(fuzz*scale*2.0));
		a3 = 4*int8(ceil(fuzz*scale*3.0));
		a4 = 4*int8(ceil(fuzz*scale*4.0));
		a5 = 4*int8(ceil(fuzz*scale*5.0));
		
		uint16 b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11;
		uint8 *d;
		int32 i;
		
		dstRow = ((uint8 *)dstBM->Bits()) + (int32(offset.y+happy)*dstRowBytes) + (int32(dstRect.left)*sizeof(rgb_color));
		
		for( dstPt.y=dstRect.top; dstPt.y < dstRect.bottom; dstPt.y += 1.0, dstRow += dstRowBytes )
		{
			dstPixel = (rgb_color *)dstRow;
			for( dstPt.x=dstRect.left; dstPt.x < dstRect.right; dstPt.x += 1.0, dstPixel++ )
			{
				for( d = (uint8 *)dstPixel, i=0; i<3; i++, d++ )
				{
					
					/*b1 = *d;
					b2 = d[4];
					b3 = d[-4];
					b4 = d[8];
					b5 = d[-8];
					*d = uint8(((b1<<2)+b2+b3+b4+b5)>>3);*/
					
					b1 = d[-a5]; // -5
					b2 = d[-a4]; // -4
					b3 = d[-a3]; // -3
					b4 = d[-a2]; // -2
					b5 = d[-a1]; // -1
					b6 = d[0]; // 0
					b7 = d[a1]; // 1
					b8 = d[a2]; // 2
					b9 = d[a3]; // 3
					b10 = d[a4]; // 4
					b11 = d[a5]; // 5
					
					*d = uint8( ((b6<<5)+((b5+b7)<<4)+((b4+b8)<<3)+((b3+b9)<<2)+((b1+b2+b10+b11)<<1))/96 );
				}
			}
		}
	}
	//OffscreenView()->DrawBitmap( srcBM, srcRect, dstRect );
}

BBitmap *InfoTerminal::MakeLineBM( const BString *s )
{
	float width = fRenderFont.StringWidth( s->String() );
	BView *view;
	BBitmap *bm;
	
	BRect bounds( 0, 0, width, fFontHeight.ascent+fFontHeight.descent );
	
	bm = new BBitmap( bounds, B_GRAY8, true );
	view = new BView( bounds, "", B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW );
	bm->AddChild( view );
	
	bm->Lock();
	rgb_color bgcolor = { 0, 0, 0, 255 };
	rgb_color textcolor = { 255, 255, 255, 255 };
	
	//view->SetLowColor( bgcolor );
	view->SetHighColor( bgcolor );
	view->FillRect( bounds );
	view->SetFont( &fRenderFont );
	view->SetLowColor( bgcolor );
	view->SetHighColor( textcolor );
	view->DrawString( s->String(), BPoint( 0, fFontHeight.ascent-1 ) );
	view->Sync();
	bm->Unlock();
	return bm;
}
