// ===========================================================================
//	ProgressView.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "ProgressView.h"
#include "Strings.h"
#include "Translation.h"
#include <Bitmap.h>
#include <Dragger.h>
#include <stdio.h>

//#define THERMOMETER

extern const char *kStatusBeginning;
extern const char *kStatusDownloading;
extern const char *kStatusLoadingHTML;
extern const char *kStatusLoadingImages;
extern const char *kStatusDNS;
extern const char *kStatusConnecting;
extern const char *kStatusRequestSent;
extern const char *kBytesPerSecLabel;
extern const char *kBytesPerSecKBLabel;
extern const char *kRemainingLabelSecs;
extern const char *kRemainingLabelMins;
extern const char *kRemainingLabelHrs;

#ifndef NOSSL
static BBitmap *sClosedPadlock = 0;
const uint32 kPadlockWidth = 13;
const uint32 kPadlockHeight = 13;
#endif

//=======================================================================
//	View to draw at the bottom of the window

ProgressView::ProgressView(BRect rect, char *name, uint32 resizeMask, bool showPadlock)
	: BView(rect, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
	mStatus(kIdle), mDone(-1), mOf(-1), mShowPadlock(showPadlock), mPadlockState(false)
{
	InitData();
#ifndef NOSSL
	if (!sClosedPadlock) {
		sClosedPadlock = TranslateGIF("netpositive:lock.gif");
	}
#endif
}

void ProgressView::InitData()
{
#ifndef NOSSL
	if (mShowPadlock && mPadlockState)
		mTextH = 6 + kPadlockWidth;
	else
#endif
		mTextH = 6;		// Offset from left edge to start of progress text / url info
	mExtraStatusData = " ";
}


ProgressView::~ProgressView()
{
}

void ProgressView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(216,216,216);
	SetLowColor(216,216,216);
	mBounds = Bounds();
	mPlainFont = BeDrawPort::GetFontFromBFont(be_plain_font);
	mPlainFont->SetViewFont(this, false);
}

void ProgressView::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
	mBounds = Bounds();
}


//	Place to draw the progress bar

void ProgressView::GetBarRect(BRect &r)
{
	r = mBounds;
	r.InsetBy(3,3);
	r.left = r.right - 64;
}

void ProgressView::Draw(BRect updateRect)
{
	BRect r = mBounds;
	SetHighColor(152,152,152);
	MovePenTo(r.left,r.top);
	StrokeLine(BPoint(r.right,r.top));
	SetHighColor(255,255,255);
//	MovePenTo(r.left,r.bottom-1);
//	StrokeLine(BPoint(r.left,r.top+1));
	MovePenTo(BPoint(r.left, r.top+1));
	StrokeLine(BPoint(r.right,r.top+1));
	SetHighColor(0,0,0);
	
#ifndef NOSSL
	if (mShowPadlock) {
		if (mPadlockState) {
			mTextH = 6 + kPadlockWidth;
			DrawBitmap(sClosedPadlock,BRect(0,0,kPadlockWidth - 1,kPadlockHeight - 1 + 1),BRect(r.left,r.top + 1, r.left+kPadlockWidth - 1,r.top + 2 + kPadlockHeight - 1));
		} else
			mTextH = 6;
	}
#endif
	
	if (mStr.Length() && updateRect.right > mTextH)
		DrawString(mStr.String(),BPoint(r.left + mTextH,r.bottom - 2));
#ifdef THERMOMETER
	if (mOf > mDone) {
		BRect therm = r;
		therm.InsetBy(2,0);
		therm.top += 2;
		therm.bottom -= 1;
		SetHighColor(128, 128, 255);
		SetLowColor(255,255,255);
		therm.right = therm.left + (therm.Width() * (float)mDone / (float)mOf);
		SetDrawingMode(B_OP_SELECT);
		FillRect(therm);
		SetDrawingMode(B_OP_COPY);
		SetHighColor(0,0,0);
		SetLowColor(255,255,255);
	}
#endif
}

void ProgressView::MouseDown(BPoint)
{
}

//	Copy difference from newStr into oldstr, return the number of equal chars

int StrDiff(const char *oldStr, const char* newStr)
{
	int i = 0;
	while (newStr[i] == oldStr[i] && newStr[i])	//	Only set the differences in the text
		i++;
	return i;
}

// (relative anchors, image map positions only redraw differences)
// Could do more just to show differences....
	
void ProgressView::SetStr(const char* newStr)
{
/*
	BRect r = Frame();
	if (r.left > 0 && BDragger::AreDraggersDrawn() == false) {
		MoveTo(0, r.top);
		ResizeTo(r.Width() + 8, r.Height());
	} else if (r.left == 0 && BDragger::AreDraggersDrawn() == true) {
		MoveTo(8, r.top);
		ResizeTo(r.Width() - 8, r.Height());
	}
*/

	if (newStr == NULL)
		return;
	if (mStr.Length() == 0)
		mStr = " ";
	int count = mStr.Length();
	int i = StrDiff(mStr.String(),newStr);
	mStr = newStr;
	if (i == 0 || count != i) {
		BRect r = mBounds;
		r.InsetBy(2,2);
		r.bottom++;
		r.left = mTextH + mPlainFont->TextWidth(mStr.String(),MIN(i,count), false);	// Only update what changed...
		Invalidate(r);
	}
}

void ByteSizeStr(int size, BString& sizeStr);

void FormatBPSStr(BString& str, int bytesPerSec, long done, long of, bool printRemaining, bool abbreviate)
{
	char numstr[256];

	if (bytesPerSec < 10000)
		sprintf(numstr,abbreviate ? kBytesPerSecAbbrevLabel : kBytesPerSecLabel, bytesPerSec);
	else
		sprintf(numstr,abbreviate ? kBytesPerSecKBAbbrevLabel : kBytesPerSecKBLabel, (float)bytesPerSec / 1024.0);
		
	str += numstr;
	
	if (of != 0 && printRemaining && bytesPerSec != 0) {
		long secsRemaining = (of - done) / bytesPerSec;
		if (secsRemaining < 60)
			sprintf(numstr, abbreviate ? kRemainingLabelSecsAbbrev : kRemainingLabelSecs, secsRemaining);
		else if (secsRemaining < 3600)
			sprintf(numstr, abbreviate ? kRemainingLabelMinsAbbrev : kRemainingLabelMins, secsRemaining / 60, secsRemaining % 60);
		else
			sprintf(numstr, abbreviate ? kRemainingLabelHrsAbbrev : kRemainingLabelHrs, secsRemaining / 3600, (secsRemaining % 3600 / 60), secsRemaining % 60);
		
		str += numstr;
	}
}

void FormatProgressStr(BString& str, long done, long beginBytes, long of, int bytesPerSec, bool printOf, bool printPercent, bool printRemaining, bool abbreviate)
{
	if (done > 0) {
		BString sizeStr;
		ByteSizeStr(done,sizeStr);
		str += sizeStr;
	}
	
	if (of > 0 && printOf) {
		BString sizeStr;
		ByteSizeStr(of,sizeStr);
		if (done > 0)
			str += "/";
		str += sizeStr;
	}

	if (done > beginBytes) {
		if (bytesPerSec > 0) {
			str += " (";

			char numstr[255];

			if (of != 0 && printPercent) {
				sprintf(numstr,"%d", (int)(((float)done/(float)of) * 100));
				str += numstr;
				str += "%, ";
			}
			
			FormatBPSStr(str, bytesPerSec, done, of, printRemaining, abbreviate);
			
			str += ")";
		}		
	}

}

void ProgressView::UpdateBar(StoreStatus)
{
	BString str;
	switch (mStatus) {
		default:
		case kIdle:
			char statusStr[256];
			sprintf(statusStr, kStatusBeginning, mExtraStatusData.String());
			str = statusStr;
			break;
		case kLoadingHTML:
			str = kStatusLoadingHTML;
			break;
		case kLoadingImages:
			str = kStatusLoadingImages;
			break;
		case kInProgress:
			str = kStatusDownloading;
			break;
	}
	
	bigtime_t now = system_time();
	if (now >= mLastTime + ONE_SECOND) {
		if (mDone > mBeginBytes) {
			bigtime_t now = system_time();
			float secs = (float)(now - mBeginTime) / 1000000.0;
			int bytesPerSec = 0;
			if (secs > 4)
				bytesPerSec = (int)((float)(mDone - mBeginBytes) / secs);
			FormatProgressStr(str, mDone, mBeginBytes, mOf, bytesPerSec, false, true, true, false);
		}
	}
	mLastTime = now;
	SetStr(str.String());
}

void ProgressView::SetStatus(StoreStatus status, long done, long of)
{
	if (mStatus != status) {
		mBeginTime = system_time();
		mLastTime = mBeginTime;
		// Since we've first started keeping track of bytes/sec now,
		// don't count the bytes we've already recevied since we don't
		// know when we first started getting them.  Only count what we
		// get from here on out.
		mBeginBytes = done;
	}
#ifdef THERMOMETER
	BRect r = mBounds;
	r.InsetBy(2,0);
	r.top += 2;
	r.bottom -= 1;
	float left = r.left;
	float width = r.Width();
	r.left = left + (width * (float)mDone / (float)mOf);
	r.right = left + (width * (float)done / (float)mOf);
	Invalidate(r);
#endif
	mStatus = status;
	mDone = done;
	mOf = of;

	switch (mStatus) {
		case kDNS: {
			char statusStr[256];
			sprintf(statusStr, kStatusDNS, mExtraStatusData.String());
			SetStr(statusStr);
			break;
		}
		case kConnect: {
			char statusStr[256];
			sprintf(statusStr, kStatusConnecting, mExtraStatusData.String());
			SetStr(statusStr);
			break;
		}
		case kRequest: {
			char statusStr[256];
			sprintf(statusStr, kStatusRequestSent, mExtraStatusData.String());
			SetStr(statusStr);
			break;
		}
			
		case kIdle:
		case kLoadingHTML:
		case kLoadingImages:
		case kInProgress:
			UpdateBar(status);	// Draw the progress bar
			break;
			
		default:
			ClearStatus();
	}
}

void ProgressView::ClearStatus()
{
	SetStr(mDefaultStatus.String());
}

void ProgressView::SetDefaultStatus(const char *status)
{
	bool needsUpdate = (mStr == mDefaultStatus);
	mDefaultStatus = status;
	if (needsUpdate)
		SetStr(mDefaultStatus.String());
}

// Don't flip status/anchor when a lookup is in progress
		
void ProgressView::SetStatusStr(const char* str)
{
	if (str == NULL)
		return;
	SetStr(str);
	mStatus = kIdle;
}

void ProgressView::MessageReceived(BMessage *msg)
{
	mExtraStatusData = " ";
	switch (msg->what) {
		case PROG_STR:
			SetStatusStr(msg->FindString("str"));
			break;
		case PROG_MSG: {
			StoreStatus status = (StoreStatus)msg->FindInt32("status");
			long done = msg->FindInt32("done");
			long of = msg->FindInt32("of");
			if (status == kDNS || status == kConnect) {
				const char *data;
				if (msg->FindString("Host", &data) == B_OK && data && *data) {
					mExtraStatusData = data;
				}
			} else if (status == kRequest || status == kIdle) {
				const char *data;
				if (msg->FindString("Request", &data) && data && *data == B_OK) {
					mExtraStatusData = data;
				}
			}
			
			if (mStatus != status || mDone != done || mOf != of)
				SetStatus(status,done,of);
			break;
		}
		case PROG_CLR:
			ClearStatus();
			break;
#ifndef NOSSL
		case PROG_LOCK: {
			mPadlockState = msg->FindBool("locked");
			Invalidate();
		}
#endif

		default:
			BView::MessageReceived(msg);
	}
}
