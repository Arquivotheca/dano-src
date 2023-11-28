// ===========================================================================
//	BeMovieWindow.h
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __BEMOVIEWINDOW__
#define __BEMOVIEWINDOW__

#include <StorageKit.h>

#include "MoviePlay.h"

// ============================================================================
//	A Quicktime player for the BeBox ....

class BeQTPlayer : public QTPlayer {
public:
						BeQTPlayer(Store *store);
	virtual				~BeQTPlayer();
	
	virtual	Boolean		SetVideoBufferSize(long width,long height,long depth);
	virtual	Boolean		GetVideoBuffer(Byte **dst, long *dstRowBytes, long *depth);
	virtual	Boolean		DrawVideo();
	
	BBitmap*			GetBitmap(BRect& r);

protected:
		BBitmap*		mBBitmap;
		long			mDepth;
		long			mWidth;
		long			mHeight;
		
		Store*			mQTStore;
};

QTPlayer* NewBeQTPlayer(entry_ref *fileref);


#endif

