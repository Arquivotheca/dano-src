#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <Handler.h>
#include <Message.h>
#include <Messenger.h>
#include <stdio.h>
#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Window.h>
#include <string.h>
#include <stdlib.h>

#include "DrawingTidbits.h"
#include "VideoView.h"
#include "VideoConsumerNode.h"
#include "debug.h"
#include "colorspace.h" // from the PPM translator

const bigtime_t kWindowLockTimeout = 20000;
float kDragTolerance = 5.0;

VideoView::VideoView(BRect rect, BPoint videoSize, const char *name, uint32 resizingMode)
	:	BView(rect, name, resizingMode, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
			| B_FRAME_EVENTS),
		fDisplayBitmap(0),
		fWindow(0),
		fVideoSize(videoSize),
		fProportionalResize(true),
		fUsingOverlay(false),
		fBeginDrag(false),
		fCapturedFrame(0)
{
	SetViewColor(B_TRANSPARENT_COLOR); // to reduce flicker
}

VideoView::~VideoView()
{
	if (fCapturedFrame)
		delete fCapturedFrame;
}

void VideoView::SetProportionalResize(bool on)
{
	fProportionalResize = on;
}

bool VideoView::ProportionalResize() const
{
	return fProportionalResize;
}

BRect VideoView::ViewRect(BRect videoRect)
{
	if (videoRect.Width() < kMinViewWidth) 
		videoRect.right = videoRect.left + kMinViewWidth;

	return videoRect;
}

BRect VideoView::VideoRect(BRect frame, BPoint videoSize)
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

BRect VideoView::ContentRect()
{
	BRect bounds(Bounds());
	if (!fProportionalResize)
		return bounds;

	return VideoRect(bounds, fVideoSize);
}

void VideoView::Draw(BRect)
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

	if (fUsingOverlay) {
		SetHighColor(fOverlayKeyColor);
		FillRect(rect); 						// Fill in overlay color
	} else if (fDisplayBitmap)
		DrawBitmapAsync(fDisplayBitmap, rect);  // Draw current frame
	else
		FillRect(rect);							// Black background
}

void VideoView::AttachedToWindow()
{	
	fWindow = Window();
}

void VideoView::SetVideoSize(BPoint newSize)
{
	fVideoSize = newSize;
}

void VideoView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	if (fUsingOverlay && fDisplayBitmap) {
		// Resize the overlay rect
		SetViewOverlay(fDisplayBitmap, fDisplayBitmap->Bounds(), ContentRect(),
			&fOverlayKeyColor, B_FOLLOW_NONE, 
			B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
	}
}

void VideoView::SetBitmap(BBitmap *bitmap, bool isOverlay)
{
	// Note that if the lock times out, the buffer gets dropped.
	// This is ok, because it is late anyway.  This may timeout
	// because the window was actually locked too long, or it
	// may have deadlocked while trying to stop (It always does
	// when dropping something in a window... sigh.)
	if (!fWindow->LockWithTimeout(100000) == B_OK) {
		fDisplayBitmap = bitmap;
		fUsingOverlay = isOverlay;
		return;
	}
		
	if (isOverlay) {
		if (bitmap == 0)
			ClearViewOverlay();
		else if (bitmap != fDisplayBitmap) {
			// new overlay bitmap
			rgb_color overlayKeyColor;
			SetViewOverlay(bitmap, bitmap->Bounds(), ContentRect(),
				&overlayKeyColor, B_FOLLOW_NONE, 
				B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL
				| B_OVERLAY_TRANSFER_CHANNEL);
			if (overlayKeyColor != fOverlayKeyColor) {
				fOverlayKeyColor = overlayKeyColor;
				SetHighColor(fOverlayKeyColor);
				FillRect(ContentRect());
				Flush();
			}
		}

		fDisplayBitmap = bitmap;
		fUsingOverlay = true;
	} else {
		if (fUsingOverlay)
			ClearViewOverlay();
	
		if (bitmap)
			DrawBitmap(bitmap, ContentRect());	
		else
			Invalidate(ContentRect());

		fDisplayBitmap = bitmap;
		fUsingOverlay = false;
	}
	
	fWindow->Unlock();
}


void VideoView::MouseDown(BPoint point)
{
	if (fDisplayBitmap == 0)
		return;

	fBeginDrag = true;
	fClickPoint = point;
	if (fCapturedFrame == 0) {
		if (fDisplayBitmap->ColorSpace() == B_YCbCr422) {
			fCapturedFrame = new BBitmap(fDisplayBitmap->Bounds(), B_RGB_32_BIT);
			if (fCapturedFrame->IsValid())
				CopyBitmapYCbCr(fDisplayBitmap, fCapturedFrame);
		} else {
			fCapturedFrame = new BBitmap(fDisplayBitmap->Bounds(), B_RGB_32_BIT);
			CopyBitmap(fDisplayBitmap, fCapturedFrame);
		}
	} else {
		if (fDisplayBitmap->ColorSpace() == B_YCbCr422)
			CopyBitmapYCbCr(fDisplayBitmap, fCapturedFrame);
		else
			CopyBitmap(fDisplayBitmap, fCapturedFrame);
	}

	if (fCapturedFrame->IsValid() == false) {
		delete fCapturedFrame;
		fCapturedFrame = 0;
	}
}

void VideoView::MouseMoved(BPoint point, uint32, const BMessage *)
{
	if (fBeginDrag && abs((int) point.x - (int) fClickPoint.x) > kDragTolerance
		&& abs((int) point.y - (int) fClickPoint.y)) {
		fBeginDrag = false;
		if (fCapturedFrame == 0)
			return;
		
		BMessage dragMessage(B_SIMPLE_DATA);
		dragMessage.AddInt32("be:actions", B_COPY_TARGET);
		dragMessage.AddString("be:types", "application/octet-stream");
		dragMessage.AddString("be:clip_name", "video_frame");

		// Build a list of types that this can export.
		translator_id *translatorList;
		int32 numTranslators;
		BTranslatorRoster::Default()->GetAllTranslators(&translatorList, &numTranslators);
		for (int translator = 0; translator < numTranslators; translator++) {
			const translation_format *formatList;
			int32 numFormats;
	
			bool foundInput = false;
			if (BTranslatorRoster::Default()->GetInputFormats(translatorList[translator],
				&formatList, &numFormats) == B_OK) {
				for (int format = 0; format < numFormats; format++) {
					if (formatList[format].group == B_TRANSLATOR_BITMAP) {
						foundInput = true;
						break;
					}
				}
			}
	
			if (!foundInput)
				continue;
	
			BTranslatorRoster::Default()->GetOutputFormats(translatorList[translator],
				&formatList, &numFormats);
			for (int format = 0; format < numFormats; format++) {
				if (formatList[format].type != B_TRANSLATOR_BITMAP) {
					dragMessage.AddString("be:filetypes", formatList[format].MIME);
					dragMessage.AddString("be:type_descriptions", formatList[format].name);
					dragMessage.AddString("be:types", formatList[format].MIME);
				}
			}
		}
	
		delete [] translatorList;

		BRect bitmapBounds = fCapturedFrame->Bounds();
		float ratio = bitmapBounds.Height() / Bounds().Height();
		BPoint imgOffset(fClickPoint);
		imgOffset.x *= ratio;
		imgOffset.y *= ratio;
		if (bitmapBounds.Width() < 255.0 && bitmapBounds.Height() < 255.0) {
			// Drag a bitmap
			BBitmap *dragBitmap = new BBitmap(fCapturedFrame);
			DragMessage(&dragMessage, dragBitmap, B_OP_BLEND, imgOffset, this);
		} else {
			// Revert to dragging an outline if the bitmap is too large
			BRect dragRect(fCapturedFrame->Bounds());
			dragRect.OffsetTo(fClickPoint.x - imgOffset.x, fClickPoint.y - imgOffset.y);
			DragMessage(&dragMessage, dragRect, this);
		}
	}
}

void VideoView::MouseUp(BPoint)
{
	fBeginDrag = false;
}

void VideoView::MessageReceived(BMessage *message)
{
	if (message->what == B_COPY_TARGET) {
		if (fCapturedFrame == 0)
			return;
			
		const char *mimeType;

		if (message->FindString("be:types", &mimeType) != B_OK)
			return;	// Bad drag response

		entry_ref dirRef;
		const char *name;
		BDirectory dir;
		BFile file;
		BMallocIO mallocStream;
		BPositionIO *outData = NULL;
		
		bool writeToFile = strcasecmp(mimeType, B_FILE_MIME_TYPE) == 0;

		if (writeToFile) {
			if (message->FindString("be:filetypes", &mimeType) != B_OK)
				return;	// Bad drag response

			if (message->FindString("name", &name) != B_OK 
				|| message->FindRef("directory", &dirRef) != B_OK)
				return;	// Bad drag response

			dir.SetTo(&dirRef);
			file.SetTo(&dir, name, O_RDWR);
			if (file.InitCheck() != B_OK)
				return;	// Error saving to file

			outData = &file;
		} else 
			outData = &mallocStream;

		BBitmapStream strm(fCapturedFrame);
		fCapturedFrame = 0;	// Note that the BBitmapStream will delete the
							// bitmap when its through with it.
		translator_id translator;
		uint32 format;
		if (FindTranslator(mimeType, &translator, &format))
			BTranslatorRoster::Default()->Translate(translator,
				&strm, NULL, outData, format);
	
		if (writeToFile) {
			BPath tmp(&dir, name);
			update_mime_info(tmp.Path(), false, true, true);
		} else {
			BMessage reply(B_MIME_DATA);
			reply.AddData(mimeType, B_MIME_TYPE, mallocStream.Buffer(),
				mallocStream.BufferLength());

			message->SendReply(&reply);
		}
	} else
		BView::MessageReceived(message);
}

bool VideoView::FindTranslator(const char *mime, translator_id *out_translator, uint32 *out_format)
{
	translator_id *translatorList;
	int32 numTranslators;
	BTranslatorRoster::Default()->GetAllTranslators(&translatorList, &numTranslators);
	for (int translator = 0; translator < numTranslators; translator++) {
		const translation_format *formatList;
		int32 numFormats;

		bool foundInput = false;
		if (BTranslatorRoster::Default()->GetInputFormats(translatorList[translator],
			&formatList, &numFormats) == B_OK) {
			for (int format = 0; format < numFormats; format++) {
				if (formatList[format].group == B_TRANSLATOR_BITMAP) {
					foundInput = true;
					break;
				}
			}
		}

		if (!foundInput)
			continue;

		BTranslatorRoster::Default()->GetOutputFormats(translatorList[translator],
			&formatList, &numFormats);
		for (int format = 0; format < numFormats; format++) {
			if (strcmp(formatList[format].MIME, mime) == 0) {
				*out_translator = translatorList[translator];
				*out_format = formatList[format].type;
				return true;						
			}
		}
	}

	delete [] translatorList;

	return false;
}

void VideoView::CopyBitmap(BBitmap *srcBitmap, BBitmap *destBitmap)
{
	color_space inspace = srcBitmap->ColorSpace();
	color_space outspace = destBitmap->ColorSpace();
	uchar *src = (uchar*) srcBitmap->Bits();
	uchar *dest = (uchar*) destBitmap->Bits();
	for (int row = 0; row < srcBitmap->Bounds().Height(); row++) {
		convert_space(inspace,outspace,src,srcBitmap->BytesPerRow(),dest);
		dest += destBitmap->BytesPerRow();
		src += srcBitmap->BytesPerRow();
	}
}

inline uchar clip8(int a)
{
	uchar c;
	if (a > 255)
		c = 255;
	else if (a < 0)
		c = 0;
	else
		c = (uchar) a;
		
	return c;
} 

inline uchar YCbCr_red(uchar y, uchar, uchar cr)
{
	return clip8((int)(1.164 * (y - 16) + 1.596 * (cr - 128)));
}

inline uchar YCbCr_green(uchar y, uchar cb, uchar cr)
{
	return clip8((int)(1.164 * (y - 16) - 0.813 * (cr - 128) -
		0.392 * (cb - 128)));
}

inline uchar YCbCr_blue(uchar y, uchar cb, uchar)
{
	return clip8((int)(1.164 * (y - 16) + 2.017 * (cb - 128)));
}

//
//	The app server doesn't handle YCbCr422 bitmaps natively (yet).
//	Convert to a RGBA bitmap so the translation kit can read it.
//
void VideoView::CopyBitmapYCbCr(BBitmap *srcBitmap, BBitmap *destBitmap)
{
	uchar *srcRowStart = (uchar*) srcBitmap->Bits();
	uchar *destRowStart = (uchar*) destBitmap->Bits();
	uint height = (uint) srcBitmap->Bounds().Height() + 1;
	uint width = (uint) srcBitmap->Bounds().Width() + 1;
	for (uint row = 0; row < height; row++) {
		uchar *src = srcRowStart;
		uchar *dest = destRowStart;
		for (uint col = 0; col < width; col += 2) {
			uint y1 = *src++;
			uint cb = *src++;
			uint y2 = *src++;
			uint cr = *src++;

			*dest++ = YCbCr_blue(y1, cb, cr);
			*dest++ = YCbCr_green(y1, cb, cr);
			*dest++ = YCbCr_red(y1, cb, cr);
			dest++;
			
			if (col + 1 < width) {
				*dest++ = YCbCr_blue(y2, cb, cr);
				*dest++ = YCbCr_green(y2, cb, cr);
				*dest++ = YCbCr_red(y2, cb, cr);
				dest++;
			}
		}
		
		destRowStart += destBitmap->BytesPerRow();
		srcRowStart += srcBitmap->BytesPerRow();
	}
}


