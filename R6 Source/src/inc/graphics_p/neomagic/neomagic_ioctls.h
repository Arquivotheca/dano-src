#ifndef __NEOMAGIC_IOCTLS_H
#define __NEOMAGIC_IOCTLS_H

#include <drivers/Drivers.h>
#include <add-ons/graphics/Accelerant.h>

/*****************************************************************************
 * ioctl() definitions.
 */
#define	NEOMAGIC_IOCTLPROTOCOL_VERSION	1

enum private_ioctls {
	NEOMAGIC_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	NEOMAGIC_IOCTL_WRITE_VGAREG,
	NEOMAGIC_IOCTL_READ_VGAREG,
	MAXIOCTL_NEOMAGIC
};

/*  NEOMAGIC_IOCTL_GETGLOBALS  */
typedef struct neomagic_getglobals {
	uint32	gg_ProtocolVersion;	// NEOMAGIC_IOCTLPROTOCOL_VERSION
	area_id	gg_GlobalArea;
} neomagic_getglobals;

/* NEOMAGIC_IOCTL_WRITE_VGAREG, NEOMAGIC_IOCTL_READ_VGAREG */
typedef struct neomagic_readwrite_vgareg {
	uint32 reg;
	uint8 value;
} neomagic_readwrite_vgareg;
#endif /* __NEOMAGIC_IOCTLS_H */
