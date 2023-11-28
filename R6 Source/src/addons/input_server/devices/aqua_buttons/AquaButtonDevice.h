#ifndef AQUABUTTONDEVICE_H
#define AQUABUTTONDEVICE_H

#include <InputServerDevice.h>

class AquaButtonDevice : public BInputServerDevice {
	public:
		AquaButtonDevice();
		virtual status_t InitCheck();
		virtual status_t Start(const char *device, void *cookie);
		virtual status_t Stop(const char *device, void *cookie);
	private:
		static int32 input_thread_entry(void *arg);

		thread_id input_thread;
		int       dev_fd;
};

#endif

