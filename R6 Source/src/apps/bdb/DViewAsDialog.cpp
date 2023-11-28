/*	$Id: DViewAsDialog.cpp,v 1.2 1999/05/03 13:10:00 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 02/18/99 11:42:57
*/

#include "bdb.h"
#include "DViewAsDialog.h"
#include "DMessages.h"
#include <Message.h>
#include <Messenger.h>

DViewAsDialog::DViewAsDialog(BRect frame, const char *name, window_type type, int flags,
	BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	FindView("cast")->MakeFocus();
	Show();
} // DViewAsDialog::DViewAsDialog
		
bool DViewAsDialog::OKClicked()
{
	BMessage m(kMsgViewAs);
	
	m.AddString("cast", GetText("cast"));
	FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&m, (BHandler *)0, 1000));
	
	return true;
} // DViewAsDialog::OKClicked


