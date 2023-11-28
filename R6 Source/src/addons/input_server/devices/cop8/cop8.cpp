#include "cop8.h"
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
	return (new Cop8InputDevice());
}


Cop8InputDevice::Cop8InputDevice()
{
	int fd = open("/dev/misc/lcd", O_RDWR);
	if(fd >= 0)
		close(fd);

	fFd = open("/dev/misc/cop8", O_RDWR);
	
	if (fFd < 0)
		return;

	fThread = B_ERROR;
	get_click_speed(&fClickSpeed);

	input_device_ref cop8Device = {"Cop8 Touch Screen", B_POINTING_DEVICE, NULL};
	input_device_ref *devices[2] = {&cop8Device, NULL};
	RegisterDevices(devices);
}


status_t
Cop8InputDevice::Start(
	const char	*device, 
	void		*cookie)
{
	fThread = spawn_thread(cop8er, device, B_DISPLAY_PRIORITY, this);
	resume_thread(fThread);

	return (B_NO_ERROR);

}


status_t
Cop8InputDevice::Stop(
	const char	*device, 
	void		*cookie)
{
	kill_thread(fThread);
	fThread = B_ERROR;

	return (B_NO_ERROR);
}


status_t
Cop8InputDevice::Control(
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


int32
Cop8InputDevice::cop8er(
	void	*arg)
{
	Cop8InputDevice		*device = (Cop8InputDevice *)arg;
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
	uint16				coord[2];

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("cop8_calibration");

	FILE *file = fopen(path.Path(), "r");
	if (file != NULL) {
		if (fscanf(file, "%f %f %f %f %f %f %f %f", &xDelta, &xScale, &xAddY, &xMulY,
		    &yDelta, &yScale, &yAddX, &yMulX) != 8) {
			xDelta = 0.0;
			xScale = 1.0;
			xAddY = 0.0;
			xMulY = 0.0;
			yDelta = 0.0;
			yScale = 1.0;
			yAddX = 0.0;
			yMulX = 0.0;
		}
		
		fclose(file);
	}

	while (read(device->fFd, &coord, sizeof(coord)) == sizeof(coord)) {
		bool move = false;

		if ((coord[0] == 0xFEFE) && (coord[1] == 0x0000)) {
			down = false;
		}
		else if ((coord[0] == 0xFEFE) && ((coord[1] & 0xff) == 0x01)) {
			uint8 buttons = coord[1] >> 8;
			if(buttons & 0x20) {
				//char foo[60];
				//sprintf(foo, "alert back button pressed: %x", buttons);
				//system(foo);
				system("Conductor BACKWARD");
			}
		}
		else {
			down = true;
		
			float kXMin = 0x1900;
			float kXRange = 0xd700 - kXMin;
			float kYMin = 0x2a00;
			float kYRange = 0xd700 - kYMin;
			
			float tmpx = ((float)coord[0] - kXMin) / kXRange;
			float tmpy = ((float)coord[1] - kYMin) / kYRange;
			
			float x = ((tmpx + xDelta) * xScale) * (1 + ((tmpy + yDelta) * yScale) * xMulY);
			float y = ((tmpy + yDelta) * yScale) * (1 + ((tmpx + xDelta) * xScale) * yMulX);

			if(!lastDown) {
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
