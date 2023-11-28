/* ++++++++++

   FILE:  listimage.cpp
   REVS:  $Revision: 1.8 $
   NAME:  peter
   DATE:  Thu Apr 06 10:33:14 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <stdlib.h>
#include <stdio.h>

#include <Errors.h>
#include <OS.h>
#include <image.h>

void
display_team(team_id tmid)
{
	int32		cookie;
	image_info	p;
	team_info	t;

	if (tmid == 1) {
		printf("\nKERNEL TEAM:\n");
	} else {
		if (get_team_info(tmid, &t) != B_NO_ERROR) {
			printf("\nteam %ld unknown\n", tmid);
			return;
		}
		printf("\nTEAM %4ld (%s):\n", t.team, t.args);
	}
	printf("   ID                                                             name     text     data seq#      init#\n");
	printf("--------------------------------------------------------------------------------------------------------\n");

	cookie = 0;
	while (get_next_image_info (tmid, &cookie, &p) == B_NO_ERROR)
		printf("%5ld %64s %.8lx %.8lx %4ld %10lu\n", p.id, p.name, (uint32)p.text, (uint32)p.data, p.sequence, p.init_order);
}

int
main(int argc, char **argv)
{
	int32			cookie;
	team_info       tinfo;
	team_id			tmid;
	int				i;

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
