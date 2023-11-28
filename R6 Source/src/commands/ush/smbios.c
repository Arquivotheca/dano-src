
#include <OS.h>

#include <stdlib.h>
#include <stdio.h>

extern status_t do_setenv(int argc, char **argv);


#define	_PACKED		__attribute__((packed))

typedef struct {
	char anchor_string[4]; /* 00h Anchor String 4 BYTEs _SM_, specified as four ASCII characters (5F 53 4D 5F).*/
	
	uint8 checksum;	/* 04h Entry Point Structure Checksum BYTE Checksum of the Entry Point Structure (EPS).
		This value, when added to all other bytes in the EPS, will result in the value 00h
		(using 8-bit addition calculations). Values in the EPS are
		summed starting at offset 00h, for Entry Point Length bytes */
	
	uint8 length;	/* 05h Entry Point Length BYTE Length of the Entry Point Structure, starting with the
		Anchor String field, in bytes, currently 1Fh.
		Note: This value was incorrectly stated in v2.1 of this
		specification as 1Eh. Because of this, there might be v2.1
		implementations that use either the 1Eh or 1Fh value, but v2.2 or
		later implementations must use the 1Fh value. */
	
	uint8 version_major;	/* 06h SMBIOS Major Version BYTE Identifies the major version of this specification implemented in
		the table structures, e.g. the value will be 0Ah for revision 10.22
		and 02h for revision 2.1. */
	
	uint8 version_minor;	/*07h SMBIOS Minor Version BYTE Identifies the minor version of this specification implemented in
		the table structures, e.g. the value will be 16h for revision 10.22
		and 01h for revision 2.1. */
	
	uint16 max_struct_size; /* 08h Maximum Structure Size WORD Size of the largest SMBIOS structure, in bytes, and encompasses
		the structure’s formatted area and text strings. This is the value
		returned as StructureSize from the Plug-and-Play Get SMBIOS
		Information function. */
	
	uint8 eps_revision; /* 0Ah Entry Point Revision BYTE Identifies the EPS revision implemented in this structure and
		identifies the formatting of offsets 0Bh to 0Fh, one of:
		00h Entry Point is based on SMBIOS 2.1 definition; formatted area is reserved and set to all 00h.
		01h-FFh Reserved for assignment via this specification */
	
	uint8 formatted_area[5];	/* 0Bh -0Fh Formatted Area 5 BYTEs The value present in the Entry Point Revision field defines the
		interpretation to be placed upon these 5 bytes. */
	
	char intermediate_anchor_string[5];	/* 10h Intermediate anchor string 5 BYTEs _DMI_, specified as five ASCII characters (5F 44 4D 49 5F).
		Note: This field is paragraph-aligned, to allow legacy DMI
		browsers to find this entry point within the SMBIOS Entry Point Structure. */
	
	uint8 ieps_checksum;	/* 15h Intermediate Checksum BYTE Checksum of Intermediate Entry Point Structure (IEPS). This
		value, when added to all other bytes in the IEPS, will result in the
		value 00h (using 8-bit addition calculations). Values in the IEPS
		are summed starting at offset 10h, for 0Fh bytes. */
	
	uint16 struct_table_length;	/* 16h Structure Table Length WORD Total length of SMBIOS Structure Table, pointed to by the
		Structure Table Address, in bytes. */
	
	uint32 struct_table_address;/* 18h Structure Table Address DWORD The 32-bit physical starting address of the read-only SMBIOS
		Structure Table, that can start at any 32-bit address. This area
		contains all of the SMBIOS structures fully packed together.
		These structures can then be parsed to produce exactly the same
		format as that returned from a Get SMBIOS Structure function call. */
	
	uint16 struct_num;	/* 1Ch Number of SMBIOS	Structures WORD Total number of structures present in the SMBIOS Structure
		Table. This is the value returned as NumStructures from the Get SMBIOS Information function. */
	
	uint8 smbios_bsd_revision;	/* 1Eh SMBIOS BCD Revision BYTE Indicates compliance with a revision of this specification. It is a
		BCD value where the upper nibble indicates the major version
		and the lower nibble the minor version. For revision 2.1, the
		returned value is 21h. If the value is 00h, only the Major and
		Minor Versions in offsets 6 and 7 of the Entry Point Structure
		provide the version information. */
} _PACKED smbios_entry_point;



typedef struct 
{
	uint8	type; /*00h Type BYTE Specifies the type of structure. Types 0 through 127 (7Fh) are reserved for
		and defined by this specification. Types 128 through 256 (80h to FFh) are
		available for system- and OEM-specific information. */

	uint8	length;	/* 01h Length BYTE Specifies the length of the formatted area of the structure, starting at the
		Type field. The length of the structure’s string-set is not included. */

	uint16 handle;	/* 02h Handle WORD Specifies the structure’s handle, a unique 16-bit number in the range 0 to
		0FFFEh (for version 2.0) or 0 to 0FEFFh (for version 2.1 and later). The handle can be used with 
		the Get SMBIOS Structure function to retrieve a specific structure; the handle numbers are not required 
		to be contiguous. For v2.1 and later, handle values in the range 0FF00h to 0FFFFh are
		reserved for use by this specification.
		If the system configuration changes, a previously assigned handle might no
		longer exist. However once a handle has been assigned by the BIOS, the
		BIOS cannot re-assign that handle number to another structure. */

} _PACKED smbios_struct_header;


/* some definitions for 'type' field in smbios_struct_header */
#define BIOS_INFORMATION_TYPE		0
#define SYSTEM_INFORMATION_TYPE		1
/* etc.*/


typedef uint8 string_num;


typedef struct
{
	smbios_struct_header	header;
	uint8	specifics[1];

} _PACKED smbios_generic_struct;


typedef struct
{
	smbios_struct_header	header;
	
	string_num				vendor;
	string_num				bios_version;
	uint16					bios_base_address_segment;
	string_num				bios_release_date;
	uint32					bios_characteristics;
	uint16					bios_rom_size;

} _PACKED smbios_bios_information;

typedef struct
{
	smbios_struct_header	header;

	string_num				manufacturer;
	string_num				product_name;
	string_num				version;
	string_num				serial_number;
	uint32					uuid[4];
	uint8					wakeup_type;

} _PACKED smbios_system_information;


static const smbios_generic_struct* 
find_smbios_struct(const smbios_generic_struct* table, uint8 struct_type, uint16 struct_count, uint16 table_len)
{
	const smbios_generic_struct*	st = table;
	uint8* const	table_end = ((uint8*)table) + table_len; 

	while(struct_count--)
	{
		uint8*	ptr;

		if(st->header.type == struct_type)
			return st;

		/* skip formatted portion */
		ptr = ((uint8*)st) + st->header.length;
		/* skip all strings */
		while((*(uint16*)ptr != 0x0000) && (ptr<table_end))
			++ptr;
		/* skip terminator */
		st = (smbios_generic_struct* )(ptr+2);
	}

	return NULL;
}


static const char*
find_string(const smbios_generic_struct* st, string_num st_num)
{
	const char*	ptr = ((const char*)st) + st->header.length;
	static const char* const	empty_string = "";
	

	/* string_num is 1-based */
	if(st_num == 0)
		return empty_string;

	while(--st_num > 0)
	{
		const size_t len = strlen(ptr);
		
		if(len == 0)
			return empty_string;	
		ptr += len+1;
	}
	return ptr;
}


static const smbios_entry_point* 
find_smbios_entry_point(uint8* vaddr0)
{
	const uint8* ptr		= vaddr0+  0xF0000;
	const uint8* const end  = vaddr0+ 0x100000;

	for(; ptr<end; ptr+=16)
	{
		const smbios_entry_point* const ep = (const smbios_entry_point*) ptr;
		if(memcmp(ep->anchor_string, "_SM_", 4) == 0)
			return ep;
	}

	return 0;
}


static bool
check_smbios_entry_point(const smbios_entry_point* ep)
{
	const uint8* ptr = (const uint8*) ep;
	uint8	checksum;
	int		i;

	for(checksum=0, i=0; i<ep->length; i++)
		checksum += ptr[i];

	if(checksum != 0)
		return false;

	return true;
}

/********************************************* debugging ********************/
#if DEBUG_SMBIOS

static void
dump_smbios_entry_point(const smbios_entry_point* ep)
{
	printf(
		"SMBIOS entry point:\n"
			"\tlength %u, version %u.%u/%x, max struct size %u, eps_revision %u, dmi %.5s\n"
			"\ttable length %u, table address %08lX, %u structs in the table\n",
		ep->length, ep->version_major, ep->version_minor, ep->smbios_bsd_revision, ep->max_struct_size, 
		ep->eps_revision, ep->intermediate_anchor_string,
		ep->struct_table_length, ep->struct_table_address, ep->struct_num);
}



static void
dump_smbios_struct_header(const smbios_struct_header * h)
{
	printf("type %02x, length %u, handle %04x\n", h->type, h->length, h->handle);
}


static void
dump_string(const smbios_generic_struct* gs, string_num sn, const char* name)
{
	printf("String #%u: %s%s\n", sn, name, find_string(gs, sn));
}

static void
dump_smbios_system_information(const smbios_system_information* si)
{
	const smbios_generic_struct* const gs = (const smbios_generic_struct*)si;

	printf("System Information Structure:\n");
	dump_smbios_struct_header(&si->header);
	dump_string(gs, si->manufacturer, "manufacturer: ");
	dump_string(gs, si->product_name, "product name: ");
	dump_string(gs, si->version, "version: ");
	dump_string(gs, si->serial_number, "serial number: ");
	printf("UUID: %08lX-%08lX-%08lX-%08lX\n", si->uuid[0], si->uuid[1], si->uuid[2], si->uuid[3]);
	printf("Wake-up type %x\n", si->wakeup_type);
}

#else

#define dump_smbios_entry_point(p)
#define dump_smbios_struct_header(p)
#define dump_string(x,y,z)
#define dump_smbios_system_information(p)

#endif /* #if DEBUG_SMBIOS */
/********************************************* end debugging ********************/

#define SERIAL_NUMBER 1
#define BIOS_VENDOR 2
#define BIOS_VERSION 3
#define BIOS_DATE 4

static status_t do_put_env(int string_id, int argc, char **argv);

status_t do_putserialnumber(int argc, char **argv)
{
	return do_put_env(SERIAL_NUMBER, argc, argv);
}

status_t 
do_put_bios_vendor(int argc, char **argv)
{
	return do_put_env(BIOS_VENDOR, argc, argv);
}

status_t 
do_put_bios_version(int argc, char **argv)
{
	return do_put_env(BIOS_VERSION, argc, argv);
}

status_t 
do_put_bios_date(int argc, char **argv)
{
	return do_put_env(BIOS_DATE, argc, argv);
}


static status_t do_put_env(int string_id, int argc, char **argv)
{
	const		smbios_entry_point*	entry_point = NULL;
	const		smbios_generic_struct* smbios_table;
	uint8*		addr0 = NULL;
	area_id		kernel_ram, user_ram;
	char*		setenv_argv[3];
	status_t	err = 1;
	
	if((kernel_ram = find_area("physical_ram")) < 0)
	{
		fprintf(stderr, "Can't find the the area for physical RAM\n");
		goto err0;
	}
	
	if((user_ram = clone_area("user_physical_ram", 
         (void**)&addr0, 
         B_ANY_ADDRESS, 
         B_READ_AREA, 
         kernel_ram)) < 0 )
	{
		fprintf(stderr, "Can't clone the the area for physical RAM to user space\n");
		goto err0;
	}

	if((entry_point = find_smbios_entry_point(addr0)) == NULL)
	{
		fprintf(stderr, "Can't find SMBIOS entry point.\n");
		goto err1;		
	}

	if(!check_smbios_entry_point(entry_point))
	{ 
		fprintf(stderr, "SMBIOS entry point is invalid.\n");
		goto err1;	
	}
	
	dump_smbios_entry_point(entry_point);

	smbios_table = (smbios_generic_struct*)(entry_point->struct_table_address + addr0);

	setenv_argv[0] = argv[0];
	setenv_argv[1] = NULL;
	setenv_argv[2] = NULL; 

	if(string_id == SERIAL_NUMBER) {
		const		smbios_system_information*	si;
		si = (const	smbios_system_information*)find_smbios_struct(smbios_table, SYSTEM_INFORMATION_TYPE, entry_point->struct_num, entry_point->struct_table_length);
		if(!si)
		{
			fprintf(stderr, "Can't find SMBIOS system information\n");
			goto err1;
		}
		dump_smbios_system_information(si);
		setenv_argv[1] = (char*)find_string((const smbios_generic_struct*)si, si->serial_number);
	}
	else  {
		const		smbios_bios_information*	bi;
		bi = (const	smbios_bios_information*)find_smbios_struct(smbios_table, BIOS_INFORMATION_TYPE, entry_point->struct_num, entry_point->struct_table_length);
		if(!bi)
		{
			fprintf(stderr, "Can't find SMBIOS bios information\n");
			goto err1;
		}
		switch(string_id) {
			case BIOS_VENDOR:
				setenv_argv[1] = (char*)find_string((const smbios_generic_struct*)bi, bi->vendor);
				break;
			case BIOS_VERSION:
				setenv_argv[1] = (char*)find_string((const smbios_generic_struct*)bi, bi->bios_version);
				break;
			case BIOS_DATE:
				setenv_argv[1] = (char*)find_string((const smbios_generic_struct*)bi, bi->bios_release_date);
				break;
		}
	}
	if(setenv_argv[1] == NULL) {
		fprintf(stderr, "Can't find SMBIOS string\n");
		goto err1;
	}

	err = do_setenv(2, setenv_argv);

err1:	
	delete_area(user_ram);
err0:
	return err;
}


