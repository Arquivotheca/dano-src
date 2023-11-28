/* :ts=8 bk=0
 *
 * i810.h:	Bit and structure definitions for Intel 810.
 *
 * Leo L. Schwab					1999.10.25
 *
 * Copyright 1999 Be Incorporated.
 */
#ifndef	__I810DEFS_H
#define	__I810DEFS_H

#ifndef	__BITTWIDDLE_H
#include "bittwiddle.h"
#endif


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
	vuint8	__reserved6[0x0004];	/*	0x03D6 - 0x03D9		*/
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
	vuint8	__reserved9[0x0004];	/*	0x03D6 - 0x03D9		*/
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
 * Ring buffer, for DMAd command sequences.
 */
typedef struct RingBuffer {
	vuint32	RINGBUF0;	/*  Tail offset and control	*/
	vuint32	RINGBUF1;	/*  Head offset			*/
	vuint32	RINGBUF2;	/*  Base address		*/
	vuint32	RINGBUF3;	/*  Buffer size			*/
} RingBuffer;


/*
 * HW status page layout.
 */
typedef struct HWStatusPage {
	vuint32	IRQStatus;			/*  0x00000 - 0x00003	*/
	vuint32	LPRingHead;			/*  0x00004 - 0x00007	*/
	vuint32	IRQRingHead;			/*  0x00008 - 0x0000B	*/
	vuint32	_IntelReserved;			/*  0x0000C - 0x0000F	*/
	/*  Defined by driver.  */
	vuint32	__reserved[0x03F7];		/*  0x00010 - 0x00FEB	*/
	vuint32	ExecutionFlags[4];		/*  0x00FEC - 0x00FFB	*/
	vuint32	PrimitiveCookie;		/*  0x00FFC - 0x00FFF   */
} HWStatusPage;


/*
 * Video overlay registers (set up in memory, then read via DMA).
 */
typedef struct OverlayRegs {
	vuint32		OBUF_0Y;		/*  0x00000 - 0x00003	*/
	vuint32		OBUF_1Y;		/*  0x00000 - 0x00003	*/
	vuint32		OBUF_0U;		/*  0x00000 - 0x00003	*/
	vuint32		OBUF_0V;		/*  0x00000 - 0x00003	*/
	vuint32		OBUF_1U;		/*  0x00010 - 0x00013	*/
	vuint32		OBUF_1V;		/*  0x00014 - 0x00017	*/
	vuint32		OV0STRIDE;		/*  0x00018 - 0x0001B	*/
	vuint32		YRGB_VPH;		/*  0x0001C - 0x0001F	*/
	vuint32		UV_VPH;			/*  0x00020 - 0x00023	*/
	vuint32		HORZ_PH;		/*  0x00024 - 0x00027	*/
	vuint32		INIT_PH;		/*  0x00028 - 0x0002B	*/
	vuint32		DWINPOS;		/*  0x0002C - 0x0002F	*/
	vuint32		DWINSZ;			/*  0x00030 - 0x00033	*/
	vuint32		SWID;			/*  0x00034 - 0x00037	*/
	vuint32		SWIDQW;			/*  0x00038 - 0x0003B	*/
	vuint32		SHEIGHT;		/*  0x0003C - 0x0003F	*/
	vuint32		YRGBSCALE;		/*  0x00040 - 0x00043	*/
	vuint32		UVSCALE;		/*  0x00044 - 0x00047	*/
	vuint32		OV0CLRC0;		/*  0x00048 - 0x0004B	*/
	vuint32		OV0CLRC1;		/*  0x0004C - 0x0004F	*/
	vuint32		DCLRKV;			/*  0x00050 - 0x00053	*/
	vuint32		DCLRKM;			/*  0x00054 - 0x00057	*/
	vuint32		SCLRKVH;		/*  0x00058 - 0x0005B	*/
	vuint32		SCLRKVL;		/*  0x0005C - 0x0005F	*/
	vuint32		SCLRKM;			/*  0x00060 - 0x00063	*/
	vuint32		OV0CONF;		/*  0x00064 - 0x00067	*/
	vuint32		OV0CMD;			/*  0x00068 - 0x0006B	*/
	vuint32		__reserved0;		/*  0x0006C - 0x0006F	*/
	vuint32		AWINPOS;		/*  0x00070 - 0x00073	*/
	vuint32		AWINZ;			/*  0x00074 - 0x00077	*/
	vuint32		__reserved1[2];		/*  0x00078 - 0x0007F	*/
} OverlayRegs;



/*
 * The rest of the I810 registers.  I haven't settled on all the names yet.
 */
typedef struct I810Regs {
	union vgaregs	VGARegs;		/*  0x00000 - 0x00FFF	*/

	/*  Instruction and Interrupt Control  */
	vuint32		DMAFIFO[0x0400];	/*  0x01000 - 0x01FFF	*/
	vuint32		FENCE[8];		/*  0x02000 - 0x0201F	*/
	vuint32		PGTBL_CTL;		/*  0x02020 - 0x02023	*/
	vuint32		PGTBL_ERR;		/*  0x02024 - 0x02027	*/
	vuint32		__reserved0[2];		/*  0x02028 - 0x0202F	*/
	RingBuffer	LPRING;			/*  0x02030 - 0x0203F	*/
	RingBuffer	IRQRING;		/*  0x02040 - 0x0204F	*/
	vuint32		__reserved00[12];	/*  0x02050 - 0x0207F	*/
	vuint32		HWS_PGA;		/*  0x02080 - 0x02083	*/
	vuint32		__reserved1;		/*  0x02084 - 0x02087	*/
	vuint32		IPEIR;			/*  0x02088 - 0x0208B	*/
	vuint32		IPEHR;			/*  0x0208C - 0x0208F	*/
	vuint16		INSTDONE;		/*  0x02090 - 0x02091	*/
	vuint16		__reserved2;		/*  0x02092 - 0x02093	*/
	vuint32		NOPID;			/*  0x02094 - 0x02097	*/
	vuint16		HWSTAM;			/*  0x02098 - 0x02099	*/
	vuint16		__reserved3[3];		/*  0x0209A - 0x0209F	*/
	vuint16		IER;			/*  0x020A0 - 0x020A1	*/
	vuint16		__reserved4;		/*  0x020A2 - 0x020A3	*/
	vuint16		IIR;			/*  0x020A4 - 0x020A5	*/
	vuint16		__reserved5;		/*  0x020A6 - 0x020A7	*/
	vuint16		IMR;			/*  0x020A8 - 0x020A9	*/
	vuint16		__reserved6;		/*  0x020AA - 0x020AB	*/
	vuint16		ISR;			/*  0x020AC - 0x020AD	*/
	vuint16		__reserved7;		/*  0x020AE - 0x020AF	*/
	vuint16		EIR;			/*  0x020B0 - 0x020B1	*/
	vuint16		__reserved8;		/*  0x020B2 - 0x020B3	*/
	vuint16		EMR;			/*  0x020B4 - 0x020B5	*/
	vuint16		__reserved9;		/*  0x020B6 - 0x020B7	*/
	vuint16		ESR;			/*  0x020B8 - 0x020B9	*/
	vuint16		__reserved10;		/*  0x020BA - 0x020BB	*/
	vuint32		__reserved11;		/*  0x020BC - 0x020BF	*/
	vuint8		INSTPM;			/*  0x020C0          	*/
	vuint8		__reserved12[3];	/*  0x020C1 - 0x020C3	*/
	vuint32		INSTPS;			/*  0x020C4 - 0x020C7	*/
	vuint32		BBP_PTR;		/*  0x020C8 - 0x020CB	*/
	vuint32		ABB_SRT;		/*  0x020CC - 0x020CF	*/
	vuint32		ABB_END;		/*  0x020D0 - 0x020D3	*/
	vuint32		DMA_FADD;		/*  0x020D4 - 0x020D7	*/
	vuint32		FW_BLC;			/*  0x020D8 - 0x020DB	*/
	vuint32		MEM_MODE;		/*  0x020DC - 0x020DF	*/
	vuint8		__reserved13[0x0F20];	/*  0x020E0 - 0x02FFF	*/

	/*  Memory control registers  */
	vuint8		DRT;			/*  0x03000          	*/
	vuint8		DRAMCL;			/*  0x03001          	*/
	vuint8		DRAMCH;			/*  0x03002          	*/
	vuint8		__reserved14[0x0FFD];	/*  0x03003 - 0x03FFF	*/

	/*  Span cursor registers (?)  */
	vuint32		__reserved15[2];	/*  0x04000 - 0x04007	*/
	vuint32		UI_SC_CTL;		/*  0x04008 - 0x0400B	*/
	vuint8		__reserved16[0x0FF4];	/*  0x0400C - 0x04FFF	*/

	/*  I/O control registers  */
	vuint32		HVSYNC;			/*  0x05000 - 0x05003	*/
	vuint32		__reserved17[3];	/*  0x05004 - 0x0500F	*/
	vuint32		GPIOA;			/*  0x05010 - 0x05013	*/
	vuint32		GPIOB;			/*  0x05014 - 0x05017	*/
	vuint8		__reserved18[0x0FE8];	/*  0x05018 - 0x05FFF	*/

	/*  Clock control and power management registers  */
	vuint32		DCLK_MN0;		/*  0x06000 - 0x06003	*/
	vuint32		DCLK_MN1;		/*  0x06004 - 0x06007	*/
	vuint32		DCLK_MN2;		/*  0x06008 - 0x0600B	*/
	vuint32		DCLK_MNLCD;		/*  0x0600C - 0x0600F	*/
	vuint8		DCLK_P0;		/*  0x06010		*/
	vuint8		DCLK_P1;		/*  0x06011		*/
	vuint8		DCLK_P2;		/*  0x06012		*/
	vuint8		DCLK_PLCD;		/*  0x06013		*/
	vuint32		PWR_CLKC;		/*  0x06014 - 0x06017	*/
	vuint8		__reserved19[0x9FE8];	/*  0x06018 - 0x0FFFF	*/

	/*  Graphics translation table  */
	vuint32		GTT[0x4000];		/*  0x10000 - 0x1FFFF	*/
	vuint32		__reserved20[0x4000];	/*  0x20000 - 0x2FFFF	*/

	/*  Overlay registers  */
	vuint32		OV0ADDR;		/*  0x30000 - 0x30003	*/
	vuint32		__reserved21;		/*  0x30004 - 0x30003	*/
	vuint32		DOV0STA;		/*  0x30008 - 0x30003	*/
	vuint32		__reserved22;		/*  0x3000C - 0x30003	*/
	vuint32		GAMMA[6];		/*  0x30010 - 0x30027	*/
	vuint32		__reserved23[0x0036];	/*  0x30028 - 0x300FF	*/
	OverlayRegs	OV;			/*  0x30100 - 0x3017F	*/
	vuint8		__reserved25[0xFE80];	/*  0x30180 - 0x3FFFF	*/

	/*  BLT engine status.  CPU can't write to these regs.  */
	vuint32		BLT_OpcodeCtl;		/*  0x40000 - 0x40003	*/
	vuint32		BLT_SetupCtl;		/*  0x40000 - 0x40007	*/
	vuint32		BLT_ClipY1;		/*  0x40000 - 0x4000B	*/
	vuint32		BLT_ClipY2;		/*  0x40000 - 0x4000F	*/
	vuint32		BLT_ClipX1X2;		/*  0x40010 - 0x40013	*/
	vuint32		BLT_SetupBGColor;	/*  0x40014 - 0x40017	*/
	vuint32		BLT_SetupFGColor;	/*  0x40018 - 0x4001B	*/
	vuint32		BLT_SetupColorPatAddr;	/*  0x4001C - 0x4001F	*/
	vuint32		BLT_DestX1X2;		/*  0x40020 - 0x40023	*/
	vuint32		BLT_DestY1;		/*  0x40024 - 0x40027	*/
	vuint32		BLT_DestY2;		/*  0x40028 - 0x4002B	*/
	vuint32		BLT_SrcPitchStatus;	/*  0x4002C - 0x4002F	*/
	vuint32		BLT_SrcAddr;		/*  0x40030 - 0x40033	*/
	vuint32		BLT_Ctl;		/*  0x40034 - 0x40037	*/
	vuint32		BLT_DestDims;		/*  0x40038 - 0x4003B	*/
	vuint32		BLT_ColorPatAddr;	/*  0x4003C - 0x4003F	*/
	vuint32		BLT_PatBGColor;		/*  0x40040 - 0x40043	*/
	vuint32		BLT_PatFGColor;		/*  0x40044 - 0x40047	*/
	vuint32		BLT_SrcBGColor;		/*  0x40048 - 0x4004B	*/
	vuint32		BLT_SrcFGColor;		/*  0x4004C - 0x4004F	*/
	vuint32		__reserved26[0x3FEC];	/*  0x40050 - 0x4FFFF	*/
	vuint32		__reserved27[0x4000];	/*  0x50000 - 0x5FFFF	*/

	/*  LCD/TV-out and DVD registers  */
	vuint32		LCD_HTotal;		/*  0x60000 - 0x60003	*/
	vuint32		LCD_HBlank;		/*  0x60004 - 0x60007	*/
	vuint32		LCD_HSync;		/*  0x60008 - 0x6000B	*/
	vuint32		LCD_VTotal;		/*  0x6000C - 0x6000F	*/
	vuint32		LCD_VBlank;		/*  0x60010 - 0x60013	*/
	vuint32		LCD_VSync;		/*  0x60014 - 0x60017	*/
	vuint32		LCD_Ctl;		/*  0x60018 - 0x6001B	*/
	vuint32		LCD_OverlayActive;	/*  0x6001C - 0x6001F	*/
	vuint32		LCD_BorderColor;	/*  0x60020 - 0x60023	*/
	vuint8		__reserved28[0xFFDC];	/*  0x60024 - 0x6FFFF	*/

	/*  Display and cursor control registers  */
	vuint32		DISP_SL;		/*  0x70000 - 0x70003	*/
	vuint32		DISP_SLC;		/*  0x70004 - 0x70007	*/
	vuint32		PIXCONF;		/*  0x70008 - 0x7000B	*/
	vuint32		BLTCTL;			/*  0x7000C - 0x7000F	*/
	vuint32		__reserved29;		/*  0x70010 - 0x70013	*/
	vuint32		SWF[3];			/*  0x70014 - 0x7001F	*/
	vuint32		DISPBASE;		/*  0x70020 - 0x70023	*/
	vuint32		DISPSTAS;		/*  0x70024 - 0x70027	*/
	vuint8		__reserved30[0x0058];	/*  0x70028 - 0x7007F	*/
	vuint32		CURSCTL;		/*  0x70080 - 0x70083	*/
	vuint32		CURSBASE;		/*  0x70084 - 0x70087	*/
	vuint32		CURSPOS;		/*  0x70088 - 0x7008B	*/
	vuint8		__reserved31[0xFF74];	/*  0x7008C - 0x7FFFF	*/
} I810Regs;


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


/*****************************************************************************
 * Graphics controller register indices.
 */
#define	GR_SETRESET				0x00
#define	GR_SETRESET_PLANEMASK			3:0

#define	GR_SETRESETENABLE			0x01
#define	GR_SETRESETENABLE_PLANEENABLEMASK	3:0

#define	GR_COLORCMP				0x02
#define	GR_COLORCMP_PLANEMASK			3:0

#define	GR_ROTATE				0x03
#define	GR_ROTATE_WRITEFUNC			4:3
#define	GR_ROTATE_WRITEFUNC_COPY		0
#define	GR_ROTATE_WRITEFUNC_AND			1
#define	GR_ROTATE_WRITEFUNC_OR			2
#define	GR_ROTATE_WRITEFUNC_XOR			3
#define	GR_ROTATE_COUNT				2:0

#define	GR_READPLANE				0x04
#define	GR_READPLANE_PLANE			1:0
#define	GR_READPLANE_PLANE_0			0
#define	GR_READPLANE_PLANE_1			1
#define	GR_READPLANE_PLANE_2			2
#define	GR_READPLANE_PLANE_3			3

#define	GR_MODE					0x05
#define	GR_MODE_SHIFTCTL			6:5
#define	GR_MODE_SHIFTCTL_16COLOR		0
#define	GR_MODE_SHIFTCTL_4COLOR			1
#define	GR_MODE_SHIFTCTL_256COLOR		2
#define	GR_MODE_ODDEVEN				4:4
#define	GR_MODE_ODDEVEN_SEQUENTIAL		0
#define	GR_MODE_ODDEVEN_ALTERNATE		1
#define	GR_MODE_READMODE			3:3
#define	GR_MODE_READMODE_NORMAL			0
#define	GR_MODE_READMODE_COMPARE		1
#define	GR_MODE_WRITEMODE			1:0

#define	GR_MISC					0x06
#define	GR_MISC_MEMMAP				3:2
#define	GR_MISC_MEMMAP_A0_BF			0
#define	GR_MISC_MEMMAP_A0_AF			1
#define	GR_MISC_MEMMAP_B0_B7			2
#define	GR_MISC_MEMMAP_B8_BF			3
#define	GR_MISC_CHAINODDEVEN			1:1
#define	GR_MISC_CHAINODDEVEN_NORMAL		0
#define	GR_MISC_CHAINODDEVEN_ALTERNATE		1
#define	GR_MISC_DISPMODE			0:0
#define	GR_MISC_DISPMODE_TEXT			0
#define	GR_MISC_DISPMODE_GRAPHICS		1

#define	GR_COLORNC				0x07
#define	GR_COLORNC_PLANEMASK			3:0

#define	GR_LATCHMASK				0x08

/*  I810-specific  */
#define	GR_ADDRMAP				0x10
#define	GR_ADDRMAP__RESERVED			7:5
#define	GR_ADDRMAP_PAGEMODE			4:4
#define	GR_ADDRMAP_PAGEMODE_VGABUF		0
#define	GR_ADDRMAP_PAGEMODE_LOCALMEM		1
#define	GR_ADDRMAP_MAPSEL			3:3
#define	GR_ADDRMAP_MAPSEL_VGABUF		0
#define	GR_ADDRMAP_MAPSEL_MEMMAP		1
#define	GR_ADDRMAP_PACKEDMODE			2:2	/*  ???  */
#define	GR_ADDRMAP_PACKEDMODE_BUSED		0
#define	GR_ADDRMAP_PACKEDMODE_EXTENDED		1
#define	GR_ADDRMAP_LINEARMAP			1:1
#define	GR_ADDRMAP_LINEARMAP_DISABLE		0
#define	GR_ADDRMAP_LINEARMAP_ENABLE		1
#define	GR_ADDRMAP_PAGEMAP			0:0
#define	GR_ADDRMAP_PAGEMAP_DISABLE		0
#define	GR_ADDRMAP_PAGEMAP_ENABLE		1

#define	GR_PAGESEL				0x11


/*****************************************************************************
 * Attribute register indices and bit definitions.
 */
#define	AR_IDX_ACCESS				5:5
#define	AR_IDX_ACCESS_UNLOCK			0
#define	AR_IDX_ACCESS_LOCK			1
#define	AR_IDX_IDX				4:0

#define	AR_PALIDX0				0x00
#define	AR_PALIDX1				0x01
#define	AR_PALIDX2				0x02
#define	AR_PALIDX3				0x03
#define	AR_PALIDX4				0x04
#define	AR_PALIDX5				0x05
#define	AR_PALIDX6				0x06
#define	AR_PALIDX7				0x07
#define	AR_PALIDX8				0x08
#define	AR_PALIDX9				0x09
#define	AR_PALIDX10				0x0a
#define	AR_PALIDX11				0x0b
#define	AR_PALIDX12				0x0c
#define	AR_PALIDX13				0x0d
#define	AR_PALIDX14				0x0e
#define	AR_PALIDX15				0x0f

#define	AR_MODE					0x10
#define	AR_MODE_PAL54SRC			7:7
#define	AR_MODE_PAL54SRC_PALETTE		0
#define	AR_MODE_PAL54SRC_COLORSEL		1
#define	AR_MODE_PIXWIDTH			6:6
#define	AR_MODE_PIXWIDTH_4			0
#define	AR_MODE_PIXWIDTH_8			1
#define	AR_MODE_PANSEL				5:5
#define	AR_MODE_PANSEL_BOTH			0
#define	AR_MODE_PANSEL_UPPER			1
#define	AR_MODE_BLINK				3:3
#define	AR_MODE_BLINK_DISABLE			0
#define	AR_MODE_BLINK_ENABLE			1
#define	AR_MODE_LINEGR				2:2
#define	AR_MODE_LINEGR_DISABLE			0
#define	AR_MODE_LINEGR_ENABLE			1
#define	AR_MODE_DISPTYPE			1:1
#define	AR_MODE_DISPTYPE_COLOR			0
#define	AR_MODE_DISPTYPE_MONO			1
#define	AR_MODE_DISPMODE			0:0
#define	AR_MODE_DISPMODE_TEXT			0
#define	AR_MODE_DISPMODE_GRAPHICS		1

#define	AR_OSCANCOLOR				0x11

#define	AR_PLANEENABLE				0x12
#define	AR_PLANEENABLE_PLANEMASK		3:0

#define	AR_HPAN					0x13
#define	AR_HPAN_PANVAL				3:0

#define	AR_COLORSEL				0x14
#define	AR_COLORSEL_PALIDX_7_6			3:2
#define	AR_COLORSEL_ALTPAL_5_4			1:0


/*****************************************************************************
 * CRT controller register indices and bit definitions.
 */
#define	CR_HTOTAL_7_0				0x00
#define	CR_HDISPEND_7_0				0x01
#define	CR_HBLANKSTART_7_0			0x02

#define	CR_HBLANKEND				0x03
#define	CR_HBLANKEND_SETTOONE			7:7
#define	CR_HBLANKEND_DISPDELAY			6:5
#define	CR_HBLANKEND_HBLANKEND_4_0		4:0

#define	CR_HSYNCSTART_7_0			0x04

#define	CR_HSYNCEND				0x05
#define	CR_HSYNCEND_HBLANKEND_5			7:7
#define	CR_HSYNCEND_SYNCDELAY			6:5
#define	CR_HSYNCEND_HSYNCEND_4_0		4:0

#define	CR_VTOTAL_7_0				0x06

#define	CR_OVERFLOW				0x07
#define	CR_OVERFLOW_VSYNCSTART_9		7:7
#define	CR_OVERFLOW_VDISPEND_9			6:6
#define	CR_OVERFLOW_VTOTAL_9			5:5
#define	CR_OVERFLOW_LINECMP_8			4:4
#define	CR_OVERFLOW_VBLANKSTART_8		3:3
#define	CR_OVERFLOW_VSYNCSTART_8		2:2
#define	CR_OVERFLOW_VDISPEND_8			1:1
#define	CR_OVERFLOW_VTOTAL_8			0:0

#define	CR_FONTROLL				0x08
#define	CR_FONTROLL_BYTEPAN			6:5
#define	CR_FONTROLL_STARTROW			4:0

#define	CR_FONTHEIGHT				0x09
#define	CR_FONTHEIGHT_SCANDOUBLE		7:7
#define	CR_FONTHEIGHT_LINECMP_9			6:6
#define	CR_FONTHEIGHT_VBLANKSTART_9		5:5
#define	CR_FONTHEIGHT_FONTHEIGHT		4:0

#define	CR_TXCURSTOP				0x0a
#define	CR_TXCURSTOP_CURSENABLE			5:5
#define	CR_TXCURSTOP_TOP			4:0

#define	CR_TXCURSBOT				0x0b
#define	CR_TXCURSBOT_CURSDELAY			6:5
#define	CR_TXCURSBOT_BOT			4:0

#define	CR_FBBASE_17_10				0x0c
#define	CR_FBBASE_9_2				0x0d
#define	CR_TXCURSLOC_15_8			0x0e
#define	CR_TXCURSLOC_7_0			0x0f
#define	CR_VSYNCSTART_7_0			0x10

#define	CR_VSYNCEND				0x11
#define	CR_VSYNCEND_PROTECT			7:7
#define	CR_VSYNCEND_VBLDISABLE			5:5
#define	CR_VSYNCEND_CLEARVBL			4:4
#define	CR_VSYNCEND_VSYNCEND_3_0		3:0

#define	CR_VDISPEND_7_0				0x12
#define	CR_PITCH_7_0				0x13

#define	CR_ULLOC				0x14
#define	CR_ULLOC_FETCH32			6:6
#define	CR_ULLOC_COUNT4				5:5
#define	CR_ULLOC_ULLOC				4:0

#define	CR_VBLANKSTART_7_0			0x15
#define	CR_VBLANKEND_7_0			0x16

#define	CR_MODE					0x17
#define	CR_MODE_NOTRESET			7:7
#define	CR_MODE_FETCH8				6:6
#define	CR_MODE_WRAP16K				5:5
#define	CR_MODE_COUNT2				3:3
#define	CR_MODE_TWICEHIGH			2:2
#define	CR_MODE_SELECTROWSCAN			1:1
#define	CR_MODE_COMPAT				0:0

#define	CR_LINECMP_7_0				0x18
#define	CR_READLATCH				0x22

#define	CR_ARTOGGLE				0x24
#define	CR_ARTOGGLE_STATE			7:7
#define	CR_ARTOGGLE_STATE_IDX			0
#define	CR_ARTOGGLE_STATE_DATA			1


/*  I810-specific  */
#define	CR_VTOTAL_11_8				0x30
#define	CR_VDISPEND_11_8			0x31
#define	CR_VSYNCSTART_11_8			0x32
#define	CR_VBLANKSTART_11_8			0x33
#define	CR_HTOTAL_8				0x35
#define	CR_HBLANKEND_6				0x39

#define	CR_FBBASE_23_18				0x40
#define	CR_FBBASE_23_18_LOADADDR		7:7
#define	CR_FBBASE_23_18_FBBASE_23_18		5:0

#define	CR_PITCH_11_8				0x41
#define	CR_FBBASE_31_24				0x42

#define	CR_LACECTL				0x70
#define	CR_LACECTL_ZERO				7:7
#define	CR_LACECTL_HALFLINEVAL			6:0

#define	CR_IOCTL				0x80
#define	CR_IOCTL_AREXTENSIONS			1:1
#define	CR_IOCTL_AREXTENSIONS_DISABLE		0
#define	CR_IOCTL_AREXTENSIONS_ENABLE		1
#define	CR_IOCTL_CREXTENSIONS			0:0
#define	CR_IOCTL_CREXTENSIONS_DISABLE		0
#define	CR_IOCTL_CREXTENSIONS_ENABLE		1

#define	CR_BLINKCTL				0x82
#define	CR_BLINKCTL_DUTYCYCLE			7:6
#define	CR_BLINKCTL_DUTYCYCLE_50PCT		0
#define	CR_BLINKCTL_DUTYCYCLE_25PCT		1
#define	CR_BLINKCTL_RATE			5:0


/*****************************************************************************
 * Other register bit definitions.
 */
/*  Status Register 0  */
#define	ST00_VBLPENDING				7:7
#define	ST00_MONITORTYPE			4:4
#define	ST00_MONITORTYPE_MONO			0
#define	ST00_MONITORTYPE_COLOR			1

/*  Status Register 1  */
#define	ST01_FEEBACKBITS			5:4
#define	ST01_VBLACTIVE				3:3
#define	ST01_NOTDISPSCANNING			0:0

/*  Feature Control Register  */
#define	FCR_VSYNCCTL				3:3
#define	FCR_VSYNCCTL_NORMAL			0
#define	FCR_VSYNCCTL_VSYNC_OR_DISPENABLE	1

/*  Miscellaneous Output Register  */
#define	MISC_VSYNCPOL				7:7
#define	MISC_VSYNCPOL_POSITIVE			0
#define	MISC_VSYNCPOL_NEGATIVE			1
#define	MISC_HSYNCPOL				6:6
#define	MISC_HSYNCPOL_POSITIVE			0
#define	MISC_HSYNCPOL_NEGATIVE			1
#define	MISC_ODDEVENPAGESEL			5:5
#define	MISC_ODDEVENPAGESEL_0K			0
#define	MISC_ODDEVENPAGESEL_64K			1
#define	MISC_CLKSEL				3:2
#define	MISC_CLKSEL_25MHZ			0
#define	MISC_CLKSEL_28MHZ			1
#define	MISC_CLKSEL_EXTRA			2
#define	MISC_0xA0000WINDOW			1:1
#define	MISC_0xA0000WINDOW_DISABLE		0
#define	MISC_0xA0000WINDOW_ENABLE		1
#define	MISC_IOADDR				0:0
#define	MISC_IOADDR_3Bx				0
#define	MISC_IOADDR_3Dx				1


/*****************************************************************************
 * Interrupt and Control register bit definitions.
 * The Intel document is inconsistent about its own names.  The names chosen
 * are semi-arbitrary.  The comments identify the other names given for the
 * same register in the Intel document (where applicable).
 */
/*  FENCE  */
#define	FENCE_BASEADDR				25:19
#define	FENCE_TILEWALK				12:12
#define	FENCE_TILEWALK_XMAJOR			0
#define	FENCE_TILEWALK_YMAJOR			1
#define	FENCE_SIZE				10:8
#define	FENCE_SIZE_512K				0
#define	FENCE_SIZE_1M				1
#define	FENCE_SIZE_2M				2
#define	FENCE_SIZE_4M				3
#define	FENCE_SIZE_8M				4
#define	FENCE_SIZE_16M				5
#define	FENCE_SIZE_32M				6
#define	FENCE_LOG2TILEPITCH			6:4	/*  4 == 16 tiles  */
#define	FENCE_ENTRYVALID			0:0

/*  PGTBL_CTL  (Thanks, Prashant!)  */
#define	PGTBL_CTL_GTTBASE			31:12
#define	PGTBL_CTL_VALID				0:0

/*  RINGBUF registers  */
#define	RINGBUF0_TAILOFFSET			20:3

#define	RINGBUF1_WRAPCOUNT			31:21
#define	RINGBUF1_HEADOFFSET			20:2
#define	RINGBUF1_HEADOFFSET64			20:3	/*  64-bit offset  */
							/*ewhac's convenience*/

#define	RINGBUF2_BUFBASE			25:12	/*  GFX-relative  */

#define	RINGBUF3_BUFSIZE			20:12	/*  4K pages - 1  */
#define	RINGBUF3_AUTOREPORTHEAD			2:1
#define	RINGBUF3_AUTOREPORTHEAD_DISABLE		0
#define	RINGBUF3_AUTOREPORTHEAD_16PAGES		1
#define	RINGBUF3_AUTOREPORTHEAD_32PAGES		2
#define	RINGBUF3_BUFFER				0:0
#define	RINGBUF3_BUFFER_DISABLE			0
#define	RINGBUF3_BUFFER_ENABLE			1

/*  IPEIR  */
#define	IPEIR_ERRSRC				2:2
#define	IPEIR_ERRSRC_BATCHBUF			0
#define	IPEIR_ERRSRC_RING			1
#define	IPEIR_RINGID				1:0
#define	IPEIR_RINGID_LPRING			0
#define	IPEIR_RINGID_IRQRING			1

/*  INSTDONE  */
#define	INSTDONE_BLT				6:6
#define	INSTDONE_MAPPING			5:5
#define	INSTDONE_RENDER				4:4
#define	INSTDONE_BATCH				3:3
#define	INSTDONE_IRQRING			1:1
#define	INSTDONE_LPRING				0:0

/*  INSTPM  */
#define	INSTPM_FLUSHSYNC_AGP			6:6
#define	INSTPM_FLUSHSYNC			5:5
#define	INSTPM_DISABLEMCOMP			4:4
#define	INSTPM_DISABLEBLT			3:3
#define	INSTPM_DISABLERENDER			2:2
#define	INSTPM_DISABLESTATE			1:1
#define	INSTPM_DISABLERENDERPALETTE		0:0

/*  HWSTAM/IER/IIR/IMR/ISR  */
#define	IxR_HWERR				15:15
#define	IxR_SYNCFLUSH				12:12
#define	IxR_DISP0FLIPPENDING			11:11
#define	IxR_OVL0FLIPPENDING			9:9
#define	IxR_VBLANK0				7:7
#define	IxR_SOMETHINGHAPPENED0			6:6
#define	IxR_USER				1:1
#define	IxR_BREAKPOINT				0:0

/*  EIR/EMR/ESR  */
#define	ExR_MMLMREFRESH				5:5
#define	ExR_PGTBL				4:4
#define	ExR_DISPUNDERRUN			3:3
#define	ExR_CAPTUREOVERRUN			2:2
#define	ExR_HOSTPORT				1:1
#define	ExR_INSTPARSER				0:0


/*  DRAMCH  */
#define	DRAMCH_REFRESH				4:3
#define	DRAMCH_REFRESH_DISABLE			0
#define	DRAMCH_REFRESH_ENABLE			1

/*  FW_BLC  */
#define	FW_BLC_OVLDELAY1			31:28
#define	FW_BLC_OVLDELAY0			27:24
#define	FW_BLC_MAINBURST			22:20	/* (n+1) * 8*64 bits */
#define	FW_BLC_MAINLOWATER			17:12	/*  64-bit units  */
#define	FW_BLC_LOCALBURST			10:8
#define	FW_BLC_LOCALWATER			5:0

/*  HVSYNC  */
#define	HVSYNC_VCTL				19:19
#define	HVSYNC_VCTL_NORMAL			0
#define	HVSYNC_VCTL_DATA			1
#define	HVSYNC_VDATA				18:18
#define	HVSYNC_HCTL				17:17
#define	HVSYNC_HCTL_NORMAL			0
#define	HVSYNC_HCTL_DATA			1
#define	HVSYNC_HDATA				16:16

/*  GPIOA  */
#define	GPIOA_INPUT1				12:12
#define	GPIOA_OUTPUT1				11:11
#define	GPIOA_OUTPUT1VALID			10:10
#define	GPIOA_DIR1				9:9
#define	GPIOA_DIR1_INPUT			0
#define	GPIOA_DIR1_OUTPUT			1
#define	GPIOA_DIR1VALID				8:8
#define	GPIOA_INPUT0				4:4
#define	GPIOA_OUTPUT0				3:3
#define	GPIOA_OUTPUT0VALID			2:2
#define	GPIOA_DIR0				1:1
#define	GPIOA_DIR0_INPUT			0
#define	GPIOA_DIR0_OUTPUT			1
#define	GPIOA_DIR0VALID				0:0

/*  GPIOB  */
#define	GPIOB_INPUT3				12:12
#define	GPIOB_OUTPUT3				11:11
#define	GPIOB_OUTPUT3VALID			10:10
#define	GPIOB_DIR3				9:9
#define	GPIOB_DIR3_INPUT			0
#define	GPIOB_DIR3_OUTPUT			1
#define	GPIOB_DIR3VALID				8:8
#define	GPIOB_INPUT2				4:4
#define	GPIOB_OUTPUT2				3:3
#define	GPIOB_OUTPUT2VALID			2:2
#define	GPIOB_DIR2				1:1
#define	GPIOB_DIR2_INPUT			0
#define	GPIOB_DIR2_OUTPUT			1
#define	GPIOB_DIR2VALID				0:0

/*  DCLK_MN{0,1,2,LCD}/DCLK_[012]D/LCD_CLKD (all formatted the same)  */
#define	DCLK_MNx_N				25:16
#define	DCLK_MNx_M				9:0

/* DCLK_P{0,1,2,LCD}/DCLK_0DS (documented as 32 bits, but really four  */
/*  identical 8-bit regs)  */
#define	DCLK_Px_2EXPP				6:4
#define	DCLK_Px_VCODIV				2:2
#define	DCLK_Px_VCODIV_DIV4			0
#define	DCLK_Px_VCODIV_DIV16			1

/*  PWR_CLKC  */
#define	PWR_CLKC_PLL				1:1
#define	PWR_CLKC_PLL_DISABLE			0
#define	PWR_CLKC_PLL_ENABLE			1
#define	PWR_CLKC_DAC				0:0
#define	PWR_CLKC_DAC_DISABLE			0
#define	PWR_CLKC_DAC_ENABLE			1

/*  LCD_Ctl  */
#define	LCD_CTL_ENABLE				31:31
#define	LCD_CTL_CENTERENABLE			29:29
#define	LCD_CTL_CLKMODE				28:28
#define	LCD_CTL_CLKMODE_LCDTV			0
#define	LCD_CTL_CLKMODE_VGA			1
#define	LCD_CTL_DATAORDER			14:14
#define	LCD_CTL_DATAORDER_740			0
#define	LCD_CTL_DATAORDER_PANEL			1
#define	LCD_CTL_LCDDEBUGENABLE			13:13
#define	LCD_CTL_FPVSYNCDISABLE			11:11
#define	LCD_CTL_FPHSYNCDISABLE			10:10
#define	LCD_CTL_FPVSYNCTRISTATE			9:9
#define	LCD_CTL_FPHSYNCTRISTATE			8:8
#define	LCD_CTL_BORDERENABLE			7:7
#define	LCD_CTL_ACTIVEDATAORDER			6:6
#define	LCD_CTL_ACTIVEDATAORDER_NORMAL		0
#define	LCD_CTL_ACTIVEDATAORDER_REVERSED	1
#define	LCD_CTL_ACTIVEDATAPOL			5:5
#define	LCD_CTL_ACTIVEDATAPOL_NORMAL		0
#define	LCD_CTL_ACTIVEDATAPOL_INVERTED		1
#define	LCD_CTL_VSYNCPOL			4:4
#define	LCD_CTL_HSYNCPOL			3:3
#define	LCD_CTL_BLANKPOL			2:2
#define	LCD_CTL_CLKSRC				1:1
#define	LCD_CTL_CLKSRC_INTERNAL			0
#define	LCD_CTL_CLKSRC_EXTERNAL			1
#define	LCD_CTL_LOCKCLK				0:0
#define	LCD_CTL_LOCKCLK_UNLOCKED		0
#define	LCD_CTL_LOCKCLK_USETVLCD		1

/*  PIXCONF  */
#define	PIXCONF_FBGAMMA				27:27
#define	PIXCONF_FBGAMMA_DISABLE			0
#define	PIXCONF_FBGAMMA_ENABLE			1
#define	PIXCONF_OVLGAMMA			26:26
#define	PIXCONF_OVLGAMMA_DISABLE		0
#define	PIXCONF_OVLGAMMA_ENABLE			1
#define	PIXCONF_VGADELAYS			20:20
#define	PIXCONF_VGADELAYS_ENABLE		0	/*  Legacy mode  */
#define	PIXCONF_VGADELAYS_DISABLE		1
#define	PIXCONF_DISPMODE			19:16
#define	PIXCONF_DISPMODE_VGA			0
#define	PIXCONF_DISPMODE_8BPP			2
#define	PIXCONF_DISPMODE_16BPP_555		4
#define	PIXCONF_DISPMODE_16BPP_565		5
#define	PIXCONF_DISPMODE_24BPP			6
#define	PIXCONF_DISPMODE_32BPP			7
#define	PIXCONF_DACWIDTH			15:15
#define	PIXCONF_DACWIDTH_6BPP			0
#define	PIXCONF_DACWIDTH_8BPP			1
#define	PIXCONF_SHOWCURSOR			12:12
#define	PIXCONF_EXTENDEDREAD			11:11
#define	PIXCONF_EXTENDEDREAD_DISABLE		0
#define	PIXCONF_EXTENDEDREAD_ENABLE		1
#define	PIXCONF_OSCANCOLOR			10:10
#define	PIXCONF_OSCANCOLOR_DISABLE		0
#define	PIXCONF_OSCANCOLOR_ENABLE		1
#define	PIXCONF_PALETTEMODE			8:8
#define	PIXCONF_PALETTEMODE_NORMAL		0
#define	PIXCONF_PALETTEMODE_EXTRA8		1
#define	PIXCONF_VGAWRAP				1:1
#define	PIXCONF_VGAWRAP_ENABLE			0
#define	PIXCONF_VGAWRAP_DISABLE			1
#define	PIXCONF_NATIVEMODE			0:0
#define	PIXCONF_NATIVEMODE_DISABLE		0	/*  Legacy mode  */
#define	PIXCONF_NATIVEMODE_ENABLE		1

/*  DRT  */
#define	DRT_LOCALCACHE				0:0
#define	DRT_LOCALCACHE_NONE			0
#define	DRT_LOCALCACHE_4M			1

/*  BLTCTL/BLTCNTL  */
#define	BLTCTL_EXPANDMODE			5:4
#define	BLTCTL_EXPANDMODE_8BPP			0
#define	BLTCTL_EXPANDMODE_16BPP			1
#define	BLTCTL_EXPANDMODE_24BPP			2
#define	BLTCTL_BLTBUSY				0:0	/*  1 == busy  */

/*  DISPSTAS  */
#define	DISPSTAS_REPORTPANELHOTPLUG		26:26
#define	DISPSTAS_REPORTVSYNC			25:25
#define	DISPSTAS_REPORTLINECMP			24:24
#define	DISPSTAS_REPORTVBLANK			17:17
#define	DISPSTAS_REPORTOVLREGUPDATED		16:16
#define	DISPSTAS_PANELHOTPLUG			15:15
#define	DISPSTAS_PANELHOTPLUG_PRESENT		0
#define	DISPSTAS_PANELHOTPLUG_ABSENT		1
#define	DISPSTAS_PANELHOTPLUGCHANGED		10:10
#define	DISPSTAS_VSYNC				9:9
#define	DISPSTAS_LINECMP			8:8
#define	DISPSTAS_VBLANK				1:1
#define	DISPSTAS_OVLREGUPDATED			0:0

/*  CURSCTL  */
#define	CURSCTL_ORIGIN				4:4
#define	CURSCTL_ORIGIN_BORDER			0	/*  Overscan area  */
#define	CURSCTL_ORIGIN_NORMAL			1
#define	CURSCTL_MODE				2:0
#define	CURSCTL_MODE_DISABLED			0
#define	CURSCTL_MODE_32x32x2_ANDXOR		1
#define	CURSCTL_MODE_64x64x2_3TRANS		4
#define	CURSCTL_MODE_64x64x2_ANDXOR		5
#define	CURSCTL_MODE_64x64x2_4COLOR		6

/*  CURSBASE  */
#define	CURSBASE_BASEADDR			31:8

/*  CURSPOS  */
#define	CURSPOS_YPOSSIGN			31:31
#define	CURSPOS_YPOS				26:16
#define	CURSPOS_XPOSSIGN			15:15
#define	CURSPOS_XPOS				10:0


/*  Individual entries for the GTT  */
#define	GTTENTRY_PAGEADDR			29:12
#define	GTTENTRY_PAGELOC			2:1
#define	GTTENTRY_PAGELOC_SYSRAM			0
#define	GTTENTRY_PAGELOC_CACHE			1
#define	GTTENTRY_PAGELOC_SYSRAM_SNOOPED		3
#define	GTTENTRY_VALID				0:0


/*****************************************************************************
 * Overlay register bit definitions.
 */
/*  OBUF_[01][YUV]  */
#define	OVBUF_xx_BUFPTR				25:0

/*  OV0STRIDE  */
#define	OV0STRIDE_PITCHUV			28:16
#define	OV0STRIDE_PITCHY			12:0

/*  YRGB_VPH  */
#define	YRGB_VPH_VPHASE1			31:20
#define	YRGB_VPH_VPHASE0			15:4

/*  UV_VPH  */
#define	UV_VPH_VPHASE1				31:20
#define	UV_VPH_VPHASE0				15:4

/*  HORZ_PH  */
#define	HORZ_PH_PHASE_UV			31:20
#define	HORZ_PH_PHASE_Y				15:4

/*  INIT_PH  */
#define	INIT_PH_INV_VY0				5:5
#define	INIT_PH_INV_VY1				4:4
#define	INIT_PH_INV_HY				3:3
#define	INIT_PH_INV_VUV0			2:2
#define	INIT_PH_INV_VUV1			1:1
#define	INIT_PH_INV_HUV				0:0

/*  DWINPOS  */
#define	DWINPOS_TOP				26:16
#define	DWINPOS_LEFT				10:0

/*  DWINSZ  */
#define	DWINSZ_HEIGHT				26:16
#define	DWINSZ_WIDTH				10:0

/*  SWID  */
#define	SWID_UV					23:16
#define	SWID_Y					8:0

/*  SWIDQW  */
#define	SWIDQW_UV				23:16
#define	SWIDQW_Y				8:0

/*  SHEIGHT  */
#define	SHEIGHT_UV				25:16
#define	SHEIGHT_Y				10:0

/*  YRGBSCALE  */
#define	YRGBSCALE_VFRAC				31:20
#define	YRGBSCALE_H				16:15
#define	YRGBSCALE_HFRAC				14:3
#define	YRGBSCALE_V				1:0

/*  UVSCALE  */
#define	UVSCALE_VFRAC				31:20
#define	UVSCALE_HFRAC				14:3
#define	UVSCALE_V				1:0

/*  OV0CLRC0  */
#define	OV0CLRC0_CONTRAST			16:8
#define	OV0CLRC0_BRIGHTNESS			7:0

/*  OV0CLRC1  */
#define	OV0CLRC1_SATURATION			9:0

/*  DCLRKV  */
#define	DCLRKV_KEYVALUE				23:0

/*  DCLRKM  */
#define	DCLRKM_KEYENABLE			31:31
#define	DCLRKM_DESTALPHABLENDENABLE		30:30
#define	DCLRKM_ALPHABLENDENABLE			29:29
#define	DCLRKM_KEYMASK				23:0

/*  SCLRKVH  */
#define	SCLRKVH_HIGHKEY				23:0

/*  SCLRKVL  */
#define	SCLRKVL_LOWKEY				23:0

/*  SCLRKM  */
#define	SCLRKM_SRCALPHABLENDENABLE		31:31
#define	SCLRKM_MASKENABLE_RED			26:26
#define	SCLRKM_MASKENABLE_GREEN			25:25
#define	SCLRKM_MASKENABLE_BLUE			24:24
#define	SCLRKM_ALPHA_RED			23:16
#define	SCLRKM_ALPHA_GREEN			15:8
#define	SCLRKM_ALPHA_BLUE			7:0

/*  OV0CONF  */
#define	OV0CONF_LINEBUF				0:0
#define	OV0CONF_LINEBUF_2x720			0
#define	OV0CONF_LINEBUF_1x1440			1

/*  OV0CMD  */
#define	OV0CMD_TOPOVERLAY			31:31
#define	OV0CMD_FILTERVCHROMA			30:28
#define	OV0CMD_FILTERVLUMA			27:25
#define	OV0CMD_FILTERHCHROMA			24:22
#define	OV0CMD_FILTERHLUMA			21:19
#define	OV0CMD_MIRROR				18:17
#define	OV0CMD_MIRROR_NONE			0
#define	OV0CMD_MIRROR_H				1
#define	OV0CMD_MIRROR_V				2
#define	OV0CMD_MIRROR_HV			3
#define	OV0CMD_YADJUST				16:16
#define	OV0CMD_YADJUST_FULL			0
#define	OV0CMD_YADJUST_NTSCSAFE			1
#define	OV0CMD_YUV422ORDER			15:14
#define	OV0CMD_YUV422ORDER_NORMAL		0
#define	OV0CMD_YUV422ORDER_UVSWAP		1
#define	OV0CMD_YUV422ORDER_YSWAP		2
#define	OV0CMD_YUV422ORDER_YUVSWAP		3
#define	OV0CMD_SRCFORMAT			13:10
#define	OV0CMD_SRCFORMAT_RGB555			2
#define	OV0CMD_SRCFORMAT_RGB565			3
#define	OV0CMD_SRCFORMAT_YUV422			8
#define	OV0CMD_SRCFORMAT_YUV411			9
#define	OV0CMD_SRCFORMAT_YUV420			12
#define	OV0CMD_SRCFORMAT_YUV410			14
#define	OV0CMD_TVOUTFLIPPOL			9:9
#define	OV0CMD_TVOUTFLIPPOL_0TO1		0
#define	OV0CMD_TVOUTFLIPPOL_1TO0		1
#define	OV0CMD_FLIPQUAL				8:7
#define	OV0CMD_FLIPQUAL_IMMEDIATE		0
#define	OV0CMD_FLIPQUAL_SYNCTVOUTFIELD		1
#define	OV0CMD_FLIPQUAL_SYNCCAPTURE		2
#define	OV0CMD_FLIPQUAL_SYNCCAPTURETVOUTFIELD	3	/*  Reserved  */
#define	OV0CMD_INITVPHASE			6:6
#define	OV0CMD_INITVPHASE_USE0			0
#define	OV0CMD_INITVPHASE_USEBOTH		1
#define	OV0CMD_FLIPTYPE				5:5
#define	OV0CMD_FLIPTYPE_FRAME			0
#define	OV0CMD_FLIPTYPE_FIELD			1
#define	OV0CMD_IGNOREBUFFERFIELD		4:4
#define	OV0CMD_BUFFERFIELDSEL			2:1
#define	OV0CMD_BUFFERFIELDSEL_B0F0		0
#define	OV0CMD_BUFFERFIELDSEL_B0F1		1
#define	OV0CMD_BUFFERFIELDSEL_B1F0		2
#define	OV0CMD_BUFFERFIELDSEL_B1F1		3
#define	OV0CMD_ENABLE				0:0

#define	OVFILT_OFF				0
#define	OVFILT_MAG_NEAREST			1
#define	OVFILT_MAG_LINEAR			2
#define	OVFILT_MIN_NEAREST			5
#define	OVFILT_MIN_LINEAR			6

/*  AWINPOS  */
#define	AWINPOS_TOP				26:16
#define	AWINPOS_LEFT				10:0

/*  AWINZ  */
#define	AWINZ_HEIGHT				26:16
#define	AWINZ_WIDTH				10:0

/*  LCD_OverlayActive  */
#define	OVERLAYACTIVE_END			26:16
#define	OVERLAYACTIVE_START			11:0


/*****************************************************************************
 * Blitter register bit definitions.
 */
/*  BLT_OpcodeCtl/BR00  */
#define	BLT_OPCODECTL_BLTBUSY			31:31	/*  1 == busy  */
#define	BLT_OPCODECTL_CLIPPING			30:30
#define	BLT_OPCODECTL_CLIPPING_DISABLE		0
#define	BLT_OPCODECTL_CLIPPING_ENABLE		1
#define	BLT_OPCODECTL_PATTERNTYPE		29:29
#define	BLT_OPCODECTL_PATTERNTYPE_MONO		0
#define	BLT_OPCODECTL_PATTERNTYPE_COLOR		1
#define	BLT_OPCODECTL_OPCODE			28:22
#define	BLT_OPCODECTL_MONOSRCSTART		19:17
#define	BLT_OPCODECTL_PACKING			16:16
#define	BLT_OPCODECTL_PACKING_BITWISE		0
#define	BLT_OPCODECTL_PACKING_BYTEWISE		1
#define	BLT_OPCODECTL_TEXTBLT			13:13
#define	BLT_OPCODECTL_SCANLINEBLT		12:12
#define	BLT_OPCODECTL_PIXELBLT			11:11
#define	BLT_OPCODECTL_TRANSMODE			10:8
#define	BLT_OPCODECTL_TRANSMODE_NONE		0
#define	BLT_OPCODECTL_TRANSMODE_SRC_NE_BG	1	/*  Src trans  */
#define	BLT_OPCODECTL_TRANSMODE_DEST_NE_BG	3
#define	BLT_OPCODECTL_TRANSMODE_SRC_EQ_BG	5
#define	BLT_OPCODECTL_TRANSMODE_DEST_EQ_BG	7	/*  Dest trans  */
#define	BLT_OPCODECTL_PATALIGN			7:5
#define	BLT_OPCODECTL_DESTRMW			4:4
#define	BLT_OPCODECTL_SRCISCOLOR		3:3
#define	BLT_OPCODECTL_SRCISMONO			2:2
#define	BLT_OPCODECTL_PATISCOLOR		1:1
#define	BLT_OPCODECTL_PATISMONO			0:0

/*  BLT_SetupCtl/BR01 (also BLT_Ctl/BR13)  */
#define	BLT_CTL_PATSRC				31:31
#define	BLT_CTL_PATSRC_NORMAL			0
#define	BLT_CTL_PATSRC_ALLZERO			1
#define	BLT_CTL_BLTDIR				30:30	/*  Not valid in  */
#define	BLT_CTL_BLTDIR_XINC			0	/*  _SetupCtl  */
#define	BLT_CTL_BLTDIR_XDEC			1
#define	BLT_CTL_MONOSRC				29:29
#define	BLT_CTL_MONOSRC_NORMAL			0
#define	BLT_CTL_MONOSRC_TRANSPARENT		1
#define	BLT_CTL_MONOPAT				28:28
#define	BLT_CTL_MONOPAT_NORMAL			0
#define	BLT_CTL_MONOPAT_TRANSPARENT		1
#define	BLT_CTL_BLTSRC				27:27
#define	BLT_CTL_BLTSRC_FB			0
#define	BLT_CTL_BLTSRC_INLINE			1	/*  "CPU blit"  */
#define	BLT_CTL_DYNCOLOR			26:26
#define	BLT_CTL_DYNCOLOR_USEGLOBAL		0
#define	BLT_CTL_DYNCOLOR_USELOCAL		1
#define	BLT_CTL_LOCALCOLORDEPTH			25:24
#define	BLT_CTL_LOCALCOLORDEPTH_8BPP		0
#define	BLT_CTL_LOCALCOLORDEPTH_16BPP		1
#define	BLT_CTL_LOCALCOLORDEPTH_24BPP		2
#define	BLT_CTL_ROP				23:16
#define	BLT_CTL_DESTPITCH			15:0

/*  BLT_ClipY1/BR02  */
/*  BLT_ClipY2/BR03  */
/*  BLT_ClipX1X2/BR04  */
#define	BLT_CLIPX1X2_X2				27:16
#define	BLT_CLIPX1X2_X1				11:0

/*  BLT_SetupBGColor/BR05  */
/*  BLT_SetupFGColor/BR06  */
/*  BLT_SetupColorPatAddr/BR07 (also BLT_ColorPatAddr/BR15)  */
#define	BLT_COLORPATADDR_ADDR_25_6		25:6

/*  BLT_DestX1X2/BR08  */
#define	BLT_DESTX1X2_X2				27:16
#define	BLT_DESTX1X2_X1				11:0

/*  BLT_DestY1/BR09  */
/*  BLT_DestY2/BR10  */
/*  BLT_SrcPitchStatus/BR11  */
#define	BLT_SRCPITCHSTATUS_STATUS		31:16
#define	BLT_SRCPITCHSTATUS_PITCH		15:0

/*  BLT_SrcAddr/BR12  */
/*  BLT_DestDims/BR14  */
#define	BLT_DESTDIMS_HEIGHT			28:16
#define	BLT_DESTDIMS_BYTEWIDTH			12:0

/*  BLT_ColorPatAddr/BR15  */
/*  BLT_PatBGColor/BR16  */
/*  BLT_PatFGColor/BR17  */
/*  BLT_SrcBGColor/BR18  */
/*  BLT_SrcFGColor/BR19  */



/*****************************************************************************
 * PCI configuration offsets.
 */
#define	PCI_I810_GMCHCFG			0x50
#define	PCI_I810_GMCHCFG_DEFERCPU		6:6
#define	PCI_I810_GMCHCFG_DEFERCPU_31CLOCKS	0
#define	PCI_I810_GMCHCFG_DEFERCPU_IMMEDIATE	1
#define	PCI_I810_GMCHCFG_FSBFREQ		4:4
#define	PCI_I810_GMCHCFG_FSBFREQ_100MHZ		0
#define	PCI_I810_GMCHCFG_FSBFREQ_133MHZ		1
#define	PCI_I810_GMCHCFG_DRAMPRECHARGE		3:3
#define	PCI_I810_GMCHCFG_DRAMPRECHARGE_BANK	0
#define	PCI_I810_GMCHCFG_DRAMPRECHARGE_ALL	1
#define	PCI_I810_GMCHCFG_D8HOLE			1:1
#define	PCI_I810_GMCHCFG_D8HOLE_DISABLE		0
#define	PCI_I810_GMCHCFG_D8HOLE_ENABLE		1
#define	PCI_I810_GMCHCFG_CDHOLE			0:0	/*  Really 0xDC000  */
#define	PCI_I810_GMCHCFG_CDHOLE_DISABLE		0
#define	PCI_I810_GMCHCFG_CDHOLE_ENABLE		1

#define	PCI_I810_DRAMPOP			0x52
#define	PCI_I810_DRAMPOP_DIMM1			7:4
#define	PCI_I810_DRAMPOP_DIMM0			3:0

#define	PCI_I815_DRAMPOP2			0x54
#define	PCI_I815_DRAMPOP2_DIMM2			3:0

#define	PCI_I810_SMRAM				0x70
#define	PCI_I810_SMRAM_GFXMODE			7:6
#define	PCI_I810_SMRAM_GFXMODE_DISABLE		0
#define	PCI_I810_SMRAM_GFXMODE_ENABLE_0K	1
#define	PCI_I810_SMRAM_GFXMODE_ENABLE_512K	2
#define	PCI_I810_SMRAM_GFXMODE_ENABLE_1024K	3
#define	PCI_I810_SMRAM_TSEG			5:4
#define	PCI_I810_SMRAM_TSEG_DISABLE		0
#define	PCI_I810_SMRAM_TSEG_ENABLE_0K		1	/*  Also  */
#define	PCI_I810_SMRAM_TSEG_ENABLE_512K		2	/*  enables  */
#define	PCI_I810_SMRAM_TSEG_ENABLE_1024K	3	/*  HSEG  */
#define	PCI_I810_SMRAM_ABSEG			3:2
#define	PCI_I810_SMRAM_ABSEG_DISABLE		0
#define	PCI_I810_SMRAM_ABSEG_NORMALRAM		1
#define	PCI_I810_SMRAM_ABSEG_SMMCODESHADOW	2
#define	PCI_I810_SMRAM_ABSEG_SMMRAM		3
#define	PCI_I810_SMRAM_LOCKSMM			1:1
#define	PCI_I810_SMRAM_LOCKSMM_UNLOCK		0
#define	PCI_I810_SMRAM_LOCKSMM_LOCK		1
#define	PCI_I810_SMRAM_SMMERR			0:0

#define	PCI_I810_MISCC2				0x80
#define	PCI_I810_MISCC2_SETTOONE		7:7	/*  All these get  */
#define	PCI_I810_MISCC2_TEXTIMMEDIATEBLIT	6:6	/*  set to 1  */
#define	PCI_I810_MISCC2_PALETTELOADSEL		2:2
#define	PCI_I810_MISCC2_INSTPARSERCLOCK		1:1



/*****************************************************************************
 * ROP/MinTerm definitions.
 */
/*
 * Windoze ROP codes are *exactly* analogous to Amiga blitter MinTerms, except
 * that the three sources for ROP codes have fixed definitions:  "Pattern,"
 * Source, and Destination (before write).  Leave it to MS to make ROP codes
 * as difficult as possible to understand what with that Reverse Polish
 * notation in all their books, so read this and be enlightened:
 * 
 * The MinTerm value/ROP code directly specifies the output of three binary
 * inputs (in the Windoze case, Pattern, Source, and Destination).  For three
 * binary sources, there are a total of eight possible input states.  Each of
 * the bits in the MinTerm value itself defines the result of each possible
 * state.  The MinTerm bits are defined as follows:
 *
 *      7       6       5       4       3       2       1       0
 *    P S D   P S D   P S D   P S D   P S D   P S D   P S D   P S D
 *    1 1 1   1 1 0   1 0 1   1 0 0   0 1 1   0 1 0   0 0 1   0 0 0
 *
 * Note that the bit position numbers exactly match the combination of the
 * three input state values.  When the state of the three inputs matches a
 * particular bit position, the value of that bit position in the MinTerm
 * is used as the output value.  So, for a value of 0xCC (the ever-popular
 * ROP_COPY), we get the following truth table:
 *
 * MinTerm value  State
 * bit position   (PSD)   Output
 *        0       (000):  0
 *        1       (001):  0
 *        2       (010):  1
 *        3       (011):  1
 *        4       (100):  0
 *        5       (101):  0
 *        6       (110):  1
 *        7       (111):  1
 *
 * So if, for example, the value of P is one, S is zero, and D is zero (state
 * 4), the output will be the bit in position 4 of the MinTerm, which is zero.
 *
 * The concept that was hardest for me to grasp was the setting of bits that I
 * "don't care" about.  For example, state 3 (PSD = 011) is set to 1.  This
 * means that the Destination (in addition to the source) must be one for the
 * output to be one.  "But I don't care about the Destination value.  Why am I
 * selecting it?"  You have to remember that the bits are not source
 * selection bits, they are truth table outputs.  Note that state 2
 * (PSD = 010) is also set to 1.  This means that the Destination must be
 * zero for the output to be one.  Taken together with state 3, it means that
 * the output will be 1 *no matter what* the value of the Destination is,
 * which is what we want.  The same trick is pulled with the Pattern source
 * (state 2 (PSD = 010) and 6 (PSD = 110)); the output is one regardless of
 * the Pattern value.  So forming a MinTerm value from scratch requires you to
 * consider *all* of the sources, whether you care about them or not.
 *
 * For multi-bit (color) pixel values, each bit position is considered
 * independently of the others.  Apply the MinTerm truth table to each bit
 * position in the source values to generate an output for it.
 *
 * This mechanism is deceptively powerful when implemented properly (unlike
 * in the Windoze universe), and can be used to do all kinds of non-obvious
 * logical and mathematical operations on large groups of numbers.
 */
#define	MT_PSD		(1<<7)
#define	MT_PSND		(1<<6)
#define	MT_PNSD		(1<<5)
#define	MT_PNSND	(1<<4)
#define	MT_NPSD		(1<<3)
#define	MT_NPSND	(1<<2)
#define	MT_NPNSD	(1<<1)
#define	MT_NPNSND	(1<<0)

#define	MINTERM_PATCOPY		(MT_PSD | MT_PSND | MT_PNSD | MT_PNSND)
#define	MINTERM_SRCCOPY		(MT_PSD | MT_PSND | MT_NPSD | MT_NPSND)
#define	MINTERM_DESTCOPY	(MT_PSD | MT_PNSD | MT_NPSD | MT_NPNSD)


/*****************************************************************************
 * Instruction formats and definitions.
 */
#define	I8INST_CLIENT				31:29

#define	I8CLI_PARSER				0
#define	I8CLI_2D				2
#define	I8CLI_3D				3


/*  Different "clients" have differing instruction formats...  */
#define	I8INST_PARSER_OPCODE			28:23
#define	I8INST_PARSER_DATA			22:0


#define	I8INST_2D_OPCODE			28:22
#define	I8INST_2D_DATA				21:0


#define	I8INST_3D_OPCODE			28:24

#define	I8INST_3D_S24_DATA			23:0

#define	I8INST_3D_S16_SUBCODE			23:19
#define	I8INST_3D_S16_DATA			18:0

#define	I8INST_3D_SMW_SUBCODE			23:16
#define	I8INST_3D_SMW_COUNT			15:0

#define	I8INST_3D_PRIM_TYPE			22:18
#define	I8INST_3D_PRIM_COUNT			17:0



/*
 * Instruction-building macros.
 */
#define	MKINSPR(opcode,data)	(SetBF (I8INST_CLIENT, I8CLI_PARSER) | \
				 SetBF (I8INST_PARSER_OPCODE, (opcode)) | \
				 SetBF (I8INST_PARSER_DATA, (data)))
#define	MKINS2D(opcode,data)	(SetBF (I8INST_CLIENT, I8CLI_2D) | \
				 SetBF (I8INST_2D_OPCODE, (opcode)) | \
				 SetBF (I8INST_2D_DATA, (data)))
#define	MKINS3D_S24(op,data)	(SetBF (I8INST_CLIENT, I8CLI_3D) | \
				 SetBF (I8INST_3D_OPCODE, (op)) | \
				 SetBF (I8INST_3D_S24_DATA, (data)))
#define	MKINS3D_S16(sub,data)	(SetBF (I8INST_CLIENT, I8CLI_3D) | \
				 SetBF (I8INST_3D_OPCODE, 0x1C) | \
				 SetBF (I8INST_3D_S16_SUBCODE, (sub)) | \
				 SetBF (I8INST_3D_S16_DATA, (data)))
#define	MKINS3D_SMW(sub,count)	(SetBF (I8INST_CLIENT, I8CLI_3D) | \
				 SetBF (I8INST_3D_OPCODE, 0x1D) | \
				 SetBF (I8INST_3D_SMW_SUBCODE, (sub)) | \
				 SetBF (I8INST_3D_SMW_COUNT, (count)))
#define	GFX3DPRIM(type,count)	(SetBF (I8INST_CLIENT, I8CLI_3D) | \
				 SetBF (I8INST_3D_OPCODE, 0x1F) | \
				 SetBF (I8INST_3D_PRIM_TYPE, (type)) | \
				 SetBF (I8INST_3D_PRIM_COUNT, (count)))


/*****************************************************************************
 * "Generic" commands.
 */
#define	GFXCMDPARSER_NOP			MKINSPR (0x00, 0)
#define	GFXCMDPARSER_NOP_IDVALID		22:22
#define	GFXCMDPARSER_NOP_IDVALUE		21:6

#define	GFXCMDPARSER_BREAKPOINT_INTERRUPT	MKINSPR (0x01, 0)
#define	GFXCMDPARSER_USER_INTERRUPT		MKINSPR (0x02, 0)

#define	GFXCMDPARSER_WAIT_FOR_EVENT		MKINSPR (0x03, 0)
#define	GFXCMDPARSER_WAIT_FOR_EVENT_VBLANK	3:3
#define	GFXCMDPARSER_WAIT_FOR_EVENT_PAGEFLIP	2:2
#define	GFXCMDPARSER_WAIT_FOR_EVENT_SCANLINES	1:1

#define	GFXCMDPARSER_FLUSH			MKINSPR (0x04, 0)
#define	GFXCMDPARSER_FLUSH_AGP			1:1
#define	GFXCMDPARSER_FLUSH_INVALIDATECACHE	0:0

#define	GFXCMDPARSER_CONTEXT_SEL		MKINSPR (0x05, 0)
#define	GFXCMDPARSER_CONTEXT_SEL_LOADISVALID	17:17
#define	GFXCMDPARSER_CONTEXT_SEL_USEISVALID	16:16
#define	GFXCMDPARSER_CONTEXT_SEL_LOAD		8:8
#define	GFXCMDPARSER_CONTEXT_SEL_USE		0:0

#define	GFXCMDPARSER_REPORT_HEAD		MKINSPR (0x07, 0)

#define	GFXCMDPARSER_ARB_ON_OFF			MKINSPR (0x08, 0)
#define	GFXCMDPARSER_ARB_ON_OFF_ENABLED		0:0

#define	GFXCMDPARSER_OVERLAY_FLIP		MKINSPR (0x11, 0)
#define	GFXCMDPARSER_LOAD_SCAN_LINES_INCL	MKINSPR (0x12, 0)
#define	GFXCMDPARSER_LOAD_SCAN_LINES_EXCL	MKINSPR (0x13, 0)

#define	GFXCMDPARSER_FRONT_BUFFER_INFO		MKINSPR (0x14, 0)
#define	GFXCMDPARSER_FRONT_BUFFER_INFO_PITCH	19:8
#define	GFXCMDPARSER_FRONT_BUFFER_INFO_TYPE	6:6
#define	GFXCMDPARSER_FRONT_BUFFER_INFO_TYPE_SYNC	0
#define	GFXCMDPARSER_FRONT_BUFFER_INFO_TYPE_ASYNC	1

#define	GFXCMDPARSER_DEST_BUFFER_INFO		MKINSPR (0x15, 0)
#define	GFXCMDPARSER_Z_BUFFER_INFO		MKINSPR (0x16, 0)
#define	GFXCMDPARSER_STORE_DWORD_IMM		MKINSPR (0x20, 1)
#define	GFXCMDPARSER_STORE_DWORD_INDEX		MKINSPR (0x21, 1)
#define	GFXCMDPARSER_BATCH_BUFFER		MKINSPR (0x30, 1)

#define	GFXCMDPARSER_NOP_ID(id)	(GFXCMDPARSER_NOP \
				 | VAL2FIELD (GFXCMDPARSER_NOP, IDVALID, TRUE) \
				 | VAL2MASKD (GFXCMDPARSER_NOP, IDVALUE, (id)))


typedef struct GCmd_OverlayFlip {
	uint32	gcmd_Instruction;	/*  GFXCMDPARSER_OVERLAY_FLIP  */
	uint32	gcmd_BaseAddr;
} GCmd_OverlayFlip;

typedef struct GCmd_LoadScanLines {
	uint32	gcmd_Instruction;	/*  GFXCMDPARSER_LOAD_SCAN_LINES_INCL  */
					/*  GFXCMDPARSER_LOAD_SCAN_LINES_EXCL  */
	uint32	gcmd_StartEndLine;
} GCmd_LoadScanLines;
#define	GCMD_STARTENDLINE_START			31:16
#define	GCMD_STARTENDLINE_END			15:0


typedef struct GCmd_FrontBufferInfo {
	uint32	gcmd_Instruction;	/* GFXCMDPARSER_FRONT_BUFFER_INFO  */
	uint32	gcmd_BaseAddr;
} GCmd_FrontBufferInfo;

typedef struct GCmd_DestBufferInfo {
	uint32	gcmd_Instruction;	/*  GFXCMDPARSER_DEST_BUFFER_INFO  */
	uint32	gcmd_AddrPitch;
} GCmd_DestBufferInfo;
#define	GCMD_ADDRPITCH_ADDR_25_12		25:12
#define	GCMD_ADDRPITCH_PITCH			1:0
#define	GCMD_ADDRPITCH_PITCH_512		0
#define	GCMD_ADDRPITCH_PITCH_1024		1
#define	GCMD_ADDRPITCH_PITCH_2048		2
#define	GCMD_ADDRPITCH_PITCH_4096		3


typedef struct GCmd_ZBufferInfo {
	uint32	gcmd_Instruction;	/*  GFXCMDPARSER_Z_BUFFER_INFO  */
	uint32	gcmd_AddrPitch;
} GCmd_ZBufferInfo;

typedef struct GCmd_Store32 {
	uint32	gcmd_Instruction;	/*  GFXCMDPARSER_STORE_DWORD_IMM  */
					/*  GFXCMDPARSER_STORE_DWORD_INDEX  */
	uint32	gcmd_PhysAddr;		/*  ...or offset into HW status  */
	uint32	gcmd_Value;
} GCmd_Store32;

typedef struct GCmd_BatchBuffer {
	uint32	gcmd_Instruction;	/*  GFXCMDPARSER_BATCH_BUFFER  */
	uint32	gcmd_StartAddrProt;
	uint32	gcmd_EndAddr;
} GCmd_BatchBuffer;


/*****************************************************************************
 * Blitter and Stretch Blitter instruction structures.
 */
/*  2D rendering instructions  */
#define	GFX2DOP_SETUP_BLT			MKINS2D (0x00, 0x06)
#define	GFX2DOP_MONO_PATTERN_SL_BLT		MKINS2D (0x10, 0x07)

#define	GFX2DOP_PIXEL_BLT			MKINS2D (0x20, 0x00)
#define	GFX2DOP_PIXEL_BLT_DESTX			21:6

#define	GFX2DOP_SCANLINE_BLT			MKINS2D (0x21, 0x01)
#define	GFX2DOP_SCANLINE_BLT_PATVALIGN		7:5

#define	GFX2DOP_TEXT_BLT			MKINS2D (0x22, 0x04)
#define	GFX2DOP_TEXT_BLT_PACKING		16:16
#define	GFX2DOP_TEXT_BLT_PACKING_BITWISE	0
#define	GFX2DOP_TEXT_BLT_PACKING_BYTEWISE	1

#define	GFX2DOP_TEXT_IMMEDIATE_BLT(len)		MKINS2D (0x30, (len) + 2)
#define	GFX2DOP_COLOR_BLT			MKINS2D (0x40, 0x03)
#define	GFX2DOP_PAT_BLT				MKINS2D (0x41, 0x03)
#define	GFX2DOP_MONO_PAT_BLT			MKINS2D (0x42, 0x06)
#define	GFX2DOP_SRC_COPY_BLT			MKINS2D (0x43, 0x04)
#define	GFX2DOP_SRC_COPY_IMMEDIATE_BLT(len)	MKINS2D (0x60, (len) + 2)

#define	GFX2DOP_MONO_SRC_COPY_BLT		MKINS2D (0x44, 0x06)
#define	GFX2DOP_MONO_SRC_COPY_BLT_MONOSRCSHIFT	19:17

#define	GFX2DOP_MONO_SRC_COPY_IMMEDIATE_BLT(len) MKINS2D (0x61, (len) + 4)
#define	GFX2DOP_MONO_SRC_COPY_IMMEDIATE_BLT_MONOSRCSHIFT	19:17

#define	GFX2DOP_FULL_BLT			MKINS2D (0x45, 0x06)	/**/
#define	GFX2DOP_FULL_MONO_SRC_BLT		MKINS2D (0x46, 0x07)	/**/
#define	GFX2DOP_FULL_MONO_PATTERN_BLT		MKINS2D (0x47, 0x09)	/**/
#define	GFX2DOP_FULL_MONO_PATTERN_MONO_SRC_BLT	MKINS2D (0x48, 0x0a)	/**/


typedef struct Blt_Setup {
	uint32	BLT_Instruction;	/*  GFX2DOP_SETUP_BLT  */
	uint32	BLT_SetupCtl;
	uint32	BLT_ClipY1;
	uint32	BLT_ClipY2;
	uint32	BLT_ClipX1X2;
	uint32	BLT_SetupBGColor;
	uint32	BLT_SetupFGColor;
	uint32	BLT_SetupColorPatAddr;
} Blt_Setup;

typedef struct Blt_MonoPatternSL {
	uint32	BLT_Instruction;	/*  GFX2DOP_MONO_PATTERN_SL_BLT  */
	uint32	BLT_SetupCtl;
	uint32	BLT_ClipY1;
	uint32	BLT_ClipY2;
	uint32	BLT_ClipX1X2;
	uint32	BLT_SetupBGColor;
	uint32	BLT_SetupFGColor;
	uint32	BLT_PatternData0;
	uint32	BLT_PatternData1;
} Blt_MonoPatternSL;

typedef struct Blt_Pixel {
	uint32	BLT_Instruction;	/*  GFX2DOP_PIXEL_BLT  */
	uint32	BLT_DestY1;
} Blt_Pixel;

typedef struct Blt_Scanline {
	uint32	BLT_Instruction;	/*  GFX2DOP_SCANLINE_BLT  */
	uint32	BLT_DestX1X2;
	uint32	BLT_DestY1;
} Blt_Scanline;

typedef struct Blt_Text {
	uint32	BLT_Instruction;	/*  GFX2DOP_TEXT_BLT  */
	uint32	BLT_DestX1X2;
	uint32	BLT_DestY1;
	uint32	BLT_DestY2;
	uint32	BLT_SrcPitchStatus;
	uint32	BLT_SrcAddr;
} Blt_Text;

typedef struct Blt_TextImmediate {
	uint32	BLT_Instruction;	/*  GFX2DOP_TEXT_IMMEDIATE_BLT  */
	uint32	BLT_DestX1X2;
	uint32	BLT_DestY1;
	uint32	BLT_DestY2;
	uint32	BLT_Data[0];		/*  Immediate data follows here  */
} Blt_TextImmediate;

typedef struct Blt_Color {
	uint32	BLT_Instruction;	/*  GFX2DOP_COLOR_BLT  */
	uint32	BLT_Ctl;		/*  Color depth only  */
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_PatBGColor;
} Blt_Color;

typedef struct Blt_Pat {
	uint32	BLT_Instruction;	/*  GFX2DOP_PAT_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_ColorPatAddr;
} Blt_Pat;

typedef struct Blt_MonoPat {
	uint32	BLT_Instruction;	/*  GFX2DOP_MONO_PAT_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_PatBGColor;
	uint32	BLT_PatFGColor;
	uint32	BLT_PatternData0;
	uint32	BLT_PatternData1;
} Blt_MonoPat;

typedef struct Blt_SrcCopy {
	uint32	BLT_Instruction;	/*  GFX2DOP_SRC_COPY_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcPitchStatus;
	uint32	BLT_SrcAddr;
} Blt_SrcCopy;

typedef struct Blt_SrcCopyImmediate {
	uint32	BLT_Instruction;	/*  GFX2DOP_SRC_COPY_IMMEDIATE_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_Data[0];		/*  Immediate data follows here  */
} Blt_SrcCopyImmediate;

typedef struct Blt_MonoSrcCopy {
	uint32	BLT_Instruction;	/*  GFX2DOP_MONO_SRC_COPY_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcSize64;		/*  # of 64-bit source words - 1  */
	uint32	BLT_SrcAddr;
	uint32	BLT_SrcBGColor;
	uint32	BLT_SrcFGColor;
} Blt_MonoSrcCopy;

typedef struct Blt_MonoSrcCopyImmediate {
	uint32	BLT_Instruction;	/*  GFX2DOP_MONO_SRC_COPY_IMMEDIATE_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcBGColor;
	uint32	BLT_SrcFGColor;
	uint32	BLT_Data[0];		/*  Immediate data follows here  */
} Blt_MonoSrcCopyImmediate;

typedef struct Blt_Full {
	uint32	BLT_Instruction;	/*  GFX2DOP_FULL_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcPitchStatus;
	uint32	BLT_SrcAddr;
	uint32	BLT_SrcBGColor;		/*  Destination transparency  */
	uint32	BLT_ColorPatAddr;
} Blt_Full;

typedef struct Blt_FullMonoSrc {
	uint32	BLT_Instruction;	/*  GFX2DOP_FULL_MONO_SRC_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcSize64;		/*  # of 64-bit source words - 1  */
	uint32	BLT_SrcAddr;
	uint32	BLT_SrcBGColor;
	uint32	BLT_SrcFGColor;
	uint32	BLT_ColorPatAddr;
} Blt_FullMonoSrc;

typedef struct Blt_FullMonoPattern {
	uint32	BLT_Instruction;	/*  GFX2DOP_FULL_MONO_PATTERN_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcPitchStatus;
	uint32	BLT_SrcAddr;
	uint32	BLT_SrcBGColor;		/*  Destination transparency  */
	uint32	BLT_PatBGColor;
	uint32	BLT_PatFGColor;
	uint32	BLT_PatternData0;
	uint32	BLT_PatternData1;
} Blt_FullMonoPattern;

typedef struct Blt_FullMonoPatternMonoSrc {
	uint32	BLT_Instruction;	/*  GFX2DOP_FULL_MONO_PATTERN_MONO_SRC_BLT  */
	uint32	BLT_Ctl;
	uint32	BLT_DestDims;
	uint32	BLT_DestAddr;
	uint32	BLT_SrcSize64;		/*  # of 64-bit source words - 1  */
	uint32	BLT_SrcAddr;
	uint32	BLT_SrcBGColor;
	uint32	BLT_SrcFGColor;
	uint32	BLT_PatBGColor;
	uint32	BLT_PatFGColor;
	uint32	BLT_PatternData0;
	uint32	BLT_PatternData1;
} Blt_FullMonoPatternMonoSrc;


/*****************************************************************************
 * "Generic" commands.
 */
/*  3D rendering instructions  */
#define	GFX3DPRIMTYPE_TRI			0
#define	GFX3DPRIMTYPE_TRISTRIP			1
#define	GFX3DPRIMTYPE_TRISTRIPREVERSED		2
#define	GFX3DPRIMTYPE_TRIFAN			3
#define	GFX3DPRIMTYPE_POLYGON			4
#define	GFX3DPRIMTYPE_LINE			5
#define	GFX3DPRIMTYPE_LINESTRIP			6
#define	GFX3DPRIMTYPE_RECT			7

#define	GFX3DSTATE_VERTEX_FORMAT		MKINS3D_S24 (0x05, 0)
#define	GFX3DSTATE_VERTEX_FORMAT_NTEXCOORDS	11:8
#define	GFX3DSTATE_VERTEX_FORMAT_SPECULARFOG	7:7
#define	GFX3DSTATE_VERTEX_FORMAT_DIFFUSEALPHA	6:6
#define	GFX3DSTATE_VERTEX_FORMAT_ZBIAS		5:5
#define	GFX3DSTATE_VERTEX_FORMAT_COORDS		3:1
#define	GFX3DSTATE_VERTEX_FORMAT_COORDS_INVALID	0
#define	GFX3DSTATE_VERTEX_FORMAT_COORDS_XYZ	1
#define	GFX3DSTATE_VERTEX_FORMAT_COORDS_XYZW	2
#define	GFX3DSTATE_VERTEX_FORMAT_COORDS_XY	3
#define	GFX3DSTATE_VERTEX_FORMAT_COORDS_XYW	4

#define	GFX3DSTATE_MAP_TEXELS			MKINS3D_S16 (0x00, 0)
#define	GFX3DSTATE_MAP_TEXELS_T1VALID		15:15
#define	GFX3DSTATE_MAP_TEXELS_T1ENABLE		14:14
#define	GFX3DSTATE_MAP_TEXELS_T1COORDSET	11:11
#define	GFX3DSTATE_MAP_TEXELS_T1MAPINFO		8:8
#define	GFX3DSTATE_MAP_TEXELS_T0VALID		7:7
#define	GFX3DSTATE_MAP_TEXELS_T0ENABLE		6:6
#define	GFX3DSTATE_MAP_TEXELS_T0COORDSET	3:3
#define	GFX3DSTATE_MAP_TEXELS_T0MAPINFO		0:0

#define	GFX3DSTATE_MAP_COORD_SETS		MKINS3D_S16 (0x01, 0)
#define	GFX3DSTATE_MAP_COORD_SETS_UPDATEIDX		16:16
#define	GFX3DSTATE_MAP_COORD_SETS_NORMALIZEDVALID	15:15
#define	GFX3DSTATE_MAP_COORD_SETS_NORMALIZED		14:14
#define	GFX3DSTATE_MAP_COORD_SETS_VSTATEVALID		7:7
#define	GFX3DSTATE_MAP_COORD_SETS_VSTATE		5:4
#define	GFX3DSTATE_MAP_COORD_SETS_VSTATE_WRAP		0
#define	GFX3DSTATE_MAP_COORD_SETS_VSTATE_MIRROR		1
#define	GFX3DSTATE_MAP_COORD_SETS_VSTATE_CLAMP		2
#define	GFX3DSTATE_MAP_COORD_SETS_VSTATE_WRAPSHORTEST	3
#define	GFX3DSTATE_MAP_COORD_SETS_USTATEVALID		3:3
#define	GFX3DSTATE_MAP_COORD_SETS_USTATE		1:0
#define	GFX3DSTATE_MAP_COORD_SETS_USTATE_WRAP		0
#define	GFX3DSTATE_MAP_COORD_SETS_USTATE_MIRROR		1
#define	GFX3DSTATE_MAP_COORD_SETS_USTATE_CLAMP		2
#define	GFX3DSTATE_MAP_COORD_SETS_USTATE_WRAPSHORTEST	3

#define	GFX3DSTATE_MAP_INFO			MKINS3D_SMW (0x00, 0x02)

#define	GFX3DSTATE_MAP_FILTER			MKINS3D_S16 (0x02, 0)
#define	GFX3DSTATE_MAP_FILTER_UPDATEIDX		16:16
#define	GFX3DSTATE_MAP_FILTER_ANISOTROPICVALID	12:12
#define	GFX3DSTATE_MAP_FILTER_ANISOTROPIC	10:10
#define	GFX3DSTATE_MAP_FILTER_MIPMODEVALID	9:9
#define	GFX3DSTATE_MAP_FILTER_MIPMODE		7:6
#define	GFX3DSTATE_MAP_FILTER_MIPMODE_NONE	0
#define	GFX3DSTATE_MAP_FILTER_MIPMODE_NEAREST	1
#define	GFX3DSTATE_MAP_FILTER_MIPMODE_DITHER	2	/* Found in XFree86 */
#define	GFX3DSTATE_MAP_FILTER_MAGMODEVALID	5:5
#define	GFX3DSTATE_MAP_FILTER_MAGMODE		3:3
#define	GFX3DSTATE_MAP_FILTER_MAGMODE_NEAREST	0
#define	GFX3DSTATE_MAP_FILTER_MAGMODE_LINEAR	1
#define	GFX3DSTATE_MAP_FILTER_MINMODEVALID	2:2
#define	GFX3DSTATE_MAP_FILTER_MINMODE		0:0
#define	GFX3DSTATE_MAP_FILTER_MINMODE_NEAREST	0
#define	GFX3DSTATE_MAP_FILTER_MINMODE_LINEAR	1

#define	GFX3DSTATE_MAP_LOD_LIMITS		MKINS3D_S16 (0x03, 0)
#define	GFX3DSTATE_MAP_LOD_LIMITS_UPDATEIDX	16:16
#define	GFX3DSTATE_MAP_LOD_LIMITS_MAXMIPVALID	13:13
#define	GFX3DSTATE_MAP_LOD_LIMITS_MAXMIP	12:5
#define	GFX3DSTATE_MAP_LOD_LIMITS_MINMIPVALID	4:4
#define	GFX3DSTATE_MAP_LOD_LIMITS_MINMIP	3:0

#define	GFX3DSTATE_MAP_LOD_CONTROL		MKINS3D_S16 (0x04, 0)
#define	GFX3DSTATE_MAP_LOD_CONTROL_UPDATEIDX	16:16
#define	GFX3DSTATE_MAP_LOD_CONTROL_LODBIASVALID	7:7
#define	GFX3DSTATE_MAP_LOD_CONTROL_LODBIAS	6:0

#define	GFX3DSTATE_MAP_PALETTE_LOAD		MKINS3D_SMW (0x82, 255)

#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES	MKINS3D_S24 (0x00, 0)
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_UPDATEBLENDIDX	21:20
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_CURRENTACCUMVALID	19:19
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_CURRENTACCUM		18:18
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_CURRENTACCUM_CURRENT	0
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_CURRENTACCUM_ACCUM	1
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1VALID		17:17
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC		16:14
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_ONE		0
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_CONST		1
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_ACCUM		2
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_ITERATED	3
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_SPECULAR	4
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_CURRENT	5
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_TEX0		6
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1SRC_TEX1		7
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1COPYALPHA2COLOR	13:13
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG1INVERT		12:12
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2VALID		11:11
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC		10:8
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_ONE		0
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_CONST		1
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_ACCUM		2
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_ITERATED	3
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_SPECULAR	4
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_CURRENT	5
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_TEX0		6
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2SRC_TEX1		7
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2COPYALPHA2COLOR	7:7
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_ARG2INVERT		6:6
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OPVALID		5:5
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP			4:0
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_DISABLE		0
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_ARG1		1
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_ARG2		2
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_MODULATE		3
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_MODULATEx2		4
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_MODULATEx4		5
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_ADD		6
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_ADDSIGNED		7
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_BLENDITERATEDALPHA	8
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_BLENDALPHA		10
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_BLENDTEX0ALPHA	16
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_BLENDTEX1ALPHA	17
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_BLENDTEX0COLOR	18
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_BLENDTEX1COLOR	19
#define	GFX3DSTATE_MAP_COLOR_BLEND_STAGES_OP_SUBTRACT		20

#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES	MKINS3D_S24 (0x01, 0)
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_UPDATEBLENDIDX	21:20
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1VALID		18:18
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1SRC		17:15
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1SRC_CONST		1
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1SRC_ITERATED	3
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1SRC_CURRENT	5
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1SRC_TEX0		6
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1SRC_TEX1		7
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG1INVERT		13:13
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2VALID		12:12
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2SRC		10:8
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2SRC_CONST		1
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2SRC_ITERATED	3
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2SRC_CURRENT	5
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2SRC_TEX0		6
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2SRC_TEX1		7
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_ARG2INVERT		6:6
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OPVALID		5:5
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP			4:0
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_ARG1		1
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_ARG2		2
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_MODULATE		3
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_MODULATEx2		4
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_MODULATEx4		5
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_ADD		6
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_ADDSIGNED		7
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_BLENDITERATEDALPHA	8
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_BLENDALPHA		10
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_BLENDTEX0ALPHA	16
#define	GFX3DSTATE_MAP_ALPHA_BLEND_STAGES_OP_BLENDTEX1ALPHA	17

#define	GFX3DSTATE_COLOR_FACTOR			MKINS3D_SMW (0x01, 0)
#define	GFX3DSTATE_COLOR_CHROMA_KEY		MKINS3D_SMW (0x02, 1)

#define	GFX3DSTATE_SRC_DEST_BLEND_MONO		MKINS3D_S24 (0x08, 0)
#define	GFX3DSTATE_SRC_DEST_BLEND_MONO_COLORMONOENABLEVALID	13:13
#define	GFX3DSTATE_SRC_DEST_BLEND_MONO_COLORMONOENABLE		12:12
#define	GFX3DSTATE_SRC_DEST_BLEND_MONO_SRCBLENDVALID		11:11
#define	GFX3DSTATE_SRC_DEST_BLEND_MONO_SRCBLEND			10:6
#define	GFX3DSTATE_SRC_DEST_BLEND_MONO_DESTBLENDVALID		5:5
#define	GFX3DSTATE_SRC_DEST_BLEND_MONO_DESTBLEND		4:0
#define	BLENDFACTOR_ZERO				1
#define	BLENDFACTOR_ONE					2
#define	BLENDFACTOR_SRCCOLOR				3
#define	BLENDFACTOR_INVSRCCOLOR				4
#define	BLENDFACTOR_SRCALPHA				5
#define	BLENDFACTOR_INVSRCALPHA				6
#define	BLENDFACTOR_DESTCOLOR				9
#define	BLENDFACTOR_INVDESTCOLOR			10
#define	BLENDFACTOR_BOTHSRCALPHA			12
#define	BLENDFACTOR_BOTHINVSRCALPHA			13

#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF	MKINS3D_S24 (0x14, 0)
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ZBIASVALID		22:22
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ZBIAS			21:14
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNCVALID		13:13
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC		12:9
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_NEVER	1
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_LT		2
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_EQ		3
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_LE		4
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_GT		5
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_NE		6
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_GE		7
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAFUNC_ALWAYS	8
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAREFVALID		8:8
#define	GFX3DSTATE_Z_BIAS_ALPHA_FUNC_REF_ALPHAREF		7:3

#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE	MKINS3D_S24 (0x02, 0)
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNCVALID	20:20
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC		18:16
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_ALWAYS	0
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_NEVER	1
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_LT		2
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_EQ		3
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_LE		4
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_GT		5
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_NE		6
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ZFUNC_GE		7
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_LINEWIDTHVALID	15:15
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_LINEWIDTH		14:12
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ALPHAMODEVALID	11:11
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ALPHAMODE		10:10
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ALPHAMODE_GOURAUD	0
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_ALPHAMDOE_FLAT	1
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_FOGMODEVALID	9:9
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_FOGMODE		8:8
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_FOGMODE_GOURAUD	0
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_FOGMODE_FLAT	1
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_SPECULARMODEVALID	7:7
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_SPECULARMODE	6:6
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_SPECULARMODE_GOURAUD	0
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_SPECULARMODE_FLAT	1
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_COLORMODEVALID	5:5
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_COLORMODE		4:4
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_COLORMODE_GOURAUD	0
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_COLORMODE_FLAT	1
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_CULLMODEVALID	3:3
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_CULLMODE		2:0
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_CULLMODE_NONE	1
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_CULLMODE_CW	2
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_CULLMODE_CCW	3
#define	GFX3DSTATE_LINE_WIDTH_CULL_SHADE_MODE_CULLMODE_BOTH	4

#define	GFX3DSTATE_BOOLEAN_ENA_1		MKINS3D_S24 (0x03, 0)
#define	GFX3DSTATE_BOOLEAN_ENA_1_ALPHASETUPENABLEVALID		17:17
#define	GFX3DSTATE_BOOLEAN_ENA_1_ALPHASETUPENABLE		16:16
#define	GFX3DSTATE_BOOLEAN_ENA_1_FOGENABLEVALID			7:7
#define	GFX3DSTATE_BOOLEAN_ENA_1_FOGENABLE			6:6
#define	GFX3DSTATE_BOOLEAN_ENA_1_ALPHAENABLEVALID		5:5
#define	GFX3DSTATE_BOOLEAN_ENA_1_ALPHAENABLE			4:4
#define	GFX3DSTATE_BOOLEAN_ENA_1_BLENDENABLEVALID		3:3
#define	GFX3DSTATE_BOOLEAN_ENA_1_BLENDENABLE			2:2
#define	GFX3DSTATE_BOOLEAN_ENA_1_ZENABLEVALID			1:1
#define	GFX3DSTATE_BOOLEAN_ENA_1_ZENABLE			0:0

#define	GFX3DSTATE_BOOLEAN_ENA_2		MKINS3D_S24 (0x04, 0)
#define	GFX3DSTATE_BOOLEAN_ENA_2_MAPCACHEENABLEVALID		17:17
#define	GFX3DSTATE_BOOLEAN_ENA_2_MAPCACHEENABLE			16:16
#define	GFX3DSTATE_BOOLEAN_ENA_2_ALPHADITHERENABLEVALID		15:15
#define	GFX3DSTATE_BOOLEAN_ENA_2_ALPHADITHERENABLE		14:14
#define	GFX3DSTATE_BOOLEAN_ENA_2_FOGDITHERENABLEVALID		13:13
#define	GFX3DSTATE_BOOLEAN_ENA_2_FOGDITHERENABLE		12:12
#define	GFX3DSTATE_BOOLEAN_ENA_2_SPECULARDITHERENABLEVALID	11:11
#define	GFX3DSTATE_BOOLEAN_ENA_2_SPECULARDITHERENABLE		10:10
#define	GFX3DSTATE_BOOLEAN_ENA_2_COLORDITHERENABLEVALID		9:9
#define	GFX3DSTATE_BOOLEAN_ENA_2_COLORDITHERENABLE		8:8
#define	GFX3DSTATE_BOOLEAN_ENA_2_FBWRITEENABLEVALID		3:3
#define	GFX3DSTATE_BOOLEAN_ENA_2_FBWRITEENABLE			2:2
#define	GFX3DSTATE_BOOLEAN_ENA_2_ZBUFWRITEENABLEVALID		1:1
#define	GFX3DSTATE_BOOLEAN_ENA_2_ZBUFWRITEENABLE		0:0

#define	GFX3DSTATE_FOG_COLOR			MKINS3D_S24 (0x15, 0)
#define	GFX3DSTATE_FOG_COLOR_RED		23:16	/*  18:16 == 0  */
#define	GFX3DSTATE_FOG_COLOR_GREEN		15:8	/*  9:8 == 0	*/
#define	GFX3DSTATE_FOG_COLOR_BLUE		7:0	/*  2:0 == 0	*/

#define	GFX3DSTATE_DRAWING_RECT_INFO		MKINS3D_SMW (0x80, 3)

#define	GFX3DSTATE_SCISSOR_ENABLE		MKINS3D_S16 (0x10, 0)
#define	GFX3DSTATE_SCISSOR_ENABLE_RECTID			16:16
#define	GFX3DSTATE_SCISSOR_ENABLE_RECTENABLEVALID		1:1
#define	GFX3DSTATE_SCISSOR_ENABLE_RECTENABLE			0:0

#define	GFX3DSTATE_SCISSOR_RECT_INFO		MKINS3D_SMW (0x81, 1)
#define	GFX3DSTATE_STIPPLE_PATTERN		MKINS3D_SMW (0x83, 0)

#define	GFX3DSTATE_ANTI_ALIASING		MKINS3D_S24 (0x16, 0)
#define	GFX3DSTATE_ANTI_ALIASING_EDGEFLAGENABLEVALID		13:13
#define	GFX3DSTATE_ANTI_ALIASING_EDGEFLAGENABLE			12:12
#define	GFX3DSTATE_ANTI_ALIASING_POLYWIDTHVALID			11:11
#define	GFX3DSTATE_ANTI_ALIASING_POLYWIDTH			10:9
#define	GFX3DSTATE_ANTI_ALIASING_POLYWIDTH_HALF			0
#define	GFX3DSTATE_ANTI_ALIASING_POLYWIDTH_ONE			1
#define	GFX3DSTATE_ANTI_ALIASING_POLYWIDTH_TWO			2
#define	GFX3DSTATE_ANTI_ALIASING_POLYWIDTH_FOUR			3
#define	GFX3DSTATE_ANTI_ALIASING_LINEWIDTHVALID			8:8
#define	GFX3DSTATE_ANTI_ALIASING_LINEWIDTH			7:6
#define	GFX3DSTATE_ANTI_ALIASING_LINEWIDTH_HALF			0
#define	GFX3DSTATE_ANTI_ALIASING_LINEWIDTH_ONE			1
#define	GFX3DSTATE_ANTI_ALIASING_LINEWIDTH_TWO			2
#define	GFX3DSTATE_ANTI_ALIASING_LINEWIDTH_FOUR			3
#define	GFX3DSTATE_ANTI_ALIASING_BBOXEXPANDVALID		5:5
#define	GFX3DSTATE_ANTI_ALIASING_BBOXEXPAND			4:2
#define	GFX3DSTATE_ANTI_ALIASING_ENABLEVALID			1:1
#define	GFX3DSTATE_ANTI_ALIASING_ENABLE				0:0

/*  PVPRULE = PROVOKING_VTX_PIXELIZATION_RULE  */
#define	GFX3DSTATE_PVPRULE			MKINS3D_S24 (0x07, 0)
#define	GFX3DSTATE_PVPRULE_TINYTRIFILTERVALID	12:12
#define	GFX3DSTATE_PVPRULE_TINYTRIFILTER	11:11
#define	GFX3DSTATE_PVPRULE_PIXRULEVALID		10:10
#define	GFX3DSTATE_PVPRULE_PIXRULE		9:9
#define	GFX3DSTATE_PVPRULE_LINESELECTVALID	8:8
#define	GFX3DSTATE_PVPRULE_LINESELECT		7:6
#define	GFX3DSTATE_PVPRULE_LINESELECT_VTX0	0
#define	GFX3DSTATE_PVPRULE_LINESELECT_VTX1	1
#define	GFX3DSTATE_PVPRULE_FANSELECTVALID	5:5
#define	GFX3DSTATE_PVPRULE_FANSELECT		4:3
#define	GFX3DSTATE_PVPRULE_FANSELECT_VTX0	0
#define	GFX3DSTATE_PVPRULE_FANSELECT_VTX1	1
#define	GFX3DSTATE_PVPRULE_FANSELECT_VTX2	2
#define	GFX3DSTATE_PVPRULE_STRIPSELECTVALID	2:2
#define	GFX3DSTATE_PVPRULE_STRIPSELECT		1:0
#define	GFX3DSTATE_PVPRULE_STRIPSELECT_VTX0	0
#define	GFX3DSTATE_PVPRULE_STRIPSELECT_VTX1	1
#define	GFX3DSTATE_PVPRULE_STRIPSELECT_VTX2	2

#define	GFX3DSTATE_DEST_BUFFER_VARIABLES	MKINS3D_SMW (0x85, 0)


typedef struct Gfx3D_MapInfo {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_MAP_INFO  */
	uint32	g3d_MapFormat;
	uint32	g3d_MapDims;
	uint32	g3d_BaseAddr;
} Gfx3D_MapInfo;
#define	G3D_MAPFORMAT_MAPIDX			28:28
#define	G3D_MAPFORMAT_FORMAT			26:21
#define	G3D_MAPFORMAT_FORMAT_CMAP8_RGB565	0x00
#define	G3D_MAPFORMAT_FORMAT_CMAP8_ARGB1555	0x01
#define	G3D_MAPFORMAT_FORMAT_CMAP8_ARGB4444	0x02
#define	G3D_MAPFORMAT_FORMAT_CMAP8_AY88		0x03
#define	G3D_MAPFORMAT_FORMAT_X8			0x08	/*  My own guess  */
#define	G3D_MAPFORMAT_FORMAT_RGB565		0x10
#define	G3D_MAPFORMAT_FORMAT_ARGB1555		0x11
#define	G3D_MAPFORMAT_FORMAT_ARGB4444		0x12
#define	G3D_MAPFORMAT_FORMAT_AY88		0x13
#define	G3D_MAPFORMAT_FORMAT_ARGB8888		0x18
#define	G3D_MAPFORMAT_FORMAT_YCrCb420		0x20
#define	G3D_MAPFORMAT_FORMAT_YCrCb422_YSWAP	0x30
#define	G3D_MAPFORMAT_OUTPUTSELECT		20:19
#define	G3D_MAPFORMAT_OUTPUTSELECT_F0F1F2F3	0
#define	G3D_MAPFORMAT_OUTPUTSELECT_xxF0xxF3	1
#define	G3D_MAPFORMAT_OUTPUTSELECT_xxF2xxF3	2
#define	G3D_MAPFORMAT_COLORCONVERTENABLE	18:18
#define	G3D_MAPFORMAT_VSKIP			17:17
#define	G3D_MAPFORMAT_VSKIPPHASE		16:16
#define	G3D_MAPFORMAT_TILINGINFO		10:10
#define	G3D_MAPFORMAT_TILINGINFO_LOCAL		0
#define	G3D_MAPFORMAT_TILINGINFO_GLOBAL		1
#define	G3D_MAPFORMAT_ISTILED			9:9
#define	G3D_MAPFORMAT_TILEWALK			8:8
#define	G3D_MAPFORMAT_TILEWALK_XMAJOR		0
#define	G3D_MAPFORMAT_TILEWALK_YMAJOR		1
#define	G3D_MAPFORMAT_LOG2PITCH			3:0

#define	G3D_MAPDIMS_DIMSARELOG2			31:31
#define	G3D_MAPDIMS_HEIGHT			25:16
#define	G3D_MAPDIMS_WIDTH			9:0


typedef struct Gfx3D_PaletteLoad {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_MAP_PALETTE_LOAD  */
	uint32	g3d_Colors[256];
} Gfx3D_PaletteLoad;

typedef struct Gfx3D_ColorFactor {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_COLOR_FACTOR  */
	uint32	g3d_Color;		/*  ARGB888 format  */
} Gfx3D_ColorFactor;

typedef struct Gfx3D_ColorChromaKey {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_COLOR_CHROMA_KEY  */
	uint32	g3d_ChromaLowValue;
	uint32	g3d_ChromaHighValue;
} Gfx3D_ColorChromaKey;
#define	G3D_CHROMALOWVALUE_KILLPIXELVALID	28:28
#define	G3D_CHROMALOWVALUE_KILLPIXEL		27:27
#define	G3D_CHROMALOWVALUE_INDEXVALUEVALID	26:26
#define	G3D_CHROMALOWVALUE_LOWVALUEVALID	25:25
#define	G3D_CHROMALOWVALUE_HIGHVALUEVALID	24:24
#define	G3D_CHROMALOWVALUE_LOWVALUE		23:0

#define	G3D_CHROMAHIGHVALUE_INDEXVALUE		31:24
#define	G3D_CHROMAHIGHVALUE_HIGHVALUE		23:0


typedef struct Gfx3D_DrawingRectInfo {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_DRAWING_RECT_INFO  */
	uint32	g3d_DitherBias;
	uint32	g3d_ClipMin;
	uint32	g3d_ClipMax;
	uint32	g3d_ClipOrigin;
} Gfx3D_DrawingRectInfo;
#define	G3D_DITHERBIAS_X			27:26
#define	G3D_DITHERBIAS_Y			25:24

#define	G3D_CLIPMIN_Y				31:16
#define	G3D_CLIPMIN_X				15:0

#define	G3D_CLIPMAX_Y				31:16
#define	G3D_CLIPMAX_X				15:0

#define	G3D_CLIPORIGIN_Y			26:16
#define	G3D_CLIPORIGIN_X			11:0


typedef struct Gfx3D_ScissorRectInfo {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_SCISSOR_RECT_INFO  */
	uint32	g3d_RectUL;
	uint32	g3d_RectLR;
} Gfx3D_ScissorRectInfo;
#define	G3D_RECTUL_Y				31:16
#define	G3D_RECTUL_X				15:0

#define	G3D_RECTLR_Y				31:16
#define	G3D_RECTLR_X				15:0


typedef struct Gfx3D_StipplePattern {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_STIPPLE_PATTERN  */
	uint32	g3d_StipPat;
} Gfx3D_StipplePattern;
#define	G3D_STIPPAT_ENABLE			16:16
#define	G3D_STIPPAT_4x4PATTERN			15:0


typedef struct Gfx3D_DestBufferVariables {
	uint32	g3d_Instruction;	/*  GFX3DSTATE_DEST_BUFFER_VARIABLES  */
	uint32	g3d_DestBufConfig;
} Gfx3D_DestBufferVariables;
#define	G3D_DESTBUFCONFIG_HBIAS			23:20
#define	G3D_DESTBUFCONFIG_VBIAS			19:16
#define	G3D_DESTBUFCONFIG_422WRITECHAN		13:12
#define	G3D_DESTBUFCONFIG_422WRITECHAN_YCrCb	0
#define	G3D_DESTBUFCONFIG_422WRITECHAN_Y	1
#define	G3D_DESTBUFCONFIG_422WRITECHAN_Cr	2
#define	G3D_DESTBUFCONFIG_422WRITECHAN_Cb	3
#define	G3D_DESTBUFCONFIG_FORMAT		10:8
#define	G3D_DESTBUFCONFIG_FORMAT_CMAP8		0
#define	G3D_DESTBUFCONFIG_FORMAT_RGB555		1
#define	G3D_DESTBUFCONFIG_FORMAT_RGB565		2
#define	G3D_DESTBUFCONFIG_FORMAT_YCrCb_YSWAP	4
#define	G3D_DESTBUFCONFIG_FORMAT_YCrCb		5
#define	G3D_DESTBUFCONFIG_FORMAT_YCrCb_UVSWAP	6
#define	G3D_DESTBUFCONFIG_FORMAT_YCrCb_UVYSWAP	7
#define	G3D_DESTBUFCONFIG_VSKIP			1:1
#define	G3D_DESTBUFCONFIG_VSKIPPHASE		0:0


#endif	/*  __I810DEFS_H  */
