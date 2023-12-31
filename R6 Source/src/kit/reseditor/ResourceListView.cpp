			put_modules();
		}

		clear_reserved_irqs();
	}

err:
	RELEASE_LOCK;

	DPRINTF(4, ("init_module exit code %lx\n", error));
	
	return error;
}

static
void uninit_module(void)
{
	DPRINTF(4, ("uninit_module\n"));

	ACQUIRE_LOCK;

	DPRINTF(4, ("uninit_module acquired lock\n"));

	if (--module_users == 0) {
		DPRINTF(4, ("uninit_module needs to do real work\n"));
		save_resource_info();
		destroy_config_manager_lock();
		put_modules();
	}

	RELEASE_LOCK;

	DPRINTF(4, ("uninit_module complete\n"));
}

static status_t
cmfdm_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			DPRINTF(1, ("config_manager driver %s %s init()\n", __DATE__, __TIME__));

			#ifdef DEBUG
				load_driver_symbols("config_manager");
			#endif
			
			return init_module();

	case B_MODULE_UNINIT:
			DPRINTF(1, ("config_manager driver: uninit()\n"));
			uninit_module();
			return B_OK;

	default:
			return B_ERROR;
	}
}

static
status_t cmfbm_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			DPRINTF(1, ("config_manager_for_bus: init()\n"));
			return B_OK;

		case B_MODULE_UNINIT:
			DPRINTF(1, ("config_manager_for_bus: uninit()\n"));
			return B_OK;

		default:
			return B_ERROR;
	}
}

static config_manager_for_driver_module_info driver_module = {
	{ B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, 0, &cmfdm_std_ops },
	&cmfdm_get_next_device_info,
	&cmfdm_get_device_info_for,
	&cmfdm_get_size_of_current_configuration_for,
	&cmfdm_get_current_configuration_for,
	&cmfdm_get_size_of_possible_configurations_for,
	&cmfdm_get_possible_configurations_for,

	&count_resource_descriptors_of_type,
	&get_nth_resource_descriptor_of_type
};

static config_manager_for_bus_module_info bus_module = {
	{ B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME, 0, &cmfbm_std_ops },
	&cmfbm_assign_configuration,
	&cmfbm_unassign_configuration,
	
	&count_resource_descriptors_of_type,
	&get_nth_resource_descriptor_of_type,
	&remove_nth_resource_descriptor_of_type,
	
	&new_device_configuration_container,
	&delete_device_configuration_container,
	&add_to_device_configuration_container,

	&add_to_possible_device_configurations
};

_EXPORT module_info	*modules[] = {
	(module_info *)&driver_module,
	(module_info *)&bus_module,
	NULL
};

#ifdef USER

static
status_t load_device(const char *fname, struct device_info_node **head)
{
	status_t size;
	struct device_info_node *c, *p;
	int fd;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		printf("error opening %s\n", fname);
		return fd;
	}

	c = malloc(sizeof(struct device_info_node));
	if (!c) goto err;
	if (read(fd, c, sizeof(struct device_info_node)) <
			sizeof(struct device_info_node)) goto err1;
	if (read(fd, &size, sizeof(int)) < sizeof(int)) goto err1;
	if (!(c->possible = malloc(size))) goto err1;
	if (read(fd, c->possible, size) < size) goto err2;
	c->assigned = malloc(sizeof(struct device_configuration_container));
	if (!(c->assigned)) goto err2;
	if (read(fd, c->assigned, sizeof(struct device_configuration_container)) <
			sizeof(struct device_configuration_container)) goto err3;
	size = sizeof(struct device_configuration) +
			c->assigned->num_allocated * sizeof(resource_descriptor);
	if ((c->assigned->c = malloc(size)) == NULL) goto err3;
	if (read(fd, c->assigned->c, size) < size) goto err4;

	p = *head;
#if 1
	if (p) {
		while (p->next) p = p->next;
		p->next = c; c->prev = p; c->next = NULL;
	} else {
		c->next = c->prev = NULL;
		*head = c;
	}
#else
	c->prev = NULL; c->next = p; if (p) p->prev = c;
	*head = c;
#endif
	close(fd);

	return B_OK;

err4:
	free(c->assigned->c);
err3:
	free(c->assigned);
err2:
	free(c->possible);
err1:
	free(c);
err:
	close(fd);

	printf("error opening file %s\n", fname);

	return EINVAL;
}

static
void load_devices(struct device_info_node **head, const char *name,
		int start, int end, int delta)
{
	int i;
	char fname[128];

	for (i=start;i<=end;i+=delta) {
		sprintf(fname, "/boot/home/devices/device.%s.%x", name, i);
		load_device(fname, head);
	}
}

int main()
{
	status_t err;
	struct device_info_node *c, *device_info_list = NULL;

	init_config_manager_settings();

	clear_allocations();

//	load_devices(&device_info_list, "pci", 1, 8, 1);
	load_devices(&device_info_list, "pci", 1, 7, 1); // no ethernet card

	load_devices(&device_info_list, "onboard", 0x10000, 0x10f00, 0x100);

//	load_devices(&device_info_list, "crystal", 0x10100, 0x10103, 1);
	load_devices(&device_info_list, "awe64", 0x10400, 0x10402, 1);
//	load_devices(&device_info_list, "mako", 0x10600, 0x10605, 1);

	load_devices(&device_info_list, "net", 0x10500, 0x10500, 1);

//	load_devices(&device_info_list, "smartone", 0x10200, 0x10200, 1);
	load_devices(&device_info_list, "creative56k", 0x10300, 0x10300, 1);

//	load_devices(&device_info_list, "jaudio", 1, 1, 1);
//	load_devices(&device_info_list, "jserial", 1, 1, 1);
//	load_devices(&device_info_list, "jnet", 1, 1, 1);

	printf("Calling assign configurations to devices.\n");
	err = assign_configurations_to_devices(&device_info_list);

	c = device_info_list;
	printf("Allocated configurations:\n");
	while (c) {
		printf("bus %x, id %x\n", c->bus_index, c->id);
		if (c->assigned && c->assigned->c && c->possible)
			dump_device_info(&c->info, c->assigned->c, c->possible);
		else
			printf("c->magic = %x, c->assigned = %x, c->possible = %x\n",\
					c->magic, c->assigned, c->possible);
		c = c->next;
	}

	dump_allocated_resources();

	return 0;
}

status_t get_module(const char *path, module_info **vec) { return B_ERROR; }
status_t put_module(const char *path) { return B_ERROR; }

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #include <drivers/KernelExport.h>

#include <string.h>

#include "config_manager.h"
#include "config_manager_p.h"
#include "debug.h"

int _assert_(char *a, int b, char *c)
{
        dprintf("tripped assertion in %s/%d (%s)\n", a, b, c);
#ifdef USER
        debugger("tripped assertion");
#else
        kernel_debugger("tripped assertion");
#endif
        return 0;
}

/************************************************************/

static
status_t count_resource_descriptors_of_type(
	const struct device_configuration *c, resource_type type)
{
	uint32 i, count = 0;
	
	if (!c) return EINVAL;
	
	for (i=0;i<c->num_resources;i++)
		if (c->resources[i].type == type)
			count++;
	return count;
}

static
status_t get_nth_resource_descriptor_of_type(
	const struct device_configuration *c, uint32 n, resource_type type,
	resource_descriptor *d, uint32 size)
{
	uint32 i;

	if (!d || !d) return EINVAL;
	
	for (i=0;i<c->num_resources;i++)
		if ((c->resources[i].type == type) && (n-- == 0)) {
			if (size > sizeof(resource_descriptor))
				size = sizeof(resource_descriptor);
			memcpy(d, c->resources + i, size);
			return B_OK;
		}

	return ENOENT;
}

/************************************************************/

static
void dump_mask(uint32 mask)
{
	bool first = true;
	int i = 0;
	dprintf("[");
	if (!mask)
		dprintf("none");
	for (;mask;mask>>=1,i++)
		if (mask & 1) {
			dprintf("%s%d", first ? "" : ",", i);
			first = false;
		}
	dprintf("]");
}

void dump_device_configuration(const struct device_configuration *c)
{
	int i, num;
	resource_descriptor r;
	
	num = count_resource_descriptors_of_type(c, B_IRQ_RESOURCE);
	if (num) {
		dprintf("irq%s ", (num == 1) ? "" : "s");
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_IRQ_RESOURCE,
					&r, sizeof(resource_descriptor));
			dump_mask(r.d.m.mask);
		}
		dprintf(" ");
	}

	num = count_resource_descriptors_of_type(c, B_DMA_RESOURCE);
	if (num) {
		dprintf("dma%s ", (num == 1) ? "" : "s");
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_DMA_RESOURCE,
					&r, sizeof(resource_descriptor));
			dump_mask(r.d.m.mask);
		}
		dprintf(" ");
	}

	num = count_resource_descriptors_of_type(c, B_IO_PORT_RESOURCE);
	if (num) {
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_IO_PORT_RESOURCE,
					&r, sizeof(resource_descriptor));
			dprintf("\n  io range:  min %lx max %lx align %lx len %lx",
					r.d.r.minbase, r.d.r.maxbase,
					r.d.r.basealign, r.d.r.len);
		}
	}

	num = count_resource_descriptors_of_type(c, B_MEMORY_RESOURCE);
	if (num) {
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_MEMORY_RESOURCE,
					&r, sizeof(resource_descriptor));
			dprintf("\n  mem range: min %lx max %lx align %lx len %lx",
					r.d.r.minbase, r.d.r.maxbase,
					r.d.r.basealign, r.d.r.len);
		}
	}
	dprintf("\n");
}

const char *base_desc[] = {
	"Legacy",
	"Mass Storage Controller",
	"NIC",
	"Display Controller",
	"Multimedia Device",
	"Memory Controller",
	"Bridge Device",
	"Communication Device",
	"Generic System Peripheral",
	"Input Device",
	"Docking Station",
	"CPU",
	"Serial Bus Controller"
};

struct subtype_descriptions {
	uchar base, subtype;
	char *name;
} subtype_desc[] = {
	{ 0, 0, "non-VGA" },
	{ 0, 0, "VGA" },
	{ 1, 0, "SCSI" },
	{ 1, 1, "IDE" },
	{ 1, 2, "Floppy" },
	{ 1, 3, "IPI" },
	{ 1, 4, "RAID" },
	{ 2, 0, "Ethernet" },
	{ 2, 1, "Token Ring" },
	{ 2, 2, "FDDI" },
	{ 2, 3, "ATM" },
	{ 3, 0, "VGA/8514" },
	{ 3, 1, "XGA" },
	{ 4, 0, "Video" },
	{ 4, 1, "Audio" },
	{ 5, 0, "RAM" },
	{ 5, 1, "Flash" },
	{ 6, 0, "Host" },
	{ 6, 1, "ISA" },
	{ 6, 2, "EISA" },
	{ 6, 3, "MCA" },
	{ 6, 4, "PCI-PCI" },
	{ 6, 5, "PCMCIA" },
	{ 6, 6, "NuBus" },
	{ 6, 7, "CardBus" },
	{ 7, 0, "Serial" },
	{ 7, 1, "Parallel" },
	{ 8, 0, "PIC" },
	{ 8, 1, "DMA" },
	{ 8, 2, "Timer" },
	{ 8, 3, "RTC" },
	{ 9, 0, "Keyboard" },
	{ 9, 1, "Digitizer" },
	{ 9, 2, "Mouse" },
	{10, 0, "Generic" },
	{11, 0, "386" },
	{11, 1, "486" },
	{11, 2, "Pentium" },
	{11,16, "Alpha" },
	{11,32, "PowerPC" },
	{11,48, "Coprocessor" },
	{12, 0, "IEEE 1394" },
	{12, 1, "ACCESS" },
	{12, 2, "SSA" },
	{12, 3, "USB" },
	{12, 4, "Fibre Channel" },
	{255,255, NULL }
};

void dump_device_info(const struct device_info *c, 
		const struct device_configuration *current,
		const struct possible_device_configurations *possible)
{
	int i;
	const struct device_configuration *C;

	switch (c->bus) {
		case B_ISA_BUS : dprintf("ISA"); break;
		case B_PCI_BUS : dprintf("PCI"); break;
		default : dprintf("unknown"); break;
	}
	dprintf(" bus: ");
	dprintf((c->devtype.base < 13) ? base_desc[c->devtype.base] : "Unknown");
	if (c->devtype.subtype == 0x80)
		dprintf(" (Other)");
	else {
		struct subtype_descriptions *s = subtype_desc;
		
		while (s->name) {
			if ((c->devtype.base == s->base) && (c->devtype.subtype == s->subtype))
				break;
			s++;
		}
		dprintf(" (%s)", (s->name) ? s->name : "Unknown");
	}
	dprintf(" [%x|%x|%x]\n", c->devtype.base, c->devtype.subtype, c->devtype.interface);

	if (!(c->flags & B_DEVICE_INFO_ENABLED))
		dprintf("Device is disabled\n");

	if (c->flags & B_DEVICE_INFO_CONFIGURED) {
		if (c->config_status == B_OK) {
			dprintf("Current configuration: ");
			dump_device_configuration(current);
		} else {
			dprintf("Current configuration error.\n");
		}
	} else {
		dprintf("Currently unconfigured.\n");
	}

	dprintf("%lx configurations\n", possible->num_possible);
	C = possible->possible + 0;
	for (i=0;i<possible->num_possible;i++) {
		dprintf("\nPossible configuration #%d: ", i);
		dump_device_configuration(C);
		NEXT_POSSIBLE(C);
	}

	dprintf("\n\n");
}

/************************************************************/
                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #ifndef _CONFIG_MANAGER_DEBUG_H_
#define _CONFIG_MANAGER_DEBUG_H_

#define ASSERT(c) (!(c) ? _assert_(__FILE__,__LINE__,#c) : 0)

#define DPRINTF(a,b) do { if (verbosity > a) dprintf b; } while (0)

int _assert_(char *a, int b, char *c);
void dump_device_configuration(const struct device_configuration *c);
void dump_device_info(const struct device_info *c, 
		const struct device_configuration *current,
		const struct possible_device_configurations *possible);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        #include <drivers/KernelExport.h>
#include <support/SupportDefs.h>

#include <malloc.h>

#include "range.h"

/* silence warnings */
struct device_info;
struct device_configuration;
struct possible_device_configurations;
#include "debug.h"

struct range {
#ifdef DEBUG
	uint32 magic;
#define RANGE_MAGIC 'egnr'
#endif
	uint32 start;
	uint32 end;

	struct range *next;
};

void dump_ranges(struct range *r)
{
	uint32 i;
	for (i=0;r;r=r->next,i++)
		dprintf("%s0x%lx -> 0x%lx", (i ? ", " : ""), r->start, r->end);
	dprintf("\n");
}

bool is_range_unassigned(struct range *r, uint32 start, uint32 end)
{
	if (end < start)
		return false;

	while (r) {
#ifdef DEBUG
		ASSERT(r->magic == RANGE_MAGIC);
#endif
		if (!((end < r->start) || (start > r->end)))
			return false;
		r = r->next;
	}

	return true;
}

status_t add_range(struct range **head, uint32 start, uint32 end)
{
	struct range *r = malloc(sizeof(struct range));

	if (!r) {
		dprintf("add_range: OUT OF CORE\n");
		return ENOMEM;
	}

#ifdef DEBUG
	r->magic = RANGE_MAGIC;
#endif

	r->start = start;
	r->end = end;
	r->next = *head;

	*head = r;

	return B_OK;
}

status_t remove_range(struct range **head, uint32 start, uint32 end)
{
	struct range *c, *p;

	c = p = *head;

	while (c) {
#ifdef DEBUG
		ASSERT(c->magic == RANGE_MAGIC);
#endif
		if ((c->start == start) && (c->end == end)) {
			if (c == p)
				*head = c->next;
			else
				p->next = c->next;
#ifdef DEBUG
			c->magic = ~c->magic;
#endif
			free(c);
			return B_OK;
		}
		p = c;
		c = c->next;
	}

	dprintf("Can't find range (%lx/%lx)!\n", start, end);

	return ENOENT;
}

status_t clear_ranges(struct range **head)
{
	struct range *c = *head, *n;

	while (c) {
#ifdef DEBUG
		ASSERT(c->magic == RANGE_MAGIC);
		c->magic = ~RANGE_MAGIC;
#endif
		n = c->next;
		free(c);
		c = n;
	}

	*head = NULL;

	return B_OK;
}

static
uint32 count_ranges(struct range *r)
{
	uint32 c = 0;
	while (r) {
		r = r->next;
		c++;
	}
	return c;
}

status_t ranges_save_area_size(struct range *head)
{
	return sizeof(uint32) + count_ranges(head) * 2 * sizeof(uint32);
}

status_t save_ranges(struct range *r, uchar **buffer)
{
	uint32 *p = (uint32 *)*buffer;
	
	*(p++) = count_ranges(r);
	while (r) {
		*(p++) = r->start;
		*(p++) = r->end;
		r = r->next;
	}
	
	*buffer = (uchar *)p;
	
	return B_OK;
}

status_t restore_ranges(struct range **head, uchar **buffer)
{
	status_t error;
	uint32 i, *p;
	
	if (!head) return EINVAL;

	p = (uint32 *)(*buffer);
	i = *(p++);
	while (i--) {
		if ((error = add_range(head, p[0], p[1])) < B_OK) {
			clear_ranges(head);
			return error;
		}
		p += 2;
	}

	*buffer = (uchar *)p;
	
	return B_OK;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                   #ifndef _RANGE_H_
#define _RANGE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct range;

void dump_ranges(struct range *r);
bool is_range_unassigned(struct range *r, uint32 start, uint32 end);
status_t add_range(struct range **head, uint32 start, uint32 end);
status_t remove_range(struct range **head, uint32 start, uint32 end);
status_t clear_ranges(struct range **head);

status_t ranges_save_area_size(struct range *head);
status_t save_ranges(struct range *r, uchar **buffer);
status_t restore_ranges(struct range **head, uchar **buffer);

#ifdef __cplusplus
}
#endif

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                        ���i                 ��������                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               ������������������������  ...Makefilei2o.c0       �     �     �     �                                                                                                                                                   RTSC     xml       �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                bar
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            -----------------------------------------------------------------------
[Installation notes for VM/CMS port of UNZIP 5.32 and ZIP 2.2]
Additional notes from Ian E. Gorman (4/98):

I have not fully tested ZIP and UNZIP in VM (for example, I have not
tried all the options), so ZIP 2.2 for VM and UNZIP 5.32 for VM
should be regarded as beta versions.  Try them out before you decide
that you can depend on them.

Most of the work for the VM port has been done by others, but I have
made some changes and compiled on a more recent version of VM/CMS.  It
is possible that I have introduced new problems or undone some of the
solutions found by previous authors.


Installation
============

The executables are stored in CMS "PACK" format instead of being
stored in ZIP archives.  This format takes a little longer to
download, but installation is slightly simpler.


Installing UNZIP
----------------

The UNZIP executable is supplied as the binary file UNZIP.MOD, in the
CMS "PACK" format.

You must get the UNZIP.MOD file on to your system as a binary file in
fixed recording mode, block size 1024.

If you are using FTP in CMS, you can get the file in the correct
format with the LOCSITE and BINARY commands.  Assuming that the UNZIP
executable is stored on the remote site as unz532vm.mod, you could
issue the following commands

    FTP where.ever.com
    <enter user id and password and other stuff>
    BINARY
    LOCSITE FIX 1024
    GET unz532vm.mod
    QUIT

If you are using a 3270 terminal session to upload from a PC, you can
upload the file in the correct format with the SEND command:

    SEND unz532vm.mod A: unz532vm mod a1 (RECFM F LRECL 1024

If your 3270 emulator does not have the SEND command, but is a GUI
application, it may allow you to upload from the menu bar.  If so,
set your options to binary, VM/CMS, fixed record length, and a length
of 1024 before you upload.

When you get the PACKed file on your CMS minidisk, you convert it to
an executable module by using the COPY command with the UNPACK option:

    COPY unz532vm mod a unzip module a1 (UNPACK OLDDATE REPLACE

You can omit the OLDDATE and REPLACE options if you want to.


Installing ZIP
--------------

The ZIP executable is supplied as the binary file ZIP.MOD, in the CMS
"PACK" format.

You must get the ZIP.MOD file on to your system as a binary file in
fixed recording mode, block size 1024.  Assuming that the file is
stored as zip22vm.mod, you can get the file the same way you got the
UNZIP.MOD file:

    Using FTP:

        FTP where.ever.com
        <enter user id and password and other stuff>
        BINARY
        LOCSITE FIX 1024
        GET zip22vm.mod
        QUIT

    Using 3270 file transfer:

        SEND unz532vm.mod A: unz532vm mod a1 (RECFM F LRECL 1024

When you get the PACKed file on your CMS minidisk, you convert it to
an executable module by using the COPY command with the UNPACK option:

    COPY zip22vm mod a zip module a1 (UNPACK OLDDATE REPLACE


Installing Documentation
------------------------

Once you have UNZIP running, you can use it to extract documentation
from ZIP archives.

You can transfer the ZIP archives to VMV/CMS as a binary file with any
record length.  A record length of 1 will work fine:

    via FTP:

        FTP where.ever.com
        <enter user id and password and other stuff>
        BINARY
        LOCSITE FIX 1
        GET zip22vm.zip zipdoc.zip
        GET unz532vm.zip unzipdoc.zip
        QUIT

    via 3270 session:

        SEND zip22vm.zip A: unzipdoc zip a1 (RECFM F LRECL 1
        SEND unz532vm.zip A: zipdoc zip a1 (RECFM F LRECL 1

Once you have the ZIP archives, extract the documentation to the minidisk of
your choice by using the -d option:

    unzip -a -d A2 unzipdoc.zip
    unzip -a -d A2 zipdoc.zip

The "-a" option is required because the documents are archived as
ASCII text files, but they must be converted to EBCDIC to read them
in VM/CMS.


Notes
=====

Different EBCDIC Character Sets
-------------------------------

The documentation may look strange on your system, because UNZIP
translates from ASCII to "Open Systems" EBCDIC (IBM-1047).  Unless
you are a software developer, you are probably using a different
kind of EBCDIC (like US EBCDIC, IBM-037).  This causes some character
codes to display as different characters.  For example, the character
codes that display as square brackets in IBM-1047 will display as
e-acute and a-grave in IBM-037.

You can use the IBM ICONV utility to translate documents from one
character set to another:

    ICONV old doc a new doc a1 (FROMCODE IBM-1047 TOCODE IBM-037


IND$FILE protocol
-----------------

This is the method by which files are transferred via 3270 sessions
between PC and mainframe.

If you know how to transfer files via 3270 session between PC and
mainframe, you know as much as you need to know about IND$FILE.

If your mainframe has IND$FILE, and your 3270 emulator does file
transfers, you can use your emulator to transfer files between PC
and mainframe.
                                                                                                               TZif                             p      ��h|�I��@���40��t �8�0���0��ܹY ��0ޛ� �ݚ0�T3 �����^ ��d0���Q,0��� �
Ұ������0��}�Ɏ0xנ�5� 3Ϡ!�i0"Ƞ#X�#�p %7�%�� 0�y01M�7�ư8�� 9֨�:�g ;��0<xI =��0>X+ ?�0@AG�A_k0B!)�C?M0D�E/0E���GK�G�ϠH�-�I���J��K�� L��Mi� N�ӰOI� Pp�0Q)t RP�0S	V T0�0T�8 V�0V�T�W�x0X�6�Y�Z0Z��[�v�\q��]�X�^Qܠ_y:�`:� aY�b� c8��c�� e�eڟ g�0g�� h��0i�c j��0k��l��0mca�n��0oCC�pj��q#%�rJ��s�t*e�t�$ v
G�v� w�)�x�� y��z�� {�(0|k� }�
0~K� r�0��ބ  �������� 	LMT BRST BRT                                                                                                                                                                                                                                                                                                                                                                                                      ,G�s�Ƅ��̐�0��	iĹ�y<��x�w8Di�um	�(M�ݯ��L�?��bCW�'PR�~t��(�d�Q�|w��g�����Od����_��u���;���(�$�6=��؇�|	��d��s�`4-,v�9�
N;��2�/h�=�9��L�5Ȳ{��e�?:��&�~n��D5��3�mY'/l��ɀ9c��\8@����c��H��#>������~p��X-!'9(�HyG���ڦ���@E��m�������{��R-����#9Q$��u��a������s���VL�|��F�5hvQ���z�|Q���Ȏ��ܩf�s�r����Ҧ��P��]E��c�V�W���m��2��V�����xi���KO��/������oIbaA�����YAB ����WFx��3�	 �?�b�gL٬$�T�âNX �l-��V���V��=���U�a�,�;!j{�l�/g�ZˌFA�X%V��4&�d����M���c۞��<8M���P�K\@�����"$H�+lUq�^,W�!כ����	�u�0��B`�4�*��k3g���ڧ
����������b�Nl�������'���_ɘ'�-(�va� �!�a���C�F��+�CdT'bc��	�� YQ����U����+�$�f����MR�h�Q`�_k�QD=���P��cO�c¼�8��r�}|��?��1<W�ή�c�]3�Z�T�>������6!�f"�NM����'}M䇗10B���������w4��g��`���aYt���SUDœ�
����"�}��b9��G�޽ܸ6���<i�Q��Y��A� һ��g �V�g9G���/���!���-�N80U��I��+g��4|D�r��?؋15z�1ij�rc�	o���+�z�E��X��GC��cj�Ն�ҟ4� ��<i��N������'���o������X�qD�� u
<u���ܑ�9g����:F�w\�����9uy�f�kKH��T���l\c��{���ɶ{���k9寜0���6��]y�Q'|y�����f��<yX��R����^¦/������K��Dn	Z"��%�N0�Y!=G!�\0^���&�t�GӇ���H�-2V����w.������$�g��s-�_�AN�\���p��ٌ�I�{���]�j{�i�C�v+��|oD�3:��5g��:a�����O�C����F�E��%�/`KV���`�w`�0�� ������9��a����@ �*Ы�a(�H����sI�XT��McB�d�W\I:\R��Tʃ�%w��A�� "/pY(�O	�����Ľ���5��{>8 ���;���j�b��u��u����uu�+�ov{R���M~u�R�N	�T{���?*tX
����ǜ�Q���	��?*� &#�R�v-��k��Y��q���R�M
����zi�ce��뙇���"_�1���'f`�*_	����D1���ԛ�*űD��<k:�!0w=n2���_���l�����sG���F�~�)�	s���=ɩ�^r��\�Z���T�^k�jB�V1Y�nim�P�5����'>/c_Yݚx�!���m_��a�b׭�Z��d�q;�ÇC㿻�dB�l<_�H0�Зǯ����a�ϔ|�4�kf`��lA�\&*���s�h;ֹ����;�n�d�Cƞ�e�_s����*Clh��Q��p���mOrw�G��}��a=��җg_ĶVq��@,O>e���#f8�;7�%�h��hP!Q<�y�)|Y�Mfy�Y<*��X��� ��p��Y���j-^D��Ʒ��͚XͶl�V�B`�KOeP-ЉV(*��}o��|{������Sr��W�T:«�g}$d�>��0G>��H�<����������ɘCC��O���fȈ���N�eX�l������,���/�ՙ��~K2B�bϕ ���݅��11�Ks|�;n�O����y؃�Mど���w9��x̠��:�d8B�@rH�Xz; s��m`'hσ$KT_���:��J�.���wl�t*h�'��}f܃[�� �e�������%Ablj��V����ΝL MD�VJ�1������'�fZ9���
�lv��{P�]��!��^ma0a�+^�`�a��GU}O��j��CI?۔P&ba��Kx�11���<`9t��H,�2�̿�U �=���8}cU5;�*l�M^�'�������F��F�*"�Bbj#
��0��ٛ��FN?V���'���|ijO�K��)ܵ������ �5{�f���+bp��Yn҇�jWhz���ߪB�V��w��H�l�H��3���1x$�\���!�)k�i?1ÎWi ��b8/���<�{;��*�T�^����0��$� 9:�H}<�?��ɐ�  �o�y�Gu�����-�,e��w	<���"z�*�ru��pŠ��1��G�zP��J_M���f�����;¨u�z���a�5��=��Ux#<�ݘ$cÓ[^���r�h:sO4m�mcq�"�ť�����E3�sN����eHc�r;5������� y�@�#>��^��cn/h��Ї��e5mp�.Բ�X^�6�N�I�4mt�#/�����<NM�?��o��^$ڲ���m���#����`l�.6H߯���liD�Z"P�%�9o��Uͥ(�)D)_vk ǥ�d���f0�����p�K%"y �'�졳~Kfz��KI 4���5}ըY��K��P,
7b�W[_�cЏ`���W�c�����H[տ\�q���?�9��4d�ޒ:
�?�8n�sG]5�o�j�L��G2��,�U���h;�[�Zu\���Rl�Vz���~�f�@�����q�꓈�����IH܅�٨�������,鷃V+I�qS����`�����3z��c�|�?���0�A�W�x�^"�s1���k�:b(��o���%�,C�Ü���02����8�MQ�({�T9V�|�D��4�v)`)�]��1�<����h"�⏎Ɓ#}.�f:=��K?v�$��ԉW9�O������
W���D�?[<G�E���~��Aliynx����z��s��-͞�����]1�C��"���,���Z��?��β�^��w9Y��pZ����zA͐�;0�)����hTIO��I��ş��<j�"4�-M�3COxk��_}NP���,�3��]t�8����XX	�bB�r�X��+ίh�Zĉ���o���T:�]�C����W�q�o�q�@��s
�K��]x�&��ü>�U֛�qO�����5���"��	�x��@q.`��OM�r9ey�8���{"�bS����t�&�	&\���[s)���g��6���w�?���.fa³UͤW�؛�fQQ�ΐN�*��&������n�>����	$Y�u@�3l��ɘw��?l�"wXP��{��,�q�Mi��H{��x�'$�a)���vٿ�/L��d>���r�_uZ\���B�*�!�M������+��X?`���ͻ�ӻI���?�UC���w ]����ѓI�%��7�M���ŵ|�K�/ᑎ�5���(o��)ToU����۵��l�9>#
q� ��pu��x��1����&Q�_��XI�c���c�϶O���ձ8:!�$�ƄSkO,�S�F����~�>E���Z�|�In���Wl��w�$��{�����ʠ�<;��3�U<wD �ʹ��?�g�åY�R�i���0�j���-Z�;+�e	ZB����ft��/�)Y�vM���e��$���#GZ�訳�w�y��)�J���$h���h�����f�o�6|L�N�܂�֯�L�_�:�M^_�D�s�[ў�s�w�Q��|GW'���-��_���|*f��#�X�2���8�C�G&ܓL���7@��B�uI��<�ǅ�If:��h�W�b�n�9����_/�Q���=�n�?��*�U
�O�y~�>��E�7w�f�h)#Ө	�$$�)�3���l��R]�;�*�{���5��*�$�JO |�d��0���[ aԟԭ���Χ�Jϭ?*����ycZ�ٞ�&5@��i���>�^�[�l������{��\{y�E?�.׊��ǿ �?'���IQ��?�{Fw�j�H����c��xl�~i)A��������P�=ڇH|5G��L��Ӱ��Y�<Ӊ75Jʃ��M��Xh��v�DϚ%b�]�S�Rj��_�SvUoS�ʪ���شؔQ@�Y�v-��	���ti�� b�)����_} ��fL��Q���"0"�'
�o��ݮv�X��.�BI�{�빌�a3^����d��s^�S�F�:-�Ø�{uގ~@��@�����Y�]F�'���J�Qޱ�Ek|�yW��	��<z���˺9k��ʳy���.u��v�,�~�On0���F�.����֦0��4��/`�w|Z��n��BHɵC�+�~Ù�Mh`=�*&rF�l�� K���ٹ�:5�I���>�,�;Y` 1<YM�$��#P���Y�d_��'�@/� mM� ��n ����o">�<}<� pi�/�b	}���,�cr��9��|���۞t�?���*)e�?�~�T���o�}��Bv�k��f`5d�9d�	7r��2ߔҗ��s���a�1��c�]8�K9T�v['�.�s��Ņ���D��L.����X�}$W<p,1_�Dn�@�4Ӟ��$�#�ƣ�l�8�M�,.}n\_���	˔�Z��`U�s.�'������n#�L�J�?=UB�~\���'eWћ�R/���+�`F��+`g<ʾ�7*��<܀b<u��Ƴtנ%�����cǵ��E����u���VF�wc,��GU} �$�a���֌�	k�.�E���;pT�k�.�qf�t5��M��R���d��B��3G�@�J�ğ�� �~i�}d����I~��8m��=��2���%K�4>���P3_�!�%��������Gr;��A�ì�����ܜ��̂"D�O�������A���U`��E �Ѝ���Z���)�ִCĸ]~�<�������{��aY�ء�H�O:޻B�G���?��"䎟^I�«p�W��X��[�l��^bd����U-eo�ks��f�)�Eٻ�؁�"�_�;�	��K9a��G���%%��s!���"a��:��N"��Q�#�k�}Y����� �Hjo����fT��+�P��o0��lP���c�t��ޛ�fn�A�p���Y�t��qM��w��. }Xr ����t�A�5�(�o�O#�D�tm{��w%�Z�s!��'���/���t×Ўį�;9��-�(��Uz��9��Y�-x׾���2�l)F(��:���P�a�s��G��Ϥ�����y����X���wK�B� Q��i��JK݆�{�9�qX�Q@u�}��[o�N5�*��D�o}�P\�m�p�|���F0l��]�H�@�=L�
�$0l�b��5�s�y��ZtE?��
]�[.�,|��РNjY^X�K�+� |�pD�]��g�h�|�0fXy*4/�:"��y�����T����f�e}#b�~��p��C�J�sy00g+ʘ���42]�Ë��$�&}M��x�t�&��8w�ć�l.����0(�-�.�k4�+��*�3\��HuFh+��o>%MO^2��k�K�e�<���rNF��Ȥ��b�" b��0ULZ�����;+�Ԏ����Z�;��ﵷ��2��C�]lw)#��l�ܫܺ�]��_#w�-�p�k;�3��+�
ބ���[�`c�j�@~f��v�~]nX�����n��>G�����ˋ�q4����ZOx�Pz_[�Z���=#�h�t	����3>��GV��T�O�3�5�V~�Cε�KQ#�?1A����ULa_QxQ��|�?�<2�c!��32N�S[!ʆ�#1 j���l4wV���Xd����a+6K���~ۧ���(Z��?��Q{{-�|��W�s!�ʁ�?����Z�IA�a�|��hRՅ����ql�ewʪ~*<�`5��P��<3��1E�X7Ǫ:snP�)�ZS��� 0 /)Y�i�=}�A:��9������?�s*��u��}��4��@��@�:����8Y�x٩M��ᰄ���}���ƇS��e�-j�͠����t�u�I����̥��w֢�G�ڪ���Aߖ-�#��&�Z/� y��I�\��K�U���<�*ӫU�ɋ��/��M;����F�z���5��O!�-M�ҺR�J1L�X�n�R�ʩ�F�SG/!w$ګ�*B����{L��*�
�v�Y��s�*�=�@��yju�����w+����0�蜎�i��d�sh��.L�i��#�?a�o�Ԅ^-UZ p#�.ܝ_C!���ʇ��A䉄 |'lQ�����&�aԝ����ŗ9�%��?Q�_��vs���<Cx���(�']}e�,+쳟3a�vWP���T�X��{�[��A�TR� �N�~G�eH�\��+Ay������&�_��wY[��,+�����A:��c�K��1�dy��E)a�	�3G1+����Ւ�P_^��������U`�߂��%�>��q�tg�$�f�u;�L(�����V�`yŇ W\S��������h�5�˳W��4��E6�u[ljX�Eq0 �lY7���[Xٚ*�@�3Ƙ]@Nԁ��8i )[�o��+�5��wޘ��[�`��:�b��3��ݸ��ٔs1��e�t�� B����x�~��E��V�z�<��v6̒���-r3�c�~8Fl�����Oݸ)P�����3�0�� 9�������ưdby�6���P<A[G.[Puѣ��D��8K.s���l���{��գ ��8s��N`��ai�RM�3#=�i\1�W��X��m2Sa_��D��]�Qٚ�ѿ��a_G���؍'����8@�<�����t���C\-
���#5Q?�̠i��}`,�i-fa��} s�3|!�M�0�P�w3ĺ�8a� �fL�L~�,4�=���3��@oA&�t��W������D���G�4����  B���-"�ت:��ڔ�z�ʏ.�:{)l+_��a�9oo�xqYl��Y�5�}&�e�e�?�q�����N�!X�\�����!L����ߺ��eE�i2|ۨf5��5�]p�.
-2\��1'S5�"�錔���AxCtJ�����N�	���I��6YRԌ2`2��f�gG?1�"������jZ�o�Y�I_�Τg@�	K㛄�����o���[5ø�t�����r`6�Y� �8� A��ņ�R�+���_��x�dH��ݝ��ߊ����T��O*�|�b`�y`��ǎĮGG��O`@rs-j��fr���V ��{��
���)O'<�q��3���M�X0 6�e�9a�E�d���B�w� &�Ȣg�$/>&�ʺhh�l���������b!t�S�����Tg������u��P�w���N_��p''6���|����}羟x�#C˜�^�3�vO]4�yC�ؚ���T��L��l�9�(��w⚣M�ʭ<z*,�)}��"(1@��m7��`g����~Fj�J�#r��,���=�-�Hj�'����g�j��=%hQ}�_��L�q� �ں��u�D�d�;N�>-�~���G�i�4���Ĵ�<�4�H����嵡��Pr�«<�9�z���<���O@�v{��c�2���a�]��'/y�C������4�?��A��{.l��_�ys�=���T����\�В~����e)��(Ԇ�[?w���JN�@r��b�ޘ̠�v"s���:.���ti���BZ�C�T����v�#A�<8 ��Hl�]����ӳ�.^�U7����&�o��@9G���!ȫ�<�9�D���v�+�&�.c�0vu�x���0��S���o���> �l������O*s:�征����7�p���f=��'�o,�t�I����J�rJ � >֧�m�)���GΏ�,��8~�ʝ"��X�^4�� [t9���w���ea���K]����ȑ��'�����0��6˸,HT��O�L�~�}���DϼNߊ!��탪{ڍ�ieC�H�Թ����	��O�tvq��=�n��������t,' m=N�T0,�M��)IK��^��ِ��{������mZb�(�5�;9�i��޹�&����I���`=-�C����v�%�N}j�E7q����I�ȞCcnoB�(~콄��8�a��N�B���edM揸�Z��7���߃�	�P����I���<���&���#�������)Ѳ��i=d�ʉ�b�>�4]ߝ�0���L��ņp����!M<hRF_;�O.��>^�i�\�@��ZV���!q���.'��������3��miB��{