/* ++++++++++

   FILE:  Riff.h
   REVS:  $Revision: 1.3 $
   NAME:  marc

   Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _RIFF_H
#define _RIFF_H

struct ChunkHeader {
  int32 fccChunk;
  int32 dwSize;
};

struct ListHeader {
  int32	fccChunk;
  int32	dwSize;
  int32 fccType;
};

struct RIFFCSETChunk {
  int16 wCodePage;
  int16 wCountryCode;
  int16 wLanguageCode;
  int16 wDialect;
};


/*
	WAVE Data Structures
*/   

#define WAVE_FORMAT_UNKNOWN    0x0000  /*  Microsoft Corporation  */
#define WAVE_FORMAT_PCM        0x0001
#define WAVE_FORMAT_ADPCM      0x0002  /*  Microsoft Corporation  */
#define WAVE_FORMAT_ALAW       0x0006  /*  Microsoft Corporation  */
#define WAVE_FORMAT_MULAW      0x0007  /*  Microsoft Corporation  */
#define WAVE_FORMAT_IMA_ADPCM  0x0011  /*  Intel Corporation  */
#define WAVE_FORMAT_DVI_ADPCM  0x0011  /*  Intel Corporation  */
#define WAVE_FORMAT_MPEG       0x0050  /*  Microsoft Corporation  */

struct WAVEfmtCommon {
  int16 wFormatTag;
  int16 wChannels;
  int32 dwSamplesPerSec;
  int32 dwAvgBytesPerSec;
  int16 wBlockAlign;
  int16 wBitsPerSample;
  int16	wSize;				/* optional */
};

struct WAVEfmtMSADPCM {
  int16 wNumCoef;
};

struct WAVEfmtIMAADPCM {
  int16 wSamplesPerBlock;
};

struct WAVEfmtMPEG {
  int16 wHeadLayer;
  int32 dwHeadBitrate;
  int16 wHeadMode;
  int16 wHeadModeExt;
  int16 wHeadEmphasis;
  int16 wHeadFlags;
  int32 dwPTSLow;
  int32 dwPSTHigh;
};

struct WAVEcuePoint {
  int32 dwName;
  int32 dwPosition;
  int32 fccChunk;
  int32 dwChunkStart;
  int32 dwBlockStart;
  int32 dwSampleOffset;
};


/*
	AVI Data Structures
*/   

#define AVIF_HASINDEX		0x00000010
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000

struct AVIavihChunk {
  int32 dwMicroSecPerFrame;
  int32 dwMaxBytesPerSec;
  int32 dwReserved1;
  int32 dwFlags;
  int32 dwTotalFrames;
  int32 dwInitialFrames;
  int32 dwStreams;
  int32 dwSuggestedBufferSize;
  int32 dwWidth;
  int32 dwHeight;
  int32 dwScale;
  int32 dwRate;
  int32 dwStart;
  int32 dwLength;
};

#define AVISF_DISABLED			0x00000001
#define AVISF_VIDEO_PALCHANGES	0x00010000

struct AVIstrhChunk {
  int32 fccType;
  int32 fccHandler;
  int32 dwFlags;
  int16 wPriority;
  int16 wLanguage;
  int32 dwInitialFrames;
  int32 dwScale;
  int32 dwRate;
  int32 dwStart;
  int32 dwLength;
  int32 dwSuggestedBufferSize;
  int32 dwQuality;
  int32 dwSampleSize;
  int16 wLeft;
  int16 wTop;
  int16 wRight;
  int16 wBottom;
};

struct AVIpcChunk {
  uint8 bFirstEntry;
  uint8 bNumEntries;
  uint16 wFlags;
};

#define AVIIF_LIST			0x00000001
#define AVIIF_KEYFRAME		0x00000010
#define AVIIF_FIRSTPART		0x00000020
#define AVIIF_LASTPART		0x00000040
#define AVIIF_MIDPART		0x00000060
#define AVIIF_NOTIME		0x00000100
#define AVIIF_CONTROLFRAME	0x00000200

struct AVIidx1Entry {
  int32 fccChunk;
  int32 dwFlags;
  int32 dwChunkOffset;
  int32 dwChunkLength;
};

#define BI_RGB		0x00000000
#define BI_RLE8		0x00000001
#define BI_RLE4		0x00000002
#define BI_NONE		0xFFFF0000
#define BI_PACK		0xFFFF0001
#define BI_TRANS	0xFFFF0002
#define BI_CCC		0xFFFF0003
#define BI_JPEGN	0xFFFF0004

struct AVIBitmapInfo {
  int32 dwSize;
  int32 dwWidth;
  int32 dwHeight;
  int16 wPlanes;
  int16 wBitCount;
  int32 dwCompression;
  int32 dwSizeImage;
  int32 dwXPelsPerMeter;
  int32 dwYPelsPerMeter;
  int32 dwClrUsed;
  int32 dwClrImportant;
};

#endif
