// ===========================================================================
//	Image.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __IMAGE__
#define __IMAGE__

#include "UResource.h"

#include <Locker.h>
class BBitmap;

class UpdateRegion;
class DrawPort;
class Pixels;
class CGIF;


// ===========================================================================
//	Image handle manages downloading etc

class Image;
class ImageGlyph;
class ImageConsumer;

class ImageHandle : public Counted {
public:
					ImageHandle(ImageGlyph* glyph,const BString& url, long docRef, bool forceCache, ConnectionManager *mgr, BMessenger *listener, bool firstFrameOnly = false);
					
			bool		IsDead();	
			bool		IsTransparent();
			bool		Complete();
			bool		ResourceComplete();
			void		Abort();
			
			
			void	Invalidate(UpdateRegion& updateRegion);
			Image*	GetImage();
			void	SetImage(Image *image);
			
			bool	GetRect(BRect* r);
			void	Draw(DrawPort *drawPort, BRect* r);
			void	SetBackgroundColor(long bgColor);
			void	SetBackgroundImage(ImageHandle *bgImage, float bgHOffset, float bgVOffset);
			void	Idle();
			void	SwitchImp(UResourceImp *imp) {mResource = imp;}

protected:
			virtual	~ImageHandle();

		ImageGlyph* mGlyph;
		UResourceImp*	mResource;
		Image*		mImage;
		bool		mHasBGColor;
		long		mBGColor;
		Pixels*		mBGPixels;
		float			mBGHOffset;
		float			mBGVOffset;
		BMessenger*	mListener;
		bool		mError;
		BString		mURL;
		bool		mFirstFrameOnly;
};

//===========================================================================
//	An image glyph is a placeholder on a page used to display an image handle
//	An image handle has a resource and uses it to draw to an image
//	An image contain the pixel data

//	Each image glyph has one image handle
//	Each image has many image handles

class ImageGlyph;

class Image {
protected:
						Image(bool firstFrameOnly = false);	// Use GetImage instead
public:
	virtual				~Image();	// Use DisposeImage instead

	static	void		Open();
	static	void		Close();
	
			bool		Lock();		// Because images can be shared among threads,
			void		Unlock();	// Locks are required. All locking is done from ImageHandle
			
	static	Image*		GetImageFromCache(const BString& url, bool& isCached, bool& imageError);
	static	Image*		GetImageFromResource(UResourceImp* resource, bool firstFrameOnly = false);
	static	void		ReferenceCachedImage(const BString& url, bool reference);
	static  void		CachedImageError(const BString& url);
	
	static	void		DisposeImage(ImageHandle *imageHandle);
	
			void		AddImageHandle(ImageHandle* imageHandle);
			
	virtual	bool		GetRect(BRect *r) = 0;	// Returns true if it is sure about its size
			void		Invalidate(UpdateRegion &updateRegion);
	
	virtual	void		Draw(DrawPort *drawPort, BRect *dstRect) = 0;
	virtual	bool		IsDead();
	virtual bool		IsTransparent();
	virtual bool		NeedsBackgroundDrawn() = 0;
			
	virtual Pixels *	GetPixels() = 0;
	
	virtual void		SetBGInfo(bool hasBGColor, long bgColor, bool hasBGImage, Pixels *bgImage, float bgHOffset, float bgVOffset);
	virtual	long		Write(uchar *d, long count) = 0;
			void		Reference();
			void		Dereference();
			void		SetURL(const char *url) {mURL = url;}
			const char*	GetURL() {return mURL.String();}
			bool		Complete() {return mComplete;}
	virtual bool		SetComplete();
			void		SetError() {mError = true;}
			bool		GetError() {return mError;}
	virtual void		SetConsumer(ImageConsumer *consumer);

protected:
			void		DeleteImageHandle(ImageHandle* imageHandle);
			int			CountHandles();
	
	static	BList*	mImageCache;	// A list of images currently in use
	static TLocker	mCacheLocker;
			
			bool						mDrawAsThumbnail;
			BList						mHandles;	// List of image handles using this image
			int							mMaxDone;	// Maximum size of image
			int32						mRefCount;
			BString						mURL;
			bool						mComplete;
			bool						mError;
			bool						mFirstFrameOnly;
			TLocker		mLocker;
};

Image *NewImage(UResourceImp *resource);

//===========================================================================
//	GIF image

class	GIFImage : public Image {
public:
						GIFImage(bool firstFrameOnly = false);

	virtual	void		Reset();
	virtual	long		Write(uchar *d, long count);

	virtual	void		Draw(DrawPort *drawPort, BRect *dstRect);
	virtual	bool		GetRect(BRect *r);
	virtual bool		IsTransparent();
	virtual Pixels *	GetPixels() {return mPixels;}
	virtual bool		NeedsBackgroundDrawn();
	virtual void		SetBGInfo(bool hasBGColor, long bgColor, bool hasBGImage, Pixels *bgImage, float bgHOffset, float bgVOffset);
	
	virtual void		SetConsumer(ImageConsumer *consumer);
	virtual bool		SetComplete();

protected:
	virtual				~GIFImage();
			Pixels*		mPixels;
			CGIF*		mGIF;
			ImageConsumer*mConsumer;
};

//===========================================================================
//	JPEG Image

#include "JPEG.h"

class	JPEGImage : public Image {
public:
						JPEGImage();

	virtual	long		Write(uchar *d, long count);

			JPEGError	DrawRow(JPEGDecoder *j);

	virtual	void		Draw(DrawPort *drawPort, BRect *dstRect);
	
	virtual	bool		GetRect(BRect *r);
	virtual Pixels *	GetPixels() {return mPixels;}
	virtual bool		NeedsBackgroundDrawn() {return false;}

protected:
	virtual				~JPEGImage();
			JPEGDecoder* mJPEG;
			BRect		mDrawRect;
			DrawPort*	mDrawPort;
			Pixels*		mPixels;
			bool		mLookingForRect;
			bool		mWaitingForScan;
			int			mMaxHeight;
};

//===========================================================================
//	Translation Kit image

class	TranslationKitImage : public Image {
public:
						TranslationKitImage();

	virtual	long		Write(uchar *d, long count);
	virtual bool		SetComplete();
	virtual	void		Draw(DrawPort *drawPort, BRect *dstRect);
	virtual	bool		GetRect(BRect *r);
	virtual Pixels *	GetPixels() {return mPixels;}
	virtual bool		NeedsBackgroundDrawn() {return false;}

protected:
	virtual				~TranslationKitImage();
	Pixels*				mPixels;
	BBitmap*			mBitmap;
	BMallocIO*			mStream;
};
#endif
