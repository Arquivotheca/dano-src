#include "bdb.h"
#include "DGetMemoryLocation.h"
#include "DMessages.h"

#include <TextControl.h>

DGetMemoryLocation::DGetMemoryLocation(BRect frame, const char *name, window_type type, int flags,
	BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	FindView("addr")->MakeFocus();
	Show();
}
		
bool DGetMemoryLocation::OKClicked()
{
	BMessage m(kMsgNewMemoryLocation);
	
	const char *s = GetText("addr");
	if (strncmp(s, "0x", 2) == 0) s += 2;
	
	ptr_t addr = strtoul(s, NULL, 16);
	m.AddInt32("address", addr);
	FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&m, (BHandler *)0, 1000));
	
	return true;
}

void DGetMemoryLocation::SetDefaultAddress(ptr_t addr)
{
	if (addr != 0) 
	{
		char textBuffer[16];
		sprintf(textBuffer, "%lx", addr);
	
		this->SetText("addr", textBuffer);

		BTextControl *view = dynamic_cast<BTextControl*>(this->FindView("addr"));

		FailNil(view);
		view->MakeFocus();
		view->TextView()->SelectAll();
	}
} // DFindDialog::SetTarget

