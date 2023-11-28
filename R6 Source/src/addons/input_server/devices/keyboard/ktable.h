/* ++++++++++
	FILE:	ktable.h
	NAME:	herold
	DATE:	May 14, 1999
	Copyright (c) 1999 by Be Incorporated.  All Rights Reserved.

	Class declaration for a lookup table for mapping scancodes to be-keys.
+++++ */

#ifndef _KEY_TABLE_H
#define _KEY_TABLE_H

#include <SupportDefs.h>

typedef struct {
	uint32	scancode;
	uint32	keycode;
} elem;

#define INITIAL_TABLE_SIZE	128
#define INCR_TABLE_SIZE		16

class ktable {
public:
					ktable();
					~ktable();
	status_t		InitFromFile(const char *path);
	status_t		Add (uint32 scancode, uint32 keycode);
	uint32			KeyCode (uint32 scancode);
	uint32			ScanCode(uint32 key_code);
private:
	void			Dump();		/* dump the current table */
	elem *			table;		/* -> table of mappings */
	size_t			total;		/* # elems in table */
	size_t			used;		/* # used elems in table */
};
	


#endif

