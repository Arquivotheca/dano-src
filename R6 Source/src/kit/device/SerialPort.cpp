//******************************************************************************
//
//	File:		SerialPort.cpp
//
//	Description:	BSerialPort class.
//			Implementation for a Serial Port class.
//	
//	Written by:	Erich Ringewald
//
//	Copyright 1995, Be Incorporated, All Rights Reserved.
//
//
//******************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <dirent.h>

#include "SerialPort.h"
#include <BeBuild.h>	// for _PR2_COMPATIBLE_
#include <List.h>
#include <Debug.h>


//------------------------------------------------------------

BSerialPort::BSerialPort()
{

	ffd = -1;
	fBaudRate = B_19200_BPS;				// set default line character
	fDataBits = B_DATA_BITS_8;
	fStopBits = B_STOP_BITS_1;
	fParityMode = B_NO_PARITY;
	fBlocking = TRUE;						// Default is blcoking reads
	fTimeout = B_INFINITE_TIMEOUT;			// and to block forever
	fFlow = B_HARDWARE_CONTROL;				// with HW flow

	_fDevices = NULL;
}

BSerialPort::~BSerialPort()
{
	close(ffd);
	if (_fDevices != NULL) for (int ix=0; ix<_fDevices->CountItems(); ix++) {
		free(_fDevices->ItemAt(ix));
	}
	delete _fDevices;
}

// Actually open the port.  Since this might fail, we have to do this
// outside of the constructor.
// valid port names are "serial1", "serial2", "serial3", and "serial4"

#if _PR2_COMPATIBLE_
/* LCS open /dev/midi1 as a BSerialPort. */
/* I promised James that we'd keep working with old binaries on PPC */
static struct legacy {
	const char * old_name;
	const char * new_name;
} legacy_ports[] = {
	{ "/dev/midi1", "/dev/midi/midi1" },
	{ "/dev/midi2", "/dev/midi/midi2" },
	{ NULL, NULL }
};
#endif /* _PR2_COMPATIBLE_ */

status_t
BSerialPort::Open(const char *portName)
{
	char  buf[64];
#if _PR2_COMPATIBLE_
	const struct legacy * ptr = legacy_ports;

	while (ptr->old_name != NULL) {
		if (!strcmp(portName, ptr->old_name)) {
			strcpy(buf, ptr->new_name);
			goto legacy_fix;
		}
		ptr++;
	}
#endif /* _PR2_COMPATIBLE_ */

	if (*portName == '/') {
		sprintf(buf, "%s", portName);
	} else {
		sprintf(buf, "/dev/ports/%s", portName);		// prepend "/dev/"
	}

legacy_fix:
	if (ffd >= 0)								// it's already open
		close(ffd);								// close to reopen

	ffd = open( buf, O_NONBLOCK|O_RDWR);
	if (ffd >= 0) {
		struct termios t;
		fcntl( ffd, F_SETFL, fcntl( ffd, F_GETFL)&~O_NONBLOCK);
		ioctl( ffd, TCGETA, &t);
		t.c_cflag |= CLOCAL;
		ioctl( ffd, TCSETA, &t);
		DriverControl();
	}

	return(ffd >= 0 ? ffd : errno);
}

void
BSerialPort::Close()
{
	close(ffd);
	ffd = -1;
}

ssize_t
BSerialPort::Read(void *buf, size_t count)
{
	ssize_t res;

	res = read( ffd, buf, count);
	if (res < 0) {
		return (errno);
	}
	return (res);
}

ssize_t
BSerialPort::Write(const void *buf, size_t count)
{
	ssize_t res;

	res = write(ffd, buf, count);
	if (res < 0) {
		return (errno);
	}
	return (res);
}

void
BSerialPort::SetBlocking(bool BlockingFlag)
{
	fBlocking = BlockingFlag;
	DriverControl();
}

static int
micro2deci( bigtime_t us)
{

	if (us == B_INFINITE_TIMEOUT)
		return (0);
	uint i = (us+100000/2) / 100000;
	return (i? i: 1);
}

status_t
BSerialPort::SetTimeout(bigtime_t Timeout)
{

	uint i = micro2deci( Timeout);
	if (i < 256) {
		fTimeout = Timeout;
		DriverControl( );
		return (B_OK);
	}
	return (B_BAD_VALUE);
}

// Set the BaudRate.

status_t
BSerialPort::SetDataRate(data_rate bps)
{
	fBaudRate = bps;
	return (DriverControl( ));
}

data_rate
BSerialPort::DataRate(void)
{
	return(fBaudRate);
}

void
BSerialPort::SetDataBits(data_bits numBits)
{
	fDataBits = numBits;
	DriverControl();
}

data_bits
BSerialPort::DataBits(void)
{
	return(fDataBits);
}

void
BSerialPort::SetStopBits(stop_bits numBits)
{
	fStopBits = numBits;
	DriverControl();
}

stop_bits
BSerialPort::StopBits(void)
{
	return(fStopBits);
}

void
BSerialPort::SetParityMode(parity_mode pm)
{
	fParityMode = pm;
	DriverControl();
}

parity_mode
BSerialPort::ParityMode(void)
{
	return(fParityMode);
}

void
BSerialPort::ClearInput()
{
	ioctl(ffd, TCFLSH, 0);
	return;
}

void
BSerialPort::ClearOutput()
{
	ioctl(ffd, TCFLSH, 1);
	return;
}

void
BSerialPort::SetFlowControl(uint32 mask)
{
	fFlow = mask & (B_HARDWARE_CONTROL | B_SOFTWARE_CONTROL);
	DriverControl();
}

uint32
BSerialPort::FlowControl(void)
{
	return fFlow;
}

status_t
Ioctl(int fd, int opcode, void *buf)
{
	status_t res;

	res = ioctl(fd, opcode, buf);
	if (res < 0) {
		return (errno);
	}
	return (res);
}

//
// Send a control message to the driver to ensure that the current
// settings are set.

int
BSerialPort::DriverControl()
{
struct termios	tctl;

	if (ffd < 0)						// cant do this without a driver
		return (B_NO_INIT);

	if (ioctl( ffd, TCGETA, &tctl) < 0)
		return (errno);

	// turn off all of the bits which we will turn on again, if necessary

	tctl.c_iflag &= ~(IXON | IXOFF | IXANY | INPCK);
	tctl.c_cflag &= ~(CSIZE | CBAUD | CSTOPB | PARENB | PARODD | CTSFLOW | RTSFLOW);
	tctl.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG);

#if __INTEL__
	tctl.c_cc[VMIN] = 0;
	tctl.c_cc[VTIME] = 0;
	if (fBlocking)
		if (fTimeout == B_INFINITE_TIMEOUT)
			tctl.c_cc[VMIN] = 1;
		else
			tctl.c_cc[VTIME] = micro2deci( fTimeout);
#elif __POWERPC__
	tctl.c_cc[VMIN] = fBlocking;
	tctl.c_cc[VTIME] = micro2deci( fTimeout);
#else
	#error - Functionality not defined for this architecture -
#endif

	if (fFlow & B_SOFTWARE_CONTROL)
		tctl.c_iflag |= (IXON | IXOFF);

	if (fFlow & B_HARDWARE_CONTROL)
		tctl.c_cflag |= (CTSFLOW | RTSFLOW);

	if (fDataBits == B_DATA_BITS_8)
		tctl.c_cflag |= CS8;	
	if (fStopBits == B_STOP_BITS_2)
		tctl.c_cflag |= CSTOPB;	
	if (fParityMode != B_NO_PARITY) {
		tctl.c_cflag |= PARENB;
		if (fParityMode == B_ODD_PARITY)
			tctl.c_cflag |= PARODD;
	}
	tctl.c_cflag |= (fBaudRate & CBAUD);
	return Ioctl(ffd, TCSETA, &tctl);
}

status_t
BSerialPort::SetDTR(bool assert)
{
	return Ioctl(ffd, TCSETDTR, &assert);
}

status_t
BSerialPort::SetRTS(bool assert)
{
	return Ioctl(ffd, TCSETRTS, &assert);
}

bool
BSerialPort::IsCTS()
{
	unsigned int	thebits;
	int res;

	res = ioctl(ffd, TCGETBITS, &thebits);
	if (res < 0) {
		return FALSE;
	}
	if (thebits & TCGB_CTS)
		return TRUE;
	else
		return FALSE;
}

bool
BSerialPort::IsDSR()
{
	unsigned int	thebits;
	int res;

	res = ioctl(ffd, TCGETBITS, &thebits);
	if (res < 0) {
		return FALSE;
	}
	if (thebits & TCGB_DSR)
		return TRUE;
	else
		return FALSE;
}

bool
BSerialPort::IsRI()
{
	unsigned int	thebits;
	int res;

	res = ioctl(ffd, TCGETBITS, &thebits);
	if (res < 0) {
		return FALSE;
	}
	if (thebits & TCGB_RI)
		return TRUE;
	else
		return FALSE;
}

bool
BSerialPort::IsDCD()
{
	unsigned int	thebits;
	int res;

	res = ioctl(ffd, TCGETBITS, &thebits);
	if (res < 0) {
		return FALSE;
	}
	if (thebits & TCGB_DCD)
		return TRUE;
	else
		return FALSE;
}

status_t
BSerialPort::NumCharsAvailable(int32 *wait_for_this_many)
{
	status_t res;

	res = ioctl(ffd, 'ichr', wait_for_this_many);
	return res;
}


ssize_t
BSerialPort::WaitForInput()
{
	int len;
	int status;

#if __INTEL__
	uint vtime;
	if ((fBlocking == 0)
	&& (vtime = micro2deci( fTimeout))) {
		struct termio t, tsave;
		ioctl( ffd, TCGETA, &t);
		tsave = t;
		t.c_cc[VTIME] = vtime;
		ioctl( ffd, TCSETA, &t);
		status = ioctl( ffd, TCWAITEVENT, &len, sizeof( len));
		ioctl( ffd, TCSETA, &tsave);
	}
	else
		status = ioctl( ffd, TCWAITEVENT, &len, sizeof( len));
#elif __POWERPC__
	status = ioctl( ffd, TCWAITEVENT, &len, sizeof( len));
#else
	#error - Functionality not defined for this architecture -
#endif
	if (status < B_NO_ERROR)
		return (errno);
	return (len);
}

/* ----------
	fragile base class padding
----- */

void BSerialPort::_ReservedSerialPort1() {}
void BSerialPort::_ReservedSerialPort2() {}
void BSerialPort::_ReservedSerialPort3() {}
void BSerialPort::_ReservedSerialPort4() {}

/* device roster -- tell user about available valid choices */

int32
BSerialPort::CountDevices()
{
	if (_fDevices == NULL) {
		ScanDevices();
	}
	return _fDevices->CountItems();
}

status_t
BSerialPort::GetDeviceName(
	int32 n,
	char * name,
	size_t bufSize)
{
	if (_fDevices == NULL) {
		ScanDevices();
	}
	if ((n<0) || (n>=_fDevices->CountItems())) {
		return B_BAD_INDEX;
	}
	if (strlen((char*)_fDevices->ItemAt(n)) >= bufSize) {
		return B_NAME_TOO_LONG;
	}
	strcpy(name, (char*)_fDevices->ItemAt(n));
	return B_OK;
}

static void
recursive_scan(
	const char * prefix,
	BList * dest)
{
	char * path = (char *)malloc(MAXPATHLEN);
	struct stat stbuf;
	struct dirent * dent;
	DIR * dir;

	if (!path) {
		return; /* out of memory */
	}
	if ((dir = opendir(prefix)) == NULL) {
		free(path);
		return;
	}
	while ((dent = readdir(dir)) != NULL) {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) {
			continue;
		}
		strcpy(path, prefix);
		strcat(path, "/");
		strcat(path, dent->d_name);
		if (stat(path, &stbuf)) {
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
			recursive_scan(path, dest);
		}
		else {
			/* remove the /dev/ports/ prefix (11 chars) */
			ASSERT(!strncmp(path, "/dev/ports/", 11));
			char * copy = strdup(path+11);
			if (copy != NULL) {
				dest->AddItem(copy);
			}
		}
	}
	closedir(dir);
	free(path);
}

void
BSerialPort::ScanDevices()
{
	if (_fDevices != NULL) for (int ix=0; ix<_fDevices->CountItems(); ix++) {
		free(_fDevices->ItemAt(ix));
	}
	else {
		_fDevices = new BList;
	}
	_fDevices->MakeEmpty();
	recursive_scan("/dev/ports", _fDevices);
}
