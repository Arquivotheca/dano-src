// ReplaceDialog.cpp

#include "UpdateDialog.h"
#include "DoIcons.h"
#include "LabelView.h"
#include "ValetMessage.h"
#include "PackageItem.h"
#include "ManagerListView.h"
#include "UpdateThread.h"
#include "NameDialog.h"
#include "ServicesDialog.h"
#include <ClassInfo.h>
#include <Control.h>
#include <StatusBar.h>
#include <String.h>
#include <Invoker.h>


#include "Util.h"
#define MODULE_DEBUG 0
#include "MyDebug.h"

void	EnableView(BView *parent, bool enable);
void	WaitForWindow(BWindow *w);

UpdateDialog::UpdateDialog(ManagerListView *_lv )
	:	StatusDialog("Checking the server for updates to the selected packages..."),
		invoker(NULL),
		lv(_lv),
		updtThread(NULL),
		updateCount(0)
{
	// move this into message received so we are not blocked

	Lock();
	
	// in this method we use lv, the parent window listView
	// it is ok since this dialog is modal
	// but we should be wary in the future
	int 	low, hi;
	ASSERT(lv);
	low = lv->LowSelection();
	hi = lv->HighSelection();
	for (long i = low; i <= hi && low >= 0; i++) {
		PackageItem *curItem = (PackageItem *)lv->ItemAt(i);
		if (!curItem->selected)
			continue;
		if (curItem->ValetSupported() && curItem->data.FindBool("upservice"))
		{
			BMessage response;
			
			const char *sid = curItem->data.FindString("sid");
			const char *pid = curItem->data.FindString("pid");
			const char *vid = curItem->data.FindString("vid");

			bool haveSerialID = (sid && *sid);	
			bool havePrefixID = (pid && *pid);
			bool haveVersionID = (vid && *vid);
			bool oldSerial = (sid && !strcmp(sid,"old"));
			
			if (!haveSerialID || oldSerial) {
				
				if (!haveVersionID) {
					// can't register/update no info to send up
					if (low == hi)
						doError("The package %s does not contain the necessary information for updating.\n\n\
Either the package developer did not enter this information or there was an error when the package \
was downloaded.",curItem->data.FindString("package"));
					
					continue;
				}
				
				// no prefix id needed, the server will find this from version id
				// this may be a bug MKK 3/25/98
				if (oldSerial) {
					sid = B_EMPTY_STRING;
				}
				else {
					pid = B_EMPTY_STRING;
				}
				// check if commercial or shareware
				int32 type = curItem->data.FindInt32("softtype");
				if (type == PackageItem::SOFT_TYPE_COMMERCIAL)
//					|| type == PackageItem::SOFT_TYPE_SHAREWARE)
				{
					if (low == hi) {
						BString buf;
						buf << "To update \"" << curItem->data.FindString("package")
							<< "\", you need to enter your product serial number.";
							
						response.MakeEmpty();
						BRect wr(0,0,300,100);
						BWindow *w = new NameDialog(wr,"Serial Number",
							B_EMPTY_STRING, &response, NULL, buf.String());
					
						WaitForWindow(w);
						sid = response.FindString("text");
						if (!sid)
							continue;
						if (!*sid) {
							// they didn't type anything in
						}
					}
					else {
						continue;
					}
				}
				else {
					// we may get a serial back from the server???
				}
//				if () {
//					
//		 			updateCount = 0;
//		 			break;
//				}
			}
			else {
				// we have a serial id that we know to be associated with a
				// version id in the database so we don't pass it up
				// changed MKK 3/17/98
				// vid = B_EMPTY_STRING;
				// but we do need a prefixID
				if (!havePrefixID) {
					// can't register/update no prefix id
					if (low == hi) {
						doError("The package %s does not contain the necessary information for updating. \
 Either the package developer did not enter the information or there was an error when the package \
 was installed or downloaded.", curItem->data.FindString("package"));
					}
					continue;
				}
			}

			// add the data
			data.AddRef("refs",&curItem->fileRef);
			data.AddString("sid",sid);
			data.AddString("pid",pid);
			data.AddString("vid",vid);
			
			updateCount++;
			PRINT(("adding pkgname %s\n",curItem->data.FindString("package")));
		}
		else if (low == hi) {
			doError("Updating is not supported for the package \"%s\".",
				curItem->data.FindString("package"));
 			updateCount = 0;
 			break;
		}
	}
	if (updateCount > 0) {
		PostMessage(PKG_UPDATE);
	}
	else {
		delete this;
		return;
	}
	
	Unlock();
	Show();
}

/***
UpdateDialog::~UpdateDialog()
{
	delete invoker;
}

status_t UpdateDialog::Go(BInvoker *i)
{
	invoker = i;
	Show();
	
	PostMessage(PKG_UPDATE);
	
	return B_NO_ERROR;
}
****/

void UpdateDialog::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		//case M_RADIO_SET:
		//replOption = msg->FindInt32("choice");
		//	break;
		case F_STATUS_ERROR: {
			updtThread = NULL;
			StatusDialog::MessageReceived(msg);
			break;
		}
		case B_CANCELED: {
			PRINT(("cancel it!\n"));
			if (updtThread) {
				PRINT(("interrupting net\n"));
				updtThread->htconn.Interrupt();
				updtThread = NULL;
			}
			else
				PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case PKG_UPDATE: {
			// ok button clicked, go ahead and check for update
			
			// in this method we use lv, the parent window listView
			// it is ok since this dialog is modal
			// but we should be wary in the future
			if (updateCount > 0) {
				// disable the view			
				EnableView(ChildAt(0), false);
				
				// set up the quit button
				BView	*v = FindView("cancel");
				BControl *c;
				if (c = cast_as(v,BControl)) {
					c->SetEnabled(true);
				}
				// set up the status bar
				BStatusBar *bar = (BStatusBar *)FindView("status");
				if (bar) {
					bar->SetMaxValue(3.0 * updateCount);
					// bar->Show();
				}
				
				BMessenger tmp(this);
				updtThread = new UpdateThread(tmp, &data);
				updtThread->Run();
			}
			else {
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default: {
			if (msg->HasBool("quit") && msg->FindBool("quit"))
				updtThread = NULL;
			StatusDialog::MessageReceived(msg);
			break;
		}
	}
}

bool UpdateDialog::QuitRequested()
{
	if (invoker) {
		BMessage	mess(*invoker->Message());
//		mess.AddInt32("option",replOption);
		invoker->Invoke(&mess);
	}
	return StatusDialog::QuitRequested();
}

///////////////////////////  UpdateView ///////////////////////////

#if 0
UpdateView::UpdateView(	BRect frame )
	:	BView(frame,"replview",B_FOLLOW_ALL,B_WILL_DRAW)
{	
}

void UpdateView::Draw(BRect up)
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

void UpdateView::AttachedToWindow()
{
	SetViewColor(light_gray_background);

	BRect r = Bounds();
	
	// Add TextView
	r.InsetBy(10,10);
	r.bottom = r.top + 34;
	
	BRect tr = r;
	tr.left += 31 + 10;
	
	BTextView *v = new LabelView(tr,"Checking the server for updates to the selected packages...");
	AddChild(v);
	//BRect textRect = tr;
	//textRect.OffsetTo(0,0);
	//textRect.InsetBy(2,2);
	// abstract this
	//BTextView *alertView = new BTextView(tr,"alertview",textRect,
	//				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,B_WILL_DRAW);

	//AddChild(alertView);
	//alertView->SetFont(be_plain_font);
	// alertView->SetFontSize(12);	
	//alertView->SetViewColor(light_gray_background);
	//alertView->SetLowColor(light_gray_background);

	//alertView->SetWordWrap(TRUE);
	//alertView->MakeEditable(FALSE);
	//alertView->MakeSelectable(FALSE);
	
	//alertView->SetText(replText);
	
	// Add Radio Box
	
	/****
	r.top = r.bottom + 8;
	const int kButtonSep = 22;
	r.bottom = r.top + kButtonSep*4 + 10;
	
	BBox	*radioBox = new BBox(r,"radiobox",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(radioBox);
	radioBox->SetLabel("Options");
	
	// Add Radio buttons
	
	BRect br;
	br = radioBox->Bounds();
	br.InsetBy(14,24);
	br.bottom = br.top + kButtonSep;
	
	BMessage	*rMsg = new BMessage(M_RADIO_SET);
	rMsg->AddInt32("choice",UPDATE_checkOnly);
	BRadioButton	*rbtn = new BRadioButton(br,B_EMPTY_STRING,"Check for update only",rMsg);
	radioBox->AddChild(rbtn);
	rbtn->SetValue(UPDATE_checkOnly == replOption);
	
	br.top = br.bottom;
	br.bottom = br.top + kButtonSep;
	rMsg = new BMessage(M_RADIO_SET);
	rMsg->AddInt32("choice",UPDATE_checkDl);
	rbtn = new BRadioButton(br,B_EMPTY_STRING,"Check for update and download if newer",rMsg);
	radioBox->AddChild(rbtn);
	rbtn->SetValue(UPDATE_checkDl == replOption);
		
	br.top = br.bottom;
	br.bottom = br.top + kButtonSep;
	rMsg = new BMessage(M_RADIO_SET);
	rMsg->AddInt32("choice",UPDATE_checkForceDl);
	rbtn = new BRadioButton(br,B_EMPTY_STRING,"Check for update and always download",rMsg);
	radioBox->AddChild(rbtn);
	rbtn->SetValue(UPDATE_checkForceDl == replOption);
	
	// Add CheckBox
//	r.top = r.bottom + 8;
//	r.bottom = r.top + 14;
	
//	BCheckBox	*cb = new BCheckBox(r,B_EMPTY_STRING,"Don't ask again, use this current setting",
//									new BMessage(M_DONT_ASK),
//									B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
//	AddChild(cb);
	
	// Add Continue Button
	***/
	
	r = Bounds();
	r.InsetBy(100,12);
	r.top = r.bottom - 24;
//	r.left = r.right - 120;

//	BButton *cont = new BButton(r,"continue","Start",new BMessage(PKG_UPDATE),
//									B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
//	AddChild(cont);
//	cont->MakeDefault(TRUE);
	
//	r.right = r.left - 20;
//	r.left = r.right - 120;
	BMessage *canMsg = new BMessage(B_CANCELED);
	// canMsg->AddBool("cancel",TRUE);
	BButton *btn = new BButton(r,"cancel","Cancel",canMsg,B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	
	AddChild(btn);
	
	r = Bounds();
	r.InsetBy(12,12);
	
	r.top = r.bottom - 24 - 34;
	r.bottom = r.top + 20;
	
	BStatusBar *bar = new BStatusBar(r,"status");
	AddChild(bar);
	bar->SetBarHeight(8.0);
	// bar->Hide();
}
#endif
