/* ++++++++++
	FILE:	DigitalPort.cpp
	REVS:	$Revision: 1.5 $
	NAME:	herold
	DATE:	Tue Jun 4 14:35:51 PDT 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "DigitalPort.h"
#include "digital_driver.h"


/* ----------
	constructor
----- */

BDigitalPort::BDigitalPort()
{
	ffd = -1;
}

/* ----------
	destructor
----- */

BDigitalPort::~BDigitalPort()
{
	SetAsInput();
	
	if (ffd >= 0)
		close(ffd);
}

/* ----------
	open - actually open the device.
----- */

status_t
BDigitalPort::Open(const char *portName)
{
	static struct {
		char	*object_name;	/* name for users of this object */
		char	*dev_name;		/* name for the driver */
	} names[2] = {
		{ "DigitalA", "/dev/beboxhw/geekport/digital/digital_port_a" },
		{ "DigitalB", "/dev/beboxhw/geekport/digital/digital_port_b" }
	};
	char	*dev = NULL;
	int		i;
	
	// look for passed name, map to device name
	for (i = 0; !dev && i < sizeof (names) / sizeof (names[0]); i++)
		if (!strcmp (names[i].object_name, portName))
			dev = names[i].dev_name;
	
	if (!dev)									// name not found?
		return B_ERROR;
		
	if (ffd >= 0)								// it's already open
		close(ffd);								// close to reopen

	ffd = open(dev, O_RDWR);					// open the port
	if (ffd >= 0)								// if it worked
		SetAsInput();							// make it inputs

	return(ffd);
}

/* ----------
	IsOpen - and vice versa
----- */

bool
BDigitalPort::IsOpen()
{
	return ((ffd>=0)?TRUE:FALSE);
}

/* ----------
	Close - close the underlying device
----- */

void
BDigitalPort::Close()
{
	if (ffd >= 0) {
		SetAsInput();						// make it inputs
		close(ffd);
	}
	ffd = -1;
}

/* ----------
	Read - get current digital port value
----- */

ssize_t
BDigitalPort::Read(uint8 *buf)
{
	return (ffd >= 0) ? read(ffd, buf, 1) : B_ERROR;
}

/* ----------
	Write - set digital port value
----- */

ssize_t
BDigitalPort::Write(uint8 value)
{
	uint8	buf = value;
	
	return (ffd >= 0) ? write(ffd, &buf, 1) : B_ERROR;
}


/* ----------
	SetAsOutput - set the digital port to be outputs
----- */
status_t
BDigitalPort::SetAsOutput(void)
{
	if (ffd < 0)
		return B_ERROR;
	fIsInput = FALSE;
	return ioctl (ffd, SET_DIGITAL_PORT_DIRECTION, 
		(void *) DIGITAL_PORT_OUTPUT);
}


/* ----------
	IsOutput - return true if port is currently ouputs
----- */

bool
BDigitalPort::IsOutput()
{
	return !fIsInput;
}

/* ----------
	SetAsInput - set the digital port to be inputs
----- */
status_t
BDigitalPort::SetAsInput(void)
{
	if (ffd < 0)
		return B_ERROR;
	fIsInput = TRUE;
	return ioctl (ffd, SET_DIGITAL_PORT_DIRECTION, 
		(void *) DIGITAL_PORT_INPUT);
}

/* ----------
	IsInput - return true if portis currently inputs
----- */
bool
BDigitalPort::IsInput()
{
	return fIsInput;
}


/* ----------
	fragile base class padding
----- */

void BDigitalPort::_ReservedDigitalPort1() {}
void BDigitalPort::_ReservedDigitalPort2() {}
void BDigitalPort::_ReservedDigitalPort3() {}
