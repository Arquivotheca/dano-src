/* +++++++++++
	FILE:	bklight.h
	REVS:	1.0
	DATE:	2/27/01

+++++ */

#ifndef _BKLIGHT_DRIVER_H
#define _BKLIGHT_DRIVER_H

#include <SupportDefs.h>
#include <Drivers.h>

#ifdef __cplusplus
extern "C" {
#endif



/*-----------------
	ioctl codes
----------------- */

enum {
	BK_LIGHT_OFF = B_DEVICE_OP_CODES_END,
	BK_LIGHT_ON
};


#ifdef __cplusplus
}
#endif

#endif
