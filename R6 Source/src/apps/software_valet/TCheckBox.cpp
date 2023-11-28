#include "TCheckBox.h"

// TCheckBox.cpp

// implementation of a check box which automatically
// enables and disables a set of BControl objects

TCheckBox::TCheckBox(	BRect frame,
						const char *name,
						const char *label,
						BMessage *message,
						ulong resizeMask, 
						ulong flags)
	:	BCheckBox(	frame, name, label, message, resizeMask, flags)
{
	fControlList = new RList<BControl *>;
}


void TCheckBox::SetValue(long value)
{
	for (long i = fControlList->CountItems()-1; i >= 0; i--)
		fControlList->ItemAt(i)->SetEnabled(value ? TRUE : FALSE);
	BCheckBox::SetValue(value);
}

void TCheckBox::SetEnabled(bool on)
{
	BCheckBox::SetEnabled(on);
	for (long i = fControlList->CountItems()-1; i >= 0; i--)
		fControlList->ItemAt(i)->SetEnabled(on);
}

bool TCheckBox::AddSlave(	BControl *slave)
{
	return fControlList->AddItem(slave);
}

bool TCheckBox::RemoveSlave( BControl *slave)
{
	return fControlList->RemoveItem(slave);
}
