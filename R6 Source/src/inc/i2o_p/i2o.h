
#ifndef I2O_BUS_MANAGER_H
#define I2O_BUS_MANAGER_H

#include <OS.h>
#include <PCI.h>
#include <SupportDefs.h>
#include <iovec.h>

#include <i2odep.h>
#include <i2obscsi.h>
#include <i2oexec.h>
#include <i2omsg.h>
#include <i2omstor.h>
#include <i2outil.h>
#include <i2oadptr.h>

#include <bus_manager.h>

typedef struct i2o_info i2o_info;

typedef struct 
{
	i2o_info *ii;
	uint32 tid;
} i2o_target;

typedef struct 
{
	bus_manager_info binfo;
	
	/* setup and teardown */
	i2o_info *(*init_device)(const char *name, const pci_info *pci);
	status_t (*uninit_device)(i2o_info *ii);
	
	/* locate and manage devices */
	status_t (*get_nth_lct)(i2o_info *ii, uint32 n, I2O_LCT_ENTRY *entry);
	
	status_t (*util_claim)(i2o_target *targ);
	status_t (*util_claim_release)(i2o_target *targ);	
	status_t (*util_params_get)(i2o_target *targ, uint32 group, 
								void *data, uint32 size);
	
	status_t (*bsa_block_read)(i2o_target *targ, off_t pos, iovec *vec, 
							   size_t count, size_t *len);
	status_t (*bsa_block_write)(i2o_target *targ, off_t pos, iovec *vec, 
								size_t count, size_t *len);
	
} i2o_module_info;

#define B_I2O_MODULE_NAME "bus_managers/i2o/v1"

#define i2o_init_device(n,p) (i2o->init_device)(n,p)
#define i2o_uninit_device(i) (i2o->uninit_device(i)

#define i2o_get_nth_lct(ii,n,e) (i2o->get_nth_lct)(ii,n,e)

#define i2o_util_claim(t) (i2o->util_claim)(t)
#define i2o_util_claim_release(t) (i2o->util_claim_release)(t)
#define i2o_util_params_get(t,g,d,s) (i2o->util_params_get)(t,g,d,s)

#define i2o_bsa_block_read(t,p,v,c,l) (i2o->bsa_block_read)(t,p,v,c,l)
#define i2o_bsa_block_write(t,p,v,c,l) (i2o->bsa_block_write)(t,p,v,c,l)

#endif