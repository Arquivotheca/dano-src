
#ifndef __CINEPAK__
#define __CINEPAK__

#include    "3dDefs.h"
//#define kFor16

typedef unsigned char Byte;

enum {
	kKeyType			= 0x00,
	kInterType			= 0x01,

	kFrameType			= 0x00,
	kKeyFrameType		= kKeyType + kFrameType,
	kInterFrameType		= kInterType + kFrameType,

	kTileType			= 0x10,
	kTileKeyType		= kKeyType + kTileType,
	kTileInterType		= kInterType + kTileType,

	kCodeBookType		= 0x20,
	kFullDBookType		= 0x00 + kCodeBookType,
	kPartialDBookType 	= 0x01 + kCodeBookType,
	kFullSBookType		= 0x02 + kCodeBookType,
	kPartialSBookType 	= 0x03 + kCodeBookType,

	kCodesType			= 0x30,
	kIntraCodesType		= 0x00 + kCodesType,
	kInterCodesType		= 0x01 + kCodesType,
	kAllSmoothCodesType = 0x02 + kCodesType
};

typedef struct {
	long	frameSize;
	short	frameWidth;
	short	frameHeight;
	short	frameTileCount;
} FrameHeader, *FrameHeaderPtr;

typedef struct {
	long	tileSize;
	short	tileRectTop;
	short	tileRectLeft;
	short	tileRectBottom;
	short	tileRectRight;
} TileHeader, *TileHeaderPtr;

#define codecBadDataErr -1
#define kCodebookSize (256*4 + 256*16)

/*================================================================================================*/

short	cpDecompress(Byte *data, Byte *baseAddr, long rowBytes, long *codeBook, long depth);
void 	cpDrawIntraframe(Byte *baseAddr,long rowBytes,long width,long height, long *codebook, Byte *data);
void 	cpDrawInterframe(Byte *baseAddr,long rowBytes,long width,long height, long *codebook, Byte *data);

void 	cpDrawIntraframe8(Byte *baseAddr,long rowBytes,long width,long height, long *codebook, Byte *data);
void 	cpDrawInterframe8(Byte *baseAddr,long rowBytes,long width,long height, long *codebook, Byte *data);

/*================================================================================================*/


#endif
