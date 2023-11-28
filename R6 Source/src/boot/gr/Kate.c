/* ++++++++++
	FILE:	Kate.c
	REVS:	$Revision: 1.2 $
	NAME:	benoit
	DATE:	Fri Aug 18 14:10:23 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include	"gr.h"


static unsigned char Kate_bitmap[] = { 
0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x20,0x04,0xd5,
0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,
0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x21,0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x70,0x00,0x50,0x09,0x60,0x00,0x44,0x20,0x1a,0x60,0x09,0x04,0x03,
0x20,0xd1,0x20,0x80,0x68,0x90,0x40,0x06,0x03,0x22,0x07,0x98,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x12,
0xd6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xd5,0x39,0xec,0x98,0x40,0x00,
0x0b,0x97,0x38,0x5f,0x77,0xdc,0xe0,0x00,0x1c,0xe7,0x79,0xde,0xff,0xdd,0x1e,0x18,
0xc2,0x31,0x77,0x9d,0xe7,0x7e,0x31,0x8c,0x63,0xfe,0x34,0x04,0x08,0x00,0x20,0x30,
0x42,0x61,0x80,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0xa9,0xae,0x73,0xbf,0x17,
0x44,0x82,0x52,0xac,0x90,0x10,0x8a,0x54,0xdb,0x62,0x11,0x4a,0xb1,0x08,0xa5,0x29,
0x04,0x92,0x0a,0xa7,0x9c,0xf5,0xa0,0x7b,0x80,0x03,0x22,0x04,0xff,0xc0,0x47,0x67,
0x00,0x04,0x80,0x04,0x00,0x00,0x00,0x39,0xce,0x78,0x00,0x0a,0xae,0x00,0x28,0x07,
0x80,0x00,0xdf,0xd6,0xb2,0xa5,0x52,0x00,0x0c,0x78,0xc4,0xd0,0x80,0x63,0x10,0x41,
0x23,0x18,0xc6,0x31,0x84,0x23,0x14,0x19,0x43,0x79,0x8c,0x63,0x18,0x92,0x31,0x8a,
0xa2,0x1a,0x1a,0x02,0x08,0x00,0x20,0x40,0x40,0x20,0x80,0x00,0x00,0x00,0x02,0x00,
0x00,0x00,0x00,0xaa,0xd1,0x8c,0x61,0x98,0xc4,0x00,0x00,0x00,0x60,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x79,0x24,0x28,0xea,0xc4,0x42,0x5e,0x02,0xa4,
0x40,0x44,0x94,0x02,0x4a,0x80,0x49,0x98,0x80,0x00,0x00,0x64,0x09,0x00,0x00,0x46,
0x31,0xa0,0x00,0x14,0xb2,0x42,0x03,0xf4,0x80,0x00,0x85,0x52,0xd4,0x42,0xe2,0x00,
0x14,0x50,0x85,0x5e,0xf0,0x63,0x1a,0xbe,0x83,0xd8,0xc6,0x11,0x84,0x21,0x14,0x1a,
0x42,0xb5,0x8c,0x63,0x18,0x12,0x31,0x89,0x22,0x29,0x10,0x00,0xff,0x39,0xee,0xe7,
0xfa,0x64,0xbd,0x67,0x79,0xf6,0x7f,0x46,0x35,0x8c,0x7e,0xa8,0x11,0x8c,0x21,0x58,
0xc5,0xef,0x7b,0xde,0xf7,0x39,0xce,0x74,0xa5,0x67,0x39,0xce,0x74,0x63,0x18,0xa6,
0x77,0x25,0xfa,0xcb,0x2d,0x5c,0x1f,0xa4,0xf6,0x48,0x7f,0x23,0x22,0xbe,0x49,0x98,
0xb9,0xc4,0x80,0x4e,0x71,0x00,0x00,0x46,0x31,0xa2,0x80,0x15,0x5c,0x05,0x44,0x04,
0x80,0x00,0x8f,0xb9,0x48,0x43,0x5f,0x9f,0x15,0x51,0x1a,0x41,0x88,0x9d,0x11,0x00,
0x45,0x5f,0xfa,0x11,0xf7,0xa7,0xf4,0x1c,0x42,0x33,0x8f,0xa3,0xe7,0x12,0x31,0x89,
0x14,0x49,0x10,0x01,0x18,0xc6,0x31,0x48,0xc6,0x68,0xab,0x98,0xc6,0x39,0x82,0x46,
0x35,0x54,0x45,0x24,0x1f,0xfc,0x3d,0x38,0xc6,0x31,0x8c,0x63,0x18,0xc6,0x31,0x8c,
0xa5,0x98,0xc6,0x31,0x8c,0x63,0x18,0xa0,0xac,0x13,0xf7,0xca,0xb1,0x00,0x04,0xf5,
0x6b,0xf4,0x89,0x25,0x12,0x94,0x47,0x68,0xd6,0x68,0xfc,0x84,0x8a,0x93,0x20,0x7f,
0xf1,0xb5,0x7f,0xe0,0x01,0xf8,0xc7,0xf4,0x80,0x00,0x85,0x15,0xb5,0x42,0x42,0x00,
0x24,0x52,0x07,0xe1,0x89,0x22,0xf0,0xbe,0x89,0xe8,0xc6,0x11,0x84,0x23,0x15,0x1a,
0x42,0x31,0x8c,0x23,0x10,0x92,0x31,0xa9,0x08,0x88,0x90,0x01,0x18,0xc2,0x3f,0x48,
0xc6,0x78,0xab,0x18,0xc6,0x30,0x72,0x46,0x35,0x24,0x48,0xa8,0x11,0x8c,0x21,0x18,
0xc6,0x31,0x8c,0x63,0x18,0x7f,0xff,0xfc,0xa5,0x18,0xc6,0x31,0x8c,0x63,0x18,0xa0,
0xa4,0x0b,0xf2,0xc7,0x2d,0x00,0x1f,0xa6,0x76,0x43,0x3f,0x29,0x22,0x94,0x40,0x08,
0xde,0xb0,0x86,0x84,0x72,0xa4,0x90,0x46,0x31,0xa5,0xc0,0x00,0x00,0x05,0x44,0x04,
0x80,0x00,0x00,0x56,0xb2,0x24,0x02,0x00,0x24,0x54,0x44,0x51,0x89,0x22,0x10,0x41,
0x01,0x08,0xc6,0x31,0x84,0x23,0x15,0x19,0x42,0x31,0x8c,0x23,0x18,0x92,0x2a,0xda,
0x89,0x08,0x90,0x01,0x38,0xc2,0x30,0x48,0xc6,0x64,0xab,0x18,0xc6,0x30,0x0a,0x4d,
0x55,0x54,0x50,0xa8,0x11,0x8c,0x61,0x18,0xc6,0x73,0x9c,0xe7,0x38,0x42,0x10,0x84,
0xa5,0x18,0xc6,0x31,0x8c,0xe7,0x39,0x80,0xac,0x64,0xe2,0xc6,0xc2,0x00,0x08,0xa4,
0x40,0x40,0x09,0x29,0x4a,0x94,0x4f,0xf5,0x53,0x31,0x85,0x04,0x84,0x64,0x90,0x46,
0x31,0xa5,0x00,0x00,0x00,0x42,0x47,0xf4,0x80,0x00,0x80,0x3a,0x4d,0x18,0x00,0x20,
0xc3,0x9f,0xb8,0x4e,0x71,0x1c,0xea,0x00,0x08,0xe8,0xf9,0xde,0xfc,0x1d,0x1e,0xe8,
0xfe,0x31,0x74,0x1d,0x17,0x11,0xc4,0x8c,0x49,0xfc,0x71,0xf8,0xdf,0x3d,0xef,0x47,
0xc6,0x63,0xeb,0x17,0x79,0xf0,0xf1,0xb4,0x8a,0x8b,0xfe,0xa8,0x11,0x8b,0xbf,0x17,
0x39,0xad,0x6b,0x5a,0xd7,0xbd,0xef,0x7c,0xa5,0x17,0x39,0xce,0x73,0x5a,0xd6,0x80,
0x77,0xa2,0x02,0xd8,0x1c,0x00,0x00,0xbb,0x81,0xff,0xc9,0xd6,0xfa,0x94,0x40,0x0d,
0xbd,0xce,0x81,0x04,0x07,0xd3,0x2a,0xc6,0x2e,0x7a,0xc0,0x00,0x00,0x00,0x3c,0x07,
0x80,0x00,0x00,0x10,0x00,0x00,0x00,0x20,0x40,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,
0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x80,0x40,0x00,0x00,0x40,0x20,0x00,0x00,
0x00,0x00,0x40,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x1c,0x00,0x40,0x00,0x00,0x00,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,
0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,
0x01,0x80,0x00,0x00,0x40,0x20,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x01,0x80,0x00,
0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x00,
0x00,0x00
};

static short Kate_offsets[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0001,0x0004,0x0009,0x000e,0x0013,
	0x0018,0x0019,0x001c,0x001f,0x0024,0x0029,0x002b,0x0030,
	0x0031,0x0035,0x003a,0x003c,0x0041,0x0046,0x004b,0x0050,
	0x0055,0x005a,0x005f,0x0064,0x0065,0x0067,0x006a,0x006f,
	0x0072,0x0077,0x007c,0x0081,0x0086,0x008b,0x0090,0x0095,
	0x009a,0x009f,0x00a4,0x00a7,0x00ac,0x00b1,0x00b6,0x00bb,
	0x00c0,0x00c5,0x00ca,0x00cf,0x00d4,0x00d9,0x00de,0x00e3,
	0x00e8,0x00ed,0x00f2,0x00f7,0x00fc,0x00fe,0x0102,0x0104,
	0x0107,0x010d,0x010f,0x0114,0x0119,0x011e,0x0123,0x0128,
	0x012c,0x0131,0x0136,0x0137,0x013a,0x013f,0x0142,0x0147,
	0x014c,0x0151,0x0156,0x015b,0x0160,0x0165,0x0169,0x016e,
	0x0173,0x0178,0x017d,0x0182,0x0187,0x018a,0x018b,0x018e,
	0x0193,0x0193,0x0198,0x019d,0x01a2,0x01a7,0x01ac,0x01b1,
	0x01b6,0x01bb,0x01c0,0x01c5,0x01ca,0x01cf,0x01d4,0x01d9,
	0x01de,0x01e3,0x01e8,0x01ed,0x01ef,0x01f1,0x01f4,0x01f7,
	0x01fc,0x0201,0x0206,0x020b,0x0210,0x0215,0x021a,0x021f,
	0x0224,0x0229,0x022c,0x0230,0x0235,0x023a,0x023f,0x0244,
	0x0249,0x024e,0x0253,0x0258,0x025e,0x0260,0x0263,0x0268,
	0x026d,0x0272,0x0277,0x027c,0x027f,0x0282,0x0287,0x028c,
	0x0290,0x0295,0x029a,0x029f,0x02a4,0x02a8,0x02ac,0x02b1,
	0x02b6,0x02bb,0x02c0,0x02c1,0x02c6,0x02cb,0x02d0,0x02d5,
	0x02da,0x02df,0x02e4,0x02e9,0x02e9,0x02ee,0x02f3,0x02f8,
	0x02fd,0x0302,0x0305,0x030b,0x030f,0x0313,0x0315,0x0317,
	0x031c,0x0321,0x0326,0x032c,0x0332,0x0000,0x0000,0x0000
};

font_desc Kate = {
		0x0000,		/* firstchar */
		0x00d9,		/* lastchar */
		0x0006,		/* width_max */
		0x000b,		/* font_height */
		0x0009,		/* ascent */
		0x0002,		/* descent */
		0x0000,		/* leading */
		0x0068,		/* bm_rowbyte */
		6,		/* default_width */
		Kate_bitmap,	/* bitmap */
		Kate_offsets,	/* offsets */
		0,		/* proportional */
		0		/* filler1 */
	};
