#include "ScreenSaverFilter.h"
#include "ScreenSaverController.h"

extern "C" _EXPORT BInputServerFilter *instantiate_input_filter(void);


BInputServerFilter *
instantiate_input_filter(void)
{
	return new ScreenSaverFilter();
}

ScreenSaverFilter::ScreenSaverFilter()
 : controller(0), active(false)
{
	// create the controller
	controller = new ScreenSaverController();
}

ScreenSaverFilter::~ScreenSaverFilter()
{
	// terminate the controller
	active = false;
	if(controller)
	{
		int32	junk;
		controller->PostMessage(B_QUIT_REQUESTED);
		wait_for_thread(controllerthread, &junk);
	}
}

status_t ScreenSaverFilter::InitCheck()
{
	// start the controller
	if((controllerthread = controller->Run()) < 0)
		return B_ERROR;
	else
	{
		active = true;
		return B_NO_ERROR;
	}
}

filter_result ScreenSaverFilter::Filter(BMessage *message, BList *)
{
	// let the controller take a look at the events
	if(active)
		return controller->ProcessInput(message) ? B_DISPATCH_MESSAGE : B_SKIP_MESSAGE;
	return B_DISPATCH_MESSAGE;
}
