// ===========================================================================
//	CGIF.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __GIF__
#define __GIF__

#include "DrawPort.h"
#include <String.h>
//===========================================================================

class CGIF /*: public NPObject*/ {
public:
				CGIF(bool firstFrameOnly = false);
		virtual	~CGIF();

		virtual	long	Write(uchar *data, long dataLength, Pixels *pixels);
		virtual	bool	GetRect(BRect *r);
		virtual	short	GetMaxYPos();
				bool	LoopAnimation();
				bool	IsAnimating();
				void	SetBG(bool hasBGColor, long bgColor, bool hasBGImage, Pixels *bgImage, float bgHOffset, float bgVOffset);
		
protected:
				long	ReadGlobalHeader(uchar *data, long dataLength);
				long	ReadExtension(uchar *data, long dataLength);
				long	ReadComment(uchar *data, long dataLength);
				long	ReadGraphicControl(uchar *data, long dataLength);
				long	ReadApplication(uchar *data, long dataLength);

				long	ReadImageHeader(uchar *data, long dataLength);
				long	SetupImageBody(uchar *data, long dataLength, Pixels *pixels);
				long	ReadImageBody(uchar *data, long dataLength, Pixels *pixels);

				long	GetBitsRead(short count);
				short	InitLZW(short initCodeSize);
				short	DecodeLZW();
				int		FillBuffer(uchar *src,int dataLength);

//		Dimensions of GIF flie

		short	mFormat;
		short	mPhase;
		bool	mLooping;		// 
		short	mImageIndex;
		
		short	mWidth;			// Global width and height
		short 	mHeight;
		
		short	mLocalTop;			// Local top,left,width and height
		short	mLocalLeft;
		short	mLocalWidth;
		short 	mLocalHeight;
		bool	mInterlaced;
		unsigned long	mDelay;		// Delay until next image is displayed in 1/100ths of a second
		unsigned long	mOldDelay;

		
		bool	mTransparency;
		short	mTransparencyIndex;
		short	mDisposal;

//		Buffers for LZW vlc decode

		uchar	mBuffer[256 + 4];
		short	mBufferCount;
		uchar	*mData;
		unsigned long	mLast32;
		short	mBits;

//		Netscape's extension for animation

		bool	mIsAnimation;
		bool	mNetscapeAnimation;
		long	mNetscapeIterations;
		
//		LZW Decode

		short	mSetCodeSize;
		short	mCodeSize;
		short	mClearCode;
		short	mEndCode;
		short	mMaxCodeSize;
		short	mMaxCode;
		short	mFirstCode;
		short	mOldCode;

		short	*mPrefix;
		short	*mSuffix;
		short	*mStack,*mSP;
		bool	mEndOfLZW;
		
//		Drawing

		short	mYPos;
		short	mXPos;
		short	mMaxYPos;
		short	mPass;

		short	mBackGroundColor;
		uchar	*mGlobalColorTable;
		uchar	*mLocalColorTable;
		
		bool	mHasBGColor;
		bool	mHasBGImage;
		long	mBGColor;
		Pixels*	mBGImage;
		float	mBGHOffset;
		float	mBGVOffset;
		bool	mFirstFrameOnly;
		
		enum {kNotSpecified = 1, kDoNotDispose, kRestoreToBackground, kRestoreToPrevious};
};

#endif
