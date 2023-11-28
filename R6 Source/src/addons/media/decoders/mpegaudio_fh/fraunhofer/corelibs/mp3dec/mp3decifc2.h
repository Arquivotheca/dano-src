/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3decifc2.h
 *   project : MPEG Layer-3 Decoder
 *   author  : Martin Sieler
 *   date    : 1998-11-06
 *   contents/description: Mp3 Decoder interface 2 (C-style)
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:55 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decifc2.h,v 1.8 1999/03/11 08:51:55 sir Exp $
 */

#ifndef __MP3DECIFC2_H__
#define __MP3DECIFC2_H__

/*------------------------ includes ---------------------------------------*/

#ifdef WIN32
  #include <windows.h>
  #include <mmreg.h>
#else
  #include "regtypes.h"
#endif

#include "l3reg.h"

/* ------------------------ structure alignment ---------------------------*/

#ifdef WIN32
  #pragma pack(push, 8)
#endif

/*-------------------------- defines --------------------------------------*/

/*
 * calling convention
 */
#ifndef MP3DECAPI
  #ifdef WIN32
    #define MP3DECAPI
  #else
    #define MP3DECAPI
  #endif
#endif

/*-------------------------------------------------------------------------*/

/*
 * link spec
 *
 * NOTE: define __MP3DEC_DLL__ when using the DLL version of the mp3 decoder
 *
 */
#if defined(WIN32) && defined(__MP3DEC_DLL__)
  #ifdef __MP3DEC_ENTRYPOINT_CPP__
    #pragma message(__FILE__": ### Building MPEG Layer-3 Decoder DLL ###")
    #define LINKSPEC _declspec(dllexport)
  #else
    #define LINKSPEC _declspec(dllimport)
  #endif
#else
  #define LINKSPEC
#endif

/*-------------------------------------------------------------------------*/

/*
 * definition of return values for mp3dec2<xxx> functions
 */

#define MP3DEC2_OK                        0
#define MP3DEC2_ERR_INITFAILED            1
#define MP3DEC2_ERR_DONEFAILED            2
#define MP3DEC2_ERR_OPENFAILED            3
#define MP3DEC2_ERR_CLOSEFAILED           4
#define MP3DEC2_ERR_RESETFAILED           5
#define MP3DEC2_ERR_CONVERT               6
#define MP3DEC2_ERR_NOTALLOWED            7
#define MP3DEC2_ERR_ILLEGALCOMBINATION    8
#define MP3DEC2_ERR_WRONGPARA             9
#define MP3DEC2_ERR_WRONGSTRUCTSIZE      10
#define MP3DEC2_ERR_NOTOPEN              11

/*-------------------------------------------------------------------------*/

/*
 * MPEG Layer-3 context structure
 * (this structure acts as a handle to all mp3 decoder calls)
 *
 * in this context 
 *   'source' describes the MPEG Layer-3 input stream and
 *   'destination' the PCM output stream
 */

typedef struct
  {
  int   cbSize;             /* set this to sizeof(MP3DEC2_CONTEXT)       */

  /* SOURCE (MP3) */

  int   nSrcBitrate;        /* bitrate of source (in bit/s)              */
                            /* set to '1', if unknown                    */
                            /* this field will only be used to calculate */
                            /* src/dst buffer sizes via                  */
                            /* mp3dec2BufferSize()                       */
  int   nSrcChannels;       /* number of channels in source              */
  int   nSrcSFreq;          /* sampling frequency of source              */
  int   nSrcBlockSize;      /* block size (in bytes) of source           */
                            /* set to '1', if unknown                    */
  int   nSrcFramesPerBlock; /* number of frames per block in source      */
                            /* set to '1', if unknown                    */

  /* DESTINATION (PCM) */

  int   nDstChannels;       /* number of channels in destination         */
  int   nDstSFreq;          /* sampling frequency of destination         */
  int   nDstBitsPerSample;  /* bits per sample of destination            */

  /* PRIVATE DATA (DECODER HANDLE) */

  void *lpClientData;       /* pointer to decoder's private data         */
                            /* it will be set during Open() call         */
                            /* pass this value to every Convert() and    */
                            /* Close() call                              */
  } MP3DEC2_CONTEXT;

/*-------------------------------------------------------------------------*/

/*
 * definition of source and destination buffer types
 */

typedef struct
  {
  int            cbSize;       /* set this to sizeof(MP3DEC2_DATA)                */
  int            cbLength;     /* length (in bytes) of buffer pointed to by lpBuf */
  int            cbLengthUsed; /* number of bytes the decoder used from lpBuf     */
                               /* this field will be set by the decoder           */
  unsigned char *lpBuf;        /* address of the data buffer                      */
  } MP3DEC2_DATA;

/*-------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

/*
 * get version of library
 */
LINKSPEC unsigned long MP3DECAPI mp3dec2GetVersion(void);

/*
 * scan a buffer and fill out a MPEGLAYER3WAVEFORMAT structure
 */
LINKSPEC int MP3DECAPI mp3dec2ScanBufferFormat
    (
    unsigned char        *pBuffer, 
    int                   cbSize, 
    MPEGLAYER3WAVEFORMAT *pwf,
    int                   fForceBlocksize
    );

/*
 * calculate buffer sizes
 */
LINKSPEC int MP3DECAPI mp3dec2BufferSize
    (
    const MP3DEC2_CONTEXT *lpContext,
    MP3DEC2_DATA          *lpSrc,
    MP3DEC2_DATA          *lpDst,
    int                    fSrcBufferGiven
    );

/*
 * open decoding of a MPEG Layer-3 stream
 */
LINKSPEC int MP3DECAPI mp3dec2Open (MP3DEC2_CONTEXT *lpContext, int fQuery);

/*
 * close decoding of a MPEG Layer-3 stream
 */
LINKSPEC int MP3DECAPI mp3dec2Close(MP3DEC2_CONTEXT *lpContext);

/*
 * convert MPEG Layer-3 frame(s)
 */
LINKSPEC int MP3DECAPI mp3dec2Convert
    (
    MP3DEC2_CONTEXT *lpContext,
    MP3DEC2_DATA    *lpSrc,
    MP3DEC2_DATA    *lpDst
    );

/*
 * reset the decoder
 */
LINKSPEC int MP3DECAPI mp3dec2Reset(MP3DEC2_CONTEXT *lpContext);

#ifdef __cplusplus
  }
#endif

/*-------------------------------------------------------------------------*/

#ifdef WIN32
  #pragma pack(pop)
#endif

/*-------------------------------------------------------------------------*/
#endif
