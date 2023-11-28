/*
   	This contains the structures needed to interface
   	with the pseudo-driver
*/

#ifndef _THDFX_H
#define _THDFX_H
#include <Drivers.h>

/* -----
	ioctl opcodes
----- */

typedef struct thdfx_access_pci_reg
{
	uint32	reg;
	uint32	value;
} thdfx_access_pci_reg;

enum private_ioctls {
	THDFX_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	THDFX_IOCTL_GET_PCI,
	THDFX_IOCTL_SET_PCI,
	THDFX_IOCTL_GET_SLOTNUM,
  THDFX_IOCTL_GET_AREA_ID,
  THDFX_IOCTL_GET_PHYSICAL,
  THDFX_IOCTL_SET_WC,
  THDFX_IOCTL_SET_UC,
  THDFX_IOCTL_GET_CARD_ADDR,
  THDFX_IOCTL_WRITE_PCI_REG,
  THDFX_IOCTL_READ_PCI_REG,
	MAXIOCTL_THDFX
};

//----------------- SST chip 3D layout -------------------------
// registers are in groups of 8 for easy decode
typedef struct vertex_Rec {
    unsigned long x;            // 12.4 format
    unsigned long y;            // 12.4
} vtxRec;

typedef volatile struct sstregs {       // THE 3D CHIP
                                        // EXTERNAL registers
    uint32 status;               // chip status, Read Only
    uint32 intrCtrl;             // interrupt control 
    vtxRec vA;                  // Vertex A,B,C
    vtxRec vB;
    vtxRec vC;

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
    vtxRec FvA;                 // floating point version
    vtxRec FvB;
    vtxRec FvC;

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
    unsigned long reservedB;

    unsigned long stipple;              // 32 bits, MSB masks pixels
    unsigned long c0;                   // 8.8.8.8 (ARGB)
    unsigned long c1;                   // 8.8.8.8 (ARGB)
    struct {                            // statistic gathering variables
        unsigned int fbiPixelsIn;
        unsigned int fbiChromaFail;
        unsigned int fbiZfuncFail;
        unsigned int fbiAfuncFail;
        unsigned int fbiPixelsOut;
    } stats;

    unsigned long fogTable[32];         // 64 entries, 2 per word, 2 bytes each

    uint32 cmdFifoBase;                  // beginning of CMDFIFO area
    uint32 cmdFifoBump;                  // number of words to bump (write only)
    uint32 cmdFifoReadPtr;               // the read (execute) pointer
    uint32 cmdFifoAmin;                  // points to first hole
    uint32 cmdFifoAmax;                  // highest address written (not visible)
    uint32 cmdFifoDepth;                 // number of valid sequential words
    uint32 cmdFifoHoles;                 // number of current holes
    unsigned long reservedC;

    unsigned long fbiInit4;
    unsigned long vRetrace;
    unsigned long backPorch;
    unsigned long videoDimensions;
    unsigned long fbiInit0;
    unsigned long fbiInit1;
    unsigned long fbiInit2;
    unsigned long fbiInit3;

    unsigned long hSync;
    unsigned long vSync;
    unsigned long clutData;
    unsigned long dacData;
    unsigned long videoFilterRgbThreshold;
    uint32 hBorder;
    uint32 vBorder;
    uint32 borderColor;

    uint32 hvRetrace;
    uint32 fbiInit5;
    uint32 fbiInit6;
    uint32 fbiInit7;
    unsigned long reservedD[2];
    uint32 fbiSwapHistory;
    uint32 fbiTrianglesOut;              // triangles out counter

    uint32 sSetupMode;
    uint32 sVx;
    uint32 sVy;
    uint32 sARGB;
    uint32 sRed;
    uint32 sGreen;
    uint32 sBlue;
    uint32 sAlpha;

    uint32 sVz;
    uint32 sOowfbi;
    uint32 sOow0;
    uint32 sSow0;
    uint32 sTow0;
    uint32 sOow1;
    uint32 sSow1;
    uint32 sTow1;

    uint32 sDrawTriCMD;
    uint32 sBeginTriCMD;
    unsigned long reservedE[6];

    uint32 bltSrcBaseAddr;
    uint32 bltDstBaseAddr;
    uint32 bltXYstrides;
    uint32 bltSrcChromaRange;
    uint32 bltDstChromaRange;
    uint32 bltClipX;
    uint32 bltClipY;
    uint32 reservedF;

    uint32 bltSrcXY;
    uint32 bltDstXY;
    uint32 bltSize;
    uint32 bltRop;
    uint32 bltColor;
    uint32 reservedG;
    uint32 bltCommand;
    uint32 bltData;

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
    uint32 tchromaKey;                   // texture chromakey
    uint32 tchromaRange;                 // texture chromarange
    uint32 reservedH[5];
    uint32 reservedI[8];
    uint32 reservedJ[8];
    uint32 reservedK[8];
} SstRegs;

#define BIT(n)  (1UL<<(n))
#define SST_TMU(sst,n)  ((SstRegs *)((0x800<<(n))+(int32)(sst)))
#define SST_TREX(sst,n) SST_TMU(sst,n)

// Additional PCI Configuration Registers
#define SST1_PCI_INIT_ENABLE 0x40
#define SST1_PCI_BUS_SNOOP0	 0x44
#define SST1_PCI_BUS_SNOOP1	 0x48

//----------------- SST initEnable (Config register) bits -----
#define SST_INITWR_EN                   BIT(0)
#define SST_PCI_FIFOWR_EN               BIT(1)
#define SST_FBIINIT23_REMAP             BIT(2)
#define SST_SNOOP0_EN                   BIT(4)
#define SST_SNOOP0_IO                   BIT(5)
#define SST_SNOOP0_RD                   BIT(6)
#define SST_SNOOP1_EN                   BIT(7)
#define SST_SNOOP1_IO                   BIT(8)
#define SST_SNOOP1_RD                   BIT(9)
#define SST_SCANLINE_SLV_OWNPCI         BIT(10)
#define SST_SCANLINE_SLI_SLV            BIT(11)

/*------- SST PCI Configuration Register defaults -----------*/
#define SST_PCI_INIT_ENABLE_DEFAULT        0x0
#define SST_PCI_BUS_SNOOP_DEFAULT          0x0


//----------------- useful addressing macros -----------------------
// return pointer to SST at specified WRAP, CHIP, or TREX
#define SST_WRAP(sst,n) ((SstRegs *)((n)*0x4000+(int32)(sst)))
#define SST_CHIP(sst,n) ((SstRegs *)((n)*0x400+(int32)(sst)))
#define SST_TMU(sst,n)  ((SstRegs *)((0x800<<(n))+(int32)(sst)))
#define SST_TREX(sst,n) SST_TMU(sst,n)

#define SST_BYTESWAP_BIT        BIT(20)
#define SST_ALTMAP_BIT          BIT(21)
#define SST_LFB_BIT             BIT(22)
#define SST_TEX_BIT             BIT(23)
#define SST_LFB_ADDR            SST_LFB_BIT
#define SST_TEX_ADDR            SST_TEX_BIT

// for new H3 code
#define SST_3D_OFFSET	  0
#define SST_LFB_OFFSET	SST_LFB_ADDR
#define SST_TEX_OFFSET  SST_TEX_ADDR

// return byte addresses of LFB and TEX spaces
#define SST_LFB_ADDRESS(sst)    (SST_LFB_BIT+(int32)(sst))
#define SST_TEX_ADDRESS(sst)    (SST_TEX_BIT+(int32)(sst))

//----------------- SST binary point locations ---------------
#define SST_LOD_SIZE     6
#define SST_LOD_FRACBITS 2
#define SST_XY_SIZE     16
#define SST_XY_INTBITS  12
#define SST_XY_FRACBITS  4
#define SST_RGBA_SIZE   24
#define SST_RGBA_INTBITS 8
#define SST_RGBA_FRACBITS 12
#define SST_Z_SIZE      32
#define SST_Z_INTBITS   16
#define SST_Z_FRACBITS  12
// NOTE: below are ONLY for the fixed point registers
#define SST_ST_SIZE     32
#define SST_ST_INTBITS  14
#define SST_ST_FRACBITS 18
#define SST_W_SIZE      32
#define SST_W_INTBITS    2
#define SST_W_FRACBITS  30

// Per register definitions

//----------------- SST status bits ---------------------------
#define SST_FIFOLEVEL           0x3F
#define SST_VRETRACE            BIT(6)
#define SST_FBI_BUSY            BIT(7)
#define SST_TMU_BUSY            BIT(8)
#define SST_TREX_BUSY           SST_TMU_BUSY
#define SST_BUSY                BIT(9)
#define SST_DISPLAYED_BUFFER_SHIFT 10
#define SST_DISPLAYED_BUFFER    (0x3<<SST_DISPLAYED_BUFFER_SHIFT)
#define SST_MEMFIFOLEVEL_SHIFT  12
#define SST_MEMFIFOLEVEL        (0xFFFF<<SST_MEMFIFOLEVEL_SHIFT)
#define SST_SWAPBUFPENDING_SHIFT 28
#define SST_SWAPBUFPENDING      (0x7<<SST_SWAPBUFPENDING_SHIFT)

/*----------------- SST fbiinit0 bits -----------------------*/
//#define SST_FBIINIT0_DEFAULT               0x00000410
// Must include SST_EN_TEX_MEMFIFO and SST_EN_LFB_MEMFIFO in FBIINIT0_DEFAULT
// or else texture memory detection will hang on some machines (see bug_3.c)
#define SST_FBIINIT0_DEFAULT               (0x00000410 | SST_EN_TEX_MEMFIFO | \
                                            SST_EN_LFB_MEMFIFO)
#define SST_GRX_RESET                      BIT(1)
#define SST_PCI_FIFO_RESET                 BIT(2)
#define SST_EN_ENDIAN_SWAPPING             BIT(3)

#define SST_EN_VGA_PASSTHRU     BIT(0)
#define SST_GRX_RESET           BIT(1)
#define SST_PCI_FIFO_RESET      BIT(2)
#define SST_EN_ENDIAN_SWAPPING  BIT(3)
#define SST_PCI_FIFO_LWM_SHIFT  6
#define SST_PCI_FIFO_LWM        (0x1F<<SST_PCI_FIFO_LWM_SHIFT)
#define SST_EN_LFB_MEMFIFO      BIT(11)
#define SST_EN_TEX_MEMFIFO      BIT(12)
#define SST_MEM_FIFO_EN         BIT(13)
#define SST_MEM_FIFO_HWM_SHIFT  14
#define SST_MEM_FIFO_HWM        (0x7FF<<SST_MEM_FIFO_HWM_SHIFT)
#define SST_MEM_FIFO_BURST_HWM_SHIFT    25
#define SST_MEM_FIFO_BURST_HWM  (0x3F<<SST_MEM_FIFO_BURST_HWM_SHIFT)

/*----------------- SST fbiinit1 bits -----------------------*/
#define SST_FBIINIT1_DEFAULT               0x00201102
#define SST_VIDEO_TILES_MASK               0x010000F0
#define SST_VIDEO_TILES_IN_X_MSB_SHIFT     24
#define SST_VIDEO_TILES_IN_X_MSB           (1<<SST_VIDEO_TILES_IN_X_MSB_SHIFT)

#define SST_PCI_WRWS_0                  0x0
#define SST_PCI_WRWS_1                  BIT(1)
#define SST_LFB_READ_EN                 BIT(3)
#define SST_VIDEO_TILES_IN_X_SHIFT      4
#define SST_VIDEO_TILES_IN_X            (0xF<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_448         (7<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_512         (8<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_640         (10<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_832         (13<<SST_VIDEO_TILES_IN_X_SHIFT)
#       define SST_VIDEO_TILES_IN_X_800 SST_VIDEO_TILES_IN_X_832
#define SST_VIDEO_RESET                 BIT(8)
#define SST_VIDEO_RUN                   0
#define SST_VIDEO_STOP                  BIT(8)
#define SST_HVSYNC_OVERRIDE             BIT(9)
#define SST_HSYNC_OVERRIDE_VAL          BIT(10)
#define SST_VSYNC_OVERRIDE_VAL          BIT(11)
#define SST_VIDEO_BLANK_EN              BIT(12)
#define SST_VIDEO_DATA_OE_EN            BIT(13)
#define SST_VIDEO_BLANK_OE_EN           BIT(14)
#define SST_VIDEO_HVSYNC_OE_EN          BIT(15)
#define SST_VIDEO_DCLK_OE_EN            BIT(16)
#define SST_VIDEO_VID_CLK_SEL           BIT(17)
#define SST_VIDEO_VID_CLK_2X            0x0
#define SST_VIDEO_VID_CLK_SLAVE         BIT(17)
#define SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT 18
#define SST_VIDEO_VCLK_2X_INPUT_DEL     (0x3<<SST_VIDEO_VCLK_2X_INPUT_DEL_SHIFT)
#define SST_VIDEO_VCLK_SEL_SHIFT        20
#       define SST_VIDEO_VCLK_SEL               (0x3<<SST_VIDEO_VCLK_SEL_SHIFT)
#       define SST_VIDEO_VCLK_DIV2              (0x1<<SST_VIDEO_VCLK_SEL_SHIFT)
#define SST_VIDEO_24BPP_EN              BIT(22)
#define SST_EN_SCANLINE_INTERLEAVE      BIT(23)
#define SST_VIDEO_FILTER_EN             BIT(25)
#define SST_VIDEO_INVERT_VID_CLK_2X     BIT(26)
#define SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT 27
#       define SST_VIDEO_VCLK_2X_OUTPUT_DEL     (0x3<<SST_VIDEO_VCLK_2X_OUTPUT_DEL_SHIFT)
#define SST_VIDEO_VCLK_DEL_SHIFT        29
#       define SST_VIDEO_VCLK_DEL               (0x3<<SST_VIDEO_VCLK_DEL_SHIFT)
#define SST_DISEN_RD_AHEAD_WR_RD        BIT(31)
#define SST_VIDEO_MASK                  0x7e7ffe00

/*----------------- SST fbiinit2 bits -----------------------*/
#define SST_FBIINIT2_DEFAULT               0x80000040
#define SST_SWAP_ALGORITHM_SHIFT           9
#define SST_SWAP_ALGORITHM                 (0x3<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_VSYNC              (0<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_DACDATA0           (1<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_FIFOSTALL          (2<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_SLISYNC            (3<<SST_SWAP_ALGORITHM_SHIFT)
#define SST_DRAM_REFRESH_16MS              (0x30 << SST_DRAM_REFRESH_CNTR_SHIFT)

#define SST_VIDEO_DITHER_SUB_EN         BIT(0)
#define SST_DRAM_BANKING_CONFIG         BIT(1)
#define SST_EN_FAST_RAS_READ            BIT(5)
#define SST_EN_OE_GEN                   BIT(6)
#define SST_EN_FAST_RD_AHEAD_WR         BIT(7)
#define SST_EN_DITHER_PASSTHRU          BIT(8)
#define SST_SWAP_ALGORITHM_SHIFT        9
#define SST_SWAP_ALGORITHM              (0x3<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_VSYNC                   (0<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_DACDATA0                (1<<SST_SWAP_ALGORITHM_SHIFT)
#       define SST_SWAP_FIFOSTALL               (2<<SST_SWAP_ALGORITHM_SHIFT)
#define SST_VIDEO_BUFFER_OFFSET_SHIFT   11
#define SST_VIDEO_BUFFER_OFFSET         (0x1FF<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_448      (77<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_512      (96<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_640      (150<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_832      (247<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#       define SST_VIDEO_BUFFER_OFFSET_1024     (384<<SST_VIDEO_BUFFER_OFFSET_SHIFT)
#define SST_VIDEO_BUFFER_OFFSET_800     SST_VIDEO_BUFFER_OFFSET_832
#define SST_EN_DRAM_BANKED              BIT(20)
#define SST_EN_DRAM_RD_AHEAD_FIFO       BIT(21)
#define SST_EN_DRAM_REFRESH             BIT(22)
#define SST_DRAM_REFRESH_CNTR_SHIFT     23
#       define SST_DRAM_REFRESH_CNTR            (0x1FF<<SST_DRAM_REFRESH_CNTR_SHIFT)
#define SST_DRAM_REFRESH_16MS           (0x30 << SST_DRAM_REFRESH_CNTR_SHIFT)

/*----------------- SST fbiinit3 bits -----------------------*/
#define SST_TEXMAP_DISABLE                 BIT(6)
#define SST_FBI_MEM_TYPE_SHIFT             8
#define SST_FBI_MEM_TYPE                   (0x7<<SST_FBI_MEM_TYPE_SHIFT)
#define SST_FBI_VGA_PASS_POWERON           BIT(12)
#define SST_FT_CLK_DEL_ADJ_SHIFT           13
#define SST_FT_CLK_DEL_ADJ                 (0xF<<SST_FT_CLK_DEL_ADJ_SHIFT)
#define SST_TF_FIFO_THRESH_SHIFT           17
#define SST_TF_FIFO_THRESH                 (0x1F<<SST_TF_FIFO_THRESH_SHIFT)
#define SST_FBIINIT3_DEFAULT               (0x001E4000|SST_TEXMAP_DISABLE)

#define SST_ALT_REGMAPPING              BIT(0)
#define SST_VIDEO_FIFO_THRESH_SHIFT     1
#       define SST_VIDEO_FIFO_THRESH            (0x1F<<SST_VIDEO_FIFO_THRESH_SHIFT)
#define SST_YORIGIN_TOP_SHIFT           22
#       define SST_YORIGIN_TOP                  (0x3FF<<SST_YORIGIN_TOP_SHIFT)
#define SST_YORIGIN_SUBVAL_SHIFT        SST_YORIGIN_TOP_SHIFT
#define SST_YORIGIN_SUBVAL                      (0x3FF<<SST_YORIGIN_SUBVAL_SHIFT)

/*----------------- SST fbiinit4 bits -----------------------*/
#define SST_FBIINIT4_DEFAULT               0x00000001
#define SST_PCI_RDWS_1                     0x0
#define SST_PCI_RDWS_2                     BIT(0)
#define SST_EN_LFB_RDAHEAD                 BIT(1)
#define SST_MEM_FIFO_LWM_SHIFT             2
#define SST_MEM_FIFO_LWM                   (0x3F<<SST_MEM_FIFO_LWM_SHIFT)
#define SST_MEM_FIFO_ROW_BASE_SHIFT        8
#define SST_MEM_FIFO_ROW_BASE              (0x3FF<<SST_MEM_FIFO_ROW_BASE_SHIFT)
#define SST_MEM_FIFO_ROW_ROLL_SHIFT        18
#define SST_MEM_FIFO_ROW_ROLL              (0x3FF<<SST_MEM_FIFO_ROW_ROLL_SHIFT)

/*----------------- SST fbiinit5 bits -----------------------*/
#define SST_DAC_24BPP_PORT                 BIT(2)
#define SST_GPIO_0                         BIT(3)
#define SST_GPIO_0_DRIVE0                  0x0
#define SST_GPIO_0_DRIVE1                  BIT(3)
#define SST_GPIO_0_SHIFT                   3
#define SST_GPIO_1                         BIT(4)
#define SST_GPIO_1_DRIVE0                  0x0
#define SST_GPIO_1_DRIVE1                  BIT(4)
#define SST_GPIO_1_SHIFT                   4
#define SST_BUFFER_ALLOC_SHIFT             9
#define SST_BUFFER_ALLOC                   (0x3 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_2C0Z       (0x0 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_2C1Z       (0x0 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_3C0Z       (0x1 << SST_BUFFER_ALLOC_SHIFT)
#       define SST_BUFFER_ALLOC_3C1Z       (0x2 << SST_BUFFER_ALLOC_SHIFT)
#define SST_VIDEO_CLK_SLAVE_OE_EN          BIT(11)
#define SST_VID_CLK_2X_OUT_OE_EN           BIT(12)
#define SST_VID_CLK_DAC_DATA16_SEL         BIT(13)
#define SST_SLI_DETECT                     BIT(14)
#define SST_HVRETRACE_SYNC_READS           BIT(15)
#define SST_COLOR_BORDER_RIGHT_EN          BIT(16)
#define SST_COLOR_BORDER_LEFT_EN           BIT(17)
#define SST_COLOR_BORDER_BOTTOM_EN         BIT(18)
#define SST_COLOR_BORDER_TOP_EN            BIT(19)
#define SST_SCAN_DOUBLE_HORIZ              BIT(20)
#define SST_SCAN_DOUBLE_VERT               BIT(21)
#define SST_GAMMA_CORRECT_16BPP_EN         BIT(22)
#define SST_INVERT_HSYNC                   BIT(23)
#define SST_INVERT_VSYNC                   BIT(24)
#define SST_VIDEO_OUT_24BPP_EN             BIT(25)
#define SST_GPIO_1_SEL                     BIT(27)
#define SST_FBIINIT5_DEFAULT \
  (SST_HVRETRACE_SYNC_READS | \
   SST_GAMMA_CORRECT_16BPP_EN | \
   SST_GPIO_1_SEL)
 
/*----------------- SST fbiinit6 bits -----------------------*/
#define SST_SLI_SWAP_VACTIVE_SHIFT         0
#define SST_SLI_SWAP_VACTIVE               (0x7<<SST_SLI_SWAP_VACTIVE_SHIFT)
#define SST_SLI_SWAP_VACTIVE_DRAG_SHIFT    3
#define SST_SLI_SWAP_VACTIVE_DRAG        (0x1F<<SST_SLI_SWAP_VACTIVE_DRAG_SHIFT)
#define SST_SLI_SYNC_MASTER                BIT(8)
#define SST_GPIO_2                         (0x3<<9)
#define SST_GPIO_2_DRIVE0                  (0x2<<9)
#define SST_GPIO_2_DRIVE1                  (0x3<<9)
#define SST_GPIO_2_FLOAT                   (0x1<<9)
#define SST_GPIO_2_SHIFT                   9
#define SST_GPIO_3                         (0x3<<11)
#define SST_GPIO_3_DRIVE0                  (0x2<<11)
#define SST_GPIO_3_DRIVE1                  (0x3<<11)
#define SST_GPIO_3_FLOAT                   (0x1<<11)
#define SST_GPIO_3_SHIFT                   11
#define SST_SLI_SYNCIN                     (0x3<<13)
#define SST_SLI_SYNCIN_DRIVE0              (0x2<<13)
#define SST_SLI_SYNCIN_DRIVE1              (0x3<<13)
#define SST_SLI_SYNCIN_FLOAT               (0x1<<13)
#define SST_SLI_SYNCOUT                    (0x3<<15)
#define SST_SLI_SYNCOUT_DRIVE0             (0x2<<15)
#define SST_SLI_SYNCOUT_DRIVE1             (0x3<<15)
#define SST_SLI_SYNCOUT_FLOAT              (0x1<<15)
#define SST_DAC_RD                         (0x3<<17)
#define SST_DAC_RD_DRIVE0                  (0x2<<17)
#define SST_DAC_RD_DRIVE1                  (0x3<<17)
#define SST_DAC_RD_FLOAT                   (0x1<<17)
#define SST_DAC_WR                         (0x3<<19)
#define SST_DAC_WR_DRIVE0                  (0x2<<19)
#define SST_DAC_WR_DRIVE1                  (0x3<<19)
#define SST_DAC_WR_FLOAT                   (0x1<<19)
#define SST_PCI_FIFO_LWM_RDY_SHIFT         21
#define SST_PCI_FIFO_LWM_RDY               (0x7f<<SST_PCI_FIFO_LWM_RDY_SHIFT)
#define SST_VGA_PASS_N                     (0x3<<28)
#define SST_VGA_PASS_N_DRIVE0              (0x2<<28)
#define SST_VGA_PASS_N_DRIVE1              (0x3<<28)
#define SST_VIDEO_TILES_IN_X_LSB_SHIFT     30
#define SST_VIDEO_TILES_IN_X_LSB           (1<<SST_VIDEO_TILES_IN_X_LSB_SHIFT)
#define SST_FBIINIT6_DEFAULT               0x0

/*----------------- SST fbiinit7 bits -----------------------*/
#define SST_CMDFIFO_EN                     BIT(8)
#define SST_CMDFIFO_STORE_OFFSCREEN        BIT(9)
#define SST_CMDFIFO_DISABLE_HOLES          BIT(10)
#define SST_CMDFIFO_RDFETCH_THRESH_SHIFT   11
#define SST_CMDFIFO_RDFETCH_THRESH    (0x1FUL<<SST_CMDFIFO_RDFETCH_THRESH_SHIFT)
#define SST_CMDFIFO_SYNC_WRITES            BIT(16)
#define SST_CMDFIFO_SYNC_READS             BIT(17)
#define SST_PCI_PACKER_RESET               BIT(18)
#define SST_TMU_CHROMA_REG_WR_EN           BIT(19)
#define SST_CMDFIFO_PCI_TIMEOUT_SHIFT      20
#define SST_CMDFIFO_PCI_TIMEOUT          (0x7FUL<<SST_CMDFIFO_PCI_TIMEOUT_SHIFT)
#define SST_TEXMEMWR_BURST_EN              BIT(27)
#define SST_FBIINIT7_DEFAULT \
  (SST_TEXMEMWR_BURST_EN | SST_TMU_CHROMA_REG_WR_EN)

//----------------- SST fbzMode bits --------------------------
#define SST_ENRECTCLIP          BIT(0)
#define SST_ENCHROMAKEY         BIT(1)
#define SST_ENSTIPPLE           BIT(2)
#define SST_WBUFFER             BIT(3)
#define SST_ENDEPTHBUFFER       BIT(4)
#       define SST_ZFUNC_LT     BIT(5)
#       define SST_ZFUNC_EQ     BIT(6)
#       define SST_ZFUNC_GT     BIT(7)
#       define SST_ZFUNC_SHIFT  5
#       define SST_ZFUNC        ( 0x7 << SST_ZFUNC_SHIFT )
#define SST_ENDITHER            BIT(8)
#define SST_RGBWRMASK           BIT(9)
#define SST_ZAWRMASK            BIT(10)
#define SST_DITHER2x2           BIT(11)
#define SST_ENSTIPPLEPATTERN    BIT(12)
#define SST_ENALPHAMASK         BIT(13)
#define SST_DRAWBUFFER_SHIFT    14
#define SST_DRAWBUFFER          (0x3<<SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_FRONT            (0<<SST_DRAWBUFFER_SHIFT)
#define SST_DRAWBUFFER_BACK             (1<<SST_DRAWBUFFER_SHIFT)
#define SST_ENZBIAS             BIT(16)
#define SST_YORIGIN             BIT(17)
#define SST_ENALPHABUFFER       BIT(18)
#define SST_ENDITHERSUBTRACT    BIT(19)
#define SST_ABLEND_DITHER_SUB_EN SST_ENDITHERSUBTRACT
#define SST_ENDITHERSUBTRACT    BIT(19)
#define SST_ZCOMPARE_TO_ZACOLOR BIT(20)
#define SST_DEPTH_FLOAT_SEL     BIT(21)

/*----------------- SST trexInit0 bits -----------------------*/
#define SST_EN_TEX_MEM_REFRESH             BIT(0)
#define SST_TEX_MEM_REFRESH_SHIFT          1
#define SST_TEX_MEM_REFRESH                (0x1FF<<SST_TEX_MEM_REFRESH_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_SHIFT        10
#define SST_TEX_MEM_PAGE_SIZE_8BITS        (0x0<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_9BITS        (0x1<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_PAGE_SIZE_10BITS       (0x2<<SST_TEX_MEM_PAGE_SIZE_SHIFT)
#define SST_TEX_MEM_SECOND_RAS_BIT_SHIFT   12
#define SST_TEX_MEM_SECOND_RAS_BIT_BIT17   (0x0<<SST_TEX_MEM_SECOND_RAS_BIT_SHIFT)
#define SST_TEX_MEM_SECOND_RAS_BIT_BIT18   (0x1<<SST_TEX_MEM_SECOND_RAS_BIT_SHIFT)
#define SST_EN_TEX_MEM_SECOND_RAS          BIT(14)
#define SST_TEX_MEM_TYPE_SHIFT             15
#define SST_TEX_MEM_TYPE_EDO               (0x0<<SST_TEX_MEM_TYPE_SHIFT)
#define SST_TEX_MEM_TYPE_SYNC              (0x1<<SST_TEX_MEM_TYPE_SHIFT)
#define SST_TEX_MEM_DATA_SIZE_16BIT        0x0
#define SST_TEX_MEM_DATA_SIZE_8BIT         BIT(18)
#define SST_TEX_MEM_DO_EXTRA_CAS           BIT(19)
#define SST_TEX_MEM2                       BIT(20)

#define SST_TREXINIT0_DEFAULT \
  ( (SST_EN_TEX_MEM_REFRESH)  \
  | (0x020 << SST_TEX_MEM_REFRESH_SHIFT) \
  | (SST_TEX_MEM_PAGE_SIZE_9BITS) \
  | (SST_TEX_MEM_SECOND_RAS_BIT_BIT18) \
  | (SST_EN_TEX_MEM_SECOND_RAS) \
  | (SST_TEX_MEM_TYPE_EDO) \
  | (SST_TEX_MEM_DATA_SIZE_16BIT) \
  | (0 & SST_TEX_MEM_DO_EXTRA_CAS) \
  | (0 & SST_TEX_MEM2)  )

#define SST_TREX0INIT0_DEFAULT             SST_TREXINIT0_DEFAULT
#define SST_TREX1INIT0_DEFAULT             SST_TREXINIT0_DEFAULT
#define SST_TREX2INIT0_DEFAULT             SST_TREXINIT0_DEFAULT

/*----------------- SST trexInit1 bits -----------------------*/
#define SST_TEX_SCANLINE_INTERLEAVE_MASTER 0x0
#define SST_TEX_SCANLINE_INTERLEAVE_SLAVE  BIT(0)
#define SST_EN_TEX_SCANLINE_INTERLEAVE     BIT(1)
#define SST_TEX_FT_FIFO_SIL_SHIFT          2
#define SST_TEX_FT_FIFO_SIL                (0x1F<<SST_TEX_FT_FIFO_SIL_SHIFT)
#define SST_TEX_TT_FIFO_SIL_SHIFT          7
#define SST_TEX_TT_FIFO_SIL                (0xF<<SST_TEX_TT_FIFO_SIL_SHIFT)
#define SST_TEX_TF_CLK_DEL_ADJ_SHIFT       12
#define SST_TEX_TF_CLK_DEL_ADJ             (0xF<<SST_TEX_TF_CLK_DEL_ADJ_SHIFT)
#define SST_TEX_RG_TTCII_INH               BIT(16)
#define SST_TEX_USE_RG_TTCII_INH           BIT(17)
#define SST_TEX_SEND_CONFIG                BIT(18)
#define SST_TEX_RESET_FIFO                 BIT(19)
#define SST_TEX_RESET_GRX                  BIT(20)
#define SST_TEX_PALETTE_DEL_SHIFT          21
#define SST_TEX_PALETTE_DEL                (0x3<<SST_TEX_PALETTE_DEL_SHIFT)
#define SST_TEX_SEND_CONFIG_SEL_SHIFT      23
#define SST_TEX_SEND_CONFIG_SEL            (0x7<<SST_TEX_SEND_CONFIG_SEL_SHIFT)

#define SST_TREXINIT1_DEFAULT \
  ( (SST_TEX_SCANLINE_INTERLEAVE_MASTER) \
  | (0 & SST_EN_TEX_SCANLINE_INTERLEAVE) \
  | (0x8 << SST_TEX_FT_FIFO_SIL_SHIFT) \
  | (0x8 << SST_TEX_TT_FIFO_SIL_SHIFT) \
  | (0xf << SST_TEX_TF_CLK_DEL_ADJ_SHIFT) \
  | (0 & SST_TEX_RG_TTCII_INH) \
  | (0 & SST_TEX_USE_RG_TTCII_INH) \
  | (0 & SST_TEX_SEND_CONFIG) \
  | (0 & SST_TEX_RESET_FIFO) \
  | (0 & SST_TEX_RESET_GRX) \
  | (0 << SST_TEX_PALETTE_DEL_SHIFT) \
  | (0 << SST_TEX_SEND_CONFIG_SEL_SHIFT) )

#define SST_TREX0INIT1_DEFAULT              SST_TREXINIT1_DEFAULT
#define SST_TREX1INIT1_DEFAULT              SST_TREXINIT1_DEFAULT
#define SST_TREX2INIT1_DEFAULT              SST_TREXINIT1_DEFAULT

//----------------- SST dacData bits --------------------------
#define SST_DACDATA_DATA                (0xFF)
#define SST_DACDATA_ADDR_SHIFT          8
#define SST_DACDATA_ADDR                (0x7<<SST_DACDATA_ADDR_SHIFT)
#define SST_DACDATA_RD                  BIT(11)
#define SST_DACDATA_WR                  0x0

/*----------------- SST video setup shifts ------------------*/
#define SST_VIDEO_HSYNC_OFF_SHIFT          16
#define SST_VIDEO_HSYNC_ON_SHIFT           0
#define SST_VIDEO_VSYNC_OFF_SHIFT          16
#define SST_VIDEO_VSYNC_ON_SHIFT           0
#define SST_VIDEO_HBACKPORCH_SHIFT         0
#define SST_VIDEO_VBACKPORCH_SHIFT         16
#define SST_VIDEO_XDIM_SHIFT               0
#define SST_VIDEO_YDIM_SHIFT               16

//----------------- SST textureMode bits -----------------------
#define SST_TPERSP_ST           BIT(0)
#define SST_TMINFILTER          BIT(1)
#define SST_TMAGFILTER          BIT(2)
#define SST_TCLAMPW             BIT(3)
#define SST_TLODDITHER          BIT(4)
#define SST_TNCCSELECT          BIT(5)  // selects which table
#define SST_TCLAMPS             BIT(6)
#define SST_TCLAMPT             BIT(7)
#define SST_T8BIT(mode) ((mode&SST_TFORMAT)<SST_ARGB8332)
#define SST_TFORMAT_SHIFT       8
#define SST_TFORMAT             (0xF<<SST_TFORMAT_SHIFT)
#       define SST_RGB332               (0<<SST_TFORMAT_SHIFT)
#       define SST_YIQ422               (1<<SST_TFORMAT_SHIFT)
#       define SST_A8                   (2<<SST_TFORMAT_SHIFT)
#       define SST_I8                   (3<<SST_TFORMAT_SHIFT)
#       define SST_AI44                 (4<<SST_TFORMAT_SHIFT)
#       define SST_P8                   (5<<SST_TFORMAT_SHIFT)
#       define SST_P8_ARGB6666          (6<<SST_TFORMAT_SHIFT)
#       define SST_ARGB8332             (8<<SST_TFORMAT_SHIFT)
#       define SST_AYIQ8422             (9<<SST_TFORMAT_SHIFT)
#       define SST_RGB565               (10<<SST_TFORMAT_SHIFT)
#       define SST_ARGB1555             (11<<SST_TFORMAT_SHIFT)
#       define SST_ARGB4444             (12<<SST_TFORMAT_SHIFT)
#       define SST_AI88                 (13<<SST_TFORMAT_SHIFT)
#       define SST_AP88                 (14<<SST_TFORMAT_SHIFT)
#define SST_TC_ZERO_OTHER       BIT(12)
#define SST_TC_SUB_CLOCAL       BIT(13)
#define SST_TC_MSELECT_SHIFT    14
#define SST_TC_MSELECT          (0x7<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MONE              (0<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MCLOCAL           (1<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MAOTHER           (2<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MALOCAL           (3<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MLOD              (4<<SST_TC_MSELECT_SHIFT)
#       define SST_TC_MLODFRAC          (5<<SST_TC_MSELECT_SHIFT)
#define SST_TC_REVERSE_BLEND    BIT(17)
#define SST_TC_ADD_CLOCAL       BIT(18)
#define SST_TC_ADD_ALOCAL       BIT(19)
#define SST_TC_INVERT_OUTPUT    BIT(20)
#define SST_TCA_ZERO_OTHER      BIT(21)
#define SST_TCA_SUB_CLOCAL      BIT(22)
#define SST_TCA_MSELECT_SHIFT   23
#define SST_TCA_MSELECT         (0x7<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MONE             (0<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MCLOCAL          (1<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MAOTHER          (2<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MALOCAL          (3<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MLOD             (4<<SST_TCA_MSELECT_SHIFT)
#       define SST_TCA_MLODFRAC         (5<<SST_TCA_MSELECT_SHIFT)
#define SST_TCA_REVERSE_BLEND   BIT(26)
#define SST_TCA_ADD_CLOCAL      BIT(27)
#define SST_TCA_ADD_ALOCAL      BIT(28)
#define SST_TCA_INVERT_OUTPUT   BIT(29)
#define SST_TRILINEAR           BIT(30)
#define SST_SEQ_8_DOWNLD        BIT(31)

// here are some abstract constants that most people will be using
// TC stands for Texture Combine (RGB channels)
// TCA stands for Texture Combine Alpha (just the Alpha channel)
// BLEND_ATT blends on the Alpha channel of the color passed from TMU to TMU
// BLEND_ALOCAL blends on the Alpha channel of the local texture color
// NOTE: and of the BLEND modes can be modified using SST_TC_REVERSE_BLEND
//       and/or SST_TCA_REVERSE_BLEND, these modifiers reverse the "polarity"
//       of the blend
#define SST_TC_MZERO  (SST_TC_MONE  | SST_TC_REVERSE_BLEND)
#define SST_TCA_MZERO (SST_TCA_MONE | SST_TCA_REVERSE_BLEND)

#define SST_TCOMBINE_SHIFT      12       // RGB COMBINE MODES
#define SST_TCOMBINE            (0x1FF<<SST_TCOMBINE_SHIFT)
#       define SST_TC_REPLACE    (SST_TC_ZERO_OTHER | SST_TC_ADD_CLOCAL)
#       define SST_TC_PASS       (SST_TC_MONE)
#       define SST_TC_ADD        (SST_TC_MONE  | SST_TC_ADD_CLOCAL)
#       define SST_TC_SUB        (SST_TC_SUB_CLOCAL | SST_TC_MONE)
#       define SST_TC_MULT       (SST_TC_MCLOCAL | SST_TC_REVERSE_BLEND)
#       define SST_TC_ZERO       (SST_TC_MZERO)
#       define SST_TC_ONE        (SST_TC_MZERO | SST_TC_INVERT_OUTPUT)
#       define SST_TC_BLEND      (SST_TC_SUB_CLOCAL | SST_TC_ADD_CLOCAL)
#       define SST_TC_BLEND_LOD         (SST_TC_BLEND | SST_TC_MLOD)
#       define SST_TC_BLEND_LODFRAC     (SST_TC_BLEND | SST_TC_MLODFRAC)
#       define SST_TC_BLEND_ATT         (SST_TC_BLEND | SST_TC_MAOTHER)
#       define SST_TC_BLEND_ALOCAL      (SST_TC_BLEND | SST_TC_MALOCAL)
#define SST_TACOMBINE_SHIFT     21      // ALPHA COMBINE MODES
#define SST_TACOMBINE           (0x1FF<<SST_TACOMBINE_SHIFT)
#       define SST_TCA_REPLACE   (SST_TCA_ZERO_OTHER | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_PASS      (SST_TCA_MONE)
#       define SST_TCA_ADD       (SST_TCA_MONE  | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_SUB       (SST_TCA_SUB_CLOCAL | SST_TCA_MONE)
#       define SST_TCA_MULT      (SST_TCA_MCLOCAL | SST_TCA_REVERSE_BLEND)
#       define SST_TCA_ONE       (SST_TCA_MZERO | SST_TCA_INVERT_OUTPUT)
#       define SST_TCA_ZERO      (SST_TCA_MZERO)
#       define SST_TCA_BLEND     (SST_TCA_SUB_CLOCAL | SST_TCA_ADD_CLOCAL)
#       define SST_TCA_BLEND_LOD        (SST_TCA_BLEND | SST_TCA_MLOD)
#       define SST_TCA_BLEND_LODFRAC    (SST_TCA_BLEND | SST_TCA_MLODFRAC)
#       define SST_TCA_BLEND_ATT        (SST_TCA_BLEND | SST_TCA_MAOTHER)
#       define SST_TCA_BLEND_ALOCAL     (SST_TCA_BLEND | SST_TCA_MALOCAL)
#define SST_TMU_ACTIVE(texMode) \
        ((texMode & (SST_TCOMBINE|SST_TACOMBINE)) != (SST_TC_PASS|SST_TCA_PASS))
#define SST_TREX_ACTIVE(texMode) SST_TMU_ACTIVE(texMode)

//----------------- SST fbzColorPath bits ---------------------------
#define SST_RGBSELECT_SHIFT     0
#define SST_RGBSELECT           (0x3<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_RGBA          (0<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_TMUOUT        (1<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_TREXOUT       SST_RGBSEL_TMUOUT
#       define SST_RGBSEL_C1            (2<<SST_RGBSELECT_SHIFT)
#       define SST_RGBSEL_LFB           (3<<SST_RGBSELECT_SHIFT)
#define SST_ASELECT_SHIFT       2
#define SST_ASELECT             (0x3<<SST_ASELECT_SHIFT)
#       define SST_ASEL_RGBA            (0<<SST_ASELECT_SHIFT)
#       define SST_ASEL_TMUOUT          (1<<SST_ASELECT_SHIFT)
#       define SST_ASEL_TREXOUT         SST_ASEL_TMUOUT
#       define SST_ASEL_C1              (2<<SST_ASELECT_SHIFT)
#       define SST_ASEL_LFB             (3<<SST_ASELECT_SHIFT)
#define SST_LOCALSELECT_SHIFT   4
#define SST_LOCALSELECT         BIT(4)
#define SST_ALOCALSELECT_SHIFT  5
#define SST_ALOCALSELECT        (0x3<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_ITERATOR             (0<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_C0                   (1<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_Z                    (2<<SST_ALOCALSELECT_SHIFT)
#define SST_ALOCAL_W                    (3<<SST_ALOCALSELECT_SHIFT)
#define SST_LOCALSELECT_OVERRIDE_WITH_ATEX      BIT(7)
#define SST_CC_ZERO_OTHER       BIT(8)
#define SST_CC_SUB_CLOCAL       BIT(9)
#define SST_CC_MSELECT_SHIFT    10
#define SST_CC_MSELECT          (0x7<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MONE              (0<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MCLOCAL           (1<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MAOTHER           (2<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MALOCAL           (3<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MATMU             (4<<SST_CC_MSELECT_SHIFT)
#       define SST_CC_MATREX            SST_CC_MATMU
#       define SST_CC_MRGBTMU           (5<<SST_CC_MSELECT_SHIFT)
#define SST_CC_REVERSE_BLEND    BIT(13)
#define SST_CC_ADD_CLOCAL       BIT(14)
#define SST_CC_ADD_ALOCAL       BIT(15)
#define SST_CC_INVERT_OUTPUT    BIT(16)
#define SST_CCA_ZERO_OTHER      BIT(17)
#define SST_CCA_SUB_CLOCAL      BIT(18)
#define SST_CCA_MSELECT_SHIFT   19
#define SST_CCA_MSELECT         (0x7<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MONE             (0<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MCLOCAL          (1<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MAOTHER          (2<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MALOCAL          (3<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MATMU            (4<<SST_CCA_MSELECT_SHIFT)
#       define SST_CCA_MATREX           SST_CCA_MATMU
#define SST_CCA_REVERSE_BLEND   BIT(22)
#define SST_CCA_ADD_CLOCAL      BIT(23)
#define SST_CCA_ADD_ALOCAL      BIT(24)
#define SST_CCA_INVERT_OUTPUT   BIT(25)
#define SST_PARMADJUST          BIT(26)
#define SST_ENTEXTUREMAP        BIT(27)
#define SST_RGBAZ_CLAMP         BIT(28)
#define SST_ENANTIALIAS         BIT(29)

// here are some abstract constants that most people will be using
// CC stands for Color Combine (RGB channels)
// CCA stands for Color Combine Alpha (just the Alpha channel)
// BLEND_ATT blends on the Alpha channel of the color passed from TMU to TMU
// BLEND_ALOCAL blends on the Alpha channel of the local texture color
// NOTE: and of the BLEND modes can be modified using SST_CC_REVERSE_BLEND
//       and/or SST_CCA_REVERSE_BLEND, these modifiers reverse the "polarity"
//       of the blend
#define SST_CC_MZERO  (SST_CC_MONE  | SST_CC_REVERSE_BLEND)
#define SST_CCA_MZERO (SST_CCA_MONE | SST_CCA_REVERSE_BLEND)

#define SST_CCOMBINE_SHIFT      8       // RGB COMBINE MODES
#define SST_CCOMBINE            (0x1FF<<SST_CCOMBINE_SHIFT)
#       define SST_CC_REPLACE    (SST_CC_ZERO_OTHER | SST_CC_ADD_CLOCAL)
#       define SST_CC_PASS       (SST_CC_MONE)
#       define SST_CC_ADD        (SST_CC_MONE  | SST_CC_ADD_CLOCAL)
#       define SST_CC_SUB        (SST_CC_SUB_CLOCAL | SST_CC_MONE)
#       define SST_CC_MULT       (SST_CC_MCLOCAL | SST_CC_REVERSE_BLEND)
#       define SST_CC_ZERO       (SST_CC_MZERO)
#       define SST_CC_ONE        (SST_CC_MZERO | SST_CC_INVERT_OUTPUT)
#       define SST_CC_BLEND      (SST_CC_SUB_CLOCAL | SST_CC_ADD_CLOCAL)
#       define SST_CC_BLEND_ATT         (SST_CC_BLEND | SST_CC_MAOTHER)
#       define SST_CC_BLEND_ATEX        (SST_CC_BLEND | SST_CC_MATMU)
#       define SST_CC_BLEND_ATEX_REV    (SST_CC_BLEND | SST_CC_MATMU | SST_CC_REVERSE_BLEND)
#       define SST_CC_BLEND_ATT_REV     (SST_CC_BLEND | SST_CC_MAOTHER | SST_CC_REVERSE_BLEND)
#       define SST_CC_BLEND_ALOCAL      (SST_CC_BLEND | SST_CC_MALOCAL)
#define SST_CACOMBINE_SHIFT     17      // ALPHA COMBINE MODES
#define SST_CACOMBINE           (0x1FF<<SST_CACOMBINE_SHIFT)
#       define SST_CCA_REPLACE   (SST_CCA_ZERO_OTHER | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_PASS      (SST_CCA_MONE)
#       define SST_CCA_ADD       (SST_CCA_MONE  | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_SUB       (SST_CCA_SUB_CLOCAL | SST_CCA_MONE)
#       define SST_CCA_MULT      (SST_CCA_MCLOCAL | SST_CCA_REVERSE_BLEND)
#       define SST_CCA_ONE       (SST_CCA_MZERO | SST_CCA_INVERT_OUTPUT)
#       define SST_CCA_ZERO      (SST_CCA_MZERO)
#       define SST_CCA_BLEND     (SST_CCA_SUB_CLOCAL | SST_CCA_ADD_CLOCAL)
#       define SST_CCA_BLEND_ATT        (SST_CCA_BLEND | SST_CCA_MAOTHER)
#       define SST_CCA_BLEND_ATEX       (SST_CCA_BLEND | SST_CCA_MATMU)
#       define SST_CCA_BLEND_ATEX_REV   (SST_CCA_BLEND | SST_CCA_MATMU | SST_CCA_REVERSE_BLEN)
#       define SST_CCA_BLEND_ATT_REV    (SST_CCA_BLEND | SST_CCA_MAOTHER | SST_CCA_REVERSE_BLEND)
#       define SST_CCA_BLEND_ALOCAL     (SST_CCA_BLEND | SST_CCA_MALOCAL)

//----------------- SST lfbMode bits --------------------------
#define SST_LFB_FORMAT_SHIFT    0
#define SST_LFB_FORMAT          (0xF<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_565              (0<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_555              (1<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_1555             (2<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u1               (3<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_888              (4<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_8888             (5<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u2               (6<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_u3               (7<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z565             (12<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z555             (13<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_Z1555            (14<<SST_LFB_FORMAT_SHIFT)
#       define SST_LFB_ZZ               (15<<SST_LFB_FORMAT_SHIFT)
#define SST_LFB_WRITEBUFSELECT_SHIFT    4
#define SST_LFB_WRITEBUFSELECT  (0x3<<SST_LFB_WRITEBUFSELECT_SHIFT)
#       define SST_LFB_WRITEFRONTBUFFER (0<<SST_LFB_WRITEBUFSELECT_SHIFT)
#       define SST_LFB_WRITEBACKBUFFER  (1<<SST_LFB_WRITEBUFSELECT_SHIFT)
#define SST_LFB_READBUFSELECT_SHIFT     6
#define SST_LFB_READBUFSELECT   (0x3<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READFRONTBUFFER  (0<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READBACKBUFFER   (1<<SST_LFB_READBUFSELECT_SHIFT)
#       define SST_LFB_READDEPTHABUFFER (2<<SST_LFB_READBUFSELECT_SHIFT)
#   define SST_LFB_READAUXBUFFER    SST_LFB_READDEPTHABUFFER
#define SST_LFB_ENPIXPIPE       BIT(8)
#define SST_LFB_RGBALANES_SHIFT         9
#define SST_LFB_RGBALANES       (0x3<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_ARGB (0<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_ABGR (1<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_RGBA (2<<SST_LFB_RGBALANES_SHIFT)
#       define SST_LFB_RGBALANES_BGRA (3<<SST_LFB_RGBALANES_SHIFT)
#define SST_LFB_WRITE_SWAP16    BIT(11)
#define SST_LFB_WRITE_BYTESWAP  BIT(12)
#define SST_LFB_YORIGIN         BIT(13)
#define SST_LFB_WSELECT         BIT(14)
#define SST_LFB_READ_SWAP16     BIT(15)
#define SST_LFB_READ_BYTESWAP   BIT(16)

#define SST_LFB_ADDR_STRIDE     1024
#define SST_LFB_ADDR_X_SHIFT    0
#define SST_LFB_ADDR_X          (0x3FF<<SST_LFB_ADDR_X_SHIFT)
#define SST_LFB_ADDR_Y_SHIFT    10
#define SST_LFB_ADDR_Y          (0x3FF<<SST_LFB_ADDR_Y_SHIFT)

/*----------------- SST dacData constants -------------------*/
#define SST_DACREG_WMA                     0x0
#define SST_DACREG_LUT                     0x1
#define SST_DACREG_RMR                     0x2
#define SST_DACREG_RMA                     0x3
#define SST_DACREG_ICS_PLLADDR_WR          0x4 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_RD          0x7 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_DATA        0x5 /* ICS only */
#define SST_DACREG_ICS_CMD                 0x6 /* ICS only */
#define SST_DACREG_ICS_COLORMODE_16BPP     0x50 /* ICS only */
#define SST_DACREG_ICS_COLORMODE_24BPP     0x70 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK0       0x0 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK1       0x1 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK7       0x7 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK1_DEFAULT 0x55 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_VCLK7_DEFAULT 0x71 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK0       0xa /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK1       0xb /* ICS only */
#define SST_DACREG_ICS_PLLADDR_GCLK1_DEFAULT 0x79 /* ICS only */
#define SST_DACREG_ICS_PLLADDR_CTRL        0xe /* ICS only */
#define SST_DACREG_ICS_PLLCTRL_CLK1SEL     BIT(4)
#define SST_DACREG_ICS_PLLCTRL_CLK0SEL     BIT(5)
#define SST_DACREG_ICS_PLLCTRL_CLK0FREQ    0x7
#define SST_DACREG_INDEXADDR               SST_DACREG_WMA
#define SST_DACREG_INDEXDATA               SST_DACREG_RMR
#define SST_DACREG_INDEX_RMR               0x0
#define SST_DACREG_INDEX_CR0               0x1
#define SST_DACREG_INDEX_MIR               0x2
#define SST_DACREG_INDEX_MIR_ATT_DEFAULT   0x84   /* AT&T */
#define SST_DACREG_INDEX_MIR_TI_DEFAULT    0x97   /* TI */
#define SST_DACREG_INDEX_DIR               0x3
#define SST_DACREG_INDEX_DIR_ATT_DEFAULT   0x9    /* AT&T */
#define SST_DACREG_INDEX_DIR_TI_DEFAULT    0x9    /* TI */
#define SST_DACREG_INDEX_TST               0x4
#define SST_DACREG_INDEX_CR1               0x5
#define SST_DACREG_INDEX_CC                0x6
#define SST_DACREG_INDEX_AA0               0xff  /* can't access */
#define SST_DACREG_INDEX_AA1               0xff  /* can't access */
#define SST_DACREG_INDEX_AB0               0xff  /* can't access */
#define SST_DACREG_INDEX_AB1               0xff  /* can't access */
#define SST_DACREG_INDEX_AB2               0xff  /* can't access */
#define SST_DACREG_INDEX_AC0               0x48
#define SST_DACREG_INDEX_AC1               0x49
#define SST_DACREG_INDEX_AC2               0x4a
#define SST_DACREG_INDEX_AD0               0x4c
#define SST_DACREG_INDEX_AD1               0x4d
#define SST_DACREG_INDEX_AD2               0x4e
#define SST_DACREG_INDEX_BA0               0xff  /* can't access */
#define SST_DACREG_INDEX_BA1               0xff  /* can't access */
#define SST_DACREG_INDEX_BB0               0xff  /* can't access */
#define SST_DACREG_INDEX_BB1               0xff  /* can't access */
#define SST_DACREG_INDEX_BB2               0xff  /* can't access */
#define SST_DACREG_INDEX_BC0               0xff  /* can't access */
#define SST_DACREG_INDEX_BC1               0xff  /* can't access */
#define SST_DACREG_INDEX_BC2               0xff  /* can't access */
#define SST_DACREG_INDEX_BD0               0x6c
#define SST_DACREG_INDEX_BD1               0x6d
#define SST_DACREG_INDEX_BD2               0x6e

#define SST_DACREG_CR0_INDEXED_ADDRESSING  BIT(0)
#define SST_DACREG_CR0_8BITDAC             BIT(1)
#define SST_DACREG_CR0_SLEEP               BIT(3)
#define SST_DACREG_CR0_COLOR_MODE_SHIFT    4
#define SST_DACREG_CR0_COLOR_MODE         (0xF<<SST_DACREG_CR0_COLOR_MODE_SHIFT)
#define SST_DACREG_CR0_COLOR_MODE_16BPP   (0x3<<SST_DACREG_CR0_COLOR_MODE_SHIFT)
#define SST_DACREG_CR0_COLOR_MODE_24BPP   (0x5<<SST_DACREG_CR0_COLOR_MODE_SHIFT)

#define SST_DACREG_CR1_BLANK_PEDASTAL_EN  BIT(4)

#define SST_DACREG_CC_BCLK_SEL_SHIFT       0
#define SST_DACREG_CC_BCLK_SELECT_BD       BIT(3)
#define SST_DACREG_CC_ACLK_SEL_SHIFT       4
#define SST_DACREG_CC_ACLK_SELECT_AD       BIT(7)

#define SST_DACREG_CLKREG_MSHIFT           0
#define SST_DACREG_CLKREG_PSHIFT           6
#define SST_DACREG_CLKREG_NSHIFT           0
#define SST_DACREG_CLKREG_LSHIFT           4
#define SST_DACREG_CLKREG_IBSHIFT          0

#define SST_FBI_DACTYPE_ATT                0
#define SST_FBI_DACTYPE_ICS                1
#define SST_FBI_DACTYPE_TI                 2


#endif 

