// ===========================================================================
//	GIF.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __GIF_H__
#define __GIF_H__

#include <OS.h>

class BBitmap;
class BRect;
const int kDictionarySize = 4096;

class GIF {
public:
	GIF();
	virtual	~GIF();
	virtual	long Write(uchar *data, long dataLength, BBitmap *bmp);
	virtual	bool GetRect(BRect *r);
	virtual	short GetMaxYPos();
	bool LoopAnimation();
	bool IsAnimating();
	bool IsTransparent();
	unsigned long	GetDelay() { return mDelay; }
	long ReadGlobalHeader(uchar *data, long dataLength);
	long ReadExtension(uchar *data, long dataLength);
	long ReadComment(uchar *data, long dataLength);
	long ReadGraphicControl(uchar *data, long dataLength);
	long ReadApplication(uchar *data, long dataLength);
	
	long ReadImageHeader(uchar *data, long dataLength);
	long SetupImageBody(uchar *data, long dataLength, BBitmap *bmp);
	long ReadImageBody(uchar *data, long dataLength, BBitmap *bmp);
	
	long GetBitsRead(short count);
	int FillBuffer(uchar *src,int dataLength);
	int GetImageIndex(){ return mImageIndex; }
	
	size_t GetMemoryUsage() const;
	
	int32 CurrentFrame() const;
	int32 CountFrames() const;
	
	// Move back into initial GIF parsing state, without
	// losing information about current animation count etc.
	void Restart();
	
	// Completely reset decoder to initial parsing state.
	void Reset();
				
protected:
	short InitLZW(short initCodeSize);
	short DecodeLZW();

	// Dimensions of GIF file
	short mFormat;
	short mPhase;
	bool mLooping;
	short mImageIndex;
	short mMaxIndex;
	
	short mWidth;			// Global width and height
	short mHeight;
	
	short mLocalTop;			// Local top,left,width and height
	short mLocalLeft;
	short mLocalWidth;
	short mLocalHeight;
	bool mInterlaced;
	unsigned long mDelay;		// Delay until next image is displayed in 1/100ths of a second
	unsigned long mOldDelay;
	
	bool mTransparency;
	bool mGlobalTransparency;
	short mTransparencyIndex;
	short mDisposal;
	char* mLastData;
	size_t mLastSize;
	
	//	Information we need about the previous frame
	short mLastTop;
	short mLastLeft;
	short mLastWidth;
	short mLastHeight;
	short mLastDisposal;
	
	// Buffers for LZW vlc decode
	uchar mBuffer[256 + 4];
	short mBufferCount;
	uchar *mData;
	unsigned long mLast32;
	short mBits;

	// Netscape's extension for animation
	bool mIsAnimation;
	bool mNetscapeAnimation;
	long mNetscapeIterations;
	
	// LZW Decode
	short mSetCodeSize;
	short mCodeSize;
	short mClearCode;
	short mEndCode;
	short mMaxCodeSize;
	short mMaxCode;
	short mFirstCode;
	short mOldCode;

	struct dictionary_entry {
		short mPrefix;
		uchar mSuffix;
	} mDictionary[kDictionarySize];
	short mStack[kDictionarySize + 1];
	short *mSP;
	bool mEndOfLZW;
	
	// Drawing
	short mYPos;
	short mXPos;
	short mMaxYPos;
	short mPass;

	short mBackGroundColor;
	uchar mGlobalColorTable[256 * 3];
	uchar *mLocalColorTable;
};

#endif
