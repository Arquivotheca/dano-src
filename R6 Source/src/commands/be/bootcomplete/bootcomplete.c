#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <priv_syscalls.h>
#include <Drivers.h>

int
main(int argc, char **argv)
{
	status_t err;
	fs_info info;
	int fd;
	partition_info pi;
	uint8 block_data[512];
	int i;
	int test = 0;
	int do_recovery = 0;
	
	if(argc > 1 && !strcmp(argv[1], "--test")) {
		test = 1;
	}
	if(argc > 1 && !strcmp(argv[1], "--do_recovery")) {
		do_recovery = 1;
	}
	
	err = _kstatfs_(-1, NULL, -1, "/boot", &info);
	if(err != B_NO_ERROR) {
		fprintf(stderr, "can't find boot device, %s\n", strerror(err));
		return 1;
	}
	fd = open(info.device_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "can't open boot device, %s\n", strerror(errno));
		return 1;
	}
	if(ioctl(fd, B_GET_PARTITION_INFO, &pi) < 0) {
		fprintf(stderr, "can't get partition info for boot device, %s\n",
		        strerror(fd));
		close(fd);
		return 1;
	}
	close(fd);
	//printf("dev %ld, name %s, raw device %s\n",
	//       info.dev, info.device_name, pi.device);
	fd = open(pi.device, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "can't open raw boot device, %s\n", strerror(errno));
		return 1;
	}
	if(read_pos(fd, 512, block_data, sizeof(block_data)) != sizeof(block_data)) {
		fprintf(stderr, "can't read raw boot device, %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	if(test) {
		int ret = 0;
		for(i = 0; i < 512; i++)
			if(block_data[i] != 0)
				break;
		close(fd);
		return (i != 512);
	}
	else if(do_recovery) {
		for(i = 0; i < 512; i++)
			if((block_data[i] != 0xCA) && (block_data[i] != 0x00))
				break;
		if(i != 512) {
			fprintf(stderr, "Boot progress sector does not have correct value\n");
			close(fd);
			return 1;
		}
		memset(block_data, 0xCA, sizeof(block_data));
		if(write_pos(fd, 512, block_data, sizeof(block_data)) != sizeof(block_data)) {
			fprintf(stderr, "can't write raw boot device, %s\n", strerror(errno));
			close(fd);
			return 1;
		}
		close(fd);
	}
	else {
		for(i = 0; i < 512; i++)
			if(block_data[i] != 0xCA)
				break;
		if(i != 512) {
			fprintf(stderr, "Boot progress sector does not have correct value\n");
			close(fd);
			return 1;
		}
		memset(block_data, 0, sizeof(block_data));
		if(write_pos(fd, 512, block_data, sizeof(block_data)) != sizeof(block_data)) {
			fprintf(stderr, "can't write raw boot device, %s\n", strerror(errno));
			close(fd);
			return 1;
		}
		close(fd);
	}
	return 0;
}
