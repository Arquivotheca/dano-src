/*
   This file contains the public interfaces to the /dev/kdebug device.

   Primarily this is only of interest to the kdbg command line utility
   and driver writers that want to write additional kdebug functions.

   Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.
*/

#ifndef _KDBG_H
#define _KDBG_H

#ifndef _DRIVERS_H
#include <Drivers.h>
#endif

enum {
	KDBG_LOAD_SYMBOLS = B_DEVICE_OP_CODES_END + 1    /* passed to control() */
};

#define KDBG_NAME_LEN      64

typedef struct kdbg_symbol
{
	unsigned long  addr;
	char           name[KDBG_NAME_LEN];
} kdbg_symbol;

typedef struct kdbg_syms
{
	int          num_syms;
	kdbg_symbol *syms;
}kdbg_syms;



/*
   This function is only for driver writers.
*/   
typedef int (*kdebug_func)(int argc, char **argv);
int add_debugger_command(char *name, kdebug_func func, char *help);


/*
   These are return values from kdebug functions.  The values
   aren't really important, it's more the names that matter
   (i.e. testing for success isn't just a test against zero
   or less than zero).
*/   

#define  CMD_NOT_FOUND  -1
#define  CMD_ERR         0
#define  CMD_OK          1
#define  CMD_CONT        2  /* this command can be re-executed */
#define  QUIT_CMD        3


#endif  /* _KDBG_H */

