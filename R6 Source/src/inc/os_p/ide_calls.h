/* ++++++++++
	FILE:	ide_calls.h
	REVS:	$Revision: 1.5 $
	NAME:	guillaume
	DATE:	May 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _IDE_CALLS_H
#define _IDE_CALLS_H

#include <Drivers.h>

struct ide_ctrl_info {
    bool	ide_0_present;	/* wether the controller is present or not */
    bool	ide_0_master_present;
    bool	ide_0_slave_present;
    int		ide_0_master_type;/* type of device: B_DISK, B_CD etc... */
    int		ide_0_slave_type;	/* same for the slave */
    bool	ide_1_present;	/* wether the controller is present or not */
    bool	ide_1_master_present;
    bool	ide_1_slave_present;
    int		ide_1_master_type;/* type of device: B_DISK, B_CD etc... */
    int		ide_1_slave_type;	/* same for the slave */
};

/* controll calls for ide driver 
Note: they must not collide with the calls defined in scsi.h */


enum {
	IDE_GET_DEVICES_INFO = B_DEVICE_OP_CODES_END + 50,

	/* privates calls for the IDE driver to be able to */
	/* deal with ofs HDs */
	IDE_BYTE_SWAP = B_DEVICE_OP_CODES_END + 150, 
	IDE_NO_BYTE_SWAP
};
#endif



