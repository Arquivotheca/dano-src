#include <stdio.h>

main(argc, argv)
int	argc;
char	**argv;
{
int	pid, newp;

	if (argc != 3)
		printf("Usage: prio pid newpriority\n"), exit(1);

	pid = atol(argv[1]);
	newp = atol(argv[2]);

	if (set_thread_priority(pid, newp) < 0)
		printf("Priority changed failed.\n"), exit(1);
	exit(0);
}
