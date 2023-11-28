/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3decifc.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-28
 *   contents/description: Mp3 Decoder Interface
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/09/16 18:34:57 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decifc.h,v 1.13 1999/09/16 18:34:57 sir Exp $
 */

#ifndef __MP3DECIFC_H__
#define __MP3DECIFC_H__

/* ------------------------ includes --------------------------------------*/

#include "mp3sscdef.h"
#include "mp3streaminfo.h"
#include "mp3fileinfo.h"

#ifdef __cplusplus
  //
  // only needed for C++ interface
  //
  #include "giobase.h"
#endif

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
    #define MP3DECAPI __cdecl
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

/*-------------------------------------------------------------------------*\
 *
 *                   C o m m o n
 *
\*-------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

/*
 * Get a textual description of an SSC status code.
 */
LINKSPEC const char * MP3DECAPI mp3decGetErrorText(SSC ssc);

#ifdef __cplusplus
  }
#endif

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

#ifdef __cplusplus

//-------------------------------------------------------------------------*
//
//                   C + +   I N T E R F A C E
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//
//                   I M p g a D e c o d e r
//
//-------------------------------------------------------------------------*

//
// Mp3 Decoder Interface.
//
// This is the interface of the ISO/MPEG decoder object that
// interfaces with the application code.
//
// Use mp3decCreateObject() or mp3decCreateObjectEx() to 
// create a decoder instance.
//
// Call the interface Release() method to destroy this instance.
//
class IMpgaDecoder
{
public:

  // release this mp3 decoder instance
  virtual void Release();

  // reset the mp3 decoder instance
  virtual void Reset() = 0;
  
  // decode one mp3 frame.
  virtual SSC DecodeFrame(unsigned char *pPcm, int cbPcm, int *pcbUsed) = 0;
  virtual SSC DecodeFrame(float         *pPcm, int cbPcm, int *pcbUsed) = 0;

  // get information about the last successfully decoded frame.
  virtual const CMp3StreamInfo *GetStreamInfo() const = 0;

  // connect a input object to the mp3 decoder
  virtual void Connect(CGioBase *gf) = 0;

  // provide mp3 data to the mp3 decoder.
  virtual int Fill(const unsigned char *pBuffer, int cbBuffer) = 0;
  
  // check, how many bytes could be provided to the mp3 decoder.
  virtual int GetInputFree() const = 0;
  
  // check, how many bytes are left in the mp3 decoder input buffer.
  virtual int GetInputLeft() const = 0;
  
  // indicate EOF (end-of-file) to the mp3 decoder.
  virtual void SetInputEof() = 0;

  // check if the mp3 decoder reached EOF.
  virtual bool IsEof() const = 0;

protected:

  virtual ~IMpgaDecoder() {}

private:

};

//-------------------------------------------------------------------------*
//
//                   I M p 3 D e c I n f o 
//
//-------------------------------------------------------------------------*

//
// Mp3 Decoder Info Interface.
//
// Use mp3decCreateInfoObject() to create a decoder info instance.
//
// Call the interface Release() method to destroy this instance.
//
class IMp3DecInfo
{
public:

  // release this mp3 dec info instance
  virtual void Release();

  // initialize the mp3 decoder info object
  virtual bool Init(unsigned char *pBuffer, unsigned int cbBuffer, unsigned int cbMp3Data) = 0;

  // get file info object
  virtual const CMp3FileInfo *GetInfo() const = 0;
  
  // get seek point in bytes by percent
  virtual unsigned int SeekPointByPercent(float fPercent) const = 0;
  
  // get a seek point in bytes by time (in milli seconds)
  virtual unsigned int SeekPointByTime(unsigned int dwTime) const = 0;

protected:

  virtual ~IMp3DecInfo() {}

private:

};

/*-------------------------------------------------------------------------*/

extern "C" {

//-------------------------------------------------------------------------*
//   mp3decCreateObject
//-------------------------------------------------------------------------*

LINKSPEC SSC MP3DECAPI mp3decCreateObject(IMpgaDecoder **ppObject);

//-------------------------------------------------------------------------*
//   mp3decCreateObjectEx
//-------------------------------------------------------------------------*

LINKSPEC SSC MP3DECAPI mp3decCreateObjectEx
    (
    int            Quality,
    int            Resolution,
    int            Downmix,
    bool           UseFixedPoint,
    IMpgaDecoder **ppObject
    );

//-------------------------------------------------------------------------*
//   mp3decCreateObjectExtBuf
//-------------------------------------------------------------------------*

LINKSPEC SSC MP3DECAPI mp3decCreateObjectExtBuf
    (
    unsigned char *pBuffer,
    int            cbBuffer,
    int            Quality,
    int            Resolution,
    int            Downmix,
    bool           UseFixedPoint,
    IMpgaDecoder **ppObject
    );

//-------------------------------------------------------------------------*
//   mp3decCreateInfoObject
//-------------------------------------------------------------------------*

LINKSPEC SSC MP3DECAPI mp3decCreateInfoObject(IMp3DecInfo **ppObject);

/*-------------------------------------------------------------------------*/

} // extern "C"

/*-------------------------------------------------------------------------*/

#endif /* ifdef __cplusplus */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*\
 *
 *                   C   I N T E R F A C E
 *
\*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

typedef void *MP3DEC_HANDLE;
typedef void *MP3DECINFO_HANDLE;

/*-------------------------------------------------------------------------*/

#ifdef __cplusplus
  extern "C" {
#endif

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*\
 *
 *                   M p 3   D e c o d e r
 *
\*-------------------------------------------------------------------------*/

/*
 * open an instance of the mp3 decoder.
 */
LINKSPEC SSC MP3DECAPI mp3decOpen
    (
    MP3DEC_HANDLE *handle,
    int            Quality,
    int            Resolution,
    int            Downmix
    );

/*
 * close an open instance of the mp3 decoder.
 */
LINKSPEC SSC MP3DECAPI mp3decClose(MP3DEC_HANDLE handle);

/*
 * reset an open instance of the mp3 decoder.
 */
LINKSPEC SSC MP3DECAPI mp3decReset(MP3DEC_HANDLE handle);

/*
 * decode one mp3 frame.
 */
LINKSPEC SSC MP3DECAPI mp3decDecode
    (
    MP3DEC_HANDLE  handle, 
    unsigned char *pPcm, 
    int            cbPcm, 
    int           *pcbUsed
    );

/*
 * get information about the last successfully decoded frame.
 */
LINKSPEC const MP3STREAMINFO * MP3DECAPI mp3decGetStreamInfo(MP3DEC_HANDLE handle);

/*
 * provide mp3 data to the mp3 decoder.
 */
LINKSPEC SSC MP3DECAPI mp3decFill
    (
    MP3DEC_HANDLE        handle,
    const unsigned char *pBuffer,
    int                  cbBuffer,
    int                 *pcbCopied
    );

/*
 * check, how many bytes could be provided to the mp3 decoder.
 */
LINKSPEC SSC MP3DECAPI mp3decGetInputFree(MP3DEC_HANDLE handle, int *pValue);

/*
 * check, how many bytes are left in the mp3 decoder input buffer.
 */
LINKSPEC SSC MP3DECAPI mp3decGetInputLeft(MP3DEC_HANDLE handle, int *pValue);

/*
 * indicate EOF (end-of-file) to the mp3 decoder.
 */
LINKSPEC SSC MP3DECAPI mp3decSetInputEof(MP3DEC_HANDLE handle);

/*
 * check if the mp3 decoder reached EOF.
 */
LINKSPEC int MP3DECAPI mp3decIsEof(MP3DEC_HANDLE handle);

/*-------------------------------------------------------------------------*\
 *
 *                   M p 3   D e c o d e r   I n f o
 *
\*-------------------------------------------------------------------------*/

/*
 * open an mp3 decoder info instance.
 */
LINKSPEC SSC MP3DECAPI mp3decInfoOpen(MP3DECINFO_HANDLE *handle);

/*
 * close an open mp3 decoder info instance.
 */
LINKSPEC SSC MP3DECAPI mp3decInfoClose(MP3DECINFO_HANDLE handle);

/*
 * initialize the mp3 decoder info instance.
 */
LINKSPEC int MP3DECAPI mp3decInfoInit
    (
    MP3DECINFO_HANDLE  handle, 
    unsigned char     *pBuffer, 
    unsigned int       cbBuffer, 
    unsigned int       cbMp3Data
    );

/*
 * get file info structure.
 */
LINKSPEC const MP3FILEINFO * MP3DECAPI mp3decInfoGetInfo(MP3DECINFO_HANDLE handle);
  

/*
 * get seek point in bytes by percent.
 */
LINKSPEC unsigned int MP3DECAPI mp3decInfoSeekPointByPercent
    (
    MP3DECINFO_HANDLE handle,
    float             fPercent
    );
  
/*
 * get a seek point in bytes by time (in milli seconds).
 */
LINKSPEC unsigned int MP3DECAPI mp3decInfoSeekPointByTime
    (
    MP3DECINFO_HANDLE handle,
    unsigned int      dwTime
    );

/*-------------------------------------------------------------------------*/

#ifdef __cplusplus
  }
#endif

/*-------------------------------------------------------------------------*/

#ifdef WIN32
  #pragma pack(pop)
#endif

/*-------------------------------------------------------------------------*/
#endif
