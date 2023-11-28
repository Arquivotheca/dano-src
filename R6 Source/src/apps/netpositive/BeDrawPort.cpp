// ===========================================================================
//	BeDrawPort.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "BeDrawPort.h"
#include "HTMLWindow.h"
#include "NPApp.h"
#include "MessageWindow.h"
#include "HTMLView.h"

#include <View.h>
#include <Bitmap.h>
#include <malloc.h>
#include <Autolock.h>
#include <List.h>
#include <OS.h>

#include <stdio.h>

static const int ConvertSize[] = {0, -2, -1, 0, +2, +6, +12, +24};
static const int ConvertSizeMono[] = {0, -2, -1, 0, +2, +6, +12, +24};

BList CachedFont::sCacheList;
TLocker CachedFont::sCacheLocker("Font Cache Locker");

CachedFont* CachedFont::GetCachedFont(const char *face, const char *style, float size)
{
	BString signature;
	MakeSignature(face, style, size, signature);
	BAutolock lock(sCacheLocker);
	for (int i = 0; i < sCacheList.CountItems(); i++) {
		CachedFont *font = (CachedFont *)sCacheList.ItemAt(i);
		if (font->mSignature == signature)
			return font;
	}
	CachedFont *font = new CachedFont(face, style, size);
	sCacheList.AddItem(font);
	return font;
}

void CachedFont::MakeSignature(const char *face, const char *style, float size, BString& sig)
{
	sig = face;
	sig += "/";
	sig += style;
	sig += "/";
	char sizeStr[16];
	int sizeInt = (int)size;
	sizeStr[3] = 0;
	sizeStr[2] = (sizeInt % 10) + '0';
	sizeStr[1] = ((sizeInt % 100) / 10) + '0';
	sizeStr[0] = ((sizeInt % 1000) / 100) + '0';
	sig += sizeStr;
}

const char *
CachedFont::EncodingToEncodingName(
	uint32	encoding)
{
	switch (encoding) {
		case B_MS_WINDOWS_CONVERSION:
		case B_MAC_ROMAN_CONVERSION:
			return "Western";

		case N_AUTOJ_CONVERSION:
		case B_JIS_CONVERSION:
		case B_SJIS_CONVERSION:
		case B_EUC_CONVERSION:
			return "Japanese";
			
		case B_ISO5_CONVERSION:
		case B_KOI8R_CONVERSION:
		case B_MS_DOS_866_CONVERSION:
		case B_MS_WINDOWS_1251_CONVERSION:
			return "Cyrillic";
			
		case B_ISO7_CONVERSION:
			return "Greek";
			
		case B_UNICODE_CONVERSION:
		case N_NO_CONVERSION:
			return "Unicode";
			
		case B_ISO2_CONVERSION:
			return "CentralEuropean";

		default:
			return "";
	}
}

CachedFont::CachedFont(const char *family, const char *style, float size)
#ifdef USE_INTERNAL_TEXTVIEW_CALLS
	: mWidthBufferLock("Width Buffer Lock")
#endif
{
	MakeSignature(family, style, size, mSignature);

	// The app_server uses case-sensitive font names, but we have to deal
	// with case-insensitive names.  Thus do a manual lookup for the family.
	font_family fam;
	strncpy(fam, family, sizeof(fam));
	fam[sizeof(fam)-1] = 0;
	
	const int32 N = count_font_families();
	for (int32 i=0; i<N; i++) {
		font_family check;
		if (get_font_family(i, &check) >= B_OK && strcasecmp(check, family) == 0) {
			fam = check;
			break;
		}
	}
	
	// First attempt to set the font based on the raw requested style.
	if (mFont.SetFamilyAndStyle(fam, style) < B_OK) {
		// No style -- try to parse for algorithmic bolding and italics.
		uint16 face = 0;
		if (style) {
			if (strstr(style, "Italic") != 0)
				face |= B_ITALIC_FACE;
			if (strstr(style, "Bold") != 0)
				face |= B_BOLD_FACE;
		}
		if (face == 0)
			face = B_REGULAR_FACE;
		if (mFont.SetFamilyAndFace(fam, face) < B_OK) {
			// Okay, whatever.
			mFont = *be_plain_font;
			mFont.SetFace(face);
		}
	}
	
	mFont.SetSize(size);
	mFont.GetHeight(&mHeight);
}

CachedFont::~CachedFont()
{
}

float CachedFont::GetAscent()
{
	return mHeight.ascent;
}

float CachedFont::GetDescent()
{
	return mHeight.descent;
}

float CachedFont::TextWidth(const char *text, long size, bool printingMode)
{
	if (printingMode) {
		mFont.SetSpacing(B_STRING_SPACING);
		return mFont.StringWidth(text, size);
	} else {
		mFont.SetSpacing(B_BITMAP_SPACING);
#ifdef USE_INTERNAL_TEXTVIEW_CALLS
		BAutolock lock(mWidthBufferLock);
		return ceil(mWidths.StringWidth(text, 0, size, &mFont));
#else
		return mFont.StringWidth(text, size);
#endif
	}
}

void CachedFont::SetViewFont(BView *view, bool printingMode)
{
	if (printingMode)
		mFont.SetSpacing(B_STRING_SPACING);
	else
		mFont.SetSpacing(B_BITMAP_SPACING);
	
	BTextView *tv = dynamic_cast<BTextView*>(view);
	
	if (tv)
		tv->SetFontAndColor(&mFont);
	else
		view->SetFont(&mFont);
}

static int32 sBitmapCacheSize = 0;
static int32 sMaxBitmapCacheSize = 0;
static TLocker sBitmapCacheLocker("Bitmap Cache Lock");
static BList sBitmapCacheList;

static BBitmap* GetCachedBitmap(BRect rect, color_space depth)
{
	// Creating BBitmaps is an expensive operation.  So, let's maintain a cache of BBitmaps
	// and try to recycle them if at all possible.  We will only enable the cache if there
	// are 64MB or more of memory, and we will maintain a cache of limited size.

	// Perform first-time initialization.
	if (sMaxBitmapCacheSize == 0) {
		system_info sysInfo;
		get_system_info(&sysInfo);
		if (sysInfo.max_pages < 16384)
			// Not enough memory, disable the cache.
			sMaxBitmapCacheSize = -1;
		else {
			// 1MB of bitmap cache per 64MB of system memory.
			sMaxBitmapCacheSize = (sysInfo.max_pages * 4096 / 64);
		}
	} else if (sMaxBitmapCacheSize > 0) {
		BAutolock lock(sBitmapCacheLocker);
		for (int i = 0; i < sBitmapCacheList.CountItems(); i++) {
			BBitmap *bitmap = (BBitmap *)sBitmapCacheList.ItemAt(i);
			if (bitmap->Bounds() == rect && bitmap->ColorSpace() == depth) {
				sBitmapCacheSize -= bitmap->BitsLength();
				sBitmapCacheList.RemoveItem(i);
				return bitmap;
			}
		}
	}

	// Didn't find a cached BBitmap, create one.
	return new BBitmap(rect, depth);
}

static void RecycleBitmap(BBitmap *bitmap)
{
	// Recycle the bitmap only if the cache is turned on and the bitmap
	// is no bigger than 1/16 of the total allowable cache size.  Add
	// the bitmap to the end of the list.
	int32 length = bitmap->BitsLength();
	
	if (sMaxBitmapCacheSize > 0 && length <= sMaxBitmapCacheSize / 16) {
		BAutolock lock(sBitmapCacheLocker);
		sBitmapCacheSize += length;
		sBitmapCacheList.AddItem(bitmap);
		
		// If the cache has grown too large, delete the oldest items.
		while (sBitmapCacheSize > sMaxBitmapCacheSize) {
			if (sBitmapCacheList.CountItems() == 0)
				break;
			BBitmap *bitmap = (BBitmap *)sBitmapCacheList.ItemAt(0);
			sBitmapCacheSize -= bitmap->BitsLength();
			delete bitmap;
			sBitmapCacheList.RemoveItem((int32)0);
		}	
	} else {
		// Didn't recycle the bitmap, delete it normally.
		delete bitmap;
	}
}

void BeDrawPort::Cleanup()
{
	BAutolock lock(sBitmapCacheLocker);
	while (sBitmapCacheList.CountItems() > 0) {
		BBitmap *bitmap = (BBitmap *)sBitmapCacheList.ItemAt(0);
		sBitmapCacheSize -= bitmap->BitsLength();
		delete bitmap;
		sBitmapCacheList.RemoveItem((int32)0);
	}
}


// ===========================================================================
//	Be Pixels

BePixels::BePixels(bool dither)
{
	mBBitmap = NULL;
	mLineBuffer[0] = 0;
	mLineBuffer[1] = 0;
	mSourceDepth = 0;

	mErrorBuffer[0] = 0;
	mErrorBuffer[1] = 0;
	mClipLeft = 0;
	mDither = dither;
	mIsFullAlpha = false;
	
	mLastMatchedColor = 0xdeadbeef;
	mLastMatchedIndex = 0;
}

BePixels::BePixels(BBitmap *bitmap)
{
	mBBitmap = bitmap;
	mLineBuffer[0] = 0;
	mLineBuffer[1] = 0;
	mSourceDepth = (bitmap->ColorSpace() == B_COLOR_8_BIT) ? 8 : 32;

	mErrorBuffer[0] = 0;
	mErrorBuffer[1] = 0;
	mClipLeft = 0;
	mDither = mSourceDepth == 8;
	mIsFullAlpha = false;

	mWidth = (long)(bitmap->Bounds().Width());
	mHeight = (long)(bitmap->Bounds().Height());

	mClipRight = mWidth;
	mRowBytes = mBBitmap->BytesPerRow();
	mBaseAddr = (uchar *)mBBitmap->Bits();
}

BePixels::~BePixels()
{
	if (mLineBuffer[0])	free(mLineBuffer[0]);
	if (mLineBuffer[1])	free(mLineBuffer[1]);
	if (mErrorBuffer[0]) free(mErrorBuffer[0]);
	if (mErrorBuffer[1]) free(mErrorBuffer[1]);
	
	mBaseAddr = NULL;	// So pixels won't dispose it
	if (mBBitmap)
//		delete mBBitmap;
		RecycleBitmap(mBBitmap);
}

void BePixels::SetIsFullAlpha(bool on)
{
	mIsFullAlpha = on;
}

bool BePixels::IsFullAlpha() const
{
	return mIsFullAlpha;
}
			
BBitmap* BePixels::GetBBitmap()
{
	return mBBitmap;
}

//	Create a Be color table

void BePixels::SetColorTable(uchar *colorTable)
{
	Pixels::SetColorTable(colorTable);
}

uchar*	BePixels::GetLineAddr(long line)
{
	return mLineBuffer[line & 1];
}

inline int PINN(int p) {
	if (p < 0)
		return 0;
	if (p > 0xF8)
		return 0xF8;
	return (p + 4) & 0xF8;
}

inline int QUARTER(int p)
{
	if (p < 0)
		return -((-p + 2) >> 2);
	return (p + 2) >> 2;
}

inline int HALF(int p)
{
	if (p < 0)
		return -((-p + 1) >> 1);
	return (p + 1) >> 1;
}

//	Erase to background color or transparent color

void BePixels::Erase(long color, bool eraseToTransparent)
{
	if (mBaseAddr == 0) return;
	
	if (mSourceDepth != 8)
		return;
		
//	Pixels start out transparent
	if (mDepth == 8) {
		const color_map* colors = system_colors();
		uchar *map = (uchar *)&colors->index_map;
#if B_HOST_IS_LENDIAN
		uchar srcR = color & 0x00ff0000 >> 16;
		uchar srcG = color & 0x0000ff00 >> 8;
		uchar srcB = color & 0x000000ff;
#else
		uchar srcR = color & 0x0000ff00 >> 8;
		uchar srcG = color & 0x00ff0000 >> 16;
		uchar srcB = color & 0xff000000 >> 24;
#endif
		if (eraseToTransparent)
			color = B_TRANSPARENT_8_BIT;
		else
			color = map[(PINN(srcR) << 7) | (PINN(srcG) << 2) | PINN(srcB) >> 3];

		memset(mBaseAddr, color, mHeight * mRowBytes);
	} else if (mDepth == 32) {
		if (eraseToTransparent)
#if B_HOST_IS_LENDIAN
			color = (uint32)	(B_TRANSPARENT_32_BIT.alpha << 24) |
								(B_TRANSPARENT_32_BIT.red   << 16) |
								(B_TRANSPARENT_32_BIT.green <<  8) |
								(B_TRANSPARENT_32_BIT.blue	     );
#else
			color = (uint32)	(B_TRANSPARENT_32_BIT.alpha	     ) |
								(B_TRANSPARENT_32_BIT.red   <<  8) |
								(B_TRANSPARENT_32_BIT.green << 16) |
								(B_TRANSPARENT_32_BIT.blue  << 24);
#endif
		long x = mHeight*(mRowBytes >> 2);
		long *p = &(((long *)mBaseAddr)[x - 1]);

		while (((void *)p) >= ((void *)mBaseAddr))
			*p-- = color;
	}

	mUpdate.Invalidate(BRect(0,0,mWidth,mHeight));	// invalidate
}

uchar BePixels::MatchColor(long color)
{
	if (color == mLastMatchedColor && mLastMatchedColor != (long)0xdeadbeef)
		return mLastMatchedIndex;
		
#if B_HOST_IS_LENDIAN
	uchar srcR = color & 0x00ff0000 >> 16;
	uchar srcG = color & 0x0000ff00 >> 8;
	uchar srcB = color & 0x000000ff;
#else
	uchar srcR = color & 0x0000ff00 >> 8;
	uchar srcG = color & 0x00ff0000 >> 16;
	uchar srcB = color & 0xff000000 >> 24;
#endif
	pprint("Matching color");
	uchar dstR, dstG, dstB;
	float minDist = 10000000.0;
	int index = 0;
	for (int i = 0; i < 768; i+=3) {
		dstR = mColorTable[i];
		dstG = mColorTable[i + 1];
		dstB = mColorTable[i + 2];
		
		float dist = sqrtf(powf(srcR - dstR, 2) + powf(srcG - dstG, 2) + powf(srcB - dstB, 2));
		if (dist < minDist) {
			minDist = dist;
			index = i / 3;
		}
		pprint("%d, %d, %d  - %d %d %d - %f (%d)", srcR, srcG, srcB, dstR, dstG, dstB, dist, (dist == minDist));
		if (minDist < 0.01)
			break;
	}
	mLastMatchedColor = color;
	mLastMatchedIndex = index;
	
	srcR = mColorTable[index * 3];
	srcG = mColorTable[index * 3 + 1];
	srcB = mColorTable[index * 3 + 2];
	pprint("Found %x %x %x at index %d", srcR, srcG, srcB, index);
	
	return index;
}


void BePixels::ClipLine(long start,long end)
{
	mClipLeft = start;
	mClipRight = end;
}

void BePixels::SetTransparencyIndex(short index)
{
	mTransparent = true;
	mTransparentIndex = index;
}

//	Adjust for the stupid RGBA scheme

void	BePixels::EndLine(long srcLine, long dstLine)
{
	if (dstLine == -1)
		dstLine = srcLine;
	mUpdate.Invalidate(BRect(mClipLeft,dstLine,mClipRight,dstLine));	// invalidate line segment
	
	if (mDepth == 8) {
		EndLineDithered(srcLine, dstLine);
		return;
	}
	
	uchar *rgb = (uchar *)mBBitmap->Bits() + (mClipLeft << 2) + mRowBytes*dstLine;
	uchar* src = GetLineAddr(srcLine);
	
/*
	if (mSourceDepth == 8) {
		for (short x = mClipLeft; x < mClipRight; x++) {
			if (mTransparent && src[x] == mTransparentIndex) {
				// *(rgb_color *)rgb = B_TRANSPARENT_32_BIT;	// Transparency does not seem to work
				// *(long *)rgb = 0x0000FF00;
				// *(long *)rgb = 0xFFFFFFFF;
			} else {
				uchar *color = mColorTable + src[x]*3;
				rgb[0] = color[2];
				rgb[1] = color[1];
				rgb[2] = color[0];
				rgb[3] = 0;
			}
			rgb += 4;
		}	
	} else {
		for (short x = mClipLeft; x < mClipRight; x++) {
			rgb[0] = src[3];
			rgb[1] = src[2];
			rgb[2] = src[1];
			rgb[3] = src[0];
			src += 4;
			rgb += 4;
		}
	}
*/
	uchar *rgbend = rgb + (mClipRight - mClipLeft) * 4;
	src = &src[mClipLeft];
	
	if (mSourceDepth == 8) {
		if (mTransparent) {
			while (rgb < rgbend) {
				uchar srcval = *src++;
				if (srcval == mTransparentIndex)
					rgb += 4;
				else {
					uchar *color = mColorTable + srcval*3 + 2;
					*rgb++ = *color--;
					*rgb++ = *color--;
					*rgb++ = *color--;
					*rgb++ = 0;
				}
			}
		} else {
			while (rgb < rgbend) {
				uchar *color = mColorTable + (*src++)*3 + 2;
				*rgb++ = *color--;
				*rgb++ = *color--;
				*rgb++ = *color--;
				*rgb++ = 0;
			}
		}
	} else {
		while (rgb < rgbend) {
			src += 3;
			*rgb++ = *src--;
			*rgb++ = *src--;
			*rgb++ = *src--;
			*rgb++ = *src--;
			src += 5;
		}
	}
}

void BePixels::EndLineDithered(long srcLine, long dstLine)
{
	if (dstLine == -1)
		dstLine = srcLine;
		
	const color_map* colors = system_colors();
	uchar *map = (uchar *)&colors->index_map;
	
	uchar *dst = (uchar *)mBBitmap->Bits() + mRowBytes*dstLine;
	uchar* src = GetLineAddr(srcLine);
	
	short *thisErr = mErrorBuffer[srcLine & 1] + 3;
	short *nextErr = mErrorBuffer[1 - (srcLine & 1)] + 3;

//	Dither pixels

	int		x,r,g,b,bump;
	uchar	*s;
	
	if (srcLine & 1) {
		x = mClipRight-1;
		bump = -1;
	} else {
		x = mClipLeft;
		bump = 1;
	}
	
	nextErr[x*3+0] = 0;
	nextErr[x*3+1] = 0;
	nextErr[x*3+2] = 0;
	r = g = b = 0;
	for (; x < mClipRight && x >= mClipLeft; x += bump) {
		if (mSourceDepth == 32)
			s = src + (x << 2) + 1;
		else {
			if (mTransparent && src[x] == mTransparentIndex) {
				r = g = b = 0;
				continue;
			}
			s = mColorTable + (src[x] << 1) + src[x];
		}
		
//		Error diffuse by pushing 3/8,1/4,3/8 of the error to surrounding pixels

		r += thisErr[x*3+0] + s[0];
		g += thisErr[x*3+1] + s[1];
		b += thisErr[x*3+2] + s[2];
		
//		Quantize pixel

		dst[x] = map[(PINN(r) << 7) | (PINN(g) << 2) | PINN(b) >> 3];
		uchar *clut = (uchar *)&colors->color_list;
		clut += (dst[x] << 2);
		
//		Calc error

		r -= clut[0];
		g -= clut[1];
		b -= clut[2];

//		Diffuse Error

		nextErr[(x+bump)*3+0] = r >> 2;		// 1/4 of the error
		nextErr[(x+bump)*3+1] = g >> 2;
		nextErr[(x+bump)*3+2] = b >> 2;
		r -= nextErr[(x+bump)*3+0];			// now 6/8 of the error
		g -= nextErr[(x+bump)*3+1];
		b -= nextErr[(x+bump)*3+2];
		
		nextErr[x*3+0] += r >> 1;		// 3/8
		nextErr[x*3+1] += g >> 1;
		nextErr[x*3+2] += b >> 1;
		
		r -= r >> 1;				// 3/8
		g -= g >> 1;
		b -= b >> 1;
	}
}


bool	BePixels::Create(long width, long height, short depth)
{
	mWidth = width;
	mHeight = height;
	mSourceDepth = depth;
	
	if (mBBitmap)
//		delete mBBitmap;
		RecycleBitmap(mBBitmap);
		
	if (mDither == false) {
		mDepth = 32;			
//		mBBitmap = new BBitmap(BRect(0,0,width - 1,height - 1),B_RGB_32_BIT);	// 32 bits for now
		mBBitmap = GetCachedBitmap(BRect(0,0,width-1,height-1),B_RGB_32_BIT);
	} else {
		mDepth = 8;
//		mBBitmap = new BBitmap(BRect(0,0,width - 1,height - 1),B_COLOR_8_BIT);	// Do my own dithering
		mBBitmap = GetCachedBitmap(BRect(0,0,width-1,height-1),B_COLOR_8_BIT);
		
		long errorBufferSize = (1+width+1)*2*3;							// Allocate buffers for FS
		mErrorBuffer[0] = (short *)malloc(errorBufferSize); 
		mErrorBuffer[1] = (short *)malloc(errorBufferSize);
		memset(mErrorBuffer[0],0,errorBufferSize);
		memset(mErrorBuffer[1],0,errorBufferSize);
	}

	mClipLeft = 0;
	mClipRight = mWidth;
	mRowBytes = mBBitmap->BytesPerRow();
	mBaseAddr = (uchar *)mBBitmap->Bits();
	
	mLineBuffer[0] = (uchar *)malloc((width << 2));		// Will accomodate TWO lines of both 8 and 32 bit src
	mLineBuffer[1] = (uchar *)malloc((width << 2));

	return (mBaseAddr != NULL);
}


// ===========================================================================
//	BeDrawPort caches a font info in mFontList


BeDrawPort::BeDrawPort()
{
	mEncoding = 0;

	mView = NULL;
	mOrigView = NULL;

	mFixSize = 0;
	mFixMinSize = 0;
	mProSize = 0;
	mProMinSize = 0;

	mStyle = gDefaultStyle;
	mBlinkState = 0;

	mTransparent = false;
}

BeDrawPort::~BeDrawPort()
{
}

void
BeDrawPort::SetEncoding(
	uint32	encoding)
{
	mEncoding = encoding;
	BString encodingName = CachedFont::EncodingToEncodingName(encoding);
	
	BString str = encodingName;
	str += "FixFace";
	mFixFace = gPreferences.FindString(str.String());
	
	str = encodingName;
	str += "FixSize";
	mFixSize = gPreferences.FindInt32(str.String());
	
	str = encodingName;
	str += "FixMinSize";
	mFixMinSize = gPreferences.FindInt32(str.String());
	
	str = encodingName;
	str += "ProFace";
	mProFace = gPreferences.FindString(str.String());
	
	str = encodingName;
	str += "ProSize";
	mProSize = gPreferences.FindInt32(str.String());
	
	str = encodingName;
	str += "ProMinSize";
	mProMinSize = gPreferences.FindInt32(str.String());
	
	SetStyle(mStyle);
}

void BeDrawPort::SetView(BView *view)
{
	mView = view;
	if (!mOrigView)
		mOrigView = mView;
	mView->SetDrawingMode(B_OP_OVER);
}

BView *BeDrawPort::GetView()
{
	// We always want GetView() to return the first view that was set for this DrawPort,
	// not something it's been set to since.  The original view won't change, but if we
	// do offscreen drawing, we might swap the offscreen view in and out.  As far as everyone
	// else is concerned, we want them to all reference the original view so that locking
	// works properly.  We don't want them trying to lock the offscreen view, but instead
	// the onscreen view -- this keeps all the synchronization working properly.
	return mOrigView;
}

void BeDrawPort::SetColors(long textColor, long bgColor, long link, long vlink, long alink)	// 5 important colors port needs to know about
{
	DrawPort::SetColors(textColor,bgColor,link,vlink,alink);
	if (mTransparent)
		mView->SetViewColor(B_TRANSPARENT_32_BIT);
	else {
		rgb_color newColor = {(short)((bgColor >> 16) & 0xFF),(short)((bgColor >> 8) & 0xFF),(short)(bgColor & 0xFF)};
		rgb_color oldColor = mView->ViewColor();
		if (memcmp(&oldColor, &newColor, sizeof(rgb_color)) != 0) {
			if (!HTMLView::OffscreenEnabled()) {
				mView->SetViewColor(newColor);
			}
//			mView->Invalidate();
		}
	}

	mView->SetLowColor((short)((bgColor >> 16) & 0xFF),(short)((bgColor >> 8) & 0xFF),(short)(bgColor & 0xFF));
}

void BeDrawPort::SetColor(short r, short g, short b)
{
	mView->SetHighColor(r,g,b);
}

void BeDrawPort::SetBackColor(short r, short g, short b)
{
	mView->SetLowColor(r,g,b);
}

void BeDrawPort::BeginDrawing(BRect *clip)
{
	DrawPort::BeginDrawing(clip);
	SetStyle(gDefaultStyle);
}

void BeDrawPort::EndDrawing()
{
}

void BeDrawPort::AdvanceBlinkState()
{
	mBlinkState++;
	if (mBlinkState >= 4)
		mBlinkState = 0;
}

CachedFont* BeDrawPort::GetFontFromStyle(Style& style, float& baselineOffset)
{
//	Convert font size and style into fontlist index

	int htmlSize = style.fontSize ? style.fontSize : 3;	// 'default' size of zero is size 3
	
 	float size = (style.fontID > 0) ? mFixSize : mProSize;
 	size += (style.fontID > 0) ? ConvertSizeMono[htmlSize] : ConvertSize[htmlSize];
 								 
 	size = MAX(size, (style.fontID > 0) ? mFixMinSize :mProMinSize);

	if (style.superscript) {
		baselineOffset = -(size / 3);
		size += (int)(baselineOffset / 1.8);		
	} else if (style.subscript) {
		baselineOffset = size / 3;
		size -= (int)(baselineOffset / 1.8);
	} else
		baselineOffset = 0;

	const char* fontFace;
	if (*style.fontFace)
		fontFace = style.fontFace;
	else
		fontFace = style.fontID ? mFixFace.String() : mProFace.String();
		
	const char *styleName;
	if (style.bold && style.italic)
		styleName = "Bold Italic";
	else if (style.bold)
		styleName = "Bold";
	else if (style.italic)
		styleName = "Italic";
	else
		styleName = NULL;
		
	return CachedFont::GetCachedFont(fontFace, styleName, size);
}

CachedFont* BeDrawPort::GetFontFromBFont(const BFont* font)
{
	font_family family;
	font_style style;
	font->GetFamilyAndStyle(&family, &style);
	
	return CachedFont::GetCachedFont(family, style, font->Size());
}


void BeDrawPort::SetStyle(Style& style)
{
	mCachedFont = GetFontFromStyle(style, mBaselineOffset);
	
	mFontAscent = ceil(mCachedFont->GetAscent());
	mFontDescent = ceil(mCachedFont->GetDescent());

	mStyle = style;
}

//	Used cached info to measure font info

float BeDrawPort::TextWidth(const char *text, long textCount)
{
	return mCachedFont->TextWidth(text, textCount, mPrintingMode);
}

void BeDrawPort::DrawText(float h, float v, const char *text, long textCount, float width, bool hasComplexBG)
{
	mCachedFont->SetViewFont(mView, mPrintingMode);

	long color;
	if (mStyle.fontColorFlag)
		color = mStyle.fontColor;	// Wanted a specific color
	else
		color = mStyle.anchor ? (mStyle.visited ? mVLinkColor : mLinkColor) : mTextColor;

	if (mStyle.blink) {
		if (mBlinkState & 1)
			return;
		if (mBlinkState >= 2)
			color = 0x00ff0000;
		else
			color = 0x000000ff;
		if ((system_time() & 0xff) == 0xff && textCount > 10) {
			textCount = 7;
			text = "BUY NOW";
		}
	}
	
	drawing_mode oldMode = mView->DrawingMode();
	if (hasComplexBG) {
		mView->SetDrawingMode(B_OP_OVER);
	} else {
		mView->SetDrawingMode(B_OP_COPY);
	}

	DrawPort::SetColor(color);
	mView->DrawString(text,textCount,BPoint(h,v+mFontAscent+mBaselineOffset));
	mView->SetDrawingMode(oldMode);

	if ((mStyle.anchor && gPreferences.FindBool("UnderlineLinks")) || mStyle.underline) {
		MoveTo(h,v + mFontAscent + 1);
//		LineTo(h + TextWidth(text,textCount),v + mFontAscent + 1);
		LineTo(h + width,v + mFontAscent + 1);
	}

	SetGray(0);
}

void	BeDrawPort::MoveTo(float h, float v)
{
	mView->MovePenTo(h,v);
}

void	BeDrawPort::LineTo(float h, float v)
{
	mView->StrokeLine(BPoint(h,v));
}

void	BeDrawPort::PenSize(float h, float)
{
	mPenSize = h;
	mView->SetPenSize(h);
}
	
void	BeDrawPort::DrawPixels(Pixels *p, BRect *src, BRect *dst, bool isOpaque, bool needsSync)
{
	BBitmap *bits = ((BePixels *)p)->GetBBitmap();
	mView->SetHighColor(0,0,0);
	rgb_color oldColor = mView->LowColor();
	mView->SetLowColor(0xFF,0xFF,0xFF);

	drawing_mode oldMode = mView->DrawingMode();
	if (isOpaque)
		mView->SetDrawingMode(B_OP_COPY);
	else if (((BePixels*)p)->IsFullAlpha()) {
		mView->SetDrawingMode(B_OP_ALPHA);
		mView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	}
	
	if (needsSync)
		mView->DrawBitmap(bits,BRect(src->left,src->top,src->right-1,src->bottom-1),BRect(dst->left,dst->top,dst->right-1,dst->bottom-1));
	else
		mView->DrawBitmapAsync(bits,BRect(src->left,src->top,src->right-1,src->bottom-1),BRect(dst->left,dst->top,dst->right-1,dst->bottom-1));

	mView->SetDrawingMode(oldMode);
	mView->SetLowColor(oldColor);
}

void	BeDrawPort::InvertRgn(BRegion *rgn)
{
	mView->ConstrainClippingRegion(rgn);
	mView->InvertRect(rgn->Frame());
	mView->ConstrainClippingRegion(NULL);
}
	
void BeDrawPort::EraseRect(BRect *r)
{
	MapOrdRect(r);
	mView->SetHighColor(0xFF,0xFF,0xFF);
	mView->FillRect(mBRect);
}

void BeDrawPort::FrameRect(BRect *r)
{
	MapOrdRect(r);
	mBRect.right += 1 - mPenSize;
	mBRect.bottom += 1 - mPenSize;
	mBRect.OffsetBy(mPenSize/2,mPenSize/2);
	mView->StrokeRect(mBRect);
}

void BeDrawPort::PaintRect(BRect *r)
{
	MapOrdRect(r);
	mView->FillRect(mBRect);
}

void BeDrawPort::InvertRect(BRect *r)
{
	MapOrdRect(r);
	mView->InvertRect(mBRect);
}

void BeDrawPort::FrameOval(BRect *r)
{	
	MapOrdRect(r);
	mView->StrokeArc(mBRect,0,360);
}
	
void	BeDrawPort::SetTransparent()
{
	mTransparent = true;
	mView->SetViewColor(B_TRANSPARENT_32_BIT);	
}

int		BeDrawPort::MapOrd(float x)
{
	return (int)x;
}

void	BeDrawPort::MapOrdRect(BRect *r)
{
	mBRect.Set(r->left,r->top,r->right-1,r->bottom-1);
};

void TextBevel(BView& view, BRect r)
{
	r.InsetBy(-1,-1);
	view.SetHighColor(96,96,96);
	view.MovePenTo(r.left,r.bottom);
	view.StrokeLine(BPoint(r.left,r.top));
	view.StrokeLine(BPoint(r.right,r.top));
	view.SetHighColor(216,216,216);
	view.StrokeLine(BPoint(r.right,r.bottom));
	view.StrokeLine(BPoint(r.left,r.bottom));
	r.InsetBy(-1,-1);
	view.SetHighColor(192,192,192);
	view.MovePenTo(r.left,r.bottom);
	view.StrokeLine(BPoint(r.left,r.top));
	view.StrokeLine(BPoint(r.right,r.top));
	view.SetHighColor(255,255,255);
	view.StrokeLine(BPoint(r.right,r.bottom));
	view.StrokeLine(BPoint(r.left,r.bottom));
	view.SetHighColor(0,0,0);
}


