#include "elo.h"
#include "utils.h"
#include "controls.h"
#include <Beep.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <InputServerDevice.h>
#include <Path.h>
#include <View.h>
#include <string.h>
#include <stdio.h>
#include <Roster.h>

#include <PlaySound.h>

const char *BRIGHTNESSFILE="dt300_contrast_brightness";

#if 0
const int MINCONTRAST=100;
const int MAXCONTRAST=200;
const int MINBRIGHTNESS=100;
const int MAXBRIGHTNESS=200;
#else
const int MINCONTRAST=0;
const int MAXCONTRAST=255;
const int MINBRIGHTNESS=0;
const int MAXBRIGHTNESS=255;
#endif

typedef struct event {
	enum { NONE=0, PENDOWN, PENUP, BUTTONS } type;
	unsigned short x;
	unsigned short y;
} event;

BInputServerDevice*
instantiate_input_device()
{
//	WhatWithTimeout("instantiate_input_device\n");
	return (new DT300InputDevice());
}


DT300InputDevice::DT300InputDevice()
{
//	WhatWithTimeout("created device");
	fSerialPort.SetDataRate(B_9600_BPS);
	fSerialPort.SetDataBits(B_DATA_BITS_8);
	fSerialPort.SetStopBits(B_STOP_BITS_1);
	fSerialPort.SetParityMode(B_NO_PARITY);
	fSerialPort.SetFlowControl(B_HARDWARE_CONTROL);
	fSerialPort.SetBlocking(true);
	fSerialPort.SetTimeout(B_INFINITE_TIMEOUT);

	if (fSerialPort.Open("/dev/ports/serial2") < 0)
		return;

	get_click_speed(&fClickSpeed);

	fControlWindow = 0;
	fDeviceThread = B_ERROR;
	fPortThread = B_ERROR;
	fPort = B_ERROR;
	raw_mode=false;
	fBeepRef.device = -1;
	fBeepHandle = -1;

	input_device_ref eloDevice = {"Elo Touch Screen", B_POINTING_DEVICE, NULL};
	input_device_ref *devices[2] = {&eloDevice, NULL};
	RegisterDevices(devices);}

void DT300InputDevice::UpdateBrightNessContrast()
{
}

status_t DT300InputDevice::Start(
	const char	*device, 
	void		*cookie)
{
//	WhatWithTimeout("Start\n");

	contrast=125;
	brightness=125;
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(BRIGHTNESSFILE);
	FILE *file = fopen(path.Path(), "r");
	if (file != NULL) 
	{
		if (fscanf(file, "%d %d", &contrast, &brightness) != 2) 
		{
			contrast=125;
			brightness=125;
		}
	}
	UpdateBrightNessContrast();

	// get all controls' default values
	for (int i = 0; i < sizeof(controls)/sizeof(controls[0]); i++) {
		switch(controls[i].type) {
		case BRIGHTNESS_CONTROL:			
			controls[i].current = 100.0/(MAXCONTRAST-MINCONTRAST)*100.0;
_sPrintf("default brightness %.1f\n", controls[i].current);
			break;
		case CONTRAST_CONTROL:
			controls[i].current = 100.0/(MAXCONTRAST-MINCONTRAST)*100.0;
_sPrintf("default contrast %.1f\n", controls[i].current);
			break;
		case VOLUME_CONTROL:
			float vol;
			bool mute;
//			if (mute)
//				controls[i].current = 0.0;
//			else
				controls[i].current = vol * 100.0;
_sPrintf("default volume %.1f\n", controls[i].current);
			break;
		}
	}
	
	fPort=create_port(10,"dt300 control port");
	fPortThread = spawn_thread(dt300portreader, device, B_DISPLAY_PRIORITY, this);
	resume_thread(fPortThread);

	fDeviceThread = spawn_thread(dt300er, device, B_DISPLAY_PRIORITY, this);
	resume_thread(fDeviceThread);

	return (B_NO_ERROR);

}


status_t
DT300InputDevice::Stop(
	const char	*device, 
	void		*cookie)
{
//	WhatWithTimeout("Stop\n");
	kill_thread(fDeviceThread);
	fDeviceThread = B_ERROR;
	
	delete_port(fPort);
	long dummy;
	wait_for_thread(fPortThread,&dummy);

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(BRIGHTNESSFILE);
	FILE *file = fopen(path.Path(), "w");
	if (file != NULL)
	{
		fprintf(file,"%d %d\n",brightness,contrast);
		fclose(file);
	}
	
	return (B_NO_ERROR);
}


status_t
DT300InputDevice::Control(
	const char	*device,
	void		*cookie,
	uint32		code, 
	BMessage	*message)
{
	//WhatWithTimeout("Control\n");
	switch (code) {
		case B_CLICK_SPEED_CHANGED:
			get_click_speed(&fClickSpeed); 		
			break;
	
		default:
			break;
	}

	return (B_NO_ERROR);
}



int32 DT300InputDevice::dt300portreader(void	*arg)
{
	//WhatWithTimeout("loop start");
	DT300InputDevice	*device = (DT300InputDevice *)arg;

	int32 msg_code;
	char buffer[200];

	while(read_port(device->fPort,&msg_code,&buffer,sizeof(buffer))>=0)
	{
		if(msg_code==0)
		{
			//WhatWithTimeout("translated mode");
			device->raw_mode=false;
		}
		else if(msg_code==1)
		{
			//WhatWithTimeout("raw mode");
			device->rawmsngr=*((BMessenger*)buffer);
			device->raw_mode=true;
		}
		else if(msg_code==2)
		{
			//WhatWithTimeout("recalibrating for real");
			device->recalibrate=true;
		}
		else if(msg_code==3)
		{
			//WhatWithTimeout("button event");
			int32 *cnb=(int32*)&buffer;
			device->contrast=*cnb++;
			device->brightness=*cnb++;
			device->UpdateBrightNessContrast();
		}
		else if(msg_code==4)
		{
			//WhatWithTimeout("recalibrating temporarily");
			device->temp_recalibrate=true;
			strncpy(device->temp_recalibrationstring,buffer,sizeof(buffer));
		}
	}
	return B_OK;
}

int32
DT300InputDevice::dt300er(
	void	*arg)
{
	//WhatWithTimeout("loop start\n");
	DT300InputDevice	*device = (DT300InputDevice *)arg;
	int32				clickCount = 0;
	float				saveX = -1.0;
	float				saveY = -1.0;	
	bigtime_t			saveWhen = 0;
	bool				saveDown = false;
	float				xDelta = 0.0;
	float				xScale = 1.0;
	float				xAddY = 0.0;
	float				xMulY = 0.0;
	float				yDelta = 0.0;
	float				yScale = 1.0;
	float				yAddX = 0.0;
	float				yMulX = 0.0;
	event				e;


	int sizeread;
	device->recalibrate=true;
	device->temp_recalibrate=false;

	const int32			kBufSize = 10;
	int32				numChars = kBufSize;
	while (device->fSerialPort.NumCharsAvailable(&numChars) == B_NO_ERROR) 
	{
//		_sPrintf("numcharswaiting: %d\n",numChars);
		numChars=kBufSize;
//		WhatWithTimeout("loop running\n");
		uchar	buf[kBufSize];
		int32	bufOffset = 0;
		int32	bytesToRead = kBufSize;
		int32	bytesRead = 0;

		memset(buf, 0, kBufSize);

		while ((bytesRead = device->fSerialPort.Read(buf + bufOffset, bytesToRead)) > 0) {
			if ((bytesRead == bytesToRead) && (buf[0] == 'U'))
				break;

			if (buf[0] == 'U') {
				bufOffset += bytesRead;
				bytesToRead -= bytesRead;
			}			
			else {
				for (int32 uOffset = 0; uOffset < bytesRead; uOffset++) {
					if (buf[uOffset] == 'U') {
						memmove(buf, buf + uOffset, bytesRead - uOffset);
						bufOffset = bytesRead - uOffset;
						bytesToRead = kBufSize - bufOffset;
						break;
					}
				}
			}
		}

		e.x = (buf[4] << 8) | buf[3];
		e.y = (buf[6] << 8) | buf[5];

		bool	down = buf[2]&1;
		bool	up = buf[2]&4;
		bool	move = false;

	//    _sPrintf("elo: %02x %02x %02x ... %02x\n",buf[0],buf[1],buf[2],buf[9]);

		if(device->recalibrate || device->temp_recalibrate)
		{
			//WhatWithTimeout("recalibrate");
			int32 numparsed=0;
			if(device->temp_recalibrate)
			{
				numparsed=sscanf(device->temp_recalibrationstring, "%f %f %f %f %f %f %f %f", &xDelta, &xScale, &xAddY, &xMulY, &yDelta, &yScale, &yAddX, &yMulX);

			}
			else
			{
				BPath path;
				find_directory(B_USER_SETTINGS_DIRECTORY, &path);
				path.Append("dt300_calibration");
				FILE *file = fopen(path.Path(), "r");
				if(file != NULL)
				{
					numparsed=fscanf(file, "%f %f %f %f %f %f %f %f", &xDelta, &xScale, &xAddY, &xMulY, &yDelta, &yScale, &yAddX, &yMulX);
					fclose(file);
				}
			}
			if(numparsed!=8)
			{
				xDelta = 0.0;
				xScale = 1.0;
				xAddY = 0.0;
				xMulY = 0.0;
				yDelta = 0.0;
				yScale = 1.0;
				yAddX = 0.0;
				yMulX = 0.0;
			}
			device->recalibrate=false;
			device->temp_recalibrate=false;
		}

		float tmpx;
		float tmpy;

		tmpx=e.x;
		tmpy=e.y;
			
		float x = ((tmpx + xDelta) * xScale) * (1 + ((tmpy + yDelta) * yScale) * xMulY);
		float y = ((tmpy + yDelta) * yScale) * (1 + ((tmpx + xDelta) * xScale) * yMulX);

		if(!down) 
		{
			float dx = x - saveX;
			float dy = y - saveY;
			const float maxdiff = 0.01;
			if(dx > -maxdiff && dx < maxdiff &&
			   dy > -maxdiff && dy < maxdiff || system_time() - saveWhen < 250000) {
			   x = saveX;
			   y = saveY;
			}
		}

		if ((x != saveX) || (y != saveY)) {
			move = true;
			saveX = x;
			saveY = y;
		}
			

		if ((!move) && (!down) && (!up))
			continue;

		if (move) {
			BMessage *event = new BMessage(B_MOUSE_MOVED);
			event->AddInt64("when", system_time());
			event->AddFloat("x", saveX);
			event->AddFloat("y", saveY);
			event->AddInt32("buttons", (saveDown) ? B_PRIMARY_MOUSE_BUTTON : 0);
			device->EnqueueMessage(event);
		}
		if(down&&device->raw_mode)
		{
			BMessage event('rawm');
			event.AddInt64("when", system_time());
			event.AddFloat("x", tmpx);
			event.AddFloat("y", tmpy);
			event.AddInt32("buttons", (saveDown) ? B_PRIMARY_MOUSE_BUTTON : 0);
			device->rawmsngr.SendMessage(&event);
		}

		if ((down) || (up)) 
		{
			bigtime_t	now = system_time();
			BMessage	*event = new BMessage((down) ? B_MOUSE_DOWN : B_MOUSE_UP);

			event->AddInt64("when", now);
			event->AddInt32("x", 0);
			event->AddInt32("y", 0);
			event->AddInt32("buttons", (down) ? B_PRIMARY_MOUSE_BUTTON : 0);

			if (down) {
				if ((saveWhen + device->fClickSpeed) > now)
					clickCount++;
				else
					clickCount = 1;
	 
				event->AddInt32("clicks", clickCount);

				saveWhen = now;
			}

			device->EnqueueMessage(event);
			saveDown=down;
		}
	}
//	char buf[200];
//	sprintf(buf,"loop stopped: %d/%d\n",sizeread,sizeof(e));
//	WhatWithTimeout(buf);

	return (B_NO_ERROR);
}

void
DT300InputDevice::HandleButtons(unsigned short buttons)
{
	control *ctl = 0;
	
	if (buttons & 0x2)				// lower left
		ctl = &controls[0];
	else if (buttons & 0x1)			// upper left
		ctl = &controls[1];
	else if (buttons & 0x4)			// lower right
		ctl = &controls[2];
	else if (buttons & 0x8)			// upper right
		ctl = &controls[3];
	
	if (ctl) {
		if (ctl->type == EXECUTE_COMMAND) {
			entry_ref ref;
			get_ref_for_path(ctl->name, &ref);
			be_roster->Launch(&ref);
			return;
		}

		if (ctl->type == TOGGLE_SOFTKB) {
			ToggleSoftKeyboard();
			return;
		}
		
		if (!fControlWindow) {
			fControlWindow = new ControlWindow(this, ctl);
			fControlWindow->Show();
		}
		else {
			if (fControlWindow->Lock()) {
				fControlWindow->Switch(ctl);
				fControlWindow->Unlock();
			}
		}
		return;
	}

	// if there is no control window, the up/down buttons
	// have no effect.
	if (!fControlWindow)
		return;
				
	if (buttons & 0x20)
		fControlWindow->Increase();
	else if (buttons & 0x10)
		fControlWindow->Decrease();
}

void
DT300InputDevice::ControlChanged(control *ctl)
{
	switch(ctl->type) {
	case BRIGHTNESS_CONTROL:
		brightness =
			(int)((ctl->current/100.0)*(MAXBRIGHTNESS-MINBRIGHTNESS))+MINBRIGHTNESS;
_sPrintf("Brightness: ctl->current = %.1f\n", ctl->current);
		// reverse -- maxbrightness is darkest for some reason
		brightness = (MAXBRIGHTNESS-MINBRIGHTNESS) - brightness + MINBRIGHTNESS;
		UpdateBrightNessContrast();
		break;
	case CONTRAST_CONTROL:
		contrast =
			(int)((ctl->current/100.0)*(MAXCONTRAST-MINCONTRAST))+MINCONTRAST;
		// reverse
		contrast = (MAXCONTRAST-MINCONTRAST) - contrast + MINCONTRAST;
		UpdateBrightNessContrast();
		break;
	case VOLUME_CONTROL:
		if (fBeepRef.device == -1) {
			BEntry entry("/etc/sounds/BeBeep", TRUE);
			if (entry.Exists())
				entry.GetRef(&fBeepRef);
		}		
		float mini_val = ctl->current/100.0;
		stop_sound(fBeepHandle);
		fBeepHandle = play_sound(&fBeepRef, true, true, true);
		break;
	}
}

void
DT300InputDevice::ControlWindowVanished()
{
	fControlWindow = 0;
}

const char kInputServerSignature[] = "application/x-vnd.Be-input_server";

void
DT300InputDevice::ToggleSoftKeyboard()
{
	BMessage reply;

	// get a messenger to the soft keyboard if we
	// don't have one already
	if (!fSoftKeyboard.IsValid()) {
		BMessenger m(kInputServerSignature, -1, NULL);
		if (m.SendMessage('hack', &reply))
			return;
		if (reply.FindMessenger("result", &fSoftKeyboard))
			return;
	}

	// get the current state
	bool visible;
	if (fSoftKeyboard.SendMessage('visb', &reply))
		return;
	if (reply.FindBool("visible", &visible))
		return;

	// toggle it
	BMessage msg('actv');
	msg.AddBool("active", !visible);
	msg.AddRect("frame", BRect(0,0,0,0));
	fSoftKeyboard.SendMessage(&msg, &reply);	
}
