// ===========================================================================
//	URLView.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef URL_VIEW_H
#define URL_VIEW_H

#include <TextControl.h>

class URLTextControl;

// ===========================================================================
//	Bar at top of window that hold url

class URLView : public BView {
public:
					URLView(BRect frame, BHandler *target, int resizeFlags);

virtual void		Draw(BRect updateRect);

	void			SetButtonEnabled(uint32 button_msg, bool enable);
	
	virtual void	MouseDown(BPoint where);
	virtual void	MessageReceived(BMessage *msg);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	void			SetDownloading(bool downloading);
	void			AddButton(uint32 msgConst, const char *url, float *right, float hSpacing, bool enabled,
							  const char *upImg, const char *downImg, const char *disabledImg);
protected:
		void		InitData();

protected:
		URLTextControl*	mURLText;
		float			mLastWidth;
		BBitmap*		mPageIcon;
		BHandler*		mTarget;
		thread_id		mAnimationThread;
		BBitmap*		mBGBitmap;
};

#endif
