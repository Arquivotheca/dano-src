#ifndef _DPC_H
#define _DPC_H

#include <OS.h>
#include <module.h>

#define B_DPC_MODULE_NAME "generic/dpc/v1"

/* DPC-like functions */
typedef	void *dpc_thread_handle; 
typedef void *dpc_handle;
typedef void dpc_handler(void* cookie);

typedef struct dpc_module_info dpc_module_info;

struct dpc_module_info 
{
	module_info minfo;
	
	status_t (*create_dpc_thread)(dpc_thread_handle *dpct_handle,  
								  const char *name, int32 priority);
	status_t (*delete_dpc_thread)(dpc_thread_handle dpct_handle);
	dpc_handle (*queue_dpc)(dpc_thread_handle dpct_handle, 
							 dpc_handler *proc, void *cookie);
};

#endif
