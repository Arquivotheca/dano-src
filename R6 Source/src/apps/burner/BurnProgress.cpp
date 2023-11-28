//
// BurnProgress.cpp
//

#include <Bitmap.h>
#include <String.h>
#include <stdio.h>
#include "BurnProgress.h"
#include "CDTrack.h"
#include "GfxUtils.h"

const rgb_color kLineColor		= {0, 0, 0, 255};
const rgb_color kBurnedColor	= {140, 255, 154, 255};
const rgb_color kEmptyColor		= {63, 184, 77, 255};
const rgb_color kTrackTextColor	= {0, 0, 0, 255};


BurnProgress::BurnProgress(BRect frame, CDTrack *head, uint32 resizingMode)
	: BView(frame, "BurnProgress", resizingMode,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
{
	fPercentComplete = 0.0f;
	fHeadTrack = head;
	fBuffer = NULL;
	fOffscreen = NULL;
	fBufferValid = false;
	SetViewColor(B_TRANSPARENT_COLOR);
}


BurnProgress::~BurnProgress()
{
	if (fBuffer != NULL) {
		fBuffer->Lock();
		if (fOffscreen != NULL) {
			fOffscreen->RemoveSelf();
		}
		fBuffer->Unlock();
		delete fOffscreen;
		delete fBuffer;
	}
}

void BurnProgress::SetPercentComplete(float percent)
{
	if (percent < 0.0f) {
		percent = 0.0f;
	} else if (percent > 100.0f) {
		percent = 100.0f;
	}
	if (percent != fPercentComplete) {
		fPercentComplete = percent;
		InvalidateBuffer();
	}
}


void BurnProgress::FrameResized(float width, float height)
{
	CreateBuffer(width, height);
}

void BurnProgress::AttachedToWindow()
{
	BRect bounds(Bounds());
	CreateBuffer(bounds.Width(), bounds.Height());
		// initializes fBuffer, fOffscreen, fBarRect, fBufferValid
//	SetBackgroundColor();
}

void BurnProgress::DetachedFromWindow()
{
	if (fBuffer != NULL) {
		fBuffer->Lock();
		if (fOffscreen != NULL) {
			fOffscreen->RemoveSelf();
		}
		fBuffer->Unlock();
		delete fOffscreen;
		delete fBuffer;
		fOffscreen = NULL;
		fBuffer = NULL;
	}	
}

void BurnProgress::Draw(BRect update)
{
	if (!fBufferValid) {
		DrawBuffer();
	}
	DrawBitmap(fBuffer, update, update);
}

void BurnProgress::DrawBuffer()
{
	if (fBuffer != NULL && fBuffer->Lock()) {
		BRect bounds(fOffscreen->Bounds());
		fOffscreen->PushState();
		fOffscreen->FillRect(bounds, B_SOLID_LOW);
		fOffscreen->SetHighColor(kLineColor);
		fOffscreen->StrokeRect(fBarRect);
		BRect rect(fBarRect);
		rect.InsetBy(1, 1);

		int32 total(0), trackCount(0);
		CDTrack *track = fHeadTrack;
		// find total frames
		while (track != NULL) {
			total += track->Length();
			trackCount++;
			track = track->Next();
		}
		
		float width = rect.Width() - trackCount - 2;
		// fill bar with colors based on percent complete
		BRect fillRect(rect);
		fillRect.right = fillRect.left + (fPercentComplete * width);
		fOffscreen->SetHighColor(kBurnedColor);
		fOffscreen->FillRect(fillRect, B_SOLID_HIGH);
		fillRect.left = fillRect.right + 1;
		fillRect.right = rect.right;
		fOffscreen->SetHighColor(kEmptyColor);
		fOffscreen->FillRect(fillRect, B_SOLID_HIGH);
		
		track = fHeadTrack;
		rect.right = rect.left;
		// draw a rectangle for each track into the bar
		font_height fh;
		fOffscreen->GetFontHeight(&fh);
		while (track != NULL) {
			int32 len = track->Length();
			rect.left = rect.right + 1;
			rect.right = rect.right + ceil(((float)len / (float)total) * width);
	
			char buf[3];
			sprintf(buf, "%d", track->Index());
			float strWidth = fOffscreen->StringWidth(buf);
			if (strWidth <= (rect.Width() - 4)) {
				fOffscreen->SetDrawingMode(B_OP_OVER);
				fOffscreen->SetHighColor(kTrackTextColor);
				fOffscreen->DrawString(buf, BPoint(floor(rect.left + (rect.Width() - strWidth) / 2.0) + 1,
					floor(rect.top + fh.ascent + fh.leading)));
				fOffscreen->SetDrawingMode(B_OP_COPY);
			}
			
			rect.right += 1;
			fOffscreen->SetHighColor(kLineColor);
			fOffscreen->StrokeLine(rect.RightTop(), rect.RightBottom(), B_SOLID_HIGH);
						
			track = track->Next();
		}

		// draw text below the bar
		BString label("00:00"); // left label
		float h = ceil(fh.ascent + fh.leading);
		fOffscreen->SetHighColor(kLineColor);
		fOffscreen->DrawString(label.String(), BPoint(fBarRect.left, fBarRect.bottom + h + 1));
		char buf[8];	
		label.SetTo(B_EMPTY_STRING);
		sprintf(buf, "%02ld:%02ld", (total / (60 * 75)), (total % (60 * 75)) / 75);
		label << buf; // right label
		fOffscreen->DrawString(label.String(),
			BPoint(ceil(fBarRect.right - fOffscreen->StringWidth(label.String())),
			fBarRect.bottom + h));
		label.SetTo(B_EMPTY_STRING);
		// draw "PP.P % complete" label in the middle
		sprintf(buf, "%.1f", fPercentComplete * 100);
		label << buf << "% complete";
		fOffscreen->DrawString(label.String(), BPoint(fBarRect.left + (fBarRect.Width() / 2)
			- (fOffscreen->StringWidth(label.String()) / 2), fBarRect.bottom + h + 1));


		fOffscreen->PopState();
		fOffscreen->Sync();
		fBufferValid = true;
		fBuffer->Unlock();
	}
}


void BurnProgress::InvalidateBuffer()
{
	fBufferValid = false;
//	Invalidate();
	Draw(Bounds());
}

void BurnProgress::CreateBuffer(float width, float height)
{
	BRect rect(0, 0, width, height);
	if (fBuffer != NULL) {
		fBuffer->Lock();
		if (fOffscreen != NULL) {
			fOffscreen->RemoveSelf();
			delete fOffscreen;
		}
		fBuffer->Unlock();
		delete fBuffer;
	}
	fBuffer = new BBitmap(rect, B_RGBA32, true);
	fBuffer->Lock();
	fOffscreen = new BView(rect, NULL, B_FOLLOW_ALL_SIDES, 0);
	fBuffer->AddChild(fOffscreen);
	SetBackgroundColor();

	BFont font;
	GetFont(&font);
	font.SetFace(B_REGULAR_FACE);
	font.SetSize(10);
	fOffscreen->SetFont(&font);
	font_height fh;
	font.GetHeight(&fh);
	
	fBarRect.Set(rect.left + 2, rect.top, rect.right - 2, rect.top + 12);

	fBuffer->Unlock();
	fBufferValid = false;
}

void BurnProgress::SetBackgroundColor()
{
	BView *parent = Parent();
	if (parent) {
		rgb_color col = parent->ViewColor();
		fBuffer->Lock();
		if (col != B_TRANSPARENT_COLOR) {
			fOffscreen->SetLowColor(col);
		} else {
			fOffscreen->SetLowColor(parent->LowColor());
		}
		fBuffer->Unlock();
	}
}
