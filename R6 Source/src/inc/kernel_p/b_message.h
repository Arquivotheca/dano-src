/* ++++++++++
	FILE:	x
	REVS:	$Revision$
	NAME:	herold
	DATE:	Fri Jan 05 11:22:42 PST 1996
	Copyright (c) 1994-96 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _B_MESSAGE_H
#define _B_MESSAGE_H

#ifndef _FS_H
#include <os/FS.h>
#endif

#define	BROWSER_HELLO	0x041767
#define	M_FILE_CREATE	0x01
#define	M_FILE_DELETE	0x02
#define	M_RENAME	0x03
#define	M_DIR_CREATE	0x04
#define	M_DIR_DELETE	0x05
#define	M_FILE_MOVE	0x06
#define	M_DIR_MOVE	0x07
#define	M_INFO		0x08
#define	M_FILE_CLOSE	0x09

/*----------------------------------------------------------------------*/

#define	TYPE_CHANGED			0x01
#define	CREATION_DATE_CHANGED		0x02
#define	MODIFICATION_DATE_CHANGED	0x04
#define	CREATOR_CHANGED			0x08

/*----------------------------------------------------------------------*/

typedef struct {
	long	what;
	long	has_loc;
	long	x_loc;
	long	y_loc;

	union {
		struct {
			FileEntry	new_entry;
			long		vol_id;
			long		dir_id;
		} f_create;

		struct {
			FileEntry	new_entry;
			long		vol_id;
			long		dir_id;
		} d_create;

		struct {
			char		name[32];
			long		vol_id;
			long		dir_id;
		} f_delete;
		
		struct {
			FileEntry	new_entry;
			long		vol_id;
			long		dir_id;
			char		old_name[32];
		} rename;

		struct {
			char		name[32];
			long		vol_id;
			long		dir_id;
		} d_delete;

		struct {
			FileEntry	new_entry;
			char		old_name[32];
			long		vol_id;
			long		old_dir_id;
			long		new_dir_id;
		} f_move;

		struct {
			FileEntry	new_entry;
			char		old_name[32];
			long		vol_id;
			long		old_dir_id;
			long		new_dir_id;
		} d_move;
		
		struct {
			FileEntry	new_entry;
			long		vol_id;
			long		dir_id;
			ulong		bits;
		} info;
	
		struct {
			FileEntry	new_entry;
			long		vol_id;
			long		dir_id;
		} f_close;
	} param;
} b_message;

#endif
