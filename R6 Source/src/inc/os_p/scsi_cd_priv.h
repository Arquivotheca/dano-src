/* ++++++++++
	$Source: /net/bally/be/rcs/src/inc/os_p/scsi_cd_priv.h,v $
	$Revision: 1.1 $
	$Author: robert $
	$Date: 1995/10/30 20:11:40 $
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

	Private data structures and control calls for using the scsi driver
+++++ */

#ifndef _SCSI_PRIV_H
#define _SCSI_PRIV_H


/* Supported methods of CD-ROM access */

enum {  CD_UNSUPPORTED = 0,
		CD_SCSI_3,
		CD_TRY_TOSHIBA,
		CD_TOSHIBA,
		CD_TRY_SONY,
		CD_SONY
};
 

#endif
