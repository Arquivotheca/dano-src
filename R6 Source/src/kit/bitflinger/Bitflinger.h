#ifndef __BITFLINGER_P_H__
#define __BITFLINGER_P_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <OS.h>
#include <GL/gl.h>
#include <opengl/bitflinger.h>

typedef struct FloatColorRec
{
	float r;
	float g;
	float b;
	float a;
} FloatColor;

typedef struct IntColorRec
{
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
} IntColor;

typedef struct PixelModeRec
{
	int32 Format;			// Format:  GL_RED, GL_RGB, GL_LUMINANCE, ...
	int32 Type;				// Type:	GL_BYTE, GL_FLOAT, GL_UNSIGNED_INT_8_8_8_8, ...
	uint8 SwapEndian;
	uint8 LsbFirst;
	
	int32 Components;
	int32 R_Bits;
	int32 G_Bits;
	int32 B_Bits;
	int32 A_Bits;
	int32 R_HBit;
	int32 G_HBit;
	int32 B_HBit;
	int32 A_HBit;
	int32 bytes;
	uint8 isSigned;
	
	uint32 cache;
} PixelMode;

typedef struct TransferModeRec
{
	float ScaleR;
	float ScaleG;
	float ScaleB;
	float ScaleA;
	float ScaleD;
	float BiasR;
	float BiasG;
	float BiasB;
	float BiasA;
	float BiasD;
	int32 IndexShift;
	int32 IndexOffset;
	uint8 MapColor;			// Color map enabled
	uint8 MapStencil;		// Stencil map enabled
	
	uint8 isSimple;
} TransferMode;

typedef struct MapEntryRec
{
	int32 Size;
	void *Map;
} MapEntry;

typedef struct cvCacheEntryRec
{
	int32 inFormat;				// Format:  GL_RED, GL_RGB, GL_LUMINANCE, ...
	int32 inType;				// Type:	GL_BYTE, GL_FLOAT, GL_UNSIGNED_INT_8_8_8_8, ...
	uint8 inSwapEndian;
	uint8 inLsbFirst;
	int32 outFormat;
	int32 outType;
	uint8 outSwapEndian;
	uint8 outLsbFirst;
	uint8 MapColor;
	uint8 MapStencil;

	uint32 age;
	void *code;
	area_id areaID;
	int32 size;
} cvCacheEntry;

#define CV_CACHE_SIZE 8

typedef struct cvCacheRec
{
	cvCacheEntry entry[CV_CACHE_SIZE];
	uint32 clock;
} cvCache;

typedef struct cvOpModeRec
{
	int8 blendEnabled;
	
	uint32 logicFunc;
	
	uint32 blendSrcFunc;
	uint32 blendDstFunc;
	uint32 blendEqu;
	float blendR;
	float blendG;
	float blendB;
	float blendA;
	
	uint32 envMode;
	float envR;
	float envG;
	float envB;
	float envA;
	float envConstR;
	float envConstG;
	float envConstB;
	float envConstA;
	
	int8 writeR;
	int8 writeG;
	int8 writeB;
	int8 writeA;
} cvOpMode;

typedef struct MMXRegRec
{
	uint16 r;
	uint16 g;
	uint16 b;
	uint16 a;
} MMXReg;

typedef struct OptInfoRec
{
	MMXReg blendConstColor;				// 
	MMXReg blendConstAlpha;
	MMXReg oneMinusBlendConstColor;
	MMXReg oneMinusBlendConstAlpha;
	
	MMXReg scale;
	MMXReg bias;
	MMXReg envColor;
	MMXReg envConstColor;
	

	uint32 writeMask;

	int8 needDest;
	int8 needBias;
	int8 needScale;
	
	

} OptInfo;


typedef struct cvContextRec
{
	OptInfo opt;
	
	TransferMode transferMode;
	PixelMode in;
	PixelMode out;
	MapEntry pixelMap[10];

	void *alloc;
	
	cvOpMode op;
	

	int32 exBaseOffset;
	int32 exStride;
	
	int32 exCurrent;
	int32 stripCurrent;

	cvCache CacheExtractor;
	cvCache CacheStrips;
} cvContext;


#define CLAMP_01( n ) { \
	if( n < 0 ) n=0; \
	if( n > 1 ) n=1; }


#ifdef __cplusplus
}
#endif

#endif




