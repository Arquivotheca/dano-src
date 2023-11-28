/*
 * NetronConfigApp.cpp
 */

#include <app/Application.h>
#include <interface/Region.h>
#include <interface/Screen.h>
#include <interface/Window.h>
#include <interface/View.h>
#include <stdio.h>
#include "NetronConfigView.h"

const rgb_color kBlack = { 0, 0, 0, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };

//----------------------------------------------------------------------

class NetronBackgroundView : public BView
{
public:
						NetronBackgroundView(BRect frame);
	virtual				~NetronBackgroundView();
	
	virtual	void		Draw(BRect update);
	
	void				FillRegionProperly(BRegion* region,
						 pattern aPattern);
};

//----------------------------------------------------------------------

class NetronConfigWindow : public BWindow
{
public:
						NetronConfigWindow(bool floating = true);
	virtual				~NetronConfigWindow();

private:

	NetronBackgroundView*	fBackground;
	NetronConfigView*		fConfig;
};


//----------------------------------------------------------------------

class NetronConfigApp : public BApplication
{
public:
						NetronConfigApp(bool debug = false);
	virtual				~NetronConfigApp();
	
private:

	NetronConfigWindow*		fWindow;
};

//----------------------------------------------------------------------

NetronBackgroundView::NetronBackgroundView(BRect frame)
	: BView(frame, "netron_background", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	SetLowColor(kBlack);
	SetHighColor(kWhite);
	SetViewColor(B_TRANSPARENT_COLOR);
}

NetronBackgroundView::~NetronBackgroundView()
{

}
	
void NetronBackgroundView::Draw(BRect update)
{
	BRect bounds(Bounds());
	BRect r, r2;
	BRegion clip;
	
	// draw 2 pixel wide white border

	r = bounds;
	r2 = r;
	r2.InsetBy(2.0f, 2.0f);
	clip.Set(r);
	clip.Exclude(r2);
	if (clip.Intersects(update)) FillRegionProperly(&clip, B_SOLID_HIGH);
	
	// draw an inscribed 20 pixel wide black rectangle
	
	r = bounds;
	r.InsetBy(2.0f, 2.0f);
	r2 = r;
	r2.InsetBy(20.0f, 20.0f);
	clip.Set(r);
	clip.Exclude(r2);
	if (clip.Intersects(update)) FillRegionProperly(&clip, B_SOLID_LOW);
	
	// fill the rest of the background with white
	
	r = bounds;
	r.InsetBy(2.0f, 2.0f);
	r2 = r;
	r2.InsetBy(20.0f, 20.0f);
	
	if (r2.Intersects(update)) FillRect(r2, B_SOLID_HIGH);
}

//----------------------------------------------------------------------

// It would appear that FillRegion() doesn't work, so I had to
// implement it myself.

void NetronBackgroundView::FillRegionProperly(BRegion* region,
 pattern aPattern)
{
	int32  i;
	BRect  rect;
	int32  rects;
	
	rects = region->CountRects();
	
	for (i = 0; i < rects; i++)
	  {
	    rect = region->RectAt(i);
	    FillRect(rect, aPattern);
	  }  
}
 

//----------------------------------------------------------------------

NetronConfigWindow::NetronConfigWindow(bool floating)
	: BWindow(	BRect(0, 0, 10, 10), "Display Adjustment Utility",
				floating ? B_NO_BORDER_WINDOW_LOOK : B_TITLED_WINDOW_LOOK,
				floating ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL,
				B_ASYNCHRONOUS_CONTROLS | B_NOT_CLOSABLE |
				B_WILL_ACCEPT_FIRST_CLICK),
	  fBackground(NULL),
	  fConfig(NULL)
{
	RemoveShortcut('Q', B_COMMAND_KEY);	// Remove quit shortcut
	RemoveShortcut('W', B_COMMAND_KEY);	// Remove close shortcut

	BRect r(BScreen().Frame());
	if (!floating) {
		r.InsetBy(5.0f, 5.0f);
		r.top += 20.0f;	
	}
	Zoom(r.LeftTop(), r.Width(), r.Height());

	r.OffsetTo(B_ORIGIN);
	fBackground = new NetronBackgroundView(r);
	AddChild(fBackground);

	
	// inset config view by the amount in the specification, but
	// don't let it be squished too small (which should never happen
	// on the Netron's 800x1024 screen)
	float xInset = 150.0f;
	float yInset = 200.0f;
	if ((r.Width() - (xInset * 2.0f)) < 400.0f) {
		xInset = (r.Width() - 400.0f) / 2.0f;
	}
	if ((r.Height() - (yInset * 2.0f)) < 400.0f) {
		yInset = (r.Height() - 400.0f) / 2.0f;
	}

	r.InsetBy(xInset, yInset);
	fConfig = new NetronConfigView(r);
	fBackground->AddChild(fConfig);
}

NetronConfigWindow::~NetronConfigWindow()
{

}

//----------------------------------------------------------------------

NetronConfigApp::NetronConfigApp(bool debug)
	: BApplication("application/x-vnd.Be.NetronConfig")
{
	fWindow = new NetronConfigWindow(!debug);
	fWindow->Show();
}

NetronConfigApp::~NetronConfigApp()
{

}

//----------------------------------------------------------------------

int main(int argc, char *argv[])
{
	bool debug = false;
	if ((argc > 1) && (strcmp(argv[1], "--debug") == 0)) {
		printf("--debug parameter found, entering debug mode\n");
		debug = true;
	}
	NetronConfigApp *app = new NetronConfigApp(debug);
	app->Run();
	
	return 0;
}

