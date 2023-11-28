#ifndef __ZIP_O_WINDOW__
#define __ZIP_O_WINDOW__

#include <Box.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>


#include "ZipOEngine.h"

class BarberPoleView;

class OffscreenBitmap {
	// a utility class for setting up offscreen bitmaps
public:
	OffscreenBitmap(BRect bounds);
	OffscreenBitmap();
	~OffscreenBitmap();

	BView *BeginUsing(BRect bounds);
	void DoneUsing();
	BBitmap *Bitmap() const;
		// blit this to your view when you are done rendering
	BView *View() const;
		// use this to render your image in
private:
	void NewBitmap(BRect frame);
	BBitmap *bitmap;
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

class ZipOView : public BBox {
public:
	ZipOView(BRect frame, bool oneShotOnly);
	virtual ~ZipOView();

	bool StopIfNeeded();
		// returns true if stopped or idle
	ZipOEngine *Engine()
		{ return &engine; }

protected:
	virtual void MessageReceived(BMessage *);
	virtual void AttachedToWindow();
	virtual void Pulse();

	void Stop();
	void Start(BMessage *);
	void Done();
private:
	ZipOEngine engine;
	BButton *stop;
	BarberPoleView *barberPole;
	BStringView *titleText;
	BStringView *progressText;
	
	BString stateString;
	BString progressString;

	bool shouldQuitWhenDone;

	typedef BBox _inherited;
};

class ZipOWindow : public BWindow {
public:
	ZipOWindow(BPoint offset, bool oneShotOnly);

	bool Quitting() const
		{ return quitting; }

protected:
	virtual void MessageReceived(BMessage *);
	virtual bool QuitRequested();

private:
	ZipOView *panel;
	bool quitting;

	typedef BWindow _inherited;
};

#endif
