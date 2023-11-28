/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3decifc2.cpp
 *   project : MPEG Layer-3 Decoder
 *   author  : Martin Sieler
 *   date    : 1998-11-17
 *   contents/description: Mp3 Decoder interface 2 (C-style)
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 10:39:18 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decifc2.cpp,v 1.10 1999/03/11 10:39:18 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mp3decifc2.h"
#include "mp3ssc.h"
#include "mpgadecoder.h"
#include "mp3bufferparse.h"

#include "dbg.h"

#ifndef WIN32
  #include <stddef.h>
#endif

/*-------------------------------------------------------------------------*/

//
// definition of structure for private decoder data
//

typedef struct
  {
  int           cbSize;
  int           cbPcmBytesPerFrame;
  CMpgaDecoder *pDecoder;
  } MP3DEC2_CLIENTDATA;

typedef MP3DEC2_CLIENTDATA *LPMP3DEC2_CLIENTDATA;

/*-------------------------------------------------------------------------*/

//
// macro to build a version DWORD
//
// format: mmnnbbbb
//
//         mm    major version
//         nn    minor version
//         bbbb  build number
//

#define MAKEVERSION(m,n,b) (((m&0xff)<<24)|((n&0xff)<<16)|(b&0xffff))

/*-------------------------------------------------------------------------*/

//
// version of this library
//

static const unsigned long gdwVersion   = MAKEVERSION(2, 0, 42);

//
// token for invalid blocks
//

static const unsigned long gdwTokenAcm = 0x41434d00;
static const unsigned long gdwTokenCrc = 0x63726300;

//
// expected size of MPEGLAYER3 structures
//

static const int gcbContext    = sizeof(MP3DEC2_CONTEXT);
static const int gcbData       = sizeof(MP3DEC2_DATA);
static const int gcbClientData = sizeof(MP3DEC2_CLIENTDATA);

/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/

#ifdef DBG_ENABLE

static inline void DBG_DECERROR(const char *pszFunc, SSC ssc)
{
  if ( pszFunc )
    Dbg(1, "%s: %s", pszFunc, (const char*)CMp3Ssc(ssc));
  else
    Dbg(1, "*** Decoder: %s", (const char*)CMp3Ssc(ssc));
}

#else
  #define DBG_DECERROR(pszFunc,ssc)
#endif

/*-------------------------------------------------------------------------*/

//--------------------------------------------------------------------------
//  
//  DWORD mp3dec2MakeFrame
//  
//  Description:
//  generate a layer3 frame with a wrong CRC checksum
//
//  Arguments:
//  unsigned char *pDst destination buffer (>= 144 bytes!!)
//  int    nChannels    number of channels
//  int    nSFreq       sampling frequency [Hz]
//
//  Return (int):
//	size of frame in bytes
//  
//-------------------------------------------------------------------------

static int mp3dec2MakeFrame
  (
  unsigned char *pDst,
  int            nChannels,
  int            nSFreq
  )
{
  const unsigned short wCheckSum  = 0xcafe;
  unsigned long        dwMd;
  unsigned long        dwMpegHdr;
  unsigned long        dwId       = 0;
  unsigned long        dwSf       = 0;
  int                  nFrameSize = 0;
  int                  i;

  //
  // set bitstream id and sampling frequency
  //
  switch ( nSFreq )
    {
    case 48000:
      nFrameSize =  96;
      dwId       = 0x3 << 19;
      dwSf       = 0x1 << 10;
      break;
    case 44100:
      nFrameSize = 104;
      dwId       = 0x3 << 19;
      dwSf       = 0x0 << 10;
      break;
    case 32000:
      nFrameSize = 144;
      dwId       = 0x3 << 19;
      dwSf       = 0x2 << 10;
      break;
    case 24000:
      nFrameSize =  24;
      dwId       = 0x2 << 19;
      dwSf       = 0x1 << 10;
      break;
    case 22050:
      nFrameSize =  26;
      dwId       = 0x2 << 19;
      dwSf       = 0x0 << 10;
      break;
    case 16000:
      nFrameSize =  36;
      dwId       = 0x2 << 19;
      dwSf       = 0x2 << 10;
      break;
    case 12000:
      nFrameSize =  48;
      dwId       = 0x0 << 19;
      dwSf       = 0x1 << 10;
      break;
    case 11025:
      nFrameSize =  52;
      dwId       = 0x0 << 19;
      dwSf       = 0x0 << 10;
      break;
    case  8000:
      nFrameSize =  72;
      dwId       = 0x0 << 19;
      dwSf       = 0x2 << 10;
      break;
    }
  
  //
  // set mode
  //
  dwMd = ((nChannels==2) ? 0x0 : 0x3) << 6;

  //
  // build MPEG header
  //
  dwMpegHdr = 0xffe21000 | dwId | dwSf | dwMd;

  //
  // reset destination buffer to zero
  //
  for ( i=0; i<nFrameSize; i++ )
    pDst[i] = 0;

  //
  // copy MPEG header to destination buffer
  //
  pDst[0] = (unsigned char)((dwMpegHdr >> 24) & 0xff);
  pDst[1] = (unsigned char)((dwMpegHdr >> 16) & 0xff);
  pDst[2] = (unsigned char)((dwMpegHdr >>  8) & 0xff);
  pDst[3] = (unsigned char)((dwMpegHdr      ) & 0xff);

  //
  // set CRC Checksum in destination buffer
  //
  pDst[4] = (wCheckSum >> 8) & 0xff;
  pDst[5] = (wCheckSum     ) & 0xff;

  //
  // return size of frame
  //
  return nFrameSize;
}

//--------------------------------------------------------------------------
//  
//  void mp3dec2FillBuffer
//  
//  Description:
//  fill input buffer of decoder
//
//  Return (int):
//	Total number of bytes used from source buffer
//  
//-------------------------------------------------------------------------

static int mp3dec2FillBuffer(const MP3DEC2_CONTEXT *lpContext, MP3DEC2_DATA *lpSrc)
{
  unsigned long  dwToken1, dwToken2;
  unsigned char *pbAct;
  unsigned char  aFrame[144];
  int            cbTotalUsed = 0;
  int            cbUsed;
  int            cbFree;
  int            cbFrame;
  int            i;

  CMpgaDecoder *pDec = LPMP3DEC2_CLIENTDATA(lpContext->lpClientData)->pDecoder;

  //
  // check if BlockSize is <= 1 and the size of the
  // source buffer is a multiple of BlockSize
  //
  if ( (lpContext->nSrcBlockSize <= 1) || 
       ((lpSrc->cbLength % lpContext->nSrcBlockSize) != 0)
     )
    {
    //
    // we do _not_ make any error concealment in this case
    // feed as much data into the decoder input buffer as possible
    //

    cbUsed = pDec->Fill(&lpSrc->lpBuf[lpSrc->cbLengthUsed], lpSrc->cbLength - lpSrc->cbLengthUsed);

    //
    // update number of source bytes used
    //
    lpSrc->cbLengthUsed += cbUsed;
    cbTotalUsed         += cbUsed;
    }
  else
    {
    //
    // process each block of "nBlockSize" bytes length
    //
    while ( (lpSrc->cbLength-lpSrc->cbLengthUsed) >= lpContext->nSrcBlockSize )
      {
      //
      // check if "nBlockSize" bytes will at least be accepted by
      // decoder's input buffer
      //
      cbFree = pDec->GetInputFree();

      if ( cbFree < lpContext->nSrcBlockSize )
        {
        Dbg(5, "mp3audioDecodeFillBuffer(): %d bytes free (%d expected)",
            cbFree, lpContext->nSrcBlockSize);
        break;
        }

      //
      // check if this is an "invalid" block
      //
      pbAct    = &lpSrc->lpBuf[lpSrc->cbLengthUsed];
      dwToken1 = (pbAct[0]<<24) | (pbAct[1]<<16) | (pbAct[2]<<  8) | (pbAct[3]);
      dwToken2 = (pbAct[4]<<24) | (pbAct[5]<<16) | (pbAct[6]<<  8) | (pbAct[7]);

      if ( (dwToken1 == gdwTokenAcm) && (dwToken2 == gdwTokenCrc) )      
        {
        Dbg(3, "mp3audioDecodeFillBuffer(): invalid Block detected");
        //
        // generate a valid layer3 frame with a wrong CRC checksum
        //
        cbFrame = mp3dec2MakeFrame(aFrame, lpContext->nSrcChannels, lpContext->nSrcSFreq);

        //
        // feed the right number of frames into the decoder's input buffer
        //
        for ( i=0; i<lpContext->nSrcFramesPerBlock; i++ )
          {
          cbUsed = pDec->Fill(aFrame, cbFrame);
          }

        //
        // update number of source bytes used
        //
        lpSrc->cbLengthUsed += lpContext->nSrcBlockSize;
        cbTotalUsed         += lpContext->nSrcBlockSize;
        }
      else
        {
        //
        // fill nBlockSize bytes
        //
        cbUsed = pDec->Fill(&lpSrc->lpBuf[lpSrc->cbLengthUsed], lpContext->nSrcBlockSize);

        //
        // update number of source bytes used
        //
        lpSrc->cbLengthUsed += cbUsed;
        cbTotalUsed         += cbUsed;

        //
        // cbUsed _must_ equal nBlockSize as we checked for enough space before
        //
        if ( cbUsed != lpContext->nSrcBlockSize )
          {
          Dbg(1, "*** mp3audioDecodeFillBuffer(): copied %d bytes (%d expected)", 
            cbUsed, lpContext->nSrcBlockSize);
          }
        }
      }
    }

  return cbTotalUsed;
}

//-------------------------------------------------------------------------*
//
//                   E x p o r t e d   F u n c t i o n s
//
//-------------------------------------------------------------------------*

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2GetVersion
//   description : return the version of this library
//
//   returns     : version
//
//--------------------------------------------------------------------------

unsigned long MP3DECAPI mp3dec2GetVersion(void)
{
  return gdwVersion;
}

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2ScanBufferFormat
//   description : scan a buffer and fill out a MPEGLAYER3WAVEFORMAT
//                 structure
//
//   returns     : offset in bits of first mpeg header in buffer
//
//--------------------------------------------------------------------------

int MP3DECAPI mp3dec2ScanBufferFormat
    (
    unsigned char        *pBuffer, 
    int                   cbSize, 
    MPEGLAYER3WAVEFORMAT *pwf,
    int                   fForceBlocksize
    )
{
  return mp3ScanBufferFormat(pBuffer, cbSize, *pwf, fForceBlocksize?true:false);
}

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2BufferSize
//   description : calculate output dst size, given a src buffer size 
//                 or vice versa
//
//   returns : error/status
//
//--------------------------------------------------------------------------

int MP3DECAPI mp3dec2BufferSize
    (
    const MP3DEC2_CONTEXT *lpContext,
    MP3DEC2_DATA          *lpSrc,
    MP3DEC2_DATA          *lpDst,
    int                    fSrcBufferGiven
    )
{
  int cb = 0;
  int nSamplesPerFrame;
  int nBytesPerFrame;
  int nPcmSamples;
  int nFrames;
  int nDownsamplingFactor;

  //
  // check if cbSize of args is correct
  //
  if ( lpContext->cbSize != gcbContext || 
       lpSrc->cbSize     != gcbData    || 
       lpDst->cbSize     != gcbData 
     )
    {
    return MP3DEC2_ERR_WRONGSTRUCTSIZE;
    }

  if ( fSrcBufferGiven )
    {
    // ---------------------------------------------------------------
    //
    // S O U R C E   B U F F E R   S I Z E   G I V E N
    //
    // ---------------------------------------------------------------

    //
    // reset destination buffer size to zero
    //
    lpDst->cbLength = 0;

    // ---------------------------------------------------------------
    //
    //  DECODING: LAYER3 -> PCM:
    //  how many destination PCM bytes are needed to hold
    //  the decoded Layer3 Audio data of lpSrc->cbLength bytes
    //
    //  always round UP
    //
    //  PCM samples/frame, PCM format   -> PCM bytes/frame
    //  bytes/frame, size of src buffer -> # frames in src buffer
    //  
    //  ---> dst buffer = # frames * PCM bytes/frame
    //
    //  !!!! the decoder will produce 
    //  nSamplesPerFrame/nDownsamplingFactor samples as output
    //

    nDownsamplingFactor = (lpContext->nSrcSFreq/lpContext->nDstSFreq);

    nSamplesPerFrame    = (lpContext->nSrcSFreq > 24000)? 1152:576;

    nBytesPerFrame      = (lpContext->nSrcBitrate * nSamplesPerFrame / 8) /
                          lpContext->nSrcSFreq;
    
    nFrames             = lpSrc->cbLength / nBytesPerFrame;
    
    // round up
    if ( lpSrc->cbLength % nBytesPerFrame )
      nFrames++;
    
    cb = (lpContext->nDstBitsPerSample / 8) *            // bytes per sample
         lpContext->nDstChannels *                       // number of channels
         nFrames * nSamplesPerFrame/nDownsamplingFactor; // number of samples
    
    Dbg(7, "mp3dec2BufferSize(): Src=%d, Dst=%d (src given)",
        lpSrc->cbLength, cb);
    
    //
    // return error if zero or negative
    //
    if ( cb <= 0 )
      {
      Dbg(1, "*** mp3dec2BufferSize(): Destination buffer <= 0");
      return MP3DEC2_ERR_ILLEGALCOMBINATION;
      }
    
    lpDst->cbLength = cb;
    return MP3DEC2_OK;
    }
  else
    {
    // ---------------------------------------------------------------
    //
    // D E S T I N A T I O N   B U F F E R   S I Z E   G I V E N
    //
    
    //
    // reset source buffer size to zero
    //
    lpSrc->cbLength = 0;
    
    // ---------------------------------------------------------------
    //
    //  DECODING: LAYER3 -> PCM
    //  how many source Layer3 Audio bytes can be decoded into a
    //  destination buffer of lpDst->cbLength bytes
    //
    //  always round DOWN
    //
    //  samples/frame, dst buffer    -> # PCM samples
    //  # PCM samples, samples/frame -> # frames
    //
    //  ---> src buffer = bytes/frame * # frames
    // 
    //  !!!! the decoder will produce 
    //  nSamplesPerFrame/nDownsamplingFactor as output
    //

    nDownsamplingFactor = (lpContext->nSrcSFreq/lpContext->nDstSFreq);

    nSamplesPerFrame    = (lpContext->nSrcSFreq > 24000)? 1152:576;

    nPcmSamples         = lpDst->cbLength /
                          (lpContext->nDstChannels * lpContext->nDstBitsPerSample/8);

    nFrames             = nPcmSamples / (nSamplesPerFrame/nDownsamplingFactor);

    nBytesPerFrame      = (lpContext->nSrcBitrate * nSamplesPerFrame / 8) /
                          lpContext->nSrcSFreq;
    
    cb = nFrames * nBytesPerFrame;
    
    Dbg(7, "mp3dec2BufferSize(): Src=%d, Dst=%d (dst given)", 
        cb, lpDst->cbLength);
    
    //
    // return error if zero
    //
    if ( cb <= 0 )
      {
      Dbg(1, "*** mp3dec2BufferSize(): Source buffer <= 0");
      return MP3DEC2_ERR_ILLEGALCOMBINATION;
      }
    
    lpSrc->cbLength = cb;
    return MP3DEC2_OK;
    }
}

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2Open
//   description : open decoding of a MPEG Layer-3 stream
//
//   returns : error/status
//
//--------------------------------------------------------------------------

int MP3DECAPI mp3dec2Open (MP3DEC2_CONTEXT *lpContext, int fQuery)
{
  //
  // check if cbSize of args is correct
  //
  if ( lpContext->cbSize != gcbContext )
    {
    return MP3DEC2_ERR_WRONGSTRUCTSIZE;
    }

  //
  // check if destination bits/sample is allowed
  //
  if ( 16 != lpContext->nDstBitsPerSample && 8 != lpContext->nDstBitsPerSample )
    {
    Dbg(1, "*** mp3dec2Open(): invalid dst bits/sample (%d)", lpContext->nDstBitsPerSample);
    return MP3DEC2_ERR_ILLEGALCOMBINATION;
    }

  //
  // check if combination of src and dst channel count is allowed
  //
  //  possible combinations:
  //    stereo Layer3 -> stereo PCM: ok
  //    stereo Layer3 -> mono   PCM: ok
  //    mono   Layer3 -> mono   PCM: ok
  //
  //  forbidden combinations:
  //    mono   Layer3 -> stereo PCM: NOT POSSIBLE
  //
  if ( (lpContext->nSrcChannels == 1) && (lpContext->nDstChannels == 2) )
    {
    Dbg(1, "*** mp3dec2Open(): invalid settings (mono mp3 -> stereo pcm)");
    return MP3DEC2_ERR_ILLEGALCOMBINATION;
    }

  //
  // check if combination of src and dst samplerate is allowed
  //
  //  this driver is able to do downsampling by 2 or 4
  //
  if ( (lpContext->nDstSFreq != lpContext->nSrcSFreq    ) && 
    (lpContext->nDstSFreq != lpContext->nSrcSFreq / 2) && 
    (lpContext->nDstSFreq != lpContext->nSrcSFreq / 4)
    )
    {
    Dbg(1, "*** mp3dec2Open(): invalid samplerate combination (src: %ld - dst: %ld)",
      lpContext->nSrcSFreq, lpContext->nDstSFreq);
    return MP3DEC2_ERR_ILLEGALCOMBINATION;
    }

  //
  // we have determined that the conversion requested is possible.
  // now check if we are just being queried for support.
  // if this is just a query, then do NOT open the decoder stream
  // just succeed the call.
  //
  if ( fQuery )
    {
    return MP3DEC2_OK;
    }

  //
  // allocate client data
  //
  LPMP3DEC2_CLIENTDATA pcd = new MP3DEC2_CLIENTDATA;

  if ( pcd )
    {
    pcd->cbSize             = sizeof(MP3DEC2_CLIENTDATA);
    pcd->pDecoder           = NULL;
    lpContext->lpClientData = pcd;
    }
  else
    {
    Dbg(1, "*** mp3dec2Open(): cannot allocate memory for client data");
    lpContext->lpClientData = NULL;
    return MP3DEC2_ERR_OPENFAILED;
    }

  //
  // calculate number of PCM bytes per frame
  //
  // - MPEG1        : 1152 samples/frame (32 kHz - 48 kHz)
  // - MPEG2/MPEG2.5:  576 samples/frame ( 8 kHz - 24 kHz)
  //
  // if downsampling is selected the number of output samples
  // must be divided by the downsampling factor
  //
  int SamplesPerFrame     = (lpContext->nSrcSFreq > 24000) ? 1152:576;
  SamplesPerFrame        /= (lpContext->nSrcSFreq/lpContext->nDstSFreq);
  pcd->cbPcmBytesPerFrame = SamplesPerFrame * 
                            lpContext->nDstChannels * (lpContext->nDstBitsPerSample/8);
  
  //
  // calculate parameters for mpegOpen
  //
  // - nQuality:    0 no downsampling
  //                1 downsampling by 2
  //                2 downsampling by 4
  //
  // - nResolution: 0 16bit PCM
  //                1  8bit PCM
  //
  // - nDownMix:    0 no downmix
  //                1 downmix (stereo mp3 - mono pcm)
  //
  int nQuality    = (lpContext->nSrcSFreq/lpContext->nDstSFreq) >> 1;
  int nResolution = (lpContext->nDstBitsPerSample==8) ? 1:0;
  int nDownmix    = ((lpContext->nSrcChannels==2)&&(lpContext->nDstChannels==1))? 1:0;
  
  //
  // open stream
  //
  Dbg(5, "mp3dec2Open(): quality = %d, resolution = %d, downmix = %d",
    nQuality, nResolution, nDownmix);
  
  pcd->pDecoder = new CMpgaDecoder(nQuality, nResolution, nDownmix);
  
  //
  // check for error
  //
  if ( NULL == pcd->pDecoder )
    {
    // delete client data
    delete pcd;

    lpContext->lpClientData = NULL;

    Dbg(1, "*** mp3dec2Open(): cannot allocate memory for decoder object");
    return MP3DEC2_ERR_OPENFAILED;
    }

  return MP3DEC2_OK;
}

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2Close
//   description : close decoding of a MPEG Layer-3 stream
//
//   returns     : error/status
//
//--------------------------------------------------------------------------

int MP3DECAPI mp3dec2Close(MP3DEC2_CONTEXT *lpContext)
{
  LPMP3DEC2_CLIENTDATA pcd = (LPMP3DEC2_CLIENTDATA)lpContext->lpClientData;

  //
  // check if cbSize of args is correct
  //
  if ( lpContext->cbSize != gcbContext )
    {
    return MP3DEC2_ERR_WRONGSTRUCTSIZE;
    }

  //
  // check if client data is valid
  //
  if ( pcd == NULL || pcd->cbSize != gcbClientData )
    {
    return MP3DEC2_ERR_NOTOPEN;
    }

  //
  // close decoding stream
  //
  if ( pcd->pDecoder )
    {
    delete pcd->pDecoder;
    pcd->pDecoder = NULL;
    }

  //
  // free client data
  //
  if ( pcd )
    {
    delete pcd;
    lpContext->lpClientData = NULL;
    }

  return MP3DEC2_OK;
}

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2Convert
//   description : convert MPEG Layer-3 frame(s)
//
//   returns     : error/status
//
//--------------------------------------------------------------------------

int MP3DECAPI mp3dec2Convert
    (
    MP3DEC2_CONTEXT *lpContext,
    MP3DEC2_DATA    *lpSrc,
    MP3DEC2_DATA    *lpDst
    )
{
  LPMP3DEC2_CLIENTDATA pcd     = (LPMP3DEC2_CLIENTDATA)lpContext->lpClientData;
  int                  nReturn = MP3DEC2_OK;
  SSC                  ssc;
  int                  nOutputSize;
  int                  cbCopied;

  //
  // check if cbSize of args is correct
  //
  if ( lpContext->cbSize != gcbContext || 
       lpSrc->cbSize     != gcbData    || 
       lpDst->cbSize     != gcbData 
     )
    {
    return MP3DEC2_ERR_WRONGSTRUCTSIZE;
    }

  //
  // check if client data is valid
  //
  if ( pcd == NULL || pcd->cbSize != gcbClientData )
    {
    return MP3DEC2_ERR_NOTOPEN;
    }

  //
  // get the decoder object
  //
  CMpgaDecoder *pDec = pcd->pDecoder;

  //
  // reset number of Src/Dst bytes used
  //
  lpSrc->cbLengthUsed = 0;
  lpDst->cbLengthUsed = 0;

  //
  // decode frame(s)
  //
  while ( 1 )
    {
    //
    // feed data into decoder's input buffer
    // (will update number of source bytes used)
    //
    cbCopied = mp3dec2FillBuffer(lpContext, lpSrc);

    //
    // try to decode all frames in the (internal) inputbuffer.
    // we break this loop if 
    //   - decoding was not successfull (warning or error).
    //     (an empty input buffer will generate a warning.)
    //   - no more space in the output buffer
    //
    ssc = SSC_OK;

    while ( SSC_SUCCESS(ssc) )
      {
      //
      // check if another frame may be decoded
      //
      if ( pcd->cbPcmBytesPerFrame > (lpDst->cbLength-lpDst->cbLengthUsed) )
        {
        break;
        }

      //
      // decode frame and update bytes used in dest. buffer
      //
      ssc = pDec->DecodeFrame(&lpDst->lpBuf[lpDst->cbLengthUsed],
                              lpDst->cbLength - lpDst->cbLengthUsed,
                              &nOutputSize);
    
      lpDst->cbLengthUsed += nOutputSize;

      //
      // check if nOutputSize matches calculated size
      //
      if ( nOutputSize && (nOutputSize != pcd->cbPcmBytesPerFrame) )
        {
        Dbg(1, "*** mp3dec2Convert(): %d bytes produced, %d expected",
            nOutputSize, pcd->cbPcmBytesPerFrame);
        nReturn = MP3DEC2_ERR_CONVERT; // should be a "fatal error" code
        break;
        }

      //
      // check state
      //
      if ( SSC_ERROR(ssc) )
        {
        //
        // decoder error -> break and return an errorcode
        //
        DBG_DECERROR("mp3dec2Convert() [DecodeFrame]", ssc);
        nReturn = MP3DEC2_ERR_CONVERT;
        break;
        }

      if ( ssc == SSC_W_MPGA_SYNCLOST )
        {
        //
        // sync lost is _ignored_ here
        //
        Dbg(1, "*** mp3dec2Convert(): SYNC LOST ***");
        }

      if ( SSC_SUCCESS(ssc) && pDec->GetStreamInfo()->GetCrcError() )
        {
        //
        // CRC errors are handled by decoder core
        //
        Dbg(3, "mp3dec2Convert(): CRC error ***");
        }
      } // while ( SSC_SUCCESS(ssc)

    //
    // break loop on (decoding) errors
    //
    if ( (MP3DEC2_OK != nReturn) || SSC_ERROR(ssc) )
      {
      break;
      }

    //
    // break loop if output buffer is full
    //
    if ( pcd->cbPcmBytesPerFrame > (lpDst->cbLength-lpDst->cbLengthUsed) )
      {
      break;
      }

    //
    // stop decoding, if
    //   - no more bytes were copied to input buffer AND
    //   - input buffer is empty 
    //
    if ( (cbCopied == 0) && ( (ssc == SSC_W_MPGA_SYNCNEEDDATA) ||
                              (ssc == SSC_W_MPGA_SYNCSEARCHED) ||
                              (ssc == SSC_W_MPGA_SYNCEOF     ) )
       )
      {
      break;
      }
    } // while (1)

  return nReturn;
}

//--------------------------------------------------------------------------
//
//   functionname: mp3dec2Reset
//   description : reset the MPEG Layer-3 decoder
//
//   returns     : error/status
//
//--------------------------------------------------------------------------

int MP3DECAPI mp3dec2Reset(MP3DEC2_CONTEXT *lpContext)
{
  LPMP3DEC2_CLIENTDATA pcd = (LPMP3DEC2_CLIENTDATA)lpContext->lpClientData;

  //
  // check if cbSize of args is correct
  //
  if ( lpContext->cbSize != gcbContext )
    {
    return MP3DEC2_ERR_WRONGSTRUCTSIZE;
    }

  //
  // check if client data is valid
  //
  if ( pcd == NULL || pcd->cbSize != gcbClientData )
    {
    return MP3DEC2_ERR_NOTOPEN;
    }

  //
  // reset decoder
  //
  pcd->pDecoder->Reset();

  return MP3DEC2_OK;
}

/*-------------------------------------------------------------------------*/
