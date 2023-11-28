/* ++++++++++
	FILE:	zook_priv.h
	REVS:	$Revision: 1.8 $
	NAME:	herold
	DATE:	Wed Nov 29 17:52:38 PST 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _ZOOK_PRIV_H
#define _ZOOK_PRIV_H

#define PRIVATE_DIR_NAME	"_be_private_"

/* volume server */

#define		CER_CONNECT			'cecv'
#define		CER_FORMAT_DEVICE	'cefd'
#define		CER_DISCONNECT		'cegv'
#define		CER_MOUNT_VOL		'cemv'
#define		CER_UNMOUNT_VOL		'ceuv'

/* posix server */

#define		POSIX_CHGDIR		'pocd'
#define		POSIX_CHGMOD		'pocm'
#define		POSIX_CLOSE			'pocl'
#define		POSIX_DUPFD			'podf'
#define		POSIX_FSTAT			'poft'
#define		POSIX_FINI			'pofi'
#define		POSIX_GETDIRPATH	'pogd'
#define		POSIX_GETFD			'pogf'
#define		POSIX_GETFL			'pogl'
#define		POSIX_GETTPCR		'pogp'
#define		POSIX_GETREF		'pogr'
#define		POSIX_INIT			'poin'
#define		POSIX_LOADPOSIX		'polp'
#define		POSIX_MKDIR			'pomd'
#define		POSIX_OPEN			'poop'
#define		POSIX_RMDIR			'pord'
#define		POSIX_RENAME		'porn'
#define		POSIX_SCANDIR		'posd'
#define		POSIX_SETFD			'posf'
#define		POSIX_SETFL			'posl'
#define		POSIX_SETTPCR		'posp'
#define		POSIX_STAT			'post'
#define		POSIX_UNLINK		'poul'
#define		POSIX_UTIME			'pout'

#define	CURRENT_DF_VERSION		0x1002

typedef struct  {
		long	signature;
		long	record_count;
        long    offset_table_ptr;
        long    hole_table_ptr;
        long    free_entry_hint;
		long	dirty_count;
		long	unique;
        long    reserved[10];
}       df_header;


#endif

