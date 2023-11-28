
#include <kernel/OS.h>
#include <string.h>
#include <stdio.h>

uint32 OperaMemory()
{
	// find the Opera team
	team_info team;

	bool found = false;
	int32 cookie = 0;
	while (get_next_team_info(&cookie, &team) == B_OK)
	{
		if (strncmp(team.args, "/boot/apps/Opera/Opera", 22) == 0)
		{
			found = true; 
			break;
		}
	}

	if (found)
	{
		area_info info;
		int32 cookie = 0;
		uint32 memUsed = 0;
		while(get_next_area_info(team.team, &cookie, &info) == B_OK)
		{
			memUsed += info.ram_size;
		}
		return memUsed;
	}
	else return 0UL;
}

int main()
{
	uint32 operaMem = OperaMemory();
	system_info sysInfo;
	get_system_info(&sysInfo);

	uint32 kTotal = sysInfo.max_pages * 4;
	uint32 kFree = (sysInfo.max_pages - sysInfo.used_pages) * 4;
	uint32 kOpera = operaMem / 1024;
	uint32 kSystem = (sysInfo.used_pages * 4) - kOpera; 
	time_t theTime = time(&theTime);

	printf("%sSystem:\t%8ld k\t%02.1f%%\nOpera:\t%8ld k\t%02.1f%%\nFree:\t%8ld k\t%02.1f%%\n\n",
		ctime(&theTime),
		kSystem, ((float)kSystem/(float)kTotal) * 100.0,
		kOpera, ((float)kOpera/(float)kTotal) * 100.0,
		kFree, ((float)kFree/(float)kTotal) * 100.0);
}

