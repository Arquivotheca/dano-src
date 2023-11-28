#include <time.h>

extern time_t real_time_clock(void);

time_t 
time(time_t *timer)
{
	time_t time = real_time_clock();

	if (timer)
		*timer = time;

	return time;
}
