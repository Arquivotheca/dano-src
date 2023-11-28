/* ++++++++++
	$Source: /net/bally/be/rcs/src/addons/app_server/cirrus/cirrus.h,v $
	$Revision: 1.2 $
	$Author: pierre $
	$Date: 1996/04/08 19:24:22 $
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _CIRRUS_H
#define _CIRRUS_H

#define ISA_ADDRESS(x)  (isa_IO+(x))

#define CHIP_COUNT  2

//----------------------------------------
#define TI_REF_FREQ		14.318  /* 3025 only */

#define  cirrus_5434  0
#define  cirrus_5430  1

// Internal resolution id.
#define vga640x480		1		// resolutions
#define vga800x600		2
#define vga1024x768		3
#define vga1152x900     4
#define vga1280x1024    5
#define vga1600x1200    6
#define vga_specific    7

// internal struct used to clone the add-on from server space to client space
typedef struct {
    int	    theVGA;
    int     theMem;
    int	    scrnRowByte;
    int	    scrnWidth;
    int	    scrnHeight;
    int	    offscrnWidth;
    int	    offscrnHeight;
    int	    scrnPosH;
    int	    scrnPosV;
    int	    scrnColors;
    void    *scrnBase;
    float   scrnRate;
    short   crtPosH;
    short   crtSizeH;
    short   crtPosV;
    short   crtSizeV;
    ulong   scrnResCode;
    int     scrnResNum;
    uchar   *scrnBufBase;
    long	scrnRes;
    ulong   available_spaces;
    int     hotpt_h;
    int     hotpt_v;
    short   lastCrtHT;
    short   lastCrtVT;
    uchar   *isa_IO;
} clone_info;

#endif
