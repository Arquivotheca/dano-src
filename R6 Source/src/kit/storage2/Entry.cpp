NULL)
		goto err1;
	if(optional_republish_driver_name)
	{
		rd->optional_republish_driver_name = strdup(optional_republish_driver_name);
		if(rd->optional_republish_driver_name == NULL)
			goto err2;
	}
	if(count){
		rd->devs = (usb_support_descriptor *) malloc(sizeof(usb_support_descriptor)*count);
		if(rd->devs ==  NULL)
			goto err3; 
		memcpy(rd->devs,devs,sizeof(usb_support_descriptor)*count);
	} else {
		rd->devs = NULL;
	}
	release_sem(usb_reg_devs_lock);
	return B_OK;
	
err3:
	free((char*)rd->optional_republish_driver_name);
err2:
	free((char*)rd->driver_name);
err1:
	free(rd);
err0:
	release_sem(usb_reg_devs_lock);
	return B_NO_MEMORY;
}
								
static void inform(usb_registered_driver *rd, bm_usb_device *d)
{
	uint i;
	if(supports(d,rd)){
		void *cookie;
		ddprintf(ID "telling \"%s\" about device @ %p\n",rd->driver_name,d);
		if(rd->hooks.device_added(d->id, &cookie) == B_OK){
			usb_driver_binding *bind = (usb_driver_binding*) malloc(sizeof(usb_driver_binding));
			bind->cookie = cookie;
			bind->device = d;
			bind->next = rd->bind;
			rd->bind = bind;
			d->refcount++;
		}
	}
	if(d->nchildren){
		usb_hub *hub = (usb_hub *) d;
		ddprintf(ID "searching children of hub @ %p\n",d);
		for(i=0;i<d->nchildren;i++){
			if(hub->children[i]) inform(rd,hub->children[i]);
		}
	}
}

static void do_republish(const char * name, const char * opt_repub_name)//, bm_usb_device * _d)
{
	rdrv * r;
	
	//dprintf("do_republish(%s, %s, %p)\n", name, opt_repub_name, _d);
	dprintf("do_republish(%s, %s)\n", name, opt_repub_name);

	//if(!d->bus->teardown) FIXME
	{
		acquire_sem(usb_repub_lock);
		
		/* if there is no repub list, start one */
		if(!first_rdrv)
			schedule_republish();
	
		if(first_rdrv) {
			r = first_rdrv;
			while(r){
				if(!strcmp(r->name, name))
					goto already_got_one;
				r = r->next;
			}
		}

		/* add to the list */
		r = (rdrv *) malloc(sizeof(rdrv));
		r->next = first_rdrv;
		r->name = opt_repub_name ? opt_repub_name : name;
		first_rdrv = r;

		
already_got_one:	
		release_sem(usb_repub_lock);
	}

	dprintf("do_republish(%s, %s) done\n", name, opt_repub_name);
	MCHECK(); // BUGBUG
}

/*	do_notify
**	
**	This function takes care of making the notify hook calls to registered
**	drivers. If destructive is true it will also call_put device for removal
**	and do the republishing so drivers can change their devfs presence.
**	
**	Non-destructive mode should only be used when changing configurations
**	to notify to the drivers that onld pointers are invalid.
*/
void do_notify(bm_usb_device *d, uint32 status)
{
	usb_registered_driver *rd;
	int found_binding = false;
	bool	deactivated = false;
	
	
	acquire_sem(usb_reg_devs_lock);
	rd = first_regdev;
	ddprintf(ID "do_notify(%p, %ld)\n", d, status);
	
	while(rd){
		dprintf("rd = %p, rd->driver_name = %s\n", rd, rd->driver_name);
		if(status == USB_DEVICE_ADDED){
			if(supports(d,rd)){
				if(rd->hooks.device_added) {
					void *cookie;
					if((rd->hooks.device_added(d->id, &cookie) == B_OK)) {
						usb_driver_binding *bind = (usb_driver_binding*) malloc(sizeof(usb_driver_binding));
						found_binding = true;
						bind->cookie = cookie;
						bind->device = d;
						bind->next = rd->bind;
						rd->bind = bind;					
						d->refcount++;
					}
				}
			} else {
				goto next_driver; /* unsupported, no need to repub */
			}
		} else {
			void *cookie;
			usb_driver_binding *bind,*last;
			bool call_hook = false;
			
			bind = rd->bind;
			last = NULL;
			cookie = NULL;
			if(!deactivated) {
				d->active = 0;
				deactivated = true;
			}
			
			while(bind){
				if(bind->device == d){
				    call_hook = true;
				    found_binding = true;
					cookie = bind->cookie;
					if(last) {
						last->next = bind->next;
					} else {
						rd->bind = bind->next;
					}
					free(bind);
					break;
				}
				last = bind;
				bind = bind->next;
			}

			if(call_hook) {
				if(rd->hooks.device_removed) {
					rd->hooks.device_removed(cookie);
				}
				put_device(d);
				MCHECK(); // BUGBUG
			} else {
			    goto next_driver;
			}
		}
		do_republish(rd->driver_name, rd->optional_republish_driver_name);
				
next_driver:		
		rd = rd->next;
	}
	
	if(!found_binding) {
		//XXX This while is a gross hack until we can fix the architecture layout
		rd = first_regdev;
		while (rd) {
			do_republish(rd->driver_name, rd->optional_republish_driver_name);
			rd = rd->next;
		}
		if(status == USB_DEVICE_REMOVED) {
			dprintf(ID "putting device that has no bindings\n");
			if(!deactivated) {
				d->active = 0;
				deactivated = true;
			}
			put_device(d);
		}
	}

	release_sem(usb_reg_devs_lock);
	MCHECK(); // BUGBUG
}

status_t install_notify(const char *driver_name, const usb_notify_hooks *hooks)
{
	usb_registered_driver *rd;
	dprintf(ID "driver \"%s\" requests notify(), hooks @ %p\n",driver_name,hooks);
	acquire_sem(usb_reg_devs_lock);
	rd = first_regdev;
	while(rd){
		if(!strcmp(rd->driver_name,driver_name)){
			memcpy(&rd->hooks,hooks,sizeof(usb_notify_hooks));
			break;
		}
		rd = rd->next;
	}
	release_sem(usb_reg_devs_lock);
	
	/* do this AFTER releasing the lock to avoid deadlocks */
	if(rd) {
		usb_bus *bus;
		for(bus = usb_bus_list; bus; bus = bus->next){
			if(bus->roothub) inform(rd, bus->roothub); 
		}
	}
	return B_OK;
}

status_t uninstall_notify(const char *driver_name)
{
	usb_registered_driver *rd;
	dprintf(ID "driver \"%s\" requests no more notify()\n",driver_name);
	acquire_sem(usb_reg_devs_lock);
	rd = first_regdev;
	while(rd){
		if(!strcmp(rd->driver_name,driver_name)){
			usb_driver_binding *bind, *next;
			for(bind = rd->bind; bind; bind = next){
				next = bind->next;
				dprintf(ID "unbinding dev %p, cookie %p\n",bind->device,bind->cookie);
				if(rd->hooks.device_removed) {
					rd->hooks.device_removed(bind->cookie);
					put_device(bind->device);
					free(bind);
				}
			}
			rd->bind = NULL;
			memset(&rd->hooks,0,sizeof(usb_notify_hooks));
			break;
		}
		rd = rd->next;
	}
	release_sem(usb_reg_devs_lock);
	return B_OK;
}

status_t usb_ioctl(uint32 opcode, void* buf, size_t buf_size)
{
	status_t ret = B_ERROR;
	
	switch(opcode)
	{
	/* Become unloadable */
	case 42:
		dprintf(ID "becoming unloadable at driver request\n");
		unloadable = true;
		ret = B_OK;
		break;
	
	/* ID to pipe */
	case 11:
	{
		bm_usb_pipe * tmp;
		
		tmp = id_to_pipe(((bm_usb_pipe *)buf)->id);
		if (tmp) {
			memcpy(buf, tmp, (buf_size >= sizeof(bm_usb_pipe) ? sizeof(bm_usb_pipe) : buf_size));
			ret = B_OK;
		} else {
			ret = B_DEV_INVALID_PIPE;
		}
		break;
	}
	
	/* ID to device */
	case 12:
	{
		bm_usb_device * tmp;
		
		tmp = id_to_device(((bm_usb_device *)buf)->id);
		if (tmp) {
			memcpy(buf, tmp, (buf_size >= sizeof(bm_usb_device) ? sizeof(bm_usb_device) : buf_size));
			ret = B_OK;
		} else {
			ret = B_DEV_INVALID_PIPE;
		}
		break;
	}
	default:
	}
	return ret; 
}

extern int serial_dprintf_enabled;

static usb_bus *usb_bus_list;

static status_t
std_ops(int32 op, ...)
{
	status_t	err;
	int			loaded, i;

	dprintf(ID "std_ops(%ld)\n", op);
	
	switch(op) {
		
	case B_MODULE_INIT:
#if LOAD_ONLY_WITH_SERIAL_DEBUG	
		if(!serial_dprintf_enabled)
			return B_ERROR;
#endif
		/* Only load the bus manager once */
		loaded = atomic_add(&usbd_loaded, 1);
		if (loaded) {
			dprintf(ID "already loaded\n");
			return B_OK;
		} else {
			dprintf(ID "loading\n");
		}
		
		unloadable = false;
		
		/* are there any USB busses? */
		usb_bus_list = NULL;
		init_busses(&usb_bus_list);
		if(!usb_bus_list) {
			dprintf (ID "found no busses\n");
			usbd_loaded = false;
			unloadable = true;
			return B_ERROR;
		}
		
		/* Create the bus managers global sems */
		usb_reg_devs_lock = create_sem(1,"usbd:devlist_lock");
		if (usb_reg_devs_lock < 0) {
			return usb_reg_devs_lock;
		}
		usb_repub_lock = create_sem(1,"usbd:repub_lock");
		if (usb_repub_lock < 0) {
			return usb_repub_lock;
		}
		id_hash_lock = create_sem(0, "usbd id hash lock");
		if (id_hash_lock < 0) {
			return id_hash_lock;
		}
		
		/* Init the id hash table */
		current_pipe_id = 1;
		for (i = 0; i < ID_HASH_TABLE_SIZE; i++) {
			id_hash_table[i] = NULL;
		}
		release_sem(id_hash_lock);
		
		/* Start up the enumerator */
		if((err = init_enum(usb_bus_list))) {
			dprintf(ID "error initializing the enumerator\n");
			uninit_busses(usb_bus_list);
			usbd_loaded = false;
			unloadable = true;

			return err;
		}
		
		/* Register kernel debugger commands */
		add_debugger_command("dump_usb_id_table", dump_usb_id_table, "dump the usb ID hash table");
		
		return B_OK;		
		
	case B_MODULE_UNINIT:
		if(unloadable ) {
			dprintf(ID "uninit request - unloading\n");
			if(usbd_loaded) {
				remove_debugger_command("dump_usb_id_table", dump_usb_id_table);
				uninit_enum(usb_bus_list);
				delete_sem(usb_reg_devs_lock);
				delete_sem(usb_repub_lock);
				delete_sem(id_hash_lock);
				uninit_busses(usb_bus_list);
				usbd_loaded = false;
			}
			return B_OK;
		} else {
			dprintf(ID "uninit request - not unloading\n");
			return B_ERROR;
		}
	default:
		return -1;
	}
}

static status_t
rescan()
{
	return 0;
}

struct usb_module_info_v2 usb_module_v2 = {
	{
		{
			"bus_managers/usb/v2",
			0,
			&std_ops
		},
		&rescan
	},
	
	&register_driver,
	&install_notify,
	&uninstall_notify,
	
	&get_device_descriptor,
	&get_nth_configuration,
	
	&get_configuration,
	&set_configuration,
	&set_alt_interface,
	
	&set_feature,
	&clear_feature,
	&get_status,
	&get_descriptor,
	&send_request_safe_v2,
	
	&queue_interrupt,
	&queue_bulk,
	&queue_isochronous,
	&queue_request_v2,
	
	&set_pipe_policy,
	&cancel_queued_transfers,
	&usb_ioctl
};

struct usb_module_info_v3 usb_module_v3 = {
	{
		{
			B_USB_MODULE_NAME,
			0,
			&std_ops
		},
		&rescan
	},
	
	&register_driver,
	&install_notify,
	&uninstall_notify,
	
	&get_device_descriptor,
	&get_nth_configuration,
	
	&get_configuration,
	&set_configuration,
	&set_alt_interface,
	
	&set_feature,
	&clear_feature,
	&get_status,
	&get_descriptor,
	&send_request_safe,
	
	&queue_interrupt,
	&queue_bulk,
	&queue_bulk_v,
	&queue_isochronous,
	&queue_request,
	
	&set_pipe_policy,
	&cancel_queued_transfers,
	&usb_ioctl
};

_EXPORT
module_info *modules[] = {
        (module_info *) &usb_module_v2,
        (module_info *) &usb_module_v3,
        NULL
};
         #
# usb bus manager makefile
#
# Copyright (C) 1994 Be Inc.  All Rights Reserved
#

#######
include $(BUILDHOME)/buildprefs/make.pre
#######

DRVRNAME	:= usb

SRCS		:= bus_manager.c  usb_bus.c  usb_device.c  usb_enum.c  usb_pipe.c \
	usb_util.c version2.c usb_interface.c atomic_list.S

OBJS := $(SRCS_LIST_TO_OBJS)

# for dpc.h and usb_hcd.h
#
CFLAGS		+= -I$(BUILDHOME)/src/inc/usb_p \
				-I$(BUILDHOME)/src/inc/driver \
			-I$(BUILDHOME)/src/nukernel/inc \
			-I$(BUILDHOME)/src/nukernel/cpu/$(CPU) \
			-I$(BUILDHOME)/src/nukernel/arch/intel \
			-I$(BUILDHOME)/src/inc/os_p


CFLAGS += -Wall -W -Wno-unused -Wpointer-arith
CFLAGS += -DINTEL=1 

TARGET			:= $(OBJ)/$(DRVRNAME)

$(TARGET):	$(OBJ_DIR) $(OBJS)
		$(LD) -o $@ $(OBJS) $(LDFLAGS)
		$(SETVERSION) $@ $(SYSTEM_VERSION) $(APP_VERSION)

#######
include $(BUILDHOME)/buildprefs/make.post
#######
                                                                                                                                                                        #include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <driver_settings.h>

#include "usbd.h"
#include "atomic_list.h"

#if __GNUC__
#define ID "usbd: [" __FUNCTION__ "] "
#else
#define ID "usbd: "
#endif

#if _USBD_DEBUG
#define ddprintf(x...)	dprintf(x)
#else
#define ddprintf(x...)
#endif

#define PHYS_ENTRY_POOL_COUNT	128		/* This is for the phys_entry buffers (1 per bus) */

#define	PACKET_POOL_ENTRY_SIZE	64		/* These are for the packet buffer pools (1 per bus) */
#define	PACKET_POOL_ENTRY_COUNT	64		/* ...they are currently 1 page large */
										/* XXX if these change size the bitmasks in usb_bus also need to change size */

static status_t init_phys_entry_pool(usb_bus * bus);
static void uninit_phys_entry_pool(usb_bus * bus);

static status_t init_packet_pool(usb_bus * bus);
static void uninit_packet_pool(usb_bus * bus);

typedef struct 
{
	hcd_module_info *ops;
	usb_bus *list;
	int number;
	char *name;
} cbinfo;

/*	hcd_cb
**	This function is called once per bus installed in the machine by the HCD drivers when they are "started"
*/
static void hcd_cb(hcd_info *hcd, void *cookie, int32 page_boundry_crossings_allowed)
{
	cbinfo		* cbi = (cbinfo*) cookie;
	usb_bus		* bus;
	char		buf [B_OS_NAME_LENGTH];
	status_t	err;
	
	dprintf(ID "adding bus number %d which allows %ld page boundry crossings\n", cbi->number, page_boundry_crossings_allowed);
	
	bus = (usb_bus*) malloc(sizeof(usb_bus));
	memset(bus,0,sizeof(usb_bus)); // This takes care of the addrmap
	
	bus->hcd = hcd;
	bus->ops = cbi->ops;
	bus->number = cbi->number++;
	bus->name = cbi->name;
	bus->page_crossings_allowed = page_boundry_crossings_allowed;
	
	/* Create some semaphores */
	sprintf(buf, "usb_bus %d addrmap_sem", bus->number);
	bus->addrmap_sem = create_sem(1, buf); //XXX check return value
	if (bus->addrmap_sem < B_OK) {
		dprintf(ID "error creating semaphore for bus %d\n", bus->number);
		goto error;
	}
	sprintf(buf, "usb_bus %d packet_pool_sem", bus->number);
	bus->packet_pool_sem = create_sem(1, buf);
	if (bus->packet_pool_sem < B_OK) {
		dprintf(ID "error creating semaphore for bus %d\n", bus->number);
		goto error;
	}
	
	/* Init the pre-allocation pools */
	err = init_packet_pool(bus);
	if (err) {
		dprintf(ID "error creating packet pool for bus %d\n", bus->number);
		goto error;
	}
	
	err = init_phys_entry_pool(bus);
	if (err) {
		dprintf(ID "error creating physical entry pool for bus %d\n", bus->number);
		goto error;
	}
	
	/* Add the bus to the global list of loaded busses */
	bus->next = cbi->list;
	cbi->list = bus;
	return;
	
error:
	cbi->number--;
	return;	
}

void init_busses(usb_bus ** bus_list