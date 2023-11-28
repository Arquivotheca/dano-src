/* ++++++++++
	FILE:	ktable.cpp
	NAME:	herold
	DATE:	May 14, 1999
	Copyright (c) 1999 by Be Incorporated.  All Rights Reserved.

	A class implementing a lookup table for mapping scancodes to be-keys.
+++++ */

#define DEBUG 1
#include <Debug.h>
#include <SupportDefs.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "ktable.h"


/* ----------
	elem_compare - comparison routine for bsearch and qsort
----- */

static int
elem_compare (const void *e1, const void *e2)
{
	uint32	sc1 = ((elem *)e1)->scancode;
	uint32	sc2 = ((elem *)e2)->scancode;

	return (sc1 < sc2) ? -1 : (sc1 > sc2) ? 1 : 0;
}


/* ----------
	constructor
----- */

ktable::ktable()
{
	used = 0;
	total = INITIAL_TABLE_SIZE;
	table = (elem *)malloc (total * sizeof (elem));
}


/* ----------
	destructor
----- */

ktable::~ktable()
{
	if (table)
		free (table);
}


/* ----------
	Dump - dump a mapping table to debug output
----- */

void
ktable::Dump()
{
	int	i;
	for (i = 0; i < used; i++)
		SERIAL_PRINT(("sc %.8x key %.8x\n", table[i].scancode, table[i].keycode));
}


/* ----------
	InitfromFile - initialize a mapping table from a file describing the
	mappings.
----- */

status_t
ktable::InitFromFile (const char *path)
{
	FILE		*fp;
	char		buff[1024], *ptr;
	int			x, y;
	status_t	err = B_OK;

	if (!table)	
		return ENOMEM;
	
	fp = fopen(path, "r");
	if (fp == NULL)
		return B_FILE_NOT_FOUND;

	while (fgets(buff, sizeof(buff), fp) != NULL) {
		ptr = &buff[0];
		while (isspace(*ptr) && *ptr != '\0')
			ptr++;
		if (*ptr == '\0')
			continue;
		if (*ptr < ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n')
			break;
		if (*ptr == '#')
			continue;
			
		if (sscanf(ptr, "%i%i", &x, &y) != 2)
			continue;
	
		if ((err = Add (x, y)) != B_OK)
			break;
	}

	
	if (used)
		qsort (table, used, sizeof (table[0]), elem_compare);

	//Dump();
	fclose (fp);
	return err;
}


/* ----------
	Add - add a new entry to the mapping table
----- */

status_t
ktable::Add (uint32 scancode, uint32 keycode)
{
	elem	*temp_table;

	//SERIAL_PRINT (("ktable::Add (%.8x, %.8x), table=%.8x, used=%.8x\n", scancode, keycode, table, used));
	if (!table)
		return ENOMEM;

	if (used == total) {
		total += INCR_TABLE_SIZE;
		temp_table = (elem *) realloc (table, total * sizeof (elem));
		if (!temp_table)
			return ENOMEM;
		table = temp_table;	
	}
	table[used].scancode = scancode;
	table[used].keycode = keycode;
	used++;
	
	return B_OK;
}


/* ----------
	Keycode - look up the passed scacode and return the Be KeyCode
----- */

uint32
ktable::KeyCode (uint32 scancode)
{
	elem	*result;
	elem	key_elem;

	//SERIAL_PRINT(("looking up %.8x\n", scancode));

	key_elem.scancode = scancode;
	result = (elem *) bsearch ((void *) &key_elem, (void *) &table[0],
		used, sizeof (elem), elem_compare);

	if (!result)
		return 0;

	return result->keycode;
}


/* ----------
	ScanCode - look up the passed Be KeyCode and return a scancode
----- */

uint32
ktable::ScanCode(uint32 key_code)
{
	int	i;

	for (i = 0; i < used; i++)
		if (table[i].keycode == key_code)
			return table[i].scancode;
	return 0;
}
