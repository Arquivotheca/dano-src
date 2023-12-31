��i��5�w�c�ړgz��� ��&M��/`\bǳ?K��Z�>�Eyk*�deik���W!n�&Cj!<5"����yd�|��rk}u/�2���O�*��B=�����r3v�������|�["��W��~�{CC��еE�Bw�W�M�E�r���߻uK���	�=ޡH���1�Ro��dn����)�/>l��i`��j&�sJ4X�E\��$�&�?�ܗ��,+-�sNA4� nm�-4q�|by���!����D�G43�o�G�	wt�l�|��ж(��ϼ[�H�ԥ`P}�3r:��>���7��v�!���V�>O?�|q�@��F�e��o�~�P�)��YJۦ+iѶ���rs0P���PE\q�J�oq�� ���|4Sr}��p��#��k��'lC�� q/�c�M���P��;��ڇ�8
d5*�4+]
����k�o�{r��w�;�M���j�#�Y���N��5ڀ�ш}�r^�o#�KՃ�B8Z������Ҋ��BHFx�Խ�Ɓ�w�F_��K��dL$e�H^�A����*� �E�Qx1/^�9_*����O� ׷_@�E��x��s[��PAd���ʘ��s��υ,.nG��5�����~qn>-�B��8����d�NӚgl��� ��W�<a�T�0��Fpb��w�o��f�('��D����/8���x�5�'N�{z�#5�����[���,r��*�r����E�;�0�7�fL�bmv9���.:ZHM���6m�c�(�R�j�˷�Y�g��x�3O~�S�`Ӕ�ICF�%�|�3R)�݁	Un�X�=)�Vqo����Ax���Sv��tPE�����@�t���H��:_���׃�� �gd��i##�Ǘ�>�w�9�w��^<���q�t�����6��7 ���x;{�i�t9�g�vr6��gYf�O�	v�.����i�lMP�˫��~�ʱ����������o�����E�˧C5H &��`3�# Makefile for driver

TARGET_NAME:=usb

SRC_DIR:=src/modules/bus_managers/usbd

SRCS		:= bus_manager.c  usb_bus.c  usb_device.c  usb_enum.c  usb_pipe.c \
	usb_util.c version2.c usb_interface.c atomic_list.S

VPATH := util

MY_INCLUDES := -I$(BUILDHOME)/src/inc/usb_p \
				-I$(BUILDHOME)/src/inc/driver \
				-I$(BUILDHOME)/src/nukernel/inc	\
				-I$(BUILDHOME)/src/nukernel/cpu/$(TARGET_CPU) \
				-I$(BUILDHOME)/src/nukernel/arch/intel \
				-I$(BUILDHOME)/src/nukernel/vm/noswap/common \
				-I$(BUILDHOME)/src/nukernel/vm/noswap/cpu/$(TARGET_CPU) \
				-I$(BUILDHOME)/src/inc/os_p


SYSTEM_LIBS:=

MY_CFLAGS := -DINTEL=1 -Wall -W -Wno-unused -Wpointer-arith

INSTALL_PREFIX := $(INSTALL_MODULES)
INSTALL_TARGET := bus_managers/$(TARGET_NAME)

MY_OPTIMIZER:=-O1
include $(SDIR_TEMPLATES)/KernelAddonTemplate.mk

                                                                                                                                                                                                                


CLEANUP.

1. Implement send_request() by using queue_request().

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #
# void *atomic_pop( void **head )
#
	.align	8									
	.globl atomic_pop
	atomic_pop:
	pushl		%ebx
	movl		8(%esp),%ebx
	cld
	atomic_pop1:
	movl		(%ebx),%eax
	movl		(%eax),%ecx
	lock
	cmpxchg 	%ecx,(%ebx)
	jnz			atomic_pop1
	popl		%ebx
	ret
	
#
# void *atomic_push( void **head, void *node )
#
	.align	8									
	.globl atomic_push
	atomic_push:
	pushl		%ebx
	movl		8(%esp),%ebx
	movl		12(%esp),%ecx
	cld
	atomic_push1:
	movl		(%ebx),%eax
	movl		%eax,(%ecx)
	lock
	cmpxchg 	%ecx,(%ebx)
	jnz			atomic_push1
	popl		%ebx
	ret

	
	
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             #ifndef ATOMIC_LIST_H
#define ATOMIC_LIST_H

/**********
a node looks something like the following:

typedef struct node
{
	struct node		*next;
	data elements...
} node_t;

The last element in the list should needs to be a terminator element.
This element points back to itself and does not actually need to be a complete node.
You have reached the end of the list when the terminator is returned;
	because it is self referencing, it is not actually removed from the list.

**********/

void *atomic_pop( void **head );
void *atomic_push( void **head, void *node );

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                  #include "atomic_list.h"
#include <KernelExport.h>

static spinlock		listlock = 0;

// Generic interrupt safe atomic list functions.
// CPU specific asm versions are much faster, but this version can be
// substituted if no such version exists for the target platform.

void *atomic_pop( void **head )
{
	cpu_status	cpu;
	void 		*node;
	
	cpu = disable_interrupts();
	acquire_spinlock( &listlock );
	
	node = *head;
	*head = *((void **)node);
	
	release_spinlock( &listlock );
	restore_interrupts( cpu );
	return node;
}

void *atomic_push( void **head, void *node )
{
	cpu_status	cpu;
	void		*previous;
	
	cpu = disable_interrupts();
	acquire_spinlock( &listlock );
	
	previous = *head;
	
	*((void **)node) = previous;
	*head = node;
	
	release_spinlock( &listlock );
	restore_interrupts( cpu );
	return previous;
}
                                                                                                                                                                                                               
/* usb/bus_manager.c
 *
 * Actual BeOS USB Bus Manager Interface 
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <KernelExport.h>

#include "usbd.h"

#if 1
#define ddprintf(x...)	dprintf(x)
#else
#define ddprintf(x...)
#endif

#if __GNUC__
#define ID "usbd: [" __FUNCTION__ "] "
#else
#define ID "usbd: "
#endif

static int		unloadable;
static int32	usbd_loaded = false;
static uint32	current_pipe_id;

static sem_id usb_reg_devs_lock = -1;
static sem_id usb_repub_lock = -1;
static sem_id id_hash_lock = -1;

typedef struct usb_driver_binding
{
	struct usb_driver_binding *next;
	bm_usb_device *device;
	void *cookie;
} usb_driver_binding;

typedef struct usb_registered_driver
{
	struct usb_registered_driver *next;
	
	const char *driver_name;
	const char *optional_republish_driver_name;
	usb_driver_binding *bind;
	
	usb_support_descriptor *devs;
	size_t devs_count;
	
	usb_notify_hooks hooks;
} usb_registered_driver;

typedef struct _rdrv
{
	struct _rdrv *next;
	const char *name;
} rdrv;

typedef struct _id_hash_entry
{
	uint32					signature;
	uint32					id;
	struct _id_hash_entry	* next;
} id_hash_entry;

#define	ID_HASH_TABLE_SIZE		100

static rdrv 					* first_rdrv = NULL;		/* list of drivers to republish in the republishizer */
static id_hash_entry			* id_hash_table[ID_HASH_TABLE_SIZE];
static usb_registered_driver	* first_regdev = NULL;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int supports_device(usb_registered_driver *rd, usb_device_descriptor *desc)
{
	uint i;
	
	for(i=0;i<rd->devs_count;i++){
		if(rd->devs[i].dev_class && 
		   (rd->devs[i].dev_class != desc->device_class)) continue;
		if(rd->devs[i].dev_subclass && 
		   (rd->devs[i].dev_subclass != desc->device_subclass)) continue;
		if(rd->devs[i].dev_protocol && 
		   (rd->devs[i].dev_protocol != desc->device_protocol)) continue;
		if(rd->devs[i].vendor && 
		   (rd->devs[i].vendor != desc->vendor_id)) continue;
		if(rd->devs[i].product && 
		   (rd->devs[i].product != desc->product_id)) continue;
		return 1;
	}
	return 0;
}

static int supports_interface(usb_registered_driver *rd, usb_interface_descriptor *desc)
{
	uint i;
	
	for(i=0;i<rd->devs_count;i++){
		if(rd->devs[i].dev_class && 
		   (rd->devs[i].dev_class != desc->interface_class)) continue;
		if(rd->devs[i].dev_subclass && 
		   (rd->devs[i].dev_subclass != desc->interface_subclass)) continue;
		if(rd->devs[i].vendor) continue;
		if(rd->devs[i].product) continue;
		return 1;
	}
	return 0;
}

/* does the registered driver support this device? */
static int supports(bm_usb_device *d, usb_registered_driver *rd)
{
	uint i,j,k;
	
	if(rd->devs_count){
		if(supports_device(rd, &(d->descriptor))) return 1;
		for(i=0;i<d->descriptor.num_configurations;i++){
			for(j=0;j<d->configuration[i].interface_count;j++){
				for(k=0;k<d->configuration[i].interface[j].alt_count;k++){
					if(supports_interface(rd,d->configuration[i].interface[j].alt[k].descr)) return 1;
				}
			}
		}
		return 0;
	} else {
		/* ultimate wildcard */
		return 1;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int dump_usb_id_table(int argc, char **argv)
{
	int i;
	id_hash_entry * foo;
	
	for(i = 0; i < ID_HASH_TABLE_SIZE; i++) {
		kprintf("Slot %d:\n", i);
		
		foo = id_hash_table[i % ID_HASH_TABLE_SIZE];
		while(foo) {
			kprintf("\tfoo = %p\tid = %lu%s\tfoo->next = %p\n", foo, foo->id, (foo->id < 100 ? "  " : ""), foo->next);
			foo = foo->next;
		}
	}
	
	return 0;	
}

/*	get_pipe_id
**	Assigns a new usb ID and inserts pointer into the lookup hash.
*/	
status_t
get_usb_id(void * ptr)
{
	status_t		err;
	id_hash_entry	* entry = (id_hash_entry *)ptr;
	usb_id			id;
	
	if (!ptr) {
		return B_BAD_VALUE;
	}
	
	err = acquire_sem(id_hash_lock);
	if (err) {
		dprintf(ID "error acquiring sem\n");
		return err;
	}
	
	// XXX protection from wrap around?
	id = current_pipe_id++;
	
	/* Add entry to the begining of this collision list */
	entry->id = id;
	entry->next = id_hash_table[id % ID_HASH_TABLE_SIZE];
	id_hash_table[id % ID_HASH_TABLE_SIZE] = entry;
	
#if _USBD_DEBUG
	{
		id_hash_entry * foo = id_hash_table[id % ID_HASH_TABLE_SIZE];
		while(foo) {
			dprintf(ID "foo = %p, id = %lu, foo->next = %p\n", foo, foo->id, foo->next);
			foo = foo->next;
		}
	}
#endif
	
	ddprintf(ID "assigned new usb id %lu, slot entry %lu, ptr = %p\n", id, id %ID_HASH_TABLE_SIZE, ptr);
	
	release_sem(id_hash_lock);
	return B_OK;
}

status_t
put_usb_id(void * ptr)
{
	status_t		err;
	id_hash_entry	* cur, * last;
	uint32			slot;
	
	if (!ptr) {
		return B_BAD_VALUE;
	}
	
	err = acquire_sem(id_hash_lock);
	if (err) {
		dprintf(ID "error acquiring semaphore\n");
		return err;
	}
	
	slot = ((id_hash_entry *)ptr)->id % ID_HASH_TABLE_SIZE;
	cur = id_hash_table[slot];
	last = NULL;
	while (cur) {
		if (cur->id == ((id_hash_entry *)ptr)->id) {
			if (last) {
				last->next = cur->next;
			} else {
				id_hash_table[slot] = cur->next;
			}
			break;
		}
		last = cur;
		cur = cur->next;
	}
	
	ddprintf(ID "removed id %lu from the table\n", ((id_hash_entry *)ptr)->id);
	
	release_sem(id_hash_lock);
	return B_OK;
}

bm_usb_pipe *
id_to_pipe(usb_id id)
{
	status_t		err;
	id_hash_entry	* cur;
	
	err = acquire_sem(id_hash_lock);
	if (err) {
		dprintf(ID "error acquiring semaphore\n");
		return NULL;
	}
	
	cur = id_hash_table[id % ID_HASH_TABLE_SIZE];
	while (cur) {
		if (cur->id == id) {
			break;
		}
		cur = cur->next;
	}
	
	release_sem(id_hash_lock);
	
	if (cur && cur->signature == SIG_PIPE) {
		return (bm_usb_pipe *)cur;
	}
	return NULL;
}

bm_usb_device *
id_to_device(usb_id id)
{
	status_t		err;
	id_hash_entry	* cur;
	
	err = acquire_sem(id_hash_lock);
	if (err) {
		dprintf(ID "error acquiring semaphore\n");
		return NULL;
	}
	
	cur = id_hash_table[id % ID_HASH_TABLE_SIZE];
	while (cur) {
		if (cur->id == id) {
			break;
		}
		cur = cur->next;
	}
	
	release_sem(id_hash_lock);
	
	if (cur && cur->signature == SIG_DEVICE) {
		return (bm_usb_device *)cur;
	}
	return NULL;
}

bm_usb_interface *
id_to_interface(usb_id id)
{
	status_t		err;
	id_hash_entry	* cur;
	
	err = acquire_sem(id_hash_lock);
	if (err) {
		dprintf(ID "error acquiring semaphore\n");
		return NULL;
	}
	
	cur = id_hash_table[id % ID_HASH_TABLE_SIZE];
	while (cur) {
		if (cur->id == id) {
			break;
		}
		cur = cur->next;
	}
	
	release_sem(id_hash_lock);
	
	if (cur && cur->signature == SIG_INTERFACE) {
		return (bm_usb_interface *)cur;
	}
	return NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void _repub_cb(void *data /* unused */)
{
	rdrv *r;
	int fd;
	ddprintf(ID "about to republish usb devices\n");

	fd = open("/dev",O_RDWR);
	if(fd < 0){
		dprintf(ID "cannot open /dev for repub\n");
	} else {
		for(;;){
			acquire_sem(usb_repub_lock);
			if(!(r = first_rdrv)) break;
			
			/* unlink it */
			first_rdrv = r->next;
			release_sem(usb_repub_lock);
			
			ddprintf(ID "## republishing %s\n",r->name);
			write(fd,r->name,strlen(r->name)+1);
			free(r);
		}
		release_sem(usb_repub_lock);
		close(fd);
	}
	ddprintf(ID "done republishing usb devices\n");
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

status_t register_driver(const char *driver_name,
						 const usb_support_descriptor *devs, 
						 size_t count,
						 const char *optional_republish_driver_name)
{
	usb_registered_driver *rd;
	dprintf(ID "driver \"%s\" registered\n",driver_name);
	acquire_sem(usb_reg_devs_lock);
	rd = first_regdev;
	while(rd){
		if(!strcmp(rd->driver_name,driver_name)){
			release_sem(usb_reg_devs_lock);
			/* XXX update record? */
			return B_OK;
		}
		rd = rd->next;
	}
	if(!rd){
		rd = (u