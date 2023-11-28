// ===========================================================================
//	DrawPort.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "DrawPort.h"
#include "Utils.h"
#include "MessageWindow.h"

#include <malloc.h>
#include <string.h>
#include <sys/param.h>
#include <InterfaceDefs.h>

// ===========================================================================
//	Simple object for maintaining update regions

UpdateRegion::UpdateRegion()
{
	Reset();
}

//	Invalidate a rectangle

void UpdateRegion::Invalidate(const BRect &r)
{
	mRect.left = MIN(r.left,mRect.left);
	mRect.right = MAX(r.right,mRect.right);
	mRect.top = MIN(r.top,mRect.top);
	mRect.bottom = MAX(r.bottom,mRect.bottom);
}

//	Invalidate a region

void UpdateRegion::Invalidate(UpdateRegion &updateRegion)
{
	Invalidate(updateRegion.mRect);
}

void UpdateRegion::OffsetBy(int x, int y)
{
	mRect.OffsetBy(x,y);
}

//	Reset the update regionto zero

void UpdateRegion::Reset()
{
	mRect.Set(0,0,0,0);
}

bool UpdateRegion::NonZero()
{
	return mRect.Width() != 0;
}

bool UpdateRegion::GetRect(BRect &r)
{
	r = mRect;
	return NonZero();
}

// ===========================================================================

Pixels::Pixels()
{
	mBaseAddr = 0;
	mRowBytes = 0;
	mWidth = mHeight = 0;
	mDepth = 0;
	mColorTable = 0;
	mTransparent = false;
	mTransparentCount = 0;
	mBackground = 0;
	mFlip = false;
	mRef = 0;
	mComplete = false;
	mMask = 0;
	mMaskRowBytes = 0;
}

Pixels::~Pixels()
{
	if (mBaseAddr)
		free(mBaseAddr);
	if (mMask)
		free(mMask);
	if (mColorTable)
		free(mColorTable);
}

//	Retung the update region up the food chain

bool Pixels::GetUpdate(UpdateRegion& updateRegion)
{
	updateRegion = mUpdate;
	mUpdate.Reset();
	return updateRegion.NonZero();
}

bool	Pixels::Create(long width, long height, short depth)
{
	mWidth = width;
	mHeight = height;
	mDepth = depth;
	mRowBytes = (depth == 8) ? (mWidth + 3) & 0xFFFFFFFC : mWidth << 2;
	mBaseAddr = (uchar *)malloc(mRowBytes*mHeight);
	return (mBaseAddr != NULL);
}

bool Pixels::IsComplete()
{
	return mComplete;
}

//	image is complete

void Pixels::Finalize()
{
	mComplete = true;
}

//	Return a cached version of pixels if one exists

long Pixels::GetRef()
{
	return mRef;
}

void Pixels::SetRef(long ref)
{
	mRef = ref;
}

//	Erase to a single color, or transparent color

void Pixels::Erase(long color, bool eraseToTransparent)
{
	if (mBaseAddr == 0) return;
	if (mDepth == 8)
		memset(mBaseAddr,(eraseToTransparent && mTransparent) ? mTransparentIndex : color,mHeight*mRowBytes);
	mUpdate.Invalidate(BRect(0,0,mWidth,mHeight));
}

void Pixels::ClipLine(long,long)
{
}

//	Get Address of a particular line

uchar *Pixels::GetLineAddr(long line)
{
	if (mFlip)
		return mBaseAddr + mRowBytes*(mHeight - 1 - line);
	else
		return mBaseAddr + mRowBytes*line;
}

void Pixels::FlipVertical(bool flip)
{
	mFlip = flip;
}

uchar *Pixels::GetBaseAddr()
{
	return mBaseAddr;
}

long Pixels::GetRowBytes()
{
	return mRowBytes;
}

long Pixels::GetHeight()
{
	return mHeight;
}

long Pixels::GetWidth()
{
	return mWidth;
}

short Pixels::GetDepth()
{
	return mDepth;
}

void Pixels::SetColorTable(uchar *colorTable)
{
	if (mColorTable)
		free(mColorTable);
	mColorTable = (uchar *)malloc(256*3);
//	NP_ASSERT(mColorTable);
	memcpy(mColorTable,colorTable,256*3);
}

uchar* Pixels::GetColorTable()
{
	return mColorTable;
}

void Pixels::SetTransparencyIndex(short index)
{
	mTransparent = true;
	mTransparentIndex = index;
	mMaskRowBytes = ((mWidth + 15) >> 4)*2;				// Monochrome Mask is aligned to 16 bits
	mMask = (uchar *)malloc(mMaskRowBytes*2*mHeight);	// 
//	NP_ASSERT(mMask);
}

//	Get the mask if one is required

bool	Pixels::GetMask(uchar **mask, long *rowBytes)
{
	if (!mTransparent) return false;
	if (mTransparentCount == 0) return false; // No pixels were actually transparent!
	*mask = mMask;
	*rowBytes = mMaskRowBytes;
	return true;
}

void Pixels::EndLine(long srcLine, long dstLine)
{
	if (!mTransparent) return;
	if (dstLine == -1)
		dstLine = srcLine;
	uchar* src = GetLineAddr(srcLine);

//	Create a mask

	uchar* mask = mMask + mMaskRowBytes*dstLine;	// Mask are not flipped...
	short bits = 0;
	short x;
	for (x = 0; x < mWidth; x++) {
		if (src[x] == mTransparentIndex) {
			bits = (bits << 1) | 1;
			mTransparentCount++;
		} else {
			bits <<= 1;
		}
		if (((x + 1) & 0x7) == 0)
			mask[x >> 3] = bits;
	}
	if ((x & 0x7) != 0)
		mask[x >> 3] = bits << (8 - (x & 0x7));	// Last bits
}

void Pixels::SetBackground(long background)
{
	mBackground = background;
}

bool Pixels::GetTransparentColor(long *color)
{
	if (mTransparent && mColorTable) {
		uchar *c = mColorTable + 3*mTransparentIndex;
		*color = (c[0] << 16) | (c[1] << 8) | c[2];
	}
	return mTransparent;
}

void Pixels::FillWithPattern(Pixels *pattern, long hOffset, long vOffset)
{
	//pprint("Pixels::FillWithPattern  mHeight %d  mWidth %d  patHeight %d  patWidth %d", mHeight, mWidth, pattern->mWidth, pattern->mHeight);
	if (mDepth != pattern->mDepth) {
		pprint("FillWithPattern: Depth mismatch");
		Erase(0, true);
		return;
	}
	
	hOffset = hOffset % pattern->mWidth;
	vOffset = vOffset % pattern->mHeight;
	
	for (int y = 0; y < mHeight; y++) {
		long curX = 0;
		uchar *srcLine = ((y + vOffset) % pattern->mHeight) * pattern->mRowBytes + pattern->mBaseAddr;
		uchar *dstLine = mRowBytes * y + mBaseAddr;
		bool firstFragment = true;
		int bpp = mDepth / 8;
		while (curX < mWidth) {
			long xSize;
			long xOffset;
			if (firstFragment) {
				xSize = MIN(pattern->mWidth - hOffset, mWidth - curX);
				xOffset = hOffset;
				firstFragment = false;
			} else {
				xSize = MIN(pattern->mWidth, mWidth - curX);
				xOffset = 0;
			}
			//pprint("Copying %d bytes from 0x%x to 0x%x", xSize * bpp, 
			memcpy(dstLine + curX * bpp, srcLine + xOffset * bpp, xSize * bpp);
			curX += xSize;
		}
	}
}


// ===========================================================================

Style gDefaultStyle = {0,0,0,0,0,0,0,0,0,0,0,0};

DrawPort::DrawPort()
{
	mFontAscent = 0;
	mFontDescent = 0;

	mTextColor = 0x000000;		// Black
	//mBGColor = GetSysColor (COLOR_BTNFACE);
	mBGColor = 0xffffff;		// gray 198
	mLinkColor = 0x0000FF;		// Link color
	mVLinkColor = 0x52188C;		// Visited link color
	mALinkColor = 0xFF0000;		// Active link
	mDefaultTextColor = 0;

	mColor = mBGColor;
	mBackColor = 0xFFFFFF;
	mPenSize = 1;
	
	mDrawingOffscreen = false;
	mPrintingMode = false;
}

DrawPort::~DrawPort()
{  
}

void DrawPort::SetColors(long textColor, long bgColor, long link, long vlink, long alink)	// 5 important colors port needs to know about
{
	mTextColor = mDefaultTextColor = textColor;
	mBGColor = bgColor;
	mLinkColor = link;
	mVLinkColor = vlink;
	mALinkColor = alink;
}

// Use the mALinkColor color when clicking on an image with a border

void DrawPort::SetBorderColor(bool selected)
{
	if (selected)
		SetColor(mALinkColor);
	else
		SetColor(mLinkColor);
}

//	Will use the windows content color

void DrawPort::SetGray(short gray)
{
	SetColor(gray,gray,gray);
}

void DrawPort::SetColor(long rgb)
{
	SetColor((short)((rgb >> 16) & 0xFF),(short)((rgb >> 8) & 0xFF),(short)(rgb & 0xFF));
}

void DrawPort::SetBackColor(long rgb)
{
	SetBackColor((short)((rgb >> 16) & 0xFF),(short)((rgb >> 8) & 0xFF),(short)(rgb & 0xFF));
}

long DrawPort::GetBackColor()
{
	return mBGColor;
}

void DrawPort::DrawRule(BRect *r, bool noShade)
{
	if (noShade)
		PaintRect(r);
	else
		DrawBevel(r);
}

void DrawPort::DrawBevel(BRect *r)
{
	if (mBGColor == 0xFFFFFF) {	// White background
		//SetGray(128);
		//FrameRect(r);

		SetGray(176);
		MoveTo(r->left,r->bottom-1);
		LineTo(r->left,r->top);
		LineTo(r->right-1,r->top);
		SetGray(232);
		LineTo(r->right-1,r->bottom-1);
		LineTo(r->left,r->bottom-1);
		SetGray(0);
		return;
	}
	
	SetGray(128);
	MoveTo(r->left,r->bottom-1);
	LineTo(r->left,r->top);
	LineTo(r->right-1,r->top);
	SetGray(240);
	LineTo(r->right-1,r->bottom-1);
	LineTo(r->left,r->bottom-1);
	SetGray(0);
}

void DrawPort::DrawAntiBevel(BRect *r)
{
	if (mBGColor == 0xFFFFFF) {	// White background
		//SetGray(128);
		//FrameRect(r);
		SetGray(232);
		MoveTo(r->left,r->bottom-1);
		LineTo(r->left,r->top);
		LineTo(r->right-1,r->top);
		SetGray(176);
		LineTo(r->right-1,r->bottom-1);
		LineTo(r->left,r->bottom-1);
		SetGray(0);
		return;
	}

	SetGray(240);
	MoveTo(r->left,r->bottom-1);
	LineTo(r->left,r->top);
	LineTo(r->right-1,r->top);
	SetGray(128);
	LineTo(r->right-1,r->bottom-1);
	LineTo(r->left,r->bottom-1);
	SetGray(0);
}

void DrawPort::DrawRoundBevel(BRect *r)
{
	r->right++;
	r->bottom++;
	SetGray(240);
	r->OffsetBy(-1,-1);
	FrameOval(r);
	SetGray(128);
	r->OffsetBy(1,1);
	FrameOval(r);
	SetGray(0);
	r->right--;
	r->bottom--;
}

void DrawPort::DrawTriBevel(BRect *r)
{
	SetGray(240);
	r->OffsetBy(-1,-1);
	MoveTo(r->left,r->top);
	LineTo(r->right,r->top);
	LineTo((r->right + r->left)/2,r->bottom);
	LineTo(r->left,r->top);

	SetGray(128);
	r->OffsetBy(1,1);
	MoveTo(r->left,r->top);
	LineTo(r->right,r->top);
	LineTo((r->right + r->left)/2,r->bottom);
	LineTo(r->left,r->top);
	SetGray(0);
}

//	Setup font caches or whatever here

void	DrawPort::BeginDrawing(BRect *clip)
{
	if (clip)
		mClip = *clip;
}

void	DrawPort::EndDrawing()
{
}


BRect&	DrawPort::GetClip()
{
	return mClip;
}

void DrawPort::SetPrintingMode(bool on)
{
	mPrintingMode = on;
}

//	Implemented in MacDrawPort.cpp or WinDPort.cpp

void	DrawPort::SetColor(short, short, short) {};
void	DrawPort::SetBackColor(short, short, short) {};
	
float		DrawPort::GetFontAscent() { return mFontAscent; };
float		DrawPort::GetFontDescent() { return mFontDescent; };
float		DrawPort::BlankLineHeight() { return 14; };

void	DrawPort::MoveTo(float, float) {};
void	DrawPort::LineTo(float, float) {};
void	DrawPort::PenSize(float, float) {};
	
void	DrawPort::DrawPixels(Pixels *, BRect *, BRect *, bool, bool) {};
void	DrawPort::InvertRgn(BRegion *) {};
	
void	DrawPort::EraseRect(BRect *) {};
void	DrawPort::FrameRect(BRect *) {};
void	DrawPort::PaintRect(BRect *) {};
void 	DrawPort::InvertRect(BRect *) {};
	
void 	DrawPort::FrameOval(BRect *) {};
	
int		DrawPort::MapOrd(float x) {return (int)x; };
void	DrawPort::MapOrdRect(BRect *) {};
void	DrawPort::SetTransparent() {};
