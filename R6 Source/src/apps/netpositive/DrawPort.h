// ===========================================================================
//	DrawPort.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __DRAWPORT__
#define __DRAWPORT__

#include <Rect.h>

class BRegion;

// ===========================================================================
// Style structure

typedef struct {
	unsigned fontSize : 4;		// 1 .. 7 (0 == default font)
	unsigned fontID : 1;		// 0 = proportional, 1 = mono
	unsigned bold : 1;
	unsigned italic : 1;
	unsigned underline : 1;
	unsigned strikethrough : 1;
	unsigned subscript : 1;
	unsigned superscript : 1;
	unsigned blink : 1;
	
	unsigned pre : 1;			// Text is preformatted
	unsigned anchor : 1;		// Text is part of an anchor 
	unsigned visited : 1;		// That may have been visited

	unsigned fontColorFlag : 1;	// Specific font color was requested
	unsigned fontColor : 24;	// Specific Color to draw text
	char	 fontFace[64];
} Style;


// ===========================================================================
//	Simple object for pixmaps

class UpdateRegion {
public:		
				UpdateRegion();
				
		void	Invalidate(const BRect &r);
		void	Invalidate(UpdateRegion &updateRegion);
		
		void	OffsetBy(int x, int y);
		void	Reset();
		
		bool	NonZero();
		bool	GetRect(BRect &r);
		
protected:
		BRect mRect;
};

// ===========================================================================
//	Object to store and dither pixels

class Pixels {
public:
				Pixels();
		virtual	~Pixels();

		bool	GetUpdate(UpdateRegion& updateRegion);
		
virtual bool	Create(long width, long height, short depth);
virtual	bool	IsComplete();

		long	GetRef();
		void	SetRef(long ref);

		void	FlipVertical(bool flip);

virtual void 	Erase(long color, bool eraseToTransparent = true);
virtual void	SetTransparencyIndex(short index);
		void	SetBackground(long background);
virtual void	SetColorTable(uchar *colorTable);
virtual uchar	*GetColorTable();

virtual void	ClipLine(long start,long end);
virtual uchar*	GetLineAddr(long line);
virtual void	EndLine(long srcLine, long dstLine = -1);
virtual	void	Finalize();

		uchar	*GetBaseAddr();
		long	GetRowBytes();
		long	GetWidth();
		long	GetHeight();
		short	GetDepth();

		bool	GetMask(uchar **mask, long *rowBytes);
		bool	GetTransparentColor(long *color);
		
		void	FillWithPattern(Pixels *pattern, long hOffset, long vOffset);
		
protected:
		bool	mTransparent;
		short	mTransparentIndex;
		long	mTransparentCount;

		uchar	*mBaseAddr;
		long	mRowBytes;
		long	mWidth;
		long	mHeight;
		short	mDepth;

		bool	mFlip;			// Flip vertical
		uchar	*mColorTable;
		long	mBackground;
		long	mRef;
		bool mComplete;
		uchar	*mMask;
		long	mMaskRowBytes;
		
		UpdateRegion mUpdate;
};

// ===========================================================================
//	Drawing port

class InputGlyph;

class DrawPort {
public:
					DrawPort();
	virtual			~DrawPort();

//	Device independant methods

	virtual	void	SetColors(long textColor, long bgColor, long link, long vlink, long alink);
			void	SetBorderColor(bool selected);
			void	SetOffscreen(bool offscreen) {mDrawingOffscreen = offscreen;}
			bool	IsOffscreen() {return mDrawingOffscreen;}
	
			void	SetGray(short gray);
			void	SetColor(long rgb);
			void	SetBackColor(long rgb);

	virtual	void	DrawRule(BRect *r, bool noShade);
			void	DrawBevel(BRect *r);
			void	DrawAntiBevel(BRect *r);
			void	DrawRoundBevel(BRect *r);
			void	DrawTriBevel(BRect *r);
	
//	Device dependant methods

	virtual	void	BeginDrawing(BRect *clip);
	virtual	void	EndDrawing();
			BRect&	GetClip();
		
	virtual	void	SetColor(short r, short g, short b);
	virtual	void	SetBackColor(short r, short g, short b);
	virtual long	GetBackColor();
	
	virtual	void	SetStyle(Style& style) = 0;
	virtual	float	TextWidth(const char *text, long textCount) = 0;
	virtual	void	DrawText(float h, float v, const char *text, long textCount, float width, bool hasComplexBG = true) = 0;

	virtual	float	GetFontAscent();
	virtual	float	GetFontDescent();
	virtual	float	BlankLineHeight();
	
	virtual	void	MoveTo(float h, float v);
	virtual	void	LineTo(float h, float v);
	virtual	void	PenSize(float h, float v);
	
	virtual	void	DrawPixels(Pixels *p, BRect *src, BRect *dst, bool isOpaque = false, bool needsSync = false);
	virtual	void	InvertRgn(BRegion *rgn);
	
	virtual	void	EraseRect(BRect *r);
	virtual	void	FrameRect(BRect *r);
	virtual	void	PaintRect(BRect *r);
	virtual	void	InvertRect(BRect *r);
	
	virtual	void	FrameOval(BRect *r);
	
	virtual void	SetTransparent();
		
	void			SetPrintingMode(bool on);
	long			GetTextColor() {return mTextColor;}
	long			GetBGColor() {return mBGColor;}
	
protected:
	virtual	int		MapOrd(float x);			// Map into big space
	virtual	void	MapOrdRect(BRect *r);	// Map into big space

			BRect	mClip;
			float	mFontAscent;
			float	mFontDescent;
	
			long	mTextColor;
			long	mBGColor;
			long	mLinkColor;			// Link color
			long	mVLinkColor;		// Visited link color
			long	mALinkColor;
	
			long	mDefaultTextColor;	// Color to draw the next text with
	
			long	mColor;				// Current foreground color
			long	mBackColor;			// Current background color
			float	mPenSize;			// Size of pen
			bool	mDrawingOffscreen;
			bool	mPrintingMode;
			Style mStyle;			// Current style
};

extern Style gDefaultStyle;

#endif
