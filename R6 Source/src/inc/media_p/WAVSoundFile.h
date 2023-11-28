/* ++++++++++

   FILE:  WAVSoundFile.h
   REVS:  $Revision: 1.5 $
   NAME:  r
   DATE:  Mon Jun 05 18:55:07 PDT 1995

   Copyright (c) 1995-1997 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _WAV_SOUND_FILE_H
#define _WAV_SOUND_FILE_H

#include <SoundFile.h>

bool _is_wav(BSoundFile *file);
status_t _read_wav_header(BSoundFile *file);
status_t _write_wav_header(BSoundFile *file);

#define _RIFF_MAGIC	'RIFF'
#define _WAVE_MAGIC	'WAVE'
#define _RIFF_FMT	'fmt '
#define _RIFF_DATA	'data'

#define WAVE_FORMAT_PCM 1

typedef struct {
	int32 riff_magic;
	int32 chunk_size;
	int32 wav_magic;
} _wav_sound_header;

typedef struct {
	int32 magic;
	int32 size;
} _riff_chunk;

typedef struct {
	int16 format;
	int16 channel_count;
	int32 sampling_rate;
	int32 average_rate; /* unused */
	int16 alignment;
	int16 sample_size; /* in bits */
} _wav_format_chunk;


#endif			// #ifdef _WAV_SOUND_FILE_H
