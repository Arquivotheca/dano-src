/* ++++++++++
	FILE:	FindDirectory.cpp
	REVS:	$Revision: 1.4 $
	NAME:	herold
	DATE:	Fri May 23 16:55:40 PDT 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <Debug.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <Volume.h>


/* ----------
	find_directory
----- */

status_t find_directory(directory_which which, BPath *path, bool create_it, BVolume *vol)
{
	status_t	err;
	char		buf [PATH_MAX];
	
	err = find_directory (which, vol ? vol->Device() : -1, create_it, buf, sizeof (buf));
	if (err != B_OK)
		return err;

	return path->SetTo (buf);
}

