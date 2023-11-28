/* ++++++++++
	FILE:	drivers_priv.h
	REVS:	$Revision: 1.4 $
	NAME:	herold
	DATE:	Sun Mar 03 20:45:59 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _DRIVERS_PRIV_H
#define _DRIVERS_PRIV_H

#include <Drivers.h>


/* ioctl(), *int32 == 1 for powersave, 0 for wakeup */
#define B_SET_POWERSAVE 998

/* ---
	used to register fixed drivers
--- */

typedef struct {
	char			*name;
	int32			api_version;
	long            (*init_hardware)(void);
	const char **	(*publish_devices)(void);
	device_hooks *	(*find_device)(const char *name);
	long            (*init_driver)(void);
	void            (*uninit_driver)(void);	
	void            (*wake_driver)(void);
	void            (*suspend_driver)(void);
} fixed_driver_info;

#endif
