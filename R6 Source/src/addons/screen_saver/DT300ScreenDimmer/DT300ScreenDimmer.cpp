#include <Debug.h>
#include <StringView.h>

#include <stdio.h>
#include <unistd.h>

#include "DT300ScreenDimmer.h"
#include "dt300.h"

const char* devPath = "/dev/misc/dt300";

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id img)
{
	return new DT300ScreenDimmer(msg, img);
}

DT300ScreenDimmer::DT300ScreenDimmer(BMessage *msg, image_id img)
		: BScreenSaver(msg, img)
{
}

void
DT300ScreenDimmer::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, 
						"DT300ScreenDimmer"));
}

status_t
DT300ScreenDimmer::StartSaver(BView */*view*/, bool preview)
{
	int fd;
	status_t err;
	
	/* Don't want to turn off the backlight if it's just doing preview.
	** We don't do a preview anyway.
	*/
	if (preview) {
		return B_ERROR;
	}
	
	/* We never need our draw function to be called, because it doesn't
	** do anything. But we should set it to something longer than the
	** default 50 msec; once a minute should be innocuous enough.
	*/
	SetTickSize(60000000LL);
	
	fd = open(devPath, O_RDWR);
	if (fd >= 0) {
		err = ioctl(fd, DT300_TURN_OFF_BACKLIGHT);
		SERIAL_PRINT(("DT300_TURN_OFF_BACKLIGHT ioctl() returned %s\n",
			strerror(err)));
		close(fd);
	}
	else {
		SERIAL_PRINT(("open(\"%s\") failed, returning %s\n", strerror(fd)));
		err = fd;
	}

	return err;
}

void
DT300ScreenDimmer::Draw(BView */*view*/, int32 /*frame*/)
{
}

void
DT300ScreenDimmer::StopSaver(void)
{
	int fd;
	status_t err;
	
	fd = open(devPath, O_RDWR);
	if (fd >= 0) {
		err = ioctl(fd, DT300_TURN_ON_BACKLIGHT);
		SERIAL_PRINT(("DT300_TURN_ON_BACKLIGHT ioctl() returned %s\n",
			strerror(err)));
		close(fd);
	}
	else {
		SERIAL_PRINT(("open(\"%s\") failed, returning %s\n", strerror(fd)));
	}
}
