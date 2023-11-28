//---------------------------------------------------------------------
//
//	File:	RIFFTypes.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	RIFF Type Definitions
//
//	Copyright ©1998 mediapede Software
//
//---------------------------------------------------------------------

#ifndef __RIFFTYPES_H__
#define __RIFFTYPES_H__

#include "AVINodeTypes.h"

//
//	Structs
//

//	RIFFChunk
typedef struct
{
	uint32 	ChunkID;	// 	Chunk ID marker
	uint32	ChunkSize;	// 	Size of the chunk data in bytes
	uchar	*ChunkData;	//	The Chunk data
} RIFFChunk;


//	AVIHeader
typedef struct 
{ 
	uint32 TimeBetweenFrames;     // Time delay between frames
	uint32 MaximumDataRate;       // Data rate of AVI data
	uint32 PaddingGranularity;    // Size of single unit of padding
	uint32 Flags;                 // Data parameters
	uint32 TotalNumberOfFrames;   // Number of video frame stored
	uint32 NumberOfInitialFrames; // Number of preview frames
	uint32 NumberOfStreams;       // Number of data streams in chunk
	uint32 SuggestedBufferSize;   // Minimum playback buffer size
	int32  Width;                 // Width of video frame in pixels
	int32  Height;                // Height of video frame in pixels
	uint32 TimeScale;             // Unit used to measure time
	uint32 DataRate;              // Data rate of playback
	uint32 StartTime;             // Starting time of AVI data
	uint32 DataLength;            // Size of AVI data chunk
} AVIHeader; 


//	AVIStreamHeader
typedef struct 
{ 
	uint32	DataType;           	// Data identifier
	uint32  DataHandler;        	// Device handler identifier
	uint32 	Flags;                 	// Data parameters
	uint32 	Priority;              	// Set to 0
	uint32 	InitialFrames;         	// Number of initial audio frames
	uint32 	TimeScale;             	// Unit used to measure time
	uint32 	DataRate;              	// Data rate of playback
	uint32 	StartTime;             	// Starting time of AVI data
	uint32 	DataLength;            	// Size of AVI data chunk
	uint32 	SuggestedBufferSize;	// Minimum playback buffer size
	uint32 	Quality;               	// Sample quailty factor
	uint32 	SampleSize;            	// Size of the sample in bytes
	uint32  rect[4];                // some sort of bounding box?
} AVIStreamHeader;

typedef struct
{
	uint32	Left;
	uint32	Top;
	uint32	Right;
	uint32	Bottom;
} AVIStreamHeaderFrame;

//	AVIIndex
typedef struct 
{ 
	uint32 ChunkID;		// Chunk identifier reference
	uint32 Flags;       // Type of chunk referenced 
	uint32 Offset;      // Position of chunk in file
	uint32 Length;      // Length of chunk in bytes
} AVIRawIndex; 

typedef struct 
{ 
	uint32 ChunkID;		// Chunk identifier reference
	uint32 Flags;       // Type of chunk referenced 
	uint64 Offset;      // Position of chunk in file
	uint32 Length;      // Length of chunk in bytes
} AVIIndex; 


//	RIFFJunk
typedef struct 
{ 
    uint32	ChunkId;       	// Chunk ID marker (JUNK)
    uint32	PaddingSize;   	// Size of the padding in bytes
    uchar 	*Padding;		// Padding
} RIFFJunk;


//	WaveCoefficient, used by MSADPCM compression
typedef struct WAVE_COEF_SET_STRUCT 
{
  int16	Coef1;
  int16	Coef2;
} WaveCoefficient;


//	AVIAUDSHeader 'auds'
typedef struct
{
	uint16 Format;					// S format
	uint16 Channels;       			// S channels
	uint32 SamplesPerSec;			// L rate
	uint32 AvgBytesPerSec;      	// L average bytes/sec
	uint16 BlockAlign;	  			// S block size of data
	uint16 BitsPerSample;           // S size
	uint16 ExtensionSize;       	// S size of following extenions - if any
	uint16 SamplesPerBlock;	  		// S Used by MSADPCM and Intel DVI
	uint16 NumCoefficients;	  		// S Used by MSADPCM  num of follow sets
	WaveCoefficient Coefficients[7];// Used by MSADPCM  coef1, coef2
	uint32 Style;          			// - SIGN2 or unsigned
	uint32 ByteCount;	  			// used to keep track of length
}	AVIAUDSHeader;


//	AVIVIDSHeader 'vids' 
typedef struct
{
	uint32 	Size;           //	Size
	int32 	Width;          // 	Width
	int32 	Height;         // 	Height
	uint16 	Planes;         // 	Planes
	uint16 	BitCount;       // 	BitCount
	uint32 	Compression;    // 	Compression
	uint32 	ImageSize;     	// 	SizeImage
	uint32 	XPelsPerMeter;	// 	XPelsPerMeter
	uint32 	YPelsPerMeter;	// 	YPelsPerMeter
	uint32 	NumColors;		//	Color used
	uint32 	ImpColors;    	//	Colors important
} AVIVIDSHeader;

#endif
