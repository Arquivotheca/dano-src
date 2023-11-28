// ===========================================================================
//	GIF.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "GIF.h"
#include "NPApp.h"
#include "MessageWindow.h"
#ifdef R4_COMPATIBLE
#include "Utils.h"
#endif

#include <malloc.h>

//===========================================================================

enum {
	kGlobalImageHeader = 0,
	kReadExtension,
	kLocalImageHeader,
	kLocalImageBodySetup,
	kLocalImageBody,
	kLocalImageEnd,
	kGlobalImageEnd,
	kGIFError
};

GIF::GIF()
{
	mWidth = mHeight = 0;
	mLocalTop = mLocalLeft = mLocalWidth = mLocalHeight = 0;
	mImageIndex = 0;
	mLooping = false;
	mNetscapeAnimation = false;
	mIsAnimation = false;
	
	mDelay = 0;
	mInterlaced = false;
	mBackGroundColor = 0;
	mFormat = 0;
	mPhase = 0;
	mTransparency = 0;
	mDisposal = kDoNotDispose;
	mTransparencyIndex = 0;

//	LZW Decode

	mPrefix = mSuffix = mStack = 0;
	mEndOfLZW = 0;
	
//	Drawing

	mGlobalColorTable = mLocalColorTable = 0;
	mMaxYPos = 0;

	mHasBGColor = false;
	mBGColor = 0;
	mHasBGImage = false;
	mBGImage = NULL;
}

GIF::~GIF()
{
	if (mPrefix) free(mPrefix);
	if (mSuffix) free(mSuffix);
	if (mStack)	 free(mStack);

	if (mLocalColorTable) free(mLocalColorTable);
	if (mGlobalColorTable) free(mGlobalColorTable);
}

bool GIF::GetRect(BRect *r)
{
	r->Set(0,0,mWidth,mHeight);
	return (mWidth && mHeight);
}

void GIF::SetBG(bool hasBGColor, long bgColor, bool hasBGImage, Pixels *bgImage, float bgHOffset, float bgVOffset)
{
	mHasBGColor = hasBGColor;
	mBGColor = bgColor;
	mHasBGImage = hasBGImage;
	mBGImage = bgImage;
	mBGHOffset = bgHOffset;
	mBGVOffset = bgVOffset;
}

//	Write a byte int the GIF decoder, see what comes out

long	GIF::Write(uchar *data, long dataLength, Pixels *pixels)
{
	switch (mPhase) {
		case kGlobalImageHeader:
			return ReadGlobalHeader(data,dataLength);
		case kReadExtension:
			return ReadExtension(data,dataLength);
		case kLocalImageHeader:
			return ReadImageHeader(data,dataLength);

//		case kLocalImageBodySetup:
//			return SetupImageBody(data,dataLength,pixels);

		case kLocalImageBodySetup:
//			if (mOldDelay) {
//				unsigned long time =  system_time()/10000;
//				if (mOldDelay > time)	// Not time to display next frame yet
//					return 0;
//				mOldDelay = mDelay;
//				
//				// If we get behind by more than a second, then catch up.
//				if (mOldDelay < time - 100)
//					mOldDelay = time;
//				mDelay = 0;
//			}
			return SetupImageBody(data,dataLength,pixels);

		case kLocalImageBody:
			return ReadImageBody(data,dataLength,pixels);
		
		case kLocalImageEnd:
			mPhase = kReadExtension;
			return 0;				// Return control at end of local image
			
		case kGlobalImageEnd:
			if (mNetscapeAnimation && mNetscapeIterations > 0)
				mNetscapeIterations--;

			if (mNetscapeAnimation && !gPreferences.FindBool("ShowAnimations"))
				return dataLength;
				
			mPhase = 0;
//			if (dataLength != 0)
//				pprint("GIF::Write: %d extra bytes at end of GIF",dataLength);
			return dataLength;				
			
		default:
			pprint("GIF::Write bad phase? %d",mPhase);
			return -1;
	}
	return 0;
}

//	Read the header info in from the GIF file

long GIF::ReadGlobalHeader(uchar *data, long dataLength)
{
	if (dataLength < 13) return 0;	// Didn't read the header

//	Reset at start of image

	mWidth = mHeight = 0;
	mLocalTop = mLocalLeft = mLocalWidth = mLocalHeight = 0;
	mImageIndex = 0;
	mDelay = 0;
	mInterlaced = false;
	mBackGroundColor = 0;
	mTransparency = false;
	mDisposal = kDoNotDispose;
	mTransparencyIndex = 0;
//	mMaxYPos = 0;

	mFormat = 0;
	if (!strncmp((char *)data,"GIF87a",6)) {
//		pprint("GIF is 87a");
		mFormat = 87;
	}
	if (!strncmp((char *)data,"GIF89a",6)) {
//		pprint("GIF is 89a");
		mFormat = 89;
	}
	if (mFormat == 0) {
		pprint("Bad GIF header: %d bytes of data",dataLength);
		if (*(unsigned short *)data == 0x00FFD8)
			pprint("(Looks like a JPEG file)");
		else
			pprintHex(data,MIN(64,dataLength));
		return -1;
	}

	mWidth = data[6] + (data[7] << 8);
	mHeight = data[8] + (data[9] << 8);
	short packed = data[10];
	mBackGroundColor = data[11];
//	pprint("Image is %d by %d, (0x%2X,0x%2X)", mWidth,mHeight,mPacked,mBackGroundColor);

//	Read global color table

	short colorTableSize = 3 << ((packed & 0x7) + 1);
//	short colorResolution = ((packed >> 4) & 0x7) + 1;
	if (packed & 0x80) {
		if (dataLength < (13 + colorTableSize))		// Read header, not enough for color table
			return 0;
			
		if (mGlobalColorTable)
			free(mGlobalColorTable);
		mGlobalColorTable = (uchar *)malloc(256*3);
//		NP_ASSERT(mGlobalColorTable);
		memcpy(mGlobalColorTable,data + 13,colorTableSize);
//		pprint("Got Global Color table: Size %d, %dbpp",colorTableSize/3,colorResoultion);

//		Want to have the image fade in from black
//		Troublesome for Win16

//		short b = 0;
//		while ((*(long *)(b + mGlobalColorTable) & 0xFFFFFF00))
//			if ((b += 3) >= colorTableSize) break;
//		if (b != colorTableSize)
//			mBackGroundColor = b/3;

	} else
		colorTableSize = 0;
	mPhase = kReadExtension;
	mImageIndex = 0;
	
	return 13 + colorTableSize;
}

//	Read a comment

long GIF::ReadComment(uchar *data, long dataLength)
{
	short 	count;
	uchar *src = data + 2;
	dataLength -= 2;
	while ((bool)(count = *src++)) {
		if (dataLength <= (count + 1))	// Not enough data here to read extension
			return 0;
		src += count;
	}

	return src - data;
}

//	Read Graphic control extension

long GIF::ReadGraphicControl(uchar *data, long dataLength)
{
	if (dataLength < 8) return 0;	// Total size of Graphic extension
	mTransparency = (data[3] & 1);
	
	mDisposal = (((data[3]>>2)&7)+1);
#if 0
	switch (mDisposal) {
		case 1: pprint("GIF: Disposal not specified");  break;
		case 2:	pprint("GIF: Do not dispose");	break;
		case 3:	pprint("GIF: Restore to background");	break;
		case 4:	pprint("GIF: Restore to previous");	break;
		default:
			pprint("Disposal: %d",mDisposal);
	}
#endif
	if (mDisposal == kNotSpecified)
		mDisposal = kDoNotDispose;
	
	mTransparencyIndex = data[6];
	mDelay = data[4] | (long)data[5] << 8;

	// When loading the first frame of animation, mOldDelay will be zero.  Initialize
	// it to the current time, but don't set mDelay, because we want the first frame
	// to display right away.	
//	if (mOldDelay == 0) {
//		mOldDelay = system_time() / 10000;
//	} else
//		if (mDelay)
//			mDelay += mOldDelay;	// Time in 100ths of a second when delay expires

//	Copybits does transparency by setting the background color to the transparent color
//	If the background color appears in the rest of the image, those pixels will be transparent too
//	Set to background color to an unlikely shade, should explicty check clut...
	
//	uchar *color = mGlobalColorTable + mTransparencyIndex*3;
//	color[0] = 0x12;
//	color[1] = 0x34;
// 	color[2] = 0x56;	// An unlikely color
	
//	if (mTransparency)
//		pprint("GIF is Transparent: Index %d",mTransparencyIndex);
	
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
			(!mNetscapeAnimation && mIsAnimation)) && gPreferences.FindBool("ShowAnimations");
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

		if (count == 11 && strstr((char *)src,"NETSCAPE")) {
			mNetscapeAnimation = true;
			mIsAnimation = true;
		} else if (count == 3 && mNetscapeAnimation && src[0] == 1) {	// Iteration count
			if (mLooping == false) {
				mNetscapeIterations = src[1] | ((long)src[2]) << 8;
//				pprintBig("Netscape Animation has %d iterations",mNetscapeIterations);
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
//			pprint("Found Global Image End");
			mPhase = kGlobalImageEnd;
			return 1;
		case 0x2C:
			return ReadImageHeader(data,dataLength);
		case 0x21:							// Control Extension
			if (dataLength < 3) return 0;	// Not enough data for the smalllest extension
			switch (data[1]) {
				case 0xFE:	return ReadComment(data,dataLength);
				case 0xF9:	return ReadGraphicControl(data,dataLength);
				case 0x01:	pprint("Plain Text Extension");				break;
				case 0xFF:	return ReadApplication(data,dataLength);
//				default:	pprint("Bad extension: %d",data[1]);
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
			pprint("Error in GIF file: Bad Extension");
			pprintHex(data,MIN(16,dataLength));
			return -1;
	}
}

//	Read local image

long GIF::ReadImageHeader(uchar *data, long dataLength)
{
	if (dataLength < 10) return 0;	// Didn't read the local image header
	if (data[0] != 0x2C) {
		pprint("Bad image separator: 0x%X",data[0]);
		return -1;
	}
	
	mLocalLeft = data[1] + (data[2] << 8);
	mLocalTop = data[3] + (data[4] << 8);
	mLocalWidth = data[5] + (data[6] << 8);
	mLocalHeight = data[7] + (data[8] << 8);

	short packed = data[9];
	mInterlaced = packed & 0x40;
	
//	Global with and height was not spec, set it now

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
		
//	pprint("Local image at (%d,%d) is %d by %d",mLocalLeft,mLocalTop,mLocalWidth,mLocalHeight);

	if (mLocalColorTable)
		free(mLocalColorTable);
	mLocalColorTable = NULL;
	
	short colorTableSize = 3 << ((packed & 0x07) + 1);
	if (packed & 0x80) {
		if (dataLength < (10 + colorTableSize))		// Read header, not enough for color table
			return 0;
//		pprint("Local color table present: %d",colorTableSize/3);
		mLocalColorTable = (uchar *)malloc(256*3);
//		NP_ASSERT(mLocalColorTable);
		memcpy(mLocalColorTable,data+10,colorTableSize);
	} else
		colorTableSize = 0;

	mPhase = kLocalImageBodySetup;
	
	return 10 + colorTableSize;
}

//===========================================================================

#define xGetCode(_x) ((_x < mBits) ? (mLast32 >> (32 - _x - (mBits -= _x))) & ((1 << _x) - 1) : GetBitsRead(_x))

long GIF::GetBitsRead(short count)
{
	long	a,b;
	short	i;

//	Load up the next 32 bits

//	NP_ASSERT(mBufferCount >= 4);	// Really tiny images (CNN's gray_dot.gif) will trip this
								// Not a bug, but hard to catch without slowing everything down

	//	Intel won't complile a >> by 32!
      
  	a = mBits ? (mLast32 >> (32 - mBits)) : 0;

	count -= mBits;
	b = ((long)mData[0]);
	b |= (((long)mData[1]) << 8);
	b |= (((long)mData[2]) << 16);
	b |= (((long)mData[3]) << 24);
	mData += 4;
	mBufferCount -= 4;
	mLast32 = b;

	i = mBits;
	mBits = 32 - count;
	return a | (mLast32 & ((1 << count) - 1)) << i;
}

//	Alocate tables for decode

short	GIF::InitLZW(short initCodeSize)
{
	short	i;

	mEndOfLZW = 0;
	mSetCodeSize = initCodeSize;
	mCodeSize = mSetCodeSize + 1;
	mClearCode = 1 << mSetCodeSize;
	mEndCode = mClearCode + 1;
	mMaxCodeSize = mClearCode << 1;
	mMaxCode = mClearCode + 2;

	if (mPrefix) free(mPrefix);
	if (mSuffix) free(mSuffix);
	if (mStack)	 free(mStack);

	mPrefix = (short *)malloc(4096*2);
	mSuffix = (short *)malloc(4096*2);
	mStack = (short *)malloc((4096 + 1)*2);
	
	if (!mPrefix || !mSuffix || !mStack)
		return -1;

	memset(mPrefix,0,4096*2);
	memset(mSuffix,0,4096*2);

	for (i = 0; i < mClearCode; i++)
		mSuffix[i] = i;

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

//	Decode the next pixel. Usually on stack .. make a macro

#define LZWPixel() ((mSP > mStack) ? *--mSP : DecodeLZW())

#include <stdio.h>
short GIF::DecodeLZW()
{
	short	i,code,thisCode;

//	if (mSP > mStack)		// Pixels on the stack, very common, put in macro
//		return *--mSP;

	if ((code = xGetCode(mCodeSize)) < 0)
		return code;

	if (code == mClearCode) {				// Clear code resets tables		
/*
		for (i = 0; i < mClearCode; i++) {
			mPrefix[i] = 0;
    		mSuffix[i] = i;
  		}
		for (; i < 4096; i++)
			mPrefix[i] = mSuffix[i] = 0;
*/
		short *prefixptr = &mPrefix[0];
		short *suffixptr = &mSuffix[0];
		short *prefixend = &mPrefix[4096];
		i = 0;
		
		while (prefixptr < prefixend) {
			*prefixptr++ = 0;
			*suffixptr++ = (i < mClearCode) ? i++ : 0;
		}

		mCodeSize = mSetCodeSize + 1;
		mMaxCodeSize = mClearCode << 1;
		mMaxCode = mClearCode + 2;
		mSP = mStack;
		mFirstCode = mOldCode = xGetCode(mCodeSize);
		return mFirstCode;
	}

	if (code == mEndCode)					// End code
		return -2;

	thisCode = code;
	if (code >= mMaxCode) {
		*mSP++ = mFirstCode;
		code = mOldCode;
	}
	
	while (code >= mClearCode) {
		*mSP++ = mSuffix[code];
		short prefixCode = mPrefix[code];
		if (code == prefixCode) {
//			pprint("Circular table entry");
			return -1;
		}
		code = prefixCode;
	}
	*mSP++ = mFirstCode = mSuffix[code];

	if ((code = mMaxCode) < 4096) {
		mPrefix[code] = mOldCode;
		mSuffix[code] = mFirstCode;
		mMaxCode++;
  		if ((mMaxCode >= mMaxCodeSize) && (mMaxCodeSize < 4096)) {
			mMaxCodeSize *= 2;
			mCodeSize++;
		}
	}
	mOldCode = thisCode;
	if (mSP > mStack)
		return *--mSP;

	return code;
}

//================================================================================
//================================================================================
//	Setup the LZW decode and allocate the image buffer and pixmap

long GIF::SetupImageBody(uchar *data, long dataLength, Pixels *pixels)
{
	uchar	*src = data;

	if (dataLength < (1 + 1 + src[1]))		// codeSize, blockCount, count
		return 0;
	if (pixels == 0)						// No Pixels yet...
		return 0;

//	Setup destination pixels

	if (mTransparency) {
		pixels->SetTransparencyIndex(mTransparencyIndex);
	}
		
	if (mImageIndex == 0)
		if (mLooping == false) {
			pixels->Create(mWidth,mHeight,8);	// Create 8 bit Pixel object
			mLooping = true;
		}
	
	if (mLocalColorTable)
		pixels->SetColorTable((uchar *)mLocalColorTable);
	else
		pixels->SetColorTable((uchar *)mGlobalColorTable);
			
	// Treat the disposal options of kRestoreToBackground and kRestoreToPrevious identically.  We know better than
	// the GIF does what to do.
	if (mImageIndex == 0 || mDisposal != kDoNotDispose)
		if (mHasBGImage && mIsAnimation) {
			pixels->FillWithPattern(mBGImage, mBGHOffset, mBGVOffset);
		} else if (mHasBGColor && mIsAnimation) {
			pixels->Erase(mBGColor, false);
		} else //if (mImageIndex == 0)
			pixels->Erase(mBackGroundColor);		// If transparent, it will erase to transparency

	mImageIndex++;
	
	if (mImageIndex > 1)
		mIsAnimation = true;
	
	mPass = 0;
	mYPos = mMaxYPos = mLocalTop;
	mXPos = mLocalLeft;
	
//	Start the LZW decode

	short codeSize = *src++;
	mBits = 0;
	mBufferCount = 0;

//	pprint("InitBuffer: %d bytes (%d)",src[0],dataLength);
	
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
//		return 0;
		return -1;
	
	if (dataLength < 1)
		return -1;

//	Add data to buffer if it is nearly empty

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

//	Read and draw the image body

long GIF::ReadImageBody(uchar *data, long dataLength, Pixels *pixels)
{
	short	p = 0;
	uchar	*src = data;
	
	if (pixels == 0)						// No Pixels yet...
		return 0;
		
	short maxY = mLocalTop + mLocalHeight;
	short maxX = mLocalLeft + mLocalWidth;
	
	while (mYPos < maxY) {
//		Write as many pixels as we can with the data in the buffer

		uchar *dst = pixels->GetLineAddr(mYPos);
		
		uchar *dstptr = &dst[mXPos];
		uchar *dstend = &dst[maxX];
//#define LZWPixel() ((mSP > mStack) ? *--mSP : DecodeLZW())
//		for (;mXPos < maxX && (mBufferCount >= 4); mXPos++) {  
//			if ((p = LZWPixel()) < 0) goto bail;
//			dst[mXPos] = p;		// Transparency will clobber previous frame, don't care in line buffer
//		}
		while (dstptr < dstend && (mBufferCount >= 4)) {  
			if ((p = LZWPixel()) < 0) goto bail;
			*dstptr++ = p;		// Transparency will clobber previous frame, don't care in line buffer
		}
		mXPos = dstptr - dst;
		if (mXPos == maxX) {
			pixels->ClipLine(mLocalLeft,maxX);
			pixels->EndLine(mYPos);
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
							pixels->EndLine(topLine, extraY);
				}

				if (mYPos >= maxY && mPass < 4)
					goto skippedPass;
				
			} else
				mYPos++;					// Draw Progressive
			if (mYPos > mMaxYPos)
				mMaxYPos = mYPos;
		}

//		If Buffer is nearly empty .. put more data into it

		int bytesRead = FillBuffer(src,dataLength);
		if (bytesRead) {
			if (bytesRead < 0)
				goto done;
			src += bytesRead;
			if (dataLength < bytesRead) {
//				pprint("Read Past end of GIF");
				p = -1;
				goto bail;
			}
			dataLength -= bytesRead;
		}
	}

bail:
	if (mYPos >= maxY || p < 0) {
	
//		Image is complete, an extra blob of data with zero size is sometimes present
	
		if (dataLength >= 1) {
			if (src[0] == 0) {
//				pprint("GIF Trailing Null");
				src++;
				--dataLength;
			}
		}
		
		mMaxYPos = mHeight;
//		if (LZWPixel() == -2)
//			pprint("Found end of Local Image");
//		else
//			pprint("Terminated at line %d",mYPos);
		if (gPreferences.FindBool("ShowAnimations"))
			mPhase = kLocalImageEnd;
		else
			mPhase = kGlobalImageEnd;
		pixels->Finalize();
	}
	
done:
	return src - data;
}

short GIF::GetMaxYPos()
{
	return mMaxYPos;
}

