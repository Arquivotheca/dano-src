#include <Be.h>
#include "GStatusWind.h"

#include "PackMessages.h"

#include "Util.h"
#include "MyDebug.h"

GStatusWindow::GStatusWindow(BRect rect, const char *title,bool needView)
	: BWindow(rect,title,B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE),
	  sview(NULL)
{
	BPoint	leftTop = rect.LeftTop();	
	MoveTo(-rect.Width()-10,-rect.Height()-10);

	if (needView && !sview) {
		Lock();
		BRect theRect = Bounds();
		theRect.OffsetTo(0,0);
		sview = new GStatusView(theRect,"statview");
		AddChild(sview);
		Unlock();
	}
	
	BWindow::Show();
	BWindow::Hide();
	MoveTo(leftTop);
}

BMessenger GStatusWindow::StatusMessenger()
{
	return BMessenger(sview);
}

void GStatusWindow::DoCancel(bool c)
{
	c;
	// perhaps send a message?
}

void GStatusWindow::Show()
{
	if (IsHidden()) BWindow::Show();
}

void GStatusWindow::Hide()
{
	if (!IsHidden()) BWindow::Hide();
}

void GStatusView::MessageReceived(BMessage *msg)
{	
	BStatusBar *sbar = (BStatusBar *)FindView("statusbar");
	bool canCancel;
	switch(msg->what) {
		/////////// general messages /////////////////
		case M_SETUP_STATUS:
			sbar->Reset(msg->FindString("message",0L),
						msg->FindString("message",1L));
			// force good redraw
			//sbar->Invalidate();
			sbar->SetMaxValue(msg->FindInt32("bytes"));
			canCancel = msg->FindBool("cancancel");
			
			((BButton *)FindView("cancel"))->SetEnabled(canCancel);
			Window()->Show();
			Window()->Activate();
			break;
		case M_UPDATE_PROGRESS:
			sbar->Update(msg->FindInt32("bytes"));
			break;
		case M_DONE:
			iCount = curCount = 0;
			
			((BButton *)FindView("cancel"))->SetEnabled(FALSE);
			// pause for 1/2 second
			snooze(1000*500);
			sbar->Reset("Idle",B_EMPTY_STRING);
			//sbar->Invalidate();
			Window()->Hide();
			break;
		case M_CANCEL :
			PRINT(("status window got cancel message\n"));
			sbar->Reset("Canceling...");
			//sbar->Invalidate();
			((BButton *)FindView("cancel"))->SetEnabled(FALSE);
			((GStatusWindow *)Window())->DoCancel(TRUE);  // virtual func
			break;
		default:
			Looper()->DispatchMessage(msg,Window());
	}
}

void GStatusView::AttachedToWindow()
{
	//SetViewColor(light_gray_background);
	((BButton *)FindView("cancel"))->SetTarget(this);
}


GStatusView::GStatusView(BRect frame, const char *name)
	: BView(frame,name,B_FOLLOW_NONE,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
	BStatusBar *sbView = 
		new BStatusBar(BRect(10,12,frame.Width()-10,26),"statusbar","Progress!","trailing");
	AddChild(sbView);	
	
	sbView->SetBarHeight(14.0);
	
	BRect r = Bounds();
	r.right -= 10;
	r.bottom -= 10;
	r.left = r.right - 80;
	r.top = r.bottom - 24;
	BButton *cancelBut = new BButton(r,"cancel",
		"Cancel",new BMessage(M_CANCEL),B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(cancelBut);
}
