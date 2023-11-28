#include <Bitmap.h>
#include <BufferConsumer.h>
#include <Debug.h>
#include <Handler.h>
#include <MediaKit.h>
#include <Message.h>
#include <Messenger.h>
#include <Window.h>

#include <signal.h>
#include <stdio.h>

#include "DrawingTidbits.h"
#include <TimedEventQueue.h>
#include "VideoView.h"
#include "VideoConsumerNode.h"

const bigtime_t kWindowLockTimeout = 20000;

VideoView::VideoView(BRect rect, BPoint videoSize, const char *name, uint32 resizingMode)
	:	BView(rect, name, resizingMode, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fCurrentBitmap(0),
		fWindow(0),
		fVideoSize(videoSize),
		fProportionalResize(true)
{
	SetViewColor(B_TRANSPARENT_COLOR); // to reduce flicker
}

VideoView::~VideoView()
{
}

void 
VideoView::SetProprotionalResize(bool on)
{
	fProportionalResize = on;
}

bool 
VideoView::ProportionalResize() const
{
	return fProportionalResize;
}

BRect 
VideoView::ViewRect(BRect videoRect)
{
	if (videoRect.Width() < kMinViewWidth) 
		videoRect.right = videoRect.left + kMinViewWidth;

	return videoRect;
}

BRect 
VideoView::VideoRect(BRect frame, BPoint videoSize)
{
	// fitting algorithm - fit the video bitmap into the surrounding
	// view butting either vertically or horizontally

	// try horizontal fit
	float scale = frame.Width() / videoSize.x;
	if (videoSize.y * scale <= frame.Height()) {
		BRect result(frame.left, 0, frame.right, videoSize.y * scale);
		result.OffsetBy(0, (frame.Height() - result.Height()) / 2);
		return result;
	}
	// try a vertical fit
	scale = frame.Height() / videoSize.y;
	ASSERT(videoSize.x * scale <= frame.Width());
	BRect result(0, frame.top, videoSize.x * scale, frame.bottom);
	result.OffsetBy((frame.Width() - result.Width()) / 2, 0);
	return result;
}

BRect 
VideoView::ContentRect()
{
	BRect bounds(Bounds());
	if (!fProportionalResize)
		return bounds;

	return VideoRect(bounds, fVideoSize);
}

void
VideoView::Draw(BRect)
{
	BRect bounds(Bounds());
	BRect rect(ContentRect());

	// draw parts of the view not covered with video in black
	SetHighColor(kBlack);
	BRect tmp(bounds);
	if (tmp.left < rect.left) {
		tmp.right = rect.left;
		FillRect(tmp);
	}
	tmp = bounds;
	if (tmp.right > rect.right) {
		tmp.left = rect.right;
		FillRect(tmp);
	}
	tmp = bounds;
	if (tmp.top < rect.top) {
		tmp.bottom = rect.top;
		FillRect(tmp);
	}
	tmp = bounds;
	if (tmp.bottom > rect.bottom) {
		tmp.top = rect.bottom;
		FillRect(tmp);
	}

	// blit the bitmap
	if (fCurrentBitmap)
		DrawBitmapAsync(fCurrentBitmap, rect);
	else 
		FillRect(rect);
}

void 
VideoView::AttachedToWindow()
{	
	fWindow = Window();
}


void
VideoView::SetBitmap(BBitmap *bitmap)
{
	fCurrentBitmap = bitmap;
	// Note that if the lock times out, the buffer gets dropped.
	// This is ok, because it is late anyway.  This may timeout
	// because the window was actually locked too long, or it
	// may have deadlocked while trying to stop (It always does
	// when dropping something in a window... sigh.)
	if (fWindow->LockWithTimeout(100000) == B_OK) {
		if (bitmap)
			DrawBitmap(bitmap, ContentRect());	
		else
			Invalidate(ContentRect());
		
		fWindow->Unlock();
	}
}

