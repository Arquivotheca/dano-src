/* ++++++++++

   FILE:  listarea.cpp
   REVS:  $Revision: 1.13 $
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
	area_info	a;
	team_info	t;

	if (tmid == 1) {
		printf("KERNEL SPACE\n");
		printf("  ID                          name  address     size  allocated  #-cow  #-in  #-out\n");
		printf("-------------------------------------------------------------------------------\n");
	} else {
		if (get_team_info(tmid, &t) != B_NO_ERROR) {
			printf("\nteam %d unknown\n", tmid);
			return;
		}
		printf("\nTEAM %4d (%s):\n", t.team, t.args);
		printf("  ID                          name  address     size   alloc. #-cow  #-in #-out\n");
		printf("-------------------------------------------------------------------------------\n");
	}

	cookie = 0;
	while (get_next_area_info (tmid, &cookie, &a) == B_NO_ERROR)
		printf("%4d %29s %.8x %8x %8x %5d %5d %5d\n",
			a.area, a.name, a.address, a.size, a.ram_size, a.copy_count,
			a.in_count, a.out_count);       
}

int
main(int argc, char **argv)
{
	int32			cookie;
	team_info       tinfo;
	system_info		sinfo;
	team_id			tmid;
	int				i;
	long			tot, avail;

	get_system_info (&sinfo);
	tot = sinfo.max_pages * 4;
	avail = (sinfo.max_pages - sinfo.used_pages) * 4;
	printf("memory: total: %4dKB, used: %4dKB, left: %4dKB\n",
		tot, tot-avail, avail);

	if (argc == 1) {
		display_team(1);
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

