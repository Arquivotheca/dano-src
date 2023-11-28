/*****************************************************************************
 * AGP.H - AGP constants and variables                                       *
 *                                                                           *
 * Copyright (c) 1999, ATI Technologies Inc.  All Rights Reserved.           *
 *****************************************************************************/

#ifndef AGP_H
#define AGP_H

// Aperture size constants for R128_InitAGP ()

#define APERTURE_SIZE_4MB   0x00400000
#define APERTURE_SIZE_8MB   0x00800000
#define APERTURE_SIZE_16MB  0x01000000
#define APERTURE_SIZE_32MB  0x02000000
#define APERTURE_SIZE_64MB  0x04000000
#define APERTURE_SIZE_128MB 0x08000000
#define APERTURE_SIZE_256MB 0x10000000

// 'agpsize' constants
#define AGP_SIZE_4MB        0x3F
#define AGP_SIZE_8MB        0x3E
#define AGP_SIZE_16MB       0x3C
#define AGP_SIZE_32MB       0x38
#define AGP_SIZE_64MB       0x30
#define AGP_SIZE_128MB      0x20
#define AGP_SIZE_256MB      0x00
#define AGP_SIZE_512MB      -1      // some vendors support larger sizes
#define AGP_SIZE_1GB        -2      // some vendors support larger sizes
#define AGP_SIZE_2GB        -3      // some vendors support larger sizes
#define AGP_RATE_1X         0x1
#define AGP_RATE_2X         0x2

#define MAX_AGP_VENDOR_IDS  1

#define VENDOR_ID_INTEL     0x8086
#define VENDOR_ID_VIA       0x1106
#define VENDOR_ID_AMD       0x1022
#define VENDOR_ID_SIS       0x1039
#define VENDOR_ID_COMPAQ    0x0E11
#define VENDOR_ID_ALI       0x10B9

// PCI config device IDs for AGP chipsets

#define MAX_AGP_CHIPSET_IDS 2

#define DEVICE_ID_597       0x0597      // VIA
#define DEVICE_ID_598       0x0598      // VIA

#define AGP_MASTER_DEVICE   0xFFFF
#define AGP_TARGET_DEVICE   0x0000

typedef struct {
    uint8 Rate;
    uint8 SBA;
    uint8 RQ;
} AGPSTATUS;

typedef struct {
    uint32 handle;
    uint32 mem_handle;
    uint32 laddress;
    uint32 paddress;
    uint32 *pointer;
    uint32 entries;
} GART_INFO;

typedef struct {
    uint16 Vendor_ID;
    uint16 Device_ID;
    uint8 bn;
    uint8 dn;
    uint8 cap_ptr;
    uint32 ApertureBase;
    uint32 ApertureSize;
    uint32 LogicalAddress;
    uint32 SystemAddress;
    uint32 Handle;
    uint8 *BytePointer;
    GART_INFO GART;
    AGPSTATUS Status;
} _AGP_INFO;

#endif

