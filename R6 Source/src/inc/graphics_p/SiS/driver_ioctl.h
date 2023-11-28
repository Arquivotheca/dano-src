#ifndef DRIVER_IOCTL_H
#define DRIVER_IOCTL_H

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
	SIS_IOCTL_SET_PRIMARY_DISPLAY_MODE,
	SIS_IOCTL_SET_SECONDARY_DISPLAY_MODE,
	SIS_IOCTL_RESTORE_PRIMARY_DISPLAY_MODE,
	MAXIOCTL_SIS
};

// DRIVER IOCTL PARAMETER STRUCTURE TYPES

typedef struct accelerant_getglobals {
	uint32	gg_ProtocolVersion;
	area_id	gg_GlobalArea;
	uint32	gg_accelerantID;
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
	uchar	extended_CRT_overflow,
			extended_horizontal_overflow ;
	uchar	sis630_ext_vertical_overflow,
			sis630_ext_horizontal_overflow1,
			sis630_ext_horizontal_overflow2,
			sis630_ext_starting_address,
			sis630_ext_pitch,
			sis630_display_line_width ;

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

#endif
