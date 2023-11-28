#include <support/SupportDefs.h>
#include <drivers/KernelExport.h>

#include <ctype.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <kFindDirectory.h>

#include <apm.h>
#include <biosdriveinfo.h>
#include <boot.h>
#include <bootscreen.h>
#include <driver_settings.h>
#include <driver_settings_p.h>
#include <fmap.h>
#include <mem.h>
#include <ram.h>
#include <sio.h>
#include <vm.h>

#include "bios.h"
#include "bootmenu.h"
#include "bt_misc.h"
#include "cpu_asm.h"
#include "platform.h"
#include "vesa.h"

#ifdef MEMORY_TOOLS
extern int flag_noswap;
extern int flag_limit16;
extern int flag_limit24;
#endif

/* prototypes for functions not defined in any headers */
extern bool load_elf_kernel(const char *mount, const char *kernel_name,
		const char *fs, char *vbase, int (**entry)(), void **kinfo, void **fsinfo,
		uint32 num_drivers, struct boot_filelist *drivers, 
		uint32 num_modules, struct boot_filelist *modules, char **brkval);
extern status_t build_directory_list(const char *path, uint32 *num,
        struct boot_filelist **files);
extern status_t create_bios_drive_info_table(
		uint32 *num_entries, struct bios_drive_info **table);

/* safe mode options */
extern int flag_safemode, flag_safemodevga, flag_nobios, flag_nomp,
		flag_noidedma, flag_nouseraddons, flag_consoledebug;

uchar *loadaddr;
static uint32 calledfromdos;
uint32 boot_drive;
static uint32 boot_partition_start;

int argc;
char **argv;

#define MEMMAP_ENTRIES 8
static uint32 memmap[MEMMAP_ENTRIES * 2];
static uint32 total_memsize;

#if 0
struct match_bpb_params
{
	int len;
	uchar *bpb;
	char *path;
};

static status_t
match_bpb(void *cookie, const char *path)
{
	int fd, len;
	uchar buffer[512];
	struct match_bpb_params *p = (struct match_bpb_params *)cookie;

	fd = open(path, 0);
	if (fd < 0)
		return 0;
	len = read_pos(fd, 0LL, buffer, 512);
	close(fd);

	if (len != 512)
		return 0;

	if (!memcmp(buffer + 11, p->bpb, p->len)) {
		strcpy(p->path, path);
		return 1;
	}

	return 0;
}
#endif

static status_t
add_fmap(void *cookie, const char *path)
{
	struct bootable_volume_info *volume = (struct bootable_volume_info *)cookie;
	struct fmap_info *f;
	char name[64];
	static int32 fmap_count = 0;
	static int32 fmap_size = 0;
	int fd;

	if (strcmp(path + strlen(path) - 4, "/raw"))
		return 0;

	fd = open(path, 0);
	if ((fd < 0) ||
		(ioctl(fd, 'fmap', &f, 0) < 0))
		panic("error getting file block map\n");
	close(fd);

	if (f->type == FMAP_TYPE_BLOCK)
		dprintf("Adding block fmap for %s: %ld runs, %ld bytes\n",
				path, f->u.block.num_block_runs, f->size);
	else
		dprintf("Adding byte fmap for %s: %ld runs, %ld bytes\n",
				path, f->u.byte.num_byte_runs, f->size);

	fmap_size += f->size;

	if (fmap_size >= 256 * 1024) {
		panic(
			"Mounted disk image files on non-bfs filesystems are too heavily "
			"fragmented. You must defragment the drives for the other "
			"operating systems before continuing."
		);
	}

	sprintf(name, BOOT_FMAP_DISK_PREFIX "%ld", fmap_count);
	add_boot_item(name, f->size, f);

	if (!strncmp(path, volume->boot_device, strlen(path) - 3)) {
		partition_info p;
		int i;

		fd = open(volume->boot_device, 0);
		if (	(fd < 0) ||
				(ioctl(fd, B_GET_PARTITION_INFO, &p, sizeof(p)) != B_OK))
			panic("error getting partition info for boot device %s\n",
					volume->boot_device);
		close(fd);

		p.size = 0;
		if (f->type == FMAP_TYPE_BLOCK) {
			for (i=0;i<f->u.block.num_block_runs;i++)
				p.size += f->u.block.block_runs[i].num_blocks;
			p.size *= f->u.block.block_size;
		} else {
			for (i=0;i<f->u.byte.num_byte_runs;i++)
				p.size += f->u.byte.byte_runs[i].num_bytes;
		}
		p.logical_block_size = 0x200;
		p.session = 0;
		sprintf(name, "/dev/disk/virtual/fmap/%ld/raw", fmap_count);
		add_boot_item(BOOT_KERNEL_DEV, strlen(name) + 1, name);
		add_boot_item(BOOT_KERNEL_OF, sizeof(off_t), &p.offset);
		add_boot_item(BOOT_KERNEL_SZ, sizeof(off_t), &p.size);
		add_boot_item(BOOT_KERNEL_LBS,sizeof(int32),&p.logical_block_size);
		add_boot_item(BOOT_KERNEL_SID, sizeof(int32), &p.session);
		add_boot_item(BOOT_KERNEL_PID, sizeof(int32), &p.partition);
	}

	fmap_count++;

	return 0;
}

#define COMMON_SETTINGS_DIR	"/boot/" K_COMMON_SETTINGS_DIRECTORY "/"
#define COMMON_ADDONS_DIR	"/boot/" K_COMMON_ADDONS_DIRECTORY "/"
#define SYSTEM_ADDONS_DIR	"/boot/" K_BEOS_ADDONS_DIRECTORY "/"

void
boot(struct bootable_volume_info *volume, int32 boot_order)
{
	void *kinfo, *fsinfo;
	char *brkval1;
	int (*entry)();
	char *vbase;
	char *fs_name = "bfs";

	int dprintf_state = dprintf_enabled || flag_consoledebug;
	uint32 num_drivers, num_modules, num_drives;
	struct boot_filelist *drivers, *modules;
	struct stat st;

	if (flag_safemode)
		flag_safemodevga = flag_nobios = flag_nomp = flag_noidedma =
				flag_nouseraddons = 1;

	recurse("/dev/fmap", add_fmap, volume);
	
	dprintf("Mounting boot volume (%s)\n", volume->boot_device);
	if (mount((boot_order != BOOT_ORDER_DISK) ? "ffs" : "bfs",
			"/boot", volume->boot_device, 0, NULL, 0) < B_OK) {
		if (mount((boot_order != BOOT_ORDER_DISK) ? "ffs" : "cfs",
				"/boot", volume->boot_device, 0, NULL, 0) < B_OK)
			panic("Could not mount %s\n", volume->boot_device);
		else
			fs_name = "cfs";
	}

	if (stat(COMMON_SETTINGS_DIR "kernel/enable_debugging_output", &st) == B_OK)
		dprintf_state = TRUE;

	build_directory_list(
			(		(boot_order == BOOT_ORDER_CD_DISK) || 
					(boot_order == BOOT_ORDER_CD_ONLY)) ?
				(flag_nouseraddons ?
					    SYSTEM_ADDONS_DIR "kernel/bus_managers"
					":" SYSTEM_ADDONS_DIR "kernel/busses"
					":" SYSTEM_ADDONS_DIR "kernel/generic" :
					    COMMON_ADDONS_DIR "kernel/bus_managers"
					":" COMMON_ADDONS_DIR "kernel/busses"
					":" COMMON_ADDONS_DIR "kernel/generic"
					":" SYSTEM_ADDONS_DIR "kernel/bus_managers"
					":" SYSTEM_ADDONS_DIR "kernel/busses"
					":" SYSTEM_ADDONS_DIR "kernel/generic") :
				(flag_nouseraddons ?
					    SYSTEM_ADDONS_DIR "kernel/bus_managers"
					":" SYSTEM_ADDONS_DIR "kernel/busses" :
					    COMMON_ADDONS_DIR "kernel/bus_managers"
					":" COMMON_ADDONS_DIR "kernel/busses"
					":" SYSTEM_ADDONS_DIR "kernel/bus_managers"
					":" SYSTEM_ADDONS_DIR "kernel/busses"),
			&num_modules, &modules);

	/* load disk drivers */
	build_directory_list(
			(		(boot_order == BOOT_ORDER_CD_DISK) || 
					(boot_order == BOOT_ORDER_CD_ONLY)) ?
				(flag_nouseraddons ?
					    SYSTEM_ADDONS_DIR "kernel/drivers/dev" :
					    COMMON_ADDONS_DIR "kernel/drivers/dev"
					":" SYSTEM_ADDONS_DIR "kernel/drivers/dev") :
				(flag_nouseraddons ?
					    SYSTEM_ADDONS_DIR "kernel/drivers/dev/disk" :
					    COMMON_ADDONS_DIR "kernel/drivers/dev/disk"
					":" SYSTEM_ADDONS_DIR "kernel/drivers/dev/disk"), 
			&num_drivers, &drivers);

	if (calledfromdos == 0)
		vbase = (char *)0x100000;
	else if (calledfromdos == 1)
		vbase = (char *)0x110000;	/* Avoid HMA */
	else {
		/* Load kernel past end of VCPI-mapped memory */
		vbase = (char *)(calledfromdos & ~0xfff);
	}

	if ((uint32)vbase >= 0x200000)
		panic("Requested kernel load address (0x%x) too high!\n", vbase);

	dprintf("Loading kernel at %p\n", vbase);

	if (load_elf_kernel("/boot", volume->kernel, fs_name, vbase, &entry, 
			&kinfo, &fsinfo, num_drivers, drivers,
			num_modules, modules, &brkval1) == FALSE)
		return;

	// *(char **)KERNEL_BRKVAL = brkval1;
	// XXXdbg -- changed the kernel heap to start at 16 megs and
	//           go up from there so that it is less likely to 
	//           collide with the VM heap (which is now put before
	//           the kernel heap in memory).  We also have a caveat
	//           so that if total memory is <= 16 megs, we shove the
	//           brkval to 8 megs so we can boot.
	if (total_memsize > 16*1024*1024)
		*(char **)KERNEL_BRKVAL = 16*1024*1024;
	else
		*(char **)KERNEL_BRKVAL = 8*1024*1024;
	  
	
	add_boot_item(BOOT_MEMMAP, 8 * sizeof(uint32), &memmap[0]);
	add_boot_item(BOOT_DEBUG_OUTPUT, 4, &dprintf_state);

	{
		struct bios_drive_info *bios_id_table;
		struct _apm_bios_info apm_info;

		if (create_bios_drive_info_table(&num_drives, &bios_id_table) == B_OK)
			add_boot_item(BOOT_BIOS_DRIVE_INFO,
					num_drives * sizeof(*bios_id_table), bios_id_table);
		if (initialize_apm(&apm_info) == B_OK)
			add_boot_item(BOOT_APM_INFO, sizeof(apm_info), &apm_info);
	}

	add_boot_item(BOOT_KERNEL_LOADER_INFO, 4, &kinfo);
	add_boot_item (BOOT_KERNEL_NAME, strlen(volume->kernel) + 1, (void *)volume->kernel);
	add_boot_item(BOOT_FS_LOADER_INFO, 4, &fsinfo);
	add_boot_item(BOOT_KERNEL_FS, 4, fs_name);
	add_boot_item(BOOT_MODULES, num_modules * sizeof(*modules), modules);
	add_boot_item(BOOT_DRIVERS, num_drivers * sizeof(*drivers), drivers);

	if (	(boot_order == BOOT_ORDER_DISK) &&
			(volume->master_boot_device[0] == 0)) {
		int fd;
		uint32 biosid;
		partition_info pi;

		fd = open(volume->boot_device, 0);
		if (fd < 0) panic("Unable to open boot device %s\n", volume->boot_device);
		ioctl(fd, B_GET_PARTITION_INFO, &pi, sizeof(pi));
		add_boot_item(BOOT_KERNEL_OF, sizeof(off_t), &pi.offset);
		add_boot_item(BOOT_KERNEL_SZ, sizeof(off_t), &pi.size);
		add_boot_item(BOOT_KERNEL_LBS,sizeof(int32),&pi.logical_block_size);
		add_boot_item(BOOT_KERNEL_SID, sizeof(int32), &pi.session);
		add_boot_item(BOOT_KERNEL_PID, sizeof(int32), &pi.partition);
		ioctl(fd, B_GET_BIOS_DRIVE_ID, &biosid, 1);
		add_boot_item(BOOT_DRIVE_ID, 1, &biosid);
		close(fd);
	}

	if (!flag_consoledebug)
		vesa_boot();

	/* VESA mode and console debugging are incompatible */
	if ((curscreen.depth != 0) && (curscreen.depth != 4))
		flag_consoledebug = FALSE;

	/* add safe-mode options */
	parse_command_line_options();
	if (flag_safemode) add_safemode_setting(B_SAFEMODE_SAFE_MODE "\n");

	curscreen.use_stub = flag_safemodevga;
	add_boot_item(BOOT_SCREEN_V2, sizeof(curscreen), &curscreen);

	#define KERNEL_SETTING "driveroption kernel { "
	#define OFF " disabled }\n"

	if (flag_nobios)
		add_safemode_setting(KERNEL_SETTING KERNEL_SETTING_BIOS_CALLS OFF);
	if (flag_nomp)
		add_safemode_setting(KERNEL_SETTING KERNEL_SETTING_MP_SUPPORT OFF);
	
	#undef OFF
	#undef KERNEL_SETTING

#ifdef MEMORY_TOOLS
	if(flag_noswap) add_safemode_setting("driveroption virtual_memory { vm 0 }\n");
#endif
				
	if (flag_noidedma) add_safemode_setting(SAFEMODE_DISABLE_IDE_DMA "\n");
	if (flag_nouseraddons) add_safemode_setting(SAFEMODE_DISABLE_USER_ADDONS"\n");
	
	add_boot_item(BOOT_CONSOLE_DEBUG, sizeof(int), &flag_consoledebug);

	if (	(boot_order == BOOT_ORDER_CD_DISK) ||
			(boot_order == BOOT_ORDER_CD_ONLY))
		add_boot_item(BOOT_ORDER, sizeof(int32), &boot_order);

	if (calledfromdos) add_boot_item(BOOT_FROM_DOS, sizeof(int), &calledfromdos);

	platform_pass_boot_icons();

	dprintf("Reading boot settings\n");
	read_boot_settings("/boot");

	dprintf("About to jump into the kernel at %p\n", entry);
	disable_interrupts();

	set_stack_and_jump((void *)(brkval1 + 
							    (BOOT_STK_SIZE * B_MAX_CPU_COUNT) + 
								VM_INIT_MEM_USAGE(total_memsize)), 
						entry);
	
	// entry();
}

#ifdef MEMORY_TOOLS
struct bootable_volume_info *mt_volume;
int32 mt_boot_order;

uint32 determine_stack_address();

void main3(void)
{
	struct bootable_volume_info boot_volume;
	memcpy(&boot_volume,mt_volume,sizeof(boot_volume));

	boot(&boot_volume, mt_boot_order);

	platform_enter_console_mode();
	dprintf("unable to load kernel\n");
	platform_get_key();

	disable_a20();
}
#endif

void main2(void)
{
	struct bootable_volume_info boot_volume, *v;
	int32 boot_order = BOOT_ORDER_NONE;

	memset(&boot_volume, 0, sizeof(boot_volume));

	/* build argument list */
	argc = 0; argv = NULL;
	if (calledfromdos) {
		uchar *p = (uchar *)loadaddr - 0x80;

		if (p[*p + 1] == 0x0d) {
			uchar last = ' ';

			argv = malloc(128 * sizeof(char *));
			argv[argc++] = NULL;

			p++;
			while ((*p != 0x0a) && (*p != 0x0d)) {
				if (isspace(last) && !isspace(*p))
					argv[argc++] = p;
				last = *p;
				if (isspace(*p))
					*p = 0;
				p++;
			}
			*p = 0;
		}
	}

	platform_initialize();

	check_boot_keys();

	if(dprintf_enabled || boot_menu_enabled) {
		set_video_mode(3);
		platform_notify_video_mode(80, 25, 0, 80, (void *)0xb8000);
	}

	if (!boot_menu_enabled && !fast_boot_enabled) {
		platform_enter_graphics_mode();
		platform_splash_screen(TRUE);
	}

	/* mount disk filesystem */
	mount("dev", "/dev", NULL, 0, NULL, 0);
	scan_partitions();
	rescan_bootable_volumes();

	/* look for boot partition */
	if (boot_drive & 0x80) {
		if (boot_partition_start == 0xffffffff) {
			char *p;
			dprintf("Probing for boot partition start.\n");
			for (p=(char *)0x7c00;p<(char *)0x7e00;p++) {
				if (*(uint32 *)p == 0x05666066) {
					boot_partition_start = *(uint32 *)(p + 4);
					break;
				}
			}
			if (p == (char *)0x7e00)
				panic("boot partition start not found\n");
			dprintf("Boot partition found at %lx\n", boot_partition_start);
		}

		for (v=bootable_volumes;v;v=v->next)
			if (	(v->biosid == boot_drive) &&
					(v->pi.offset / 0x200 == boot_partition_start)) {
				boot_order = BOOT_ORDER_DISK;
				boot_volume = *v;
				break;
			}
	} else if (calledfromdos) {
#if 0
		if (argc == 2) {
			char filename[261], bpb[53], device[32];
			int32 length;
			if (	(get_canonical_name(argv[1], filename) == B_OK) &&
					(filename[0] >= 'C') &&
					((length = get_bpb(filename[0], bpb)) > 0)) {
				struct match_bpb_params p;
				p.len = length;
				p.bpb = bpb;
				p.path = device;
				recurse("/dev/disk", match_bpb, &p);

				if (!device[0])
					panic("Unable to find drive for zbeos %s\n", filename);
				dprintf("%s is on partition %s\n", filename, device);
			}
		}
#endif
		;
	} else {
		char devname[32];

		dprintf("Checking for boot image on floppy\n");

		sprintf(devname, "/dev/disk/0x%2.2x/raw", (int)boot_drive);
		if (mount("ffs", "/ffs", devname, 0, NULL, 0) == B_OK) {
			unmount("/ffs");
			boot_order = BOOT_ORDER_CD_DISK;
		}
	}

	/* If there is only one bootable volume, use it as the default */
	if (	(boot_order == BOOT_ORDER_NONE) &&
			(boot_volume.boot_device[0] == 0) &&
			bootable_volumes &&
			(bootable_volumes->next == NULL)) {
		boot_order = BOOT_ORDER_DISK;
		boot_volume = *bootable_volumes;
	}

	/* If booted from DOS and there is exactly one bootable disk image on a 
	 * FAT volume, boot from there */
	if (	(boot_order == BOOT_ORDER_NONE) &&
			calledfromdos &&
			bootable_volumes) {
		struct bootable_volume_info *vol = NULL;
		for (v=bootable_volumes;v;v=v->next) {
			if (!strcmp(v->boot_fs, "dos")) {
				if (vol) {
					vol = NULL;
					break;
				}
				vol = v;
			}
		}
		if (vol) {
			boot_order = BOOT_ORDER_DISK;
			boot_volume = *vol;
		}
	}

	if (boot_menu_enabled || (boot_order == BOOT_ORDER_NONE)) {
		bootmenu(&boot_order, &boot_volume);

		if (!flag_consoledebug && !fast_boot_enabled && !curscreen.depth) {
			set_video_mode(0x12);
			platform_notify_video_mode(640, 480, 4, 640 / 8, (void *)0xa0000);
			platform_splash_screen(FALSE);
		}
	}

	if (	(boot_order == BOOT_ORDER_CD_DISK) ||
			(boot_order == BOOT_ORDER_CD_ONLY)) {
		sprintf(boot_volume.boot_device, "/dev/disk/0x%2.2x/raw", (int)boot_drive);
		strcpy(boot_volume.kernel, "kernel_intel");
	}

#ifdef MEMORY_TOOLS
	{
		uint32 limit = 0;
		int i;
		
		if(flag_limit24) limit = 24 * 1024 * 1024;
		if(flag_limit16) limit = 16 * 1024 * 1024;
			
		if(limit){
			uint32 stack;
			
			for(i=0;i<MEMMAP_ENTRIES*2;i+=2){
				if(memmap[i] > limit){
					memmap[i] = 0;
					memmap[i+1] = 0;
				} else if((memmap[i] + memmap[i+1]) > limit){
					memmap[i+1] = limit - memmap[i];
				}
			}	
			
			stack = determine_stack_address();
			mt_volume = &boot_volume;
			mt_boot_order = boot_order;
			set_stack_and_jump((void *)stack, main3);
		}	
	}
#endif
				
	boot(&boot_volume, boot_order);

	platform_enter_console_mode();
	dprintf("unable to load kernel\n");
	platform_get_key();

	disable_a20();
}

uint32
determine_stack_address()
{
	uint32	b, s;
	uint32	stack;
	uint32	minsize;
	uint32	memsize;
	int		i;

	memsize = 0;
	for(i=0; i<MEMMAP_ENTRIES; i++)
		if (memmap[i*2 + 0] + memmap[i*2 + 1] > memsize)
			memsize = memmap[i*2 + 0] + memmap[i*2 + 1];

	memsize = (memsize + 0x000fffff) & ~0x000fffff;

	stack = 0;
	minsize = BOOT_STK_SIZE * B_MAX_CPU_COUNT + VM_INIT_MEM_USAGE(memsize);

	for(i=0; i<MEMMAP_ENTRIES; i++) {
		b = RNDINTUP(memmap[i*2 + 0]);
		s = RNDINTDWN(memmap[i*2 + 0] + memmap[i*2 + 1]) - RNDINTUP(memmap[i*2 + 0]);

		if (b > BOOT_STK_TOP)
			continue;
		if (b+s > BOOT_STK_TOP)
			s = BOOT_STK_TOP - b;
		if ((b + s > stack) && (s >= minsize))
			stack = b + s;
	}
	stack -= 0x10;
	return stack;
}

void main1(uchar *_loadaddr, uchar *_scratch, uint32 _boot_partition_start,
		uint32 _boot_drive, uint32 _calledfromdos)
{
	extern double time_base_to_usec;
	extern char *brkval, _end[];
	uint32		stack;
	uint32		memsize, state;

	loadaddr = _loadaddr;
	scratch = _scratch;
	*LOMEM_BASE_SEG_ADDR = (uint16)(((uint32)(scratch + (64+4)*1024)) >> 4); 
	boot_partition_start = _boot_partition_start;
	boot_drive = _boot_drive;
	calledfromdos = _calledfromdos;

	state = disable_interrupts();
	calculate_cpu_clock();
	init_timing();
	scc_init();
	restore_interrupts(state);

	/* Originally this checked _PRODUCT_IAD, which sucked. The contained comment
	 * is all I had to go on. I checked the check_boot_keys() function, and the
	 * only reason I could see where the _PRODUCT_IAD build was different was
	 * because of support for the release_mode extensions. As such, I'm using
	 * the _SUPPORTS_RELEASE_MODE flag here as well. (steven 9-11-00)
	*/
#if _SUPPORTS_RELEASE_MODE
	/*
	 * to early for calling check_boot_keys() that needs to call read_disk()
	 */
#else
	check_boot_keys();
#endif

	dprintf("Welcome to this world\n");
	dprintf("CPU speed is %dMHz\n", (int)(1.0/time_base_to_usec + 0.5));

	total_memsize = memsize = construct_memory_map(memmap, MEMMAP_ENTRIES);
	dprintf("%ld MB of memory detected\n", memsize / 1024 / 1024);

	if (enable_a20() != B_OK)
		panic("Could not enable A20 gate");
	brkval = _end + 0x1000;

	/* Can't allocate memory until after this point since not having the
	 * A20 gate enabled can cause funny problems */

	//stack = determine_stack_address();
	// XXXdbg -- throw the stack at 12 megs.  It's only temporary
	//           (until we jump into the kernel).  NOTE: this 
	//           could be a problem if the kernel + drivers ever
	//           exceeds 5 megs of space (kernel is loaded at the
	//           1 meg location in memory).
	//
	//           Also note that bt_ffs.c needs a bunch of space for
	//           the data it reads from the floppy and the decompressed
	//           version of that data (about 4 megs total).  If you 
	//           change this value you need to be aware of what's going
	//           over there.
	stack = 12*1024*1024;
	
	set_stack_and_jump((void *)stack, main2);
}

