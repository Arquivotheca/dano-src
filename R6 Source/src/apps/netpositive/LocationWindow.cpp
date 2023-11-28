// ===========================================================================
//	LocationWindow.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef JAVASCRIPT
#include "NPApp.h"
#include "URL.h"
#include "FindWindow.h"
#include "NetPositive.h"
#include "HTMLWindow.h"

#include <TextView.h>
#include <Button.h>

extern const char *kOpenButtonTitle;
extern const char *kOpenLocationWindowTitle;

static BWindow* sLocationWindow = NULL;
static BRect sLastPosition(100,300,380,380);


// ============================================================================
//	FindWindow is modeless...

#define LOCATIONBUTTON 'loca'

class LocationPanel : public BView {
public:
					LocationPanel(BRect rect);
	virtual	void	AttachedToWindow();
	virtual	void	MessageReceived(BMessage *msg);
	virtual	void	Draw(BRect updateRect);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	virtual void	DetachedFromWindow();
	
			void	DoLocation();

protected:
		BButton*	mLocationButton;
		BTextView*	mBTextView;
};


LocationPanel::LocationPanel(BRect rect) : BView(rect, "LocationPanel", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	BRect r = rect;
	r.bottom -= 30;
	r.InsetBy(8,8);
	BRect text = r;
	text.OffsetTo(B_ORIGIN);
	text.InsetBy(2,2);
	mBTextView = new DialogTextView(r,"BTextView",text,B_FOLLOW_ALL,B_WILL_DRAW);
	mBTextView->DisallowChar('\n');
	AddChild(mBTextView);
	
	r = rect;
	r.left = r.right - 100;
	r.top = r.bottom - 36;
	r.InsetBy(10,8);
	mLocationButton = new BButton(r,"LOCATIONBUTTON",kOpenButtonTitle, new BMessage(LOCATIONBUTTON),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	mLocationButton->SetEnabled(false);
	AddChild(mLocationButton);
}

void LocationPanel::AttachedToWindow()
{
	SetViewColor(216,216,216);
	Window()->SetDefaultButton(mLocationButton);
	mLocationButton->SetTarget(this);
	mBTextView->MakeFocus(true);
	BView::AttachedToWindow();
}

void LocationPanel::DetachedFromWindow()
{
	sLastPosition = Window()->Frame();
	sLocationWindow = NULL;
	BView::DetachedFromWindow();
}

void LocationPanel::Draw(BRect)
{
	BRect r = mBTextView->Frame();
	
	r.InsetBy(-1,-1);
	SetHighColor(128,128,128);
	MovePenTo(r.left,r.bottom);
	StrokeLine(BPoint(r.left,r.top));
	StrokeLine(BPoint(r.right,r.top));
	SetHighColor(240,240,240);
	MovePenTo(r.right,r.bottom);
	StrokeLine(BPoint(r.left,r.bottom));
	SetHighColor(0,0,0);
}

//	Send copy to html view?

void LocationPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case LOCATIONBUTTON:
			DoLocation();
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void LocationPanel::KeyDown(const char *, int32)
{
	int32 length = mBTextView->TextLength();
	bool enabled = mLocationButton->IsEnabled();

	if (length > 0 && !enabled)
		mLocationButton->SetEnabled(true);
	else if (length == 0 && enabled)
		mLocationButton->SetEnabled(false);
}

//	Send the find message to the nearest html window

void LocationPanel::DoLocation()
{
	mBTextView->SelectAll();
	BString url = mBTextView->Text();
	
	if (ValidateURL(url) == false)		// add http if it is missing....
		return;
	
	if (gPreferences.FindBool("DesktopMode")) {
		HTMLWindow *window = HTMLWindow::FrontmostHTMLWindow();
		if (window) {
			BMessage msg(B_NETPOSITIVE_OPEN_URL);
			msg.AddString("be:url", url.String());
			window->PostMessage(&msg);
		}
	} else
		NetPositive::NewWindowFromURL(url);
	
	if (Window()->QuitRequested())
		Window()->Close();
}


// ============================================================================


void OpenLocationWindow()
{
	if (sLocationWindow) {
		sLocationWindow->Activate();
		return;
	}
	sLocationWindow = new FFMWarpingWindow(sLastPosition, kOpenLocationWindowTitle, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
	BRect r = sLastPosition;
	r.OffsetTo(0,0);
	LocationPanel *panel = new LocationPanel(r);
	sLocationWindow->AddChild(panel);
	sLocationWindow->Show();
}

#endif
