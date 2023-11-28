//******************************************************************************
//
//	File:			GIFContent.cpp
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//  Written by: Mike Clifton
//
//******************************************************************************

#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <Message.h>
#include <ResourceCache.h>
#include <Screen.h>

#include "Gehnaphore.h"
#include "Content.h"
#include "Timer.h"
#include "GIF.h"

using namespace Wagner;

static const bigtime_t kMinUpdateInterval = 10000;
static const uint32 msgUpdateFlags = 'mupf';

class GIFContentInstance : public ContentInstance {
public:
	GIFContentInstance(Content *content, GHandler *handler);
	virtual ~GIFContentInstance();
	
	void UpdateTransparency();
	
	virtual status_t Draw(BView *into, BRect exposed);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *outFlags);
	virtual	status_t ContentNotification(BMessage *msg);
};

class GIFContent : public Content, public GHandler {
public:
	GIFContent(void *handle);
	virtual ~GIFContent();
	bool IsDone();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	bool FeedSelf();
	BBitmap *GetBitmap();
	void InstanceGone();
	
	void SetAlpha(bool hasIt);
	bool HasAlpha() const;
	
	virtual size_t GetMemoryUsage();
	virtual	bool IsInitialized();
	virtual	void Cleanup();

	virtual status_t HandleMessage(BMessage* data);
	
private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);
	static int32 AnimLoop(void *data);
	status_t CreateBitmap();

	Gehnaphore fAccess;
	BBitmap *fBitmap;
	GIF *fGIF;
	bigtime_t fLastUpdate;
	bool fDone;
	bool fError;
	bool fLastLoop;
	bool fStartingUp;
	bool fMsgSent;
	bool fHasAlpha;
	uchar *fData;
	uint32 fMaxData;
	uint32 fDataWritePos;
	uint32 fDataReadPos;
	bool fIsAnimated;
	int32 fInstanceCount;
	BRect fImageSize;
	
	friend class GIFContentInstance;
};



GIFContentInstance::GIFContentInstance(Content *content, GHandler *handler)
	: ContentInstance(content, handler)
{
	UpdateTransparency();
}

GIFContentInstance::~GIFContentInstance()
{
	GIFContent *content = (GIFContent *)GetContent();
	if (content) content->InstanceGone();
}

void GIFContentInstance::UpdateTransparency()
{
	GIFContent *content = (GIFContent*) GetContent();
	if (content)
		SetFlags((Flags() & ~(cifHasTransparency | cifDoubleBuffer))
			| (content->HasAlpha() ? (cifDoubleBuffer | cifHasTransparency) : 0));
}

status_t GIFContentInstance::Draw(BView *into, BRect /* exposed */)
{
	GIFContent *content = (GIFContent *)GetContent();
	BBitmap *bitmap = content ? content->GetBitmap() : 0;
	if (bitmap) {
		if (content->HasAlpha() || !content->IsDone())
			into->SetDrawingMode(B_OP_ALPHA);
		else
			into->SetDrawingMode(B_OP_COPY);

		into->DrawBitmapAsync(bitmap, bitmap->Bounds(), FrameInParent());
	}

	return B_OK;
}

status_t GIFContentInstance::GetSize(int32 *width, int32 *height, uint32 *outFlags)
{
	GIFContent *content = (GIFContent *)GetContent();
	BBitmap *bitmap = content->GetBitmap();
	if (bitmap) {
		BRect r = bitmap->Bounds();
		*width = (int32)(r.Width()+1);
		*height = (int32)(r.Height()+1);
	} else {
		*width = 0;
		*height = 0;
	}

	*outFlags = 0;
	return B_OK;
}

status_t GIFContentInstance::ContentNotification(BMessage *msg)
{
	if (msg && msg->what == msgUpdateFlags) {
		UpdateTransparency();
		return B_OK;
	}
	
	return ContentInstance::ContentNotification(msg);
}

bool GIFContent::IsInitialized()
{
	return fBitmap;
}

size_t GIFContent::GetMemoryUsage()
{
	return sizeof(GIFContent)
		+ (fBitmap ? fBitmap->BitsLength() : 0)
		+ (fData ? fMaxData : 0);
		+ fGIF ? fGIF->GetMemoryUsage() : 0;
}

GIFContent::GIFContent(void *handle)
	:	Content(handle),
		GHandler("GIF"),
		fBitmap(NULL),
		fGIF(0),
		fLastUpdate(system_time()),
		fDone(false),
		fError(false),
		fLastLoop(false),
		fStartingUp(false),
		fMsgSent(false),
		fHasAlpha(false),
		fData(NULL),
		fMaxData(0),
		fDataWritePos(0),
		fDataReadPos(0),
		fIsAnimated(false),
		fInstanceCount(0)
{
}

void GIFContent::Cleanup()
{
	GehnaphoreAutoLock l(fAccess);
	Content::Cleanup();
	GHandler::Cleanup();
}

GIFContent::~GIFContent()
{
	delete fBitmap;
	delete fGIF;
	free(fData);	
}

bool GIFContent::IsDone()
{
	return fDone;
}

ssize_t GIFContent::Feed(const void *d, ssize_t count, bool done)
{
	GehnaphoreAutoLock l(fAccess);

	if (fError) return count;
	
	if (!fData) {
		// fMaxData is an educated guess here.  I printed sizes from a lot of sites,
		// and 2-4 is a pretty good start.  Minimizing reallocs
		// is very good, as it will probably involve a copy
		// with lots of little GIFs being allocated.
		fMaxData = 0x1000;
		fData = (uchar*) malloc(fMaxData);
		fDataWritePos = fDataReadPos = 0;
	}

	if (fDataWritePos + count > fMaxData) {
		fMaxData = MAX(fMaxData * 2, fDataWritePos + count);
		fData = (uchar*) realloc(fData, fMaxData);
		if (!fData)
			return B_ERROR;
	}

	memcpy(fData + fDataWritePos, d, count);
	fDataWritePos += count;
	
	if (fGIF == 0)
		fGIF = new GIF;

	ssize_t	retval = fGIF->Write(fData + fDataReadPos, fDataWritePos - fDataReadPos, fBitmap);
	fDataReadPos += retval;

	if (!fBitmap && fGIF->GetRect(&fImageSize))
		if (fImageSize.Width() + 1 > 0 && fImageSize.Height() + 1 > 0)
			if (CreateBitmap() < 0)
				return B_NO_MEMORY;

	bool is_animated = fGIF->IsAnimating();
	
	// If this doesn't look like an animated GIF, consume data and display
	// immediately like a normal image.
	if (!is_animated) {
		do {
			retval = fGIF->Write(fData + fDataReadPos, fDataWritePos - fDataReadPos, fBitmap);
			PRINT(("Feed %p: Wrote %ld bytes of %ld, now at %ld\n",
					this, retval, fDataWritePos-fDataReadPos, fDataReadPos+retval));
			if (retval >= 0) {
				fDataReadPos += retval;
				if (fDataReadPos >= fDataWritePos) {
					if (fDone) // reset read position back to start
						fDataReadPos = 0;
	
					break;
				}
			} else {
				// error in reader: stop now.
				done = fDone = fError = true;
				fDataReadPos = 0;
				fGIF->Reset();
			}
			if (fGIF->IsAnimating())
				is_animated = true;
		} while (retval > 0 && !is_animated);
		
		if (fGIF->IsTransparent())
			SetAlpha(true);

		// If it now does look like an animated GIF, start up the animation
		// timer and perform further reads from there, so that data is
		// pushed through in time with the animation rather than when it is
		// received.
		if (fGIF->IsAnimating()) {
			PRINT(("*** Found animated GIF -- starting animation mode.\n"));
			fIsAnimated = true;
			if (fInstanceCount >= 1 && fStartingUp == false) {
				// If instances have already been made and they have
				// tried to start the animation, then start the animation
				// right now.
				PRINT(("Already have instances, starting immediately.\n"));
				FeedSelf();
			} else {
				// Otherwise, indicate that the animation should start
				// and let it be taken care of the first time a bitmap
				// is requested.
				PRINT(("Flagging to start animation when bitmap drawn.\n"));
				fStartingUp = true;
			}

			MarkAllDirty();
		}
	}
	
	if (done) {
		fDone = true;
		if (!fGIF->IsAnimating()) {
			// the image has been decoded - don't need the data or decoder anymore
			free(fData);
			fData = NULL;
			delete fGIF;
			fGIF = NULL;
			fMaxData = 0;
		}
	}

	if (system_time() - fLastUpdate > kMinUpdateInterval) {
		fLastUpdate = system_time();
		MarkAllDirty();
	} else if (done && retval == count)
		MarkAllDirty();

	return count;
}

// returns true if some data was consumed
bool GIFContent::FeedSelf()
{
	// in this case, you'll probably want to force a redraw
	if (fData == NULL || fGIF == NULL) {
		PRINT(("fData == NULL or !fGIF.IsAnimating(), bailing\n"));
		return false;
	}
	
	// if there is no more looping and we are back at the file start,
	// don't do anything else.
	if (fDataReadPos == 0 && fLastLoop) {
		PRINT(("fDataReadPos == 0 && fLastLoop == true, bailing\n"));
		return false;
	}
	
	// If no more instances are viewing this content, do nothing.
	if (fInstanceCount <= 0) {
		PRINT(("fInstanceCount <= 0, bailing\n"));
		return false;
	}
	
	bigtime_t delay = (fGIF->GetDelay() - system_time()/10000) * 10000;
	if (delay > 0) {
		PRINT(("Before read: delay=%.4f, time=%.4f, interval=%.4f\n",
				(float)fGIF->GetDelay() / (1000000/10000),
				(float)system_time() / 1000000,
				(float)delay / 1000000));
		if (!fMsgSent) PostDelayedMessage(new BMessage(), delay);
		fMsgSent = true;
		return false;
	}
	
	ssize_t retval;
	bool didFeed = false;
	
	do {
		retval = fGIF->Write(fData + fDataReadPos, fDataWritePos - fDataReadPos, fBitmap);
		PRINT(("FeedSelf %p: Wrote %ld bytes of %ld, now at %ld\n",
			this, retval, fDataWritePos-fDataReadPos, fDataReadPos+retval));
		if (retval < 0) {
			// error in reader: jump to end of animation and restart
			// if desired.
			fDataReadPos = fDataWritePos;
			fDone = fError = true;
			PRINT(("Error at frame %ld of %ld\n",
				fGIF->CurrentFrame(), fGIF->CountFrames()));
			if (fGIF->CountFrames() > 1) {
				// if this has at least two frames, restart
				// animation right now.
				retval = 1;
			} else {
				// if only one frame, use as single image.
				PRINT(("*** Only one frame!  Bailing on animation.\n"));
				free(fData);
				fData = NULL;
				delete fGIF;
				fGIF = NULL;
				fMaxData = 0;
				fIsAnimated = false;
				return true;
			}
			fGIF->Restart();
		} else {
			fDataReadPos += retval;
		}
		if (fDataReadPos >= fDataWritePos) {
			PRINT(("*** Reached end of animation.\n"));
			if (fDone) {
				// reset read position back to start
				fDataReadPos = 0;
				PRINT(("Now at start of data, length=%ld\n", fDataWritePos-fDataReadPos));
				if (!fGIF->LoopAnimation()) {
					fLastLoop = true;
					if (fGIF->IsTransparent()) SetAlpha(true);
					// No more animations, dispose of decoder
					delete fGIF;
					fGIF = 0;
					return didFeed;
				}
			}
		}
		if (retval > 0)
			didFeed = true;
			
	} while (retval > 0);
	
	if (fGIF->IsTransparent()) SetAlpha(true);
	
	delay = ((bigtime_t)fGIF->GetDelay())*10000 - system_time();
	
	PRINT(("After read: delay=%.4f, time=%.4f, interval=%.4f\n",
			(float)fGIF->GetDelay() / (1000000/10000),
			(float)system_time() / 1000000,
			(float)delay / 1000000));
	
	if (delay <= 0)
		delay = 100;

	if (!fMsgSent)
		PostDelayedMessage(new BMessage(), delay);

	fMsgSent = true;
	
	return didFeed;
}

status_t GIFContent::CreateBitmap()
{
	BScreen screen;
	int32 y;
	uint8 *rowPtr;
	uint32 rowBytes;
	color_space cspace;
	switch (screen.ColorSpace()) {
		case B_RGB16:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_CMAP8:
			cspace = B_RGBA15;
			break;
		case B_RGB32:
		case B_RGBA32:
		case B_RGB24:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB24_BIG:
		default:
			cspace = B_RGBA32;
			break;
	}

	fBitmap = new BBitmap(fImageSize, cspace);
	if (!fBitmap || !fBitmap->IsValid()) {
		delete fBitmap;
		fBitmap = NULL;
		return B_NO_MEMORY;
	}

	if (fBitmap) {
		// clear the bitmap (including the alpha channel)
		rowPtr = (uint8*) fBitmap->Bits();
		rowBytes = fBitmap->BytesPerRow();
		for (y = (int32) fImageSize.Height() + 1; y; y--) {
			memset(rowPtr, 0, rowBytes);
			rowPtr += rowBytes;
		}
	}

	return B_OK;
}

BBitmap *GIFContent::GetBitmap()
{
	if (fStartingUp) {
		PRINT(("GetBitmap() %p with fStartingUp -- start animation.\n", this));
		GehnaphoreAutoLock l(fAccess);
		if (fStartingUp) {
			PRINT(("Feeding...\n"));
			if (FeedSelf()) MarkAllDirty();
			fStartingUp = false;
		}
	}
	
	return fBitmap;
}

void GIFContent::InstanceGone()
{
	GehnaphoreAutoLock l(fAccess);
	fInstanceCount--;
	if (fInstanceCount == 0) {
		fStartingUp = false;

		// Dispose of the decoder if its around
		delete fGIF;
		fGIF = 0;

		if (fIsAnimated) {
			// If we're animated, dispose of the bitmap too, since we have
			// the data to recreate it.
			delete fBitmap;
			fBitmap = 0;
		}
	}
}

void GIFContent::SetAlpha(bool hasIt)
{
	if (hasIt != fHasAlpha) {
		fHasAlpha = hasIt;
		BMessage update(msgUpdateFlags);
		NotifyInstances(&update);
	}
}

bool GIFContent::HasAlpha() const
{
	return fHasAlpha;
}

status_t GIFContent::CreateInstance(ContentInstance **outInstance, GHandler *handler,
	const BMessage&)
{
	GehnaphoreAutoLock l(fAccess);
	fInstanceCount++;
	if (fInstanceCount == 1 && fIsAnimated) {
		// If this is the first instance, restart animation.
		PRINT(("First animation, reset to start.\n"));
		fDataReadPos = 0;
		fLastLoop = false;
		fStartingUp = true;
		
		// Recreate the bitmap, which we trashed when we stopped animating
		if (CreateBitmap() < 0)
			return B_ERROR;

		// Restart the animation.  Create a new decoder and feed it the global
		// file header.
		if (fGIF) fGIF->Reset();
		if (fData) {
			if (!fGIF)
				fGIF = new GIF;
			fDataReadPos = fGIF->Write(fData, fDataWritePos, fBitmap);
			if (fDataReadPos < 0)
				fDataReadPos = 0;
			FeedSelf();
		}
	}

	*outInstance = new GIFContentInstance(this, handler);
	
	return B_OK;
}

status_t GIFContent::HandleMessage(BMessage*)
{
	fAccess.Lock();
	fMsgSent = false;
	bool mark = (fInstanceCount && FeedSelf());
	fAccess.Unlock();

	if (mark) MarkAllDirty();

	return B_OK;
}

// ----------------------- GIFContentFactory -----------------------

class GIFContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "image/gif");
		into->AddString(S_CONTENT_EXTENSIONS, "gif");
	}
	
	virtual Content* CreateContent(void* handle, const char*, const char*)
	{
		return new GIFContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you,
	uint32 flags, ...)
{
	if (n == 0) return new GIFContentFactory;
	return 0;
}
