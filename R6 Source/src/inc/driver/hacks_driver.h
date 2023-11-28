#if !defined(_HACKS_DRIVER_H_)
#define _HACKS_DRIVER_H_ 1

#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define HACKS_PRIVATE_DATA_MAGIC	'HACK' /* a private driver rev, of sorts */

enum {
	HACKS_GET_MEMORY_MAP = B_DEVICE_OP_CODES_END + 1,
	HACKS_GET_NTH_PCI_INFO,
	HACKS_GET_PCI_CONFIG,
	HACKS_SET_PCI_CONFIG,
	HACKS_GET_ISA_IO,
	HACKS_SET_ISA_IO
};

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	void	*addr;		/* -> virtual buffer to translate */
	uint32	size;		/* size of virtual buffer */
	physical_entry
		*table;		/* -> caller supplied table */
	uint32	num_entries;	/* # entries in table */
} hacks_get_memory_map;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uchar
		bus,
		device,
		function,
		offset,		/* PCI config space offset */
		size;		/* number of bytes: 1,2 or 4 */
	uint32
		value;		/* value to set or value read */
} hacks_get_set_pci_config;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint32
		offset,		/* ISA I/O address */
		size,		/* number of bytes: 1,2 or 4 */
		value;		/* value to set or value read */
} hacks_get_set_isa_io;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint32
		index;		/* Nth PCI structure to get */
	pci_info
		pcii;		/* the PCI structure requested */
} hacks_get_nth_pci_info;

#if defined(__cplusplus)
}
#endif

#endif /* _HACKS_DRIVER_H_ */
