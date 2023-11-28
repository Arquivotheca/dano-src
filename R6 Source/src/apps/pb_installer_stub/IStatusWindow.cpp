// IStatusWindow.cpp
#include <Button.h>
#include "IStatusWindow.h"
#include "InstallMessages.h"
#include "InstallPack.h"

#include "Util.h"
#include "MyDebug.h"
#include <string.h>

StatusWindow::StatusWindow(BRect rect, const char *title)
	: BWindow(rect,title,B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE)
{
	Lock();
	
	MoveTo(-rect.Width()-10,-rect.Height()-10);
			
	StatusView *sview = new StatusView(Bounds(),"statview");
	AddChild(sview);

	BWindow::Show();
	BWindow::Hide();
	PositionWindow(this,0.5,0.5);
	Unlock();	
}

void StatusView::MessageReceived(BMessage *msg)
{	
	switch(msg->what) {
		case M_UPDATE_PROGRESS: {
			sbar->Update(msg->FindInt32("bytes"));
			BMessage	repM(B_CANCELED);
			repM.AddBool("canceled",canceled);
			msg->SendReply(&repM);
			
			if (canceled) {
				PRINT(("status window got cancel message\n"));
				sbar->Reset("Canceling...");
				sbar->Invalidate();
			}
			break;
		}
		case M_EXTRACT_ITEMS:			
			msg->what = M_GENERAL_TASK;
			msg->AddString("text","Installing: ");
			
			((BButton *)FindView("cancel"))->SetEnabled(TRUE);
			MessageReceived(msg);
			canceled = false;
			break;
		case M_GENERAL_TASK:
			iCount = msg->FindInt32("item count");
			curCount = 0;
			if (iCount)
				sprintf(status," of %d",iCount);
			else
				strcpy(status,B_EMPTY_STRING);
			
			Window()->Show();	
			Window()->Activate();
			sbar->Reset(msg->FindString("text"),status);
			sbar->Invalidate();
			sbar->SetMaxValue(msg->FindInt32("bytes"));
			break;
		case M_CURRENT_FILENAME: {
			curCount++;
	
			sprintf(status,"%d",curCount);
			
			sbar->Update(0,msg->FindString("filename"),status);			
			break;
		}
		case M_DONE:
			iCount = curCount = 0;
			sbar->Update(0,"Finished",B_EMPTY_STRING);
			
			// pause for 1 sec.
			snooze(1000*500);
			
			((BButton *)FindView("cancel"))->SetEnabled(FALSE);
			Window()->Hide();
			sbar->Reset("Idle",B_EMPTY_STRING);
			canceled = false;
			break;
		case M_CANCEL: {
			canceled = true;
			//PRINT(("status window got cancel message\n"));
			//((StatusWindow *)Window())->packFile->doCancel = TRUE;
			((BButton *)FindView("cancel"))->SetEnabled(FALSE);
			break;
		}
	}
}

void StatusWindow::Show()
{
	if (IsHidden())
		BWindow::Show();
}

void StatusWindow::Hide()
{
	if (!IsHidden())
		BWindow::Hide();
}

void StatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(light_gray_background);
	((BButton *)FindView("cancel"))->SetTarget(this);
}


StatusView::StatusView(BRect frame, const char *name)
	: BView(frame,name,B_FOLLOW_NONE,B_WILL_DRAW)
{
	sbar = new BStatusBar(BRect(10,12,frame.Width()-10,26),"statusbar","Progress!","trailing");
	AddChild(sbar);
	
	sbar->SetBarHeight(14.0);
	
	BRect r = Bounds();
	r.right -= 10;
	r.bottom -= 10;
	r.left = r.right - 80;
	r.top = r.bottom - 24;
	BButton *cancelBut = new BButton(r,"cancel",
		"Cancel",new BMessage(M_CANCEL),B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(cancelBut);
	
	canceled = true;
}
