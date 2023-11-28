#if !defined(_VGA_MAP_H_)
#define _VGA_MAP_H_

#include <Drivers.h>
#include <PCI.h>

#define STUB_MODE_COUNT_MAGIC	0xdeadbeef
#define STUB_MAP_MAGIC	'STUB'

enum {
	VGA_MAP_GET_COUNT = B_DEVICE_OP_CODES_END + 1,
	VGA_MAP_MAP_NTH,
	VGA_MAP_UNMAP_NTH
};

typedef struct {
	int32 nth;
	pci_info pcii;
} vga_map_nth_pci;

#endif
