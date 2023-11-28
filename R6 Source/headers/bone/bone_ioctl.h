/*
	bone_ioctl.h
	
	a directory of all the ioctl codes used by BONE

	Copyright 2000, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_IOCTL
#define H_BONE_IOCTL

enum {
	/* absolute lowest ioctl code used by bone */
	BONE_IOCTL_BASE = 0xbe230000,

	/* commands sent to the datalink  - see sockio.h */
	BONE_SOCKIO_IOCTL_BASE = BONE_IOCTL_BASE + 0x100,

	/* commands sent to the bone api driver - see bone_api.h */
	BONE_API_IOCTL_BASE = BONE_IOCTL_BASE + 0x200,

	/* commands sent to the bone_dhcp module - see net/dhcp.h */
	BONE_DHCP_IOCTL_BASE = BONE_IOCTL_BASE + 0x300,

	/* commands sent to the bone_serial module - see bone_serial.h */
	BONE_SERIAL_IOCTL_BASE = BONE_IOCTL_BASE + 0x400,
	
	/* commands sent to the bone_serial_ppp module - see bone_serial_ppp.h */
	BONE_SERIAL_PPP_IOCTL_BASE = BONE_IOCTL_BASE + 0x500,

	/* commands sent to the bone_pppoe module - see bone_pppoe.h */
	BONE_PPPOE_IOCTL_BASE = BONE_IOCTL_BASE + 0x600,
	
	/* commands to configure IP packet aliasing - see bone_pppoe.h */
	BONE_ALIAS_IOCTL_BASE = BONE_IOCTL_BASE + 0x700,
};

#endif	/* H_BONE_IOCTL */
