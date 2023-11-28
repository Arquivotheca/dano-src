#include <time.h>

short _Hour(time_t t, char* str);
short _Minute(time_t t, char* str);
short _Seconds(time_t t, char* str);
bool _Interval(time_t  t, char* str);

short _Date(time_t t, char* str);				// 	date 1-31
short _Day(time_t t, char* str, bool full);		//	day of the week sun-sat
short _Month(time_t t, char* str, bool full);
short _Year(time_t t, char* str);

void _MonthInfo(time_t *currtime,short *weekday,short *daycnt);
bool _DayLightSavings(time_t *t);
void _SetDayLightSavings(bool state);

void TimeZoneSetting(char* region, char* city);
void CurrentTZSettings( char *region, char *city);
void SetSystemTimeZone(char* city_path);
void SetTimeZone(const char* path);
