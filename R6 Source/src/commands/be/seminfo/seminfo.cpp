#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	int rc = 0;
	if (argc != 2)
	{
		fprintf(stderr, "Usage: seminfo semid\n");
		rc = 1;
	}
	else
	{
		sem_info si;
		sem_id sem = (sem_id) atoi(argv[1]);
		status_t err = get_sem_info(sem, &si);
		if (err)
		{
			fprintf(stderr, "Error 0x%x getting info for sem_id %d (%s)\n", (unsigned) err, (int) sem, strerror(err));
			rc = 1;
		}
		else
		{
			printf("Semaphore %d:\n", (int) sem);
			printf("\tteam = %d\n\tname = %s\n\tcount = %d\n\tlatest_holder = %d\n",
				(int) si.team, si.name, (int) si.count, (int) si.latest_holder);
		}
	}
	return rc;
}
