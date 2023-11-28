/*
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
*/
#include <support/SupportDefs.h>
#include <drivers/Drivers.h>

#include <partition.h>
#include <intel_map.h>

partition_type	types[] = 	{
		{BEOS_PARTITION, "BeOS", FALSE},
		{0x00, "Empty", TRUE},
		{0x01, "DOS 12-bit FAT", FALSE},
//		{0x02, "XENIX system", FALSE},
//		{0x03, "XENIX user", FALSE},
		{0x04, "DOS 16-bit FAT < 32MB", FALSE},
		{EXTENDED_PARTITION1, "Extended", TRUE},
		{0x06, "DOS 16-bit FAT >= 32MB", FALSE},
		{0x07, "HPFS/NTFS", FALSE},
		{0x08, "AIX", FALSE},
		{0x09, "AIX bootable", FALSE},
		{0x0a, "OS/2 Boot Manager", FALSE},
		{0x0b, "DOS 32-bit FAT", FALSE},
		{0x0c, "DOS 32-bit FAT (LBA)", FALSE},
		{0x0e, "DOS 16-bit FAT (LBA)", FALSE},
		{EXTENDED_PARTITION2, "Extended (LBA)", TRUE},
//		{0x10, "OPUS", FALSE},
//		{0x40, "Venix 80286", FALSE},
		{0x50, "Disk Manager", FALSE},
		{0x51, "Novell", FALSE},
//		{0x52, "Microport", FALSE},
		{0x63, "GNU HURD", FALSE},
		{0x64, "Novell Netware 286", FALSE},
		{0x65, "Novell Netware 386", FALSE},
//		{0x75, "PC/IX", FALSE},
		{0x77, "QNX4", FALSE},
		{0x78, "QNX4 2nd part", FALSE},
		{0x79, "QNX4 3rd part", FALSE},
		{0x80, "MINIX", FALSE},
		{0x81, "Linux", FALSE},
		{0x82, "Linux swap", FALSE},
		{0x83, "Linux native", FALSE},
		{EXTENDED_PARTITION3, "Linux extended", TRUE},
//		{0x93, "Amoeba", FALSE},
//		{0x94, "Amoeba BBT", FALSE},
		{0xa5, "BSD/386", FALSE},
		{0xa7, "NeXTStep 486", FALSE},
		{0xb7, "BSDI fs", FALSE},
		{0xb8, "BSDI swap", FALSE},
//		{0xc7, "Syrinx", FALSE},
//		{0xdb, "CP/M", FALSE},
//		{0xe1, "DOS access", FALSE},
//		{0xe3, "DOS R/O", FALSE},
//		{0xe4, "SpeedStor 16-bit FAT", FALSE},
//		{0xf1, "SpeedStor", FALSE},
//		{0xf2, "DOS secondary", FALSE},
//		{0xfe, "LANstep", FALSE},
//		{0xff, "Bad Block Table", FALSE},
		{-1, "E-O-F", FALSE}
};
