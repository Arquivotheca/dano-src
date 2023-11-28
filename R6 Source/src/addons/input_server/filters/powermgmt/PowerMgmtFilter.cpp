#include "PowerMgmtFilter.h"
#include "PowerMgmtController.h"

extern "C" _EXPORT BInputServerFilter *instantiate_input_filter(void);


BInputServerFilter *
instantiate_input_filter(void)
{
	return new PowerMgmtFilter();
}

PowerMgmtFilter::PowerMgmtFilter()
 : controller(0), active(false)
{
	// create the controller
	controller = new PowerMgmtController();
}

PowerMgmtFilter::~PowerMgmtFilter()
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

status_t PowerMgmtFilter::InitCheck()
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

filter_result PowerMgmtFilter::Filter(BMessage *message, BList *)
{
	// let the controller take a look at the events
	if(active)
		return controller->ProcessInput(message) ? B_DISPATCH_MESSAGE : B_SKIP_MESSAGE;
	return B_DISPATCH_MESSAGE;
}
