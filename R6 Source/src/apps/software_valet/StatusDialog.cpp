#include "StatusDialog.h"
#include "Util.h"
#include "DoIcons.h"
#include "LabelView.h"
#include <StatusBar.h>
#include <string.h>
#include <malloc.h>
#include <Button.h>



StatusDialog::StatusDialog(const char *intxt)
	:	BWindow(BRect(0,0,300,120),B_EMPTY_STRING,
				B_MODAL_WINDOW,
				B_NOT_RESIZABLE | B_NOT_MOVABLE)
{
	Lock();
	PositionWindow(this,0.5,0.25);
	AddChild(new StatusDialogView(Bounds(),intxt));
	Unlock();
}

void StatusDialog::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case F_STATUS_UPDATE: {
			BStatusBar *bar = (BStatusBar *)FindView("status");
			float amt = msg->FindFloat("amount");
			const char *txt = msg->FindString("message");
			if (bar) bar->Update(amt,txt);
			
			if (msg->HasBool("quit") && msg->FindBool("quit")) {
				snooze(1000*250);
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case F_STATUS_ERROR: {
			BStatusBar *bar = (BStatusBar *)FindView("status");
			
			const char *err = msg->FindString("message");
			if (!err)
				err = "Error checking for update";
			
			if (bar) bar->Update(INT_MAX,err);
			snooze(1000*250);
			doError(err);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case B_CANCELED: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


StatusDialogView::StatusDialogView(BRect frame, const char *intxt)
	:	BView(frame,"sdview",B_FOLLOW_ALL,B_WILL_DRAW),
		txt(NULL)
{
	if (intxt) txt = strdup(intxt);
}

StatusDialogView::~StatusDialogView()
{
	free(txt);
}

void StatusDialogView::Draw(BRect up)
{
	BView::Draw(up);
	
	BRect r = Bounds();
	r.InsetBy(10,10);
	r.right = r.left + 31;
	r.bottom = r.top + 31;
	
	SetLowColor(255,255,255);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(gYellowWarnIcon,r);
	SetDrawingMode(B_OP_COPY);
}

void StatusDialogView::AttachedToWindow()
{
	SetViewColor(light_gray_background);

	BRect r = Bounds();
	
	// Add TextView
	r.InsetBy(10,8);
	r.bottom = r.top + 14*4;
	
	BRect tr = r;
	tr.left += 31 + 10;
	
	BTextView *v = new LabelView(tr,txt ? txt : B_EMPTY_STRING);
	AddChild(v);
	
	r = Bounds();
	r.InsetBy(100,8);
	r.top = r.bottom - 24;

	BMessage *canMsg = new BMessage(B_CANCELED);
	BButton *btn = new BButton(r,"cancel","Cancel",canMsg,B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	
	AddChild(btn);
	
	r = Bounds();
	r.InsetBy(12,12);
	
	r.top = r.bottom - 24 - 30;
	r.bottom = r.top + 20;
	
	BStatusBar *bar = new BStatusBar(r,"status");
	AddChild(bar);
	bar->SetBarHeight(8.0);
}
