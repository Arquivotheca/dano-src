#ifndef __RADEON_IOCTLS_H__
#define __RADEON_IOCTLS_H__

#include <drivers/Drivers.h>
#include <add-ons/graphics/Accelerant.h>

/*****************************************************************************
 * ioctl() definitions.
 */
#define	RADEON_IOCTLPROTOCOL_VERSION	1

enum private_ioctls {
	RADEON_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	RADEON_IOCTL_GET_PCI,
	RADEON_IOCTL_SET_PCI,

	RADEON_IOCTL_ENABLE_TVOUT,
	
	RADEON_IOCTL_READ_I2C_REG,
	RADEON_IOCTL_WRITE_I2C_REG,

	RADEON_IOCTL_WRITE_PCI_32,
	RADEON_IOCTL_READ_PCI_32,
	RADEON_IOCTL_WRITE_PHYS_8,
	RADEON_IOCTL_READ_PHYS_8,
	RADEON_IOCTL_WRITE_PHYS_16,
	RADEON_IOCTL_CHECKFORROOM,

	RADEON_IOCTL_MODE_COUNT,
	RADEON_IOCTL_GET_MODE_LIST,
	RADEON_IOCTL_PROPOSE_DISPLAY_MODE,
	RADEON_IOCTL_SET_DISPLAY_MODE,
	RADEON_IOCTL_GET_DISPLAY_MODE,
	RADEON_IOCTL_GET_FB_CONFIG,
	RADEON_IOCTL_GET_PIXEL_CLOCK_LIMITS,
	RADEON_IOCTL_SET_INDEXED_COLORS,
	RADEON_IOCTL_GET_DPMS_MODE,
	RADEON_IOCTL_SET_DPMS_MODE,

	RADEON_IOCTL_SET_2ND_VIDEO_MODE,
	RADEON_IOCTL_RESET_VIDEO_MODE,
	RADEON_IOCTL_GET_DMA_BUFFER,

	MEM_IOCTL_GET_MEMMGR_AREA,
	MEM_IOCTL_GET_MEMMGR_ID,
	MEM_IOCTL_SET_SEM,
	MEM_IOCTL_RELEASE_SEMS,
	
	MAXIOCTL_RADEON
};

/*  RADEON_IOCTL_GETGLOBALS  */
typedef struct radeon_getglobals {
	uint32	ProtocolVersion;	// RADEON_IOCTLPROTOCOL_VERSION
	area_id	GlobalArea;
} radeon_getglobals;

/*  RADEON_IOCTL_GET_PCI, RADEON_IOCTL_SET_PCI  */
typedef struct radeon_getsetpci {
	uint32	gsp_Offset;
	uint32	gsp_Size;
	uint32	gsp_Value;
} radeon_getsetpci;

/*  THDFX_IOCTL_READ_PCI_32, THDFX_IOCTL_WRITE_PCI_32  */
typedef struct radeon_readwritepci32 {
	uint32	offset;
	uint32	val;
} radeon_readwritepci32;

typedef struct radeon_set2ndvideomode {
	int32 w;
	int32 h;
	int32 is32bpp;
	int32 hz;
} radeon_set2ndvideomode;

typedef struct radeon_getDMAbuffer {
	uint32 *addr;
	uint32 cardOff;
	uint32 size;
} radeon_getDMAbuffer;

#endif

