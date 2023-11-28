/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: mp3fileinfo.h
 *   project : MPEG Layer-3 Decoder
 *   author  : Martin Sieler
 *   date    : 1999-09-13
 *   contents/description: mp3 dec info stucture
 *
\***************************************************************************/

/*
 * $Date: 1999/09/14 17:37:32 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3fileinfo.h,v 1.1 1999/09/14 17:37:32 sir Exp $
 */

#ifndef __MP3FILEINFO_H__
#define __MP3FILEINFO_H__

/* ------------------------ structure alignment ---------------------------*/

#ifdef WIN32
  #pragma pack(push, 1)
#endif

/*-------------------------------------------------------------------------*/

typedef struct
{
  int          m_AvgBitrate;           /* bitrate (bit/s)                    */
  int          m_Channels;             /* number of channels                 */
  int          m_Samplerate;           /* sampling frequency (Hz)            */
  unsigned int m_Duration;             /* duration of file in milli seconds  */
  int          m_bVbr;                 /* variable or constant bitrate       */
  int          m_bVbrHeader;           /* vbr header available               */
  int          m_bSeekPointsAvailable; /* seek points available              */
  } MP3FILEINFO;

/*-------------------------------------------------------------------------*/

#ifdef __cplusplus

//
// Mp3 file info object.
//
//  Object holding information on a mp3 file.
//

class CMp3FileInfo : protected MP3FILEINFO
{
public:

  int          GetAvgBitrate()        const { return m_AvgBitrate; }
  int          GetChannels()          const { return m_Channels; }
  int          GetSamplerate()        const { return m_Samplerate; }
  unsigned int GetDuration()          const { return m_Duration; }
  bool         GetIsVbr()             const { return m_bVbr ? true : false; }
  bool         GetIsVbrHeader()       const { return m_bVbrHeader ? true : false; }
  bool         GetIsSeekPointsAvail() const { return m_bSeekPointsAvailable ? true : false; }

protected:

  friend class CMp3DecInfo;

  void Reset()
    {
    m_AvgBitrate           = 0;
    m_Channels             = 0;
    m_Samplerate           = 0;
    m_Duration             = 0;
    m_bVbr                 = 0;
    m_bVbrHeader           = 0;
    m_bSeekPointsAvailable = 0;
    }

private:

};

#endif /* __cplusplus */

/*-------------------------------------------------------------------------*/

#ifdef WIN32
  #pragma pack(pop)
#endif

/*-------------------------------------------------------------------------*/
#endif
