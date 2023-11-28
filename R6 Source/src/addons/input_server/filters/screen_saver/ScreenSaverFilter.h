#if ! defined SCREENSAVERFILTER_INCLUDED
#define SCREENSAVERFILTER_INCLUDED

#include <OS.h>
#include <InputServerFilter.h>
#include <Message.h>

class ScreenSaverController;

class ScreenSaverFilter : public BInputServerFilter
{
	thread_id				controllerthread;
	ScreenSaverController	*controller;
	volatile bool			active;

public:
							ScreenSaverFilter();
	virtual					~ScreenSaverFilter();

	// InputServerFilter stuff
	virtual	status_t		InitCheck();
	virtual	filter_result	Filter(BMessage *message, BList *outList);
};

#endif
