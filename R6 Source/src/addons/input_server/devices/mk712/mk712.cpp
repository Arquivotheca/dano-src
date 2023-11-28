#include "mk712.h"
#include <Beep.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <InputServerDevice.h>
#include <Path.h>
#include <View.h>
#include <string.h>


BInputServerDevice*
instantiate_input_device()
{
	return (new mk712InputDevice());
}


mk712InputDevice::mk712InputDevice()
{
	fFd = open("/dev/input/touchscreen/mk712/0", O_RDWR);
	
	if (fFd < 0)
		return;

	fThread = B_ERROR;
	fPortThread = B_ERROR;
	fPort = B_ERROR;
	raw_mode=false;
	get_click_speed(&fClickSpeed);

	input_device_ref mk712Device = {"mk712 Touch Screen", B_POINTING_DEVICE, NULL};
	input_device_ref *devices[2] = {&mk712Device, NULL};
	RegisterDevices(devices);
}


status_t
mk712InputDevice::Start(
	const char	*device, 
	void		*cookie)
{
	fPort=create_port(10,"dt300 control port");
	fPortThread = spawn_thread(mk712portreader, device, B_DISPLAY_PRIORITY, this);
	resume_thread(fPortThread);

	fThread = spawn_thread(mk712er, device, B_DISPLAY_PRIORITY, this);
	resume_thread(fThread);

	return (B_NO_ERROR);

}


status_t
mk712InputDevice::Stop(
	const char	*device, 
	void		*cookie)
{
	kill_thread(fThread);
	fThread = B_ERROR;
	delete_port(fPort);
	long dummy;
	wait_for_thread(fPortThread,&dummy);
	fPortThread = B_ERROR;

	return (B_NO_ERROR);
}


status_t
mk712InputDevice::Control(
	const char	*device,
	void		*cookie,
	uint32		code, 
	BMessage	*message)
{
	switch (code) {
		case B_CLICK_SPEED_CHANGED:
			get_click_speed(&fClickSpeed); 		
			break;
	
		default:
			break;
	}

	return (B_NO_ERROR);
}

typedef struct {
	bigtime_t  time;
	uint16     x;
	uint16     y;
	bool       pressed;
} sample_t;

int32 mk712InputDevice::mk712portreader(void	*arg)
{
	//WhatWithTimeout("loop start");
	mk712InputDevice	*device = (mk712InputDevice *)arg;

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
			//int32 *cnb=(int32*)&buffer;
			//device->contrast=*cnb++;
			//device->brightness=*cnb++;
			//device->UpdateBrightNessContrast();
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
mk712InputDevice::mk712er(
	void	*arg)
{
	mk712InputDevice		*device = (mk712InputDevice *)arg;
	int32				clickCount = 0;
	float				saveX = -1.0;
	float				saveY = -1.0;	
	bigtime_t			saveWhen = 0;
	bool				down = false;
	bool				lastDown = false;
	float				xDelta = 0.0;
	float				xScale = 1.0;
	float				xAddY = 0.0;
	float				xMulY = 0.0;
	float				yDelta = 0.0;
	float				yScale = 1.0;
	float				yAddX = 0.0;
	float				yMulX = 0.0;
	sample_t			sample;

	device->recalibrate=true;
	device->temp_recalibrate=false;

	while (read(device->fFd, &sample, sizeof(sample)) == sizeof(sample)) {
		bool move = false;

		if(device->recalibrate || device->temp_recalibrate)
		{
			int32 numparsed=0;
			if(device->temp_recalibrate)
			{
				numparsed = sscanf(device->temp_recalibrationstring,
				                   "%f %f %f %f %f %f %f %f",
				                   &xDelta, &xScale, &xAddY, &xMulY,
				                   &yDelta, &yScale, &yAddX, &yMulX);
			}
			else {
				BPath path;
				find_directory(B_USER_SETTINGS_DIRECTORY, &path);
				path.Append("dt300_calibration");
			
				FILE *file = fopen(path.Path(), "r");
				if (file != NULL) {
					numparsed = fscanf(file, "%f %f %f %f %f %f %f %f",
					                   &xDelta, &xScale, &xAddY, &xMulY,
					                   &yDelta, &yScale, &yAddX, &yMulX);
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

		if(!sample.pressed) {
			down = false;
		}
		else {
			down = true;
		
			float kXMin = 0x000;
			float kXRange = 0x1000;
			float kYMin = 0x000;
			float kYRange = 0x1000;
			
#if ROTATE_DISPLAY
			tmpy = ((float)sample.x - kXMin) / kXRange;
			tmpx = ((float)sample.y - kYMin) / kYRange;
#else
			tmpx = ((float)sample.x - kXMin) / kXRange;
			tmpy = ((float)sample.y - kYMin) / kYRange;
#endif
			
			float x = ((tmpx + xDelta) * xScale) * (1 + ((tmpy + yDelta) * yScale) * xMulY);
			float y = ((tmpy + yDelta) * yScale) * (1 + ((tmpx + xDelta) * xScale) * yMulX);

			if(down == lastDown) {
				float dx = x - saveX;
				float dy = y - saveY;
				const float maxdiff = 0.01;
				if(dx > -maxdiff && dx < maxdiff &&
				   dy > -maxdiff && dy < maxdiff) {
				   x = saveX;
				   y = saveY;
				}
			}

			if ((x != saveX) || (y != saveY)) {
				move = true;
				saveX = x;
				saveY = y;
			}
			
		}

		if ((!move) && (down == lastDown))
			continue;

		if (move) {
			BMessage *event = new BMessage(B_MOUSE_MOVED);

			event->AddInt64("when", system_time());
			event->AddFloat("x", saveX);		
			event->AddFloat("y", saveY);	
			event->AddInt32("buttons", (lastDown) ? B_PRIMARY_MOUSE_BUTTON : 0);

			device->EnqueueMessage(event);
		}
		if(down && device->raw_mode) {
			BMessage event('rawm');
			event.AddInt64("when", system_time());
			event.AddFloat("x", tmpx);
			event.AddFloat("y", tmpy);
			event.AddInt32("buttons", (lastDown) ? B_PRIMARY_MOUSE_BUTTON : 0);
			device->rawmsngr.SendMessage(&event);
		}

		if (down != lastDown) {
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

			lastDown = down;
			device->EnqueueMessage(event);
		}
	}

	return (B_NO_ERROR);
}
