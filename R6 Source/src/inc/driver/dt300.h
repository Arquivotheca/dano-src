#ifndef	DT300_H
#define DT300_H

#include <Drivers.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum
{
	DT300_READ_BATTERY_STATUS = B_DEVICE_OP_CODES_END + 1,	
	DT300_TURN_OFF_BACKLIGHT,
	DT300_TURN_ON_BACKLIGHT
};

enum
{
	BATTERY_CRITICAL = 0,
	BATTERY_VERY_LOW,
	BATTERY_LOW,
	BATTERY_NOT_LOW,
	BATTERY_FULL
};

#if defined(__cplusplus)
}
#endif

#endif
