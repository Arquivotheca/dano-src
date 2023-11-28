/* ++++++++++
	FILE:	flash_driver.h
	REVS:	$Revision: 1.1 $
	NAME:	herold
	DATE:	Thu Mar 28 17:26:57 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _FLASH_DRIVER_H
#define _FLASH_DRIVER_H

#ifndef _DRIVERS_H
#include <Drivers.h>
#endif

/* -----
	ioctl codes
----- */

enum {
	B_GET_FLASH_SECTOR_SIZE = B_DEVICE_OP_CODES_END + 1,
	B_IS_FLASH_SECTOR_PROTECTED
};


#endif
