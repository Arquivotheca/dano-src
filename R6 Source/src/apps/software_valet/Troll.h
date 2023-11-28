#ifndef _TROLL_H_
#define _TROLL_H_

// Troll.h

#include "HTConnection.h"
#include "MThread.h"

class PackageItem;
class PackageDB;
class BMessenger;

status_t	PerformUpdateCheck(BMessage *msg,
							PackageItem *it,
							PackageDB	&db,
							HTConnection &htconn,
							BMessenger &response,
							BMessage *report = NULL);
							
int32		Troll();

class	TrollThread : public MThread
{
public:
	TrollThread()
		:	MThread("troll thread")
	{
	}
protected:
	virtual long Execute()
	{
		return Troll();
	}
};

#endif

