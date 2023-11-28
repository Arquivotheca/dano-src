#include "private.h"

int fd;
int memtypefd;
int can_do_overlays;
shared_info *si;
accelerant_info *ai;
vuint32 *regs;
area_id	my_mode_list_area;
display_mode *my_mode_list;
int accelerantIsClone;

SUPPORTEDDEVICESTRUCT SupportedDevices[] =
{	// VenID	DevID		XClk	XClks/Hit
	// AGP Rage LT Pro
	{ 0x1002,	0x4C42,		100.0,	1.3 }, // ATI 3D Rage LT Pro AGP (normal)
	{ 0x1002,	0x4C44,		100.0,	1.3 }, // ATI 3D Rage LT Pro AGP 1x

	// PCI Rage LT Pro
	{ 0x1002,	0x4C49,		100.0,	1.3 }, // ATI 3D Rage LT Pro PCI
	// { 0x1002,	0x4C4D,		100.0,	1.3 }, // ATI 3D Mobility P3D (doesn't work currently)
	{ 0x1002,	0x4C50,		100.0,	1.3 }, // ATI 3D Rage LT Pro PCI
	{ 0x1002,	0x4C51,		100.0,	1.3 }, // ATI 3D Rage LT Pro PCI (Limited 3D?)

	// AGP Rage Pro
	{ 0x1002,	0x4742,		100.0,	1.3 }, // ATI 3D Rage Pro AGP (normal) (GB)
	{ 0x1002,	0x4744,		100.0,	1.3 }, // ATI 3D Rage Pro AGP 1x (GD)

	// PCI Rage Pro
	{ 0x1002,	0x4749,		100.0,	1.3 }, // ATI 3D Rage Pro PCI (GI)
#if EMACHINE
	{ 0x1002,	0x474D,		100.0,	1.3 }, // ATI Rage XL/XC AGP (E-machine)
#endif
	{ 0x1002,	0x4750,		100.0,	1.3 }, // ATI 3D Rage Pro PCI (GJ)
	{ 0x1002,	0x4751,		100.0,	1.3 }, // ATI 3D Rage Pro (limited 3D) (GK)

	// user reported as a IIc AGP.  May need special casing in InitAccelerant.c
	{ 0x1002,	0x475A,		100.0,	1.3 }, // ATI 3D Rage IIc AGP
	// user reported as a Mach 64 GX.  May need special casing in InitAccerlant.c
	// { 0x1002,	0x4758,		66.0,	2.0 }, // ATI Mach 64 GX

	// PCI Rage II+/IIC
	// ATI's documentation lies - the 4755 runs at 66.0 and 2, and it's
	// probably safest to assume that the 4756 does also.
	{ 0x1002,	0x4755,		66.0,	2.0 }, // ATI 3D Rage II+ PCI (GU)
	{ 0x1002,	0x4756,		66.0,	2.0 }, // ATI 3D Rage IIC PCI (GV)
	// Yet Another IIc AGP!
	{ 0x1002,	0x4757,		66.0,	2.0 }, // ATI 3D Rage IIC PCI (GV)

	// PCI Rage/Rage II
	{ 0x1002,	0x4754,		66.0,	2.6 }, // ATI 3D Rage PCI/Rage II PCI (GT)

	// End of list
	{ 0x0000,	0x0000,		0.0,	0.0 }
};
