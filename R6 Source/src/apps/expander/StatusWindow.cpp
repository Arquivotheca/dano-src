#define DEBUGON 1
#include "StatusWindow.h"

#define TEST 1

TExpanderStatusWindow::TExpanderStatusWindow(void)
  : BWindow(	BRect(0,0,1,1), "Expander Status", B_TITLED_WINDOW,
		B_FRAME_EVENTS | B_NOT_ZOOMABLE | B_WILL_DRAW,
		B_CURRENT_WORKSPACE)
{
	SetSizeLimits(kStatusWindWidth,kStatusMaxWindWidth,kStatusWindHeight,kStatusWindHeight);
  	MoveTo(kStatusWindXLoc,kStatusWindYLoc);
	ResizeTo(kStatusWindWidth,kStatusWindHeight);

	AddParts();  
 }

TExpanderStatusWindow::~TExpanderStatusWindow(void)
{
}

void TExpanderStatusWindow::MessageReceived(BMessage *msg)
{
  switch (msg->what) {
	default:
		be_app->PostMessage(msg);
  }
}

bool TExpanderStatusWindow::QuitRequested()
{
	if (be_app->CountWindows() == 1) {
  		be_app->PostMessage(B_QUIT_REQUESTED);
  	}
  	
  	return true;
}

void TExpanderStatusWindow::AddParts(void)
{
	BRect visibleRect;
	
	visibleRect.Set(0,0,Bounds().Width(),Bounds().Height());
  	fBackdrop = new BView(visibleRect,"backdrop",B_FOLLOW_ALL,B_WILL_DRAW);
	fBackdrop->SetViewColor(203,203,203);
	this->AddChild(fBackdrop);
	
	visibleRect.Set(5.0,5.0,Bounds().Width()-5,22);
	fMsg = new BStringView(visibleRect,"msg","",B_FOLLOW_TOP | B_FOLLOW_LEFT | B_FOLLOW_RIGHT);
	fMsg->SetViewColor(220,220,220);
	fBackdrop->AddChild(fMsg);
}

void TExpanderStatusWindow::SetMsg(char *msg)
{
	if (Lock()) {
		fMsg->SetText(msg);
		Unlock();
	}
}

bool TExpanderStatusWindow::DropInMsg(BPoint pt)
{
	bool retval=false;
	
	if (Lock()) {
		fMsg->ConvertFromScreen(&pt);
		retval = fMsg->Frame().Contains(pt);
		Unlock();
	}
	
	return retval;
}
