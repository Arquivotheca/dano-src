// ===========================================================================
//	JPEG.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __JPEG__
#define __JPEG__

#include <Rect.h>


//	Scaled DCT
// we can set I_SHIFT_F to 9 for better accuracy, but that
// means we need to do long multiplies in the IDCT code, which 
// cost one extra cycle - don't know if it's worth it...
const int I_SHIFT_F = 10;					// scale factor for scaled quantization table
const int O_SCALE_F = (16-I_SHIFT_F);		// unscale factor for transformed coefficients
const int M_SCALE_F = (O_SCALE_F*2);		// intermediate scale factor for multiplies

// Constants
const int kMaxHuffmanTables= 8;
const int kMaxQuantizationTables = 4;

const int kMaxScanComponents = 3;			// standard allows 4 but we only support up to 3
const int kMaxFrameComponents = 3;			// standard allows 4 but we only support up to 3
const int kNumDecompressBuffers = kMaxFrameComponents;

// size of a MCU block 
const int kMCUWidth = 8;
const int kMCUHeight = 8;
const int kMCUSize = (kMCUWidth * kMCUHeight);
const int kMCUSizeShift	= 6;							// (1<<6) == kMCUWidth * kMCUHeight 

const int kMaxMCUs = 6;									// the standard allows up to 10, but we only support up to 22:11:11 sampling so...
const int kMDUMin=1024;									// Minimum data in the buffer to safely decode an MDU

const int kJPEGBufferSize = (kMaxMCUs + 2) * kMDUMin;	// read buffer size - must fit in an short 


//	Errors
enum JPEGError {
	kJPEGCancelled = 1,
	kNoError = 0,
	kLowMemory = -1001,
	kGenericError = -1002,
	kCannotParse = -1003,
	
	kNoSOIMarkerErr	= -1024,	// No Start of Image marker, bad JPEG data
	kNoSOFMarkerErr	= -1025,	// No Start of Frame marker, bad JPEG data
	kNoSOSMarkerErr	= -1026,	// No Start of Scan marker, bad JPEG data

	kBadSOFMarkerErr = -1027,	// Bad Start of Frame marker, bad JPEG data
	kBadSOSMarkerErr = -1028,	// Bad Start of Scan marker, bad JPEG data
	kBadMarkerErr	= -1029,	// Other marker that I don't want to know about
	kBadHuffmanErr	= -1030,	// Bad huffman code
	kOutOfDataErr	= -1031		// Read past end of data
};


//
//	JPEGDecoder contains all the state information for the decoder
//

inline long LONG(void *l)
{
	uchar *b = (uchar *)l;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}     
	

//	Huffman tables
typedef struct FastRec{		// Record for fast huffmand decode
	short	value;			// Decoded symbol value
	char	length;			// Length of decoded symbol or code (-ve for code)
	uchar	run;			// Zero run length or 0 for DC
} FastRec;

typedef struct HuffmanTable{
	FastRec	fast[256];		// for fast decoding of the huffman data
	long	mincode[17];	// smallest code of length (i+1)
	long	maxcode[18];	// largest code of length (i+1) (-1 if none)
	short	valptr[17+1];	// huffval[] index of 1st symbol of length (i+1)
	uchar	huffval[256];	// Symbols in increasing code length
	uchar	bits[17+3];		// bits[i] = # of symbols with codes of length (i+1) bits
} HuffmanTable;


//	Components store lots of state for progressive JPEG
typedef struct CompSpec{			// Component spec 
									// following are set once per image
	short	*blockBufferData;		//	block buffer ( for progressive )
	short	*blockBuffer;			//	current block buffer pointer
	long	blockCount;				//  #blocks buffered

									// following set once per frame
	uint16	*QTable;						// 		quantization table
	uchar	blocksMCU;				// 		blocks in a frame MCU ( MDU )
	uchar	Ci;						// 		Component identifier
	uchar	Hi;						// 		Horizontal blocks in MCU
	uchar	Vi;						// 		Vertical blocks in MCU

									// following set once per scan
	HuffmanTable	*DCH;			// 		dc huffman table
	HuffmanTable	*ACH;			// 		ac huffman table
	uchar	dcBitPos;				// 		successive approx bit position
	uchar	acBitPos;				// 		successive approx bit position
	uchar	dcPrevBitPos;			// 		dc successive approximation needed ( for progressive)
	uchar	acPrevBitPos;			// 		ac successive approximation needed ( for progressive)
	uchar	specStart;				// 		spectrum start (for scan, progressive )
	uchar	specEnd;				//		spectrum end (for scan, progressive )
	short	DC;						// 		dc value
	bool	inScan;					// 		true if included current scan
} CompSpec;

typedef enum JPEGDecodePhase {
	kWaitingForNewImage = 0,
	kWaitingForFrame,
	kWaitingForScan,
	kProcessingScan,
	kSkippingScan,
	kSkippingToEnd
} JPEGDecodePhase;

const	int kJPEGCompletedPass	= -42;		// returned from JPEGWrite when done with pass

//	Drawing proc
//	Define the draw proc that gets called when a line of blocks is complete
//	Returns 1 to cancel decode
typedef JPEGError (*DrawRowProcPtr)(void* j);

//	Main decoder structure
typedef struct JPEGDecoder {
	uchar			*WriteBuffer;
	long			inBuffer;
	uchar			*data;
	uchar			*dataEnd;
	long			Width;					// number of horizontal lines defined in frame header
	long			Height;					// number of vertical lines defined in frame header
	
	JPEGDecodePhase		phase;			// Phase of the decode (0,kSOI,kSOS,kEOI)

	CompSpec		Comp[kMaxFrameComponents];			// Up to 4 components per frame
	CompSpec*		scanComps[kMaxScanComponents];	
	ushort			*QTables[kMaxQuantizationTables];	// QTables
	uchar			*HTables[kMaxHuffmanTables];		// Huffman tables (DC 0-3, AC 0-3)

	char			bits;								// For reading bit by bit
	ulong			last32;
	ulong			next32;
	
	DrawRowProcPtr	SingleRowProc;	
	void*			DrawRefCon;
	
	BRect			mDrawRect;		// drawing position and size
	BRect			mClipRect;		// Clip drawing of MCU's
	
	short			blocks[kMCUSize*kMaxMCUs];			// Decoded MCU ( if not buffered )

	uchar			*buffer[kNumDecompressBuffers];		// Decompression image buffers
	long			rowBytes[kNumDecompressBuffers];

	long			Interval;		// Number of MCU's left in restart interval
	long			MCUCount;		// Number of MCU's drawn so far
	long			ScanMCUs;		// Number of MCU's in this scan

									// clipping information by MCU
	long			firstMCU;		// 		first visible MCU in image 
	long			lastMCU;		// 		last visible MCU in image
	long			leftMCU;		// 		first visible MCU in each row 
	long			rightMCU;		// 		last visible MCU in each row
		
									// clipping information by slice
	long			currentSlice;	//		current slice being decoded
	long			firstSlice;		//		first visible slice
	long			lastSlice;		//		last visible slice

	long			unusedSkip;		// unused marker data to ignore

	uchar			RstMarker;							// Last restart maker detected
	uchar			SkipToRestartMarker;				// skip all scan data up to and including this restart marker
	uchar			CompInFrame;						// Number of components in frame
	uchar			CompInScan;							// Number of components in scan
	
	short			RstInterval;						// 16 bit restart interval

	ushort			sampling;						// sampling interval code ( for display )
	
									// progressive decode stat
	ushort			bandSkip;		// number of blocks to skip in the current spectrum band

	ushort			scansProcessed;
	ushort			maxScansToDisplay;

	ushort			multiPassScan;

	
	ushort			WidthMCU;		// Width of image in MCU's
	ushort			ScanWidth;		// actual number of mcus in scan ( may differ if not interleaved )
	ushort			HeightMCU;		// Height of image in MCU's
	uchar			MCUHPixels;		// # of horizontal pixels in MCU
	uchar			MCUVPixels;		// # of vertical pixels in MCU

	JPEGError			error;							// Error during decode

	const			uchar *ZZ;

	unsigned		isProgressive:1;
	unsigned		thumbnail:1;			// Creates a 1/8 size thumbnail image instead	
	unsigned		skipToFirstSlice:1;
	unsigned		didDraw:1;
	unsigned		didClearDrawBuffer:1;
	unsigned		anotherPass:1;			// needs another full display pass on image
} JPEGDecoder;


//	Useful macros
//	Figure F.12 - Extending the sign bit of a decoded value in v
#define Extend(v, t) ((v) < (1 << ((t)-1)) ? (v) + (-1 << (t)) + 1 : (v))

//	Markers
#define	kSOF0	0xC0		// Baseline DCT
#define	kSOF1	0xC1		// Extended Sequential DCT
#define	kSOF2	0xC2		// Progressive DCT
#define	kSOF3	0xC3		// Lossless

#define kDHT	0xC4		// Define Huffman tables
#define kRST	0xD0		// kRSTm (0xD0 - 0xD7)*
#define	kSOI	0xD8		// Start of image*
#define	kEOI	0xD9		// End of image*
#define kSOS	0xDA		// Start of Scan
#define kDQT	0xDB		// Define quantization tables
#define kDNL	0xDC		// Define number of lines
#define kDRI	0xDD		// Define restart intervals
#define kAPP	0xE0		// Application specific (0xE0 - 0xEF)
#define kJPG	0xF0		// JPEG extentions (0xF0 - 0xFD)
#define kCOM	0xFE		// Comment


JPEGError JPEGBounds(const uchar* image, long size, BRect* bounds);
JPEGError JPEGDecodeImage(JPEGDecoder* j, const uchar* image, long size);

// Create and dispose JPEG decoders
JPEGDecoder* NewJPEGDecoder(const BRect* r, short thumbnail, DrawRowProcPtr drawProc, const BRect* invalid);
void		DisposeJPEGDecoder(JPEGDecoder* j);

//		JPEGWrite
//			Write data into the decoder
//			Returns number of bytes used or
//			0 if size is too small or
//			-ve if there has been an error
long	JPEGWrite(JPEGDecoder* j, const uchar* data, long dataLength,BRect *drewRect = NULL);

//		Interpret Q tables, huffman tables, frame, scan and APP info
JPEGError	InterpretSOF(JPEGDecoder* j, const uchar* data, short size);
JPEGError	InterpretSOS(JPEGDecoder* j, const uchar* data, short size);
JPEGError	InterpretDQT(JPEGDecoder* j, const uchar* data, short size);
JPEGError	InterpretDHT(JPEGDecoder* j, const uchar* data, short size);
JPEGError	InterpretDRI(JPEGDecoder* j, const uchar* data, short size);
JPEGError	InterpretAPP(JPEGDecoder* j, const uchar* data, short size);
JPEGError	InterpretCOM(JPEGDecoder* j, const uchar* data, short size);


void SetMaxScansToDisplay(JPEGDecoder* j,long scansToDisplay);

// Calls to draw decoded blocks
JPEGError	SetupDrawing(JPEGDecoder* j);
void DrawMCU(JPEGDecoder* j,const short *blocks,long x,long y);

// VLC low level calls
long DecodeComponent(JPEGDecoder* j, short count,CompSpec* cspec, short* blocks);


//	needed the following...
void DoTransform(JPEGDecoder *j,long mcuNum,const short *src);
void DumpJPEGDecode(JPEGDecoder* j);
JPEGError DrawSingleJPEGRow(void *i);
void DrawMCUPiece(JPEGDecoder *j, const short *blocks,long h,long v);
void TransformBlocks(JPEGDecoder *j,CompSpec *cspec,long count,const short *block, short *dqblock);

#endif
