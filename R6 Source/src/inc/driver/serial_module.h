#ifndef _SERIAL_MODULE_H
#define _SERIAL_MODULE_H

#include <module.h>

typedef struct serial_info serial_info;

struct serial_info 
{
	module_info minfo;
	status_t (*get_nth_device)(int n, ushort *base, ushort *intr);
};

#endif
