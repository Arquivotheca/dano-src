/*		
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	Author:	John R. Dance
			20 August 1999

*/

#include "RTypes.r"
#include "DMessages.h"

Resource 'MENU' (300, "View Menu")
{
	"View",
	{
		Item		{ "Refresh", kMsgRefreshMemory, none, nokey },
		Separator	{},
		Item		{ "Scroll to Selection", kMsgScrollToAnchor, none, nokey },
		Item		{ "View at Address...", kMsgNewMemoryLocationCmd, none, nokey },
		Separator	{},
		Item		{ "Previous Memory Page", kMsgPreviousMemory, none, nokey },
		Item		{ "Next Memory Page", kMsgNextMemory, none, nokey },
	}
};

Resource 'MENU' (301, "Data Menu")
{
	"Data",
	{
		Item		{ "Add Watchpoint", kMsgAddWatchpoint, none, noKey },
	}
};

Resource 'MBAR' (300, "MemoryWindowMenuBar" )
{
	{
		300, 301
	}
};
