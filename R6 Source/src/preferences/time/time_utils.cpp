#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <priv_syscalls.h>

#include "dt_utils.h"
#include "time_utils.h"

// *************************************************************************** //

short
_Hour(time_t t, char* str)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	format = "%I";
	strftime(str, 3, format, t_info);
	
	return atoi(str);
}

short
_Minute(time_t t, char* str)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	format = "%M";
	strftime(str, 3, format, t_info);
	
	return atoi(str);
}

short
_Seconds(time_t t, char* str)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	format = "%S";
	strftime(str, 3, format, t_info);
	
	return atoi(str);
}

bool
_Interval(time_t t, char* str)
{
	char* format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	format = "%p";
	strftime(str, 3, format, t_info);

	return strcmp(str, "AM") == 0;
}

//

short
_Date(time_t t, char* str)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	format = "%d";
	strftime(str, 3, format, t_info);
	
	return atoi(str);
}

short
_Day(time_t t, char* str, bool full)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	if (full)
		format = "%A";
	else
		format = "%a";
	strftime(str, 16, format, t_info);
	
	return atoi(str);
}

short
_Month(time_t t, char* str, bool full)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	if (full)
		format = "%B";
	else
		format = "%b";
	strftime(str, 16, format, t_info);
	
	char temp[3];
	format = "%m";
	strftime(temp, 3, format, t_info);

	return atoi(temp);
}

short
_Year(time_t t, char* str)
{
	char *format;
	struct tm *t_info = localtime(&t);
	str[0] = 0;
	
	format = "%Y";
	strftime(str, 5, format, t_info);
	
	return atoi(str);
}

static short DayInWeek(struct tm *timeinfo)
{
	char thedaystr[2],temp[32];
	short theday;
	
	temp[0] = 0;
	thedaystr[0] = 0;
	
	strftime(temp,32,"%A",timeinfo);
#if DEBUG
	printf("full weekday name is %s\n",temp);
#endif	
	strftime(thedaystr,2,"%w",timeinfo);
	theday = atoi(thedaystr);
#if DEBUG
	printf("current week day is %s\n",thedaystr);
#endif	
	
	return theday;
}

#define kSecsInDay 86400 // 24 * 60 * 60

//
//	Returns the number of days in the month as specified in the struct
//
static short DaysInMonth(time_t *firstdayofmonth)
{
	struct tm *timeinfo;
	time_t currday;
	short daycnt=28;
	char currmonth[4],targetmonth[4];
	
	timeinfo = localtime(firstdayofmonth);
	strftime(targetmonth,4,"%m",timeinfo);
	
	currday = (*firstdayofmonth) + (27 * kSecsInDay);
	while(true)
	{
		currday += kSecsInDay;
		timeinfo = localtime(&currday);		
		strftime(currmonth,32,"%m",timeinfo);

		if (strcmp(currmonth,targetmonth) != 0)
			break;
		else {
			daycnt++;
			if (daycnt >= 31) {
				daycnt = 31;
				break;
			}
		}
	}
	
	return daycnt;
}

//	converts currtime to seconds of first day of month
//	returns start day of week and number of days in month
void
_MonthInfo(time_t *currtime,short *startday,short *daycnt)
{
	struct tm *timeinfo;
	
	timeinfo = localtime(currtime);

	timeinfo->tm_sec = 0;						//	set the time to 00:00
	timeinfo->tm_min = 0;						
	timeinfo->tm_hour = 0;	
	timeinfo->tm_mday = 1;						//	first day of this month
	
	*currtime = mktime(timeinfo);
	
	*startday = DayInWeek(timeinfo);
	*daycnt = DaysInMonth(currtime);
#if DEBUG
	printf("currtime %i, startday %i, days %i\n",*currtime,*startday,*daycnt);
#endif
}

bool
_DayLightSavings(time_t *t)
{
	struct tm *new_time = localtime(t);
	int dst = new_time->tm_isdst;
//printf("dst is %i\n", dst);
	return (dst <= 0);	// not in effect or not available
}

void
_SetDayLightSavings(bool state)
{
	time_t t = time(NULL);
	struct tm *new_time = localtime(&t);
	int dst = new_time->tm_isdst;
//printf("set dst - %i %i\n", dst, state);
	if (dst != state) {
		new_time->tm_isdst = state;
		t = mktime(new_time);
		
		stime(&t);
	}
}

//


void
TimeZoneSetting(char* region, char* city)
{
	char* str;
	char city_path[B_PATH_NAME_LENGTH];

	int ret = _kget_tzfilename_(city_path);
	if (ret != B_NO_INIT) {
		BEntry c(city_path);
		c.GetName(city);
		if ((str = strstr(city, "_IN")) != 0)
			strcpy(str, ", Indiana");
		else if ((str = strstr(city, "__Calif")) != 0)
			strcpy(str, ", Calif");

		replace_underscores(city);
		
		BEntry d;
		c.GetParent(&d);
		d.GetName(region);

		if (strcmp(region, "Pacific") == 0
			|| strcmp(region, "Atlantic") == 0
			|| strcmp(region, "Indian") == 0) {
			
			strcat(region, " Ocean");
			
		} else	
			replace_underscores(region);

	} else {
		region[0] = 0;
		city[0] = 0;
	}
}

void
CurrentTZSettings( char *region, char *city)
{
	if (!city)
		return;
		
	int ret;
	if ((ret = _kget_tzfilename_(city)) != 0) {
		return;
	}

	if (!region)
		return;
	_DirectoryNameFromPath(city, region);
}

void
SetSystemTimeZone(char* city_path)
{
	rtc_info info;
	_kget_rtc_info_(&info);
//	if (info.is_gmt) {
		// 	sets the timezone environment variable
		//	any call to localtime will now return that tz's time
		//	need to call set_timezone to actually change
		//	the system clock
		char temp[B_PATH_NAME_LENGTH + 4];
		sprintf(temp, "TZ=%s", city_path);
		putenv(temp);
		tzset();
//	} 

	set_timezone(city_path);
}

void
SetTimeZone(const char* path)
{
	rtc_info info;
	_kget_rtc_info_(&info);
//	if (info.is_gmt) {
		// 	sets the timezone environment variable
		//	any call to localtime will now return that tz's time
		//	need to call set_timezone to actually change
		//	the system clock
		char temp[B_PATH_NAME_LENGTH + 4];
		sprintf(temp, "TZ=%s", path);
		putenv(temp);
		tzset();
//	} 
}
