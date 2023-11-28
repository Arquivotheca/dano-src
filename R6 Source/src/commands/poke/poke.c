/* ++++++++++
	FILE:  poke.c
	REVS:  $Revision: 1.9 $
	NAME:  herold
	DATE:  Tue Mar 28 14:58:42 PST 1995

	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<OS.h>
#include	<PCI.h>

#include	"priv_syscalls.h"

#define ISA_IO(x) (isa_base+(x))

static void cmd_db();
static void cmd_dh();
static void cmd_dw();
static void cmd_sb();
static void cmd_sh();
static void cmd_sw();
static void cmd_dm();
static void cmd_dpb();
static void cmd_dph();
static void cmd_dpw();
static void cmd_inb();
static void cmd_outb();
static void cmd_inh();
static void cmd_outh();
static void cmd_inw();
static void cmd_outw();
static void cmd_idxinb();
static void cmd_idxoutb();
static void cmd_pci();
static void cmd_cfinb();
static void cmd_cfoutb();
static void cmd_cfinh();
static void cmd_cfouth();
static void cmd_cfinw();
static void cmd_cfoutw();
static void cmd_quit();
static void cmd_help();

static int				done = 0;
static volatile char	*isa_base = 0;
static area_id			isa_area;

/* -----
	the table of debugger commands, with arguments and explanations for
	the 'help' display, and a function pointer to each command's handler
----- */

static struct cmd_rec {
	char	*name;
	char	*help_args;
	char	*help_str;
	void	(*handler) (int argc, ulong args[]);
} cmd_table [] = {
	"db", 		"addr",						"display a byte in virtual space", 					cmd_db,
	"dh", 		"addr",						"display a halfword in virtual space (16 bits)",	cmd_dh,
	"dw", 		"addr",						"display a word in virtual space (32 bits)", 		cmd_dw,
	"sb", 		"addr value",				"set a byte in virtual space", 						cmd_sb,
	"sh", 		"addr value",				"set a halfword in virtual space (16 bits)",		cmd_sh,
	"sw", 		"addr value",				"set a word in virtual space (32 bits)", 			cmd_sw,
	"dm", 		"addr [count]",				"display memory in virtual space", 					cmd_dm,
	"dpb", 		"addr",						"display a byte in physical space", 				cmd_dpb,
	"dph", 		"addr",						"display a halfword in physical space (16 bits)",	cmd_dph,
	"dpw", 		"addr",						"display a word in physical space (32 bits)",		 cmd_dpw,
	"inb",		"addr",						"read io byte",				cmd_inb,
	"outb",		"addr value",				"set an io byte",			cmd_outb,
	"inh",		"addr",						"read io halfword",			cmd_inh,
	"outh",		"addr value",				"set an io halfword",		cmd_outh,
	"inw",		"addr",						"read io word",				cmd_inw,
	"outw",		"addr value",				"set an io word",			cmd_outw,
	"idxinb",	"port index [last-idx]",	"display VGA-style indexed regs", cmd_idxinb,
	"idxoutb",	"port index value",			"write a VGA-style indexed reg", cmd_idxoutb,
	"pci",		"[device]",					"display pci device info",	cmd_pci,
	"cfinb",	"bus dev func offset [last-offset]","read pci config byte", 	cmd_cfinb,
	"cfoutb",	"bus dev func offset value","write pci config byte", 	cmd_cfoutb,
	"cfinh",	"bus dev func offset [last-offset]","read pci config halfword", cmd_cfinh,
	"cfouth",	"bus dev func offset value","write pci config halfword", cmd_cfouth,
	"cfinw",	"bus dev func offset [last-offset]","read pci config word", 	cmd_cfinw,
	"cfoutw",	"bus dev func offset value","write pci config word", 	cmd_cfoutw,
	"quit",		"",							"quit",						cmd_quit,
	"help", 	"",							"help", 					cmd_help,
};


#if __POWERPC__

/* ----------
	isa_ok check if there is an ISA bus
----- */
static bool
isa_ok (void)
{
	static bool	initialized = FALSE;

	if (!initialized) {

		initialized = TRUE;

		isa_area = clone_area (
			"isa_io_clone",
			&isa_base,
			B_ANY_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			find_area ("isa_io"));

		if (isa_area < 0)
			isa_base = NULL;
	}

	if (!isa_base) {
		printf ("sorry, no ISA bus on this machine\n");
		return FALSE;
	}
	return TRUE;
}


/* ----------
	read_isa_io - reads from isa bus i/o space
----- */

static int
read_isa_io (int bus, int addr, int size)
{
	if (!isa_ok())
		return 0;

	switch (size) {
	case 4:
		return *(vuint32 *) ISA_IO (addr);
	case 2:
		return *(vuint16 *) ISA_IO (addr);
	}
	return *(vuint8 *) ISA_IO (addr);
}


/* ----------
	write_isa_io - writes to isa bus i/o space
----- */

static void
write_isa_io (int bus, int addr, int size, int value)
{
	if (!isa_ok())
		return;

	switch (size) {
	case 4:
		*(vuint32 *) ISA_IO (addr) = value;
	case 2:
		*(vuint16 *) ISA_IO (addr) = value;
	default:
		*(vuint8 *) ISA_IO (addr) = value;
	}
	return;
}
#else

#define isa_ok() TRUE

#endif


/* ----------
	printable returns a printable translation of the passed character.
----- */

uchar
printable (uchar c)
{
	return (c >= ' ' && c <= '~') ? c : '.';
}


/* ----------
	display_line displays a line of memory.
----- */

#define width 80

static void
display_line (char *prefix, uchar *loc)
{
	char b[width];
	int offset = strlen(prefix);
	int i;

	/* if prefix too big, truncate to fit on a line */

	if (offset > width - 60) {
		offset = width - 60;
		prefix[offset] = 0;
	}

	sprintf(b, 
	"%s  %.2x%.2x %.2x%.2x %.2x%.2x %.2x%.2x  %.2x%.2x %.2x%.2x %.2x%.2x %.2x%.2x  ",
	prefix, loc[0], loc[1], loc[2], loc[3], loc[4], loc[5], loc[6], loc[7], 
	loc[8], loc[9], loc[10], loc[11], loc[12], loc[13], loc[14], loc[15]);

	/* show the ascii form of the data */

	for (i = 0; i < 16; i++) {
		b[offset+44+i] = printable(loc[i]);
	}
	b[offset+44+i] = 0;
	printf("%s\n",b);
}


/* ----------
	display_memory displays memory
----- */
void
display_memory (long addr, int count)
{
	char b[9];

	/* convert count to number of 16 byte lines */

	count = (count + 15)/16;
	if (count <= 0) count = 1;

	while (count--) {
		sprintf(b,"%.8x", addr);
		display_line(b, (uchar *) addr);
		addr += 16;
	}
}

/* ----------
	cmd_db displays a byte of memory
----- */
static void
cmd_db (int argc, ulong args[])
{
	printf ("%.8x  %.2x\n", args[0], *(uchar *) args[0]);
}


/* ----------
	cmd_dh displays a halfword of memory
----- */
static void
cmd_dh (int argc, ulong args[])
{
	printf ("%.8x  %.4x\n", args[0], *(ushort *) args[0]);
}


/* ----------
	cmd_dw displays a word of memory
----- */
static void
cmd_dw (int argc, ulong args[])
{
	printf ("%.8x  %.8x\n", args[0], *(ulong *) args[0]);
}


/* ----------
	cmd_dpb displays a byte of physical memory
----- */
static void
cmd_dpb (int argc, ulong args[])
{
	uint8		c;
	char		*va;
	area_id		aid;

	aid = _kmap_physical_memory_("poke", (void *)(args[0] & ~(B_PAGE_SIZE-1)), B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS, B_READ_AREA, (void **)&va);
	if (aid < 0) {
		printf("unable to map physical memory\n");
		return;
	}
	c = *(uint8 * )(va + (args[0] & (B_PAGE_SIZE - 1)));
	delete_area(aid);

	printf ("%.8x  %.2x\n", args[0], c);
}


/* ----------
	cmd_dph displays a halfword of physical memory 
----- */
static void
cmd_dph (int argc, ulong args[])
{
	uint16		s;
	char		*va;
	area_id		aid;

	aid = _kmap_physical_memory_("poke", (void *)(args[0] & ~(B_PAGE_SIZE-1)), B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS, B_READ_AREA, (void **)&va);
	if (aid < 0) {
		printf("unable to map physical memory\n");
		return;
	}
	s = *(uint16 * )(va + (args[0] & (B_PAGE_SIZE - 1)));
	delete_area(aid);

	printf ("%.8x  %.4x\n", args[0], s);
}


/* ----------
	cmd_dpw displays a word of physical memory
----- */
static void
cmd_dpw (int argc, ulong args[])
{
	uint32		l;
	char		*va;
	area_id		aid;

	aid = _kmap_physical_memory_("poke", (void *)(args[0] & ~(B_PAGE_SIZE-1)), B_PAGE_SIZE, B_ANY_KERNEL_ADDRESS, B_READ_AREA, (void **)&va);
	if (aid < 0) {
		printf("unable to map physical memory\n");
		return;
	}
	l = *(uint32 * )(va + (args[0] & (B_PAGE_SIZE - 1)));
	delete_area(aid);

	printf ("%.8x  %.8x\n", args[0], l);
}

/* ----------
	cmd_sb sets a byte of memory
----- */
static void
cmd_sb (int argc, ulong args[])
{
	* (char *) args[0] = args[1];
}


/* ----------
	cmd_sh sets a halfword of memory
----- */
static void
cmd_sh (int argc, ulong args[])
{
	* (short *) args[0] = args[1];
}


/* ----------
	cmd_sw sets a word of memory
----- */
static void
cmd_sw (int argc, ulong args[])
{
	* (long *) args[0] = args[1];
}


/* ----------
	cmd_dm displays memory
----- */
static void
cmd_dm (int argc, ulong args[])
{
	if (args[1] > 1024)
		args[1] = 16;
	
	display_memory (args[0], args[1]);
}


/* ----------
	cmd_inb displays an io byte
----- */
static void
cmd_inb (int argc, ulong args[])
{
	if (!isa_ok())
		return;

	printf ("io byte %.8x  %.2x\n", args[0], read_isa_io (0, args[0], 1));
}


/* ----------
	cmd_outb sets an io byte
----- */
static void
cmd_outb (int argc, ulong args[])
{
	if (!isa_ok())
		return;

	write_isa_io (0, args[0], 1, args[1]);
}


/* ----------
	cmd_inh displays an io halfword
----- */
static void
cmd_inh (int argc, ulong args[])
{
	if (!isa_ok())
		return;

	printf ("io halfword %.8x  %.4x\n", args[0], read_isa_io (0, args[0], 2));
}


/* ----------
	cmd_outh sets an io halfword
----- */
static void
cmd_outh (int argc, ulong args[])
{
	if (!isa_ok())
		return;

	write_isa_io (0, args[0], 2, args[1]);
}


/* ----------
	cmd_inw displays an io word
----- */
static void
cmd_inw (int argc, ulong args[])
{
	if (!isa_ok())
		return;

	printf ("io word %.8x  %.8x\n", args[0], read_isa_io (0, args[0], 4));
}


/* ----------
	cmd_outw sets an io word
----- */
static void
cmd_outw (int argc, ulong args[])
{
	if (!isa_ok())
		return;

	write_isa_io (0, args[0], 4, args[1]);
}


/* ----------
	cmd_idxinb reads VGA-style indexed I/O registers.
----- */
static void
cmd_idxinb (int ac, ulong args[])
{
	register int	first, last, i;

	if (!isa_ok ())
		return;

	if (ac < 2) {
		printf ("Invalid number of arguments.\n");
		return;
	}

	first = args[1];
	if (ac == 3)
		last = args[2];
	else
		last = first;

	while (first <= last) {
		printf ("0x%04x.%02x:", args[0], first);
		for (i = 16;  --i >= 0; ) {
			write_isa_io (0, args[0], 1, first++);
			printf (" %02x", read_isa_io (0, args[0] + 1, 1));
			if (first > last)
				break;
		}
		printf ("\n");
	}
}

/* ----------
	cmd_idxoutb writes a value to a VGA-style indexed register in a
	convenient way.
----- */
static void
cmd_idxoutb (int ac, ulong args[])
{
	if (!isa_ok ())
		return;

	if (ac != 3) {
		printf ("Invalid number of arguments.\n");
		return;
	}

	write_isa_io (0, args[0], 1, args[1]);
	write_isa_io (0, args[0] + 1, 1, args[2]);
}


/* ----------
	print_pci_info does just that
----- */
static void
print_pci_info (pci_info *h)
{
	int i;

	printf ("bus %.2x device %.2x function %.2x: vendor %.4x device %.4x revision %.2x\n", 
		h->bus, h->device, h->function, h->vendor_id, h->device_id, h->revision);
	printf ("  class_base = %.2x class_function = %.2x class_api = %.2x\n", 
		h->class_base, h->class_sub, h->class_api);
	printf ("  line_size=%.2x latency_timer=%.2x header_type = %.2x BIST=%.2x\n", 
		h->line_size, h->latency, h->header_type, h->bist);
	printf ("  rom_base=%.8x  pci %.8x size=%.8x\n",
		h->u.h0.rom_base, h->u.h0.rom_base_pci, h->u.h0.rom_size);
	printf ("  interrupt_line=%.2x interrupt_pin=%.2x min_grant=%.2x max_latency=%.2x\n",
		h->u.h0.interrupt_line, h->u.h0.interrupt_pin, h->u.h0.min_grant,
		h->u.h0.max_latency);
	printf ("  cardbus_cis=%.8x subsystem_id=%.4x subsystem_vendor_id=%.4x\n",
		h->u.h0.cardbus_cis, h->u.h0.subsystem_id, h->u.h0.subsystem_vendor_id);
	for (i = 0; i < 6; i++)
		printf ("  base reg %d: host addr %.8x pci %.8x size %.8x, flags %.2x\n",
			i, h->u.h0.base_registers[i], h->u.h0.base_registers_pci [i], h->u.h0.base_register_sizes[i], h->u.h0.base_register_flags[i]);
}


/* ----------
	cmd_pci dumps out info on one or all pci devices
----- */
static void
cmd_pci (int argc, ulong args[])
{
	pci_info	h;
	int			i;

	for (i = 0; ; i++) {
		if (get_nth_pci_info (i, &h) != B_NO_ERROR)
			break;
		if (argc) {
			if (h.bus == args[0] && h.device == args[1] && h.function == args[2]) {
				print_pci_info (&h);
				argc = -argc;
				break;
			}
		} else
			print_pci_info (&h);
	}
	if (argc < 0)
		printf ("could not find that pci device\n");
}
		

/* ----------
	cmd_cfinb reads a byte from pci configuration space
----- */
static void
cmd_cfinb (int argc, ulong args[])
{
	register int	first, last, i;

	if (argc < 4) {
		printf ("Insufficient number of arguments.\n");
		return;
	}

	first = args[3];
	if (argc == 5)
		last = args[4];
	else
		last = first;

	printf ("bus %.2x device %.2x function %.2x:\n",
	        args[0], args[1], args[2]);

	while (first <= last) {
		printf ("  0x%02x:", first);
		for (i = 16 / sizeof (uint8);  --i >= 0; ) {
			printf (" %02x", read_pci_config (args[0],
			                                  args[1],
			                                  args[2],
			                                  first,
			                                  sizeof (uint8)));
			first += sizeof (uint8);
			if (first > last)
				break;
		}
		printf ("\n");
	}
}


/* ----------
	cmd_cfinh reads a halfword from pci configuration space
----- */
static void
cmd_cfinh (int argc, ulong args[])
{
	register int	first, last, i;

	if (argc < 4) {
		printf ("Insufficient number of arguments.\n");
		return;
	}

	first = args[3];
	if (first & 1) {
		printf ("Offset must be a multiple of two.\n");
		return;
	}

	if (argc == 5)
		last = args[4];
	else
		last = first;

	printf ("bus %.2x device %.2x function %.2x:\n",
	        args[0], args[1], args[2]);

	while (first <= last) {
		printf ("  0x%02x:", first);
		for (i = 16 / sizeof (uint16);  --i >= 0; ) {
			printf (" %04x", read_pci_config (args[0],
			                                  args[1],
			                                  args[2],
			                                  first,
			                                  sizeof (uint16)));
			first += sizeof (uint16);
			if (first > last)
				break;
		}
		printf ("\n");
	}
}


/* ----------
	cmd_cfinw reads a word from pci configuration space
----- */
static void
cmd_cfinw (int argc, ulong args[])
{
	register int	first, last, i;

	if (argc < 4) {
		printf ("Insufficient number of arguments.\n");
		return;
	}

	first = args[3];
	if (first & 3) {
		printf ("Offset must be a multiple of four.\n");
		return;
	}

	if (argc == 5)
		last = args[4];
	else
		last = first;

	printf ("bus %.2x device %.2x function %.2x:\n",
	        args[0], args[1], args[2]);

	while (first <= last) {
		printf ("  0x%02x:", first);
		for (i = 16 / sizeof (uint32);  --i >= 0; ) {
			printf (" %08x", read_pci_config (args[0],
			                                  args[1],
			                                  args[2],
			                                  first,
			                                  sizeof (uint32)));
			first += sizeof (uint32);
			if (first > last)
				break;
		}
		printf ("\n");
	}
}


/* ----------
	cmd_cfoutb writes a byte to pci configuration space
----- */
static void
cmd_cfoutb (int argc, ulong args[])
{
	write_pci_config (args[0], args[1], args[2], args[3], 1, args[4]);
	printf ("bus %.2x device %.2x function %.2x offset %.2x:  wrote %.2x\n",
		args[0], args[1], args[2], args[3], args[4]);
}


/* ----------
	cmd_cfouth writes a halfword to pci configuration space
----- */
static void
cmd_cfouth (int argc, ulong args[])
{
	if (args[3] & 1)
		printf ("offset must be a multiple of two\n");
	else {
		write_pci_config (args[0], args[1], args[2], args[3], 2, args[4]);
		printf ("bus %.2x device %.2x function %.2x offset %.2x:  wrote %.4x\n",
			args[0], args[1], args[2], args[3], args[4]);
	}
}


/* ----------
	cmd_cfoutw writes a word to pci configuration space
----- */
static void
cmd_cfoutw (int argc, ulong args[])
{
	if (args[3] & 3)
		printf ("offset must be a multiple of four\n");
	else {
		write_pci_config (args[0], args[1], args[2], args[3], 4, args[4]);
		printf ("bus %.2x device %.2x function %.2x offset %.2x:  wrote %.8x\n",
			args[0], args[1], args[2], args[3], args[4]);
	}
}


/* ----------
	cmd_quit quits
----- */
static void
cmd_quit (int argc, ulong args[])
{
	done = 1;
}


/* ----------
	cmd_help shows helpful information about the commands
----- */
static void
cmd_help (int argc, ulong args[])
{
	int i;

	for (i = 0; i < sizeof (cmd_table) / sizeof (struct cmd_rec); i++)
		printf ("%-7s  %-33s   %s\n",
			cmd_table[i].name, cmd_table[i].help_args, cmd_table[i].help_str);
}


/* ----------
	cmd_err is called for unrecognizable input
----- */
static void
cmd_err (void)
{
	printf ("huh?\n");
}


/* ----------
	stupid little debugger
----- */
int
main (void)
{
	#define BUFLEN	64
	#define MAXARG	5
	#define SEPARATORS " \t.,\n"

	char	buf[BUFLEN];
	char	*cmd;
	char	*arg;
	ulong	args [MAXARG];
	int		i;
	int		argc;
	printf("type 'help' if you need it\n");

	done = 0;
	while (!done) {
		/* get a command */
		fflush (stdout);
		printf ("poke: ");
		fflush (stdout);
		gets(buf);
		
		cmd = strtok (buf, SEPARATORS);
		if (!cmd) {
			cmd_err();
			continue;
		}

		/* parse & convert arguments */
		memset (args, 0, sizeof(args));
		argc = 0;
		for (i = 0; i < MAXARG; i++) {
			arg = strtok (NULL, SEPARATORS);
			if (!arg)
				break;
			args[i] = strtoul (arg, NULL, 16);
			argc++;
		}

		/* call command handler, if there is one */
		for (i = 0; i < sizeof (cmd_table) / sizeof (struct cmd_rec); i++) {
			if (!strcmp (cmd, cmd_table[i].name)) {
				(*cmd_table[i].handler) (argc, args);
				break;
			}
		}
		if (i >= sizeof(cmd_table) / sizeof (struct cmd_rec))
			cmd_err();
	}
	if (isa_area >= 0)
		delete_area (isa_area);
	return 0;
}
