/* test/set the boot mode persistent state. */
/* bootmode -- will print current boot mode */
/* bootmode modename -- will return 1 if not in modename, and 0 if in modename */
/* bootmode --set [--altq] modename -- set a new mode (optionally allowing altq) */
/* returns 2 for usage error and 3 for I/O error */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <priv_syscalls.h>
#include <Drivers.h>


static void
set_mode(
	const char * mode,
	int altq)
{
	/* should really look up TellBrowser here and send a message... */
	FILE * f = fopen("/boot/home/config/settings/beia-bootmode", "w");
	if (f != NULL) {
		fprintf(f, "BOOTMODE=\"%s\"\nALTQ=%d\n", mode, altq);
		fclose(f);
	}
	else {
		exit(3);
	}
}

static
int
open_boot_device(void)
{
	/*
	 * most of this is ripped from bootcomplete
	 *
	 * BeIA MBR is composed of 3 sectors, the first one is
	 * the acutal code, the second one is a flag sector used
	 * for boot failure detection. The third one are the
	 * string messages for the MBR.
	 *
	 * This third sector is also used for disabling bootkeys
	 * when the system is converted to release mode.
	 *
	 * If this sector contain at offset 0xF0 the strin "Tori"
	 * and the checksum for the whole sector is 0 the bootkeys
	 * are disabled.
	 *
	 * See file bt_misc.c in zbeos sources.
	 *
	 */
	status_t err;
	fs_info info;
	int fd;
	partition_info pi;
	
	err = _kstatfs_(-1, NULL, -1, "/boot", &info);
	if(err != B_NO_ERROR) {
		fprintf(stderr, "can't find boot device, %s\n", strerror(err));
		return -1;
	}
	fd = open(info.device_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "can't open boot device, %s\n", strerror(errno));
		return -1;
	}
	if(ioctl(fd, B_GET_PARTITION_INFO, &pi) < 0) {
		fprintf(stderr, "can't get partition info for boot device, %s\n",
		        strerror(fd));
		close(fd);
		return -1;
	}
	close(fd);
	//printf("dev %ld, name %s, raw device %s\n",
	//       info.dev, info.device_name, pi.device);
	fd = open(pi.device, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "can't open raw boot device, %s\n", strerror(errno));
		return -1;
	}
	
	return fd;
}

static
status_t
set_no_boot_keys()
{
	int fd;
	uint8 block_data[512];
	int i;
	uint8 acum;

    fd= open_boot_device();
    if(fd< 0) {
    	return B_ERROR;
    }

	if(read_pos(fd, 1024, block_data, sizeof(block_data)) != sizeof(block_data)) {
		fprintf(stderr, "can't read raw boot device, %s\n", strerror(errno));
		close(fd);
		return B_ERROR;
	}
	
	block_data[0xF0]= 'T';
	block_data[0xF1]= 'o';
	block_data[0xF2]= 'r';
	block_data[0xF3]= 'i';
	block_data[0xF4]=  0;
	acum= 0;
	for(i = 0; i < 512; i++) {
		acum+= block_data[i];
	}
	acum^= 0xFF;
	acum++;
	block_data[0xF4]=  acum;
	
	if(write_pos(fd, 1024, block_data, sizeof(block_data)) != sizeof(block_data)) {
		fprintf(stderr, "can't write raw boot device, %s\n", strerror(errno));
		close(fd);
		return B_ERROR;
	}
	close(fd);

	return B_OK;
}

#include <recovery/factory_settings.h>
#define BOOTSECTOR_SIZE 512
static
status_t
configure_recovery(char const *file)
{
	int    i;
	int    fd;
	
	uchar sect[BOOTSECTOR_SIZE];
    uchar mbr[BOOTSECTOR_SIZE];
    struct partition_entry {
        uchar   active;
        uchar   start_chs[3];
        uchar   id;
        uchar   end_chs[3];
        uint32  start_lba;
        uint32  len_lba;
    } *pent= (struct partition_entry *)(mbr+0x1be);

	uint32 zr_offset;
	uint32 zr_length;

	uint32 factory_offset;
	uint32 factory_size;

	factory_settings_t *factory;

	if(!file) {
		return B_ERROR;
	}
	
	fd= open_boot_device();
	if(fd< 0) {
		return B_ERROR;
	}
	
	/*
	 * fetch the MBR
	 */
	if(read_pos(fd, 0LL, mbr, sizeof(mbr))!= sizeof(mbr)) {
		close(fd);
		return B_ERROR;
	}

	/*
	 * query second partition
	 */
	if(pent[1].id!= 0xeb) {
		close(fd);
		fprintf(stderr, "Recovery partition not found\n");
		return B_ERROR;
	}
	
	zr_offset= pent[1].start_lba;	 
	zr_length= pent[1].len_lba;
	
	/*
	 * verify the recovery image
	 */
	if(read_pos(fd, BOOTSECTOR_SIZE*zr_offset, sect, sizeof(sect))!= sizeof(sect)) {
		close(fd);
		fprintf(stderr, "Recovery partition not found or corrupted\n");
		return B_ERROR;
	}
	if((sect[0x00]!= 0xeb) || (sect[BOOTSECTOR_SIZE-2]!= 0x55) || (sect[BOOTSECTOR_SIZE-1]!= 0xaa)) {
		close(fd);
		fprintf(stderr, "Recovery partition test 1 failed %02x %02x %02x\n", sect[0x00], sect[BOOTSECTOR_SIZE-2], sect[BOOTSECTOR_SIZE-1]);
		return B_ERROR;
	}
	if(((unsigned short*)(sect+0x1f0))[0]!= zr_length) {
		close(fd);
		fprintf(stderr, "Size mismach between zrecover and partition table\n");
		return B_ERROR;
	}
	
	/*
	 * try to locate the old factory settings
	 */
	factory_size  = 0;
	factory_offset= 0;
	for(i= 1; i< zr_length; i++) {
		factory_settings_buffer_t *aux= (factory_settings_buffer_t*)(sect);
		
		if(read_pos(fd, BOOTSECTOR_SIZE*(zr_offset+i), sect, sizeof(sect))!= sizeof(sect)) {
			close(fd);
			fprintf(stderr, "Read error while searching for settings\n");
			return B_ERROR;
		}
		
		if(aux->magik!= ZR_FACTORY_MAGIC) {
			continue;
		}
		if(aux->size/BOOTSECTOR_SIZE+i!= zr_length) {
			continue;
		}
		if(aux->used> aux->size) {
			continue;
		}
		
		factory_offset= i;
		factory_size  = aux->size;
	}

	
	if(!factory_size) {
		close(fd);
		fprintf(stderr, "Factory settings reserved area not found\n");
		return B_ERROR;
	}
	
	/*
	 * create settings from file & flash them
	 */
	factory= create_settings_from_file(file, factory_size);
	if(!factory) {
		close(fd);
		fprintf(stderr, "Could not create a factory settings description\n");
		return B_ERROR;
	}
	
	if(factory->size!= factory_size) {
		close(fd);
		fprintf(stderr, "Internal error.\n");
		return B_ERROR;
	}
	if(write_pos(fd, BOOTSECTOR_SIZE*(zr_offset+factory_offset), factory->buffer, factory->size)!= factory_size) {
		close(fd);
		fprintf(stderr, "Problems updating the recovery settings... system may be damaged\n");
		return B_ERROR;
	}
	
	close(fd);
	return B_OK;
}

int
main(
	int argc,
	char * argv[])
{
	int altq = 0;
	if (argc > 4 || (argc == 2 && !strcmp(argv[1], "--help"))) {
usage:
		fprintf(stderr, "usage: bootmode [[--set [--altq] mode] | [testmode] | --no-boot-keys ] | --configure-recovery FactorySettingsFile\n");
		return 2;
	}
	if (argc == 1) {
		if (!getenv("BOOTMODE")) {
			fprintf(stderr, "BOOTMODE is not currently set.\n");
			return 3;
		}
		puts(getenv("BOOTMODE"));
		return 0;
	}
	if (!strcmp(argv[1], "--set")) {
		if (!argv[2]) {
			goto usage;
		}
		if (!strcmp(argv[2], "--altq")) {
			altq = 1;
		}
		if (argv[2+altq] == NULL) {
			goto usage;
		}
		set_mode(argv[2+altq], altq);
		fprintf(stderr, "You must reboot for the new mode to take effect.\n");
	}
	else if (strcmp(argv[1], "--no-boot-keys")== 0) {
		set_no_boot_keys();
	}
	else if( (strcmp(argv[1], "--configure-recovery")== 0) && (argc== 3)) {
		configure_recovery(argv[2]);
	}
	else if (argc != 2) {
		goto usage;
	}
	else {
		if (!getenv("BOOTMODE")) {
			fprintf(stderr, "BOOTMODE is not currently set.\n");
			return 3;
		}
		if (!strcmp(getenv("BOOTMODE"), argv[1])) {
			return 0;
		}
		return 1;
	}
	return 0;
}
