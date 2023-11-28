/*	$Id: DFindDialog.cpp,v 1.4 1999/05/03 13:09:50 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 11/22/98 13:34:15
*/

#include "bdb.h"
#include "DFindDialog.h"
#include "DMessages.h"
#include "HPreferences.h"

#include <TextControl.h>

DFindDialog::DFindDialog(BRect frame, const char *name, window_type type, int flags,
	BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	SetOn("icase", gPrefs->GetPrefInt("find: ignore case", false));
	Show();
} // DFindDialog::DFindDialog

void DFindDialog::SetTarget(const char *what, BHandler *h)
{
	SetText("find", what);
	fHandler = h;

	BTextControl *view = dynamic_cast<BTextControl*>(FindView("find"));

	FailNil(view);
	view->MakeFocus();
	view->TextView()->SelectAll();
} // DFindDialog::SetTarget

bool DFindDialog::OKClicked()
{
	BMessage msg(kMsgDoFind);
	msg.AddString("find", GetText("find"));
	msg.AddBool("icase", IsOn("icase"));
	msg.AddBool("startAtTop", IsOn("startAtTop"));
	FailMessageTimedOutOSErr(BMessenger(fHandler).SendMessage(&msg, (BHandler *)0, 1000));

	gPrefs->SetPrefString("find: text", GetText("find"));
	gPrefs->SetPrefInt("find: ignore case", IsOn("icase"));

	return true;
} // DFindDialog::OKClicked
