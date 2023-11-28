// ==========================================================================
//	GIF.cpp
//  Copyright 1998-2000 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
//
//	Yanked out of NetPositive by Mike Clifton for use in Wagner
// ==========================================================================

#include "GIF.h"
#include <Debug.h>
#include <stdio.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <InterfaceDefs.h>
#include <Bitmap.h>

#define PACK_RGB16(r,g,b) ((uint16)( 0x8000 | (((r) << 7) & 0x7c00) | (((g) << 2) & 0x03e0) | (((b) >> 3) & 0x001f) ))
#define PACK_RGB32(r,g,b) ((uint32)( 0xff000000 | ((r) << 16) | ((g) << 8) | (b) ))
#define PACK_ARGB32(a,r,g,b) ((uint32)( ((a) << 24) | ((r) << 16) | ((g) << 8) | (b) ))
#define xGetCode(_x) ((_x < mBits) ? (mLast32 >> (32 - _x - (mBits -= _x))) & ((1 << _x) - 1) : GetBitsRead(_x))
#define LZWPixel() ((mSP > mStack) ? *--mSP : DecodeLZW())

enum {
	kGlobalImageHeader = 0,
	kReadExtension,
	kLocalImageHeader,
	kLocalImageBodySetup,
	kLocalImageBody,
	kLocalImageStop,
	kLocalImageEnd,
	kGlobalImageEnd,
	kGIFError
};

enum {
	kNotSpecified = 1,
	kDoNotDispose,
	kRestoreToBackground,
	kRestoreToPrevious
};

#if DITHER
uint32 jb_seed=0x12345678;
#endif

GIF::GIF()
	:
		mFormat(0),
		mPhase(0),
		mLooping(false),
		mImageIndex(0),
		mMaxIndex(0),
		mWidth(0),
		mHeight(0),
		mLocalTop(0),
		mLocalLeft(0),
		mLocalWidth(0),
		mLocalHeight(0),
		mInterlaced(false),
		mDelay(0),
		mOldDelay(0),
		mTransparency(0),
		mGlobalTransparency(0),
		mTransparencyIndex(0),
		mDisposal(kDoNotDispose),
		mLastData(0),
		mLastSize(0),
		mIsAnimation(false),
		mNetscapeAnimation(false),
		mEndOfLZW(0),
		mMaxYPos(0),
		mBackGroundColor(0),
		mLocalColorTable(0)
{
}

GIF::~GIF()
{
	Reset();
}

bool GIF::GetRect(BRect *r)
{
	r->Set(0, 0, mWidth - 1, mHeight - 1);
	return (mWidth && mHeight);
}

long GIF::Write(uchar *data, long dataLength, BBitmap *bmp)
{
	switch (mPhase) {
		case kGlobalImageHeader:
			return ReadGlobalHeader(data,dataLength);
		case kReadExtension:
			return ReadExtension(data,dataLength);
		case kLocalImageHeader:
			return ReadImageHeader(data,dataLength);
		case kLocalImageBodySetup:
			if (mOldDelay) {
				unsigned long time =  system_time()/10000;
				if (mOldDelay > time) // Not time to display next frame yet
					return 0;
				
				mOldDelay = mDelay;
				
				// If we get behind by more than a second, then catch up.
				if (mOldDelay < time - 100)
					mOldDelay = time;
				
				// DKH -- don't clear mDelay; if the next image doesn't
				// include a delay, the existing delay will still be
				// before the current time so it won't matter.  Clearing
				// this makes it impossible for the caller to get the
				// delay until the next animation.
				//mDelay = 0;
			}
			return SetupImageBody(data,dataLength,bmp);

		case kLocalImageBody:
			return ReadImageBody(data,dataLength,bmp);
		
		case kLocalImageStop:
			if (mImageIndex == 1) {
				// check if first image was completely read;
				// if it wasn't, there are some transparent parts.
				short	maxY = mLocalTop + mLocalHeight;
				short	maxX = mLocalLeft + mLocalWidth;
				if (mXPos < maxX || mYPos < maxY) {
					mGlobalTransparency = true;
				}
			}
			mPhase = kLocalImageEnd;
			return 0;
			
		case kLocalImageEnd:
			mPhase = kReadExtension;
			return Write(data,dataLength,bmp);
			
		case kGlobalImageEnd:
			if (mNetscapeAnimation && mNetscapeIterations > 0)
				mNetscapeIterations--;
			mPhase = 0;
			return Write(data, dataLength, bmp);
			
		default:
			return -1;
	}

	return 0;
}

int32 GIF::CurrentFrame() const
{
	return mImageIndex;
}

int32 GIF::CountFrames() const
{
	return mMaxIndex;
}

size_t GIF::GetMemoryUsage() const
{
	return mLocalColorTable ? (256*3) : 0 + mLastSize;
}

void GIF::Restart()
{
	mPhase = kGlobalImageHeader;
	mImageIndex = 0;
}

void GIF::Reset()
{
	mPhase = kGlobalImageHeader;
	free(mLocalColorTable);
	mLocalColorTable = 0;
	free(mLastData);
	mLastData = 0;
	mLastSize = 0;
	mLooping = false;
	mImageIndex = mMaxIndex = 0;
	mDelay = 0;
	mOldDelay = 0;
	mInterlaced = false;
	mBackGroundColor = 0;
	mTransparency = mGlobalTransparency = false;
	mDisposal = mLastDisposal = kDoNotDispose;
	mTransparencyIndex = 0;
}

// Read the header info in from the GIF file
long GIF::ReadGlobalHeader(uchar *data, long dataLength)
{
	if (dataLength < 13)
		return 0;	// Didn't read the header

	//	Reset at start of image
	mWidth = mHeight = 0;
	mLocalTop = mLocalLeft = mLocalWidth = mLocalHeight = 0;
	mImageIndex = 0;
	mDelay = 0;
	mInterlaced = false;
	mBackGroundColor = 0;
	mTransparency = mGlobalTransparency = false;
	mLastDisposal = mDisposal;
	mDisposal = kDoNotDispose;
	mTransparencyIndex = 0;

	mFormat = 0;
	if (!strncmp((char *)data,"GIF87a",6))
		mFormat = 87;
	else if (!strncmp((char *)data,"GIF89a",6))
		mFormat = 89;

	if (mFormat == 0)
		return -1;

	mWidth = data[6] + (data[7] << 8);
	mHeight = data[8] + (data[9] << 8);
	short packed = data[10];
	mBackGroundColor = data[11];

	//	Read global color table
	short colorTableSize = 3 << ((packed & 0x7) + 1);
	if (packed & 0x80) {
		PRINT(("Need %d bytes for full header\n", 13+colorTableSize));
		if (dataLength < (13 + colorTableSize))		// Read header, not enough for color table
			return 0;
			
		memcpy(mGlobalColorTable,data + 13,colorTableSize);
	} else
		colorTableSize = 0;
	
	mPhase = kReadExtension;
	mImageIndex = 0;
	
	return 13 + colorTableSize;
}

long GIF::ReadComment(uchar *data, long dataLength)
{
	short count;
	uchar *src = data + 2;
	dataLength -= 2;
	while ((bool)(count = *src++)) {
		if (dataLength <= (count + 1))	// Not enough data here to read extension
			return 0;
		src += count;
	}

	return src - data;
}

long GIF::ReadGraphicControl(uchar *data, long dataLength)
{
	if (dataLength < 8) return 0;	// Total size of Graphic extension
	mTransparency = (data[3] & 1);
	if (mTransparency)
		mGlobalTransparency = true;
	
	mLastDisposal = mDisposal;
	mDisposal = (((data[3]>>2)&7)+1);
#if DEBUG
	switch (mDisposal) {
		case kNotSpecified: printf("GIF: Disposal not specified\n");  break;
		case kDoNotDispose:	printf("GIF: Do not dispose\n");	break;
		case kRestoreToBackground:	printf("GIF: Restore to background\n");	break;
		case kRestoreToPrevious:	printf("GIF: Restore to previous\n");	break;
		default:
			printf("Disposal: %d\n",mDisposal);
	}
#endif

	if (mDisposal == kNotSpecified)
		mDisposal = kDoNotDispose;
	
	mTransparencyIndex = data[6];
	mDelay = data[4] | (long)data[5] << 8;

	// Make delay no shorter than one display field (1/30 sec).
	if (mDelay < (100/30))
		mDelay = 100/30;
	
	// When loading the first frame of animation, mOldDelay will be zero.  Initialize
	// it to the current time, but don't set mDelay, because we want the first frame
	// to display right away.	
	if (mOldDelay == 0)
		mOldDelay = system_time() / 10000;
	else if (mDelay)
		mDelay += mOldDelay;	// Time in 100ths of a second when delay expires
	else
		mDelay = system_time() / 10000 + 1;
	
	return 8;
}

//	Check to see if the animation should continue looping
bool GIF::LoopAnimation()
{
	return mNetscapeAnimation && mNetscapeIterations != 0;
}

bool GIF::IsAnimating()
{
	return ((mNetscapeAnimation && mNetscapeIterations != 0) || 
			(!mNetscapeAnimation && mIsAnimation))/* && gPreferences.FindBool("ShowAnimations")*/;
}

bool GIF::IsTransparent()
{
	return mTransparency || mGlobalTransparency;
}

//	Read an application extension
long GIF::ReadApplication(uchar *data, long dataLength)
{
	uchar *src = data + 2;
	short count;
	
	dataLength -= 2;
	while ((bool)(count = *src++)) {
		if (dataLength <= (count + 1))	// Not enough data here to read extension
			return 0;

		PRINT(("Reading app extension: count=%d, src=%p, data=%p\n",
				count, src, data));
				
		if (count == 11 && strncmp((char *)src,"NETSCAPE",8)==0) {
			PRINT(("Set netscape animation\n"));
			mNetscapeAnimation = true;
			mIsAnimation = true;
		} else if (count == 3 && mNetscapeAnimation && src[0] == 1) {	// Iteration count
			if (mLooping == false) {
				mNetscapeIterations = src[1] | ((long)src[2]) << 8;
				PRINT(("Netscape Animation has %ld iterations\n",mNetscapeIterations));
				if (mNetscapeIterations == 0)
					mNetscapeIterations = -1;	// No interations mean loop forever like everyone else ¥¥¥¥¥¥¥¥¥¥
			}
		}

		src += count;
	}
	return src - data;
}


//	Read the next extension

long GIF::ReadExtension(uchar *data, long dataLength)
{
	short	count;

	uchar	*src;
	switch (*data) {
		case 0x3B:
			mPhase = kGlobalImageEnd;
			return 1;
		case 0x2C:
			return ReadImageHeader(data,dataLength);
		case 0x21:							// Control Extension
			if (dataLength < 3) return 0;	// Not enough data for the smalllest extension
#if DEBUG
			{
				printf("Control:");
				for( int i=0; i<data[2]+3; i++ ) printf(" %02x", data[i]);
				printf("\n");
			}
#endif
			switch (data[1]) {
				case 0xFE:	return ReadComment(data,dataLength);
				case 0xF9:	return ReadGraphicControl(data,dataLength);
				case 0x01:	/*pprint("Plain Text Extension");*/				break;
				case 0xFF:	return ReadApplication(data,dataLength);
			}
			src = data + 2;
			dataLength -= 2;
			while ((bool)(count = *src++)) {
				if (dataLength <= (count + 1))	// Not enough data here to read extension
					return 0;
				src += count;
			}
			return src - data;
			break;
		default:
			return -1;
	}
}

// Read local image
long GIF::ReadImageHeader(uchar *data, long dataLength)
{
	if (dataLength < 10) return 0;	// Didn't read the local image header
	if (data[0] != 0x2C)
		return -1;
	
	mLastLeft = mLocalLeft;
	mLastTop = mLocalTop;
	mLastWidth = mLocalWidth;
	mLastHeight = mLocalHeight;
	
	mLocalLeft = data[1] + (data[2] << 8);
	mLocalTop = data[3] + (data[4] << 8);
	mLocalWidth = data[5] + (data[6] << 8);
	mLocalHeight = data[7] + (data[8] << 8);

	short packed = data[9];
	mInterlaced = packed & 0x40;
	
	// Global with and height was not spec, set it now
	if (mWidth == 0 || mLocalWidth > mWidth)
		mWidth = mLocalWidth;
	if (mHeight == 0 || mLocalHeight > mHeight)
		mHeight = mLocalHeight;
	
	// If the GIF is corrupt and specifies bogus local
	// coordinates, fix them up.
	if (mLocalLeft + mLocalWidth > mWidth)
		mLocalLeft = mWidth - mLocalWidth;
	if (mLocalTop + mLocalHeight >= mHeight)
		mLocalTop = mHeight - mLocalHeight;
		
	short colorTableSize = 3 << ((packed & 0x07) + 1);
	if (packed & 0x80) {
		if (dataLength < (10 + colorTableSize))		// Read header, not enough for color table
			return 0;

		if (mLocalColorTable == 0)
			mLocalColorTable = (uchar *)malloc(256*3);

		memcpy(mLocalColorTable,data+10,colorTableSize);
	} else {
		if (mLocalColorTable)
			free(mLocalColorTable);

		mLocalColorTable = NULL;
		colorTableSize = 0;
	}

	mPhase = kLocalImageBodySetup;
	
	return 10 + colorTableSize;
}

long GIF::GetBitsRead(short count)
{
	long a,b;
	short i;
	if( mBufferCount < 4 ) {
		b = 0;
		switch (mBufferCount) {
		case 3:
			b |= (((long)mData[2]) << 16);
		case 2:
			b |= (((long)mData[1]) << 8);
		case 1:
			b |= ((long)mData[0]);
			break;

		default:
			return -1;
		}
		mData += mBufferCount;
		mBufferCount = 0;
	}
	else {
		b = ((long)mData[0]);
		b |= (((long)mData[1]) << 8);
		b |= (((long)mData[2]) << 16);
		b |= (((long)mData[3]) << 24);
		mData += 4;
		mBufferCount -= 4;
	}

  	a = mBits ? (mLast32 >> (32 - mBits)) : 0;
	count -= mBits;
	mLast32 = b;

	i = mBits;
	mBits = 32 - count;
	return a | (mLast32 & ((1 << count) - 1)) << i;
}

// Alocate tables for decode
short GIF::InitLZW(short initCodeSize)
{
	mEndOfLZW = 0;
	mSetCodeSize = initCodeSize;
	mCodeSize = mSetCodeSize + 1;
	mClearCode = 1 << mSetCodeSize;
	mEndCode = mClearCode + 1;
	mMaxCodeSize = mClearCode << 1;
	mMaxCode = mClearCode + 2;

	memset(&mDictionary[mClearCode], 0, (kDictionarySize - mClearCode) * sizeof(dictionary_entry));
	for (int i = 0; i < mClearCode; i++) {
		mDictionary[i].mSuffix = i;
		mDictionary[i].mPrefix = 0;
	}

	short code;
	do {
		code = xGetCode(mCodeSize);
	} while (code == mClearCode);
	mFirstCode = code;
	mOldCode = code;
	mSP = mStack;
	*mSP++ = mFirstCode;
	return 0;
}

// Decode the next pixel.
short GIF::DecodeLZW()
{
	short code,thisCode;
	short* stackEnd = mStack + (kDictionarySize + 1);
	if ((code = xGetCode(mCodeSize)) < 0)
		return code;

	if (code == mClearCode) {				// Clear code resets tables		
		memset(&mDictionary[mClearCode], 0, (kDictionarySize - mClearCode) * sizeof(dictionary_entry));
		mCodeSize = mSetCodeSize + 1;
		mMaxCodeSize = mClearCode << 1;
		mMaxCode = mClearCode + 2;
		mSP = mStack;
		mFirstCode = mOldCode = xGetCode(mCodeSize);
		return mFirstCode;
	}

	if (code == mEndCode)
		return -2;

	thisCode = code;
	if (code >= mMaxCode) {
		if (mSP >= stackEnd ) {
			DEBUGGER("LZW stack out of bounds");
			return -1;
		}

		*mSP++ = mFirstCode;
		code = mOldCode;
	}
	
	while (code >= mClearCode) {
		if( mSP >= stackEnd ) {
			DEBUGGER("LZW stack out of bounds");
			return -1;
		}
		*mSP++ = mDictionary[code].mSuffix;
		short prefixCode = mDictionary[code].mPrefix;
		if (code == prefixCode) {
			PRINT(("*** Circular table entry\n"));
			return -1;
		}

		code = prefixCode;
	}

	if( mSP >= stackEnd ) {
		DEBUGGER("LZW stack out of bounds");
		return -1;
	}

	*mSP++ = mFirstCode = mDictionary[code].mSuffix;
	if ((code = mMaxCode) < kDictionarySize) {
		mDictionary[code].mPrefix = mOldCode;
		mDictionary[code].mSuffix = mFirstCode;
		mMaxCode++;
  		if ((mMaxCode >= mMaxCodeSize) && (mMaxCodeSize < kDictionarySize)) {
			mMaxCodeSize *= 2;
			mCodeSize++;
		}
	}

	mOldCode = thisCode;
	if (mSP > mStack) {
		return *--mSP;
	}

	return code;
}

//	Setup the LZW decode and allocate the image buffer and pixmap
static void erase_rect(uchar* bits, color_space colors, size_t rowBytes, size_t size,
						uint32 left, uint32 top, uint32 width, uint32 height)
{
	uchar* startAddr = bits + rowBytes*top;
	uchar* endAddr = bits + rowBytes*(top+height);
	if (endAddr > (bits+size))
		endAddr = bits+size;
	
	width += left;
	if (colors == B_RGBA15) {
		uint16* pixPtr = (uint16*)startAddr;
		while ((uchar*)pixPtr < endAddr) {
			for( uint32 i=left; i<width; i++ )
				pixPtr[i] = B_TRANSPARENT_MAGIC_RGBA15;

			pixPtr += rowBytes/2;
		}
	} else if (colors == B_RGBA32) {
		uint32* pixPtr = (uint32*)startAddr;
		while ((uchar*) pixPtr <  endAddr) {
			for( uint32 i=left; i<width; i++ )
				pixPtr[i] = B_TRANSPARENT_MAGIC_RGBA32;

			pixPtr += rowBytes/4;
		}
	}
}

long GIF::SetupImageBody(uchar *data, long dataLength, BBitmap *bmp)
{
	uchar *src = data;
	if (dataLength < (1 + 1 + src[1]))		// codeSize, blockCount, count
		return 0;

	if (bmp == NULL)						// No Pixels yet...
		return 0;

	// Setup destination pixels
	if (mImageIndex == 0)
		if (mLooping == false)
			mLooping = true;

	// For first image, we pretend the last image covered everything
	// and erased it all to the background color.
	if( mImageIndex == 0 ) {
		mLastDisposal = kRestoreToBackground;
		mLastTop = mLastLeft = 0;
		mLastWidth = mWidth;
		mLastHeight = mHeight;
	}
	
	// Cache bitmap information.
	uchar	*baseAddr = (uchar *)bmp->Bits();
	color_space colors = bmp->ColorSpace();
	const uint32 rowBytes = bmp->BytesPerRow();
	const size_t size = bmp->BitsLength();
	
	if (mLastDisposal == kRestoreToPrevious) {
		// Dispose by restoring previous data over last image.
		size_t bpp = 0;
		if (colors == B_RGBA15) bpp = 2;
		else if (colors == B_RGBA32) bpp = 4;
		else if (colors == B_CMAP8) bpp = 1;
		if (bpp && mLastData && mLastWidth > 0 && mLastHeight > 0) {
			for (int32 i=0; i<mLastHeight; i++) {
				memcpy(baseAddr+(i+mLastTop)*rowBytes+mLastLeft*bpp,
					   mLastData+i*mLastWidth*bpp,
					   mLastWidth*bpp);
			}
		}
	
	} else if (mLastDisposal == kRestoreToBackground)
		erase_rect(baseAddr, colors, rowBytes, size, mLastLeft, mLastTop,
			mLastWidth, mLastHeight);
	
	if (mDisposal == kRestoreToPrevious) {
		// Need to save current background so it can be restored.
		size_t bpp = 0;
		if (colors == B_RGBA15) bpp = 2;
		else if (colors == B_RGBA32) bpp = 4;
		else if (colors == B_CMAP8) bpp = 1;
		if (bpp && mLocalWidth > 0 && mLocalHeight > 0) {
			mLastSize = mLocalWidth*mLocalHeight*bpp;
			mLastData = (char*)realloc(mLastData, mLastSize);
			for (int32 i=0; i<mLocalHeight; i++) {
				memcpy(mLastData+i*mLocalWidth*bpp,
					   baseAddr+(i+mLocalTop)*rowBytes+mLocalLeft*bpp,
					   mLocalWidth*bpp);
			}
		}
	} else if (mLastData) {
		free(mLastData);
		mLastData = 0;
		mLastSize = 0;
	}
	
	mImageIndex++;
	if (mImageIndex > mMaxIndex) mMaxIndex = mImageIndex;
	
	if (mImageIndex > 1)
		mIsAnimation = true;
	
	mPass = 0;
	mYPos = mMaxYPos = mLocalTop;
	mXPos = mLocalLeft;
	
	// Start the LZW decode
	short codeSize = *src++;
	mBits = 0;
	mBufferCount = 0;

	mBufferCount = *src++;			// Write the first chunk of data into the decoder
	memcpy(mBuffer,src,mBufferCount);
	src += mBufferCount;
	mData = mBuffer;
	if (InitLZW(codeSize))
		return -1;

	mPhase = kLocalImageBody;		// Start reading the body
	return src - data;
}

int GIF::FillBuffer(uchar *src, int dataLength)
{
	int added = 0;
	
	if (mEndOfLZW)
		return 0;
	
	if (dataLength < 1)
		return -1;

	// Add data to buffer if it is nearly empty
	short i;
	if (mBufferCount < 4) {
		if (dataLength <= *src)
			return -1;							// Run out of data, return

		for (i = 0; i < mBufferCount; i++)
			mBuffer[i] = *mData++;

		added = mBufferCount = *src++;
		added++;
		if (mBufferCount == 0) {
			mBufferCount = 8;
			memset(mBuffer+i,0,mBufferCount);		// Lie so writing will finish
			mData = mBuffer;
			mEndOfLZW = true;
		} else
			memcpy(mBuffer+i,src,mBufferCount);

		mBufferCount += i;
		mData = mBuffer;
	}

	return added;
}

#if DITHER
inline uint32 jb_rand()
{
	return (jb_seed=(jb_seed<<5|jb_seed>>27)+3141592654UL);
}
#endif

long GIF::ReadImageBody(uchar *data, long dataLength, BBitmap *bmp)
{
	short	p = 0;
	uchar	*src = data;

	if (bmp == NULL)						// No Pixels yet...
		return 0;

	short maxY = mLocalTop + mLocalHeight;
	short maxX = mLocalLeft + mLocalWidth;
	uchar *baseAddr = (uchar *)bmp->Bits();
	uint32 rowBytes = bmp->BytesPerRow();
	uchar *rowPtr;
	uchar *colorTable = mGlobalColorTable;

	if (mLocalColorTable)
		colorTable = mLocalColorTable;

	while (mYPos < maxY) {
		rowPtr = baseAddr + rowBytes * mYPos;
		if (bmp->ColorSpace() == B_RGBA15) {
			uint16	*pixPtr = (uint16 *)rowPtr;
			pixPtr += mXPos;
			while (mXPos < maxX && mBufferCount >= 4) {
				if ((p = LZWPixel()) < 0) goto bail;
				if (!mTransparency || p != mTransparencyIndex) {
					// Lookup in color table
					uchar red = colorTable[p * 3];
					uchar green = colorTable[p * 3 + 1];
					uchar blue = colorTable[p * 3 + 2];

#if DITHER
					// Dither
					red=(red>=248)?248:((red&7)<(jb_rand()&7))?red&248:red+8&248;
					green=(green>=248)?248:((green&7)<(jb_rand()&7))?green&248:green+8&248;
					blue=(blue>=248)?248:((blue&7)<(jb_rand()&7))?blue&248:blue+8&248;
#endif

					// Pack
					*pixPtr = PACK_RGB16(red, green, blue);
				}
				pixPtr++;
				mXPos++;
			}
		} else if (bmp->ColorSpace() == B_RGBA32) {
			uint32	*pixPtr = (uint32 *)rowPtr;
			pixPtr += mXPos;
			while (mXPos < maxX && mBufferCount >= 4) {
				if ((p = LZWPixel()) < 0) goto bail;
				if (!mTransparency || p != mTransparencyIndex)
					*pixPtr = PACK_RGB32(colorTable[p * 3], colorTable[p * 3 + 1], colorTable[p * 3 + 2]);

				pixPtr++;
				mXPos++;
			}
		}
		
		if (mXPos == maxX) {
			mXPos = mLocalLeft;				// Move to the next line
			if (mInterlaced) {
skippedPass:
				switch (mPass) {			// Draw interlaced
					case 0:
					case 1:	mYPos += 8;	break;
					case 2:	mYPos += 4;	break;
					case 3: mYPos += 2;	break;
				}

				if (mYPos >= (mLocalTop + mLocalHeight)) {
					switch (++mPass) {
						case 1:	mYPos = mLocalTop + 4; break;
						case 2:	mYPos = mLocalTop + 2; break;
						case 3:	mYPos = mLocalTop + 1; break;
					}
				}

				if (!mTransparency && !mNetscapeAnimation && !mIsAnimation) {
					// Teporarily fill in the rest of the GIF with duplicate copies of this
					// line to give the effect of an image whose resolution progressively
					// improves.
					int bottomLine = 0;
					int topLine = 0;
					switch (mPass) {
						case 0: topLine = mYPos - 8; bottomLine = mYPos    ; break;
						case 1: topLine = mYPos - 8; bottomLine = mYPos - 4; break;
						case 2: topLine = mYPos - 4; bottomLine = mYPos - 2; break;
						case 3: break;
					}
					
					bottomLine = MIN(bottomLine, mLocalTop + mLocalHeight);
					
					if (topLine > 0)
						for (int extraY = topLine + 1; extraY < bottomLine; extraY++)
							memcpy(baseAddr + rowBytes * extraY, baseAddr + rowBytes * topLine, rowBytes);
				}

				if (mYPos >= maxY && mPass < 4)
					goto skippedPass;
			} else
				mYPos++;					// Draw Progressive

			if (mYPos > mMaxYPos)
				mMaxYPos = mYPos;
		}

		// If Buffer is nearly empty .. put more data into it
		int bytesRead = FillBuffer(src,dataLength);
		if (bytesRead) {
			if (bytesRead < 0)
				goto done;
				
			src += bytesRead;
			if (dataLength < bytesRead) {
				p = -1;
				goto bail;
			}

			dataLength -= bytesRead;
		} else if (mBufferCount < 4)
			break;
	}

bail:
	if (mYPos >= maxY || p < 0) {
		// Image is complete, an extra blob of data with zero size is sometimes present
		if (dataLength >= 1) {
			if (src[0] == 0) {
				src++;
				--dataLength;
			}
		}
		
		mMaxYPos = mHeight;
		mPhase = kLocalImageStop;
	}
	
done:
	return src - data;
}

short GIF::GetMaxYPos()
{
	return mMaxYPos;
}
