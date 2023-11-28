// ===========================================================================
//	BeDrawPort.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __BEDRAWPORT__
#define __BEDRAWPORT__

#include "DrawPort.h"
#include "Utils.h"
#include <Region.h>
#include <Font.h>
#include <Locker.h>
#include <Messenger.h>
#include <String.h>

#define USE_INTERNAL_TEXTVIEW_CALLS 1

#if USE_INTERNAL_TEXTVIEW_CALLS
#include "../../../src/kit/interface/TextViewSupport.h"
#endif




// ===========================================================================

class BePixels : public Pixels {
public:				
						BePixels(bool dither = true);
						BePixels(BBitmap *bitmap);
	virtual				~BePixels();
			
	virtual	void		SetTransparencyIndex(short index);
	virtual	void		Erase(long color, bool eraseToTransparent = true);
	
	virtual	unsigned char*		GetLineAddr(long line);
	virtual	void		ClipLine(long start,long end);
	virtual	void		EndLine(long srcLine, long dstLine = -1);
			void		EndLineDithered(long srcLine, long dstLine = -1);
			
	virtual	bool		Create(long width, long height, short depth);
	virtual	void		SetColorTable(unsigned char *colorTable);
	
			void		SetIsFullAlpha(bool on);
			bool		IsFullAlpha() const;
			
			unsigned char		MatchColor(long color);
	
			BBitmap*	GetBBitmap();

protected:
		unsigned char*		mLineBuffer[2];	// Up to two lines at once
		short*		mErrorBuffer[2];
			
		short		mClipLeft;
		short		mClipRight;
		short		mSourceDepth;
		BBitmap*	mBBitmap;
		bool		mDither;
		bool		mIsFullAlpha;
		
		long		mLastMatchedColor;
		unsigned char		mLastMatchedIndex;
};

// ===========================================================================

class CachedFont {
public:

static	CachedFont*	GetCachedFont(const char *face, const char *style, float size);
static 	const char*	EncodingToEncodingName(uint32 encoding);
		float		GetAscent();
		float		GetDescent();
		float		TextWidth(const char *text, long size, bool printingMode);
		void		SetViewFont(BView *view, bool printingMode);
	
private:
static	void		MakeSignature(const char *face, const char *style, float size, BString& sig);
					CachedFont(const char *face, const char *style, float size);
					~CachedFont();
	
		BFont			mFont;
		BString			mSignature;
#ifdef USE_INTERNAL_TEXTVIEW_CALLS
		_BWidthBuffer_	mWidths;
		TLocker			mWidthBufferLock;
#endif
		font_height		mHeight;
	
static	BList			sCacheList;
static	TLocker			sCacheLocker;
};


class BeDrawPort : public DrawPort {
public:
					BeDrawPort();
	virtual			~BeDrawPort();
	
	void			SetEncoding(uint32 encoding);

			void	SetView(BView *view);
			BView*	GetView();
			
	virtual	void	BeginDrawing(BRect *clip);
	virtual	void	EndDrawing();

	virtual	void	SetColor(short r, short g, short b);
	virtual	void	SetBackColor(short r, short g, short b);
	virtual	void	SetColors(long textColor, long bgColor, long link, long vlink, long alink);
	
	virtual	void	SetStyle(Style& style);
	virtual	float	TextWidth(const char *text, long textCount);
	virtual	void	DrawText(float h, float v, const char *text, long textCount, float width, bool hasComplexBG = true);
	
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
	
	CachedFont*		GetFontFromStyle(Style& style, float& baselineOffset);
static	CachedFont*		GetFontFromBFont(const BFont *font);
	void			AdvanceBlinkState();
static void			Cleanup();

protected:
	virtual	int		MapOrd(float x);			// Map into big space
	virtual	void	MapOrdRect(BRect *r);	// Map into big space
	
	uint32			mEncoding;
	float			mFixSize;
	float			mFixMinSize;
	float			mProSize;
	float			mProMinSize;
	BString			mFixFace;
	BString			mProFace;
	CachedFont*		mCachedFont;

	int				mBlinkState;
	BView			*mView;
	BView			*mOrigView;
	BRect			mBRect;
	float			mBaselineOffset;
	bool			mTransparent;
};

#endif

