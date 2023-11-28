/* ++++++++++
	FILE:	A2D.cpp
	REVS:	$Revision: 1.4 $
	NAME:	herold
	DATE:	Tue Jun  4 14:46:15 PDT 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "A2D.h"


/* ----------
	constructor
----- */

BA2D::BA2D()
{
	ffd = -1;
}

/* ----------
	destructor
----- */

BA2D::~BA2D()
{
	if (ffd >= 0)
		close(ffd);
}

/* ----------
	open - actually open the device.
----- */

status_t
BA2D::Open(const char *portName)
{
	static struct {
		char	*object_name;	/* name for users of this object */
		char	*dev_name;		/* name for the driver */
	} names[4] = {
		{ "A2D1", "/dev/beboxhw/geekport/a2d/a2d_1" },
		{ "A2D2", "/dev/beboxhw/geekport/a2d/a2d_2" },
		{ "A2D3", "/dev/beboxhw/geekport/a2d/a2d_3" },
		{ "A2D4", "/dev/beboxhw/geekport/a2d/a2d_4" }
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

	ffd = open(dev, O_RDONLY);				// open the port

	return(ffd);
}

/* ----------
	IsOpen - answer the question
----- */

bool
BA2D::IsOpen()
{
	return ((ffd>=0)?TRUE:FALSE);
}

/* ----------
	Close - close the underlying device
----- */

void
BA2D::Close()
{
	if (ffd >= 0)
		close(ffd);
	ffd = -1;
}

/* ----------
	Read - get current digital port value
----- */

ssize_t
BA2D::Read(ushort *buf)
{
	return (ffd >= 0) ? read(ffd, buf, 1) : B_ERROR;
}


/* ----------
	fragile base class padding
----- */

void BA2D::_ReservedA2D1() {}
void BA2D::_ReservedA2D2() {}
void BA2D::_ReservedA2D3() {}
