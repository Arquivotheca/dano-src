#include <Application.h>
#include <Alert.h>

#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <support/SupportDefs.h>
#include <drivers/Drivers.h>

#include <StorageKit.h>

#include <kernel/fs_info.h>

#define patch1_offset full_patch1_offset
#define patch2_offset full_patch2_offset
#define bootsector full_bootsector

#include "bootsector.h"

#undef bootsector
#undef patch1_offset
#undef patch2_offset

#define patch1_offset safe_patch1_offset
#define patch2_offset safe_patch2_offset
#define bootsector safe_bootsector

#include "safe.h"

#undef bootsector
#undef patch1_offset
#undef patch2_offset

struct _ioctl_params {
		uint32 magic;
		uint32 bufflen;
		uchar  buff[0];
};

bool alert = FALSE;
char *self;

int error(const char *format, ...)
{
	va_list args;
	char buf[4096];
	int ret;

	va_start(args, format);
	ret = vsprintf(buf, format, args);
	if (alert) {
		if (!be_app) new BApplication("application/x-vnd.Be.makebootable");

		(new BAlert("Error", buf, "Okay", NULL, NULL, B_WIDTH_AS_USUAL,
				B_STOP_ALERT))->Go();
	} else {
		fprintf(stderr, buf);
	}
	va_end(args);

	return ret;
}


static
int makebootable(const char *base, uint32 patch1_offset, uint32 patch2_offset,
		struct _ioctl_params *ioctl_params)
{
	int fd, err;
	partition_info info;
	fs_info fsinfo;
	char root[B_FILE_NAME_LENGTH], device[B_FILE_NAME_LENGTH];
	uchar biosid;
	BPath path;

	path.SetTo(base);

	if (path.InitCheck() != B_OK) {
		error("%s: Invalid path \"%s\"\n", self, base);
		return EINVAL;
	}

	if (!strncmp(path.Path(), "/dev/", 5)) {
		BVolumeRoster roster;
		BVolume volume;

		strcpy(device, path.Path());
		root[0] = 0;

		roster.Rewind();
		while (roster.GetNextVolume(&volume) == B_OK)
			if ((fs_stat_dev(volume.Device(), &fsinfo) == 0) &&
					!strcmp(fsinfo.device_name, device)) {
				BDirectory dir;
				BEntry entry;
				BPath path;
				if (volume.GetRootDirectory(&dir) ||
						dir.GetEntry(&entry) ||
						entry.GetPath(&path)) {
					error("%s: error getting root directory for device %s\n", self, device);
					return EINVAL;
				}
				strcpy(root, path.Path());
				break;
			}
	} else {
		strcpy(root, path.Path());

		if (fs_stat_dev(dev_for_path(root), &fsinfo) < 0) {
			error("%s: error locating device for %s\n", self, root);
			return EINVAL;
		}

		strcpy(device, fsinfo.device_name);
	}

	if (strncmp(device, "/dev/disk/", 10)) {
		if (root)
			error("%s: %s lies on an invalid device (%s)\n", self, root, device);
		else
			error("%s: Can't make device %s bootable\n", self, device);

		return EINVAL;
	}

	if (root[0] == 0) {
		error("%s: %s must be mounted to be made bootable\n", self, device);
		return EINVAL;
	}

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		error("%s: error opening %s\n", self, device);
		return fd;
	}

	err = ioctl(fd, B_GET_BIOS_DRIVE_ID, &biosid, sizeof(uchar));
	if (err < 0) {
		error("Cannot find the BIOS drive id for:\n\n\t%s\n\nIf you have trouble booting the BeOS, use the BeOS boot floppy.\n", device);
		biosid = 0x80;
	}

	err = ioctl(fd, B_GET_PARTITION_INFO, &info, sizeof(info));
	close(fd);

	if (err < 0) {
		if (!strcmp(device + strlen(device) - 4, "/raw")) {
			info.offset = 0;
			info.logical_block_size = 512;
		} else {
			error("%s: error fetching partition information for %s\n", self, device);
			return err;
		}
	}

	*(uchar *)(ioctl_params->buff + patch1_offset) = biosid;
	*(uint32 *)(ioctl_params->buff + patch2_offset) =
			(uint32)(info.offset / info.logical_block_size);

	fd = open(root, O_RDONLY);
	if (fd < 0) {
		error("%s: error opening %s\n", self, root);
		return fd;
	}

	if (ioctl(fd, 10004, ioctl_params, sizeof(struct _ioctl_params)) < 0) {
		error("%s: error writing boot block information to %s\n", self, device);
		close(fd);
		return -1;
	}

	close(fd);

	fprintf(stderr, "%s successfully made bootable.\n", device);

	return 0;
}

int main(int argc, char **argv)
{
	struct _ioctl_params *ioctl_params;
	int i;
	uchar *bootsector;
	uint32 patch1_offset, patch2_offset;

	struct {
		char	*name;
		int		*patch1_offset;
		int		*patch2_offset;
		uchar	*code;
	} bootsectors[] = {
		{ "full", &full_patch1_offset, &full_patch2_offset, full_bootsector },
		{ "safe", &safe_patch1_offset, &safe_patch2_offset, safe_bootsector },
		{ NULL, NULL, NULL }
	};

	self = strrchr(*argv, '/');
	self = self ? self+1 : *argv;

	if (argc < 2) {
		error("\
usage: %s [-alert] [-safe | -full] [ disk1 [ disk2 [ ... ] ] ]\n\
       Writes BeOS boot code to the boot sector of the specified partition.\n\
       It does NOT make the partition the default boot volume nor does it\n\
       make the partition active. Use the Boot preferences to set the default\n\
       BeOS boot volume and DriveSetup to set the active partition.\n\
\n\
       -alert directs errors to alerts instead of the console\n\
\n\
       -safe selects the R3-compatible boot sector\n\
             use this if you still use BeOS R3\n\
       -full selects the full boot sector (default)\n\
             use this if -safe fails or if you don't run BeOS R3\n\
\n\
       disk# is either the name of the device or the name of a\n\
       file on the device to be made bootable\n\
\n\
sample usage:\n\
       %s -full /layne
             make the volume named 'layne' bootable with the full boot sector\n\
       %s /boot /dev/disk/ide/ata/0/master/0/0_0\n\
             make both the boot device and the first partition on the master\n\
             drive of the primary IDE chain bootable with the safe boot sector\n"
			, self, self, self);
		return 1;
	}

	patch1_offset = *(bootsectors[0].patch1_offset);
	patch2_offset = *(bootsectors[0].patch2_offset);
	bootsector = bootsectors[0].code;

	i = 1;
	while (argv[i][0] == '-') {
		int j;
		if (!strcasecmp(argv[i] + 1, "alert")) {
			alert = TRUE;
		} else {
			for (j=0;bootsectors[j].name;j++) {
				if (!strcasecmp(argv[i] + 1, bootsectors[j].name)) {
					printf("Using %s mode bootsector\n", bootsectors[j].name);
					patch1_offset = *(bootsectors[j].patch1_offset);
					patch2_offset = *(bootsectors[j].patch2_offset);
					bootsector = bootsectors[j].code;
					break;
				}
			}
			if (bootsectors[j].name == NULL) {
				printf("Unknown option '%s'\n", argv[i]);
				return 1;
			}
		}
		i++;
	}

	ioctl_params = (struct _ioctl_params *)malloc(
			sizeof(struct _ioctl_params) + 0x400);
	if (ioctl_params == NULL) {
		error("%s: out of core\n", self);
		return 1;
	}

	ioctl_params->magic = 'yzzO';
	ioctl_params->bufflen = 0x400;
	memcpy(ioctl_params->buff, bootsector, 0x400);

	if (i == argc) {
		error("Please specify the drive to be made bootable\n");
		return 1;
	}

	for (;i<argc;i++)
		makebootable(argv[i], patch1_offset, patch2_offset, ioctl_params);

	free(ioctl_params);

	return 0;
}

