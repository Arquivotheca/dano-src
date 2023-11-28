/* ++++++++++
	FILE:	pci_priv.h
	REVS:	$Revision$
	NAME:	herold
	DATE:	Mon Oct 28 16:22:04 PST 1996
	Copyright (c) 1996-1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _PCI_PRIV_H
#define _PCI_PRIV_H

/* ---
	generic infomation about a PCI bus.  Should be enough to let
	platform-dependent code do whatever need be done.
--- */

typedef struct {
	void	*isa_io;				/* -> ISA compatible i/o */
	void	*config_addr;			/* -> addr reg for config access */
	void	*config_data;			/* -> data reg for config access */
	bool	host_bridge;			/* flag: connected to host bus */
	char	bus;					/* bus number */
	char	last_bus;				/* highest bus number downstream */
} pci_bus;

extern int		num_busses;
extern pci_bus	*busses;
extern int		get_pci_bus_count(void);
extern pci_info	*get_pci_table(void);
extern int		get_pci_function_count(void);

#ifdef APIC
extern pci_info*	get_nth_pci_info_ptr(long index);
#endif

#endif
