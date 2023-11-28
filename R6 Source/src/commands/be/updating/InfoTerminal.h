#ifndef INFO_TERMINAL
#define INFO_TERMINAL

#include "RenderView.h"
#include <String.h>
#include <Locker.h>
#include <Font.h>

class BView;

// Options; all set by default
enum
{
	IT_SCALE_HEIGHT = 1, // Scale text
	IT_SCALE_COLOR = 2, // Overexposed -> text color
	IT_FROM_CENTER = 4, // Origin of text scaling; From top otherwise
	IT_BLUR = 8, // Go from blurred to focused
	IT_LINEAR = 16, // Approach rate mode; Exponential otherwise
	IT_ADD_PIXELS = 32, // Transfer mode; Copy otherwise
	IT_SCALE_DOWN = 64, // Scale Color Down instead of up
};

class InfoTerminal : public BRenderView
{
	public:
		InfoTerminal( BRect frame, const char *name, uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT, uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS );
		~InfoTerminal( void );
		
		void SetColors( rgb_color bg_color, rgb_color text, float raster ); // raster is a % of bg_color
		void SetText( const char *text );
		void SetText( const char *text, int32 length );
		void SetFont( const BFont &font );
		void SetAlignment( alignment flag );
		void SetOptions( uint32 options );
		uint32 Options( void ) { return fOptions; };
		void SetApproachRate( float rate ); // 0 < rate < 1.0; larger numbers = faster
		void Display( void );
		
		inline BView *OffscreenView( void ) { return fBMview; };
	
	protected:
		virtual void BitmapInit( BBitmap **bmap, BRect bounds, color_space depth );
		virtual bool ShouldSuspend( void );
		
		virtual void ThreadInit( void );
		//virtual status_t ThreadExit( void );
		
		virtual BRect Render( void );
		
		void RenderBG( BRect bounds );
		void RenderLine( BPoint offset, float scale, BBitmap *src );
		BBitmap *MakeLineBM( const BString *s );
		
		void Init( void );
	
	protected:
		BView 		*fBMview;
		BBitmap		*fLineBM;
		rgb_color	fHighBG, fLowBG, fTextColor;
		BFont		fRenderFont;
		BString		fText;
		BLocker		fTextLock;
		font_height	fFontHeight;
		uint32		fOptions;
		float		fApproachRate;
		float		fLineScale;
		float		fLineWidth;
		float		fBlurWidth;
		int32		fCycle;
		float		fLineNumber;
		int32		fCharOffset;
		alignment	fAlignment;
		bool		fComplete;
		bool		fInit;
};

#endif