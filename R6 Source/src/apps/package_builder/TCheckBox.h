// #include <InterfaceKit.h>
// TCheckBox.h

#include "RList.h"

#ifndef _TCHECKBOX_H
#define _TCHECKBOX_H

class TCheckBox : public BCheckBox
{
public:
				TCheckBox(	BRect frame,
							const char *name,
							const char *label,
							BMessage *message,
							ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
							ulong flags = B_WILL_DRAW | B_NAVIGABLE);

	virtual void	SetValue(long value);
	virtual void	SetEnabled(bool on);
	
	bool		AddSlave( BControl *slave);
	bool		RemoveSlave( BControl *slave);
private:
	RList<BControl *>	*fControlList;
};

#endif
