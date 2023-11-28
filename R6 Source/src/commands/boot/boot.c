/* ++++++++++
	FILE:	boot.c
	REVS:	$Revision: 1.5 $
	NAME:	herold
	DATE:	Tue May 09 13:39:26 PDT 1995

	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <nvram.h>

/* ----------
	error - print error message, usage message and exit
----- */

static void
error (char *format, ...)
{
	va_list		args;

	va_start(args,format);
	if (format)
		vprintf (format, args);
	printf ("usage: boot [ dev_num args... ]\n");
	printf ("for ide:  dev_num = 0, 1 arg (0 = master, 1 = slave)\n");
	printf ("for scsi: dev_num = 1, arg1 = scsi id, arg2 = lun\n");
	va_end(args);
	exit (1);
}

int
main(long argc, char* argv[])
{
	int dev;
	int sub1;
	int sub2;

	read_config_item(CFG_bootdev, (char*)&dev);
	read_config_item(CFG_boot_sub1, (char*)&sub1);
	read_config_item(CFG_boot_sub1, (char*)&sub2);

	if (argc > 4)
		error ("wrong number of arguments\n");

	switch (dev) {
	case bootdev_ide:
		printf ("currently booting from ide drive, %s\n", (sub1 == 0) ? "master" : "slave");
		break;

	case bootdev_scsi:
		printf ("currently booting from onboard scsi, target %d, lun %d\n", sub1, sub2);
		break;

	default:
		error ("current boot device %d is invalid\n", dev);
		break;
	}
	
	if (argc == 1)
		return 0;

	errno = 0;
	dev = strtol (argv[1], 0, 0);
	if (errno) {
		perror ("bad dev_num");
		error ("\n");
	}
	switch (dev) {
	case bootdev_ide:
		if (argc != 3)
			error ("wrong number of arguments to boot from ide\n");
		sub1 = strtol (argv[2], 0, 0);
		write_config_item(CFG_bootdev, (char*)&dev);
		write_config_item(CFG_boot_sub1, (char*)&sub1);
		printf ("changed to boot from ide drive, %s\n", (sub1 == 0) ? "master" : "slave");
		break;

	case bootdev_scsi:
		if (argc != 4)
			error ("wrong number of arguments to boot from scsi\n");
		sub1 = strtol (argv[2], 0, 0);
		sub2 = strtol (argv[3], 0, 0);
		write_config_item(CFG_bootdev, (char*)&dev);
		write_config_item(CFG_boot_sub1, (char*)&sub1);
		write_config_item(CFG_boot_sub2, (char*)&sub2);
		printf ("changed to boot from onboard scsi, target %d, lun %d\n", sub1, sub2);
		break;

	default:
		error ("new boot device %d is invalid\n", dev);
		break;
	}
	return 0;
}
