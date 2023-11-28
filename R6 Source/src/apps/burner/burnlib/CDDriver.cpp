#include "CDDriver.h"


CDDriver::CDDriver(void)
{
	lock = create_sem(1,"CDDriver:lock");
}


CDDriver::~CDDriver()
{
	delete_sem(lock);
}

