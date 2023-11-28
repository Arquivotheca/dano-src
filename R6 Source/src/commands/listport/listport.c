/* ++++++++++

   FILE:  listport.cpp
   REVS:  $Revision: 1.8 $
   NAME:  peter
   DATE:  Thu Apr 06 10:33:14 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <stdlib.h>
#include <stdio.h>

#include <Errors.h>
#include <OS.h>

void
display_team(team_id tmid)
{
	int32		cookie;
	port_info	p;
	team_info	t;

	if (get_team_info(tmid, &t) != B_NO_ERROR) {
		printf("\nteam %d unknown\n", tmid);
		return;
	}
	printf("\nTEAM %4d (%s):\n", t.team, t.args);
	printf("   ID                         name  capacity  queued\n");
	printf("----------------------------------------------------\n");

	cookie = 0;
	while (get_next_port_info (tmid, &cookie, &p) == B_NO_ERROR)
		printf("%5d %28s  %8d  %6d\n", p.port, p.name, p.capacity, p.queue_count);
}

int
main(int argc, char **argv)
{
	int32			cookie;
	team_info       tinfo;
	system_info		sinfo;
	team_id			tmid;
	int				i;
	long			tot, used;

	get_system_info (&sinfo);
	tot = sinfo.max_ports;
	used = sinfo.used_ports;
	printf("port: total: %5d, used: %5d, left: %5d\n",
		tot, used, tot-used);

	if (argc == 1) {
		cookie = 0;
		while (get_next_team_info(&cookie, &tinfo) == B_NO_ERROR)
			display_team(tinfo.team);
	} else {
		for(i=1; i<argc; i++) {
			tmid = atoi(argv[i]);
			display_team(tmid);
		}
	}
		
	return 0;
}
