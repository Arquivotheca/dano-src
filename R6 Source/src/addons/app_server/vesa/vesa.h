/*****************************************************************************
//
//	File:		 vesa.h
//
//	Description: add-on for graphics cards using vesa BIOS, header
//
//	Copyright 1996, Be Incorporated
//
*****************************************************************************/

#ifndef _VESA_H
#define _VESA_H

#include <GraphicsCard.h>

#define ISA_ADDRESS(x)  (isa_IO+(x))
 
/* Standard macro. */
#ifdef abs
#undef abs
#endif
#define abs(a) (((a)>0)?(a):(-(a)))

#define far_to_charp(x) ((char*)(((x).offset+((x).segment<<4))))

typedef struct {
  uint16	offset;
  uint16	segment;
} far_ptr;

typedef struct {
  char	    sig[4];
  uint16	version;
  far_ptr	OEMstringptr;
  uint32	cap;
  far_ptr	modeptr;
  uint16	totalmemory;
  uint16	rev;
  far_ptr	vendorname;
  far_ptr	productname;
  far_ptr	revisionstring;
  char	    reserved[222];
  char	    OemData[256];
} vesa_info_t;

typedef struct {
  uint16    ModeAttributes;
  uint8     WinAAttributes;
  uint8     WinBAttributes;
  uint16    WinGranularity;
  uint16    WinSize;
  uint16    WinASegment;
  uint16    WinBSegment;
  far_ptr   WinFuncPtr;
  uint16    BytesPerScanLine;
  uint16    XResolution;
  uint16    YResolution;
  uint8     XCharSize;
  uint8     YCharSize;
  uint8     NumberOfPlanes;
  uint8     BitsPerPixel;
  uint8     NumberOfBanks;
  uint8     MemoryModel;
  uint8     BankSize;
  uint8     NumberOfImagePages;
  uint8     ReservedPage;
  uint8     RedMaskSize;
  uint8     RedFieldPosition;
  uint8     GreenMaskSize;
  uint8     GreenFieldPosition;
  uint8     BlueMaskSize;
  uint8     BlueFieldPosition;
  uint8     RsvdMaskSize;
  uint8     RsvdFieldPosition;
  uint8     DirectColorModeInfo;
  uint32    PhysBasePtr;
  uint32    OffScreenMemOffset;
  uint16    OffScreenMemSize;
  uint8     Reserved[206];
} vesa_mode_info;

#define FUNCTION_COUNT 11

#define MODE_COUNT_MAX 256

#define is_dac_8_bits    0x00000001
#define is_vga           0x00000002
#define needs_blanking   0x00000004

typedef struct {
  uint16    code;
  uint8     be_code;
  uint8     bits_per_pixel;
  uint16    height;
  uint16    width;
  uint16    bytes_per_row;
  uint32    base_addr;
} vesa_mode;

typedef struct {
  bool      functions[FUNCTION_COUNT];
  uint32    capabilities;
  vesa_mode modes[MODE_COUNT_MAX];
  int32     mode_count;
  uint32    be_flags;
  uint16    cur_mode;
  uint8     *pci_base;
  uint32    phys_pci_base;
  indexed_color_table my_ct;
} vesa_state;

#endif






















