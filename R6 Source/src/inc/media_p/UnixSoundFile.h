/* ++++++++++

   FILE:  UnixSoundFile.h
   REVS:  $Revision: 1.5 $
   NAME:  r
   DATE:  Mon Jun 05 18:55:07 PDT 1995

   Copyright (c) 1995-1997 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _UNIX_SOUND_FILE_H
#define _UNIX_SOUND_FILE_H

#include <SoundFile.h>

bool _is_unix(BSoundFile *file);
status_t _read_unix_header(BSoundFile *f);
status_t _write_unix_header(BSoundFile *f);

typedef struct {
  int32	magic;					/* magic # */
  int32	dataLocation;			/* offset (file) or pointer (mem) */
  int32	dataSize;				/* raw data size (not incl header) */
  int32	dataFormat;				/* format code */
  int32	samplingRate;			/* frames per second */
  int32	channelCount;			/* samples per frame */
  char 	info[4];				/* null-terminated string of ind. length */
} _unix_sound_header;

#define _UNIX_MAGIC		'.snd'

#define SND_FORMAT_UNSPECIFIED          (0)
#define SND_FORMAT_MULAW_8              (1)
#define SND_FORMAT_LINEAR_8             (2)
#define SND_FORMAT_LINEAR_16            (3)
#define SND_FORMAT_LINEAR_24            (4)
#define SND_FORMAT_LINEAR_32            (5)
#define SND_FORMAT_FLOAT                (6)
#define SND_FORMAT_DOUBLE               (7)
#define SND_FORMAT_INDIRECT             (8)
#define SND_FORMAT_NESTED               (9)
#define SND_FORMAT_DSP_CORE             (10)
#define SND_FORMAT_DSP_DATA_8           (11)
#define SND_FORMAT_DSP_DATA_16          (12)
#define SND_FORMAT_DSP_DATA_24          (13)
#define SND_FORMAT_DSP_DATA_32          (14)
#define SND_FORMAT_DISPLAY              (16)
#define SND_FORMAT_MULAW_SQUELCH        (17)
#define SND_FORMAT_EMPHASIZED           (18)
#define SND_FORMAT_COMPRESSED           (19)
#define SND_FORMAT_COMPRESSED_EMPHASIZED (20)
#define SND_FORMAT_DSP_COMMANDS         (21)
#define SND_FORMAT_DSP_COMMANDS_SAMPLES (22)
#define SND_FORMAT_ADPCM_G721           (23)
#define SND_FORMAT_ADPCM_G722           (24)
#define SND_FORMAT_ADPCM_G723_3         (25)
#define SND_FORMAT_ADPCM_G723_5         (26)
#define SND_FORMAT_ALAW_8               (27)
#define SND_FORMAT_AES                  (28)
#define SND_FORMAT_DELTA_MULAW_8		(29)

#endif			// #ifdef _UNIX_SOUND_FILE_H
