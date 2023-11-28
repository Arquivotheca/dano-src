/*
 * This is a standalone tool which provides status information about the 
 * system.
 */

#include <stdio.h>
#include <string.h>
#include <OS.h>

void	c_status();
char 	*pstate_names[] = {
	"",
	"run",
	"rdy",
	"msg",
	"zzz",
	"sus",
	"sem"
	};

int
do_ps (int argc, char **argv)
{
	int32		tmcookie, thcookie;
	thread_info	*t;
	team_info	*tm;
	int			secs;
	char		buf[B_OS_NAME_LENGTH+16];
	ulong		pages_avail, pages_total;
	float		ratio;
	system_info	sinfo;
	sem_info si;
	team_id	which_tid = 0;
	const char* which_team = NULL;

	/* argc == 2 means it's "ps teamname" */
	/* argc == 3 means it's "ps -p team_id" */
	if (argc ==2) {
		which_team = argv[1];
	}
	else if ((argc == 3) && !strcmp(argv[1], "-p")) {
		which_tid = (team_id) atoi(argv[2]);
	}

	t  = (thread_info *) malloc (sizeof (thread_info));
	tm = (team_info *) malloc (sizeof (team_info));

	printf("\n thread           name      state prio   user  kernel semaphore\n");
	printf(  "-----------------------------------------------------------------------\n");
	
	tmcookie = 0;
	while (get_next_team_info(&tmcookie, tm) == B_NO_ERROR) {
		/* skip it if the arguments constrain the results */
		if (which_tid && (tm->team != which_tid)) continue;
		if (which_team) {
			/* find the base name of this team, compare it against the argument */
			char* tname = strrchr(tm->args, '/');
			if (tname)
				tname++;
			else
				tname = tm->args;
			if (strncmp(which_team, tname, strlen(which_team))) continue;
		}

		printf("%s (team %d) (uid %d) (gid %d)\n", tm->args, tm->team,tm->uid,tm->gid);
			   
		thcookie = 0;
		while (get_next_thread_info(tm->team, &thcookie, t) == B_NO_ERROR) {
			if (t->state == B_THREAD_WAITING) {
				get_sem_info(t->sem, &si);
				sprintf(buf, "%s(%d)", si.name, t->sem);
			} else
				buf[0] = '\0';

			printf("%7d %20s%5s%4d%8Ld%8Ld %s\n",
				   t->thread, t->name,
				   pstate_names[t->state], t->priority,
					t->user_time/1000,
					t->kernel_time/1000,
					buf);
		}
	}
	
	free (t);
	free (tm);
	get_system_info (&sinfo);
	pages_avail = 4096 * (sinfo.max_pages - sinfo.used_pages);
	pages_total = 4096 * sinfo.max_pages;
	printf("\n%5dk (%8d bytes) total memory\n", pages_total/1024, pages_total);
	printf("%5dk (%8d bytes) currently committed\n",
		(pages_total-pages_avail)/1024, pages_total-pages_avail);
	printf("%5dk (%8d bytes) currently available\n",
		pages_avail/1024, pages_avail);
	ratio = ((float)(pages_total-pages_avail)/(float)pages_total)*100.0;
	printf("%2.1f%% memory utilisation\n", ratio); 
	return 0;
}

#ifndef PS_LOOP

int
main (int argc, char **argv)
{
	return do_ps (argc, argv);
}

#endif
