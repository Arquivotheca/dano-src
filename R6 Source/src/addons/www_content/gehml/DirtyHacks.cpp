
#include "DirtyHacks.h"

// I've never done anything quite this awful before...
#define private public
#include <Application.h>
// Oooh!  it felt good!

port_id app_server_port()
{
	return be_app->fServerTo;
}

void * rw_offs_to_ptr(uint32 offs)
{
	return be_app->rw_offs_to_ptr(offs);
}

void * ro_offs_to_ptr(uint32 offs)
{
	return be_app->ro_offs_to_ptr(offs);
}
