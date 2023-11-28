
#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the ram driver.
*/
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "RAM "#x" benaphore");  x.ben = 0;
#define AQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);


#define RAM_PRIVATE_DATA_MAGIC	'RAMB' /* a private driver rev, of sorts */

#define MAX_RAM_DEVICE_NAME_LENGTH 32

#define RKD_MOVE_CURSOR    0x00000001
#define RKD_PROGRAM_CLUT   0x00000002
#define RKD_SET_START_ADDR 0x00000004
#define RKD_SET_CURSOR     0x00000008
#define RKD_HANDLER_INSTALLED 0x80000000

enum {
	RAM_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	RAM_DEVICE_NAME
};

typedef struct {
	area_id	fb_area;	/* Frame buffer's area_id.  The addresses are shared
							with all teams. */
	void	*framebuffer;	/* As viewed from virtual memory */
	void	*framebuffer_pci;	/* As viewed from the PCI bus (for DMA) */
	area_id	mode_area;	/* Contains the list of display modes the driver supports */
	uint32	mode_count;	/* Number of display modes in the list */

	int32	flags;

	display_mode
			dm;		/* current display mode configuration */
	frame_buffer_config
			fbc;	/* bytes_per_row and start of frame buffer */
	uint32	mem_size;			/* Frame buffer memory, in bytes. */
} shared_info;

/* Set some boolean condition (like enabling or disabling interrupts) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;		/* state to set */
} ram_set_bool_state;

/* Retrieve the area_id of the kernel/accelerant shared info */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	area_id	shared_info_area;	/* area_id containing the shared information */
} ram_get_private_data;

/* Retrieve the device name.  Usefull for when we have a file handle, but want
to know the device name (like when we are cloning the accelerant) */
typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;		/* The name of the device, less the /dev root */
} ram_device_name;

enum {
	RAM_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif


#endif
