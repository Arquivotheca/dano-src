// NameDialog.cpp

#include "NameDialog.h"
#include "LabelView.h"
#include "Util.h"
#include "MyDebug.h"

#include <ClassInfo.h>
#include <MessageFilter.h>
#include <TextControl.h>
#include <Button.h>

class KeyFilter : public BMessageFilter
{
public:
				KeyFilter()
					:	BMessageFilter(B_KEY_DOWN)
				{
				}
	virtual		filter_result Filter(BMessage *m, BHandler **h)
				{
					m;
					BTextView *tv;
					if ((tv = cast_as(*h,BTextView)))
					{
						// check the text length
						(*h)->Looper()->PostMessage('CTxt');
					}
					return B_DISPATCH_MESSAGE;
				}
};


NameDialog::NameDialog(BRect frame,const char *label,
		const char *text, BMessage *model, BHandler *target,
		const char *helpText)
	: BWindow(frame,"dialog",B_MODAL_WINDOW, B_NOT_MOVABLE)
{
	Lock();
	
	fTarget = target;
	fModelMessage = model;
	/////////////////////////
	
	PositionWindow(this,0.5,0.333);
	
	/////////////////////////
	BView *gray = new BView(Bounds(),"background",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(gray);
	
	gray->SetViewColor(light_gray_background);

	/////////////////////////
	BRect r = Bounds();
	
	r.InsetBy(10,10);
	
	if (helpText) {
		r.bottom = r.top + 28;
		gray->AddChild(new LabelView(r,helpText));
		r.top = r.bottom;
		r.bottom = r.top;
	}
	r.top += 5;
	r.bottom = r.top + 20;
	r.right -= 2;
	BTextControl *tc = new BTextControl(r,"Text",label,text,
		new BMessage(B_QUIT_REQUESTED));
	
	gray->AddChild(tc);
	tc->TextView()->AddFilter(new KeyFilter());

	tc->SetDivider(r.Width()/3.0);
	// select the default text
	tc->MakeFocus(true);
		
	r = Bounds();
	r.InsetBy(10,10);
	r.left = r.right - 70;
	r.top = r.bottom - 20;
	
	BMessage *mes = new BMessage(B_QUIT_REQUESTED);
	mes->AddBool("canceled",false);
	BButton *btn = new BButton(r,"ok","OK",mes);
	btn->MakeDefault(TRUE);
	gray->AddChild(btn);
	btn->SetEnabled(text && *text);

	// move sideways to the left
	r.right = r.left - 14;
	r.left = r.right - 70;
	
	mes = new BMessage(B_QUIT_REQUESTED);
	mes->AddBool("canceled",true);
	btn = new BButton(r,"cancel","Cancel",mes);
	gray->AddChild(btn);		

	Show();
	Unlock();
}

bool NameDialog::QuitRequested()
{
	BMessage *msg = CurrentMessage();
	
	if (msg->FindBool("canceled") == false) {
		BTextControl *tc = (BTextControl *)(FindView("Text"));
		
		fModelMessage->AddString("text",tc->Text());
		
		if (fTarget) {
			fTarget->Looper()->PostMessage(fModelMessage,fTarget);
			delete fModelMessage;
		}
	}
	return TRUE;
}

void	NameDialog::MessageReceived(BMessage *msg)
{
	if (msg->what ==  'CTxt') {
		BTextControl *tc = (BTextControl *)(FindView("Text"));
		BControl *cntl = (BControl *)FindView("ok");
		if (cntl)
			cntl->SetEnabled(*tc->Text());
	}
	else
		BWindow::MessageReceived(msg);	
}
