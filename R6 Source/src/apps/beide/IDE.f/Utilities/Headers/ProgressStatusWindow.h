// ---------------------------------------------------------------------------
/*
	ProgressStatusWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
				
	Pieces of ProgressStatusWindow liberally taken from Zip-o-Matic by Pavel Cisler

	Create a ProgressStatusWindow as an MThread is starting, give it
		the worker thread
		a title
	During the work, give it a
		current status
	
	If the stop button is pressed, the worker thread will be cancelled
	


*/
// ---------------------------------------------------------------------------

#ifndef _PROGRESSSTATUS_H
#define _PROGRESSSTATUS_H

#include "BarberPoleView.h"

#include <Window.h>
#include <View.h>
#include <StringView.h>
#include <Box.h>

class ProgressStatusView;
class OffscreenBitmap;
class FlickerFreeStringView;
class SeparatorLine;
class MThread;

// ---------------------------------------------------------------------------
//	Class ProgressStatusWindow
// ---------------------------------------------------------------------------

class ProgressStatusWindow : public BWindow {
public:
						ProgressStatusWindow(BPoint offset,
											 const char* taskTitle,
											 MThread* workerThread);
	virtual				~ProgressStatusWindow();
	
	virtual void		TaskStarted();
	virtual void		StatusUpdate(const char* status);
	virtual void		TaskDone();

	virtual void 		MessageReceived(BMessage* message);
	
private:
	ProgressStatusView*	fPanel;
	MThread*			fWorkerThread;

};

// ---------------------------------------------------------------------------
// Class ProgressStatusView
// ---------------------------------------------------------------------------

class ProgressStatusView : public BBox {
public:
					ProgressStatusView(BRect frame, const char* taskTitle);
	virtual			~ProgressStatusView();

	virtual void	Start();
	virtual void	Stop();
	virtual void	SetStatus(const char* statusString);

protected:
	virtual void	AttachedToWindow();


private:	
	BButton*		fStopButton;
	BarberPoleView*	fBarberPole;
	BStringView*	fTitleText;
	BStringView*	fProgressText;
};


// ---------------------------------------------------------------------------
// Class OffscreenBitmap
// a utility class for setting up offscreen bitmaps
// ---------------------------------------------------------------------------

class OffscreenBitmap {
public:
	OffscreenBitmap(BRect bounds);
	OffscreenBitmap();
	~OffscreenBitmap();

	BView *BeginUsing(BRect bounds);
	void DoneUsing();

	// blit this to your view when you are done rendering
	BBitmap *Bitmap() const;

	// use this to render your image in
	BView *View() const;

private:
	void NewBitmap(BRect frame);
	BBitmap *bitmap;
};

// ---------------------------------------------------------------------------
// class FlickerFreeStringView
// ---------------------------------------------------------------------------

class FlickerFreeStringView : public BStringView {
public:
	FlickerFreeStringView(BRect bounds, const char *name, 
		const char *text, uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW);
	virtual ~FlickerFreeStringView();
	virtual void AttachedToWindow();
	virtual void SetViewColor(rgb_color color);
	virtual void SetLowColor(rgb_color color);
	
	virtual void Draw(BRect);

private:
	void Draw(BRect, bool direct);
	
	OffscreenBitmap *bitmap;
	rgb_color viewColor;
	rgb_color lowColor;
};

// ---------------------------------------------------------------------------
// class SeparatorLine
// ---------------------------------------------------------------------------

class SeparatorLine : public BView {
public:
	SeparatorLine(BPoint , float , bool vertical, const char *name = "");
	virtual	void Draw(BRect bounds);
};

#endif
