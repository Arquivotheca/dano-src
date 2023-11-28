#ifndef __CHECK_BFS__
#define __CHECK_BFS__


#include <Application.h>
#include <String.h>
#include <Window.h>
#include <View.h>

#include "Engine.h"

class BarberPoleView;
class BBitmap;
class Engine;

const int32 kBitmapWidth = 196;
const int32 kBitmapHeight = 13;
const color_space kBitmapColorSpace = B_COLOR_8_BIT;

extern const unsigned char kBitmapBits1[];
extern const unsigned char kBitmapBits2[];
extern const unsigned char kBitmapBits3[];
extern const unsigned char kBitmapBits4[];
extern const unsigned char kBitmapBits5[];
extern const unsigned char kBitmapBits6[];
extern const unsigned char kBitmapBits7[];

extern const unsigned char kDropFilesBitmapBits[];

class OffscreenBitmap {
	// simple offscreen for easy flicker free drawing
	// - all the drawing operations can be done identically to
	// the corresponding draws done directly in the target bitmap
public:
	OffscreenBitmap();
	OffscreenBitmap(BView *target, BRect frame);
		// frame is the portion of target that you wish to buffer
	~OffscreenBitmap();

	BView *BeginBlitting(BView *target, BRect destFrame,
		bool copyTargetViewState = true, bool erase = true);
		// pass true in <copyTargetViewState> if <target> view state
		// changed since last time we used it
	void DoneBlitting();
		// when you are done, call this to blit the result into your target
		// view

	BView *OffscreenView() const;
		// do your drawing into this

private:
	void SetToCommon(BView *, BRect);
	void SetUpState(BView *, BRect);
	
	BBitmap *bitmap;
	BView *target;
	BRect destinationFrame;
};


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

class SeparatorLine : public BView {
public:
	SeparatorLine(BPoint , float , bool vertical, const char *name = "");
	virtual	void Draw(BRect bounds);
};

class CheckBFSView : public BBox {
public:
	CheckBFSView(BRect frame);
	virtual ~CheckBFSView();

	bool StopIfNeeded();
		// returns true if stopped or idle
	Engine *TargetEngine()
		{ return &engine; }

	void Stop();
	void Start(const char *volume);
	void Done();

protected:
	virtual void MessageReceived(BMessage *);
	virtual void AttachedToWindow();
	virtual void Pulse();

private:
	Engine engine;
	BButton *stop;
	BarberPoleView *barberPole;
	BStringView *titleText;
	BStringView *progressText;
	
	BString stateString;
	BString progressString;

	typedef BBox _inherited;
};

const int32 kBarberPoleSize = kBitmapWidth * kBitmapHeight;
const int32 kBevel = 2;

class BarberPoleView : public BView {
public:
	BarberPoleView(BRect, const char *, const unsigned char *const *barberPoleArray,
		int32 barberPoleCount, const unsigned char *dropHereBits);

	void SetPaused();
	void SetProgressing();
	void SetWaitingForDrop();

protected:
	virtual	void Draw(BRect);
	virtual void Pulse();

	void DrawCurrentBarberPole();
private:
	BBitmap bitmap;	
	const unsigned char *const *bits;
	const unsigned char *dropHereBits;
	int32 count;
	int32 indx;
	bool progress;
	bool paused;
};

class CheckBFSWindow : public BWindow {
public:
	CheckBFSWindow(BPoint offset, const char *volume);

	bool Quitting() const
		{ return quitting; }

protected:
	virtual void MessageReceived(BMessage *);
	virtual bool QuitRequested();

private:
	CheckBFSView *panel;
	volatile bool quitting;

	typedef BWindow _inherited;
};


class CheckBFSApp : public BApplication {
public:
	CheckBFSApp();

protected:
	virtual void MessageReceived(BMessage *);
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void ReadyToRun();

private:

	BString volume;

	typedef BApplication _inherited;
};

#endif
