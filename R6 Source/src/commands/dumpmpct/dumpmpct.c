/* Copied from nukernel/arch/intel/mps.h & mps.c with all warts. */

#include <OS.h>

#include <stdlib.h>
#include <stdio.h>

#define dprintf printf

#define	_PACKED		__attribute__((packed))

#define	BIOS_AREA	0x00000		/* BIOS Area */
#define	BIOS_EBDA_PTR	0x40E	/* EBDA pointer */
#define	BIOS_BASE_MEM	0x413	/* Base mem area */

/* types of entries (stored in bytes[0]) */
#define MP_ET_PROC	0	 	/* processor */
#define MP_ET_BUS	1	 	/* bus */
#define MP_ET_IOAPIC	2	 	/* i/o apic */
#define MP_ET_I_INTR	3	 	/* interrupt assignment -> i/o apic */
#define MP_ET_L_INTR	4	 	/* interrupt assignment -> local apic */


struct pcmp_fptr {
	char	sig[4];
	uint32	paddr;
	uint8	len;
	uint8	rev;
	uint8	checksum;
	uint8	mp_feature1;
	uint8	mp_feature2;
	uint8	fill[3];
} _PACKED;


struct mpchdr {
	char	sign[4];		 	/* signature "MPAT" */
	uint16	tbl_len;		 	/* length of table */
	uint8	spec_rev;		 	/* MP+AT specification revision no. */
	uint8	checksum;
	char	oem_id[8];
	char	product_id[12];
	uint32	oem_ptr;	 	/* pointer to oem table (optional) */
	uint16	oem_len;		 	/* length of above table */
	uint16	num_entry;	 	/* number of 'entry's to follow */
	uint32	loc_apic_adr;	 	/* local apic physical address */
	uint16	ext_table_len;
	uint8	ext_table_checksum;
	uint8	res;
} _PACKED;



struct mpe_proc {
	uint8	entry_type;
	uint8	apic_id;
	uint8	apic_vers;
	uint8	cpu_flags;
	uint32		stepping	: 4,
	    		model		: 4,
	    		family		: 4,
				fill1		: 20;
	uint32	features;
	uint32	fill2[2];
} _PACKED;

struct mpe_bus {
	uint8	entry_type;
	uint8	bus_id;
	char	name[6];
} _PACKED;

struct mpe_ioapic {
	uint8	entry_type;
	uint8	apic_id;
	uint8	apic_vers;
	uint8	ioapic_flags;
	uint32	io_apic_adr;
} _PACKED;

#define MP_INTR_POLARITY_MASK			0x03
#define MP_INTF_POLARITY_BUS_DEFAULT	0x00
#define MP_INTF_POLARITY_HIGH			0x01
#define MP_INTF_POLARITY_RESERVED		0x02
#define MP_INTF_POLARITY_LOW			0x03

#define MP_INTR_TRIGGER_MASK			0x0C
#define MP_INTR_TRIGGER_BUS_DEFAULT		0x00
#define MP_INTR_TRIGGER_EDGE			0x04
#define MP_INTR_TRIGGER_RESERVED		0x08
#define MP_INTR_TRIGGER_LEVEL			0x0C



struct mpe_intr {
	uint8	entry_type;
	uint8	intr_type;
	uint8	intr_flags;
	uint8	fill;
	uint8	src_bus;
	uint8	src_irq;
	uint8	dest_apicid;
	uint8	dest_line;
} _PACKED;


union mpcentry {
	uint8				bytes[20];
	struct mpe_proc		p;
	struct mpe_bus		b;
	struct mpe_ioapic	a;
	struct mpe_intr		i;
} _PACKED;


struct mpconfig {
	struct mpchdr hdr;
	union mpcentry entry[1];
} _PACKED;


typedef void* (*dump_mp_entry_handler)(void* mp_entry);

static void* dump_cpu(void* entry);
static void* dump_bus(void* entry);
static void* dump_ioapic(void* entry);
static void* dump_io_int(void* entry);
static void* dump_local_int(void* entry);
	
#define NUM_DUMP_HANDLERS  5
dump_mp_entry_handler dump_handlers[NUM_DUMP_HANDLERS] =
{
	dump_cpu,
	dump_bus,
	dump_ioapic,
	dump_io_int,
	dump_local_int
};


uint32	mp_table_paddr;



static void dump_string(const char* str, unsigned len)
{
	unsigned i;
	
	for(i=0; i<len; i++)
	{
		dprintf("%c", *str);
		++str;
	}
}



int search_pcmp_fptr(uint8*	start_addr, ulong length, struct pcmp_fptr *pcmpfptr)
{
	ulong	map_addr;
	struct pcmp_fptr  *p, *endp;
		
	map_addr = ((uint32)start_addr) & 0xfffff000;		/* Align addr to 4K */
	
	p = (struct pcmp_fptr *)map_addr;
	endp = (struct pcmp_fptr *)(map_addr + length + (((uint32)start_addr) & 0xfff) -
			sizeof(*p));
	for (; p <= endp; p++)
	{
		if (p->sig[0] == '_' && p->sig[1] == 'M' &&
			p->sig[2] == 'P' && p->sig[3] == '_')
		{
			uint8 *cp, sum;
			int i;
	
			sum = 0;
			cp = (uint8 *)p;
			for (i=0; i<sizeof(*p); i++)
				sum += *cp++;
			if (sum == 0)
			{
				/* Initialize the MP feature bytes */

				pcmpfptr->mp_feature1 = p->mp_feature1;
				pcmpfptr->mp_feature2 = p->mp_feature2;
				pcmpfptr->paddr = p->paddr;

				if (pcmpfptr->mp_feature1 == 0)
				{
					return 0;
				}
				else
				{
					dprintf("Found default configuration #%u\n", pcmpfptr->mp_feature1);
					exit(1);
					return -1;
				}
			}
		}
	}
	return -1;
} 




int apic_table_look(void** table_vaddr_ptr, uint8* addr0)
{
	uint8 *bios_addr;
	struct	pcmp_fptr pcmp;
	ulong	ebda_seg;
	ulong	base_mem;


	bios_addr = addr0 + BIOS_AREA;

	/* Get the EBDA segment and Base Memory size of the system */

	ebda_seg = *(unsigned short *)((ulong)bios_addr + BIOS_EBDA_PTR);
	base_mem = *(unsigned short *)((ulong)bios_addr + BIOS_BASE_MEM);

	/* Check of the system supports EBDA Segment and if so search for 
	 * MPS floating MP config table in EBDA segment else search in
	 * the high 1K of base memory
	 */

	/* 
	 * Search for MPS version 1.1 
	 */

	if ((((ebda_seg << 4) == (base_mem * 1024)) && 	
		!(search_pcmp_fptr(addr0 + (ebda_seg << 4), 1024, &pcmp) < 0))  ||
		!(search_pcmp_fptr(addr0 + (base_mem - 1) * 1024, 1024, &pcmp) < 0) ||
		!(search_pcmp_fptr(addr0 + 0xf0000, 64*1024, &pcmp) < 0))
	{
		dprintf("Found MPC Table version 1.%d at 0x%08X\n", pcmp.rev, pcmp.paddr);
		if(pcmp.mp_feature2 & 0x80)
			dprintf("IMCRP & PIC Mode\n");
		if(pcmp.paddr >= 0x80000000)
		{
			dprintf("Can't read MP Table from such high address: 0x%08X\n", pcmp.paddr); // FIXME HACK
			exit(6);
		}
		*table_vaddr_ptr = (void*)(addr0 + pcmp.paddr);
	}
	else
	{
		return(B_ERROR);		/* Does not support MPS Spec */
	}

	return(B_OK);
}


status_t 
apic_table_chk(void* table_vaddr)
{
	int i,n;
	struct mpconfig *mp;
	uint8 *cp, sum;
	int		mpc_table_size;

	/* get virtual address for it */
	mp =  (struct mpconfig *)table_vaddr;
	if (mp->hdr.sign[0] != 'P' || mp->hdr.sign[1] != 'C' ||
	    mp->hdr.sign[2] != 'M' || mp->hdr.sign[3] != 'P') 
		return  B_ERROR;

	/* check table length */
	n = mp->hdr.tbl_len;
	if (n < sizeof(struct mpchdr) || n > 0x1000) 
		return  B_ERROR;

	/* calculate checksum */
	sum = 0;
	cp = (uint8 *)mp;
	for (i=0; i<n; i++)
		sum += *cp++;
	if (sum != 0) 
		return  B_ERROR;
	
	return B_OK;
}


void dump_mpc_table(void* table_vaddr)
{
	unsigned i;
	struct mpconfig* mp = (struct mpconfig*)table_vaddr; 
	struct mpchdr* header = &(mp->hdr); 
	unsigned num_entry = header->num_entry;
	uint8* entry_ptr = (uint8*)(mp->entry);

	
	dprintf("\nHeader: base_len %d, spec_rev 1.%d, OEM_ID ",  header->tbl_len, header->spec_rev);
	dump_string(header->oem_id, sizeof(header->oem_id));
	dprintf(", product_id ");
	dump_string(header->product_id, sizeof(header->product_id));
	dprintf("\nOEM_table_ptr %08x, OEM_table_size %d, entry_count: %d, LAPIC addr %08x, ext_table_size %d\n",
		header->oem_ptr, header->oem_len, header->num_entry, header->loc_apic_adr, header->ext_table_len);
	
	for(i=0; i<num_entry; i++)
	{
		uint8 entry_type = entry_ptr[0];
		
		if(entry_type>=NUM_DUMP_HANDLERS)
		{
			dprintf("Unknown MPC entry: type=0x%x\n", entry_type);
			break;
		} 
		entry_ptr = dump_handlers[entry_type](entry_ptr);
	}
	dprintf("\n");
}



static void* dump_cpu(void* entry)
{
	struct mpe_proc* cpu_entry = entry;
	
	dprintf("CPU: LAPIC_id 0x%02x, LAPIC_version %02x, %s, %s, family %u, model %u, stepping %u, features %08x\n",
		 cpu_entry->apic_id, cpu_entry->apic_vers, 
		 (cpu_entry->cpu_flags&0x01)? "enabled " : "disabled",
		 (cpu_entry->cpu_flags&0x02)? "bootstrap   " : "nonbootstrap",
		 cpu_entry->family, cpu_entry->model, cpu_entry->stepping, cpu_entry->features);
	 
	return ((uint8 *)entry + sizeof(struct mpe_proc));
}


static void* dump_bus(void* entry)
{
	struct mpe_bus* bus_entry = entry;
	
	dprintf("Bus: bus_id %u, type: ", bus_entry->bus_id);
	dump_string(bus_entry->name, sizeof(bus_entry->name));
	dprintf("\n");
	
	return ((uint8 *)entry + sizeof(struct mpe_bus));
}

static void* dump_ioapic(void* entry)
{
	struct mpe_ioapic* ioapic_entry = entry;
	
	dprintf("IOAPIC: id %u, version %02x, flags %02x, address %08x\n",
		ioapic_entry->apic_id, ioapic_entry->apic_vers, ioapic_entry->ioapic_flags,
		ioapic_entry->io_apic_adr);
	
	return ((uint8 *)entry + sizeof(struct mpe_ioapic));
}




static void* dump_io_int(void* entry)
{
	const char* int_types[] = {	" INT", " NMI", " SMI", "EINT"};
	const char* polarity[] = {"bus spec   ", "active high", "reserved   ", "active low "};
	const char* trigger_mode[] = {"bus spec", "edge    ", "reserved", "level   "};
	
	struct mpe_intr* intr_entry = entry;
	
	/* handle both io and local interrupt */
	
	if(intr_entry->entry_type == MP_ET_I_INTR)
		dprintf("IO INTERRUPT:    ");
	else
		dprintf("LOCAL INTERRUPT: ");
	
	dprintf("type: %s, polarity: %s, trigger mode: %s, bus id %u, bus IRQ %02x, APIC id %u, APIC INTIN# %u\n",
		int_types[intr_entry->intr_type],
		polarity    [intr_entry->intr_flags & 0x03],
		trigger_mode[(intr_entry->intr_flags & 0x0C) >> 2],
		intr_entry->src_bus, intr_entry->src_irq,
		intr_entry->dest_apicid, intr_entry->dest_line );
	
	return ((uint8 *)entry + sizeof(struct mpe_intr));
}


static void* dump_local_int(void* entry)
{
	/* local interrupt entry has the same structure as io entry */
	return dump_io_int(entry);
}



int main()
{
	void*	table_vaddr = NULL;
	void*	addr0 = NULL;
	area_id	kernel_ram, user_ram;
	
	if((kernel_ram = find_area("physical_ram")) < 0)
	{
		dprintf("Can't find the the area for physical RAM\n");
		return 3;
	}
	
	
	if((user_ram = clone_area("user_physical_ram", 
         &addr0, 
         B_ANY_ADDRESS, 
         B_READ_AREA, 
         kernel_ram)) < 0 )
	{
		dprintf("Can't clone the the area for physical RAM to user space\n");
		return 4;
	}
	
	if(apic_table_look(&table_vaddr, (uint8*)addr0) < 0)
	{
		dprintf("The system does not support MP Specification.\n");
		delete_area(user_ram);
		return 1;		
	}


	if(apic_table_chk(table_vaddr) < 0)
	{ 
		dprintf("MPC Table is invalid.\n");
		delete_area(user_ram);
		return 2;	
	}
	
	dump_mpc_table(table_vaddr);
	
	delete_area(user_ram);
	
	return 0;
}


