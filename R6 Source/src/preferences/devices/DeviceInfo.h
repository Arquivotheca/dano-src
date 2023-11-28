/* ++++++++++
	FILE:	ppc/dbprocs.c
	REVS:	$Revision: 1.7 $
	NAME:	herold
	DATE:	Wed May 24 16:27:36 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdio.h>

#include <KernelExport.h>
#include <debugger.h>

#include "thread.h"
#include "kdebug.h"
#include "vm.h"

#include "wait_priv.h"


/* -----
	Globals
----- */

port_id		default_debugger_port = -1;		/* default debugger port */
team_id		default_debugger_team = -1;		/* team id of default debugger */


/* ----------
	install_default_debugger installs a 'default' external debugger, used
	when a thread crashes, issues a debugger() call, or is stopped.
----- */
status_t
install_default_debugger (port_id to_debugger_port)
{
	if (isbadport((port_id) to_debugger_port))
		return B_BAD_PORT_ID;
	default_debugger_port = (port_id) to_debugger_port;
	default_debugger_team = thread_tab[thidtoslot(getthid())]->tmr->tmid;
	return B_NO_ERROR;
}


void
debugger_kill_my_team(team_id tmid)
{
	thread_info info;
	int32 cookie = 0;
	thread_id thid = getthid();
	team_rec *tmr;

	/*
	 * A thread in this team may be stuck in the kernel
	 * waiting for a reply from the debugger thread. So
	 * send it a dummy response so it can die cleanly too.
	 */
	while (get_next_thread_info(tmid, &cookie, &info) == B_NO_ERROR) {
		thread_rec *other_thr;
		if (info.thread == thid)
			continue;
		other_thr = thread_tab[thidtoslot(info.thread)];
		kill_thread (info.t