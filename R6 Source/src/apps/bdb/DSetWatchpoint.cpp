#include "bdb.h"
#include "DSetWatchpoint.h"
#include "DMessages.h"
#include <Message.h>
#include <Messenger.h>

DSetWatchpoint::DSetWatchpoint(BRect frame, const char *name, window_type type, int flags,
	BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	FindView("addr")->MakeFocus();
	Show();
}
		
bool DSetWatchpoint::OKClicked()
{
	BMessage m(kMsgSetWatchpoint);
	
	const char *s = GetText("addr");
	if (strncmp(s, "0x", 2) == 0) s += 2;
	
	ptr_t addr = strtoul(s, NULL, 16);
	m.AddInt32("address", addr);
	FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&m, (BHandler *)0, 1000));
	
	return true;
}

