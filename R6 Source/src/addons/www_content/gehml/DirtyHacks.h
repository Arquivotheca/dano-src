
#ifndef _DIRTYHACKS_H_
#define _DIRTYHACKS_H_

#include <OS.h>
#include <SupportDefs.h>

#define BDirectWindow GehmlWindow
#include <Region.h>
#undef BDirectWindow

extern port_id		app_server_port();
extern void *		rw_offs_to_ptr(uint32 offs);
extern void *		ro_offs_to_ptr(uint32 offs);

#endif
