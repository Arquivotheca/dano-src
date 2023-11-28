//	kudos to Jason

#ifndef MIXER_I586_H
#define MIXER_I586_H

#include <support2/SupportDefs.h>

#if defined(__cplusplus)
#define EC extern "C"
#else
#define EC extern
#endif

#if __POWERPC__
#pragma internal on
#endif
EC void convertBufferFloatToShort( int16 *dest, float *src, int32 count, float scale );

EC void unsignedByteToFloatN( float *dest, unsigned char *src, float *gain, int channels, int samples );
EC void unsignedByteToFloat1( float *dest, unsigned char *src, float *gain, int samples );
EC void unsignedByteToFloat2( float *dest, unsigned char *src, float *gain, int samples );
EC void unsignedByteToFloat1to2( float *dest, unsigned char *src, float *gain, int samples );
EC void wordToFloatN( float *dest, int16 *src, float *gain, int channels, int samples );
EC void wordToFloat1( float *dest, int16 *src, float *gain, int samples );
EC void wordToFloat2( float *dest, int16 *src, float *gain, int samples );
EC void wordToFloat1to2( float *dest, int16 *src, float *gain, int samples );
EC void intToFloatN( float *dest, int32 *src, float *gain, int channels, int samples );
EC void intToFloat1( float *dest, int32 *src, float *gain, int samples );
EC void intToFloat2( float *dest, int32 *src, float *gain, int samples );
EC void intToFloat1to2( float *dest, int32 *src, float *gain, int samples );
EC void floatToFloatN( float *dest, float *src, float *gain, int channels, int samples );
EC void floatToFloat1( float *dest, float *src, float *gain, int samples );
EC void floatToFloat2( float *dest, float *src, float *gain, int samples );
EC void floatToFloat1to2( float *dest, float *src, float *gain, int samples );

EC void unsignedByteToFloatAccumN( float *dest, unsigned char *src, float *gain, int channels, int samples );
EC void unsignedByteToFloatAccum1( float *dest, unsigned char *src, float *gain, int samples );
EC void unsignedByteToFloatAccum2( float *dest, unsigned char *src, float *gain, int samples );
EC void unsignedByteToFloatAccum1to2( float *dest, unsigned char *src, float *gain, int samples );
EC void wordToFloatAccumN( float *dest, int16 *src, float *gain, int channels, int samples );
EC void wordToFloatAccum1( float *dest, int16 *src, float *gain, int samples );
EC void wordToFloatAccum2( float *dest, int16 *src, float *gain, int samples );
EC void wordToFloatAccum1to2( float *dest, int16 *src, float *gain, int samples );
EC void intToFloatAccumN( float *dest, int32 *src, float *gain, int channels, int samples );
EC void intToFloatAccum1( float *dest, int32 *src, float *gain, int samples );
EC void intToFloatAccum2( float *dest, int32 *src, float *gain, int samples );
EC void intToFloatAccum1to2( float *dest, int32 *src, float *gain, int samples );
EC void floatToFloatAccumN( float *dest, float *src, float *gain, int channels, int samples );
EC void floatToFloatAccum1( float *dest, float *src, float *gain, int samples );
EC void floatToFloatAccum2( float *dest, float *src, float *gain, int samples );
EC void floatToFloatAccum1to2( float *dest, float *src, float *gain, int samples );

EC void convertBufferFloatToUByte( int32 *dest, float *src, float gain, int32 samples );
EC void convertBufferFloatToWord( int32 *dest, float *src, float gain, int32 samples );
EC void convertBufferFloatToInt( int32 *dest, float *src, float gain, int32 samples );

#undef EC

#if __POWERPC__
#pragma internal reset
#endif

#endif // MIXER_I586_H
