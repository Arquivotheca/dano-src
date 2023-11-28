#include "bdb.h"
#include "DShowFunction.h"
#include "DMessages.h"

#include <TextControl.h>

DShowFunction::DShowFunction(BRect frame, const char *name, window_type type, int flags,
	BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	FindView("name")->MakeFocus();
	Show();
}
		
bool DShowFunction::OKClicked()
{
	BMessage m(kMsgShowFunction);
	
	m.AddString("name", GetText("name"));
	FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&m, (BHandler *)0, 1000));
	
	return true;
}

void DShowFunction::SetDefaultName(const char* name)
{
	this->SetText("name", name);
	
	BTextControl *view = dynamic_cast<BTextControl*>(this->FindView("name"));
	
	FailNil(view);
	view->MakeFocus();
	view->TextView()->SelectAll();
} // DShowFunction::SetDefaultName

