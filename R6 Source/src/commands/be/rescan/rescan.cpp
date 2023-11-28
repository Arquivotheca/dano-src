/* ++++++++++
	FILE:	rescan.cpp
	REVS:	$Revision: 1.1 $
	NAME:	herold
	DATE:	Thu Mar 20 10:21:51 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <scsi.h>
#include <errno.h>
#include <string.h>

int
rescan(const char *dev)
{
	int	fd;

	if ((fd = open("/dev",O_WRONLY)) < 0) {
		printf ("rescan: open of /dev failed, %s\n", dev, strerror(errno));
		goto bail0;
	}
	
	if(write(fd, dev, strlen(dev)) < 0) {
		printf ("rescan: rescan failed, %s\n", strerror(errno));	
		goto bail;
	}

	close (fd);
	return 0;
	
bail:
	close (fd);
bail0:
	return -1;
}

int main(long argc, char* argv[])
{
	int	err = 0;
	int i;
	
	if (argc == 1) {
		printf("scanning scsi disks...\n");
		err |= rescan("scsi_dsk");
		printf("scanning scsi cdroms...\n");
		err |= rescan("scsi_cd");
		printf("scanning ide ata...\n");
		err |= rescan("ata");
		printf("scanning ide atapi...\n");
		err |= rescan("atapi");
		return err;
	}
	
	for (i = 1; i < argc; i++) {
		printf("scanning %s...\n", argv[i]);
		err |= rescan(argv[i]);
	}

	return err;
}
