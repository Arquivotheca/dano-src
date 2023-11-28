/* ++++++++++
	FILE:	digital_driver.h
	REVS:	$Revision$
	NAME:	herold
	DATE:	Thu Mar 21 21:01:00 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

	Interface to /dev/digital, the digital port driver.
+++++ */

#ifndef _DIGITAL_DRIVER_H
#define _DIGITAL_DRIVER_H

#ifndef _DRIVERS_H
#include <Drivers.h>
#endif

/* -----
	The digital portion of the GeekPort(tm) consists of two 8 bit digital
	ports, port a and port b.  Each port can be sets to be all inputs or
	all outputs.  The ports can be read and written using read and write.

	If set as outputs, a read will still read the state of the pin (which
	could be externally forced to be different than the output).
----- */

/* -----
	ioctl codes
----- */

enum {
	SET_DIGITAL_PORT_DIRECTION = B_DEVICE_OP_CODES_END + 1,
	GET_DIGITAL_PORT_DIRECTION
};


/* -----
	values passed/returned in *data arg to ioctl
----- */

enum {
	DIGITAL_PORT_INPUT = 0,
	DIGITAL_PORT_OUTPUT
};

#endif

