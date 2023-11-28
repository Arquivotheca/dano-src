/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: inputfile.c
 *   project : MPEG Layer-3 Audio Encoder
 *   author  : Stefan Gewinner gew@iis.fhg.de
 *   date    : 1999-08-18
 *   contents/description: file reading utility functions
 *
 * $Header: /home/cvs/menc/ciEncoder/common/Attic/inputfile.c,v 1.1.2.7 1999/12/06 14:54:18 gew Exp $
 *
\***************************************************************************/

#include "inputfile.h"

/*
 * * *  WIN32 platform specifics  * * *
 */

#if defined(WIN32)

#include <windows.h>
#include <mmsystem.h>

#pragma comment (lib, "winmm.lib")

static MMCKINFO theParent, theSubchunk ;
static PCMWAVEFORMAT theFormat ;
static HMMIO theFile = NULL ;

unsigned int openInputFile (char *filename)
{
  MMRESULT err ;

  ZeroMemory (&theParent, sizeof (MMCKINFO)) ;
  ZeroMemory (&theFormat, sizeof (PCMWAVEFORMAT)) ;

  theFile = mmioOpen (filename, NULL, MMIO_READ | MMIO_ALLOCBUF)  ;
  if (theFile == NULL) return 0 ;

  theParent.fccType = mmioFOURCC ('W', 'A', 'V', 'E') ;
  theSubchunk.ckid = mmioFOURCC ('f', 'm', 't', ' ') ;

  if (err = mmioDescend (theFile, &theParent, NULL, MMIO_FINDRIFF))
    return 0 ;

  if (err = mmioDescend (theFile, &theSubchunk, &theParent, MMIO_FINDCHUNK))
    return 0 ;

  mmioRead (theFile, (LPSTR) &theFormat, sizeof (PCMWAVEFORMAT)) ;

  if (theFormat.wf.wFormatTag != WAVE_FORMAT_PCM)
    return 0 ;

  if (err = mmioAscend (theFile, &theSubchunk, 0))
    return 0 ;

  theSubchunk.ckid = mmioFOURCC ('d', 'a', 't', 'a') ;

  if (err = mmioDescend (theFile, &theSubchunk, &theParent, MMIO_FINDCHUNK))
    return 0 ;

  return 1 ;
}

unsigned int getSampleRateFromInputFile (void)
{
  return (theFile ? theFormat.wf.nSamplesPerSec : 0) ;
}

unsigned int getChannelCountFromInputFile (void)
{
  return (theFile ? theFormat.wf.nChannels : 0) ;
}

unsigned int getInputFileDataSize (void)
{
  return theSubchunk.cksize ;
}

unsigned int readFromInputFile (void *buffer, unsigned int buflen)
{
  return (theFile ? mmioRead (theFile, (HPSTR) buffer, buflen) : 0) ;
}

unsigned int closeInputFile (void)
{
  if (theFile)
  {
    mmioClose (theFile, 0) ;
    theFile = NULL ;

    return 1 ;
  }

  return 0 ;
}

/*
 * * *  MacOS platform specifics  * * *
 */

#elif defined(macintosh)

#include <StandardFile.h>
#include <Sound.h>

static short theFile = 0 ;

static unsigned long numFrames ;
static unsigned long dataOffset ;
static SoundComponentData sndInfo ;

unsigned int openInputFile (char *filename)
{
  StandardFileReply reply ;

  OSErr err = noErr ;
  OSType aiffType = 'AIFF' ;

  theFile = 0 ;

  filename ;

  /* toolbox must be initialized when we get here! */
  /* open and parse input file */

  StandardGetFile (NULL, 1, &aiffType, &reply) ;
  if (!reply.sfGood) return 0 ;

  err = FSpOpenDF (&reply.sfFile, fsRdPerm, &theFile) ;
  if (err != noErr) return 0 ;

  err = ParseAIFFHeader (theFile, &sndInfo, &numFrames, &dataOffset) ;
  if (err != noErr) return 0 ;

  err = SetFPos (theFile, fsFromStart, dataOffset) ;
  if (err != noErr) return 0 ;

  return 1 ;
}

unsigned int getSampleRateFromInputFile (void)
{
  return (theFile ? (UnsignedFixed) sndInfo.sampleRate >> 16 : 0) ;
}

unsigned int getChannelCountFromInputFile (void)
{
  return (theFile ? sndInfo.numChannels : 0) ;
}

unsigned int readFromInputFile (void *buffer, unsigned int buflen)
{
  if (theFile)
  {
    long numBytes = buflen ;
    FSRead (theFile, &numBytes, buffer) ;
    return (unsigned int) numBytes ;
  }

  return 0 ;
}

unsigned int closeInputFile (void)
{
  if (theFile)
  {
    FSClose (theFile) ;
    theFile = 0 ;
 
    return 1 ;
  }

  return 0 ;
}

#else

/*
 * * *  generic platform specifics  * * *
 *
 * reads 16-bit linear pcm data from a raw (headerless) file
 * requires some user interaction to determine audio parameters
 *
 */

#include <stdio.h>

static FILE *theFile = NULL ;

unsigned int openInputFile (char *filename)
{
  return ((theFile = fopen (filename, "rb")) ? 1 : 0) ;
}

unsigned int getSampleRateFromInputFile (void)
{
  unsigned int sampleRate = 0 ;

  printf ("sampling rate of raw input file: ") ;
  return (scanf ("%d", &sampleRate) ? sampleRate : 44100) ; // default?
}

unsigned int getChannelCountFromInputFile (void)
{
  unsigned int numChannels = 0 ;

  printf ("number of channels in raw input file: ") ;
  return (scanf ("%d", &numChannels) ? numChannels : 2) ; // default?
}

unsigned int readFromInputFile (void *buffer, unsigned int buflen)
{
  return (theFile ? fread (buffer, 1, buflen, theFile) : 0) ;
}

unsigned int closeInputFile (void)
{
  if (theFile)
  {
    fclose (theFile) ;
    theFile = NULL ;
 
    return 1 ;
  }

  return 0 ;
}

#endif
