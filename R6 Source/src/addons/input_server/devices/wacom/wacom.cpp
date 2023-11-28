#include "wacom.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <dirent.h>
#include <Path.h>
#include <View.h>

void init_from_settings(int driver);


WacomInputDevice* WacomInputDevice::sDevice = NULL;


status_t
convertpositiontostruct(const uint8 *data, tablet_position &);
status_t
convertinfotostruct(const uint8 *data, tablet_info &);
static long 			__reader(void *arg);
static bool	reader_quit = false;
static bool	reader_ready = false;
thread_id	reader_id;

const uint8 kSyncBit		= 7;
const uint8 kProximityBit 	= 6;
const uint8 kPointerBit		= 5;
const uint8 kButtonBit		= 3;
const uint8 kPressureSignBit	= 6;

const uint8	kSyncBitMask = 0x1 << kSyncBit;
const uint8 kProximityBitMask = 0x1 << kProximityBit;
const uint8 kPointerBitMask = 0x1 << kPointerBit;
const uint8 kButtonBitMask = 0x1 << kButtonBit;
const uint8 kPressureSignBitMask = 0x1 << kPressureSignBit;

status_t
convertinfotostruct(const uint8 *data, tablet_info &info)
{	
}

status_t
convertpositiontostruct(const uint8 *data, tablet_position &info)
{	
	if (!(data[0] & kSyncBitMask))
	{
		_sPrintf("convertpositiontostruct - early return, no sync bit\n");
		return B_ERROR;
	}
	
	info.proximity = (data[0] & kProximityBitMask) >> kProximityBit;
	info.stylus = (data[0] & kPointerBitMask) >> kPointerBit;
	info.buttonflag = (data[0] & kButtonBitMask) >> kButtonBit;

	info.x = (data[0] & 0x3) << 14;
	info.x += (data[1] & 0x7f) << 7;
	info.x += (data[2] & 0x7f);
	
	info.buttons = (data[3] & 0x78) >> 3;
	
	info.y = (data[3] & 0x3) << 14;
	info.y += (data[4] & 0x7f) << 7;
	info.y += (data[5] & 0x7f);
		
	info.pressuresign = (data[6] & kPressureSignBitMask) >> kPressureSignBit;
	info.pressuredata = (data[6] & 0x3f);
}

BTSWacomTablet::BTSWacomTablet(const char *port, status_t *err)
{
	fPort.SetDataRate(B_9600_BPS);
	fPort.SetDataBits(B_DATA_BITS_8);
	fPort.SetStopBits(B_STOP_BITS_1);
	fPort.SetParityMode(B_NO_PARITY);
	fPort.SetFlowControl(B_NOFLOW_CONTROL);
	fPort.SetBlocking(false);
	fPort.SetTimeout(250000);		// Timeout of quarter a second

	_sPrintf("WACOM open %s\n", port);
	if (fPortStatus = fPort.Open(port) < 0) {
		_sPrintf("WACOM open failed\n");
		*err = B_ERROR;
		return;
	}
	
	SendCommand("TE\r");

	char testBuf[256];
	GetLine(testBuf, sizeof(testBuf));
	_sPrintf("%s\n", testBuf);

	if (strstr(testBuf, "WACOM") == NULL) {
		_sPrintf("WACOM TE failed\n");
		*err = B_ERROR;
		fPort.Close();
		return;
	}

	_sPrintf("WACOM found tablet!\n");
	fPort.SetBlocking(true);

	SelectIVCommandSet();
	
	// Start port reading thread
	//resume_thread(reader_id = spawn_thread(__reader,"wacom_reader",B_NORMAL_PRIORITY,&fPort));	
	//reader_ready = true;
	// Don't Continue until thread is ready
	// Block on semaphore waiting to be released.
	//while(!reader_ready)
	//	;
		
	strcpy(fCommandTerminator, "\r\n");

	*err = B_NO_ERROR;
}

BTSWacomTablet::~BTSWacomTablet()
{
	int32 dummy;
	
	reader_quit= true;
	fPort.Close();
	//wait_for_thread(reader_id,&dummy);
}

void	
BTSWacomTablet::SendCommand(const char *cmd)
{
	fPort.Write(cmd,strlen(cmd));
}

//
// Retrieve a line of data from the tablet.  It should be
// terminated with a <cr>
void
BTSWacomTablet::GetLine(char *buff, const long buffLen)
{
	//return;
	
	bool done = false;
	char *ptr = buff;
	uint8 aChar;
	long bytestoread;
	long totalread = 0;
	
	while(!done && (totalread < buffLen))
	{
		bytestoread = fPort.WaitForInput();
		if (bytestoread < 1)
			break;

		//_sPrintf("BytesToRead - %d\n", bytestoread);
		while(bytestoread)
		{
			long numRead = fPort.Read(&aChar, 1);
			if (numRead>0)
			{
				//_sPrintf("%c",aChar);
				//fflush(stdout);
				if (aChar == '\r')
				{
					*ptr = '\0';
					done = true;
					break;
				} else
					*ptr = aChar;
				ptr++;
				totalread ++;
			}	
			bytestoread -= numRead;
		}
	}	
}

status_t
BTSWacomTablet::GetBytes(uint8 *bytes, const uint32 numBytes)
{
	long bytestoread = numBytes;
	long numRead;
	uint8 *buffPtr = bytes;
	
	long byteswaiting = fPort.WaitForInput();
	if (byteswaiting < 1)
	{
		return B_ERROR;
	}

	while (bytestoread)
	{
		numRead = fPort.Read(buffPtr, bytestoread);
		buffPtr += numRead;
		bytestoread  -= numRead;
	}

	return B_NO_ERROR;
}


status_t
BTSWacomTablet::GetModel(char *buff)
{
	buff[0] = '\0';
	
	SendCommand("~#\r");
	GetLine(buff,32);
	
	return B_NO_ERROR;
}

status_t	
BTSWacomTablet::GetMaxCoordinates(int32 &x,int32 &y)
{
	char buff[32];

	//SendCommand("~C\r");
	//GetLine(buff,32);
	//sscanf(buff, "~C%05d,%05d", x, y);
	//_sPrintf("GetMaxCoordinates %s %ld %ld\n", buff, x, y);
	x = 6400;
	y = 4780;
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::GetCurrentSettings(char *buff, const int32 buffLen)
{
	SendCommand("~R\r");
	GetLine(buff,buffLen);
	
	return B_NO_ERROR;
}

status_t	
BTSWacomTablet::GetTabletInfo(tablet_info &info)
{
	// Send the individual request command
	SendCommand("RQ1\r");
	
	// Try to get the position information
	uint8 buff[14];
	status_t status = GetBytes(buff, 7);
	
	if (status == B_ERROR)
	{
		_sPrintf("No bytes read.\n");
		return status;
	}

	//status = convertinfotostruct(buff, info);
	
	if (status == B_ERROR)
	{
		_sPrintf("Invalid Data.\n");
		return status;
	}
		
	return B_NO_ERROR;
}

status_t	
BTSWacomTablet::GetPosition(int32 &x, int32 &y, int32 &buttons)
{	
	tablet_position info;
	GetPosition(info);
	
	x = info.x;
	y = info.y;
	buttons = info.buttons;
	
	return B_NO_ERROR;
}

status_t	
BTSWacomTablet::GetPosition(tablet_position &position)
{	
	uint8 buff[128];

	SendCommand("RQ1\r");
	long numRead = fPort.Read(buff, 7);
	convertpositiontostruct(buff, position);
	
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SetASCIIFormat()
{
	SendCommand("AS0\r");
}

status_t
BTSWacomTablet::SetBinaryFormat()
{
	SendCommand("AS0\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SetPointMode()
{
	SendCommand("PO\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SetRelativeMode()
{
	SendCommand("DE1\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SetAbsoluteMode()
{
	SendCommand("DE0\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SelectIISCommandSet()
{
	SendCommand("$\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SelectIVCommandSet()
{
	SendCommand("#\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::Select1201CommandSet()
{
	SendCommand("@\r");
	return B_NO_ERROR;
}

status_t
BTSWacomTablet::SetAlwaysTransmit(const int8)
{
	SendCommand("AL%d\r");
	return B_NO_ERROR;
}

status_t	
BTSWacomTablet::SetTransmissionInterval(const int16)
{
	return B_NO_ERROR;
}


status_t
BTSWacomTablet::Reset()
{
	SendCommand("RE\r");
	fPort.ClearInput();
	fPort.ClearOutput();
	return B_NO_ERROR;
}

// We need a separate reading thread which will take care of just
// sitting around waiting for input.  This way, when there is input
// it can be sent immediately to the receiver.  In some cases it 
// will be the response to a command, and in others it will be
// the continuous stream of positional information.

//=======================================================
// Independent reading thread
//=======================================================
long __reader(void *arg)
{
	BSerialPort    *port = (BSerialPort*)arg;
	
	//_sPrintf("__reader\n");
	char buff[10];
	int numread=0;
	long bytestoread; 
	
	reader_ready = true;
	while (!reader_quit) 
	{
		bytestoread = port->WaitForInput();
		while(bytestoread)
		{
			numread = port->Read(buff,1);
			if (numread < 1)
			{	
				_sPrintf("__reader - numread < 1\n");
				break;
			}	
			_sPrintf("%c",buff[0]);	
			bytestoread -= numread;
		}
		fflush(stdout);
	}

	return 0;
}


BInputServerDevice*
instantiate_input_device()
{
	return (new WacomInputDevice());
}


WacomInputDevice::WacomInputDevice()
	: BInputServerDevice()
{
	sDevice = this;
	fTablet = NULL;
	fWacomer = B_ERROR;

	return;

	DIR *portsDir = opendir("/dev/ports");
	if (portsDir != NULL) {
		struct dirent *thePort = NULL;
		while ((thePort = readdir(portsDir)) != NULL) {
			if ( (strcmp(thePort->d_name, ".") == 0) || 
				 (strcmp(thePort->d_name, "..") == 0) )
				continue;
	
			status_t err = B_NO_ERROR;
			fTablet = new BTSWacomTablet(thePort->d_name, &err);
			if (err == B_NO_ERROR) 
				break;
			else {
				delete (fTablet);
				fTablet = NULL;
			}
		}
	}

	if (fTablet != NULL) {
		input_device_ref	device = {"WACOM Tablet", B_POINTING_DEVICE, NULL};
		input_device_ref	*devices[2] = {
			&device,
			NULL
		};

		RegisterDevices(devices);
	}
}


WacomInputDevice::~WacomInputDevice()
{
	delete (fTablet);
}


status_t
WacomInputDevice::Start(
	const char	*device,
	void		*cookie)
{
	fWacomer = spawn_thread(wacomer, device, B_REAL_TIME_DISPLAY_PRIORITY, fTablet);
	resume_thread(fWacomer);

	return (B_NO_ERROR);
}


status_t
WacomInputDevice::Stop(
	const char	*device,
	void		*cookie)
{
	kill_thread(fWacomer);
	fWacomer = B_ERROR;

	return (B_NO_ERROR);
}


status_t
WacomInputDevice::Control(
	const char	*device,
	void		*cookie,
	uint32		code,
	BMessage	*message)
{
	return (B_NO_ERROR);
}


int32
WacomInputDevice::wacomer(
	void	*arg)
{
	BTSWacomTablet *tablet = (BTSWacomTablet *)arg;

	int32 maxX = 0;
	int32 maxY = 0;
	tablet->GetMaxCoordinates(maxX, maxY);

	int32 saveAbsX = -1;
	int32 saveAbsY = -1;
	int32 saveButtons = 0;

	while (true) {
		int32 x = 0;
		int32 y = 0;
		int32 buttons = 0;

		tablet->GetPosition(x, y, buttons);
		float absX = (float)x / (float)maxX;
		float absY = (float)y / (float)maxY;				
	
		if ((absX != saveAbsX) || (absY != saveAbsY)) {
			BMessage *event = new BMessage(B_MOUSE_MOVED);
			event->AddInt64("when", system_time());
			event->AddFloat("x", absX);
			event->AddFloat("y", absY);
			event->AddInt32("buttons", buttons);

			sDevice->EnqueueMessage(event);	

			saveAbsX = (int32)absX;
			saveAbsY = (int32)absY;
		}

		if (buttons != saveButtons) {
			bool mouseDown = (buttons > 0);
			if (((mouseDown) && (saveButtons < 1)) || ((!mouseDown) && (saveButtons > 0))) {
				//_sPrintf("%d\n", buttons);
				BMessage *event = new BMessage(mouseDown ? B_MOUSE_DOWN : B_MOUSE_UP);
				event->AddInt64("when", system_time());
				event->AddFloat("x", saveAbsX);
				event->AddFloat("x", saveAbsY);
				event->AddInt32("modifiers", modifiers());
				if (event->what == B_MOUSE_DOWN) {
					event->AddInt32("buttons", B_PRIMARY_MOUSE_BUTTON);
					event->AddInt32("clicks", 0);
				}
	
				sDevice->EnqueueMessage(event);
			}

			saveButtons = buttons;
		}
	}

	return (B_NO_ERROR);
}


void
init_from_settings(
	int	driver)
{
}



