/* :ts=8 bk=0
 *
 * i740defs.h:	Bit and structure definitions for Intel-740
 *
 * Leo L. Schwab					1998.07.08
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef	__I740DEFS_H
#define	__I740DEFS_H

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
 * The rest of the I740 registers.  I haven't settled on all the names yet.
 */
typedef struct I740Regs {
	union vgaregs	VGARegs;		/*  0x00000 - 0x00FFF	*/

	/*  Instruction and Interrupt Control  */
	vuint32		LPFIFO[0x0400];		/*  0x01000 - 0x01FFF	*/
	vuint32		HPFIFO[0x0400];		/*  0x02000 - 0x02FFF	*/
	vuint32		LMFT[7];		/*  0x03000 - 0x0301B	*/
	vuint32		__reserved0;		/*  0x0301C - 0x0301F	*/
	vuint32		INPEIR;			/*  0x03020 - 0x03023	*/
	vuint32		NOPID;			/*  0x03024 - 0x03027	*/
	vuint32		HWSTVA;			/*  0x03028 - 0x0302B	*/
	vuint16		HWSTM;			/*  0x0302C - 0x0302D	*/
	vuint16		__reserved1;		/*  0x0302E - 0x0302F	*/
	vuint16		IER;			/*  0x03030 - 0x03031	*/
	vuint16		IIR;			/*  0x03032 - 0x03033	*/
	vuint16		IMR;			/*  0x03034 - 0x03035	*/
	vuint16		ISR;			/*  0x03036 - 0x03037	*/
	vuint8		INPMODE;		/*  0x03038		*/
	vuint8		INSTMASK;		/*  0x03039		*/
	vuint16		__reserved2;		/*  0x0303A - 0x0303B	*/
	vuint32		PIOFLUSH;		/*  0x0303C - 0x0303F	*/
	vuint16		INPOINT;		/*  0x03040 - 0x03041	*/
	vuint16		INPARS;			/*  0x03042 - 0x03043	*/
	vuint32		DMAPARP;		/*  0x03044 - 0x03047	*/
	vuint32		DMAADR;			/*  0x03048 - 0x0304B	*/
	vuint16		THREEDSKEW;		/*  0x0304C - 0x0304D	*/
	vuint16		BLTSKEW;		/*  0x0304E - 0x0304F	*/
	vuint32		MAGP_HC_COMP;		/*  0x03050 - 0x03053	*/
	vuint32		AGP_QD_COMP;		/*  0x03054 - 0x03057	*/
	vuint8		__reserved3[0x2FA8];	/*  0x03058 - 0x05FFF	*/
	vuint32		FWATER_BLC;		/*  0x06000 - 0x06003	*/
	vuint8		__reserved4[0x0FFC];	/*  0x06004 - 0x06FFF	*/

	/*  DVD  */
	vuint32		DVDMAP[0x0040];		/*  0x07000 - 0x070FF	*/
	vuint32		DVDDMA;			/*  0x07100 - 0x07103	*/
	vuint32		__reserved5;		/*  0x07104 - 0x07107	*/
	vuint32		DDBADDR;		/*  0x07108 - 0x0710B	*/
	vuint32		__reserved6;		/*  0x0710C - 0x0710F	*/
	vuint32		DBURSTDST;		/*  0x07110 - 0x07113	*/
	vuint32		DHOSTCTL;		/*  0x07114 - 0x07117	*/
	vuint16		DHOSTSTA;		/*  0x07118 - 0x07119	*/
	vuint8		__reserved7[0x38EE6];	/*  0x0711A - 0x3FFFF	*/

/*******
 * WARNING:
 * HAZARDOUS REGISTERS
 * THE CPU MUST NOT WRITE TO ANY OF THESE REGISTERS.  USE ONLY I740 DMA
 * "INSTRUCTIONS" TO WRITE TO THIS SPACE.  Reads are (mostly) safe.
 */
	/*  BLT  */
	vuint32		BLT_Pitches;		/*  0x40000 - 0x40003	*/
	vuint32		BLT_PatBGColor;		/*  0x40004 - 0x40007	*/
	vuint32		BLT_PatFGColor;		/*  0x40008 - 0x4000B	*/
	vuint32		BLT_MonoSrcCtl;		/*  0x4000C - 0x4000F	*/
	vuint32		BLT_BlitCtl;		/*  0x40010 - 0x40013	*/
	vuint32		BLT_PatSrcAddr;		/*  0x40014 - 0x40017	*/
	vuint32		BLT_SrcAddr;		/*  0x40018 - 0x4001B	*/
	vuint32		BLT_DstAddr;		/*  0x4001C - 0x4001F	*/
	vuint32		BLT_DstSize;		/*  0x40020 - 0x40023	*/
	vuint32		BLT_SrcBGColor;		/*  0x40024 - 0x40027	*/
	vuint32		BLT_SrcFGColor;		/*  0x40028 - 0x4002B	*/
	vuint8		__reserved8[0x00D4];	/*  0x4002C - 0x400FF	*/

	/*  Stretch BLT  */
	vuint32		SBLT_SrcAddr;		/*  0x40100 - 0x40103	*/
	vuint32		SBLT_SrcWidth;		/*  0x40104 - 0x40107	*/
	vuint32		SBLT_SrcHeightCtl;	/*  0x40108 - 0x4010B	*/
	vuint32		SBLT_DstAddr;		/*  0x4010C - 0x4010F	*/
	vuint32		SBLT_DstWidth;		/*  0x40110 - 0x40113	*/
	vuint32		SBLT_DstHeightCtl;	/*  0x40114 - 0x40117	*/
	vuint32		SBLT_HFactor;		/*  0x40118 - 0x4011B	*/
	vuint32		SBLT_VFactor;		/*  0x4011C - 0x4011F	*/
	vuint32		SBLT_DDAPhase;		/*  0x40120 - 0x40123	*/
	vuint32		SBLT_Status;		/*  0x40124 - 0x40127	*/
	vuint32		SBLT_SrcMask;		/*  0x40128 - 0x4012B	*/
	vuint32		SBLT_SrcCmpColorMin;	/*  0x4012C - 0x4012F	*/
	vuint32		SBLT_SrcCmpColorMax;	/*  0x40130 - 0x40133	*/
	vuint32		SBLT_DstCmpColorMin;	/*  0x40134 - 0x40137	*/
	vuint32		SBLT_DstCmpColorMax;	/*  0x40138 - 0x4013B	*/
	vuint32		SBLT_Start;		/*  0x4013C - 0x4013F	*/
	vuint8		__reserved9[0xFEC0];	/*  0x40140 - 0x4FFFF	*/

	vuint32		BLTDATA[0x4000];	/*  0x50000 - 0x5FFFF	*/
	vuint32		SBLTDATA[0x4000];	/*  0x60000 - 0x6FFFF	*/
/*
 * End of hazardous registers.
 ******/

	/*  Multimedia Event Counter Interrupt Registers  */
	vuint32		CHI_CNT_CMP;		/*  0x70000 - 0x70003	*/
	vuint32		CHI_CNTL;		/*  0x70004 - 0x70007	*/
	vuint32		CHI_ACK_CNT;		/*  0x70008 - 0x7000B	*/
	vuint8		__reserved10[0x0014];	/*  0x7000C - 0x7001F	*/
	vuint32		DHI_CNT_CMP;		/*  0x70020 - 0x70023	*/
	vuint32		DECC;			/*  0x70024 - 0x70027	*/
	vuint32		DHI_ACK_CNT;		/*  0x70028 - 0x7002B	*/
	vuint8		__reserved11[0xFFD4];	/*  0x7002C - 0x7FFFF	*/
} I740Regs;


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

#define	CR_TVCTL				0x75
#define	CR_TVCTL_SYNCPOL			7:7
#define	CR_TVCTL_SYNCPOL_HIGH			0
#define	CR_TVCTL_SYNCPOL_LOW			1
#define	CR_TVCTL_TEST				6:6
#define	CR_TVCTL_CLKSEL				5:5
#define	CR_TVCTL_CLKSEL_INTERNAL		0
#define	CR_TVCTL_CLKSEL_EXTERNAL		1
#define	CR_TVCTL_SYNCSEL			4:4
#define	CR_TVCTL_SYNCSEL_INTERNAL		0
#define	CR_TVCTL_SYNCSEL_EXTERNAL		1
#define	CR_TVCTL_DATADIR			3:3
#define	CR_TVCTL_DATADIR_OUTPUT			0
#define	CR_TVCTL_DATADIR_INPUT			1
#define	CR_TVCTL_BLANKDIR			2:2
#define	CR_TVCTL_BLANKDIR_OUTPUT		0
#define	CR_TVCTL_BLANKDIR_INPUT			1
#define	CR_TVCTL_SYNCDIR			1:1
#define	CR_TVCTL_SYNCDIR_OUTPUT			0
#define	CR_TVCTL_SYNCDIR_INPUT			1
#define	CR_TVCTL_FLICKERFILTER			0:0
#define	CR_TVCTL_FLICKERFILTER_DISABLE		0
#define	CR_TVCTL_FLICKERFILTER_ENABLE		1

#define	CR_TVHCOUNT_11_8			0x76
#define	CR_TVHCOUNT_7_0				0x77
#define	CR_TVVCOUNT_11_8			0x78
#define	CR_TVVCOUNT_7_0				0x79


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
 * Configuration Extension register indices and bit definitions.
 */
#define	XR_VENDORID_LO				0x00
#define	XR_VENDORID_HI				0x01
#define	XR_DEVICEID_LO				0x02
#define	XR_DEVICEID_HI				0x03
#define	XR_REVISIONID				0x04
#define	XR_FBHOSTBASE_23_16			0x05
#define	XR_FBHOSTBASE_31_24			0x06

#define	XR_IOCTL				0x09
#define	XR_IOCTL_AREXTENSIONS			1:1
#define	XR_IOCTL_AREXTENSIONS_DISABLE		0
#define	XR_IOCTL_AREXTENSIONS_ENABLE		1
#define	XR_IOCTL_CREXTENSIONS			0:0
#define	XR_IOCTL_CREXTENSIONS_DISABLE		0
#define	XR_IOCTL_CREXTENSIONS_ENABLE		1

#define	XR_ADDRMAPPING				0x0a
#define	XR_ADDRMAPPING_BIOSMAP			4:4
#define	XR_ADDRMAPPING_BIOSMAP_NORMAL		0
#define	XR_ADDRMAPPING_BIOSMAP_0xA0000		1
#define	XR_ADDRMAPPING_FBMAP			3:3
#define	XR_ADDRMAPPING_FBMAP_LOCAL		0
#define	XR_ADDRMAPPING_FBMAP_MMAP		1
#define	XR_ADDRMAPPING_PACKEDMODE		2:2
#define	XR_ADDRMAPPING_LINEARMODE		1:1
#define	XR_ADDRMAPPING_PAGEMODE			0:0

#define	XR_REGHOSTBASE_23_19			0x0c
#define	XR_REGHOSTBASE_31_24			0x0d
#define	XR_64KPAGESEL				0x0e
#define	XR_SUBSYSVENDORID_LO			0x10
#define	XR_SUBSYSVENDORID_HI			0x11
#define	XR_SUBSYSID_LO				0x12
#define	XR_SUBSYSID_HI				0x13

#define	XR_POWERDOWNCTL				0x16
#define	XR_POWERDOWNCTL_SYNCBYDAC		2:2
#define	XR_POWERDOWNCTL_SYNCBYDAC_NORMAL	0
#define	XR_POWERDOWNCTL_SYNCBYDAC_OVERRIDE	1

#define	XR_BLINKCTL				0x19
#define	XR_BLINKCTL_DUTYCYCLE			7:6
#define	XR_BLINKCTL_DUTYCYCLE_25PCT		1
#define	XR_BLINKCTL_DUTYCYCLE_50PCT		2
#define	XR_BLINKCTL_DUTYCYCLE_75PCT		3
#define	XR_BLINKCTL_RATE			5:0

#define	XR_PINCTL0				0x1a
#define	XR_PINCTL0_PIN8MODE			7:7
#define	XR_PINCTL0_PIN8MODE_WEIRD		0
#define	XR_PINCTL0_PIN8MODE_NORMAL		1
#define	XR_PINCTL0_PINCTL_8_4			4:0

#define	XR_PINCTL1				0x1b

#define	XR_PINCTL2				0x1c
#define	XR_PINCTL2_PINCTL_1			7:6
#define	XR_PINCTL2_PINCTL_1_INPUT		2
#define	XR_PINCTL2_PINCTL_1_OUTPUT		3
#define	XR_PINCTL2_PINCTL_0			4:3
#define	XR_PINCTL2_PINCTL_0_INPUT		2
#define	XR_PINCTL2_PINCTL_0_OUTPUT		3

#define	XR_BLTCTL				0x20
#define	XR_BLTCTL_EXPANDMODE			5:4
#define	XR_BLTCTL_EXPANDMODE_8BPP		0
#define	XR_BLTCTL_EXPANDMODE_16BPP		1
#define	XR_BLTCTL_EXPANDMODE_24BPP		2
#define	XR_BLTCTL_MASTERRESET			1:1
#define	XR_BLTCTL_STATUS			0:0
#define	XR_BLTCTL_STATUS_IDLE			0
#define	XR_BLTCTL_STATUS_BUSY			1

#define	XR_DISPCTL				0x40
#define	XR_DISPCTL_VGAWRAP			1:1
#define	XR_DISPCTL_VGAWRAP_256K			0
#define	XR_DISPCTL_VGAWRAP_DISABLED		1
#define	XR_DISPCTL_NATIVEMODE			0:0
#define	XR_DISPCTL_NATIVEMODE_DISABLE		0
#define	XR_DISPCTL_NATIVEMODE_ENABLE		1

#define	XR_DRAMROWTYPE				0x50
#define	XR_DRAMCTLLO				0x51
#define	XR_DRAMCTLHI				0x52
#define	XR_DRAMCTLX				0x53
#define	XR_DRAMTIMING				0x54
#define	XR_DRAMROWBOUND0			0x55
#define	XR_DRAMROWBOUND1			0x56

#define	XR_VIDPINCTL				0x60
#define	XR_VIDPINCTL_BLANKPOL			5:5
#define	XR_VIDPINCTL_BLANKPOL_NEGATIVE		0
#define	XR_VIDPINCTL_BLANKPOL_POSITIVE		1
#define	XR_VIDPINCTL_VIDCAPTURE			1:0
#define	XR_VIDPINCTL_VIDCAPTURE_DISABLE		0
#define	XR_VIDPINCTL_VIDCAPTURE_ENABLE		3

#define	XR_DPMSSYNCCTL				0x61
#define	XR_DPMSSYNCCTL_VSYNCMODE		3:3
#define	XR_DPMSSYNCCTL_VSYNCMODE_NORMAL		0
#define	XR_DPMSSYNCCTL_VSYNCMODE_DATA		1
#define	XR_DPMSSYNCCTL_VSYNCDATA		2:2
#define	XR_DPMSSYNCCTL_HSYNCMODE		1:1
#define	XR_DPMSSYNCCTL_HSYNCMODE_NORMAL		0
#define	XR_DPMSSYNCCTL_HSYNCMODE_DATA		1
#define	XR_DPMSSYNCCTL_HSYNCDATA		0:0

#define	XR_GPIOCTL				0x62
#define	XR_GPIOCTL_PINCTL_3_2			3:2

#define	XR_GPIODATA0				0x63
#define	XR_GPIODATA0_PINDATA_3			3:3
#define	XR_GPIODATA0_PINDATA_2			2:2
#define	XR_GPIODATA0_PINDATA_3_0		3:0

#define	XR_GPIODATA1				0x64
#define	XR_GPIODATA1_PINMASK_6_4		6:4
#define	XR_GPIODATA1_PINDATA_7_4		3:0

#define	XR_GPIODATA2				0x65
#define	XR_GPIODATA2_PINMASK_8			4:4
#define	XR_GPIODATA2_PINDATA_8			0:0

#define	XR_SYNCTRISTATE				0x67
#define	XR_SYNCTRISTATE_NORMAL			0
#define	XR_SYNCTRISTATE_TRISTATE		1

#define	XR_PIXELCONFIG0				0x80
#define	XR_PIXELCONFIG0_DACWIDTH		7:7
#define	XR_PIXELCONFIG0_DACWIDTH_6BPP		0
#define	XR_PIXELCONFIG0_DACWIDTH_8BPP		1
#define	XR_PIXELCONFIG0_SHOWCURSOR		4:4
#define	XR_PIXELCONFIG0_EXTENDEDREAD		3:3
#define	XR_PIXELCONFIG0_EXTENDEDREAD_DISABLE	0
#define	XR_PIXELCONFIG0_EXTENDEDREAD_ENABLE	1
#define	XR_PIXELCONFIG0_OSCANCOLOR		1:1
#define	XR_PIXELCONFIG0_OSCANCOLOR_DISABLE	0
#define	XR_PIXELCONFIG0_OSCANCOLOR_ENABLE	1
#define	XR_PIXELCONFIG0_PALETTEMODE		0:0
#define	XR_PIXELCONFIG0_PALETTEMODE_NORMAL	0
#define	XR_PIXELCONFIG0_PALETTEMODE_EXTRA8	1

#define	XR_PIXELCONFIG1				0x81
#define	XR_PIXELCONFIG1_VGADELAYS		4:4
#define	XR_PIXELCONFIG1_VGADELAYS_ENABLE	0
#define	XR_PIXELCONFIG1_VGADELAYS_DISABLE	1
#define	XR_PIXELCONFIG1_DISPMODE		3:0
#define	XR_PIXELCONFIG1_DISPMODE_VGA		0
#define	XR_PIXELCONFIG1_DISPMODE_8BPP		2
#define	XR_PIXELCONFIG1_DISPMODE_16BPP_555	4
#define	XR_PIXELCONFIG1_DISPMODE_16BPP_565	5
#define	XR_PIXELCONFIG1_DISPMODE_24BPP		6
#define	XR_PIXELCONFIG1_DISPMODE_32BPP		7

#define	XR_PIXELCONFIG2				0x82
#define	XR_PIXELCONFIG2_FBGAMMA			3:3
#define	XR_PIXELCONFIG2_FBGAMMA_DISABLE		0
#define	XR_PIXELCONFIG2_FBGAMMA_ENABLE		1
#define	XR_PIXELCONFIG2_OVLGAMMA		2:2
#define	XR_PIXELCONFIG2_OVLGAMMA_DISABLE	0
#define	XR_PIXELCONFIG2_OVLGAMMA_ENABLE		1

#define	XR_CURSORCTL				0xa0
#define	XR_CURSORCTL_BLINKEN			7:7
#define	XR_CURSORCTL_ORIGIN			4:4
#define	XR_CURSORCTL_ORIGIN_BORDER		0
#define	XR_CURSORCTL_ORIGIN_NORMAL		1
#define	XR_CURSORCTL_VSIZE			3:3
#define	XR_CURSORCTL_VSIZE_FIXED		0
#define	XR_CURSORCTL_VSIZE_VARIABLE		1
#define	XR_CURSORCTL_MODE			2:0
#define	XR_CURSORCTL_MODE_DISABLED		0
#define	XR_CURSORCTL_MODE_32x32x2_ANDXOR	1
#define	XR_CURSORCTL_MODE_128x128x1_2COLOR	2
#define	XR_CURSORCTL_MODE_128x128x1_1TRANS	3
#define	XR_CURSORCTL_MODE_64x64x2_3TRANS	4
#define	XR_CURSORCTL_MODE_64x64x2_ANDXOR	5
#define	XR_CURSORCTL_MODE_64x64x2_4COLOR	6

#define	XR_CURSORHEIGHT				0xa1

#define	XR_CURSORBASEPTR_LO			0xa2
#define	XR_CURSORBASEPTR_LO_15_12		7:4
#define	XR_CURSORBASEPTR_LO_IMAGESEL		3:0

#define	XR_CURSORBASEPTR_HI			0xa3
#define	XR_CURSORBASEPTR_HI_21_16		5:0

#define	XR_CURSORXPOS_LO			0xa4
#define	XR_CURSORXPOS_HI			0xa5
#define	XR_CURSORXPOS_HI_SIGN			7:7
#define	XR_CURSORXPOS_HI_10_8			2:0

#define	XR_CURSORYPOS_LO			0xa6
#define	XR_CURSORYPOS_HI			0xa7
#define	XR_CURSORYPOS_HI_SIGN			7:7
#define	XR_CURSORYPOS_HI_10_8			2:0

#define	XR_MAINPLL_M				0xb0
#define	XR_MAINPLL_N				0xb1
#define	XR_MAINPLL_PDIV				0xb2

#define	XR_VCLK0_M				0xc0
#define	XR_VCLK0_N				0xc1
#define	XR_VCLK0_MNHI				0xc2
#define	XR_VCLK0_PDIV				0xc3

#define	XR_VCLK1_M				0xc4
#define	XR_VCLK1_N				0xc5
#define	XR_VCLK1_MNHI				0xc6
#define	XR_VCLK1_PDIV				0xc7

#define	XR_VCLK2_M				0xc8
#define	XR_VCLK2_N				0xc9
#define	XR_VCLK2_MNHI				0xca
#define	XR_VCLK2_PDIV				0xcb

/*  All three clocks are formatted identically  */
#define	XR_VCLKx_MNHI_N_9_8			5:4
#define	XR_VCLKx_MNHI_M_9_8			1:0

#define	XR_VCLKx_PDIV_2EXPP			6:4
#define	XR_VCLKx_PDIV_VCODIV			2:2
#define	XR_VCLKx_PDIV_VCODIV_DIV4		0
#define	XR_VCLKx_PDIV_VCODIV_DIV16		1
#define	XR_VCLKx_PDIV_REFCLKDIV			1:1
#define	XR_VCLKx_PDIV_REFCLKDIV_NONE		0
#define	XR_VCLKx_PDIV_REFCLKDIV_DIV5		1
#define	XR_VCLKx_PDIV_REFDIVSEL			0:0
#define	XR_VCLKx_PDIV_REFDIVSEL_DIV4		0
#define	XR_VCLKx_PDIV_REFDIVSEL_DIV1		1

#define	XR_CLKEN1				0xcc
#define	XR_CLKEN2				0xcd

#define	XR_MCLK					0xce
#define	XR_MCLK_FREQ				1:0
#define	XR_MCLK_FREQ_66MHZ			0
#define	XR_MCLK_FREQ_75MHZ			1
#define	XR_MCLK_FREQ_88MHZ			2
#define	XR_MCLK_FREQ_100MHZ			3

#define	XR_MODULEPOWERDOWN			0xd0
#define	XR_MODULEPOWERDOWN_LATCH		6:6
#define	XR_MODULEPOWERDOWN_LATCH_DISABLE	0
#define	XR_MODULEPOWERDOWN_LATCH_ENABLE		1
#define	XR_MODULEPOWERDOWN_MMCAPTURE		5:5
#define	XR_MODULEPOWERDOWN_MMPLAYBACK		4:4
#define	XR_MODULEPOWERDOWN_MCLKPLL		3:3
#define	XR_MODULEPOWERDOWN_VCLKPLL		2:2
#define	XR_MODULEPOWERDOWN_DAC			0:0

#define	XR_SCRATCH0				0xe0


/*****************************************************************************
 * Multimedia Extension register and bit definitions.
 * (Just the registers for supporting overlays.)
 */
#define	MR_OVLCTL				0x1e
#define	MR_OVLCTL_LACEENABLE			4:4
#define	MR_OVLCTL_ZOOMY_ENABLE			3:3
#define	MR_OVLCTL_ZOOMX_ENABLE			2:2
#define	MR_OVLCTL_DIR_Y				1:1
#define	MR_OVLCTL_DIR_Y_NORMAL			0
#define	MR_OVLCTL_DIR_Y_FLIPPED			1
#define	MR_OVLCTL_DIR_X				0:0
#define	MR_OVLCTL_DIR_X_NORMAL			0
#define	MR_OVLCTL_DIR_X_FLIPPED			1

#define	MR_OVLCTL2				0x1f
#define	MR_OVLCTL2_FILTER_Y			7:6
#define	MR_OVLCTL2_FILTER_Y_REPLICATE		0
#define	MR_OVLCTL2_FILTER_Y_FILTERED		2
#define	MR_OVLCTL2_FILTER_Y_RUNNINGAVERAGE	3
#define	MR_OVLCTL2_FILTER_X			5:5
#define	MR_OVLCTL2_FILTER_X_REPLICATE		0
#define	MR_OVLCTL2_FILTER_X_FILTERED		1
#define	MR_OVLCTL2_PIXFMT			3:0
#define	MR_OVLCTL2_PIXFMT_YUV422		0
#define	MR_OVLCTL2_PIXFMT_YUV422_SWAP		1
#define	MR_OVLCTL2_PIXFMT_YUV422_2SCMP		2
#define	MR_OVLCTL2_PIXFMT_YUV422_2SCMP_SWAP	3
#define	MR_OVLCTL2_PIXFMT_RGB565		8
#define	MR_OVLCTL2_PIXFMT_RGB555		9

#define	MR_OVLCTL3				0x20
#define	MR_OVLCTL3_DBUFLOCK			5:5
#define	MR_OVLCTL3_DBUFLOCK_UNLOCKED		0
#define	MR_OVLCTL3_DBUFLOCK_YBOTTOM		1
#define	MR_OVLCTL3_OVLSRC			4:4
#define	MR_OVLCTL3_OVLSRC_PTR1			0
#define	MR_OVLCTL3_OVLSRC_PTR2			1
#define	MR_OVLCTL3_FLIPSRC			3:3
#define	MR_OVLCTL3_FLIPSRC_HW			0
#define	MR_OVLCTL3_FLIPSRC_SW			1
#define	MR_OVLCTL3_DBUFTRIGGER			2:2
#define	MR_OVLCTL3_DBUFTRIGGER_LOADPTR		1

#define	MR_DBUFSTAT				0x21
#define	MR_DBUFSTAT_OVLSRC			1:1
#define	MR_DBUFSTAT_OVLSRC_PTR1			0
#define	MR_DBUFSTAT_OVLSRC_PTR2			1
#define	MR_DBUFSTAT_DBUFTRIGGER			0:0
#define	MR_DBUFSTAT_DBUFTRIGGER_PENDING		1

#define	MR_OVLPTR1_7_3				0x22
#define	MR_OVLPTR1_15_8				0x23
#define	MR_OVLPTR1_23_16			0x24

#define	MR_OVLPTR2_7_3				0x25
#define	MR_OVLPTR2_15_8				0x26
#define	MR_OVLPTR2_23_16			0x27

#define	MR_OVLPITCH				0x28	/* (bytes / 8) - 1 */

#define	MR_OVL_XLEFT_7_0			0x2a
#define	MR_OVL_XLEFT_10_8			0x2b
#define	MR_OVL_XRIGHT_7_0			0x2c
#define	MR_OVL_XRIGHT_10_8			0x2d
#define	MR_OVL_YTOP_7_0				0x2e
#define	MR_OVL_YTOP_10_8			0x2f
#define	MR_OVL_YBOT_7_0				0x30
#define	MR_OVL_YBOT_10_8			0x31

#define	MR_ZOOMX				0x32
#define	MR_ZOOMX_ZOOMFRAC			7:2

#define	MR_ZOOMY				0x33
#define	MR_ZOOMY_ZOOMFRAC			7:2

#define	MR_OVLKEYCRL				0x3c
#define	MR_OVLKEYCRL_LSBDISABLE			7:7	/*  WTF?  */
#define	MR_OVLKEYCRL_LSBDISABLE_NORMAL		0
#define	MR_OVLKEYCRL_LSBDISABLE_DISABLE		1
#define	MR_OVLKEYCRL_MSBKEY			6:6	/*  WTF?  */
#define	MR_OVLKEYCRL_MSBKEY_NORMAL		0
#define	MR_OVLKEYCRL_MSBKEY_G7toB0		1
#define	MR_OVLKEYCRL_BLANKDISP			5:5
#define	MR_OVLKEYCRL_RECTENABLE			2:2
#define	MR_OVLKEYCRL_RECTENABLE_testmode	0
#define	MR_OVLKEYCRL_CKEYENABLE			1:1
#define	MR_OVLKEYCRL_OVLENABLE			0:0

#define	MR_OVLCKEY_RED				0x3d
#define	MR_OVLCKEY_GREEN			0x3e
#define	MR_OVLCKEY_BLUE				0x3f

#define	MR_OVLCKEYMASK_RED			0x40
#define	MR_OVLCKEYMASK_GREEN			0x41
#define	MR_OVLCKEYMASK_BLUE			0x42

#define	MR_CURSCANLINE_7_0			0x43
#define	MR_CURSCANLINE_11_8			0x44

#define	MR_OVLPTR1_31_24			0x50
#define	MR_OVLPTR2_31_42			0x51


/*****************************************************************************
 * Interrupt and Control register bit definitions.
 * The Intel document is inconsistent about its own names.  The names chosen
 * are semi-arbitrary.  The comments identify the other names given for the
 * same register in the Intel document (where applicable).
 */
/*  LMFT  */
#define	LMFT_PITCH				28:24
#define	LMFT_2KPAGE_UPPER			23:12
#define	LMFT_2KPAGE_LOWER			11:0

/*  INPEIR/CPEIR  */
#define	INPEIR_ERRSRC				17:17
#define	INPEIR_ERRSRC_FIFO			0
#define	INPEIR_ERRSRC_DMA			1
#define	INPEIR_PRISRC				16:16
#define	INPEIR_PRISRC_LP			0
#define	INPEIR_PRISRC_HP			1
#define	INPEIR_ERRID				15:12
#define	INPEIR_CLIENT				11:9
#define	INPEIR_FUNC				8:7
#define	INPEIR_TARGET				6:0

/*  INSTMASK/INSTRMASK/CSTRMASK  */
#define	INSTMASK_SBLTBUSY			4:4
#define	INSTMASK_3DBUSY				3:3
#define	INSTMASK_DMADONE			2:2
#define	INSTMASK_HPFIFODONE			1:1
#define	INSTMASK_LPFIFODONE			0:0

/*  INPOINT/INSTPNT  */
#define	INPOINT_HPFIFOCOUNT			15:8
#define	INPOINT_LPFIFO				7:0

/*  IER/IIR/IMR/ISR  */
#define	IxR_VMIINTB				13:13
#define	IxR_GPIO4				12:12
#define	IxR_PAGEFLIP				11:11
#define	IxR_DVDDMADONE				10:10
#define	IxR_VBLANK				9:9
#define	IxR_INSTRDONE				8:8
#define	IxR_SYNTAXERROR				7:7
#define	IxR_USER				6:6
#define	IxR_BREAKPOINT				5:5
#define	IxR_HCOUNT_DISP				4:4
#define	IxR_VSYNC				3:3
#define	IxR_HCOUNT_CAPTURE			2:2
#define	IxR_VSYNC_CAPTURE			1:1
#define	IxR_PIPEFLUSHED				0:0
#define	IxR__ALLIRQS				13:0

/*  INPMODE/COMPARS  */
#define	INPMODE_2D				3:3
#define	INPMODE_2D_ENABLE			0
#define	INPMODE_2D_DISABLE			1
#define	INPMODE_3D				2:2
#define	INPMODE_3D_ENABLE			0
#define	INPMODE_3D_DISABLE			1
#define	INPMODE_STATEVARS			1:1
#define	INPMODE_STATEVARS_ENABLE		0
#define	INPMODE_STATEVARS_DISABLE		1
#define	INPMODE_PALETTE				0:0
#define	INPMODE_PALETTE_ENABLE			0
#define	INPMODE_PALETTE_DISABLE			1

/*  FWATER_BLC  */
#define	FWATERBLC_LMI_BURSTLEN			30:24
#define	FWATERBLC_LMI_LOWATER			21:16
#define	FWATERBLC_AGP_BURSTLEN			14:8
#define	FWATERBLC_AGP_LOWATER			5:0


/*****************************************************************************
 * Blitter and Stretch Blitter register bit definitions.
 */
/*  BLT_Pitches/BR00  */
#define	BLT_PITCHES_DEST			28:16
#define	BLT_PITCHES_SRC				12:0

/*  BLT_MonoSrcCtl/BR03  */
#define	BLT_MONOSRCCTL_MONOSRC			27:27
#define	BLT_MONOSRCCTL_MONOSRC_PATTERN		0
#define	BLT_MONOSRCCTL_MONOSRC_IMAGE		1
#define	BLT_MONOSRCCTL_MONOALIGN		28:24
#define	BLT_MONOSRCCTL_MONOALIGN_1BIT		1
#define	BLT_MONOSRCCTL_MONOALIGN_8BIT		2
#define	BLT_MONOSRCCTL_MONOALIGN_16BIT		3
#define	BLT_MONOSRCCTL_MONOALIGN_32BIT		4
#define	BLT_MONOSRCCTL_MONOALIGN_64BIT		5
#define	BLT_MONOSRCCTL_CHOPINITIAL		21:16
#define	BLT_MONOSRCCTL_CLIPRIGHT		13:8
#define	BLT_MONOSRCCTL_CLIPLEFT			5:0

/*  BLT_BlitCtl/BR04  */
#define	BLT_BLTCTL_BUSY				31:31
#define	BLT_BLTCTL_DEPTH			25:24
#define	BLT_BLTCTL_DEPTH_8BPP			0
#define	BLT_BLTCTL_DEPTH_16BPP			1
#define	BLT_BLTCTL_DEPTH_24BPP			2
#define	BLT_BLTCTL_DEPTH_32BPP			3
#define	BLT_BLTCTL_USELOCALDEPTH		23:23
#define	BLT_BLTCTL_PATVALIGN			22:20
#define	BLT_BLTCTL_PATSRC			19:19
#define	BLT_BLTCTL_PATSRC_PATTERN		0
#define	BLT_BLTCTL_PATSRC_ZERO			1
#define	BLT_BLTCTL_PATTYPE			18:18
#define	BLT_BLTCTL_PATTYPE_COLOR		0
#define	BLT_BLTCTL_PATTYPE_MONO			1
#define	BLT_BLTCTL_PATISWRITEMASK		17:17
#define	BLT_BLTCTL_CKEYMODE			16:14
#define	BLT_BLTCTL_CKEYMODE_NONE		0
#define	BLT_BLTCTL_CKEYMODE_SRCMATCHNOT		1
#define	BLT_BLTCTL_CKEYMODE_DESTMATCHNOT	3
#define	BLT_BLTCTL_CKEYMODE_SRCMATCH		5
#define	BLT_BLTCTL_CKEYMODE_DESTMATCH		7
#define	BLT_BLTCTL_SRCISWRITEMASK		13:13
#define	BLT_BLTCTL_SRCTYPE			12:12
#define	BLT_BLTCTL_SRCTYPE_COLOR		0
#define	BLT_BLTCTL_SRCTYPE_MONO			1
#define	BLT_BLTCTL_SRC				10:10
#define	BLT_BLTCTL_SRC_MEM			0
#define	BLT_BLTCTL_SRC_CPUFIFO			1
#define	BLT_BLTCTL_YINCDIR			9:9
#define	BLT_BLTCTL_YINCDIR_DOWNWARD		0
#define	BLT_BLTCTL_YINCDIR_UPWARD		1
#define	BLT_BLTCTL_XINCDIR			8:8
#define	BLT_BLTCTL_XINCDIR_RIGHTWARD		0
#define	BLT_BLTCTL_XINCDIR_LEFTWARD		1
#define	BLT_BLTCTL_MINTERM			7:0

/*  BLT_DstSize/BR08  */
#define	BLT_DSTSIZE_HEIGHT			28:16
#define	BLT_DSTSIZE_BYTEWIDTH			12:0


/*  SBLT_SrcWidth/BR11  */
#define	SBLT_SRCWIDTH_BYTES			31:16
#define	SBLT_SRCWIDTH_PIXELS			15:0

/*  SBLT_SrcHeightCtl/BR12  */
#define	SBLT_SRCHEIGHTCTL_CMPCTL		27:24
#define	SBLT_SRCHEIGHTCTL_CMPCTL_NEVER		0
#define	SBLT_SRCHEIGHTCTL_CMPCTL_OUTSIDE	1
#define	SBLT_SRCHEIGHTCTL_CMPCTL_INSIDE		4
#define	SBLT_SRCHEIGHTCTL_CMPCTL_ALWAYS		5
#define	SBLT_SRCHEIGHTCTL_VRESIZE		23:22
#define	SBLT_SRCHEIGHTCTL_VRESIZE_OFF		0
#define	SBLT_SRCHEIGHTCTL_VRESIZE_EXPAND	1
#define	SBLT_SRCHEIGHTCTL_VRESIZE_REDUCE	2
#define	SBLT_SRCHEIGHTCTL_HRESIZE		21:20
#define	SBLT_SRCHEIGHTCTL_HRESIZE_OFF		0
#define	SBLT_SRCHEIGHTCTL_HRESIZE_EXPAND	1
#define	SBLT_SRCHEIGHTCTL_HRESIZE_REDUCE	2
#define	SBLT_SRCHEIGHTCTL_SRC			19:19
#define	SBLT_SRCHEIGHTCTL_SRC_LOCALMEM		0
#define	SBLT_SRCHEIGHTCTL_SRC_HOSTMEM		1
#define	SBLT_SRCHEIGHTCTL_SRCFMT		18:16
#define	SBLT_SRCHEIGHTCTL_SRCFMT_8BPP		0
#define	SBLT_SRCHEIGHTCTL_SRCFMT_16BPP_555	1
#define	SBLT_SRCHEIGHTCTL_SRCFMT_16BPP_565	2
#define	SBLT_SRCHEIGHTCTL_SRCFMT_24BPP		3
#define	SBLT_SRCHEIGHTCTL_SRCFMT_32BPP		4
#define	SBLT_SRCHEIGHTCTL_HEIGHT		15:0

/*  SBLT_DstWidth/BR14  */
#define	SBLT_DSTWIDTH_BYTES			31:16
#define	SBLT_DSTWIDTH_PIXELS			15:0

/*  SBLT_DstHeightCtl/BR15  */
#define	SBLT_DSTHEIGHTCTL_FILLMODE		27:24
#define	SBLT_DSTHEIGHTCTL_FILLMODE_COPY		10
#define	SBLT_DSTHEIGHTCTL_FILLMODE_SOLID	15
#define	SBLT_DSTHEIGHTCTL_DSTFMT		23:16
#define	SBLT_DSTHEIGHTCTL_DSTFMT_8BPP		0
#define	SBLT_DSTHEIGHTCTL_DSTFMT_16BPP_555	1
#define	SBLT_DSTHEIGHTCTL_DSTFMT_16BPP_565	2
#define	SBLT_DSTHEIGHTCTL_DSTFMT_24BPP		3
#define	SBLT_DSTHEIGHTCTL_DSTFMT_32BPP		4
#define	SBLT_DSTHEIGHTCTL_HEIGHT		15:0

/*  SBLT_HFactor/BR16  */
#define	SBLT_HFACTOR_SPANBYTES			28:24
#define	SBLT_HFACTOR_FACTOR			19:2	/*  4.14 FP  */

/*  SBLT_VFactor/BR17  */
#define	SBLT_VFACTOR_STARTA			26:24
#define	SBLT_VFACTOR_FACTOR			19:2	/*  4.14 FP  */

/*  SBLT_DDAPhase/BR18  */
#define	SBLT_DDAPHASE_H				31:16
#define	SBLT_DDAPHASE_V				15:0

/*  SBLT_Status/BR19  */
#define	SBLT_STATUS_BUSY			0:0

/*  SBLT_SrcMask/BR1A  */
#define	SBLT_SRCMASK_RED			23:16
#define	SBLT_SRCMASK_GREEN			15:8
#define	SBLT_SRCMASK_BLUE			7:0

/*  SBLT_SrcCmpColorMin/BR1B  */
#define	SBLT_SRCCMPCOLORMIN_FILLHIGH		31:24
#define	SBLT_SRCCMPCOLORMIN_RED			23:16
#define	SBLT_SRCCMPCOLORMIN_GREEN		15:8
#define	SBLT_SRCCMPCOLORMIN_BLUE		7:0

/*  SBLT_SrcCmpColorMax/BR1C  */
#define	SBLT_SRCCMPCOLORMAX_RED			23:16
#define	SBLT_SRCCMPCOLORMAX_GREEN		15:8
#define	SBLT_SRCCMPCOLORMAX_BLUE		7:0

/*  SBLT_DstCmpColorMin/BR1D  */
#define	SBLT_DSTCMPCOLORMIN_RED			23:16
#define	SBLT_DSTCMPCOLORMIN_GREEN		15:8
#define	SBLT_DSTCMPCOLORMIN_BLUE		7:0

/*  SBLT_DstCmpColorMax/BR1E  */
#define	SBLT_DSTCMPCOLORMAX_RED			23:16
#define	SBLT_DSTCMPCOLORMAX_GREEN		15:8
#define	SBLT_DSTCMPCOLORMAX_BLUE		7:0

/*  SBLT_Start/BR1F  */
#define	SBLT_START_STARTB			26:24


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
#define	I7INST_CLIENT				31:29
#define	I7INST_FUNCTION				28:23
#define	I7INST_TARGET				22:16
#define	I7INST_DATA				15:0

/*  NOP fields are a special case  */
#define	I7INST_NOPCODE				28:24
#define	I7INST_NOPID				23:0

#define	I7CLI_RENDERER				0
#define	I7CLI_STATE				1
#define	I7CLI_PALETTE				2
#define	I7CLI_2D				3
#define	I7CLI_INST				4
#define	I7CLI_NOP				5

/*
 * Instruction-building macros.
 */
#define	MKINS(client,func,target)	(SetBF (I7INST_CLIENT, (client)) | \
					 SetBF (I7INST_FUNCTION, (func)) | \
					 SetBF (I7INST_TARGET, (target)))
#define	MKINSD(client,func,target,data)	(SetBF (I7INST_CLIENT, (client)) | \
					 SetBF (I7INST_FUNCTION, (func)) | \
					 SetBF (I7INST_TARGET, (target)) | \
					 SetBF (I7INST_DATA, (data)))


#define	GFXTLTRIANGLE				MKINS (I7CLI_RENDERER, 0, 0x00)
#define	GFXTLLINE				MKINS (I7CLI_RENDERER, 0, 0x01)

#define	GFXRENDERSTATE_DITHER_ENABLE		MKINS (I7CLI_STATE, 0, 0x00)
#define	GFXRENDERSTATE_EDGE_ANTIALIAS		MKINS (I7CLI_STATE, 0, 0x01)
#define	GFXRENDERSTATE_MONOCHROME		MKINS (I7CLI_STATE, 0, 0x02)
#define	GFXRENDERSTATE_Z_ENABLE			MKINS (I7CLI_STATE, 0, 0x03)
#define	GFXRENDERSTATE_Z_WRITE_ENABLE		MKINS (I7CLI_STATE, 0, 0x04)
#define	GFXRENDERSTATE_ALPHA_TEST_ENABLE	MKINS (I7CLI_STATE, 0, 0x05)
#define	GFXRENDERSTATE_BLEND_ENABLE		MKINS (I7CLI_STATE, 0, 0x06)
#define	GFXRENDERSTATE_FOG_ENABLE		MKINS (I7CLI_STATE, 0, 0x07)
#define	GFXRENDERSTATE_SPECULAR_ENABLE		MKINS (I7CLI_STATE, 0, 0x08)
#define	GFXRENDERSTATE_STIPPLE_ENABLE		MKINS (I7CLI_STATE, 0, 0x09)
#define	GFXRENDERSTATE_TEXTURE_LOD_DITHER_WEIGHT	\
						MKINS (I7CLI_STATE, 0, 0x0a)
#define	GFXRENDERSTATE_DST_ALP_IN_Z_BUF		MKINS (I7CLI_STATE, 0, 0x0b)
#define	GFXRENDERSTATE_STIPPLE_ALPHA		MKINS (I7CLI_STATE, 0, 0x0c)
#define	GFXRENDERSTATE_TEXTURE_ENABLE		MKINS (I7CLI_STATE, 0, 0x0d)
#define	GFXRENDERSTATE_WRAP_U			MKINS (I7CLI_STATE, 0, 0x0e)
#define	GFXRENDERSTATE_WRAP_V			MKINS (I7CLI_STATE, 0, 0x0f)
#define	GFXRENDERSTATE_TEX_MIN			MKINS (I7CLI_STATE, 0, 0x10)
#define	GFXRENDERSTATE_TEX_MAG			MKINS (I7CLI_STATE, 0, 0x11)
#define	GFXRENDERSTATE_TEX_MAP_BLEND		MKINS (I7CLI_STATE, 0, 0x12)
#define	GFXRENDERSTATE_TEXTURE_LOD_BIAS		MKINS (I7CLI_STATE, 0, 0x13)
#define	GFXRENDERSTATE_BOUNDING_BOX_EXPANSION	MKINS (I7CLI_STATE, 0, 0x14)
#define	GFXRENDERSTATE_SRC_BLEND		MKINS (I7CLI_STATE, 0, 0x15)
#define	GFXRENDERSTATE_DST_BLEND		MKINS (I7CLI_STATE, 0, 0x16)
#define	GFXRENDERSTATE_CULL_MODE		MKINS (I7CLI_STATE, 0, 0x17)
#define	GFXRENDERSTATE_Z_FUNC			MKINS (I7CLI_STATE, 0, 0x18)
#define	GFXRENDERSTATE_ALPHA_REF		MKINS (I7CLI_STATE, 0, 0x19)
#define	GFXRENDERSTATE_ALPHA_FUNC		MKINS (I7CLI_STATE, 0, 0x1a)
#define	GFXRENDERSTATE_FILL_MODE		MKINS (I7CLI_STATE, 0, 0x1b)
#define	GFXRENDERSTATE_SHADE_MODE		MKINS (I7CLI_STATE, 0, 0x1c)
#define	GFXRENDERSTATE_Z_BIAS			MKINS (I7CLI_STATE, 0, 0x1d)
#define	GFXRENDERSTATE_LINE_WIDTH		MKINS (I7CLI_STATE, 0, 0x1e)
#define	GFXRENDERSTATE_POLY_EDGE_OFFSET		MKINS (I7CLI_STATE, 0, 0x1f)
#define	GFXRENDERSTATE_LINE_AA_REGION		MKINS (I7CLI_STATE, 0, 0x20)
#define	GFXRENDERSTATE_POLY_AA_REGION		MKINS (I7CLI_STATE, 0, 0x21)
#define	GFXRENDERSTATE_COLOR_KEY_ENA		MKINS (I7CLI_STATE, 0, 0x22)
#define	GFXRENDERSTATE_CHROMA_KEY_HVAL		MKINS (I7CLI_STATE, 0, 0x23)
#define	GFXRENDERSTATE_CHROMA_KEY_LVAL		MKINS (I7CLI_STATE, 0, 0x24)
#define	GFXRENDERSTATE_TEXT_LOD_ROT_VAR		MKINS (I7CLI_STATE, 0, 0x25)
#define	GFXRENDERSTATE_COLOR_INDEX_KEY		MKINS (I7CLI_STATE, 0, 0x26)
#define	GFXRENDERSTATE_TEXTURE_MAP_INFO		MKINS (I7CLI_STATE, 1, 0x00)
#define	GFXRENDERSTATE_BACK_BUFFER_INFO		MKINS (I7CLI_STATE, 1, 0x01)
#define	GFXRENDERSTATE_FRONT_BUFFER_INFO	MKINS (I7CLI_STATE, 1, 0x02)
#define	GFXRENDERSTATE_Z_BUFFER_INFO		MKINS (I7CLI_STATE, 1, 0x03)
#define	GFXRENDERSTATE_FOG_COLOR		MKINS (I7CLI_STATE, 1, 0x04)

#define	GFXPALSTIPPROC_STIPPLE_PATTERN		MKINS (I7CLI_PALETTE, 0, 0x00)
#define	GFXPALSTIPPROC_TEXTURE_PALETTE_LOAD	MKINS (I7CLI_PALETTE, 0, 0x01)

#define	GFX2DOPREG_BLTER_FULL_LOAD		MKINSD (I7CLI_2D, 0, 0x00, 0x0a)
#define	GFX2DOPREG_BLTER_COMMON_BLT		MKINSD (I7CLI_2D, 0, 0x01, 0x07)
#define	GFX2DOPREG_BLTER_TEXT_BLT		MKINSD (I7CLI_2D, 0, 0x02, 0x03)
#define	GFX2DOPREG_BLTER_SCANLINE_BLT		MKINSD (I7CLI_2D, 0, 0x03, 0x02)
#define	GFX2DOPREG_BLTER_SETUP_BLT		MKINSD (I7CLI_2D, 0, 0x04, 0x04)
#define	GFX2DOPREG_BLTER_IMMEDIATE(n)		MKINSD (I7CLI_2D, 0, 0x05, (n))
#define	GFX2DOPREG_BLTER_FULL_PCI		MKINSD (I7CLI_2D, 0, 0x06, 0x0a)
#define	GFX2DOPREG_BLTER_PCI			MKINSD (I7CLI_2D, 0, 0x07, 0x03)
#define	GFX2DOPREG_STRETCH_BLTER_FULL_LOAD	MKINSD (I7CLI_2D, 1, 0x00, 0x0f)
#define	GFX2DOPREG_STRETCH_BLTER_COMMON_BLT	MKINSD (I7CLI_2D, 1, 0x01, 0x07)

#define	GFXCMDPARSER_STORE_DWORD		MKINS (I7CLI_INST, 0, 0x00)
#define	GFXCMDPARSER_WAIT_ON_VBLANK		MKINS (I7CLI_INST, 0, 0x01)
#define	GFXCMDPARSER_BATCH_BUFFER		MKINS (I7CLI_INST, 0, 0x02)
#define	GFXCMDPARSER_BREAKPOINT_INTERRUPT	MKINS (I7CLI_INST, 0, 0x03)
#define	GFXCMDPARSER_USER_INTERRUPT		MKINS (I7CLI_INST, 0, 0x04)
#define	GFXCMDPARSER_AGP_FLUSH			MKINS (I7CLI_INST, 0, 0x07)
#define	GFXCMDPARSER_FLUSH_PIPE			MKINS (I7CLI_INST, 0, 0x08)
#define	GFXCMDPARSER_WAIT_ON_FLIP_PENDING	MKINS (I7CLI_INST, 0, 0x09)

#define	GFX_NOP					MKINS (I7CLI_NOP, 0, 0x00)

#define	GFX_NOP_ID(id)		(SetBF (I7INST_CLIENT, I7CLI_NOP) | \
				 SetBF (I7INST_NOPCODE, 1) | \
				 SetBF (I7INST_NOPID, (id)))


/*****************************************************************************
 * Blitter and Stretch Blitter instruction structures.
 */
typedef struct BltFullLoad {
	uint32	BLT_Instruction;	/*  GFX2DOPREG_BLTER_FULL_LOAD  */
					/* also: GFX2DOPREG_BLTER_FULL_PCI  */
	uint32	BLT_Pitches;
	uint32	BLT_PatBGColor;
	uint32	BLT_PatFGColor;
	uint32	BLT_MonoSrcCtl;
	uint32	BLT_BlitCtl;
	uint32	BLT_PatSrcAddr;
	uint32	BLT_SrcAddr;
	uint32	BLT_DstAddr;
	uint32	BLT_SrcBGColor;
	uint32	BLT_SrcFGColor;
	uint32	BLT_DstSize;
} BltFullLoad;

typedef struct BltCommonBlt {
	uint32	BLT_Instruction;	/*  GFX2DOPREG_BLTER_COMMON_BLT  */
	uint32	BLT_Pitches;
	uint32	BLT_PatBGColor;
	uint32	BLT_PatFGColor;
	uint32	BLT_BlitCtl;
	uint32	BLT_PatSrcAddr;
	uint32	BLT_SrcAddr;
	uint32	BLT_DstAddr;
	uint32	BLT_DstSize;
} BltCommonBlt;

typedef struct BltTextBlt {
	uint32	BLT_Instruction;	/*  GFX2DOPREG_BLTER_TEXT_BLT  */
					/* also: GFX2DOPREG_BLTER_PCI  */
	uint32	BLT_MonoSrcCtl;
	uint32	BLT_SrcAddr;
	uint32	BLT_DstAddr;
	uint32	BLT_DstSize;
} BltTextBlt;

typedef struct BltSetup {
	uint32	BLT_Instruction;	/*  GFX2DOPREG_BLTER_SETUP_BLT  */
	uint32	BLT_Pitches;
	uint32	BLT_PatBGColor;
	uint32	BLT_PatFGColor;
	uint32	BLT_BlitCtl;
	uint32	BLT_PatSrcAddr;
} BltSetup;

typedef struct BltScanlineBlt {
	uint32	BLT_Instruction;	/*  GFX2DOPREG_BLTER_SCANLINE_BLT  */
	uint32	BLT_BlitCtl;
	uint32	BLT_DstAddr;
	uint32	BLT_DstSize;
} BltScanlineBlt;

typedef struct SBltFullLoad {
	uint32	SBLT_Instruction;	/* GFX2DOPREG_STRETCH_BLTER_FULL_LOAD */
	uint32	SBLT_SrcAddr;
	uint32	SBLT_SrcWidth;
	uint32	SBLT_SrcHeightCtl;
	uint32	SBLT_DstAddr;
	uint32	SBLT_DstWidth;
	uint32	SBLT_DstHeightCtl;
	uint32	SBLT_HFactor;
	uint32	SBLT_VFactor;
	uint32	SBLT_DDAPhase;
	uint32	SBLT_Status;
	uint32	SBLT_SrcMask;
	uint32	SBLT_SrcCmpColorMin;
	uint32	SBLT_SrcCmpColorMax;
	uint32	SBLT_DstCmpColorMin;
	uint32	SBLT_DstCmpColorMax;
	uint32	SBLT_Start;
} SBltFullLoad;

typedef struct SBltCommonBlt {
	uint32	SBLT_Instruction;	/*GFX2DOPREG_STRETCH_BLTER_COMMON_BLT*/
	uint32	SBLT_SrcAddr;
	uint32	SBLT_SrcWidth;
	uint32	SBLT_SrcHeightCtl;
	uint32	SBLT_DstAddr;
	uint32	SBLT_DstWidth;
	uint32	SBLT_DstHeightCtl;
	uint32	SBLT_HFactor;
	uint32	SBLT_VFactor;
} SBltCommonBlt;




#endif	/*  __I740_H  */
