#ifndef DRIVER_H
#define DRIVER_H

#include <kernel/OS.h>
#include <device/graphic_driver.h>
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <drivers/ISA.h>
#include <Debug.h>
#include <Drivers.h>
#include <Accelerant.h>
#include <GraphicsCard.h>
#include <video_overlay.h>
#include <SupportDefs.h>
#include <stdio.h>
#include <string.h>
#include <drivers/genpool_module.h>

#include "SiS/bena4.h"
#include "SiS/sis5598defs.h"
#include "SiS/sis6326defs.h"
#include "SiS/sis620defs.h"
#include "SiS/sis_accelerant.h"

#define DEBUG 1
#define SIS_VERBOSE
#define SIS_MEGA_VERBOSE


////////////////////////
// DRIVER IOCTL CODES //
////////////////////////

enum private_ioctls {
	SIS_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
//	SIS_IOCTL_GET_PCI,
//	SIS_IOCTL_SET_PCI,
	SIS_IOCTL_SETDPMS,
	SIS_IOCTL_GETDPMS,
	SIS_IOCTL_SETCLOCK,
	SIS_IOCTL_SET_CURSOR_SHAPE,
	SIS_IOCTL_MOVE_CURSOR,
	SIS_IOCTL_SHOW_CURSOR,
	SIS_IOCTL_SET_CRT,
	SIS_IOCTL_INIT_CRT_THRESHOLD,
	SIS_IOCTL_SET_COLOR_MODE,
	SIS_IOCTL_MOVE_DISPLAY_AREA,
	SIS_IOCTL_SETUP_DAC,
	SIS_IOCTL_SET_INDEXED_COLORS,
	SIS_IOCTL_SCREEN_OFF,
	SIS_IOCTL_RESTART_DISPLAY,
	MAXIOCTL_SIS
};

// DRIVER IOCTL PARAMETER STRUCTURE TYPES

typedef struct accelerant_getglobals {
	uint32	gg_ProtocolVersion;
	area_id	gg_GlobalArea;
} accelerant_getglobals;

typedef struct data_ioctl_set_cursor_shape {
	uint16 width, height;
	uint16 hotX, hotY;
	uchar *andMask, *xorMask;
	} data_ioctl_set_cursor_shape ;

typedef struct {
	int16 screenX, screenY ;
	} data_ioctl_move_cursor ;
	
typedef struct data_ioctl_sis_CRT_settings {
	ushort CRT_data[0x18+1];
	uchar extended_CRT_overflow, extended_horizontal_overflow ;
	} data_ioctl_sis_CRT_settings;

typedef struct data_ioctl_move_display_area {
	uint16 x,y ;
	} data_ioctl_move_display_area;
	
typedef struct data_ioctl_set_indexed_colors {
	uint count;
	uint8 first;
	uint8 *color_data;
	uint32 flags;
	} data_ioctl_set_indexed_colors;


// SIS CHIPSET

#define MEMORY_MARGIN (256*1024)

///////////////////
// DEBUG SETTINGS
///////////////////

// vvddprintf : mega verbose mode
// vddprintf : verbose mode
// ddprintf  : debug mode

#ifdef SIS_MEGA_VERBOSE
#define vvddprintf ddprintf
#define SIS_VERBOSE
#else
#define vvddprintf (void)
#endif

#ifdef SIS_VERBOSE
#define vddprintf ddprintf
#else
#define vddprintf (void)
#endif

#if DEBUG > 0

#ifdef COMPILING_ACCELERANT
#define DPRINTF_ON _kset_dprintf_enabled_(TRUE) 
#define ddprintf(a) _kdprintf_ a
#else
#define DPRINTF_ON  set_dprintf_enabled(TRUE)
#define ddprintf(a) dprintf a
#endif

#else

#define DPRINTF_ON
#define ddprintf(a)

#endif



////////////////
// STRUCTURES //
////////////////

typedef struct devinfo {
	pci_info	di_PCI;
	area_id		di_BaseAddr0ID;
	area_id		di_BaseAddr1ID;
	area_id		di_sisCardInfo_AreaID; // clones shared memory area
	uint32		di_PoolID;
	sis_card_info	*di_sisCardInfo;

	uint32		di_Opened;
	int32		di_IRQEnabled;
	uint32		di_NInterrupts;	// DEBUG
	uint32		di_NVBlanks;	// DEBUG
	uint32		di_LastIrqMask;	// DEBUG
	char		di_Name[B_OS_NAME_LENGTH];
} devinfo;

#define	MAXDEVS			4

typedef struct driverdata {
	Bena4		dd_DriverLock;	            /*  Driver-wide benaphore	*/
	uint32		dd_NDevs;	                /*  # of devices found		*/
	uint32		dd_NInterrupts;	            /*  # of IRQs from this device	*/
	char		*dd_DevNames[MAXDEVS + 1];  /*  For export  */
	devinfo		dd_DI[MAXDEVS];	            /* Device-specific stuff	*/
} driverdata;


///////////////////////////
// INPUT/OUTPUT FUNCTIONS
///////////////////////////

extern pci_module_info		*pci_bus;
extern isa_module_info		*isa_bus;

#define	get_pci(pci,o,s)	(*pci_bus->read_pci_config)((pci)->bus, \
							    (pci)->device, \
							    (pci)->function, \
							    (o), (s))
#define	set_pci(pci,o,s,v)	(*pci_bus->write_pci_config)((pci)->bus, \
							     (pci)->device, \
							     (pci)->function, \
							     (o), (s), (v))

#define isa_outb(a,b)	(*isa_bus->write_io_8)((a),(b))
#define isa_inb(a)		(*isa_bus->read_io_8)((a))

#define inb(a)      *((vuchar *)(ci->ci_RegBase+(a)))
#define outb(a,b)   *((vuchar *)(ci->ci_RegBase+(a)))=(b)
#define inw(a)      *((vushort*)(ci->ci_RegBase+(a)))
#define outw(a,b)   *((vushort*)(ci->ci_RegBase+(a)))=(b)
#define inl(a)      *((vulong *)(ci->ci_RegBase+(a)))
#define outl(a,b)   *((vulong *)(ci->ci_RegBase+(a)))=(b)

#endif
