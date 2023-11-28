#include <stdio.h>
#include <time.h>
#include <OS.h>
#include <FindDirectory.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <priv_syscalls.h>

#undef DEBUG
#define DEBUG 1
#include <Debug.h>

static char *get_timezone_setting(void);

typedef enum {
	no_rtc_setting_file = -1,
	rtc_is_local,
	rtc_is_gmt
} rtc_setting;

static rtc_setting	get_rtc_setting(void);

main()
{
	rtc_info	info;
	char 		*tzname;
	status_t	ret;
	rtc_setting	rtc_status = no_rtc_setting_file;
	
#if __INTEL__
	rtc_status = get_rtc_setting();
	if (rtc_status == no_rtc_setting_file) {
		fprintf(stderr, "clockconfig: No knowledge about contents of RTC\n");
		SERIAL_PRINT(("clockconfig: No knowledge about contents of RTC\n"));
		exit(0);
	} else if (rtc_status == rtc_is_gmt) {
		/*
		 * Things are cool! 
		 */
		fprintf(stderr, "clockconfig: RTC stores GMT time.\n");	
		SERIAL_PRINT(("clockconfig: RTC stores GMT time.\n"));
	} else {
		fprintf(stderr, "clockconfig: RTC stores local time.\n");
		SERIAL_PRINT(("clockconfig: RTC stores local time.\n"));
	}
#endif	
	
	tzname = get_timezone_setting();

	if (tzname == NULL) {
		/*
		 * assume RTC time is identical to localtime.
		 * do nothing.
		 */
		fprintf(stderr, "clockconfig: No timezone setting.\n");
		SERIAL_PRINT(("clockconfig: No timezone setting.\n"));
		exit(0);
	}

	SERIAL_PRINT(("clockconfig: Setting timezone to '%s'\n", tzname));
	printf("clockconfig: Setting timezone to '%s'\n", tzname);
	_kset_tzfilename_(tzname, strlen(tzname), rtc_status == rtc_is_gmt);

#if __POWERPC__ 
	if (_kget_rtc_info_(&info) != 0) {
		fprintf(stderr, "clockconfig: couldn't get RTC information\n");
		exit(1);
	}

	if (info.is_gmt) {
		/*
		 * Things are cool! 
		 */
		SERIAL_PRINT(("clockconfig: RTC stores GMT time.\n"));
	} else {
		SERIAL_PRINT(("clockconfig: RTC stores local time.\n"));
	}
#endif

	/*
	 * If the RTC stores localtime, we need to set variables
	 * such that time() is warped by the proper factor. "timezone" represents
	 * the offset in seconds from GMT for this timezone. It comes from
	 * localtime.c in the C library.
	 *

	 * Even if GMT is stored in the RTC, gettimeofday requires that
	 * the timezone specs are filled in in the kernel.
	 */
	tzset();
	{
		time_t tn = time(0);
		struct tm *t = localtime(&tn);
		if(!t) {
			fprintf(stderr, "clockconfig: could not get timezone\n");
			SERIAL_PRINT(("clockconfig: could not get timezone\n"));
			exit(1);
		}
		_kset_tzspecs_(-t->tm_gmtoff, t->tm_isdst);
	}
	return 0;
}


static char *
get_timezone_setting(void)
{
	char *tzdefault;
	char temp[MAXPATHLEN+1];
	struct stat s;
	int n;

	tzdefault = (char *) malloc(MAXPATHLEN+1);
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, tzdefault, MAXPATHLEN);
	strcat(tzdefault, "/timezone");

	printf("Looking for %s file\n", tzdefault);
	if (lstat(tzdefault, &s) < 0) {
		perror("clockconfig: stat failed");
		return NULL;
	}

	if (S_ISLNK(s.st_mode)) {
		n = readlink(tzdefault, temp, MAXPATHLEN);
		if (n < 0) {
			perror("clockconfig: readlink failed");
			return NULL;
		}
		temp[n] = '\0';
		strcpy(tzdefault, temp);
	}
	
	if (access(tzdefault, R_OK) < 0) {
		free(tzdefault);
		return NULL;
	}

	return tzdefault;
}


static rtc_setting
get_rtc_setting(void)
{
	char setting_file[MAXPATHLEN+1];
	char temp[MAXPATHLEN+1];
	struct stat s;
	char c = 0;
	int fd;

	printf("Looking for RTC settings file\n");
	SERIAL_PRINT(("Looking for RTC settings file\n"));
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, TRUE, setting_file, MAXPATHLEN);
	strcat(setting_file, "/" RTC_SETTINGS_FILE);

	fd = open(setting_file, O_RDONLY);
	if (fd < 0) {
		perror("clockconfig: open of RTC setting file failed");
		return no_rtc_setting_file;
	}
	
	read(fd, &c, 1);
	close(fd);
	if (c == 'l')
		return rtc_is_local;
	else if (c == 'g')
		return rtc_is_gmt;
	else
		return no_rtc_setting_file;
}
