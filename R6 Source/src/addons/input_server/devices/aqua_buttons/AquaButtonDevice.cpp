
#include "AquaButtonDevice.h"
#include <unistd.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device();

BInputServerDevice *
instantiate_input_device()
{
	return new AquaButtonDevice();
}

AquaButtonDevice::AquaButtonDevice()
{
	dev_fd = open("/dev/input/buttons/aqua_buttons/0", O_RDWR);
	input_thread = -1;
}

status_t 
AquaButtonDevice::InitCheck()
{
	status_t err;
	input_device_ref *device_list[2];
	input_device_ref device = {"AquaButtons", B_KEYBOARD_DEVICE, NULL};
	
	if(dev_fd < 0)
		return B_ERROR;

	device_list[0] = &device;
	device_list[1] = NULL;
	
	return RegisterDevices(device_list);
}

status_t 
AquaButtonDevice::Start(const char *device, void *cookie)
{
	input_thread = spawn_thread(input_thread_entry, device,
	                            B_DISPLAY_PRIORITY, this);
	if(input_thread < 0)
		return input_thread;
	resume_thread(input_thread);
	return B_NO_ERROR;
}

status_t 
AquaButtonDevice::Stop(const char *device, void *cookie)
{
	int32 dummy;
	kill_thread(input_thread);
	wait_for_thread(input_thread, &dummy);
	input_thread = -1;
	return B_NO_ERROR;
}

int32 
AquaButtonDevice::input_thread_entry(void *arg)
{
	AquaButtonDevice *device = (AquaButtonDevice *)arg;
	struct {
		bigtime_t  time;
		uint8      buttons;
		uint8      reserved[3+4];
	} sample;
	uint8 buttons;
	
	while(read(device->dev_fd, &sample, sizeof(sample)) == sizeof(sample)) {
		for(int i = 0; i < 8; i++) {
			if((buttons & (1 << i)) != (sample.buttons & (1 << i))) {
				BMessage *message = new BMessage((sample.buttons & (1 << i)) ?
					B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP);
				message->AddInt32("when", sample.time);
				message->AddInt32("key", 0x300000 + i);
				device->EnqueueMessage(message);
			}
		}
	}
}
