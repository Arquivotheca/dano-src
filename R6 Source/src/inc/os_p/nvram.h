/* ++++++++++
	FILE:	nvram.h
	REVS:	$Revision: 1.3 $
	NAME:	herold
	DATE:	Mon Dec 04 17:13:15 PST 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _NVRAM_H
#define _NVRAM_H

#include <BeBuild.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----
	prototypes.

	Note that items smaller than 4 bytes are always passed
	as longs.  Hence the passed buffer pointer should point to
	a longword.
----- */

extern int read_config_item (int item_id, void *buf);
extern int write_config_item (int item_id, void *buf);


/* -----
	configuration item ids (sizes in parenthesis)
----- */

#define CFG_boot_dev	0	/* (1) boot device code - see below */
#define CFG_boot_bus	1	/* (1) boot device bus */
#define CFG_boot_id		2	/* (1) boot device id */
#define CFG_boot_lun	3	/* (1) boot device logical unit */
#define CFG_boot_sid	4	/* (1) boot device sesion id */
#define CFG_boot_pid	5	/* (1) boot device partition id */

#define CFG_ide_flags	6	/* Guillaume's ide flags */
#define CFG_atapi_flags	7	/* and an other one for atapi */


/* -----
	boot device codes - scsi or ide
----- */

#define bootdev_ata		0x00	/* onboard ide */
#define bootdev_scsi	0x01	/* onboard scsi */
#define bootdev_atapi   0x02

#ifdef __cplusplus
}
#endif

#endif

