// ===========================================================================
//	ProgressView.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef PROGRESS_VIEW_H
#define PROGRESS_VIEW_H

#include "UResource.h"
#include "BeDrawPort.h"
#include <View.h>
#include <String.h>

//=======================================================================

#define PROG_MSG		'prog'
#define PROG_STR		'prgs'
#define PROG_LOCK		'prlk'
#define PROG_CLR		'prcl'

// ===========================================================================
//	Bar at bottom of window for progress

class ProgressView : public BView {
public:
				ProgressView(BRect frame, char *name, uint32 resizeMask = B_FOLLOW_BOTTOM, bool showPadlock = false);
virtual			~ProgressView();

		void		InitData();

		void	SetStatus(StoreStatus status, long done, long of);
		void	SetStatusStr(const char* str);
		const char *GetStatus() {return mStr.String();}
		const char *GetDefaultStatus() {return mDefaultStatus.String();}
		void	SetDefaultStatus(const char *status);
		void	UpdateBar(StoreStatus status);
		void	ClearStatus();
		
		void	SetStr(const char* newStr);
virtual	void	MessageReceived(BMessage *msg);
virtual void	FrameResized(float new_width, float new_height);

virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
virtual	void	MouseDown(BPoint point);

protected:

		void	GetBarRect(BRect &r);
		
		int			mTextH;		// Place to draw progress string....
		StoreStatus	mStatus;
		long		mDone;
		long		mOf;
		
		bigtime_t	mBeginTime;
		long		mBeginBytes;
		bigtime_t	mLastTime;
		BString		mExtraStatusData;
		
		BString		mStr;
		bool		mShowPadlock;
		bool		mPadlockState;
		BRect		mBounds;
		CachedFont*	mPlainFont;
		
		BString		mDefaultStatus;
};

#endif
