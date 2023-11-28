#include <Be.h>
// StatusWindow.cpp

#include "StatusWindow.h"
#include "PackMessages.h"

#include "Util.h"
#include "MyDebug.h"

#include "CopyFile.h"

StatusWindow::StatusWindow(const char *title, bool *_cancel)
	:	GStatusWindow(BRect(0,0,250,90),title,FALSE),
		cancelVar(_cancel)
{
	Lock();
	
	PositionWindow(this,0.5,0.4);
	
	sview = new StatusView(Bounds(),"statview");
	AddChild(sview);
	Unlock();
}

void StatusView::SetupStatus(BMessage *msg)
{
	PRINT(("sending setup status to base class\n"));
	msg->what = M_SETUP_STATUS;
	GStatusView::MessageReceived(msg);
}

void StatusWindow::DoCancel(bool c)
{
	*cancelVar = c;
}

void StatusView::MessageReceived(BMessage *msg)
{
	BStatusBar *sbar = (BStatusBar *)FindView("statusbar");
	
//	PRINT(("status view got message\n"));
	
	switch(msg->what) {
			////////// specific messages //////////////
		case M_ADD_PATCH:
			SetupStatus(msg);
			break;
		case M_UPDATE_TEMP:
			msg->AddString("message","Updating Temporary File   ");
			msg->AddBool("cancancel",FALSE);
			SetupStatus(msg);
			break;
		case M_ADD_ITEMS:
			iCount = msg->FindInt32("item count");
			curCount = 0;
			sprintf(status," of %d",iCount);
			
			msg->AddString("message","Compressing:  ");
			msg->AddString("message",status);
			msg->AddBool("cancancel",TRUE);
			SetupStatus(msg);
			break;
		case M_EXTRACT_ITEMS:
			iCount = msg->FindInt32("item count");
			curCount = 0;
			sprintf(status," of %d",iCount);
			
			msg->AddString("message","Decompressing:  ");
			msg->AddString("message",status);
			msg->AddBool("cancancel",TRUE);
			SetupStatus(msg);
			break;
		case M_REMOVE_ITEMS:
			// update remaining values
			
			iCount = msg->FindInt32("item count");
			curCount = 0;
			
			sprintf(status," of %d",iCount);
			
			msg->AddString("message","Deleting:  ");
			msg->AddString("message",status);
			msg->AddBool("cancancel",FALSE);
			SetupStatus(msg);
			break;
		case M_BUILD_INSTALLER:
			msg->AddString("message","Building Installer   ");
			msg->AddBool("cancancel",FALSE);
			SetupStatus(msg);
			break;
		case M_WRITE_CATALOG:
			Window()->Show();
			sbar->Reset("Updating catalog",B_EMPTY_STRING);
			//sbar->Invalidate();
			sbar->Update(sbar->MaxValue(),B_EMPTY_STRING);
			break;
		case M_CURRENT_FILENAME:
			curCount++;
			
			sprintf(status,"%d",curCount);
			sbar->Update(0,msg->FindString("filename"),status);
			break;
		default:
			GStatusView::MessageReceived(msg);
	}
}

StatusView::StatusView(BRect frame, const char *name)
	: GStatusView(frame,name)
{
}


// ===========================================================
/*****

void	GrayStringView::Draw(BRect up)
{
	// 3.0 for baseline
	MovePenTo(Bounds().left, Bounds().bottom - 3.0);
	DrawString(Text());
}

void	GrayStringView::AttachedToWindow()
{
	SetViewColor(light_gray_background);
	SetFontName("Erich");
	SetLowColor(light_gray_background);
	SetHighColor(0,0,0);
}

void ProgressBarView::Draw(BRect update)
{

	BRect area = Bounds();
	SetHighColor(0,0,0);
	StrokeRect(area);
	area.InsetBy(1.0,1.0);
	//SetHighColor(208,208,250);
	//FillRect(area);
	
	if (fVal > fMax)
		fVal = fMax;
	if (fVal != fMin) {
		float pct = (float)fVal/(float)fMax;
		area.right = area.Width()*pct + area.left;
		SetHighColor(100,100,150);
		//PRINT(("status bar rect is "));
		//area.PrintToStream();
		FillRect(area);
	}
}

void ProgressBarView::AttachedToWindow()
{
	SetViewColor(208,208,250);
} ***/
