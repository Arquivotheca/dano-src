#include "CCITTFaxDecode.h"
#include <stdlib.h>
#include <Debug.h>

#if 0
#include "TeePusher.h"
#define FAX_TEE_PUSHER 1
#endif

#if 1
#define PASS			0
#define HORIZONTAL		1
#define HORIZONTAL_R1	2
#define HORIZONTAL_R2	3
#define VERTICAL_L3		4
#define VERTICAL_L2		5
#define VERTICAL_L1		6
#define VERTICAL_0		7
#define VERTICAL_R1		8
#define VERTICAL_R2		9
#define VERTICAL_R3		10
#define BUILDING_CODE	11
#define ERROR_CODE		12
#define GROUP_3_1D		13
#define GROUP_3_2D		14

#if 0
CCITTFaxDecode::fax_code CCITTFaxDecode::two_dim_codes[] = {
	{ 1, 0x00, BUILDING_CODE},	// 0
	{ 1, 0x01, VERTICAL_0},		// 1
	{ 1, 0x02, ERROR_CODE}, 	// 
	{ 1, 0x03, ERROR_CODE}, 	//
	
	{ 2, 0x00, BUILDING_CODE},	// 00
	{ 2, 0x01, BUILDING_CODE},	// 01
	{ 2, 0x02, ERROR_CODE}, 	// 10
	{ 2, 0x03, ERROR_CODE},		// 11
	
	{ 3, 0x00, BUILDING_CODE},	// 000
	{ 3, 0x01, HORIZONTAL},		// 001
	{ 3, 0x02, VERTICAL_L1},	// 010
	{ 3, 0x03, VERTICAL_R1},	// 011
	
	{ 4, 0x00, BUILDING_CODE},	// 0000
	{ 4, 0x01, PASS},			// 0001
	{ 4, 0x02, ERROR_CODE},		// 0010
	{ 4, 0x03, ERROR_CODE},		// 0011
	
	{ 5, 0x00, BUILDING_CODE},	// 00000
	{ 5, 0x01, BUILDING_CODE},	// 00001
	{ 5, 0x02, ERROR_CODE},		// 00010
	{ 5, 0x03, ERROR_CODE},		// 00011
	
	{ 6, 0x00, ERROR_CODE},		// 000000 
	{ 6, 0x01, BUILDING_CODE},	// 000001
	{ 6, 0x02, VERTICAL_L2},	// 000010 
	{ 6, 0x03, VERTICAL_R2},	// 000011
	
	{ 7, 0x00, ERROR_CODE},		// 0000000
	{ 7, 0x01, ERROR_CODE},		// 0000001
	{ 7, 0x02, VERTICAL_L3},	// 0000010 
	{ 7, 0x03, VERTICAL_R3},	// 0000011
};
#endif
const uint8 CCITTFaxDecode::modes[8][4] = {
	{ ERROR_CODE,		ERROR_CODE,		ERROR_CODE,		ERROR_CODE },	// run length 0
	{ BUILDING_CODE,	VERTICAL_0,		ERROR_CODE,		ERROR_CODE },	// 1
	{ BUILDING_CODE,	BUILDING_CODE,	ERROR_CODE,		ERROR_CODE },	// 2
	{ BUILDING_CODE,	HORIZONTAL,		VERTICAL_L1,	VERTICAL_R1 },	// 3
	{ BUILDING_CODE,	PASS,			ERROR_CODE,		ERROR_CODE },	// 4
	{ BUILDING_CODE,	BUILDING_CODE,	ERROR_CODE,		ERROR_CODE },	// 5
	{ ERROR_CODE,		BUILDING_CODE,	VERTICAL_L2,	VERTICAL_R2 },	// 6
	{ ERROR_CODE,		ERROR_CODE,		VERTICAL_L3,	VERTICAL_R3 }	// 7
};

const CCITTFaxDecode::fax_code CCITTFaxDecode::white_runs[] = {
	{  4, 0x07,    2 }, // 0111
	{  4, 0x08,    3 }, // 1000
	{  4, 0x0b,    4 }, // 1011
	{  4, 0x0c,    5 }, // 1100
	{  4, 0x0e,    6 }, // 1110
	{  4, 0x0f,    7 }, // 1111
	{  5, 0x07,   10 }, // 00111
	{  5, 0x08,   11 }, // 01000
	{  5, 0x12,  128 }, // 10010
	{  5, 0x13,    8 }, // 10011
	{  5, 0x14,    9 }, // 10100
	{  5, 0x1b,   64 }, // 11011
	{  6, 0x03,   13 }, // 000011
	{  6, 0x07,    1 }, // 000111
	{  6, 0x08,   12 }, // 001000
	{  6, 0x17,  192 }, // 010111
	{  6, 0x18, 1664 }, // 011000
	{  6, 0x2a,   16 }, // 101010
	{  6, 0x2b,   17 }, // 101011
	{  6, 0x34,   14 }, // 110100
	{  6, 0x35,   15 }, // 110101
	{  7, 0x03,   22 }, // 0000011
	{  7, 0x04,   23 }, // 0000100
	{  7, 0x08,   20 }, // 0001000
	{  7, 0x0c,   19 }, // 0001100
	{  7, 0x13,   26 }, // 0010011
	{  7, 0x17,   21 }, // 0010111
	{  7, 0x18,   28 }, // 0011000
	{  7, 0x24,   27 }, // 0100100
	{  7, 0x27,   18 }, // 0100111
	{  7, 0x28,   24 }, // 0101000
	{  7, 0x2b,   25 }, // 0101011
	{  7, 0x37,  256 }, // 0110111
	{  8, 0x02,   29 }, // 00000010
	{  8, 0x03,   30 }, // 00000011
	{  8, 0x04,   45 }, // 00000100
	{  8, 0x05,   46 }, // 00000101
	{  8, 0x0a,   47 }, // 00001010
	{  8, 0x0b,   48 }, // 00001011
	{  8, 0x12,   33 }, // 00010010
	{  8, 0x13,   34 }, // 00010011
	{  8, 0x14,   35 }, // 00010100
	{  8, 0x15,   36 }, // 00010101
	{  8, 0x16,   37 }, // 00010110
	{  8, 0x17,   38 }, // 00010111
	{  8, 0x1a,   31 }, // 00011010
	{  8, 0x1b,   32 }, // 00011011
	{  8, 0x24,   53 }, // 00100100
	{  8, 0x25,   54 }, // 00100101
	{  8, 0x28,   39 }, // 00101000
	{  8, 0x29,   40 }, // 00101001
	{  8, 0x2a,   41 }, // 00101010
	{  8, 0x2b,   42 }, // 00101011
	{  8, 0x2c,   43 }, // 00101100
	{  8, 0x2d,   44 }, // 00101101
	{  8, 0x32,   61 }, // 00110010
	{  8, 0x33,   62 }, // 00110011
	{  8, 0x34,   63 }, // 00110100
	{  8, 0x35,    0 }, // 00110101
	{  8, 0x36,  320 }, // 00110110
	{  8, 0x37,  384 }, // 00110111
	{  8, 0x4a,   59 }, // 01001010
	{  8, 0x4b,   60 }, // 01001011
	{  8, 0x52,   49 }, // 01010010
	{  8, 0x53,   50 }, // 01010011
	{  8, 0x54,   51 }, // 01010100
	{  8, 0x55,   52 }, // 01010101
	{  8, 0x58,   55 }, // 01011000
	{  8, 0x59,   56 }, // 01011001
	{  8, 0x5a,   57 }, // 01011010
	{  8, 0x5b,   58 }, // 01011011
	{  8, 0x64,  448 }, // 01100100
	{  8, 0x65,  512 }, // 01100101
	{  8, 0x67,  640 }, // 01100111
	{  8, 0x68,  576 }, // 01101000
	{  9, 0x98, 1472 }, // 010011000
	{  9, 0x99, 1536 }, // 010011001
	{  9, 0x9a, 1600 }, // 010011010
	{  9, 0x9b, 1728 }, // 010011011
	{  9, 0xcc,  704 }, // 011001100
	{  9, 0xcd,  768 }, // 011001101
	{  9, 0xd2,  832 }, // 011010010
	{  9, 0xd3,  896 }, // 011010011
	{  9, 0xd4,  960 }, // 011010100
	{  9, 0xd5, 1024 }, // 011010101
	{  9, 0xd6, 1088 }, // 011010110
	{  9, 0xd7, 1152 }, // 011010111
	{  9, 0xd8, 1216 }, // 011011000
	{  9, 0xd9, 1280 }, // 011011001
	{  9, 0xda, 1344 }, // 011011010
	{  9, 0xdb, 1408 }, // 011011011
	{ 11, 0x08, 1792 }, // 00000001000
	{ 11, 0x0c, 1856 }, // 00000001100
	{ 11, 0x0d, 1920 }, // 00000001101
	{ 12, 0x01, 0xffff}, // 000000000001 EOL
	{ 12, 0x12, 1984 }, // 000000010010
	{ 12, 0x13, 2048 }, // 000000010011
	{ 12, 0x14, 2112 }, // 000000010100
	{ 12, 0x15, 2176 }, // 000000010101
	{ 12, 0x16, 2240 }, // 000000010110
	{ 12, 0x17, 2304 }, // 000000010111
	{ 12, 0x1c, 2368 }, // 000000011100
	{ 12, 0x1d, 2432 }, // 000000011101
	{ 12, 0x1e, 2496 }, // 000000011110
	{ 12, 0x1f, 2560 },  // 000000011111
	{ 0xff, 0xff, 0xffff},
	{ 0xff, 0xff, 0xffff}
};

const CCITTFaxDecode::fax_code* CCITTFaxDecode::white_runs_starts[] = {
	0,	// 0 bits
	0,	// 1 bit
	0,	// 2 bits
	0,	// 3 bits
	CCITTFaxDecode::white_runs +  0,
	CCITTFaxDecode::white_runs +  6,
	CCITTFaxDecode::white_runs + 12,
	CCITTFaxDecode::white_runs + 21,
	CCITTFaxDecode::white_runs + 33,
	CCITTFaxDecode::white_runs + 75,
	CCITTFaxDecode::white_runs + 91,	// no 10 bit codes
	CCITTFaxDecode::white_runs + 91,
	CCITTFaxDecode::white_runs + 94,
	CCITTFaxDecode::white_runs + 105	// 13 bits
	//CCITTFaxDecode::white_runs + 106	// 14 bits
};

const CCITTFaxDecode::fax_code CCITTFaxDecode::black_runs[] = {
	{  2, 0x02,    3 }, // 10
	{  2, 0x03,    2 }, // 11
	{  3, 0x02,    1 }, // 010
	{  3, 0x03,    4 }, // 011
	{  4, 0x02,    6 }, // 0010
	{  4, 0x03,    5 }, // 0011
	{  5, 0x03,    7 }, // 00011
	{  6, 0x04,    9 }, // 000100
	{  6, 0x05,    8 }, // 000101
	{  7, 0x04,   10 }, // 0000100
	{  7, 0x05,   11 }, // 0000101
	{  7, 0x07,   12 }, // 0000111
	{  8, 0x04,   13 }, // 00000100
	{  8, 0x07,   14 }, // 00000111
	{  9, 0x18,   15 }, // 000011000
	{ 10, 0x08,   18 }, // 0000001000
	{ 10, 0x0f,   64 }, // 0000001111
	{ 10, 0x17,   16 }, // 0000010111
	{ 10, 0x18,   17 }, // 0000011000
	{ 10, 0x37,    0 }, // 0000110111
	{ 11, 0x08, 1792 }, // 00000001000
	{ 11, 0x0c, 1856 }, // 00000001100
	{ 11, 0x0d, 1920 }, // 00000001101
	{ 11, 0x17,   24 }, // 00000010111
	{ 11, 0x18,   25 }, // 00000011000
	{ 11, 0x28,   23 }, // 00000101000
	{ 11, 0x37,   22 }, // 00000110111
	{ 11, 0x67,   19 }, // 00001100111
	{ 11, 0x68,   20 }, // 00001101000
	{ 11, 0x6c,   21 }, // 00001101100
	{ 12, 0x01, 0xffff}, // 000000000001 EOL
	{ 12, 0x12, 1984 }, // 000000010010
	{ 12, 0x13, 2048 }, // 000000010011
	{ 12, 0x14, 2112 }, // 000000010100
	{ 12, 0x15, 2176 }, // 000000010101
	{ 12, 0x16, 2240 }, // 000000010110
	{ 12, 0x17, 2304 }, // 000000010111
	{ 12, 0x1c, 2368 }, // 000000011100
	{ 12, 0x1d, 2432 }, // 000000011101
	{ 12, 0x1e, 2496 }, // 000000011110
	{ 12, 0x1f, 2560 }, // 000000011111
	{ 12, 0x24,   52 }, // 000000100100
	{ 12, 0x27,   55 }, // 000000100111
	{ 12, 0x28,   56 }, // 000000101000
	{ 12, 0x2b,   59 }, // 000000101011
	{ 12, 0x2c,   60 }, // 000000101100
	{ 12, 0x33,  320 }, // 000000110011
	{ 12, 0x34,  384 }, // 000000110100
	{ 12, 0x35,  448 }, // 000000110101
	{ 12, 0x37,   53 }, // 000000110111
	{ 12, 0x38,   54 }, // 000000111000
	{ 12, 0x52,   50 }, // 000001010010
	{ 12, 0x53,   51 }, // 000001010011
	{ 12, 0x54,   44 }, // 000001010100
	{ 12, 0x55,   45 }, // 000001010101
	{ 12, 0x56,   46 }, // 000001010110
	{ 12, 0x57,   47 }, // 000001010111
	{ 12, 0x58,   57 }, // 000001011000
	{ 12, 0x59,   58 }, // 000001011001
	{ 12, 0x5a,   61 }, // 000001011010
	{ 12, 0x5b,  256 }, // 000001011011
	{ 12, 0x64,   48 }, // 000001100100
	{ 12, 0x65,   49 }, // 000001100101
	{ 12, 0x66,   62 }, // 000001100110
	{ 12, 0x67,   63 }, // 000001100111
	{ 12, 0x68,   30 }, // 000001101000
	{ 12, 0x69,   31 }, // 000001101001
	{ 12, 0x6a,   32 }, // 000001101010
	{ 12, 0x6b,   33 }, // 000001101011
	{ 12, 0x6c,   40 }, // 000001101100
	{ 12, 0x6d,   41 }, // 000001101101
	{ 12, 0xc8,  128 }, // 000011001000
	{ 12, 0xc9,  192 }, // 000011001001
	{ 12, 0xca,   26 }, // 000011001010
	{ 12, 0xcb,   27 }, // 000011001011
	{ 12, 0xcc,   28 }, // 000011001100
	{ 12, 0xcd,   29 }, // 000011001101
	{ 12, 0xd2,   34 }, // 000011010010
	{ 12, 0xd3,   35 }, // 000011010011
	{ 12, 0xd4,   36 }, // 000011010100
	{ 12, 0xd5,   37 }, // 000011010101
	{ 12, 0xd6,   38 }, // 000011010110
	{ 12, 0xd7,   39 }, // 000011010111
	{ 12, 0xda,   42 }, // 000011011010
	{ 12, 0xdb,   43 }, // 000011011011
	{ 13, 0x4a,  640 }, // 0000001001010
	{ 13, 0x4b,  704 }, // 0000001001011
	{ 13, 0x4c,  768 }, // 0000001001100
	{ 13, 0x4d,  832 }, // 0000001001101
	{ 13, 0x52, 1280 }, // 0000001010010
	{ 13, 0x53, 1344 }, // 0000001010011
	{ 13, 0x54, 1408 }, // 0000001010100
	{ 13, 0x55, 1472 }, // 0000001010101
	{ 13, 0x5a, 1536 }, // 0000001011010
	{ 13, 0x5b, 1600 }, // 0000001011011
	{ 13, 0x64, 1664 }, // 0000001100100
	{ 13, 0x65, 1728 }, // 0000001100101
	{ 13, 0x6c,  512 }, // 0000001101100
	{ 13, 0x6d,  576 }, // 0000001101101
	{ 13, 0x72,  896 }, // 0000001110010
	{ 13, 0x73,  960 }, // 0000001110011
	{ 13, 0x74, 1024 }, // 0000001110100
	{ 13, 0x75, 1088 }, // 0000001110101
	{ 13, 0x76, 1152 }, // 0000001110110
	{ 13, 0x77, 1216 }, // 0000001110111
	{ 0xff, 0xff, 0xffff}
};

const CCITTFaxDecode::fax_code* CCITTFaxDecode::black_runs_starts[] = {
	0, // 0 bits
	0, // 1 bit
	CCITTFaxDecode::black_runs +  0,	// 2 bits
	CCITTFaxDecode::black_runs +  2,
	CCITTFaxDecode::black_runs +  4,
	CCITTFaxDecode::black_runs +  6,
	CCITTFaxDecode::black_runs +  7,
	CCITTFaxDecode::black_runs +  9,
	CCITTFaxDecode::black_runs + 12,
	CCITTFaxDecode::black_runs + 14,
	CCITTFaxDecode::black_runs + 15,
	CCITTFaxDecode::black_runs + 20,
	CCITTFaxDecode::black_runs + 30,
	CCITTFaxDecode::black_runs + 85,
	CCITTFaxDecode::black_runs + 105	// 14 bits
};
#endif

/*
 * Bit handling utilities. (swiped from libTIFF, almost verbatim)
 */

static const uint8 zeroruns[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,	/* 0x00 - 0x0f */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* 0x10 - 0x1f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0x20 - 0x2f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0x30 - 0x3f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x40 - 0x4f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x50 - 0x5f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x60 - 0x6f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x70 - 0x7f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x80 - 0x8f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x90 - 0x9f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xa0 - 0xaf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xb0 - 0xbf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xc0 - 0xcf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xd0 - 0xdf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xe0 - 0xef */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0xf0 - 0xff */
};
static const uint8 oneruns[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x00 - 0x0f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x10 - 0x1f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x20 - 0x2f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x30 - 0x3f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x40 - 0x4f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x50 - 0x5f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x60 - 0x6f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0x70 - 0x7f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x80 - 0x8f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0x90 - 0x9f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0xa0 - 0xaf */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 0xb0 - 0xbf */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0xc0 - 0xcf */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 0xd0 - 0xdf */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* 0xe0 - 0xef */
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8,	/* 0xf0 - 0xff */
};

/*
 * Find a span of ones or zeros using the supplied
 * table.  The byte-aligned start of the bit string
 * is supplied along with the start+end bit indices.
 * The table gives the number of consecutive ones or
 * zeros starting from the msb and is indexed by byte
 * value.
 */
static int
findspan(uint8** bpp, int bs, int be, register const u_char* tab)
{
	register uint8 *bp = *bpp;
	register int bits = be - bs;
	register int n, span;

	/*
	 * Check partial byte on lhs.
	 */
	if (bits > 0 && (n = (bs & 7))) {
		span = tab[(*bp << n) & 0xff];
		if (span > 8-n)		/* table value too generous */
			span = 8-n;
		if (span > bits)	/* constrain span to bit range */
			span = bits;
		if (n+span < 8)		/* doesn't extend to edge of byte */
			goto done;
		bits -= span;
		bp++;
	} else
		span = 0;
	/*
	 * Scan full bytes for all 1's or all 0's.
	 */
	while (bits >= 8) {
		n = tab[*bp];
		span += n;
		bits -= n;
		if (n < 8)		/* end of run */
			goto done;
		bp++;
	}
	/*
	 * Check partial byte on rhs.
	 */
	if (bits > 0) {
		n = tab[*bp];
		span += (n > bits ? bits : n);
	}
done:
	*bpp = bp;
	return (span);
}

/*
 * Return the offset of the next bit in the range
 * [bs..be] that is different from the specified
 * color.  The end, be, is returned if no such bit
 * exists.
 */
static int
finddiff(uint8* cp, int bs, int be, int color)
{
	if (bs < 0)
	{
		if (color && (*cp & 0x80)) return 0;
		else bs = 0;
	}		
	cp += bs >> 3;			/* adjust byte offset */
	return (bs + findspan(&cp, bs, be, color ? oneruns : zeroruns));
}

/*
 * Fill a span with ones.
 */
static void
fillspan(register uint8* cp, register int x, register int count)
{
	static const unsigned char masks[] =
	    { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

	if (count <= 0)
		return;
	cp += x>>3;
	if (x &= 7) {			/* align to byte boundary */
		if (count < 8 - x) {
			*cp++ |= masks[count] >> x;
			return;
		}
		*cp++ |= 0xff >> x;
		count -= 8 - x;
	}
	while (count >= 8) {
		*cp++ = 0xff;
		count -= 8;
	}
	*cp |= masks[count];
}

/*
 * End of swiped code.
 */

CCITTFaxDecode::CCITTFaxDecode(Pusher *sink, PDFObject *parms)
	:
#if FAX_TEE_PUSHER
	Pusher(new TeePusher(sink, "/boot/home/CCITTFaxDecode")),
#else
	Pusher(sink),
#endif
	m_run_length(0),
	m_code_length(0), m_code_bits(0),
	m_source_length(0), m_source_bits(0),
	m_doing_black(false)
{
	PDFObject *obj;

	m_K = ((obj = parms->Find(PDFAtom.K)) ? obj->GetInt32() : 0);
	m_EndOfLine = ((obj = parms->Find(PDFAtom.EndOfLine)) ? obj->GetBool() : false);
	m_EncodedByteAlign = ((obj = parms->Find(PDFAtom.EncodedByteAlign)) ? obj->GetBool() : false);
	m_Columns = ((obj = parms->Find(PDFAtom.Columns)) ? obj->GetInt32() : 1728);
	m_BlackIs1 = ((obj = parms->Find(PDFAtom.BlackIs1)) ? obj->GetBool() : false);
	//m_BlackIs1 = true;
	m_EndOfBlock = ((obj = parms->Find(PDFAtom.EndOfBlock)) ? obj->GetBool() : true);
	m_Rows = ((obj = parms->Find(PDFAtom.Rows)) ? obj->GetInt32() : 0);
	m_DamagedRowsBeforeError = ((obj = parms->Find(PDFAtom.DamagedRowsBeforeError)) ? obj->GetInt32() : 0);

	m_a0 = -1;
	m_mode = m_K < 0 ? BUILDING_CODE : (m_K == 0 ? GROUP_3_1D : GROUP_3_2D);
	m_row_bytes = (m_Columns + 7) >> 3;
	m_prev_line = m_K ? new uint8[m_row_bytes] : 0;
	m_this_line = new uint8[m_row_bytes];
	if (m_prev_line) memset(m_prev_line, 0, m_row_bytes);
	if (m_this_line) memset(m_this_line, 0, m_row_bytes);
#ifndef NDEBUG
		m_last_code_length = 0;
		m_last_code_bits = 0;
		m_line_num = 0;
#endif
}


CCITTFaxDecode::~CCITTFaxDecode()
{
	delete [] m_this_line;
	delete [] m_prev_line;
}

int 
CCITTFaxDecode::fax_code_compare(uint8 const *key, fax_code const *item)
{
	return (int)*key - (int)item->code_bits;
}

CCITTFaxDecode::fax_code const * 
CCITTFaxDecode::findPair(void)
{
	fax_code const *first, *last;
	if (m_doing_black)
	{
		if (m_code_length > 13) return 0;
		first = black_runs_starts[m_code_length];
		last = black_runs_starts[m_code_length+1];
	}
	else
	{
		if (m_code_length > 12) return 0;
		first = white_runs_starts[m_code_length];
		last = white_runs_starts[m_code_length+1];
	}
	if (!first) return 0;
	// binary search for the code_bits
	return (fax_code*)bsearch(&m_code_bits, first, last - first, sizeof(fax_code),
		reinterpret_cast<int(*)(void const *, void const *)>(fax_code_compare));
}

bool
CCITTFaxDecode::doOneDimBitRun(void)
{
	fax_code const * const match = findPair();
	if (match)
	{
		uint32 run_length = match->run_length;
		// accumulate run length;
		m_run_length += run_length;
		//ASSERT(m_run_length <= m_Columns);
		// a terminating code?
		if (run_length < 64)
		{
			if (m_a0 < 0) m_a0 = 0;
			//ASSERT(m_a0 + m_run_length <= m_Columns);
			if (m_a0 + m_run_length > m_Columns)
				m_run_length = m_Columns - m_a0;
			// extrude run
			if (m_doing_black)
			{
				fillspan(m_this_line, m_a0, m_run_length);
			}
			// ajust mark
			m_a0 += m_run_length;
			// toggle color
			m_doing_black = !m_doing_black;
			// reset
			m_run_length = 0;
		}
		else if (run_length == 0xffff)
		{
			// EOL marker - start with white
			m_doing_black = false;
			ASSERT(m_run_length == 0);
		}
#ifndef NDEBUG
		m_last_code_length = m_code_length;
		m_last_code_bits = m_code_bits;
#endif
		// prep for next code
		m_code_length = 0;
		m_code_bits = 0;
	}
	
	return ((m_code_length == 0) && (m_run_length == 0));
}

ssize_t 
CCITTFaxDecode::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	bool end_of_stream = false;
	ssize_t orig_length = length;
#if DEBUG > 0
	printf("CCITTFaxDecode::Write(%p, %ld, %s)\n", buffer, length, finish ? "true" : "false");
#endif
#if FAX_TEE_PUSHER
	return Pusher::Write(buffer, length, finish);
#else
	while (length && !end_of_stream)
	{
		if ((m_source_length == 0) && (length != 0))
		{
			m_source_length = 8;
			m_source_bits = *buffer++;
			length--;
		}
		// add a bit to the code
		m_code_length++;
		m_code_bits <<= 1;
		m_code_bits |= (m_source_bits & 0x80) >> 7;
		m_source_bits <<= 1;
		m_source_length--;
		// a string of 12 zero bits can ONLY mean a zero FILL run.
		if ((m_code_length >= 12) && (m_code_bits == 0))
		{
			// drop the run length count so we can pick up the 12 bit EOL
			m_code_length = 11;
			continue;
		}
retry_mode:
		switch (m_mode)
		{
			case PASS:
			{
				m_b2 = finddiff(m_prev_line, m_a0, m_Columns, !m_doing_black);
				m_b1 = finddiff(m_prev_line, m_b2, m_Columns, m_doing_black);
				m_b2 = finddiff(m_prev_line, m_b1, m_Columns, !m_doing_black);
				if (m_doing_black)
				{
					if (m_a0 < 0) m_a0 = 0;
					fillspan(m_this_line, m_a0, m_b2 - m_a0);
				}
				m_a0 = m_b2;
				m_mode = m_K < 0 ? BUILDING_CODE : (m_K == 0 ? GROUP_3_1D : GROUP_3_2D);
#ifndef NDEBUG
		m_last_code_length = m_code_length;
		m_last_code_bits = m_code_bits;
#endif
				m_code_length = 0;
				m_code_bits = 0;
			} break;
			case HORIZONTAL:
				// switch to next mode
				m_mode = HORIZONTAL_R1;
				// clear out code
#ifndef NDEBUG
		m_last_code_length = m_code_length;
		m_last_code_bits = m_code_bits;
#endif
				m_code_length = 0;
				m_code_bits = 0;
				break;
			case HORIZONTAL_R1:
				if (doOneDimBitRun()) m_mode = HORIZONTAL_R2;
				break;
			case HORIZONTAL_R2:
				if (doOneDimBitRun()) m_mode = BUILDING_CODE;
				break;
			case VERTICAL_L3:
			case VERTICAL_L2:
			case VERTICAL_L1:
			case VERTICAL_0:
			case VERTICAL_R1:
			case VERTICAL_R2:
			case VERTICAL_R3:
				m_b2 = finddiff(m_prev_line, m_a0, m_Columns, !m_doing_black);
				m_b1 = finddiff(m_prev_line, m_b2, m_Columns, m_doing_black);
				m_b1 += m_mode - VERTICAL_0;
				ASSERT(m_a0 < m_b1);
				// extrude run
				if (m_doing_black)
				{
					if (m_a0 < 0) m_a0 = 0;
					fillspan(m_this_line, m_a0, m_b1 - m_a0);
				}
				// reset for next code
				m_mode = m_K < 0 ? BUILDING_CODE : (m_K == 0 ? GROUP_3_1D : GROUP_3_2D);
				m_doing_black = !m_doing_black;
				m_a0 = m_b1;
#ifndef NDEBUG
		m_last_code_length = m_code_length;
		m_last_code_bits = m_code_bits;
#endif
				m_code_length = 0;
				m_code_bits = 0;
				break;
			case BUILDING_CODE:
				m_mode = modes[m_code_length][m_code_bits];
				switch (m_mode)
				{
					case BUILDING_CODE:
					case ERROR_CODE:
						continue;
						break;
					default:
						goto retry_mode;
				}
				break;
			case ERROR_CODE:
				if ((m_code_length == 12) && (m_code_bits = 1))
				{
					// EOL marker
					end_of_stream = true;
				}
				break;
			case GROUP_3_1D:
				doOneDimBitRun();
				m_mode = m_K < 0 ? BUILDING_CODE : (m_K == 0 ? GROUP_3_1D : GROUP_3_2D);
				break;
			case GROUP_3_2D:
				if (m_code_bits) m_mode = BUILDING_CODE;
				else m_mode = GROUP_3_1D;
				// clear out code
#ifndef NDEBUG
		m_last_code_length = m_code_length;
		m_last_code_bits = m_code_bits;
#endif
				m_code_length = 0;
				m_code_bits = 0;
				break;
		}
		// did we pass the end of the row?
		if ((m_a0 >= m_Columns) && (m_mode >= BUILDING_CODE))
		{
			// output this line
			if (!m_BlackIs1) InvertThisLine();
			if (Pusher::Write(m_this_line, m_row_bytes, false) != (int32)m_row_bytes)
				return Pusher::SINK_FULL;
			if (!m_BlackIs1) InvertThisLine();
#ifndef NDEBUG
			{
				fprintf(stdout, "%d ", m_line_num++);
				for (int i = 0; i < m_row_bytes; i++)
				{
				fprintf(stdout, "%.2x", m_this_line[i]);
				}
				fprintf(stdout, "\n"); fflush(stdout);
				//ASSERT(m_line_num <= 1676);
			}
#endif
			// ensure byte aligned row data?
			if (m_EncodedByteAlign)
			{
				// flush bits
				m_source_bits = 0;
				m_source_length = 0;
			}
			// not a one dimensional encoding?
			if (m_K != 0)
			{
				// save current line as previous line
				uint8 *temp = m_prev_line;
				m_prev_line = m_this_line;
				m_this_line = temp;
			}
			// in any case, blank the current line
			memset(m_this_line, 0, m_row_bytes);
			// reset the pixel counter
			m_a0 = -1;
			// always start the line with white
			m_doing_black = false;
		}

	}
	if (end_of_stream || finish) Pusher::Write(0, 0, true);
#if DEBUG > 0
	printf("end_of_stream: %s, finish: %s\n", end_of_stream ? "T" : "F", finish ? "T" : "F");
#endif
	return orig_length;
#endif
}

void 
CCITTFaxDecode::InvertThisLine(void)
{
	for (uint32 i = 0; i < m_row_bytes; i++)
		m_this_line[i] = ~m_this_line[i];
}
