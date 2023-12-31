usb_interface_descriptor *d)
{
	dprintf("  interface number:        %d\n",d->interface_number);
	dprintf("  alternate setting:       %d\n",d->alternate_setting);
	dprintf("  number endpoints:        %d\n",d->num_endpoints);
	dprintf("  class/subclass/protocol: %d / %d / %d\n",
			d->interface_class,d->interface_subclass,d->interface_protocol);
	dprintf("  interface:               %s\n",string_descr(dev,d->interface));
}

static void print_cfg_descr(bm_usb_device *dev, usb_configuration_descriptor *d)
{
	dprintf("number interfaces:       %d\n",d->number_interfaces);
	dprintf("configuration_value:     %d\n",d->configuration_value);
	dprintf("configuration (text):    %s\n",string_descr(dev,d->configuration));
	dprintf("attributes:              0x%02x\n",d->attributes);
	dprintf("max power (mA):          %d\n",d->max_power*2);
}

static void print_ept_descr(bm_usb_device *dev, usb_endpoint_descriptor *d)
{
	char *types[4] = { "Control", "Isochronous", "Bulk", "Interrupt" };
	dprintf("    endpoint type:           %s\n",types[d->attributes&0x03]);
	dprintf("    endpoint address:        %d (%s)\n",d->endpoint_address & 0x0F,
			d->endpoint_address & 0x80 ? "IN" : "OUT");  	
	dprintf("    max packet size:         %d\n",d->max_packet_size);
	dprintf("    interval (ms):           %d\n",d->interval);
}

void dump_configs(bm_usb_device *dev)
{
	uint x,i,j,k;
	usb_configuration_info *c;
	usb_interface_info *ii;
	print_dev_descr(dev,&(dev->descriptor));
	
	for(x=0;x<dev->descriptor.num_configurations;x++){
		c=&(dev->configuration[x]);
		
		dprintf("[Configuration %d]\n",x+1);
		print_cfg_descr(dev,c->descr);
		for(i=0;i<c->interface_count;i++){
			for(k=0;k<c->interface[i].alt_count;k++){
				ii = &(c->interface[i].alt[k]);
				if(k){
					dprintf("  [Interface %d, Alternate %d]\n",i+1,k);
				} else {
					dprintf("  [Interface %d]\n",i+1);
				}				
				print_ifc_descr(dev,ii->descr);
				for(j=0;j<ii->endpoint_count;j++){
					dprintf("    [Endpoint %d]\n",j+1);
					print_ept_descr(dev,ii->endpoint[j].descr);
				}
				for(j=0;j<ii->generic_count;j++){
					dprintf("    [Unknown Descriptor]\n    ");
					dump_buf(ii->generic[j],ii->generic[j]->generic.length);
				}
			}
		}
	}
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
#ifndef _USBD_H
#define _USBD_H

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <lock.h>
#include <malloc.h>

#include <USB.h>

#include <usb_hcd.h>

#include "usb_hub.h"

extern void check_heap(void);
#if USB_MEMORY_CHECK
#define MCHECK()	{dprintf("checking memory at %s %d... ", __FILE__, __LINE__); check_heap(); dprintf("done checking memory\n"); }
#else
#define MCHECK()
#endif

#define USB_DEVICE_ADDED 0
#define USB_DEVICE_REMOVED 1

/* bus manager's private types for the the public types */
typedef struct bm_usb_device bm_usb_device;	
typedef struct bm_usb_interface bm_usb_interface;
typedef struct bm_usb_pipe bm_usb_pipe;

typedef struct usb_iob usb_iob;
typedef struct usb_bus usb_bus;
typedef struct usb_hub usb_hub;

struct usb_iob
{
	usb_iob *next;
	
	uchar buffer[8];
	
	usb_callback_func callback;
	void *cookie;		
	
	hcd_transfer transfer;
};

struct usb_bus
{
	usb_bus *next;
	hcd_info *hcd;
	hcd_module_info *ops;
	char *name;
	
	/* phys entry pool stuff*/
	hcd_phys_entry	* phys_entry_pool;
	void			* phys_entry_pool_buffer;
	hcd_phys_entry	phys_entry_terminator;
	
	/* Packet pool stuff */
	area_id			packet_pool_area;
	void			* packet_pool_buffer;
	uint32			packet_pool_bitmask_1;	/* Use 2 int32s because compiler does bad */
	uint32			packet_pool_bitmask_2;	/* things with shifts on int64s */
	sem_id			packet_pool_sem;
	
	int number;
	int32 page_crossings_allowed;
	uint32 addrmap[4];
	sem_id addrmap_sem;
	
	bool teardown;
	
	bm_usb_pipe *lowspeed;
	bm_usb_pipe *fullspeed;
	bm_usb_device *roothub;
};

//struct bm_usb_device;

struct bm_usb_pipe
{
	/* Used for the ID list, DO NOT MOVE */
	uint32 signature;
	usb_id id;
	void * next;

	mlock lock;
	
	usb_iob *pool;
	int32 pool_lock;
	size_t iobsz;

	hcd_endpoint *ept;
	int32 addr_flags;
	
	bigtime_t timeout;
	uint16 max_packet_size;
	
	bm_usb_interface * ifc;
};

#define	PIPE_IN			0x00000080
#define PIPE_LOWSPEED	0x00000100
#define PIPE_STALLED	0x00000200
#define PIPE_CONTROL	0x00000400
#define PIPE_BULK		0x00000800
#define PIPE_ISOCH		0x00001000
#define PIPE_INT		0x00002000

#define PIPE_ADDR_MASK	0x000000ff

#define is_inbound_pipe(pipe)	((pipe)->addr_flags & PIPE_IN)

struct bm_usb_device
{
	/* Used for the ID list, DO NOT MOVE */
	uint32 signature;
	usb_id id;
	void * next;
	
	/* struct */ bm_usb_device *parent;
	int refcount;
	int active; /* device is live and well */
	
	sem_id lock;
	
	char *name;
	
	bm_usb_pipe *control;
	usb_bus * bus;
	
	usb_device_descriptor descriptor;	
	usb_configuration_info *configuration;
	usb_configuration_info *active_conf;
	
	/* if non-zero this is really a usb_hub */
	uint nchildren;
	
	uint dev_address;
};

struct usb_hub
{
	bm_usb_device d;
	
	usb_hub_descriptor hubdesc;
	
	struct {
		uint16 port_status;
		uint16 port_changed;
	} portstat;
	
	uint hubdata[8];
	
	bigtime_t	last_error_time;
	int	 		interrupt_error_count;

	struct bm_usb_device *children[16];		
};

struct bm_usb_interface
{
	/* Used for the ID list, DO NOT MOVE */
	uint32		signature;
	usb_id		id;
	void		* next;
	
	lock		lock;
	
	bool		active;
	
	bm_usb_device	* dev;
	
	uint32		endpoint_count;
	bm_usb_pipe	** endpoints;
	
	const usb_interface_info	* info;
};

/* from usb_enum.c */
status_t init_enum(usb_bus *list);
void uninit_enum(usb_bus *list);

void schedule_republish(void);

/* from usb_pipe.c */
status_t create_pipe(usb_bus *bus, bm_usb_pipe **_pipe, 
					 usb_endpoint_descriptor *descr, 
					 int address, bool lowspeed, bm_usb_interface * dev);

void destroy_pipe(bm_usb_pipe *pipe);

/* from usb_bus.c */
void init_busses(usb_bus ** bus_list);
void uninit_busses(usb_bus *list);

int get_address(usb_bus * bus);
void put_address(usb_bus * bus, int addr);

hcd_phys_entry * get_phys_entry(usb_bus * bus);
void put_phys_entry(usb_bus * bus, hcd_phys_entry * node);

void * get_packet_buffer(usb_bus * bus);
void put_packet_buffer(usb_bus * bus, void * packet);

/* from usb_device.c */
status_t set_address(const usb_id id);

void get_device(bm_usb_device *dev);
bm_usb_device *new_device(usb_bus *bus, bm_usb_device *parent, int lowspeed, int portnum);

void put_device(bm_usb_device *dev);
void deactivate_device(bm_usb_device * d);
status_t tear_down_endpoints(bm_usb_device *d, bool destroy_control_pipe);