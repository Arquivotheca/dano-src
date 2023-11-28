#if ! defined POWERMANAGEMENTFILTER_INCLUDED
#define POWERMANAGEMENTFILTER_INCLUDED

#include <OS.h>
#include <InputServerFilter.h>
#include <Message.h>

class PowerMgmtController;

class PowerMgmtFilter : public BInputServerFilter
{
	thread_id				controllerthread;
	PowerMgmtController	*controller;
	volatile bool			active;

public:
							PowerMgmtFilter();
	virtual					~PowerMgmtFilter();

	// PowerMgmtFilter stuff
	virtual	status_t		InitCheck();
	virtual	filter_result	Filter(BMessage *message, BList *outList);
};

#endif
