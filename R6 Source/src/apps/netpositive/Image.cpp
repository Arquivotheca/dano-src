// ===========================================================================
//	Image.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Image.h"
#include "ImageGlyph.h"
#include "CGIF.h"
#include "BeDrawPort.h"
#include "Cache.h"
#include "NPApp.h"
#include "MessageWindow.h"

#include <Screen.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <stdio.h>
#include <BitmapStream.h>

class ImageConsumer : public Consumer {
public:
	ImageConsumer(ImageHandle *imageHandle);
	virtual long Write(uchar *data, long count);
	virtual void SetComplete(bool complete);
	virtual void SetError(bool error);
	virtual void MessageReceived(BMessage *msg);
	
protected:
	~ImageConsumer();
	ImageHandle *mImageHandle;
	Image *mImage;
};


ImageConsumer::ImageConsumer(ImageHandle *imageHandle) : 
	mImageHandle(imageHandle), mImage(NULL)
{
	mImageHandle->Reference();
}


ImageConsumer::~ImageConsumer()
{
	if (mImageHandle) {
		mImageHandle->Dereference();
	}
	if (mImage)
		mImage->Dereference();
}

void ImageConsumer::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case msg_ResourceSwitched:
			msg->FindPointer("NewImp", (void **)&mResourceImp);
			if (mImageHandle)
				mImageHandle->SwitchImp(mResourceImp);
			break;
		default:
			Consumer::MessageReceived(msg);
			break;
	}
}

long ImageConsumer::Write(uchar *data, long count)
{
	if (mImage == 0) {
//		if (mImageHandle->GetImage() == NULL)
//			mImageHandle->Idle();
		if ((mImage = mImageHandle->GetImage()) == NULL) {
			if (mImageHandle->IsDead()) {
				SetError(true);
				return -1;
			}
			return 0;
		}
		mImage->Reference();
		mImageHandle->Dereference();
		mImageHandle = NULL;
		mImage->SetConsumer(this);
	}
	return mImage->Write(data, count);
}


void ImageConsumer::SetComplete(bool complete)
{
	Consumer::SetComplete(complete);
	
	if (complete) {
		bool shouldKill = false;
		if (mImage)
			shouldKill = mImage->SetComplete();
		if (shouldKill)
			Kill();
	}
}


void ImageConsumer::SetError(bool error)
{
	if (error && mImage)
		mImage->SetError();
	Consumer::SetError(error);
}


class ImageEntry : public Counted {
public:
	ImageEntry(Image *image, const char *url) : mImage(image), mURL(url), mError(false) {}
	
	Image *mImage;
	BString mURL;
	bool	mError;
protected:
	~ImageEntry() {}
};


//=========================================================================
//	All images are stored in a central cache

ImageHandle::ImageHandle(ImageGlyph* glyph, const BString& url, long docRef, bool forceCache, ConnectionManager *mgr, BMessenger *listener, bool firstFrameOnly)
{
	mGlyph = glyph;
	mResource = NULL;
	bool isCached;
	mImage = Image::GetImageFromCache(url, isCached, mError);			// Try the cache
	mHasBGColor = false;
	mBGPixels = NULL;
	mListener = listener;
	mError = false;
	mURL = url;
	mFirstFrameOnly = firstFrameOnly;
	// If it's cached but not enough of the image is loaded enough to create an Image,
	// then return now.  We'll pick up the image later on idle.
	if (isCached && mImage == NULL) {
		Image::ReferenceCachedImage(url, true);
		return;
	}
	
	if (mImage == NULL) {
		if (!isCached) {
			BString errMsg;
			mResource = GetUResource(mgr, url,docRef, errMsg, forceCache, NULL, NULL);	// Create a new resource
			if (!mResource) {
				Image::CachedImageError(url);
				mError = true;
				return;
			}
	
			ImageConsumer *consumer = new ImageConsumer(this);
			consumer->SetResourceImp(mResource);
			
			BMessage msg(msg_ResourceChanged);
			msg.AddPointer("ResourceImp", mResource);
			consumer->PostMessage(&msg);
			
			BMessage msg2(msg_AddConsumer);
			msg2.AddPointer("Consumer", consumer);
			listener->SendMessage(&msg2);
			
			msg2.what = msg_ConsumerUpdate;
			listener->SendMessage(&msg2);
		}
	} else {
		if (!mImage->Lock()) return;
		mImage->AddImageHandle(this);
		mImage->Unlock();
	}
}

ImageHandle::~ImageHandle()
{
	//delete mResource;
	if (mResource)
		mResource->RefCount(-1);
	if (mImage)
		Image::DisposeImage(this);	// Remove this ImageHandle from image
	else
		Image::ReferenceCachedImage(mURL, false);
}


void ImageHandle::SetImage(Image *image)
{
	mImage = image;
}

void ImageHandle::SetBackgroundColor(long color)
{
	mHasBGColor = true;
	mBGColor = color;
}


void ImageHandle::SetBackgroundImage(ImageHandle *image, float bgHOffset, float bgVOffset)
{
	if (image && image->mImage)
		mBGPixels = image->mImage->GetPixels();
	mBGHOffset = bgHOffset;
	mBGVOffset = bgVOffset;
}


//	Stop loading this image

void ImageHandle::Abort()
{
	if (mImage && mImage->Complete())
		return;
		
	if (mImage)
		Image::DisposeImage(this);	// Remove this ImageHandle from image
	mImage = NULL;
	mError = true;
}


//	Continue to Consume the image in progress -or-
//	Create a new image based on the requested resource

void ImageHandle::Idle()
{
	if (mImage == NULL) {
		// See if the image has shown up in the cache yet.
		bool isCached;
		mImage = Image::GetImageFromCache(mURL, isCached, mError);
		
		if (!mResource && mImage == NULL)
			return;

		if (!mImage)
			mImage = Image::GetImageFromResource(mResource, mFirstFrameOnly);	// Create a new image from resource
		if (mImage) {
			if (!mImage->Lock()) return;
			mImage->AddImageHandle(this);
			mImage->Unlock();
			
			UpdateRegion updateRegion;
			Pixels *p = mImage->GetPixels();
			if (p) {
				updateRegion.Invalidate(BRect(0,0,p->GetWidth(),p->GetHeight()));
				Invalidate(updateRegion);
			}
		} else {
			StoreStatus status = mResource->GetStatus();
			if (status == kTimeout || status == kAbort || status == kError) {
				//delete mResource;
				mResource->RefCount(-1);
				mResource = NULL;
				mError = true;
			}

		}
	}
	
	if (mImage != NULL) {
		// If an error occurred, give up on this image
		
		if (mImage->IsDead()) {
			Image::DisposeImage(this);	// Remove this ImageHandle from image
			mImage = NULL;
			mError = true;
		}
	}
}

//	If resource died and image wasn't in cache, its dead

bool ImageHandle::IsDead()
{
	return mError || (mImage && mImage->GetError());
}


bool ImageHandle::IsTransparent()
{
	if (mImage)
		return mImage->IsTransparent();
	else
		return false;
}


bool ImageHandle::Complete()
{
	if (mImage)
		return mImage->Complete();
	return false;
}

bool ImageHandle::ResourceComplete()
{
	if (!mResource)
		return true;

	long currentSize,expectedSize;
	mResource->GetProgress(&currentSize,&expectedSize);
	return (expectedSize && currentSize == expectedSize);
}


void ImageHandle::Invalidate(UpdateRegion& updateRegion)
{
	mGlyph->Invalidate(updateRegion);

	// If the image didn't previously know its size and lied about
	// it to get the layout to continue, then call KnowSize now so
	// it can tell the document that it needs relayout and so the
	// relayout will happen before the redraw that we're calling for
	// here.
	mGlyph->KnowSize();
}


Image*	ImageHandle::GetImage()
{
	return mImage;
}

			
bool ImageHandle::GetRect(BRect* r)
{
	bool result = false;
	if (mImage) {
		if (!mImage->Lock()) return false;
		result = mImage->GetRect(r);
		mImage->Unlock();
	}
	return result;
}

	
void ImageHandle::Draw(DrawPort *drawPort, BRect* r)
{
	if (mImage == NULL)
		return;

	if (!mImage->Lock()) return;
	if (mImage->NeedsBackgroundDrawn())
		mImage->SetBGInfo(mHasBGColor, mBGColor, mBGPixels != NULL, mBGPixels, mBGHOffset, mBGVOffset);
	else
		mImage->SetBGInfo(false, 0, false, NULL, 0, 0);
	mImage->Draw(drawPort,r);
	mImage->Unlock();
}

//=========================================================================
//	All images are stored in a central cache

BList* Image::mImageCache = NULL;
TLocker Image::mCacheLocker("Image Cache Lock");

void Image::Open()
{
	if (!mCacheLocker.Lock())
		return;
	mImageCache = new BList;
	mCacheLocker.Unlock();
}

void Image::Close()
{
/*
	if (!mCacheLocker.Lock())
		return;
	delete mImageCache;
	mImageCache = NULL;
	mCacheLocker.Unlock();
*/
}

Image::Image(bool firstFrameOnly)
	: mLocker("Image Lock")
{
	mFirstFrameOnly = firstFrameOnly;
	mDrawAsThumbnail = 0;
	mRefCount = 0;
	mError = false;
	mComplete = false;
}

Image::~Image()
{
	// If the consumer gets killed before all of the ImageHandles
	// call DisposeImage on us, we need to clean up.
	for (int i = 0; i < mHandles.CountItems(); i++)
		((ImageHandle *)mHandles.ItemAt(i))->SetImage(NULL);
	if (!mCacheLocker.Lock())
		return;
	for (int i = 0; i < mImageCache->CountItems(); i++) {
		ImageEntry *imgEntry = (ImageEntry *)mImageCache->ItemAt(i);
		if (imgEntry->mImage == this) {
			mImageCache->RemoveItem(i);
//			delete imgEntry;
			imgEntry->Dereference();
		}
	}
	mCacheLocker.Unlock();
}

bool Image::Lock()
{
	return mLocker.Lock();
}

void Image::Unlock()
{
	mLocker.Unlock();
}

void Image::AddImageHandle(ImageHandle* imageHandle)
{
	mHandles.AddItem(imageHandle);
}

void Image::DeleteImageHandle(ImageHandle* imageHandle)
{
	mHandles.RemoveItem(imageHandle);
}

int Image::CountHandles()
{
	return mHandles.CountItems();
}

//	Some part of the image has changed, inform glyphs to update it

void Image::Invalidate(UpdateRegion& updateRegion)
{
	for (int i = 0; i < mHandles.CountItems(); i++)
		((ImageHandle *)mHandles.ItemAt(i))->Invalidate(updateRegion);
}

//===========================================================================
//	Return an image from the cache
//	Static method, called from multiple threads

//	Look in cache first

Image* Image::GetImageFromCache(const BString& url, bool& isCached, bool& imageError)
{
	if (!mCacheLocker.Lock()) {
		return NULL;
	}
	Image *image = NULL;
	isCached = false;
	for (int i = 0; i < mImageCache->CountItems(); i++) {
		ImageEntry *imgEntry = (ImageEntry *)mImageCache->ItemAt(i);
		if (imgEntry->mURL == url) {
			image = imgEntry->mImage;
			isCached = true;
			imageError = imgEntry->mError;
			break;
		}
	}
	if (!isCached) {
		ImageEntry *imgEntry = new ImageEntry(NULL, url.String());
		mImageCache->AddItem(imgEntry);
	}
	mCacheLocker.Unlock();
	return image;
}

void Image::ReferenceCachedImage(const BString& url, bool reference)
{
	if (!mCacheLocker.Lock()) {
		return;
	}
	for (int i = 0; i < mImageCache->CountItems(); i++) {
		ImageEntry *imgEntry = (ImageEntry *)mImageCache->ItemAt(i);
		if (imgEntry->mURL == url) {
			if (reference)
				imgEntry->Reference();
			else
				if (imgEntry->Dereference() == 0)
					mImageCache->RemoveItem(imgEntry);
			break;
		}
	}
	mCacheLocker.Unlock();
}

void Image::CachedImageError(const BString& url)
{
	if (!mCacheLocker.Lock()) {
		return;
	}
	for (int i = 0; i < mImageCache->CountItems(); i++) {
		ImageEntry *imgEntry = (ImageEntry *)mImageCache->ItemAt(i);
		if (imgEntry->mURL == url) {
			imgEntry->mError = true;
			break;
		}
	}
	mCacheLocker.Unlock();
}

void DetermineFileType(const char *filename, ulong fileType, const char *data, int dataCount, char *typeStr);
//	In BePlatform.cpp

//	Create a new image from this resource when it is ready

Image* Image::GetImageFromResource(UResourceImp* resource, bool firstFrameOnly)
{
	if (resource->LockWithTimeout(ONE_SECOND / 10) != B_OK)
		return NULL;
	if (!mCacheLocker.Lock())
		return NULL;

	Image *image = NULL;
	
//	NP_ASSERT(resource);
//	NP_ASSERT(resource->GetImp());
	
//	Get enough of the data determine file type
//	Don't trust the mime types, lots of GIF's show up as JPEG etc
	
	ImageEntry *imgEntry = 0;
	const char *url = resource->GetURL();
	for (int i = 0; i < mImageCache->CountItems(); i++) {
		ImageEntry *ie = (ImageEntry *)mImageCache->ItemAt(i);
		if (ie->mURL == url)
			imgEntry = ie;
	}
	
	if (!imgEntry) {
		imgEntry = new ImageEntry(NULL, url);
		mImageCache->AddItem(imgEntry);
	}
	
	if (imgEntry->mImage) {
		mCacheLocker.Unlock();
		resource->Unlock();
		return imgEntry->mImage;
	}

	long currentSize,expectedSize;
	resource->GetProgress(&currentSize,&expectedSize);	
	if (currentSize < 6) {
		mCacheLocker.Unlock();
		resource->Unlock();
		return NULL;
	}
		
	char* data = (char*)resource->GetData(0,6);	
	if (data == NULL) {
		mCacheLocker.Unlock();
		resource->Unlock();
		return NULL;						// And error occured locking the resource buffer
	}

	char t[256];
	DetermineFileType("?", '????',data,6,t);
	resource->ReleaseData(data,6);			// Relase data so buffer can refill...

//	If resource is of a known image type, create one

	if (strstr(t,"jpeg") || strstr(t,"jpg") || strstr(t,"image/jpeg")) {
		image = new JPEGImage;
//	} else if (strstr(t,"jpeg") || strstr(t,"jpg") || strstr(t,"image/jpeg") || strstr(t, "image/png")) {
	} else if (strstr(t,"image/png")) {
		image = new TranslationKitImage;
	} else if (strstr(t,"gif") || strstr(t,"image/gif")) {
		image = new GIFImage(firstFrameOnly);
	} else {
		//	Don't know what type of image it could be!
		pprint("Image::GetImage: Bad Image Type: %s",t);
		resource->SetStatus(kAbort);
	}
	if (image) {
		image->SetURL(resource->GetURL());
		imgEntry->mImage = image;
	}
	mCacheLocker.Unlock();
	resource->Unlock();
	return image;
}

//	Disposing of an imagehandle will dispose its image
//	If no other handles reference that image, the image pixels will be deleted from the cache

void Image::DisposeImage(ImageHandle *imageHandle)
{
	Image *image = imageHandle->GetImage();
	if (image == NULL)
		return;

	image->DeleteImageHandle(imageHandle);

	if (!mCacheLocker.Lock())
		return;

	if (image->CountHandles() <= 0 && image->mRefCount <= 0) {	// Refcount == 0, Image is dead, get rid of it
		delete image;
	}
	mCacheLocker.Unlock();
}

void Image::Reference()
{
	atomic_add(&mRefCount, 1);
}

void Image::Dereference()
{
	if (atomic_add(&mRefCount, -1) <= 1 && CountHandles() <= 0)
		delete this;
}

//	An error occurred parsing the image. Rats

bool Image::IsDead()
{
	return mError;
}

bool Image::IsTransparent()
{
	return false;
}

void Image::SetBGInfo(bool, long, bool, Pixels *, float, float)
{
}

void Image::SetConsumer(ImageConsumer *)
{
}

bool Image::SetComplete()
{
	mComplete = true;
	return true;
}



//===========================================================================

GIFImage::GIFImage(bool firstFrameOnly)
{
	mFirstFrameOnly = firstFrameOnly;
	mGIF = new CGIF(mFirstFrameOnly);
	mPixels = NULL;
}

GIFImage::~GIFImage()
{
	delete mGIF;
	delete mPixels;
}

void GIFImage::Reset()
{
	delete mGIF;
	delete mPixels;
	
	mGIF = new CGIF(mFirstFrameOnly);
	mPixels = NULL;
	if (mConsumer)
		mConsumer->Reset();
}

bool GIFImage::IsTransparent()
{
	long bogus;
	if (mPixels)
		return mPixels->GetTransparentColor(&bogus);
	else
		return false;
}

long GIFImage::Write(uchar *d, long count)
{
	if (mPixels == NULL) {
//		mPixels = gBePlatform->NewPixels(8);	// 8 bit source pixels
		mPixels = new BePixels(NetPositive::MainScreenColorSpace() == B_COLOR_8_BIT);
	}

	long retval = mGIF->Write(d,count,mPixels);
		
//	If pixels changed, invalidate them in glyphs that display them

	UpdateRegion updateRegion;
	if (mPixels && mPixels->GetUpdate(updateRegion)) {
		Invalidate(updateRegion);
	}
	return retval;
}


bool GIFImage::SetComplete()
{
	Image::SetComplete();
	
	if (gPreferences.FindBool("ShowAnimations") && mError == 0 && mGIF->LoopAnimation() && mConsumer) {
		mConsumer->Reset();
		return false;
	} else if (mConsumer)
		mConsumer = NULL;
	return true;
}

void GIFImage::SetConsumer(ImageConsumer *consumer)
{
	mConsumer = consumer;
}

//	Reset the consumer if the GIF animation wants to loop....

void GIFImage::SetBGInfo(bool hasBGColor, long bgColor, bool hasBGImage, Pixels *bgImage, float bgHOffset, float bgVOffset)
{
	mGIF->SetBG(hasBGColor, bgColor, hasBGImage, bgImage, bgHOffset, bgVOffset);
}


void GIFImage::Draw(DrawPort *drawPort, BRect *dstRect)
{
	if (mPixels == NULL)
		return;
		
	BRect srcR;
	GetRect(&srcR);
	if (!srcR.bottom)
		return;
		
	if (mGIF->GetMaxYPos() == 0) {
		return;     		// nothing to draw yet
	}
	
	drawPort->DrawPixels(mPixels,&srcR,dstRect, !IsTransparent() && Complete(), mGIF->IsAnimating());	// Draw all that has come in so far
}

bool GIFImage::NeedsBackgroundDrawn()
{
	BRect srcR;
	GetRect(&srcR);
	return mPixels && srcR.bottom && mGIF->GetMaxYPos() && IsTransparent() && mGIF->IsAnimating();
}

bool	GIFImage::GetRect(BRect *r)
{
	return mGIF->GetRect(r);
}


//===========================================================================
//	Draws a JPEG image a row at a time

JPEGImage::JPEGImage()
{
	mJPEG = 0;
	mPixels = 0;
	mMaxHeight = 0;
	mWaitingForScan = false;
}

JPEGImage::~JPEGImage()
{
	if (mJPEG)
		DisposeJPEGDecoder(mJPEG);
	if (mPixels)
		delete(mPixels);
}


//===========================================================================
//	Called each time a single row is ready to be drawn

short MakeColorSlice(JPEGDecoder *j, Pixels *pixels, short line, short width, short height);

JPEGError JPEGImage::DrawRow(JPEGDecoder *j)
{
	short	top = j->currentSlice * j->MCUVPixels;
	short	bottom = j->MCUVPixels;
	
	short	height = j->Height;
	short	width = j->Width;
	
	short	fullWidth = j->MCUHPixels * j->WidthMCU;
	short	fullHeight = j->HeightMCU * j->MCUVPixels;

	if (j->thumbnail) {
		height = height >> 3;
		width = width >> 3;
	}
	if (top + bottom > height)
		bottom = height % j->MCUVPixels;

//	Lazy init of pixels
	if (mPixels == NULL) {
//		mPixels = gBePlatform->NewPixels(32);
		mPixels = new BePixels(NetPositive::MainScreenColorSpace() == B_COLOR_8_BIT);
		if (!mPixels->Create(fullWidth,fullHeight,32))
			return kJPEGCancelled;
	}
	
//	Create the slice of pixels

	MakeColorSlice(j,mPixels,top,fullWidth,j->MCUVPixels);
	if (top + bottom == j->Height)				// Image is complete
		mPixels->Finalize();
	mMaxHeight = top + bottom;
	
//	Update region

	UpdateRegion updateRegion;
 	if (mPixels->GetUpdate(updateRegion)) {
 		Image *image = (Image *)j->DrawRefCon;
 		image->Invalidate(updateRegion);
 	}
 	
	return kNoError;
}

//	Hook Proc to call DrawRow

JPEGError DrawRowProc(void *decoder)
{
	JPEGDecoder *j = (JPEGDecoder *)decoder;
	JPEGImage *image = (JPEGImage *)j->DrawRefCon;
	if (image == 0)
		return kJPEGCancelled;
	return image->DrawRow(j);
}

//	Write

long JPEGImage::Write(uchar *d, long count)
{
	if (!mJPEG) {
		BRect r(0,0,32000,32000);	// Huge draw size to avoid clipping
		mJPEG = NewJPEGDecoder(&r,mDrawAsThumbnail,DrawRowProc,NULL);	// This one is used for progressive drawing
		if (!mJPEG)
			return -1;
		mJPEG->DrawRefCon = this;
	}
	
/*
	if (mLookingForRect && mJPEG->Width) {	// Looking for the rectangle, just found it
		mLookingForRect = false;
		return 0;							// Just found size, suspend consume
	}
*/
	if (mWaitingForScan) {					// Suspend consume after every scan
		mWaitingForScan = false;
		return 0;
	}
	
	long result = JPEGWrite(mJPEG,d,count);
	if (mJPEG->isProgressive && mJPEG->phase == kWaitingForScan)
		mWaitingForScan = true;				// Suspend consume after every scan for progressive
		
	UpdateRegion updateRegion;
	Pixels *p = GetPixels();
	if (p) {
		updateRegion.Invalidate(BRect(0,0,p->GetWidth(),p->GetHeight()));
		Invalidate(updateRegion);
	}
	return result;
}

//	Draw fully in response to an update

void JPEGImage::Draw(DrawPort *drawPort,BRect *dstRect)
{
	if (mJPEG == NULL)
		return;
		
	if (mJPEG->WidthMCU) {	//	Redraw what DrawProgressive has drawn
		if (mMaxHeight) {
			BRect src;
			src.top = src.left = 0;
			src.right = mJPEG->Width;
			src.bottom = mMaxHeight;
			
			dstRect->bottom = dstRect->top + (mMaxHeight * (dstRect->bottom - dstRect->top) + (mJPEG->Height / 2))/mJPEG->Height;
			dstRect->bottom = MIN(dstRect->bottom, mJPEG->Height + dstRect->top);
			drawPort->DrawPixels(mPixels,&src,dstRect, true);
		}
	}
}

//	Return the size of the image as soon as we know it
//	еееееееееееееее a little messy still

bool	JPEGImage::GetRect(BRect *r)
{
	if (mJPEG && mJPEG->Width) {
		r->Set(0,0,mJPEG->Width,mJPEG->Height);
		return true;
	}
	return false;
}
	
/*
class BlockingMallocIO : public BMallocIO {
public:
					BlockingMallocIO();
	virtual			~BlockingMallocIO();
	virtual ssize_t	Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);
	virtual ssize_t	ReadAt(off_t pos, void *buffer, size_t size);
	virtual ssize_t	WriteAt(off_t pos, const void *buffer, size_t size);
protected:
	sem_id			mSemaphore;
};

BlockingMallocIO::BlockingMallocIO()
{
	mSemaphore = create_sem(0, "BlockingMallocIO");
}

BlockingMallocIO::~BlockingMallocIO()
{
	delete_sem(mSemaphore);
}

ssize_t BlockingMallocIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t bytes = 0;
	while (bytes == 0) {
		bytes = BMallocIO::ReadAt(pos, buffer, size);
		if (bytes == 0) {
			// If there were no bytes to be read, wait at the semaphore until
			// more come in and WriteAt releases it.
//printf("ReadAt acquiring semaphore\n");
			if (acquire_sem(mSemaphore) != B_OK)
				return -1;
//printf("ReadAt got semaphore\n");
		}
	}
	return bytes;
}

ssize_t BlockingMallocIO::Read(void *buffer, size_t size)
{
	ssize_t bytes = 0;
	while (bytes == 0) {
		bytes = BMallocIO::Read(buffer, size);
		if (bytes == 0) {
			// If there were no bytes to be read, wait at the semaphore until
			// more come in and WriteAt releases it.
//printf("Read acquiring semaphore\n");
			if (acquire_sem(mSemaphore) != B_OK)
				return -1;
//printf("Read got semaphore\n");
		}
	}
	return bytes;
}

ssize_t BlockingMallocIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	ssize_t bytes = BMallocIO::WriteAt(pos, buffer, size);
	
//printf("WriteAt acquiring semaphore\n");
	// If someone else is waiting on the semaphore, release them.  The acquire_sem
	// will fail if someone else is waiting.
	acquire_sem_etc(mSemaphore, 1, B_TIMEOUT, 0);
	release_sem(mSemaphore);
//printf("WriteAt releasing semaphore\n");

	return bytes;
}

ssize_t BlockingMallocIO::Write(const void *buffer, size_t size)
{
	ssize_t bytes = BMallocIO::Write(buffer, size);
	
//printf("Write acquiring semaphore\n");
	// If someone else is waiting on the semaphore, release them.  The acquire_sem
	// will fail if someone else is waiting.
	acquire_sem_etc(mSemaphore, 1, B_TIMEOUT, 0);
	release_sem(mSemaphore);
//printf("Write releasing semaphore\n");

	return bytes;
}
*/

class IncrementalBitmapStream : public BBitmapStream {
	IncrementalBitmapStream() : BBitmapStream(NULL) {}
	BBitmap *GetInternalBitmap() {return fMap;}
};

TranslationKitImage::TranslationKitImage()
{
	pprint("Creating Image");
	mPixels = NULL;
	mStream = NULL;
	mBitmap = NULL;
}

TranslationKitImage::~TranslationKitImage()
{
	delete mPixels;
	delete mStream;
}


long TranslationKitImage::Write(uchar *d, long count)
{
	if (!mStream) {
//		mStream = new BlockingMallocIO;
		mStream = new BMallocIO;
	}

	long result = mStream->Write(d, count);

	return result;
}


bool TranslationKitImage::SetComplete()
{
	if (!mBitmap && !mError && mStream && mStream->Position() >= 16) {
//printf("We have %ld bytes of image, trying to get bitmap.\n", mStream->Position());
		mBitmap = BTranslationUtils::GetBitmap(mStream);
//printf("Got bitmap\n");

		if (!mPixels && mBitmap && mStream->Position() >= 16) {
			if (mBitmap->Bounds().Width() > 0 && mBitmap->Bounds().Height() > 0) {
				BePixels* p = new BePixels(mBitmap);
				p->SetIsFullAlpha(true);
				mPixels = p;
			}
		}
			
		if (mPixels) {
			UpdateRegion updateRegion;
			updateRegion.Invalidate(BRect(0,0,mPixels->GetWidth(),mPixels->GetHeight()));
			Invalidate(updateRegion);
		}
	}
	return true;
}

void TranslationKitImage::Draw(DrawPort *drawPort, BRect *dstRect)
{
	if (mPixels == NULL)
		return;
		BRect srcR;
	GetRect(&srcR);
	if (!srcR.bottom) return;
	
	drawPort->DrawPixels(mPixels,&srcR,dstRect, !IsTransparent() && Complete());	// Draw all that has come in so far
}

bool TranslationKitImage::GetRect(BRect *r)
{
	if (mPixels) {
		r->top = r->left = 0;
		r->bottom = mPixels->GetHeight();
		r->right = mPixels->GetWidth();
		return true;
	} else if (mBitmap) {
		*r = mBitmap->Bounds();
		return (r->Width() > 0 && r->Height() > 0);
	}
	return false;		
}

