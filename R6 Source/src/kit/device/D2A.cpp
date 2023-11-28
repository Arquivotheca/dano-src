/* ++++++++++
	FILE:	D2A.cpp
	REVS:	$Revision: 1.4 $
	NAME:	herold
	DATE:	Tue Jun  4 14:46:20 PDT 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "D2A.h"


/* ----------
	constructor
----- */

BD2A::BD2A()
{
	ffd = -1;
}

/* ----------
	destructor
----- */

BD2A::~BD2A()
{
	if (ffd >= 0)
		close(ffd);
}

/* ----------
	open - actually open the device.
----- */

status_t
BD2A::Open(const char *portName)
{
	static struct {
		char	*object_name;	/* name for users of this object */
		char	*dev_name;		/* name for the driver */
	} names[4] = {
		{ "D2A1", "/dev/beboxhw/geekport/d2a/d2a_0" },
		{ "D2A2", "/dev/beboxhw/geekport/d2a/d2a_1" },
		{ "D2A3", "/dev/beboxhw/geekport/d2a/d2a_2" },
		{ "D2A4", "/dev/beboxhw/geekport/d2a/d2a_3" }
	};
	char	*dev = NULL;
	int		i;
	
	// look for passed name, map to device name
	for (i = 0; !dev && i < sizeof (names) / sizeof (names[0]); i++)
		if (!strcmp (names[i].object_name, portName))
			dev = names[i].dev_name;
	
	if (!dev)								// name not found?
		return B_ERROR;
		
	if (ffd >= 0)							// it's already open
		close(ffd);							// close to reopen

	ffd = open(dev, O_RDWR);				// open the port

	return(ffd);
}

/* ----------
	IsOpen - or not
----- */

bool
BD2A::IsOpen()
{
	return ((ffd>=0)?TRUE:FALSE);
}

/* ----------
	Close - close the underlying device
----- */

void
BD2A::Close()
{
	if (ffd >= 0)
		close(ffd);
	ffd = -1;
}

/* ----------
	Read - get last value written
----- */

ssize_t
BD2A::Read(uint8 *buf)
{
	return (ffd >= 0) ? read(ffd, buf, 1) : B_ERROR;
}

/* ----------
	Write - set d2a output
----- */

ssize_t
BD2A::Write(uint8 value)
{
	char	buf = value;
	
	return (ffd >= 0) ? write(ffd, &buf, 1) : B_ERROR;
}

/* ----------
	fragile base class padding
----- */

void BD2A::_ReservedD2A1() {}
void BD2A::_ReservedD2A2() {}
void BD2A::_ReservedD2A3() {}

