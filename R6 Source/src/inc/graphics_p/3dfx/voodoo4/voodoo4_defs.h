/* :ts=8 bk=0
 *
 * i740defs.h:	Bit and structure definitions for Intel-740
 *
 * Leo L. Schwab					1998.07.08
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef	__3DFX_VOODOO4_DEFS_H
#define	__3DFX_VOODOO4_DEFS_H

#ifndef	__BITTWIDDLE_H
#include <graphics_p/3dfx/common/bittwiddle.h>
#endif

#define SSTIOADDR(regName)      ((uint16)offsetof(regs_io, regName))

#define ISET32(addr, value)     outl((uint16) ((uint16) regBase + (uint16) (SSTIOADDR(addr))), value)
#define IGET32(addr)            inl((uint16) ((uint16) regBase + (uint16) (SSTIOADDR(addr))))

#define ISET8PHYS(a,b) {\
uint16 port = (uint16) (regBase) + (uint16) (a);\
outb(port, (uint8) (b));}

#define ISET16PHYS(a,b) {\
uint16 port = (uint16)(regBase) + (uint16)(a);\
outw(port, (uint16) (b));}

#define IGET8PHYS(a) inb((uint16) ((uint16) (regBase) + (uint16) (a)))
#define IGET16PHYS(a) _inpw((uint16) ((uint16) (regBase) + (uint16)(a)))

#define CHECKFORROOM while (! (inb((uint16) regBase) & (uint16)(0x3f)))

#define BIT(n)  (1UL<<(n))

// Banshee Memory Types
#define MEM_TYPE_SGRAM  0
#define MEM_TYPE_SDRAM  1

#define	SIZEOF_CURSORPAGE	0x1000	/* 4 KBytes */
#define SIZEOF_TEXTUREBUFFER (4 << 20) /* 4 Megabytes */
#define SIZEOF_CMDFIFO	0x10000 /* 64 KBytes */

// offsets from the base of memBaseAddr0
#define SST_IO_OFFSET           0x000000
#define SST_CMDAGP_OFFSET       0x080000
#define SST_2D_OFFSET           0x100000
#define SST_3D_OFFSET           0x200000
#define SST_3D_ALT_OFFSET       0x400000
#ifdef H4
#define SST_TEX_OFFSET          0x600000
#define SST_TEX0_OFFSET         0x600000
#define SST_TEX1_OFFSET         0x800000
#define SST_RESERVED_OFFSET     0xA00000
#else
#define SST_TEX_OFFSET          0x600000
#define SST_RESERVED_OFFSET     0x800000
#endif
#define SST_YUV_OFFSET          0xC00000
#define SST_LFB_OFFSET          0x1000000



/*
 * The stupid-ass VGA "standard" decided to place the same register at
 * different offsets depending on whether you were reading or writing it.
 * These two structures define the correct offsets for reads and writes.
 * Then just specify the correct union when reading or writing, and you'll
 * get the right offset.  Poof!
 */
struct vgaregs_read {
	vuint8	__reserved0[0x03b4];	/*	0x0000 - 0x03B3		*/
	vuint8	VGAMono_CR_Idx;		/*  0x03B4			*/
	vuint8	VGAMono_CR_Val;		/*  0x03B5			*/
	vuint8	__reserved1[0x0004];	/*	0x03B6 - 0x03B9		*/
	vuint8	VGAMono_ST01;		/*  0x03BA			*/
	vuint8	__reserved2[0x0005];	/*	0x03BB - 0x03BF		*/
	vuint8	VGA_AR_IdxValW;		/*  0x03C0			*/
	vuint8	VGA_AR_ValR;		/*  0x03C1			*/
	vuint8	VGA_ST00;		/*  0x03C2			*/
	vuint8	VGA_Enable;		/*  0x03C3 (???)		*/
	vuint8	VGA_SR_Idx;		/*  0x03C4			*/
	vuint8	VGA_SR_Val;		/*  0x03C5			*/
	vuint8	VGA_DACMask;		/*  0x03C6			*/
	vuint8	VGA_DACState;		/*  0x03C7			*/
	vuint8	VGA_DACWIdx;		/*  0x03C8			*/
	vuint8	VGA_DACVal;		/*  0x03C9			*/
	vuint8	VGA_FCR;		/*  0x03CA			*/
	vuint8	__reserved3;		/*	0x03CB			*/
	vuint8	VGA_Misc;		/*  0x03CC			*/
	vuint8	__reserved4;		/*	0x03CD			*/
	vuint8	VGA_GR_Idx;		/*  0x03CE			*/
	vuint8	VGA_GR_Val;		/*  0x03CF			*/
	vuint8	__reserved5[0x0002];	/*	0x03D0 - 0x03D1		*/
	vuint8	MMX_Idx;		/*  0x03D2			*/
	vuint8	MMX_Val;		/*  0x03D3			*/
	vuint8	VGA_CR_Idx;		/*  0x03D4			*/
	vuint8	VGA_CR_Val;		/*  0x03D5			*/
	vuint8	XR_Idx;			/*  0x03D6			*/
	vuint8	XR_Val;			/*  0x03D7			*/
	vuint8	__reserved6[0x0002];	/*	0x03D8 - 0x03D9		*/
	vuint8	VGA_ST01;		/*  0x03DA			*/
	vuint8	__reserved7[0x0c25];	/*	0x03DB - 0x0FFF		*/
};

struct vgaregs_write {
	vuint8	__reserved0[0x03b4];	/*	0x0000 - 0x03B3		*/
	vuint8	VGAMono_CR_Idx;		/*  0x03B4			*/
	vuint8	VGAMono_CR_Val;		/*  0x03B5			*/
	vuint8	__reserved1[0x0004];	/*	0x03B6 - 0x03B9		*/
	vuint8	VGAMono_FCR;		/*  0x03BA			*/
	vuint8	__reserved2[0x0005];	/*	0x03BB - 0x03BF		*/
	vuint8	VGA_AR_IdxValW;		/*  0x03C0			*/
	vuint8	__reserved3;		/*	0x03C1			*/
	vuint8	VGA_Misc;		/*  0x03C2			*/
	vuint8	VGA_Enable;		/*  0x03C3 (???)		*/
	vuint8	VGA_SR_Idx;		/*  0x03C4			*/
	vuint8	VGA_SR_Val;		/*  0x03C5			*/
	vuint8	VGA_DACMask;		/*  0x03C6			*/
	vuint8	VGA_DACRIdx;		/*  0x03C7			*/
	vuint8	VGA_DACWIdx;		/*  0x03C8			*/
	vuint8	VGA_DACVal;		/*  0x03C9			*/
	vuint8	__reserved4;		/*	0x03CA			*/
	vuint8	__reserved5;		/*	0x03CB			*/
	vuint8	__reserved6;		/*	0x03CC			*/
	vuint8	__reserved7;		/*	0x03CD			*/
	vuint8	VGA_GR_Idx;		/*  0x03CE			*/
	vuint8	VGA_GR_Val;		/*  0x03CF			*/
	vuint8	__reserved8[0x0002];	/*	0x03D0 - 0x03D1		*/
	vuint8	MMX_Idx;		/*  0x03D2			*/
	vuint8	MMX_Val;		/*  0x03D3			*/
	vuint8	VGA_CR_Idx;		/*  0x03D4			*/
	vuint8	VGA_CR_Val;		/*  0x03D5			*/
	vuint8	XR_Idx;			/*  0x03D6			*/
	vuint8	XR_Val;			/*  0x03D7			*/
	vuint8	__reserved9[0x0002];	/*	0x03D8 - 0x03D9		*/
	vuint8	VGA_FCR;		/*  0x03DA			*/
	vuint8	__reserved10[0x0c25];	/*	0x03DB - 0x0FFF		*/
};
#define	VGA_AR_Idx	VGA_AR_IdxValW
#define	VGA_AR_ValW	VGA_AR_IdxValW


/*
 * Use this union to access the registers based on whether you're reading
 * or writing them.
 */
typedef union vgaregs {
	struct vgaregs_read	r;
	struct vgaregs_write	w;
} vgaregs;



/*
 * The rest of the 3Dfx Banshee registers. 
 */
typedef struct regs_io_struct
{
	/* I/O Register Remap */
	vuint32	status;
	vuint32	pciInit0;
	vuint32	sipMonitor;
	vuint32	lfbMemoryConfig;
	vuint32 miscInit0;
	vuint32 miscInit1;
	vuint32 dramInit0;
	vuint32 dramInit1;
	vuint32 agpInit;
	vuint32 tmuGbeInit;
	vuint32 vgaInit0;
	vuint32 vgaInit1;
	vuint32 dramCommand;
	vuint32 dramData;
	vuint32 _reserved0[1];

	vuint32 vidTvOutBlankVCount;
	
	vuint32 pllCtrl0;
	vuint32 pllCtrl1;
	vuint32 pllCtrl2;
	
	vuint32 dacMode;
	vuint32 dacAddr;
	vuint32 dacData;
	
	vuint32 vidMaxRGBDelta;
	vuint32 vidProcCfg;
	vuint32 hwCurPatAddr;
	vuint32 hwCurLoc;
	vuint32 hwCurC0;
	vuint32 hwCurC1;
	vuint32 vidInFormat;
	vuint32 vidTvOutBlankHCount;			// vidInStatus for Voodoo Banshee
	vuint32 vidSerialParallelPort;
	vuint32 vidInXDecimDeltas;
	vuint32 vidInDecimInitErrs;
	vuint32 vidInYDecimDeltas;
	vuint32 vidPixelBufThold;
	vuint32 vidChromaMin;
	vuint32 vidChromaMax;
	vuint32 vidCurrentLine;
	vuint32 vidScreenSize;
	vuint32 vidOverlayStartCoords;
	vuint32 vidOverlayEndScreenCoord;
	vuint32 vidOverlayDudx;
	vuint32 vidOverlayDudxOffsetSrcWidth;
	vuint32 vidOverlayDudy;
	
	vuint32 _reserved_vga[12];			/* Access the VGA Registers through I/O space */
	
	vuint32 vidOverlayDvdyOffset;
	vuint32 vidDesktopStartAddr;
	vuint32 vidDesktopOverlayStride;
	vuint32 vidInAddr0;
	vuint32 vidInAddr1;
	vuint32 vidInAddr2;
	vuint32 vidInStride;
	vuint32 vidCurrOverlayStartAddr;
} regs_io;

typedef struct regs_2d_struct
{	
	/*  2D Register Map  */
	vuint32	status;
	vuint32	intrCtrl;
	vuint32	clip0Min;
	vuint32	clip0Max;
	vuint32	dstBaseAddr;
	vuint32	dstFormat;
	vuint32	srcColorkeyMin;
	vuint32	srcColorkeyMax;
	vuint32	dstColorkeyMin;
	vuint32	dstColorkeyMax;
	vuint32	bresError0;
	vuint32	bresError1;
	vuint32	rop;
	vuint32	srcBaseAddr;
	vuint32	commandExtra;
	vuint32	lineStipple;
	vuint32	lineStyle;
	vuint32	pattern0Alias;
	vuint32	pattern1Alias;
	vuint32	clip1Min;
	vuint32	clip1Max;
	vuint32	srcFormat;
	vuint32	srcSize;
	vuint32	srcXY;
	vuint32	colorBack;
	vuint32	colorFore;
	vuint32	dstSize;
	vuint32	dstXY;
	vuint32	command;
	vuint32	__reserved0;
	vuint32	__reserved1;
	vuint32	__reserved2;
	vuint32	launchArea[0x20];
	vuint32	colorPattern[0x40];
} regs_2d;

typedef struct tdfx_cmdfifo {
    vuint32 baseAddrL;
    vuint32 baseSize;
    vuint32 bump;
    vuint32 readPtrL;
    vuint32 readPtrH;
    vuint32 aMin;
    vuint32 unusedA;
    vuint32 aMax;
    vuint32 unusedB;
    vuint32 depth;
    vuint32 holeCount;
    vuint32 reserved;
} tdfx_CmdFifo;

//---------- SST chip AGP/CMD Transfer/Misc Register layout ------------------
typedef volatile struct regs_agp_struct {
  // AGP
    vuint32 agpReqSize;
    vuint32 hostAddrLow;
    vuint32 hostAddrHigh;
    vuint32 graphicsAddr;
    vuint32 graphicsStride;
    vuint32 moveCMD;
    vuint32 reservedL[2];

  // CMD FIFO 0,1
    tdfx_CmdFifo cmdFifo0;
    tdfx_CmdFifo cmdFifo1;

    vuint32 cmdFifoThresh;
    vuint32 cmdHoleInit;
    vuint32 reservedO[6];
    vuint32 reservedP[8];
    vuint32 reservedQ[8];
    vuint32 reservedR[8];
  // misc
    vuint32 yuvBaseAddr;
    vuint32 yuvStride;
    vuint32 reservedS[6];
    vuint32 crc1;
    vuint32 reservedT[3];
    vuint32 crc2;
} regs_agp;

//----------------- SST chip 3D layout -------------------------
// registers are in groups of 8 for easy decode
typedef struct tdfx_vertex_Rec {
    unsigned long x;            // 12.4 format
    unsigned long y;            // 12.4
} tdfx_vtxRec;

typedef struct regs_3d_struct
{
    vuint32 status;               // chip status, Read Only
    vuint32 intrCtrl;             // interrupt control
    tdfx_vtxRec vA;                  // Vertex A,B,C
    tdfx_vtxRec vB;
    tdfx_vtxRec vC;

    long r;             // 12.12        Parameters
    long g;             // 12.12
    long b;             // 12.12
    long z;             // 20.12
    long a;             // 12.12
    long s;             // 14.18
    long t;             // 14.18
    long w;             //  2.30

    long drdx;                  // X Gradients
    long dgdx;
    long dbdx;
    long dzdx;
    long dadx;
    long dsdx;
    long dtdx;
    long dwdx;

    long drdy;                  // Y Gradients
    long dgdy;
    long dbdy;
    long dzdy;
    long dady;
    long dsdy;
    long dtdy;
    long dwdy;

    unsigned long triangleCMD;  // execute a triangle command (float)
    unsigned long reservedA;
    tdfx_vtxRec FvA;                 // floating point version
    tdfx_vtxRec FvB;
    tdfx_vtxRec FvC;

    long Fr;                    // floating point version
    long Fg;
    long Fb;
    long Fz;
    long Fa;
    long Fs;
    long Ft;
    long Fw;

    long Fdrdx;
    long Fdgdx;
    long Fdbdx;
    long Fdzdx;
    long Fdadx;
    long Fdsdx;
    long Fdtdx;
    long Fdwdx;

    long Fdrdy;
    long Fdgdy;
    long Fdbdy;
    long Fdzdy;
    long Fdady;
    long Fdsdy;
    long Fdtdy;
    long Fdwdy;

    unsigned long FtriangleCMD;         // execute a triangle command
    unsigned long fbzColorPath;         // color select and combine
    unsigned long fogMode;              // fog Mode
    unsigned long alphaMode;            // alpha Mode
    unsigned long fbzMode;              // framebuffer and Z mode
    unsigned long lfbMode;              // linear framebuffer Mode
    unsigned long clipLeftRight;        // (6)10(6)10
    unsigned long clipBottomTop;        // (6)10(6)10

    unsigned long nopCMD;       // execute a nop command
    unsigned long fastfillCMD;  // execute a fast fill command
    unsigned long swapbufferCMD;// execute a swapbuffer command
    unsigned long fogColor;             // (8)888
    unsigned long zaColor;              // 8(8)16
    unsigned long chromaKey;            // (8)888
    unsigned long chromaRange;
    unsigned long userIntrCmd;

    unsigned long stipple;              // 32 bits, MSB masks pixels
    unsigned long c0;                   // 8.8.8.8 (ARGB)
    unsigned long c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        unsigned long fbiPixelsIn;
        unsigned long fbiChromaFail;
        unsigned long fbiZfuncFail;
        unsigned long fbiAfuncFail;
        unsigned long fbiPixelsOut;
    } stats;

    unsigned long fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    unsigned long reservedB[3];

    unsigned long colBufferAddr;
    unsigned long colBufferStride;
    unsigned long auxBufferAddr;
    unsigned long auxBufferStride;
    unsigned long reservedC;

    unsigned long clipLeftRight1;
    unsigned long clipBottomTop1;
    unsigned long reservedD[6];         // NOTE: used to store TMUprivate ptr
                                        // NOTE: used to store CSIMprivate ptr

    unsigned long reservedE[8];

    unsigned long reservedF[3];  
    unsigned long swapBufferPend;
    unsigned long leftOverlayBuf;
    unsigned long rightOverlayBuf;
    unsigned long fbiSwapHistory;
    unsigned long fbiTrianglesOut;      // triangles out counter

    vuint32 sSetupMode;
    vuint32 sVx;
    vuint32 sVy;
    vuint32 sARGB;
    vuint32 sRed;
    vuint32 sGreen;
    vuint32 sBlue;
    vuint32 sAlpha;

    vuint32 sVz;
    vuint32 sOowfbi;
    vuint32 sOow0;
    vuint32 sSow0;
    vuint32 sTow0;
    vuint32 sOow1;
    vuint32 sSow1;
    vuint32 sTow1;

    vuint32 sDrawTriCMD;
    vuint32 sBeginTriCMD;
    unsigned long reservedG[6];

    unsigned long reservedH[8];

    unsigned long reservedI[8];

    unsigned long textureMode;          // texture Mode
    unsigned long tLOD;                 // texture LOD settings
    unsigned long tDetail;              // texture detail settings
    unsigned long texBaseAddr;          // current texture base address
    unsigned long texBaseAddr1;
    unsigned long texBaseAddr2;
    unsigned long texBaseAddr38;
    unsigned long trexInit0;            // hardware init bits
    unsigned long trexInit1;            // hardware init bits
   
    unsigned long nccTable0[12];        // NCC decode tables, bits are packed
    unsigned long nccTable1[12];        // 4 words Y, 4 words I, 4 words Q

    unsigned long tChromaKeyMin;
    unsigned long tChromaKeyMax;
}  regs_3d;	

typedef struct thdfxRegs
{
	union regs_io_u			{ struct regs_io_struct regs_io;  vuint8 padding0[0x80000]; } regs_io_u;
	union regs_agp_u		{ struct regs_agp_struct regs_agp; vuint8 padding1[0x80000]; } regs_agp_u;
	union regs_2d_u		{ struct regs_2d_struct regs_2d; vuint8 padding2[0x0100000];  } regs_2d_u;
	union regs_3d_u 		{ struct regs_3d_struct regs_3d; vuint8 padding3[0x600000]; } regs_3d_u;
} thdfxRegs;

/******** BANSHEE Register Bit Definitions ****/

//-------------------- DAC Registers --------------------
//-------------------- dacMode
#define SST_DAC_MODE_2X				BIT(0)
#define SST_DAC_DPMS_ON_VSYNC			BIT(1)
#define SST_DAC_FORCE_VSYNC			BIT(2)
#define SST_DAC_DPMS_ON_HSYNC			BIT(3)
#define SST_DAC_FORCE_HSYNC			BIT(4)

//-------------------- dacAddr
#define SST_DAC_ADDR_SHIFT			0
#define SST_DAC_ADDR				(0xFF<<SST_DAC_ADDR_SHIFT)

//-------------------- dacData
#define SST_DAC_DATA_BLUE_SHIFT 		0
#define SST_DAC_DATA_BLUE			(0xFF<<SST_DAC_DATA_BLUE_SHIFT)
#define SST_DAC_DATA_GREEN_SHIFT 		8
#define SST_DAC_DATA_GREEN			(0xFF<<SST_DAC_DATA_GREEN_SHIFT)
#define SST_DAC_DATA_RED_SHIFT 			16
#define SST_DAC_DATA_RED			(0xFF<<SST_DAC_DATA_RED_SHIFT)

//------------------- SST Video ----------------------------
//------------------- vidProcCfg
#define SST_VIDEO_PROCESSOR_EN                BIT(0)
#define SST_CURSOR_MODE_SHIFT                 1
#define SST_CURSOR_MODE                       (1<<SST_CURSOR_MODE_SHIFT)
#       define SST_CURSOR_X11                 (1<<SST_CURSOR_MODE_SHIFT)
#       define SST_CURSOR_MICROSOFT           (0<<SST_CURSOR_MODE_SHIFT)
#define SST_OVERLAY_STEREO_EN                 BIT(2)
#define SST_INTERLACED_EN                     BIT(3)
#define SST_HALF_MODE                         BIT(4)
#define SST_CHROMA_EN                         BIT(5)
#define SST_CHROMA_INVERT                     BIT(6)
#define SST_DESKTOP_EN                        BIT(7)
#define SST_OVERLAY_EN                        BIT(8)
#define SST_VIDEOIN_AS_OVERLAY                BIT(9)
#define SST_DESKTOP_CLUT_BYPASS               BIT(10)
#define SST_OVERLAY_CLUT_BYPASS               BIT(11)
#define SST_DESKTOP_CLUT_SELECT               BIT(12)
#define SST_OVERLAY_CLUT_SELECT               BIT(13)
#define SST_OVERLAY_HORIZ_SCALE_EN            BIT(14)
#define SST_OVERLAY_VERT_SCALE_EN             BIT(15)
#define SST_OVERLAY_FILTER_MODE_SHIFT         16
#define SST_OVERLAY_FILTER_MODE               (3L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#       define SST_OVERLAY_FILTER_POINT       (0L<<SST_OVERLAY_FILTER_MODE_SHIFT)    
#       define SST_OVERLAY_FILTER_2X2         (1L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#       define SST_OVERLAY_FILTER_4X4         (2L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#       define SST_OVERLAY_FILTER_BILINEAR    (3L<<SST_OVERLAY_FILTER_MODE_SHIFT)
#define SST_DESKTOP_PIXEL_FORMAT_SHIFT        18
#define SST_DESKTOP_PIXEL_FORMAT              (7L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_PAL8         (0L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_RGB565       (1L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_RGB24        (2L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#       define SST_DESKTOP_PIXEL_RGB32        (3L<<SST_DESKTOP_PIXEL_FORMAT_SHIFT)
#define SST_OVERLAY_PIXEL_FORMAT_SHIFT        21
#define SST_OVERLAY_PIXEL_FORMAT              (7L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_RGB565U      (1L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_YUV411       (4L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_YUYV422      (5L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_UYVY422      (6L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#       define SST_OVERLAY_PIXEL_RGB565D      (7L<<SST_OVERLAY_PIXEL_FORMAT_SHIFT)
#define SST_DESKTOP_TILED_EN                  BIT(24)
#define SST_OVERLAY_TILED_EN                  BIT(25)
#define SST_VIDEO_2X_MODE_EN                  BIT(26)
#define SST_CURSOR_EN                         BIT(27)
#define SST_OVERLAY_EACH_VSYNC                BIT(29)
#define SST_OVERLAY_STRIDE_ADJUST             BIT(30)
#define SST_OVERLAY_DEINTERLACE_EN            BIT(31)

//-------------------- hwCurLoc
#define SST_CURSOR_X_SHIFT                    0
#define SST_CURSOR_X                          (0x7FF<<SST_CURSOR_X_SHIFT)
#define SST_CURSOR_Y_SHIFT                    16
#define SST_CURSOR_Y                          (0x7FF<<SST_CURSOR_Y_SHIFT)

//-------------------- hwCurC0, hwCurC1
#define SST_CURSOR_BLUE_SHIFT                 0
#define SST_CURSOR_BLUE                       (0xFF<<SST_CURSOR_BLUE_SHIFT)
#define SST_CURSOR_RED_SHIFT                  8
#define SST_CURSOR_RED                        (0xFF<<SST_CURSOR_RED_SHIFT)
#define SST_CURSOR_GREEN_SHIFT                16
#define SST_CURSOR_GREEN                      (0xFF<<SST_CURSOR_GREEN_SHIFT)

//------------vidOverlayDudxOffsetSrcWidth, vidOverlayDvDy
#define SST_OVERLAY_DELTA                     SST_MASK(20)
#define SST_OVERLAY_DELTA_OFFSET              SST_MASK(19)
#define SST_OVERLAY_FETCH_SIZE_SHIFT          19
#define SST_OVERLAY_FETCH_SIZE                (0x1FFF<<SST_OVERLAY_FETCH_SIZE_SHIFT)

#define SST_VIDEO_START_ADDR_SHIFT            0
#define SST_VIDEO_START_ADDR                  (0xFFFFFF<<SST_VIDEO_START_ADDR_SHIFT)
#define SST_DESKTOP_STRIDE_SHIFT              0
#define SST_DESKTOP_LINEAR_STRIDE             (0x3FFF<<SST_DESKTOP_STRIDE_SHIFT)
#define SST_DESKTOP_TILE_STRIDE               (0x7F<<SST_DESKTOP_STRIDE_SHIFT)
#define SST_OVERLAY_STRIDE_SHIFT              16
#define SST_OVERLAY_LINEAR_STRIDE             (0x3FFF<<SST_OVERLAY_STRIDE_SHIFT)
#define SST_OVERLAY_TILE_STRIDE               (0x7F<<SST_OVERLAY_STRIDE_SHIFT)

//-------------------- vidInFormat
#define SST_VIDEOIN_PIXEL_FORMAT_SHIFT        1
#define SST_VIDEOIN_PIXEL_FORMAT              (7<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#       define SST_VIDEOIN_PIXEL_YUV411       (4<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#       define SST_VIDEOIN_PIXEL_YUYV422      (5<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#       define SST_VIDEOIN_PIXEL_UYUV422      (6<<SST_VIDEOIN_PIXEL_FORMAT_SHIFT)
#define SST_VIDEOIN_DEINTERLACE_EN            BIT(4)
#define SST_VIDEOIN_VSYNC_POLARITY_LOW        BIT(5)
#define SST_VIDEOIN_HSYNC_POLARITY_LOW        BIT(6)
#define SST_VIDEOIN_VACTIVE_POLARITY_LOW      BIT(7)
#define SST_VIDEOIN_G4_FOR_POSEDGE            BIT(8)
#define SST_VIDEOIN_BUFFERING_MODE_SHIFT      9
#define SST_VIDEOIN_BUFFERING_MODE            (3<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#       define SST_VIDEOIN_SINGLE_BUFFERING   (0<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#       define SST_VIDEOIN_DOUBLE_BUFFERING   (1<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#       define SST_VIDEOIN_TRIPLE_BUFFERING   (2<<SST_VIDEOIN_BUFFERING_MODE_SHIFT)
#define SST_VIDEOIN_TILED_EN                  BIT(11)
#ifdef H4
#define SST_TVOUT_VSYNC_POLARITY_LOW	      BIT(12)
#define SST_TVOUT_HSYNC_POLARITY_LOW	      BIT(13)
#endif
#define SST_VIDEOIN_INTERFACE_SHIFT           14
#define SST_VIDEOIN_INTERFACE                 (3<<SST_VIDEOIN_INTERFACE_SHIFT)
#       define SST_VIDEOIN_INTERFACE_OFF      (0<<SST_VIDEOIN_INTERFACE_SHIFT)
#       define SST_VIDEOIN_VMI_ENABLE         (1<<SST_VIDEOIN_INTERFACE_SHIFT)
#       define SST_VIDEOIN_TVOUT_ENABLE       (2<<SST_VIDEOIN_INTERFACE_SHIFT)
#define SST_VIDEOIN_GENLOCK_ENABLE            BIT(16)
#define SST_VIDEOIN_NOT_USE_VGA_TIMING        BIT(17)
#ifdef H4
#define SST_VIDEOIN_GENLOCK_SOURCE_SHIFT      18
#define SST_VIDEOIN_GENLOCK_SOURCE            (1<<SST_VIDEOIN_GENLOCK_SOURCE_SHIFT)
#define SST_VIDEOIN_GENLOCK_SOURCE_TV         (1<<SST_VIDEOIN_GENLOCK_SOURCE_SHIFT)
#define SST_VIDEOIN_GENLOCK_SOURCE_VMI        (0<<SST_VIDEOIN_GENLOCK_SOURCE_SHIFT)
#endif
#define SST_VIDEOIN_DATA_CAPTURE_EN           BIT(19)
#define SST_VIDEOIN_HORIZ_DECIM_EN            BIT(20)
#define SST_VIDEOIN_VERT_DECIM_EN             BIT(21)

//-------------------- vidInStatus
#define SST_VIDEOIN_LAST_FIELD                BIT(1)
#define SST_VIDEOIN_LAST_BUFFER_SHIFT         1
#define SST_VIDEOIN_LAST_BUFFER               (3<<SST_VIDEOIN_LAST_BUFFER_SHIFT)

//-------------------- vidTvOutBlankHCount
#define SST_TVOUT_HCOUNT_BLANK_BEGINS_SHIFT   0
#define SST_TVOUT_HCOUNT_BLANK_BEGINS         (0x7FF<<SST_TVOUT_HCOUNT_BLANK_BEGINS_SHIFT)
#define SST_TVOUT_HCOUNT_BLANK_ENDS_SHIFT			16
#define SST_TVOUT_HCOUNT_BLANK_ENDS           (0x7FF<<SST_TVOUT_HCOUNT_BLANK_ENDS_SHIFT)

//-------------------- vidTvOutBlankVCount
#define SST_TVOUT_VCOUNT_BLANK_BEGINS_SHIFT   0
#define SST_TVOUT_VCOUNT_BLANK_BEGINS         (0x7FF<<SST_TVOUT_HCOUNT_BLANK_BEGINS_SHIFT)
#define SST_TVOUT_VCOUNT_BLANK_ENDS_SHIFT			16
#define SST_TVOUT_VCOUNT_BLANK_ENDS           (0x7FF<<SST_TVOUT_HCOUNT_BLANK_ENDS_SHIFT)

//-------------------- vidSerialParallelPort
#define SST_SERPAR_VMI_EN			BIT(0)
#define SST_SERPAR_VMI_CS_N			BIT(1)
#define SST_SERPAR_VMI_DS_N			BIT(2)
#define SST_SERPAR_VMI_RD_N			BIT(2)
#define SST_SERPAR_VMI_RW_N			BIT(3)
#define SST_SERPAR_VMI_DTACK_N			BIT(4)
#define SST_SERPAR_VMI_RDY			BIT(4)
#define SST_SERPAR_VMI_DATA_OE_N		BIT(5)
#define SST_SERPAR_VMI_DATA_SHIFT		6
#define SST_SERPAR_VMI_DATA			(0xFF<<SST_SERPAR_VMI_DATA_SHIFT)
#define SST_SERPAR_VMI_ADDR_SHIFT		14
#define SST_SERPAR_VMI_ADDR			(0x7<<SST_SERPAR_VMI_ADDR_SHIFT)
#define SST_SERPAR_TVOUT_GPIO_0			BIT(2)
#define SST_SERPAR_TVOUT_GPIO_0_OE_N		BIT(3)
#define SST_SERPAR_TVOUT_GPIO_2			BIT(14)
#define SST_SERPAR_TVOUT_GPIO_2_OE_N		BIT(15)
#define SST_SERPAR_TVOUT_GPIO_1			BIT(16)
#define SST_SERPAR_TVOUT_GPIO_1_OE_N		BIT(17)
#define SST_SERPAR_DDC_EN			BIT(18)
#define SST_SERPAR_DDC_DCK_OUT			BIT(19)
#define SST_SERPAR_DDC_DDA_OUT			BIT(20)
#define SST_SERPAR_DDC_DCK_IN			BIT(21)
#define SST_SERPAR_DDC_DDA_IN			BIT(22)
#define SST_SERPAR_I2C_EN			BIT(23)
#define SST_SERPAR_I2C_SCK_OUT			BIT(24)
#define SST_SERPAR_I2C_DSA_OUT			BIT(25)
#define SST_SERPAR_I2C_SCK_IN			BIT(26)
#define SST_SERPAR_I2C_DSA_IN			BIT(27)
#define SST_SERPAR_VMI_RESET_N			BIT(28)
#define SST_SERPAR_RESET_N			BIT(28)
#define SST_SERPAR_GPIO_1			BIT(29)
#define SST_SERPAR_GPIO_2			BIT(30)

//-------------------- vidChromaKeyMin/Max
#define SST_VIDEO_CHROMA_8BPP_INDEX_SHIFT	0
#define SST_VIDEO_CHROMA_8BPP_INDEX		(0xFF<<SST_VIDEO_CHROMA_8BPP_SHIFT)

#define SST_VIDEO_CHROMA_15BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_15BPP_BLUE		(0x1F<<SST_VIDEO_CHROMA_15BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_15BPP_GREEN_SHIFT	5
#define SST_VIDEO_CHROMA_15BPP_GREEN		(0x1F<<SST_VIDEO_CHROMA_15BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_15BPP_RED_SHIFT	10
#define SST_VIDEO_CHROMA_15BPP_RED		(0x1F<<SST_VIDEO_CHROMA_15BPP_RED_SHIFT)

#define SST_VIDEO_CHROMA_16BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_16BPP_BLUE		(0x1F<<SST_VIDEO_CHROMA_16BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_16BPP_GREEN_SHIFT	5
#define SST_VIDEO_CHROMA_16BPP_GREEN		(0x3F<<SST_VIDEO_CHROMA_16BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_16BPP_RED_SHIFT	11
#define SST_VIDEO_CHROMA_16BPP_RED		(0x1F<<SST_VIDEO_CHROMA_16BPP_RED_SHIFT)

#define SST_VIDEO_CHROMA_24BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_24BPP_BLUE		(0xFF<<SST_VIDEO_CHROMA_24BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_24BPP_GREEN_SHIFT	8
#define SST_VIDEO_CHROMA_24BPP_GREEN		(0xFF<<SST_VIDEO_CHROMA_24BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_24BPP_RED_SHIFT	16
#define SST_VIDEO_CHROMA_24BPP_RED		(0xFF<<SST_VIDEO_CHROMA_24BPP_RED_SHIFT)

#define SST_VIDEO_CHROMA_32BPP_BLUE_SHIFT	0
#define SST_VIDEO_CHROMA_32BPP_BLUE		(0xFF<<SST_VIDEO_CHROMA_32BPP_BLUE_SHIFT)
#define SST_VIDEO_CHROMA_32BPP_GREEN_SHIFT	8
#define SST_VIDEO_CHROMA_32BPP_GREEN		(0xFF<<SST_VIDEO_CHROMA_32BPP_GREEN_SHIFT)
#define SST_VIDEO_CHROMA_32BPP_RED_SHIFT	16
#define SST_VIDEO_CHROMA_32BPP_RED		(0xFF<<SST_VIDEO_CHROMA_32BPP_RED_SHIFT)

//-------------vidOverlayStartCoords, vidOverlayEndCoords
#define SST_OVERLAY_X_SHIFT                   0
#define SST_OVERLAY_X                         (0xFFF<<SST_OVERLAY_X_SHIFT)
#define SST_OVERLAY_Y_SHIFT                   12
#define SST_OVERLAY_Y                         (0xFFF<<SST_OVERLAY_Y_SHIFT)
#define SST_OVERLAY_XADJ_SHIFT                24
#define SST_OVERLAY_XADJ                      (3<<SST_OVERLAY_XADJ_SHIFT)
#define SST_OVERLAY_YADJ_SHIFT                26
#define SST_OVERLAY_YADJ                      (3<<SST_OVERLAY_YADJ_SHIFT)

//----------------- SST *Init*  ----------------------------

// pciInit0
#define SST_PCI_STALL_ENABLE            BIT(0)
#define SST_PCI_LOWTHRESH_SHIFT         2
#define SST_PCI_LOWTHRESH               (0xF << SST_PCI_LOWTHRESH_SHIFT)
#define SST_PCI_HARDCODE_BASE           BIT(7)
#define SST_PCI_READ_WS                 BIT(8)
#define SST_PCI_WRITE_WS                BIT(9)
#define SST_PCI_DISABLE_IO              BIT(11)
#define SST_PCI_DISABLE_MEM             BIT(12)
#define SST_PCI_RETRY_INTERVAL_SHIFT    13
#define SST_PCI_RETRY_INTERVAL          (0x1F << SST_PCI_RETRY_INTERVAL_SHIFT)
#define SST_PCI_INTERRUPT_ENABLE        BIT(18)
#define SST_PCI_TIMEOUT_ENABLE          BIT(19)

// sipMonitor

#define SST_SIPROCESS_OSC_CNTR             0xFFFF
#define SST_SIPROCESS_PCI_CNTR_SHIFT       16
#define SST_SIPROCESS_PCI_CNTR             (0xFFF<<SST_SIPROCESS_PCI_CNTR_SHIFT)
#define SST_SIPROCESS_OSC_CNTR_RESET_N     0
#define SST_SIPROCESS_OSC_CNTR_RUN         BIT(28)
#define SST_SIPROCESS_OSC_NAND_SEL         0
#define SST_SIPROCESS_OSC_NOR_SEL          BIT(29)
#define SST_SIPROCESS_OSC_FORCE_ENABLE     BIT(30)

// lfbMemoryConfig
#define SST_RAW_LFB_TILE_BEGIN_PAGE_SHIFT       0
#define SST_RAW_LFB_TILE_BEGIN_PAGE             (0x1FFF<<SST_RAW_LFB_TILE_BEGIN_PAGE_SHIFT)
#define SST_RAW_LFB_ADDR_STRIDE_SHIFT           13
#define SST_RAW_LFB_ADDR_STRIDE                 (0x7<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_1K       (0x0<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_2K       (0x1<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_4K       (0x2<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_8K       (0x3<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_16K      (0x4<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#       define SST_RAW_LFB_ADDR_STRIDE_MAX      (0x4<<SST_RAW_LFB_ADDR_STRIDE_SHIFT)
#define SST_RAW_LFB_TILE_STRIDE_SHIFT           16
#define SST_RAW_LFB_TILE_STRIDE                 (0x7F<<SST_RAW_LFB_TILE_STRIDE_SHIFT)

#define SST_RAW_LFB_ADDR_SHIFT                  0
#define SST_RAW_LFB_ADDR                        (0x1FFFFFF<<SST_RAW_LFB_ADDR_SHIFT)

// miscInit0
#define SST_GRX_RESET                   BIT(0)
#define SST_FBI_FIFO_RESET              BIT(1)
#define SST_REGISTER_BYTE_SWIZZLE_EN    BIT(2)
#define SST_REGISTER_WORD_SWIZZLE_EN    BIT(3)
#define SST_VIDEO_RESET                 BIT(4)
#define SST_2D_RESET                    BIT(5)
#define SST_MEMORY_TIMING_RESET         BIT(6)
#define SST_VGA_TIMING_RESET         	BIT(7)
#define SST_TVOUT_BLANK_DELAY_SHIFT	8
#define SST_TVOUT_BLANK_DELAY    	(0x7<<SST_TVOUT_BLANK_DELAY_SHIFT)
#define SST_TVOUT_SYNC_DELAY_SHIFT	11
#define SST_TVOUT_SYNC_DELAY    	(0x7<<SST_TVOUT_SYNC_DELAY_SHIFT)
#define SST_MONITOR_SYNC_DELAY_SHIFT	14
#define SST_MONITOR_SYNC_DELAY    	(0x7<<SST_MONITOR_SYNC_DELAY_SHIFT)
#define SST_YORIGIN_TOP_SHIFT           (18)
#define SST_YORIGIN_TOP                 (0xFFF<<SST_YORIGIN_TOP_SHIFT)
#define SST_RAWLFB_BYTE_SWIZZLE_EN      BIT(30)
#define SST_RAWLFB_WORD_SWIZZLE_EN      BIT(31)

// miscInit1
#define SST_INVERT_CLUT_ADDRESS         BIT(0)
#define SST_TRI_MODE_SHIFT		1
#define SST_TRI_MODE			(0x3<<SST_TRI_MODE_SHIFT)
#define SST_WRITE_SUBSYSTEM             BIT(3)
#define SST_ENABLE_ROM_WRITES		BIT(4)
#define SST_ALT_REGMAPPING              BIT(5)
#define SST_DISABLE_TEXTURE             BIT(6)
#define SST_POWERDOWN_CLUT              BIT(7)
#define SST_POWERDOWN_DAC               BIT(8)
#define SST_POWERDOWN_VID_PLL           BIT(9)
#define SST_POWERDOWN_GRX_PLL           BIT(10)
#define SST_POWERDOWN_MEM_PLL           BIT(11)
#define SST_BLOCK_WRITE_THRESH_SHIFT    12
#define SST_BLOCK_WRITE_THRESH          (0x7<<SST_BLOCK_WRITE_THRESH_SHIFT)
#define SST_DISABLE_2D_BLOCK_WRITE      BIT(15)
#define SST_DISABLE_2D_STALL_ON_3D      BIT(16)
#define SST_DISABLE_3D_STALL_ON_2D      BIT(17)
#define SST_CMDSTREAM_RESET             BIT(19)
#define SST_PCI_FAST_DEVICE		BIT(24)
#define SST_PCI_BIOS_SIZE		BIT(25)
#define SST_PCI_66_MHZ			BIT(26)
#define SST_PCI_AGP_ENABLED		BIT(27)
#define SST_PCI_DEVICE_TYPE		BIT(28)
#ifdef H3
#define SST_TVOUT_CLK_INV		BIT(28)
#define SST_TVOUT_CLK_DEL_ADJ_SHIFT     30
#define SST_TVOUT_CLK_DEL_ADJ		(0x3<<SST_TVOUT_CLK_DEL_ADJ_SHIFT)
#endif

// dramInit0
#define SST_SGRAM_RRD_SHIFT             0
#define SST_SGRAM_RRD                   (0x3L<<SST_SGRAM_RRD_SHIFT)
#define SST_SGRAM_RCD_SHIFT             2
#define SST_SGRAM_RCD                   (0x3L<<SST_SGRAM_RCD_SHIFT)
#define SST_SGRAM_RP_SHIFT              4
#define SST_SGRAM_RP                    (0x3L<<SST_SGRAM_RP_SHIFT)
#define SST_SGRAM_RAS_SHIFT             6
#define SST_SGRAM_RAS                   (0xFL<<SST_SGRAM_RAS_SHIFT)
#define SST_SGRAM_RC_SHIFT              10
#define SST_SGRAM_RC                    (0xFL<<SST_SGRAM_RC_SHIFT)
#define SST_SGRAM_CAS_SHIFT             14
#define SST_SGRAM_CAS                   (0x3L<<SST_SGRAM_CAS_SHIFT)
#define SST_SGRAM_MRS_SHIFT             16
#define SST_SGRAM_MRS                   BIT(16)
#define SST_SGRAM_RD2DQM                BIT(17)
#define SST_SGRAM_BWC                   BIT(18)
#define SST_SGRAM_WL                    BIT(19)
#define SST_SGRAM_BWL_SHIFT             20
#define SST_SGRAM_BWL                   (0x3L<<SST_SGRAM_BWL_SHIFT)
#define SST_SGRAM_RL                    BIT(22)
#define SST_SGRAM_USE_BST               BIT(23)
#define SST_SGRAM_NO_DEAD               BIT(24)
#define SST_SGRAM_EN_WPB                BIT(25)
#define SST_SGRAM_NUM_CHIPSETS          BIT(26)
#define SST_SGRAM_TYPE_SHIFT            27
#define SST_SGRAM_TYPE                  (0x1L<<SST_SGRAM_TYPE_SHIFT)
#       define SST_SGRAM_TYPE_8MBIT     (0x0L<<SST_SGRAM_TYPE_SHIFT)
#       define SST_SGRAM_TYPE_16MBIT    (0x1L<<SST_SGRAM_TYPE_SHIFT)

// dramInit1
#define SST_DRAM_REFRESH_EN             BIT(0)
#define SST_DRAM_REFRESH_VALUE_SHIFT    1
#define SST_DRAM_REFRESH_VALUE          (0x1FF<<SST_DRAM_REFRESH_VALUE_SHIFT)
#define SST_VIDEO_OVERRIDE_EN           BIT(10)
#define SST_TRIPLE_BUFFER_EN            BIT(11)
#define SST_DITHER_PASSTHROUGH          BIT(12)
#define SST_SGRAM_CLK_NODELAY           BIT(13)
#define SST_SGRAM_USE_INV_SAMPLE        BIT(14)
#define SST_SGRAM_DEL_CLK_INVERT        BIT(15)
#define SST_SGRAM_CLK_ADJ_SHIFT         16
#define SST_SGRAM_CLK_ADJ               (0xf << SST_SGRAM_CLK_ADJ_SHIFT)
#define SST_SGRAM_OFLOP_DEL_ADJ_SHIFT   20
#define SST_SGRAM_OFLOP_DEL_ADJ         (0xf << SST_SGRAM_OFLOP_DEL_ADJ_SHIFT)
#define SST_SGRAM_OFLOP_TRANS_LATCH     BIT(24)
#define SST_MCTL_SHORT_POWER_ON         BIT(25)
#define SST_MCTL_NO_AGGRESSIVE          BIT(26)
#define SST_MCTL_PAGEBREAK              BIT(27)
#define SST_MCTL_TRISTATE_OUTPUTS       BIT(28)
#define SST_MCTL_NO_VIN_LOCKING         BIT(29)
#define SST_MCTL_TYPE_SDRAM             BIT(30)

// vgaInit0
#define SST_VGA0_DISABLE                BIT(0)
#define SST_VGA0_EXTERNAL_TIMING        BIT(1)
#define SST_VGA0_CLUT_SELECT_SHIFT      2
#define SST_VGA0_CLUT_SELECT            (1 << SST_CLUT_SELECT_SHIFT)
#       define SST_CLUT_SELECT_6BIT     0
#       define SST_CLUT_SELECT_8BIT     1
#define SST_VGA0_EXTENSIONS             BIT(6)
#define SST_VGAINIT0_R7                 BIT(7)
#define SST_VGA0_WAKEUP_SELECT_SHIFT    8
#define SST_VGA0_WAKEUP_SELECT          (1 << SST_VGA0_WAKEUP_SELECT_SHIFT)
#       define SST_WAKEUP_46e8          0
#       define SST_WAKEUP_3C3           1
#define SST_VGA0_LEGACY_DECODE_SHIFT    9
#define SST_VGA0_LEGACY_DECODE          (1 << SST_VGA0_LEGACY_DECODE_SHIFT)
#       define SST_VGA0_ENABLE_DECODE   0
#       define SST_VGA0_DISABLE_DECODE  1
#define SST_VGA0_CONFIG_READBACK_SHIFT  10
#define SST_VGA0_CONFIG_READBACK        (1 << SST_VGA0_CONFIG_READBACK_SHIFT)
#       define SST_ENABLE_ALT_READBACK  0
#       define SST_DISABLE_ALT_READBACK 1


// fill in the reset of vgaInit0 sometime
#define SST_VGA0_VGA_BASE_ADDR_SHIFT    14
#define SST_VGA0_VGA_BASE_ADDR          (0xff << SST_VGA0_VGA_BASE_ADDR_SHIFT)

// vgaInit1
// fill this in some day, please
#define SST_VGA_LEGACY_DECODE BIT(9)

// vga legacy definitions!
#define SST_VGA_SEQ_CLKMODE_SCRN_OFF    BIT(5)

//----------------- SST status bits ---------------------------
#define SST_FIFOLEVEL           0x3F
#define SST_PCIFIFO_FREE        0x1F
#define SST_PCIFIFO_BUSY        BIT(5)
#define SST_VRETRACE            BIT(6)
#define SST_FBI_BUSY            BIT(7)
#define SST_TMU_BUSY            BIT(8)
#define SST_TREX_BUSY           SST_TMU_BUSY
#define SST_BUSY                BIT(9)
#define SST_GUI_BUSY            BIT(10)
#define SST_CMD0_BUSY           BIT(11)
#define SST_CMD1_BUSY           BIT(12)
#define SST_SWAPBUFPENDING_SHIFT 28
#define SST_SWAPBUFPENDING      (0x7L<<SST_SWAPBUFPENDING_SHIFT)
#define SST_PCIINTERRUPTED      BIT(31)

//----------------- SSTG command bits ---------------------------
#define SSTG_COMMAND_SHIFT      0
#define SSTG_COMMAND            (0xF<<SSTG_COMMAND_SHIFT)
#       define SSTG_NOP                 (0<<SSTG_COMMAND_SHIFT)
#       define SSTG_BLT                 (1<<SSTG_COMMAND_SHIFT)
#       define SSTG_STRETCH_BLT         (2<<SSTG_COMMAND_SHIFT)
#       define SSTG_HOST_BLT            (3<<SSTG_COMMAND_SHIFT)
#       define SSTG_HOST_STRETCH_BLT    (4<<SSTG_COMMAND_SHIFT)
#       define SSTG_RECTFILL            (5<<SSTG_COMMAND_SHIFT)
#       define SSTG_LINE                (6<<SSTG_COMMAND_SHIFT)
#       define SSTG_POLYLINE            (7<<SSTG_COMMAND_SHIFT)
#       define SSTG_POLYFILL            (8<<SSTG_COMMAND_SHIFT)

#define SSTG_GO                 BIT(8)
#define SSTG_REVERSIBLE         BIT(9)
#define SSTG_UPDATE_DSTX        BIT(10)
#define SSTG_UPDATE_DSTY        BIT(11)
#define SSTG_EN_LINESTIPPLE     BIT(12)
#define SSTG_MONO_PATTERN       BIT(13)
#define SSTG_XDIR               BIT(14)
#define SSTG_YDIR               BIT(15)
#define SSTG_TRANSPARENT        BIT(16)
#define SSTG_X_PATOFFSET_SHIFT  17
#       define SSTG_X_PATOFFSET         (0x7UL<<SSTG_X_PATOFFSET_SHIFT)
#define SSTG_Y_PATOFFSET_SHIFT  20
#       define SSTG_Y_PATOFFSET         (0x7UL<<SSTG_Y_PATOFFSET_SHIFT)
#define SSTG_CLIPSELECT         BIT(23)
#define SSTG_ROP0_SHIFT         24
#       define SSTG_ROP0                (0xFFUL<<SSTG_ROP0_SHIFT)

//----------------- SSTG commandEx bits ---------------------------
#define SSTG_EN_SRC_COLORKEY_EX BIT(0)
#define SSTG_EN_DST_COLORKEY_EX BIT(1)
#define SSTG_WAIT_FOR_VSYNC_EX  BIT(2)
#define SSTG_PAT_FORCE_ROW0     BIT(3)

//----------------- SSTG src,dstBaseAddr bits ---------------------------
#define SSTG_IS_TILED   BIT(31)
#define SSTG_BASEADDR_SHIFT 0
#define SSTG_BASEADDR (0xFFFFFF << SSTG_BASEADDR_SHIFT)

//----------------- SSTG srcFormat bits ---------------------------
#define SSTG_SRC_STRIDE_SHIFT   0
#       define SSTG_SRC_LINEAR_STRIDE   (0x3FFFUL << SSTG_SRC_STRIDE_SHIFT)
#       define SSTG_SRC_TILE_STRIDE     (0x7FUL << SSTG_SRC_STRIDE_SHIFT)
#define SSTG_SRC_FORMAT_SHIFT   16
#       define SSTG_SRC_FORMAT  (0xFUL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_HOST_BYTE_SWIZZLE  BIT(20)
#define SSTG_HOST_WORD_SWIZZLE  BIT(21)
#define SSTG_SRC_PACK_SHIFT     22
#       define SSTG_SRC_PACK    (0x3UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_SRC (0x0UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_8  (0x1UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_16 (0x2UL << SSTG_SRC_PACK_SHIFT)
#       define SSTG_SRC_PACK_32 (0x3UL << SSTG_SRC_PACK_SHIFT)

//----------------- SSTG dstFormat bits ---------------------------
#define SSTG_DST_STRIDE_SHIFT   0
#       define SSTG_DST_LINEAR_STRIDE   (0x3FFFUL << SSTG_DST_STRIDE_SHIFT)
#       define SSTG_DST_TILE_STRIDE     (0x7FUL << SSTG_DST_STRIDE_SHIFT)
#define SSTG_DST_FORMAT_SHIFT   16
#       define SSTG_DST_FORMAT  (0x7UL << SSTG_DST_FORMAT_SHIFT)

#define SSTG_PIXFMT_1BPP        (0x0UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_8BPP        (0x1UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_15BPP       (0x2UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_16BPP       (0x3UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_24BPP       (0x4UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_32BPP       (0x5UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_422YUV      (0x8UL << SSTG_SRC_FORMAT_SHIFT)
#define SSTG_PIXFMT_422UYV      (0x9UL << SSTG_SRC_FORMAT_SHIFT)
#ifdef YUV411
411 has been removed from the spec as a blit source format -KMW
#define SSTG_PIXFMT_411YUV      (0xAUL << SSTG_SRC_FORMAT_SHIFT)
#endif

#if SSTG_SRC_FORMAT_SHIFT != SSTG_DST_FORMAT_SHIFT
        error: these need to be the same
#endif

//----------------- SSTG lineStyle bits ---------------------------
#define SSTG_LSREPEAT_SHIFT     0
#define SSTG_LSREPEAT           (0xFFUL << SSTG_LSREPEAT_SHIFT)
#define SSTG_LSSIZE_SHIFT       8
#define SSTG_LSSIZE             (0x1FUL << SSTG_LSSIZE_SHIFT)
#define SSTG_LSPOS_FRAC_SHIFT   16
#define SSTG_LSPOS_FRAC         (0xFFUL << SSTG_LSPOS_FRAC_SHIFT)
#define SSTG_LSPOS_INT_SHIFT    24
#define SSTG_LSPOS_INT          (0x1FUL << SSTG_LSPOS_INT_SHIFT)

//----------------- SSTG rop defines ---------------------------
#define SSTG_ROP_ZERO   0x00UL
#define SSTG_ROP_NOR    0x11UL
#define SSTG_ROP_ANDI   0x22UL
#define SSTG_ROP_NSRC   0x33UL
#define SSTG_ROP_ANDR   0x44UL
#define SSTG_ROP_NDST   0x55UL
#define SSTG_ROP_XOR    0x66UL
#define SSTG_ROP_NAND   0x77UL
#define SSTG_ROP_AND    0x88UL
#define SSTG_ROP_XNOR   0x99UL
#define SSTG_ROP_DST    0xaaUL
#define SSTG_ROP_ORI    0xbbUL
#define SSTG_ROP_SRC    0xccUL
#define SSTG_ROP_ORR    0xddUL
#define SSTG_ROP_OR     0xeeUL
#define SSTG_ROP_ONE    0xffUL

// SSTG ternary rop defines (named rop3's that use pattern)
#define SSTG_ROP_PATINVERT      0x5aUL
#define SSTG_ROP_MERGECOPY      0xC0UL
#define SSTG_ROP_PATCOPY        0xF0UL
#define SSTG_ROP_PATPAINT       0xFBUL

#define SSTG_ISBINARYROP(rop) ((rop>>4)&0xF) == (rop&0xF)

// It's so annoying to have to put that << in real code, I'm shifting
// the rops here, damnit
#define SSTG_ROP_SRCCOPY        (0xccUL << SSTG_ROP0_SHIFT)

//-----------------------Command FIFO Registers
//------------------- baseAddrL
#define SST_BASEADDRL_SHIFT     0
#define SST_BASEADDRL           (0xffffff << SST_BASEADDRL_SHIFT)

//------------------- baseSize
#define SST_BASESIZE_SHIFT      0
#define SST_BASESIZE            (0xf << SST_BASESIZE_SHIFT)
#define SST_CMDFIFOEN_SHIFT     8
#define SST_CMDFIFOEN           (1 << SST_CMDFIFOEN_SHIFT)
#define SST_CMDFIFOAGP_SHFIT    9
#define SST_CMDFIFOAGP          (1 << SST_CMDFIFOAGP_SHFIT)
#define SST_HOLECNTDISABLE_SHIFT        10
#define SST_HOLECNTDISABLE      (1 << SST_HOLECNTDISABLE_SHIFT)

//------------------- bump
#define SST_CMDBUMP_SHIFT       0
#define SST_CMDBUMP             (0xffff << SST_CMDBUMP_SHIFT)

//------------------- readPtrL
#define SST_READPTRL_SHIFT      0
#define SST_READPTRL            (0xffffffff << SST_READPTRL_SHIFT)

//------------------- readPtrH
#define SST_READPTRH_SHIFT      0
#define SST_READPTRH            (0xf << SST_READPTRH_SHIFT)

//------------------- aMin
#define SST_CMDAMIN_SHIFT       0
#define SST_CMDAMIN             (0x1ffffff << SST_CMDAMIN_SHIFT)

//------------------- aMax
#define SST_CMDAMAX_SHIFT       0
#define SST_CMDAMAX             (0x1ffffff << SST_CMDAMIN_SHIFT)

//------------------- depth
#define SST_CMDDEPTH_SHIFT      0
#define SST_CMDDEPTH            (0xfffff << SST_CMDDEPTH_SHIFT)

//------------------- holeCount
#define SST_CMDHOLECNT_SHIFT    0
#define SST_CMDHOLECNT          (0xffff << SST_CMDHOLECNT_SHIFT)

//------------------- cmdFifoThresh
#define SST_FETCHTHRESH_SHIFT   0
#define SST_FETCHTHRESH         (0x1f << SST_FETCHTHRESH_SHIFT)
#define SST_HIGHWATER_SHIFT     5
#define SST_HIGHWATER           (0xf << SST_HIGHWATER_SHIFT)

//------------------- cmdHoleInit
#define SST_HOLETIMEOUT_SHIFT   0
#define SST_HOLETIMEOUT         (0x3fffff << SST_HOLETIMEOUT_SHIFT)
#define SST_HOLETIMEOUT_EN_SHIFT        22
#define SST_HOLETIMEOUT_EN      (1 << SST_HOLETIMEOUT_EN_SHIFT)

//------------------- yuvBaseAddr
#define SST_YUVBASEADDR_SHIFT   0
#define SST_YUVBASEADDR         (0x1ffffff << SST_YUVBASEADDR_SHIFT)

//------------------- yuvStride
#define SST_YUVSTRIDE_SHIFT     0
#define SST_YUVSTRIDE           (0x3fff << SST_YUVSTRIDE_SHIFT)
#define SST_YUVDESTTILED_SHIFT  31
#define SST_YUVDESTTILED        (1 << SST_YUVDESTTILED_SHIFT)

//----------------- SST cmdFifo*.baseSize bits ---------------------------
#define SST_CMDFIFO_SIZE                0xFF
#define SST_EN_CMDFIFO                  BIT(8)
#define SST_CMDFIFO_AGP                 BIT(9)
#define SST_CMDFIFO_DISABLE_HOLES       BIT(10)

// SST COMMAND PACKET defines
#define SSTCP_PKT_SIZE  3
#define SSTCP_PKT       SST_MASK(SSTCP_PKT_SIZE)
#define SSTCP_PKT0      0
#define SSTCP_PKT1      1
#define SSTCP_PKT2      2
#define SSTCP_PKT3      3
#define SSTCP_PKT4      4
#define SSTCP_PKT5      5
#define SSTCP_PKT6      6
#define SSTCP_PKT7      7

#define SSTCP_BOGUS_WORDS_SHIFT 29
#define SSTCP_BOGUS_WORDS       (7 << SSTCP_BOGUS_WORDS_SHIFT)

// packet 0 defines
#define SSTCP_PKT0_FUNC_SHIFT   3
#define SSTCP_PKT0_FUNC              (7<<SSTCP_PKT0_FUNC_SHIFT)
#define SSTCP_PKT0_NOP              ((0<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_JSR              ((1<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_RET              ((2<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_JMP_LOCAL    ((3<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_JMP_AGP        ((4<<SSTCP_PKT0_FUNC_SHIFT) | SSTCP_PKT0)
#define SSTCP_PKT0_ADDR_SHIFT   6
#define SSTCP_PKT0_ADDR             (0x7FFFFF<<SSTCP_PKT0_ADDR_SHIFT)

// packet 1 defines
#define SSTCP_REGBASE_SHIFT     SSTCP_PKT_SIZE
#ifdef H4
#define SSTCP_REGBASE           (0x7FF<<SSTCP_REGBASE_SHIFT)
#define SSTCP_REGBASE_FROM_ADDR(x) ( (((x)>>2) & 0x7FF) << SSTCP_REGBASE_SHIFT )
#else
#define SSTCP_REGBASE           ((0x3FF)<<SSTCP_REGBASE_SHIFT)
#define SSTCP_REGBASE_FROM_ADDR(x) ( (((x)>>2) & 0x3FF) << SSTCP_REGBASE_SHIFT )
#endif
#define SSTCP_PKT1_2D           BIT(14)
#define SSTCP_INC               BIT(15)
#define SSTCP_PKT1_NWORDS_SHIFT 16
#define SSTCP_PKT1_NWORDS       (0xFFFFUL<<SSTCP_PKT1_NWORDS_SHIFT)

// packet 2 defines
#define SSTCP_PKT2_MASK_SHIFT   SSTCP_PKT_SIZE
#define SSTCP_PKT2_MASK         (0x1FFFFFFFUL<<SSTCP_PKT2_MASK_SHIFT)

// packet 3 defines
#define SSTCP_PKT3_CMD_SHIFT    SSTCP_PKT_SIZE
#define SSTCP_PKT3_CMD          (0x7<<SSTCP_PKT3_CMD_SHIFT)
#  define SSTCP_PKT3_BDDBDD     (0<<SSTCP_PKT3_CMD_SHIFT)
#  define SSTCP_PKT3_BDDDDD     (1<<SSTCP_PKT3_CMD_SHIFT)
#  define SSTCP_PKT3_DDDDDD     (2<<SSTCP_PKT3_CMD_SHIFT)
#define SSTCP_PKT3_NUMVERTEX_SHIFT (SSTCP_PKT_SIZE+3)
#define SSTCP_PKT3_NUMVERTEX    (0xF << SSTCP_PKT3_NUMVERTEX_SHIFT)
#define SSTCP_PKT3_PMASK_SHIFT  (SSTCP_PKT_SIZE+3+4)
#define SSTCP_PKT3_PMASK        (0xFFFUL<<SSTCP_PKT3_PMASK_SHIFT)
#define SSTCP_PKT3_SMODE_SHIFT  (SSTCP_PKT3_PMASK_SHIFT+12)
#define SSTCP_PKT3_SMODE        (0x3FUL<<SSTCP_PKT3_SMODE_SHIFT)
#define SSTCP_PKT3_PACKEDCOLOR  BIT(28)

// packet 4 defines
#define SSTCP_PKT4_2D           BIT(14)
#define SSTCP_PKT4_MASK_SHIFT   15
#define SSTCP_PKT4_MASK         (0x3FFFUL<<SSTCP_PKT4_MASK_SHIFT)

// packet 5 defines
#define SSTCP_PKT5_NWORDS_SHIFT 3
#define SSTCP_PKT5_NWORDS       (0x7FFFFUL<<SSTCP_PKT5_NWORDS_SHIFT)
#define SSTCP_PKT5_BYTEN_WN_SHIFT 22
#define SSTCP_PKT5_BYTEN_WN     (0xFUL<<SSTCP_PKT5_BYTEN_WN_SHIFT)
#define SSTCP_PKT5_BYTEN_W2_SHIFT 26
#define SSTCP_PKT5_BYTEN_W2     (0xFUL<<SSTCP_PKT5_BYTEN_W2_SHIFT)
#define SSTCP_PKT5_SPACE_SHIFT  30
#define SSTCP_PKT5_SPACE        (0x3UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_LFB          (0x0UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_YUV          (0x1UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_3DLFB        (0x2UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_TEXPORT      (0x3UL<<SSTCP_PKT5_SPACE_SHIFT)
#define SSTCP_PKT5_BASEADDR     0x1FFFFFFUL

// packet 6 defines
#define SSTCP_PKT6_SPACE_SHIFT  3
#define SSTCP_PKT6_SPACE        (0x3UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_LFB          (0x0UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_YUV          (0x1UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_3DLFB        (0x2UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_TEXPORT      (0x3UL<<SSTCP_PKT6_SPACE_SHIFT)
#define SSTCP_PKT6_NBYTES_SHIFT 5
#define SSTCP_PKT6_NBYTES       (SST_MASK(20) << SSTCP_PKT6_NBYTES_SHIFT)
#define SSTCP_PKT6_SRC_BASELOW            SST_MASK(32)
#define SSTCP_PKT6_SRC_WIDTH              SST_MASK(14)
#define SSTCP_PKT6_SRC_STRIDE_SHIFT       14
#define SSTCP_PKT6_SRC_STRIDE             (SST_MASK(14) <<SSTCP_PKT6_SRC_STRIDE_SHIFT)
#define SSTCP_PKT6_SRC_BASEHIGH_SHIFT     28
#define SSTCP_PKT6_SRC_BASEHIGH           (SST_MASK(4) <<SSTCP_PKT6_SRC_BASEHIGH_SHIFT)
#define SSTCP_PKT6_FRAME_BUFFER_OFFSET    SST_MASK(26)
#define SSTCP_PKT6_DST_STRIDE             SST_MASK(15)

//------------------- SST I/O ----------------------------
#define SSTCP_CLIP0MIN         0x00000001L
#define SSTCP_CLIP0MAX         0x00000002L
#define SSTCP_DSTBASEADDR      0x00000004L
#define SSTCP_DSTFORMAT        0x00000008L
#define SSTCP_SRCCOLORKEYMIN   0x00000010L
#define SSTCP_SRCCOLORKEYMAX   0x00000020L
#define SSTCP_DSTCOLORKEYMIN   0x00000040L
#define SSTCP_DSTCOLORKEYMAX   0x00000080L
#define SSTCP_BRESERROR0       0x00000100L
#define SSTCP_BRESERROR1       0x00000200L
#define SSTCP_ROP              0x00000400L
#define SSTCP_SRCBASEADDR      0x00000800L
#define SSTCP_COMMANDEXTRA     0x00001000L
#define SSTCP_LINESTIPPLE      0x00002000L
#define SSTCP_LINESTYLE        0x00004000L
#define SSTCP_PATTERN0ALIAS    0x00008000L
#define SSTCP_PATTERN1ALIAS    0x00010000L
#define SSTCP_CLIP1MIN         0x00020000L
#define SSTCP_CLIP1MAX         0x00040000L
#define SSTCP_SRCFORMAT        0x00080000L
#define SSTCP_SRCSIZE          0x00100000L
#define SSTCP_SRCXY            0x00200000L
#define SSTCP_COLORBACK        0x00400000L
#define SSTCP_COLORFORE        0x00800000L
#define SSTCP_DSTSIZE          0x01000000L
#define SSTCP_DSTXY            0x02000000L
#define SSTCP_COMMAND          0x04000000L

/**********************************
 * 2D Registers
 * These register offset values are used with the Fifo access
 * mechanism to cause FIFO'd writes to the registers
 */
#define SST_2D_OFFSET           0x100000
#define SST_2D_CLIP0MIN SST_2D_OFFSET+0x8
#define SST_2D_CLIP0MAX SST_2D_OFFSET+0xC
#define SST_2D_DSTBASEADDR SST_2D_OFFSET+0x10
#define SST_2D_DSTFORMAT SST_2D_OFFSET+0x14
#define SST_2D_SRCCOLORKEYMIN SST_2D_OFFSET+0x18
#define SST_2D_SRCCOLORKEYMAX SST_2D_OFFSET+0x1c
#define SST_2D_DSTCOLORKEYMIN SST_2D_OFFSET+0x20
#define SST_2D_DSTCOLORKEYMAX SST_2D_OFFSET+0x24
#define SST_2D_BRESERROR0 SST_2D_OFFSET+0x28
#define SST_2D_BRESERROR1 SST_2D_OFFSET+0x2c
#define SST_2D_ROP SST_2D_OFFSET+0x30
#define SST_2D_SRCBASEADDR SST_2D_OFFSET+0x34
#define SST_2D_COMMANDEXTRA SST_2D_OFFSET+0x38
#define SST_2D_LINESTIPPLE SST_2D_OFFSET+0x3c
#define SST_2D_LINESTYLE SST_2D_OFFSET+0x40
#define SST_2D_CLIP1MIN SST_2D_OFFSET+0x4C
#define SST_2D_CLIP1MAX SST_2D_OFFSET+0x50
#define SST_2D_SRCFORMAT SST_2D_OFFSET+0x54
#define SST_2D_SRCSIZE SST_2D_OFFSET+0x58
#define SST_2D_SRCXY SST_2D_OFFSET+0x5C
#define SST_2D_COLORBACK SST_2D_OFFSET+0x60
#define SST_2D_COLORFORE SST_2D_OFFSET+0x64
#define SST_2D_DSTSIZE SST_2D_OFFSET+0x68
#define SST_2D_DSTXY SST_2D_OFFSET+0x6C
#define SST_2D_COMMAND SST_2D_OFFSET+0x70
#define SST_2D_LAUNCH SST_2D_OFFSET+0x80
#define SST_2D_PATTERN0 SST_2D_OFFSET+0x100
#define SST_2D_PATTERN1 SST_2D_OFFSET+0x104

/*****************************************************************************
 * "Standard" VGA register indices and bit definitions.
 ****
 * Sequencer bit and index definitions
 * I've decided to use the XFree86/NVidia bit definition form, even though
 * it makes the use of ctags difficult.
 */
#define	SR_RESET				0x00
#define	SR_RESET_SYNC				1:1
#define	SR_RESET_SYNC_ENABLE			0
#define	SR_RESET_SYNC_DISABLE			1
#define	SR_RESET_ASYNC				0:0
#define	SR_RESET_ASYNC_ENABLE			0
#define	SR_RESET_ASYNC_DISABLE			1

#define	SR_CLOCK				0x01
#define	SR_CLOCK_VIDDISABLE			5:5
#define	SR_CLOCK_SHIFTLOAD4			4:4
#define	SR_CLOCK_SHIFTLOAD4_1			0
#define	SR_CLOCK_SHIFTLOAD4_4			1
#define	SR_CLOCK_CLKDIV				3:3
#define	SR_CLOCK_CLKDIV_DIV1			0
#define	SR_CLOCK_CLKDIV_DIV2			1
#define	SR_CLOCK_SHIFTLOAD2			2:2
#define	SR_CLOCK_SHIFTLOAD2_1			0
#define	SR_CLOCK_SHIFTLOAD2_2			1
#define	SR_CLOCK_TXCELLWIDTH			0:0
#define	SR_CLOCK_TXCELLWIDTH_9			0	/*  Backwards from  */
#define	SR_CLOCK_TXCELLWIDTH_8			1	/* other VGA cores  */

#define	SR_PLANES				0x02
#define	SR_PLANES_MASK				3:0

#define	SR_FONTBASE				0x03
#define	SR_FONTBASE_SETB_OFFLO			5:5
#define	SR_FONTBASE_SETB_OFFLO_0K		0
#define	SR_FONTBASE_SETB_OFFLO_8K		1
#define	SR_FONTBASE_SETA_OFFLO			4:4
#define	SR_FONTBASE_SETA_OFFLO_0K		0
#define	SR_FONTBASE_SETA_OFFLO_8K		1
#define	SR_FONTBASE_SETB_OFFHI			3:2
#define	SR_FONTBASE_SETB_OFFHI_0K		0
#define	SR_FONTBASE_SETB_OFFHI_16K		1
#define	SR_FONTBASE_SETB_OFFHI_32K		2
#define	SR_FONTBASE_SETB_OFFHI_48K		3
#define	SR_FONTBASE_SETA_OFFHI			1:0
#define	SR_FONTBASE_SETA_OFFHI_0K		0
#define	SR_FONTBASE_SETA_OFFHI_16K		1
#define	SR_FONTBASE_SETA_OFFHI_32K		2
#define	SR_FONTBASE_SETA_OFFHI_48K		3

#define	SR_MODE					0x04
#define	SR_MODE_CHAIN4				3:3
#define	SR_MODE_ODDEVEN				2:2
#define	SR_MODE_ODDEVEN_ALTERNATE		0
#define	SR_MODE_ODDEVEN_SEQUENTIAL		1
#define	SR_MODE_EXTMEM				1:1
#define	SR_MODE_EXTMEM_DISABLE			0
#define	SR_MODE_EXTMEM_ENABLE			1

#define	SR_HCOUNTRESET				0x07


#endif	/*  __3DFX_VOODOO4_H  */
