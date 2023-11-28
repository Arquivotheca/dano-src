/* ++++++++++

   FILE:  AIFFSoundFile.h
   REVS:  $Revision: 1.6 $
   NAME:  r
   DATE:  Mon Jun 05 18:55:07 PDT 1995

   Copyright (c) 1995-1997 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _AIFF_SOUND_FILE_H
#define _AIFF_SOUND_FILE_H

#include <SoundFile.h>

bool _is_aiff(BSoundFile *);
status_t _read_aiff_header(BSoundFile *);

#define _AIFF_MAGIC		'FORM'
#define _AIFF_TYPE		'AIFF'
#define _AIFC_TYPE		'AIFC'

#define _AIFC_VERSION	'FVER'
#define _AIFF_COMMON	'COMM'
#define _AIFF_DATA		'SSND'

typedef struct {
	int32 magic;
	int32 zappo;
} _aiff_chunk_header;

typedef struct {
	int32 magic;
	int32 data_size;
	int32 file_type;
} _aiff_format_chunk;

typedef struct {
	int32 timestamp;
} _aifc_version_chunk;

typedef struct {	
	int16 channel_count;
	int32 frame_count;
	int16 sample_size; /* bits */
	int16 srate_exponent;
	uint32 srate_mantissa_0;
	uint32 srate_mantissa_1;
} _aiff_common_chunk;
	
typedef struct {	
	int32 compression_type;
	char* compression_name;
} _aiff_compression_chunk;

typedef struct {
	int32 offset;
	int32 block_size;
} _aiff_data_chunk;


#endif			// #ifdef _AIFF_SOUND_FILE_H
