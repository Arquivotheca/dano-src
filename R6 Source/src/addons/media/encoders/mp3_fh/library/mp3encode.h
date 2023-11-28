/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: mp3encode.h
 *   project : MPEG Layer-3 Audio Encoder
 *   author  : Stefan Gewinner gew@iis.fhg.de
 *   date    : 1999-08-18
 *   contents/description: public interface header file
 *
 * $Header: /home/cvs/menc/ciEncoder/common/Attic/mp3encode.h,v 1.1.2.19 1999/12/21 13:33:49 gew Exp $
 *
\***************************************************************************/

#ifndef __MP3ENCODE_H__
#define __MP3ENCODE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef WIN32
  #pragma pack(push, 8)
  #ifndef MP3ENCAPI
    #define MP3ENCAPI __stdcall
  #endif
#else
  #ifdef macintosh
    #if defined(__MRC__) || defined(__MWERKS__)
      #pragma options align=mac68k
    #endif
  #endif

  #ifndef MP3ENCAPI
    #define MP3ENCAPI
  #endif
#endif

/* v e r s i o n  s t a m p
  */

#define MP3ENC_VERSION ((unsigned int)0x00030101)

/* d a t a  t y p e s  
  */

typedef void *mp3EncHandle ;
typedef int mp3EncRetval ;

typedef struct mp3EncOutputFormat
{
  unsigned int sampleRate ;
  unsigned int numChannels ;
  unsigned int bitRate ;

} mp3EncOutputFormat, *mp3EncOutputFormatPtr ;

typedef struct mp3EncOutputFormatList
{
  unsigned int numEntries ;
  mp3EncOutputFormat format [1] ; /* contains numEntries entries */

} mp3EncFormatList, *mp3EncFormatListPtr ;

typedef enum mp3EncInputFormat
{
  mp3EncInputShort,
  mp3EncInputFloat

} mp3EncInputFormat ;

typedef enum mp3EncPaddingMode
{
  mp3EncPaddingISO,
  mp3EncPaddingNever,
  mp3EncPaddingAlways

} mp3EncPaddingMode ;

typedef enum mp3EncQuality
{
  mp3EncQualityFast,
  mp3EncQualityMedium,
  mp3EncQualityHighest

} mp3EncQualityMode ;

typedef struct mp3EncConfiguration
{
  mp3EncInputFormat inputFormat ;
  mp3EncPaddingMode paddingMode ;

  unsigned int allowIntensity ;
  unsigned int allowMidside ;
  unsigned int allowDownmix ;

  unsigned int writeCRCheck ;

  unsigned int vbrMode ; /* 0 = cbr, 1 - 100 = vbr quality */

  unsigned int privateBit ;
  unsigned int copyrightBit ;
  unsigned int originalBit ;

  float maximumBandwidth ;

  // read-only information:

  unsigned int codecDelay ;

} mp3EncConfiguration, *mp3EncConfigurationPtr ;

/* f u n c t i o n s
  */

mp3EncHandle MP3ENCAPI mp3EncOpen (unsigned int sampleRate,
                                   unsigned int numChannels,
                                   mp3EncQualityMode qualityMode) ;

mp3EncRetval MP3ENCAPI mp3EncEncode (mp3EncHandle hEncoder,
                                     void *inputBuffer,
                                     unsigned int bytesInput,
                                     unsigned int *bytesConsumed,
                                     void *outputBuffer,
                                     unsigned int bufferSize) ;

mp3EncFormatListPtr MP3ENCAPI mp3EncGetSupportedFormatList (mp3EncHandle hEncoder) ;
mp3EncRetval MP3ENCAPI mp3EncSetOutputFormat (mp3EncHandle hEncoder, mp3EncOutputFormatPtr format) ;

mp3EncConfigurationPtr MP3ENCAPI mp3EncGetCurrentConfiguration (mp3EncHandle hEncoder) ;
mp3EncRetval MP3ENCAPI mp3EncSetConfiguration (mp3EncHandle hEncoder, mp3EncConfigurationPtr config) ;

mp3EncRetval MP3ENCAPI mp3EncGetVBRIndexTable (mp3EncHandle hEncoder,
                                               void *outputBuffer,
                                               unsigned int bufferSize) ;

mp3EncRetval MP3ENCAPI mp3EncClose (mp3EncHandle hEncoder) ;

#ifdef WIN32
  #pragma pack(pop)
#else
  #ifdef macintosh
    #if defined(__MRC__) || defined(__MWERKS__)
      #pragma options align=reset
    #endif
  #endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MP3ENCODE_H__ */
