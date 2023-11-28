/* ++++++++++
	FILE:	debug_support.h
	REVS:	$Revision: 1.14 $
	NAME:	pierre
	DATE:	Wed Mar 12 14:59:12 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _DEBUG_SUPPORT_H
#define _DEBUG_SUPPORT_H

#include <OS.h>
#include <Screen.h>

/* Input : pass the team_id of the team who crashed
   Output : set the BScreen object to the screen selected.
            ws_index is the index of the workspace that you need to
            switch to on that BScreen. */

void _find_unblocked_screen_(team_id bad_team, BScreen *screen, int32 *ws_index);

#endif







