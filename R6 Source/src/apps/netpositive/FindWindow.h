// ===========================================================================
//	FindWindow.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef FINDWINDOW_H
#define FINDWINDOW_H

#include <Window.h>
#include <TextView.h>

#include "NPApp.h"

class FindPanel;

// ============================================================================
//	Floating find window, just one of them.....

class FindWindow : public FFMWarpingWindow {
friend class FindPanel;
public:
					FindWindow();
	virtual			~FindWindow();

	static void		FindAgain(BWindow *window);
	static void		Find(BWindow *window);
	static bool		IsFindWindowOpen() {return mFindWindow;}
	static void		Close() {if (mFindWindow) mFindWindow->PostMessage(B_QUIT_REQUESTED);}
	
protected:
	static void		DoFind(BWindow *window, const char *text);
	
	static	FindWindow*	mFindWindow;
			FindPanel*	mFindPanel;
	static	BRect		mLastPosition;
};


class DialogTextView : public BTextView {
public:
					DialogTextView(BRect frame, const char *name, BRect textRect, uint32 resizingMode, uint32 flags);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	virtual void	MouseDown(BPoint point);
	virtual	void	InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns);
	virtual	void	DeleteText(int32 fromOffset, int32 toOffset);
};

// ============================================================================

#endif
