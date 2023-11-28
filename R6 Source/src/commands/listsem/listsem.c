/* ++++++++++

   FILE:  listsem.cpp
   REVS:  $Revision: 1.8 $
   NAME:  peter
   DATE:  Thu Apr 06 10:24:37 PDT 1995

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
	sem_info	s;
	team_info	t;

	if (get_team_info(tmid, &t) != B_NO_ERROR) {
		printf("\nteam %d unknown\n", tmid);
		return;
	}
	printf("\nTEAM %4d (%s):\n", t.team, t.args);
	printf("     ID                           name  count\n");
	printf("---------------------------------------------\n");
	cookie = 0;
	while (get_next_sem_info (tmid, &cookie, &s) == B_NO_ERROR)
		printf(" %6d %30s  %5d\n", s.sem, s.name, s.count);
}

void
display_sem(sem_id smid)
{
	int32		cookie;
	sem_info	s;
	team_info	t;

	status_t result = get_sem_info(smid, &s);
	
	if (result == B_OK) {
		printf("\nSemaphore %d:\n");
		printf("----------------------\n");
		printf("ID:            %d\n", s.sem);
		printf("Team:          %d\n", s.team);
		printf("Name:          %s\n", s.name);
		printf("Count:         %d\n", s.count);
		printf("Latest holder: %d\n\n", s.latest_holder);
	} else
		printf("\nsemaphore %d unknown\n", smid);
}

void
usage(char *prog)
{
	fprintf(stderr, "\nUsage:  %s [-s semid]  [teamid]\n", prog);
	fprintf(stderr, "\tList the semaphores allocated by the specified\n");
	fprintf(stderr, "\tteam, or all teams if none is specified.\n");
	fprintf(stderr, "\n\tThe -s option displays the sem_info data for a\n");
	fprintf(stderr, "\tspecified semaphore.\n");

	exit(0);
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
	int				displayed_header = 0;

	get_system_info (&sinfo);
	tot = sinfo.max_sems;
	used = sinfo.used_sems;

	if (argc == 1) {
		printf("sem: total: %5d, used: %5d, left: %5d\n",
			tot, used, tot-used);
		cookie = 0;
		while (get_next_team_info(&cookie, &tinfo) == B_NO_ERROR)
			display_team(tinfo.team);
	} else {
		for(i=1; i<argc; i++) {
			if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
				usage(argv[0]);
			else if (strcmp(argv[i], "-s") == 0) {
				if (argc > i + 1)
					display_sem(atoi(argv[++i]));
			} else {
				if (!displayed_header) {
					printf("sem: total: %5d, used: %5d, left: %5d\n",
						tot, used, tot-used);
					displayed_header = 1;
				}
			
				tmid = atoi(argv[i]);
				display_team(tmid);
			}
		}
	}
		
	return 0;
}
