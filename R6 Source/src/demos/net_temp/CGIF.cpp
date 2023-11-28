// ===========================================================================
//	CGIF.cp
// 	й1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include <Be.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "CGIF.H"

//===========================================================================

#define	pprint	printf
#define	pprintHex
#define	NP_ASSERT

enum {
	kGlobalImageHeader = 0,
	kReadExtension,					//1
	kLocalImageHeader,				//2
	kLocalImageBodySetup,			//3
	kLocalImageBody,				//4
	kLocalImageEnd,					//5
	kGlobalImageEnd,				//6
	kGIFError						//7
};

CGIF::CGIF()
{
	pixels = 0;
	mWidth = mHeight = 0;
	mLocalTop = mLocalLeft = mLocalWidth = mLocalHeight = 0;
	mImageIndex = 0;
	mLooping = false;
	mNetscapeAnimation = false;
	
	mInterlaced = false;
	mBackGroundColor = 0;
	mFormat = 0;
	mPhase = 0;
	mTransparency = 0;
	mTransparencyIndex = 0;

//	LZW Decode

	mPrefix = mSuffix = mStack = 0;
	mEndOfLZW = 0;
	
//	Drawing

	mGlobalColorTable = mLocalColorTable = 0;
	mMaxYPos = 0;
}

CGIF::~CGIF()
{
	if (mPrefix) free(mPrefix);
	if (mSuffix) free(mSuffix);
	if (mStack)	 free(mStack);

	if (mLocalColorTable) free(mLocalColorTable);
	if (mGlobalColorTable) free(mGlobalColorTable);
}

char CGIF::GetRect(BRect *r)
{
	r->Set(0,0,mWidth,mHeight);
	return (mWidth && mHeight);
}

//	Write a byte int the GIF decoder, see what comes out

long	CGIF::Write(Byte  *data, long dataLength)
{
	switch (mPhase) {
		case kGlobalImageHeader:	return ReadGlobalHeader(data,dataLength);
		case kReadExtension:		return ReadExtension(data,dataLength);
		case kLocalImageHeader:		return ReadImageHeader(data,dataLength);
		case kLocalImageBodySetup:	return SetupImageBody(data,dataLength);
		case kLocalImageBody:		return ReadImageBody(data,dataLength);
		
		case kLocalImageEnd:
			mPhase = kReadExtension;
			return 0;				// Return control at end of local image
			
		case kGlobalImageEnd:
			if (mNetscapeAnimation)
				mNetscapeIterations--;
			mPhase = 0;
			if (dataLength != 0)
				pprint("CGIF::Write: %d extra bytes at end of GIF",dataLength);
			return dataLength;				
			
		default:
			pprint("CGIF::Write bad phase? %d",mPhase);
			return -1;
	}
	return 0;
}

//	Read the header info in from the GIF file

long CGIF::ReadGlobalHeader(Byte *data, long dataLength)
{
	if (dataLength < 13) return 0;	// Didn't read the header

//	Reset at start of image

	mWidth = mHeight = 0;
	mLocalTop = mLocalLeft = mLocalWidth = mLocalHeight = 0;
	mImageIndex = 0;
	mInterlaced = false;
	mBackGroundColor = 0;
	mTransparency = false;
	mTransparencyIndex = 0;
	mMaxYPos = 0;

	mFormat = 0;
	if (!strncmp((char *)data,"GIF87a",6)) {
		//pprint("GIF is 87a");
		mFormat = 87;
	}
	if (!strncmp((char *)data,"GIF89a",6)) {
		//pprint("GIF is 89a");
		mFormat = 89;
	}
	if (mFormat == 0) {
		pprint("Bad GIF header: %d bytes of data",dataLength);
		if (*(unsigned short *)data == 0x00FFD8)
			pprint("(Looks like a JPEG file)");
		else
			;//pprintHex(data,MIN(64,dataLength));
		return -1;
	}

	mWidth = data[6] + (data[7] << 8);
	mHeight = data[8] + (data[9] << 8);
	short packed = data[10];
	mBackGroundColor = data[11];
	//pprint("Image is %d by %d, (0x%2X)", mWidth,mHeight,mBackGroundColor);

//	Read global color table

	short colorTableSize = 3 << ((packed & 0x7) + 1);
	short colorResoultion = ((packed >> 4) & 0x7) + 1;
	if (packed & 0x80) {
		if (dataLength < (13 + colorTableSize))		// Read header, not enough for color table
			return 0;
			
		if (mGlobalColorTable)
			free(mGlobalColorTable);
		mGlobalColorTable = (Byte *)malloc(256*3);
		NP_ASSERT(mGlobalColorTable);
		memcpy(mGlobalColorTable,data + 13,colorTableSize);
		//pprint("Got Global Color table: Size %d, %dbpp",colorTableSize/3,colorResoultion);

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

long CGIF::ReadComment(Byte *data, long dataLength)
{
#if 1

	short 	count;
	Byte *src = data + 2;
	dataLength -= 2;
	while (count = *src++) {
		if (dataLength <= (count + 1))	// Not enough data here to read extension
			return 0;
		src += count;
	}

	return src - data;

#else
	Handle	comment = NewHandle(0);
	short 	c,count;

	Byte *src = data + 2;
	dataLength -= 2;
	while (count = *src++) {
		if (dataLength <= (count + 1)) {	// Not enough data here to read extension
			DisposeHandle(comment);
			return 0;
		}
		PtrAndHand(src,comment,count);
		src += count;
	}

//	Could do something useful with the comment ....

	c = 0;
	PtrAndHand(&c,comment,1);	// Add the nul
	//pprint("Comment is %d bytes",GetHandleSize(comment));
	pprintMono(1);
	HLock(comment);
	char *s = (char *)*comment;
	while (*s) {
		char	str[1024];
		short	i = 0;
		while (c = (str[i++] = *s++))
			if (c == '\n' || c == '\r') break;
		if (!c) break;
		str[i-1] = 0;
		if (i > 1)
			pprint(">%s",str);
	}
	pprintMono(0);
	DisposeHandle(comment);

	return src - data;

#endif
}

//	Read Graphic control extension

long CGIF::ReadGraphicControl(Byte *data, long dataLength)
{
	if (dataLength < 8) return 0;	// Total size of Graphic extension
	mTransparency = (data[3] & 1);
	
#if 0
	int disposal = (((data[3]>>2)&7)+1);
	switch (disposal) {
		case 2:	pprint("GIF: Do not dispose");	break;
		case 3:	pprint("GIF: Restore to background");	break;
		case 4:	pprint("GIF: Restore to previous");	break;
		default:
			pprint("Disposal: %d",disposal);
	}
#endif
	
	mTransparencyIndex = data[6];

//	Copybits does transparency by setting the background color to the transparent color
//	If the background color appears in the rest of the image, those pixels will be transparent too
//	Set to background color to an unlikely shade, should explicty check clut...
	
//	Byte *color = mGlobalColorTable + mTransparencyIndex*3;
//	color[0] = 0x12;
//	color[1] = 0x34;
// 	color[2] = 0x56;	// An unlikely color
	
	//if (mTransparency)
	//	pprint("GIF is Transparent: Index %d",mTransparencyIndex);
	return 8;
}


//	Read an application extension

long CGIF::ReadApplication(Byte *data, long dataLength)
{
	Byte *src = data + 2;
	short count;
	
	dataLength -= 2;
	while (count = *src++) {
		if (dataLength <= (count + 1))	// Not enough data here to read extension
			return 0;

		if (count == 11 && strstr((char *)src,"NETSCAPE"))
			mNetscapeAnimation = true;
		else if (count == 3 && mNetscapeAnimation && src[0] == 1) {	// Iteration count
			if (mLooping == false) {
				mNetscapeIterations = src[1] | ((long)src[2]) << 8;
				//pprintBig("Netscape Animation has %d iterations",mNetscapeIterations);
				if (mNetscapeIterations == 0)
					mNetscapeIterations = 100;	// No interations mean loop forever like everyone else ееееееееее
			}
		}

		src += count;
	}
	return src - data;
}


//	Read the next extension

long CGIF::ReadExtension(Byte *data, long dataLength)
{
	short	count;

	Byte	*src;
	switch (*data) {
		case 0x3B:
			//pprint("Found Global Image End");
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
				default:	pprint("Bad extension: %d",data[1]);
			}
			src = data + 2;
			dataLength -= 2;
			while (count = *src++) {
				if (dataLength <= (count + 1))	// Not enough data here to read extension
					return 0;
				src += count;
			}
			return src - data;
			break;
		default:
			pprint("Error in GIF file: Bad Extension");
			return -1;
	}
}

//	Read local image

long CGIF::ReadImageHeader(Byte *data, long dataLength)
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

	if (mWidth == 0)
		mWidth = mLocalWidth;
	if (mHeight == 0)
		mHeight = mLocalHeight;
	
	//pprint("Local image at (%d,%d) is %d by %d",mLocalLeft,mLocalTop,mLocalWidth,mLocalHeight);

	if (mLocalColorTable)
		free(mLocalColorTable);
	mLocalColorTable = 0;
	
	short colorTableSize = 3 << ((packed & 0x07) + 1);
	if (packed & 0x80) {
		if (dataLength < (10 + colorTableSize))		// Read header, not enough for color table
			return 0;
		//pprint("Local color table present: %d",colorTableSize/3);
		mLocalColorTable = (Byte *)malloc(256*3);
		NP_ASSERT(mLocalColorTable);
		memcpy(mLocalColorTable,data+10,colorTableSize);
	} else
		colorTableSize = 0;

	mPhase = kLocalImageBodySetup;
	return 10 + colorTableSize;
}

//===========================================================================

#define xGetCode(_x) ((_x < mBits) ? (mLast32 >> (32 - _x - (mBits -= _x))) & ((1 << _x) - 1) : GetBitsRead(_x))

long CGIF::GetBitsRead(short count)
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

short	CGIF::InitLZW(short initCodeSize)
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
	memset(mPrefix,0,4096*2);
	memset(mSuffix,0,4096*2);
	mStack = (short *)malloc((4096 + 1)*2);
	
	if (!mPrefix || !mSuffix || !mStack)
		return -1;
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

short CGIF::DecodeLZW()
{
	short	i,code,thisCode;

//	if (mSP > mStack)		// Pixels on the stack, very common, put in macro
//		return *--mSP;

	if ((code = xGetCode(mCodeSize)) < 0)
		return code;

	if (code == mClearCode) {				// Clear code resets tables
		for (i = 0; i < mClearCode; i++) {
			mPrefix[i] = 0;
    		mSuffix[i] = i;
  		}
		for (; i < 4096; i++)
			mPrefix[i] = mSuffix[i] = 0;

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
		if (code == mPrefix[code]) {
			pprint("Circular table entry");
			return -1;
		}
		code = mPrefix[code];
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

long CGIF::SetupImageBody(Byte *data, long dataLength)
{
	Byte	*src = data;

	if (dataLength < (1 + 1 + src[1]))		// codeSize, blockCount, count
		return 0;
	pixels = new BBitmap(BRect(0, 0, mWidth,mHeight),  B_COLOR_8_BIT);
	if (pixels == 0)						// No Pixels yet...
		return 0;

//	Setup destination pixels

	//if (mTransparency)
	//	pixels->SetTransparencyIndex(mTransparencyIndex);
		
	if (mImageIndex == 0)
		if (mLooping == false) {
			mLooping = true;
		}
	
	// BGS
	//if (mLocalColorTable)
	//	pixels->SetColorTable((Byte *)mLocalColorTable);
	//else
	//	pixels->SetColorTable((Byte *)mGlobalColorTable);
	
	//if (mImageIndex == 0)
	//	pixels->Erase(mBackGroundColor);		// If transparent, it will erase to transparency
	mImageIndex++;
	
	mPass = 0;
	mYPos = mMaxYPos = mLocalTop;
	mXPos = mLocalLeft;
	
//	Start the LZW decode

	short codeSize = *src++;
	mBits = 0;
	mBufferCount = 0;

	//pprint("InitBuffer: %d bytes (%d)",src[0],dataLength);
	
	mBufferCount = *src++;			// Write the first chunk of data into the decoder
	memcpy(mBuffer,src,mBufferCount);
	src += mBufferCount;
	mData = mBuffer;
	
	if (InitLZW(codeSize))
		return -1;

	mPhase = kLocalImageBody;		// Start reading the body
	return src - data;
}

int CGIF::FillBuffer(Byte *src, int dataLength)
{
	int added = 0;
	
	if (mEndOfLZW)
		return 0;
	
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

ulong	mapper(ulong c)
{
	return c;
	
	ulong	sum;
	
	sum = (c>>24) + ((c>>16)&0xff) + ((c>>8)&0xff);
	sum /= 4;
	
	if (sum > 55 && sum < 150)
		sum += 150;
		
	
	if (sum > 255)
		sum = 255;
	
	c = (sum<<24) | (sum<<16) | (sum<<8) | (sum);
	return c;
	
}


long CGIF::ReadImageBody(Byte *data, long dataLength)
{
	short	p,imageEnd = 0;
	Byte 	*src = data;
	ulong	tmp;
	long	tmp1;
	
	if (pixels == 0)						// No Pixels yet...
		return 0;
		
	short maxY = mLocalTop + mLocalHeight;
	short maxX = mLocalLeft + mLocalWidth;
	
	while (mYPos < maxY) {
//		Write as many pixels as we can with the data in the buffer


		uchar *dst = (uchar *)(((Byte *)pixels->Bits()) + pixels->BytesPerRow()*(mYPos));
		
		
		for (;mXPos < maxX && (mBufferCount >= 4); mXPos++) {  
			if ((p = LZWPixel()) < 0) goto bail;
			
			tmp = (mGlobalColorTable[p*3+0] << 16)  |
				  (mGlobalColorTable[p*3+1] << 8)  |
				  (mGlobalColorTable[p*3+2] << 0);
		
			tmp1 = (tmp>>16) & 0xff;
			tmp1 += (tmp>>8) & 0xff;
			tmp1 += (tmp) & 0xff;

			tmp1 *= 4;
			tmp1 /= 15;
			tmp1 -= 12;
			if (tmp1 < 0) tmp1 = 0;
			if (tmp1 > (4*31)) tmp1 = (4*31);
				  
			dst[mXPos] =tmp1;		// Transparency will clobber previous frame, don't care in line buffer
		}
		if (mXPos == maxX) {
			//pixels->ClipLine(mLocalLeft,maxX);
			//pixels->EndLine(mYPos);
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
				//pprint("Read Past end of GIF");
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
				pprint("GIF Trailing Null");
				src++;
				--dataLength;
			}
		}
		
		mMaxYPos = mHeight;
		//if (LZWPixel() == -2)
		//	pprint("Found end of Local Image");
		//else
		//	pprint("Terminated at line %d",mYPos);
		mPhase = kLocalImageEnd;
		//pixels->Finalize();
	}
	
done:
	return src - data;
}

short CGIF::GetMaxYPos()
{
	return mMaxYPos;
}

// =======================================================================================
// =======================================================================================
// Mask generation as an exercise
// =======================================================================================
// =======================================================================================
