#include "AcerWPTouchDevice.h"

#include <string.h>

#include <Beep.h>
#include <Entry.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <Path.h>
#include <Roster.h>
#include <View.h>

const char kByline[] = "AcerWPTouchDevice: ";
const char kDevicePath[] = "/dev/input/touchscreen/acer_wp_touch/0";
const char kCalibratePath[] = "/boot/beos/preferences/calibrate2";

BInputServerDevice*
instantiate_input_device(void)
{
	return new AcerWPTouchDevice();
}

AcerWPTouchDevice::AcerWPTouchDevice(void)
	: mPortThread(B_ERROR), mPort(B_ERROR), mRawMode(false),
		mThread(B_ERROR)
{
	status_t err;
	
	mFileDesc = open(kDevicePath, O_RDWR);
	if (mFileDesc < 0) {
		SERIAL_PRINT((kByline "Error opening %s: %s\n", kDevicePath,
			strerror(mFileDesc)));
		return;
	}
	
	get_click_speed(&mClickSpeed);
	
	input_device_ref acerWPTouch = { "Acer Webpad Touch Screen",
		B_POINTING_DEVICE, NULL };
	input_device_ref* devices[2] = { &acerWPTouch, NULL };
	
	err = RegisterDevices(devices);
	if (err != B_OK) {
		SERIAL_PRINT((kByline "Error: RegisterDevices() said %s\n",
			strerror(err)));
	}
}

status_t
AcerWPTouchDevice::Start(const char* device, void* cookie)
{
	mPort = create_port(10, "dt300 control port");
	mPortThread = spawn_thread(AcerWPTouchPortReader, "AcerWPTouchPortReader",
		B_DISPLAY_PRIORITY, this);
	resume_thread(mPortThread);
	
	mThread = spawn_thread(AcerWPToucher, "AcerWPToucher", B_DISPLAY_PRIORITY,
		this);
	resume_thread(mThread);
	
	return B_OK;
}

status_t
AcerWPTouchDevice::Stop(const char* device, void* cookie)
{
	status_t dummy;
	
	kill_thread(mThread);
	mThread = B_ERROR;
	delete_port(mPort);
	wait_for_thread(mPortThread, &dummy);
	mPortThread = B_ERROR;
	
	return B_OK;
}

status_t
AcerWPTouchDevice::Control(const char* device, void* cookie, uint32 code,
	BMessage* message)
{
	switch (code) {
		case B_CLICK_SPEED_CHANGED:
			get_click_speed(&mClickSpeed);
			break;
		default:
			break;
	}
	
	return B_OK;
}

// needs to match the one in the driver; should be in a header file
typedef struct {
	bigtime_t time;
	uint16 x;
	uint16 y;
	bool pressed;
} sample_t;

int32
AcerWPTouchDevice::AcerWPTouchPortReader(void* data)
{
	AcerWPTouchDevice* device = (AcerWPTouchDevice*)data;
	
	int32 what;
	char buffer[200];
	
	while (read_port(device->mPort, &what, &buffer, sizeof (buffer)) >= 0) {
		switch (what) {
			case 0:
				// translated mode
				device->mRawMode = false;
				break;
			case 1:
				// raw mode
				device->mRawMessenger = *((BMessenger*)buffer);
				device->mRawMode = true;
				break;
			case 2:
				// recalibrating for real
				device->mRecalibrate = true;
				break;
			case 4:
				// recalibrating temporarily
				device->mTempRecalibrate = true;
				strncpy(device->mTempRecalibrationString, buffer,
					sizeof (buffer));
				break;
		}
	}
		
	return B_OK;
}	

int32 
AcerWPTouchDevice::AcerWPToucher(void* data)
{
	AcerWPTouchDevice* device = (AcerWPTouchDevice*)data;
	int32 clickCount = 0;
	float saveX = -1.0;
	float saveY = -1.0;
	bigtime_t saveWhen = 0;
	bool down = false;
	bool lastDown = false;
	float xDelta = 0.0;
	float xScale = 1.0;
	float xAddY = 0.0;
	float xMulY = 0.0;
	float yDelta = 0.0;
	float yScale = 1.0;
	float yAddX = 0.0;
	float yMulX = 0.0;
	sample_t sample;
	BPath settingsPath;

	device->mRecalibrate = true;
	device->mTempRecalibrate = false;
	
	find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	settingsPath.Append("dt300_calibration");
	
	{
		BEntry settings(settingsPath.Path());
		if (!settings.Exists()) {
			status_t err;
			entry_ref calibrate;

			err = get_ref_for_path(kCalibratePath, &calibrate);
			if (err == B_ENTRY_NOT_FOUND) {
				SERIAL_PRINT((kByline "Oopsie: %s not found.\n",
					kCalibratePath));
			} else {
				err = be_roster->Launch(&calibrate);
				if (err != B_OK) {
					SERIAL_PRINT((kByline "Launch() b0rked: %s\n",
						strerror(err)));
				}
			}
		}
	}
	
	while (read(device->mFileDesc, &sample, sizeof (sample))
			== sizeof (sample)) {
		bool move = false;
		
		if (device->mRecalibrate || device->mTempRecalibrate) {
			int32 numParsed = 0;
			
			if (device->mTempRecalibrate) {
				numParsed = sscanf(device->mTempRecalibrationString,
					"%f %f %f %f %f %f %f %f",
					&xDelta, &xScale, &xAddY, &xMulY,
					&yDelta, &yScale, &yAddX, &yMulX);
			} else {
				FILE* file = fopen(settingsPath.Path(), "r");
				if (file != NULL) {
					numParsed = fscanf(file, "%f %f %f %f %f %f %f %f",
						&xDelta, &xScale, &xAddY, &xMulY,
						&yDelta, &yScale, &yAddX, &yMulX);
					fclose(file);
				}
			}
			
			if (numParsed != 8) {
				xDelta = 0.0;
				xScale = 1.0;
				xAddY = 0.0;
				xMulY = 0.0;
				yDelta = 0.0;
				yScale = 1.0;
				yAddX = 0.0;
				yMulX = 0.0;
			}
			
			device->mRecalibrate = false;
			device->mTempRecalibrate = false;
		}
		
		float tempX = -1.0;
		float tempY = -1.0;
		
		if (!sample.pressed) {
			down = false;
		} else {
			down = true;
			
			float kXMin = 0x000;
			float kXRange = 0x1000;
			float kYMin = 0x000;
			float kYRange = 0x1000;
			
			tempX = ((float)sample.x - kXMin) / kXRange;
			tempY = ((float)sample.y - kYMin) / kYRange;
			
			float x = ((tempX + xDelta) * xScale)
				* (1 + ((tempY + yDelta) * yScale) * xMulY);
			float y = ((tempY + yDelta) * yScale)
				* (1 + ((tempX + xDelta) * xScale) * yMulX);
				
			/*if (!down)*/ {
				float dx = x - saveX;
				float dy = y - saveY;
				const float maxDiff = 0.01;
				
				if (dx > -maxDiff && dx < maxDiff
						&& dy > -maxDiff && dy < maxDiff
						|| system_time() - saveWhen < 250000) {
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
			BMessage* event = new BMessage(B_MOUSE_MOVED);
			event->AddInt64("when", system_time());
			event->AddFloat("x", saveX);
			event->AddFloat("y", saveY);
			event->AddInt32("buttons", (lastDown) ? B_PRIMARY_MOUSE_BUTTON : 0);
			device->EnqueueMessage(event);
		}
		
		if (down && device->mRawMode) {
			BMessage event('rawm');
			event.AddInt64("when", system_time());
			event.AddFloat("x", tempX);
			event.AddFloat("y", tempY);
			event.AddInt32("buttons", (lastDown) ? B_PRIMARY_MOUSE_BUTTON : 0);
			device->mRawMessenger.SendMessage(&event);
		}
		
		if (down != lastDown) {
			bigtime_t now = system_time();
			BMessage* event = new BMessage((down) ? B_MOUSE_DOWN : B_MOUSE_UP);
			event->AddInt64("when", now);
			event->AddInt32("x", 0); // TT: why is this an integer?
			event->AddInt32("y", 0); // TT: again?
			event->AddInt32("buttons", (down) ? B_PRIMARY_MOUSE_BUTTON : 0);
			
			if (down) {
				if ((saveWhen + device->mClickSpeed) > now)
					clickCount++;
				else
					clickCount = 1;
				
				event->AddInt32("clicks", clickCount);
				
				saveWhen = now;
			}
			
			device->EnqueueMessage(event);
			lastDown = down;
		}
	}
	
	return B_OK;
}
