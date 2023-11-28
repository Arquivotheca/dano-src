/* ++++++++++
	FILE:	rtc_info.h
	NAME:	mani
	DATE:	2/1998
	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _RTC_INFO_H
#define _RTC_INFO_H

typedef struct {
        uint32          time;
        bool            is_gmt;
        int32           tz_minuteswest;
        int32           tz_dsttime;
} rtc_info;

#define RTC_SETTINGS_FILE	"RTC_time_settings"

extern status_t user_get_rtc_info(rtc_info *);
extern status_t get_rtc_info(rtc_info *);

#endif /* _RTC_INFO_H */
