// ReplaceDialog.cpp
#include <Control.h>
#include <Box.h>
#include <RadioButton.h>
#include <Button.h>
#include <CheckBox.h>
#include "ReplaceDialog.h"
#include "DestManager.h"
#include "FSIcons.h"
#include "LabelView.h"

#include "Util.h"
#include "MyDebug.h"


ReplaceDialog::ReplaceDialog(	int32	*_replOption,
								bool	*_applyToAll,
								char	*_replText,
								uchar	_type)
	:	BWindow(BRect(0,0,300,230), B_EMPTY_STRING,
				B_MODAL_WINDOW,
				B_NOT_RESIZABLE),
		replOption(_replOption),
		applyToAll(_applyToAll),
		type(_type)
{
	Lock();
	
	PositionWindow(this,0.5,0.25);
	AddChild(new ReplaceView(Bounds(),
						*_replOption,
						_replText,
						_type));
	
	Unlock();
}

status_t ReplaceDialog::Go()
{
	long threadReturn;
	Show();
	wait_for_thread(Thread(),&threadReturn);
	
	return B_NO_ERROR;
}

void ReplaceDialog::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_RADIO_SET:
			*replOption = msg->FindInt32("choice");
			break;
		case M_DONT_ASK:
			BControl *cntl;
			msg->FindPointer("source",(void **)&cntl);
			*applyToAll = cntl->Value() == B_CONTROL_ON;
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool ReplaceDialog::QuitRequested()
{
	BMessage *msg = CurrentMessage();
	if (msg->HasBool("canceled") && msg->FindBool("canceled"))
		*replOption = B_CANCELED;
		
	return BWindow::QuitRequested();
}

///////////////////////////  ReplaceView ///////////////////////////

ReplaceView::ReplaceView(	BRect frame,
							short	_replOption,
							char	*_replText,
							uchar _type)
	:	BView(frame,"replview",B_FOLLOW_ALL,B_WILL_DRAW),
		replOption(_replOption),
		replText(_replText),
		type(_type)
{	
}

void ReplaceView::Draw(BRect up)
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

void ReplaceView::AttachedToWindow()
{
	SetViewColor(light_gray_background);

	BRect r = Bounds();
	
	// Add TextView
	r.InsetBy(10,10);
	r.bottom = r.top + 52;
	
	BRect tr = r;
	tr.left += 31 + 10;
	// abstract this
	
	BTextView *alertView = new LabelView(r,replText);
	AddChild(alertView);
	
	// Add Radio Box
	
	r.top = r.bottom + 8;
	r.bottom = r.top + 20*4 + 10;
	
	BBox	*radioBox = new BBox(r,"radiobox",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(radioBox);
	radioBox->SetViewColor(light_gray_background);
	radioBox->SetLowColor(light_gray_background);
	radioBox->SetLabel("Options");
	
	// Add Radio buttons
	char *replaceLabel(NULL), *renameLabel(NULL), *skipLabel(NULL);

	switch (type) {
	case ASK_IF_VERSION:
		replaceLabel = "Replace newer version with older";
		renameLabel = "Rename newer version and install older";
		break;
	case ASK_IF_CREATION:
		replaceLabel = "Replace newer item with older";
		renameLabel = "Rename newer item and install older";
		break;
	case ASK_IF_MODIFICATION:
		replaceLabel = "Replace newer item with older";
		renameLabel = "Rename newer item and install older";
		break;
	case ASK_ALWAYS:
		// fall through	
	default:
		replaceLabel = "Replace existing item";
		renameLabel = "Rename existing item";	
		break;
	}
	skipLabel = "Skip this item";

	BRect br;
	br = radioBox->Bounds();
	br.InsetBy(12,20);
	
	br.bottom = br.top + 20;
	
	BMessage	*rMsg = new BMessage(M_RADIO_SET);
	rMsg->AddInt32("choice",askUserREPLACE);
	BRadioButton	*rbtn = new BRadioButton(br, B_EMPTY_STRING, replaceLabel, rMsg);
	radioBox->AddChild(rbtn);
	rbtn->SetValue(askUserREPLACE == replOption);
	
	br.top = br.bottom;
	br.bottom = br.top + 20;
	rMsg = new BMessage(M_RADIO_SET);
	rMsg->AddInt32("choice",askUserRENAME);
	rbtn = new BRadioButton(br,B_EMPTY_STRING, renameLabel, rMsg);
	radioBox->AddChild(rbtn);
	rbtn->SetValue(askUserRENAME == replOption);
		
	br.top = br.bottom;
	br.bottom = br.top + 20;
	rMsg = new BMessage(M_RADIO_SET);
	rMsg->AddInt32("choice",askUserSKIP);
	rbtn = new BRadioButton(br,B_EMPTY_STRING, skipLabel, rMsg);
	radioBox->AddChild(rbtn);
	rbtn->SetValue(askUserSKIP == replOption);
	
	// Add CheckBox
	r.top = r.bottom + 8;
	r.bottom = r.top + 14;
	
	BCheckBox	*cb = new BCheckBox(r,B_EMPTY_STRING,"Don't ask again, use this current setting",
									new BMessage(M_DONT_ASK),
									B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(cb);
	
	// Add Continue Button
	
	r = Bounds();
	r.InsetBy(12,12);
	r.left = r.right - 120;
	r.top = r.bottom - 24;

	BButton *cont = new BButton(r,"continue","Continue Install",new BMessage(B_QUIT_REQUESTED),
									B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(cont);
	cont->MakeDefault(TRUE);
	
	r.right = r.left - 20;
	r.left = r.right - 120;
	BMessage *canMsg = new BMessage(B_QUIT_REQUESTED);
	canMsg->AddBool("canceled",TRUE);	
	cont = new BButton(r,"cancel","Cancel Install",canMsg,B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	
	AddChild(cont);
}

