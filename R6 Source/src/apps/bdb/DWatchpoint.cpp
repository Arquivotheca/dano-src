#include "bdb.h"
#include "DWatchpoint.h"

unsigned char *DWatchpoint::sfIcon = NULL;

unsigned char *
DWatchpoint::EnabledIconBits() const
{
	if (!sfIcon)
		FailNilRes(sfIcon = (unsigned char *)HResources::GetResource('MICN', 1003));
	return sfIcon;
}

