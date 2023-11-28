
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "drivers_priv.h"
#include <OS.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

static int ioctl_devices(const char * names[], uint32 code, int32 arg);

static int
ioctl_dir(
	const char * dir,
	uint32 code,
	int32 arg)
{
	DIR * d = opendir(dir);
	struct dirent * dent;
	int ret = 0;
	const char * nn[2];
	char name[256];
	char * end = &name[strlen(dir)];

	if (!d) {
		perror(dir);
		return -1;
	}
	strcpy(name, dir);
	*end++ = '/';
	while ((dent = readdir(d)) != 0) {
		if (dent->d_name[0] == '.') continue;
		strcpy(end, dent->d_name);
		nn[0] = name;
		nn[1] = NULL;
		ret = ioctl_devices(nn, code, arg) ? -1 : ret;
	}
	closedir(d);
	return ret;
}

static int
ioctl_devices(
	const char * names[],
	uint32 code,
	int32 arg)
{
	int fd;
	int err;
	int ret = 0;
	struct stat st;
	while (*names != 0) {
		if (!stat(*names, &st) && S_ISDIR(st.st_mode)) {
			ret = ioctl_dir(*names, code, arg) ? -1 : ret;
		}
		else {
			fd = open(*names, O_RDWR);
			if (fd < 0) {
				perror(*names);
				ret = -1;
				goto next;
			}
			err = ioctl(fd, code, &arg);
			if (err < 0) {
				perror(*names);
				ret = -1;
				goto close_next;
			}
	close_next:
			close(fd);
		}
next:
		names++;
	}
	return err;
}

int
port_names(
	const char *names[],
	uint32 cmd)
{
	port_id p;
	status_t err;
	int ret = 0;
	while (*names != 0) {
		p = find_port(*names);
		if (p < 0) {
			errno = p;
			perror(*names);
			ret = -1;
			goto next;
		}
		err = write_port_etc(p, cmd, 0, 0, B_TIMEOUT, 1000000LL);
		if (err < 0) {
			errno = p;
			perror(*names);
			ret = -1;
			goto next;
		}
next:
		names++;
	}
	return ret;
}


int
main(
	int argc,
	const char * argv[])
{
	if (argc < 2) {
error:
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "nut check    (returns true for awake, and false for sleeping)\n");
		fprintf(stderr, "nut toggle    (inverts the sense of sleep, without talking to devices)\n");
		fprintf(stderr, "nut sleep device +\n");
		fprintf(stderr, "nut wakeup device +\n");
		fprintf(stderr, "nut ioctl code intval device +\n");
		fprintf(stderr, "nut port command portname +\n");
		return 1;
	}
	if (!strcmp(argv[1], "check")) {
		struct stat st;
		if (stat("/tmp/sleeping", &st) < 0) {
			return 0;	//	awake
		}
		else {
			return 1;	//	sleeping
		}
	}
	if (!strcmp(argv[1], "toggle")) {
		struct stat st;
		if (stat("/tmp/sleeping", &st) < 0) {
			close(open("/tmp/sleeping", O_RDWR | O_CREAT | O_TRUNC));
			return 0;	//	was awake
		}
		else {
			unlink("/tmp/sleeping");
			return 1;	//	was sleeping
		}
	}
	if (argc < 3) {
		goto error;
	}
	if (!strcmp(argv[1], "sleep")) {
		return ioctl_devices(argv+2, B_SET_POWERSAVE, 1);
	}
	if (!strcmp(argv[1], "wakeup")) {
		return ioctl_devices(argv+2, B_SET_POWERSAVE, 0);
	}
	if (argc < 4) goto error;
	if (!strcmp(argv[1], "port")) {
		uint32 cmd = atol(argv[2]);
		return port_names(argv+3, cmd);
	}
	if (argc < 5) goto error;
	if (!strcmp(argv[1], "ioctl")) {
		uint32 code = atol(argv[2]);
		int32 value = atol(argv[3]);
		if (code == 0) goto error;
		return ioctl_devices(argv+4, code, value);
	}
	goto error;
	return 0;
}
