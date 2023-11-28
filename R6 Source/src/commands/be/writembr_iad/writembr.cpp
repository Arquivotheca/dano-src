#include <Application.h>
#include <private/storage/DeviceMap.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <support/Errors.h>

#include "mbr.h"

status_t GetDriveID(const char *device, uchar *id)
{
	status_t err;
	int fd;
	assert(id);
	*id = 0xff;
	fd = open(device, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "error opening %s\n", device);
		return fd;
	}
	err = ioctl(fd, B_GET_BIOS_DRIVE_ID, id, sizeof(uchar));
	close(fd);
	return err;
}

Device *FindBootDevice(Device *device, void *cookie)
{
	uchar id;
	GetDriveID(device->Name(), &id);
	if (id == 0x80) {
		*(Device **)cookie = device;
		fprintf(stderr, "Found boot device: %s\n", device->Name());
		return device;
	}
	return NULL;
}

int usage(char *self)
{
	fprintf(stderr, "usage: %s [ device ]\n", self);
	fprintf(stderr, "        Rewrites the MBR for the specified device\n");
	fprintf(stderr, "        If no device is specified, the boot device is used\n");
	return EINVAL;
}

int main(int argc, char **argv)
{
	unsigned char oldmbr[0x200], newmbr[0x200];
	char *self, device[512], confirm[8];
	int fd, error;

	self = strrchr(*argv, '/');
	self = self ? (self + 1) : *argv;

	if (argc > 2)
		return usage(self);

	if (argc == 2) {
		struct stat st;
		if (stat(argv[1], &st) != B_OK) {
			fprintf(stderr, "error opening %s\n", argv[1]);
			return usage(self);
		}
		if (strncmp(argv[1], "/dev/disk/", 10) ||
				strcmp(argv[1] + strlen(argv[1]) - 4, "/raw")) {
			fprintf(stderr, "%s: Must be run on a raw device\n", self);
			return usage(self);
		}
		strncpy(device, argv[1], sizeof(device) - 1);
		device[sizeof(device)-1] = 0;
	} else {
		BApplication app("application/x-vnd.Be-writembr");
		DeviceList Devices;
		Device *BootDevice = NULL;
		Devices.RescanDevices(true);
		Devices.UpdateMountingInfo();
		Devices.EachDevice(FindBootDevice, &BootDevice);
		if (BootDevice == NULL) {
			fprintf(stderr, "%s: Unable to locate boot device.\n", self);
			return usage(self);
		}
		strncpy(device, BootDevice->Name(), sizeof(device) - 1);
		device[sizeof(device)-1] = 0;
	}

	fprintf(stderr,
"About to overwrite the MBR boot code on %s.\n"
"This may disable any partition managers you have installed. If you installed\n"
"a program such as DiskManager to access portions of the disk past 512 MB,\n"
"2 GB, or 8 GB, then you shouldn't continue.\n"
"Do you wish to proceed? ", device);
	if (fgets(confirm,7,stdin) == NULL)
		return -1;
	if (strcasecmp(confirm,"y\n") && strcasecmp(confirm,"yes\n")) {
		printf("MBR NOT written\n");
		return -1;
	}

	fprintf(stderr, "Rewriting MBR for %s\n", device);

	fd = open(device, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: Error opening %s (%s)\n", self, device, strerror(fd));
		fprintf(stderr, "%s: MBR *NOT* written\n", self);
		return fd;
	}

	error = read(fd, oldmbr, 0x200);
	if (error < 0x200) {
		fprintf(stderr, "%s: Error reading %s (%s)\n", self, device, strerror(error));
		fprintf(stderr, "%s: MBR *NOT* written\n", self);
		return error;
	}

	memcpy(newmbr, mbrcode, 0x200);
	memcpy(newmbr + 0x1be, oldmbr + 0x1be, 0x40);

	error = write_pos(fd, 0, newmbr, 0x200);
	close(fd);
	if (error < 0x200) {
		fprintf(stderr, "%s: Error writing MBR to %s (%s)\n", self, device,
				strerror(error));
		return error;
	}

	return 0;
}
