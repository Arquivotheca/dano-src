#ifndef __3DFX_IOCTLS_H
#define __3DFX_IOCTLS_H

#include <drivers/Drivers.h>
#include <add-ons/graphics/Accelerant.h>

/*****************************************************************************
 * ioctl() definitions.
 */
#define	THDFX_IOCTLPROTOCOL_VERSION	1

enum private_ioctls {
	THDFX_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	THDFX_IOCTL_GET_PCI,
	THDFX_IOCTL_SET_PCI,

	THDFX_IOCTL_ENABLE_TVOUT,
	
	THDFX_IOCTL_READ_I2C_REG,
	THDFX_IOCTL_WRITE_I2C_REG,

	THDFX_IOCTL_WRITE_PCI_32,
	THDFX_IOCTL_READ_PCI_32,
	THDFX_IOCTL_WRITE_PHYS_8,
	THDFX_IOCTL_READ_PHYS_8,
	THDFX_IOCTL_WRITE_PHYS_16,
	THDFX_IOCTL_CHECKFORROOM,

	THDFX_IOCTL_SET_VIDEO_MODE,
	THDFX_IOCTL_LOCK_3D_ENGINE,
	THDFX_IOCTL_UNLOCK_3D_ENGINE,

	MEM_IOCTL_GET_MEMMGR_AREA,
	MEM_IOCTL_GET_MEMMGR_ID,
	MEM_IOCTL_SET_SEM,
	MEM_IOCTL_RELEASE_SEMS,
	
	MAXIOCTL_THDFX
};

/*  THDFX_IOCTL_GETGLOBALS  */
typedef struct thdfx_getglobals {
	uint32	gg_ProtocolVersion;	// THDFX_IOCTLPROTOCOL_VERSION
	area_id	gg_GlobalArea;
} thdfx_getglobals;

/*  THDFX_IOCTL_GET_PCI, THDFX_IOCTL_SET_PCI  */
typedef struct thdfx_getsetpci {
	uint32	gsp_Offset;
	uint32	gsp_Size;
	uint32	gsp_Value;
} thdfx_getsetpci;

/*  THDFX_IOCTL_READ_PCI_32, THDFX_IOCTL_WRITE_PCI_32  */
typedef struct thdfx_readwritepci32 {
	uint32	offset;
	uint32	val;
} thdfx_readwritepci32;

typedef struct thdfx_setvideomode {
	uint32 xRes;		// xResolution in pixels
	uint32 yRes;		// yResolution in pixels
	uint32 refresh;		// refresh rate in Hz
	uint32 loadClut;	// really a bool, should we load the lookup table
	uint32 isPrimary;
} thdfx_setvideomode;


#endif /* __3DFX_IOCTLS_H */
