/* ++++++++++

   FILE:  UnixSoundFile.h
   REVS:  $Revision: 1.13 $
   NAME:  r
   DATE:  Mon Jun 05 18:55:07 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <UnixSoundFile.h>
#include <byteorder.h>
#include <stdio.h>

bool _is_unix(BSoundFile *file)
{
  _unix_sound_header h;

  file->fSoundFile->Seek(0, SEEK_SET);		/* rewind to start of file */
  if (file->fSoundFile->Read(&h, sizeof(h)) != sizeof(h))
	return FALSE;

  if (B_BENDIAN_TO_HOST_INT32(h.magic) == _UNIX_MAGIC) {
	  file->SetFileFormat(B_UNIX_FILE);
	  return TRUE;
  }
  return FALSE;
}

status_t _read_unix_header(BSoundFile *file)
{
	_unix_sound_header h;

	file->fSoundFile->Seek(0,SEEK_SET);
	if (file->fSoundFile->Read(&h, sizeof(h)) != sizeof(h))
	  return B_ERROR;

	file->SetSamplingRate(B_BENDIAN_TO_HOST_INT32(h.samplingRate));
	file->SetChannelCount(B_BENDIAN_TO_HOST_INT32(h.channelCount));
	file->SetByteOrder(B_BIG_ENDIAN);

	switch (B_BENDIAN_TO_HOST_INT32(h.dataFormat)) {
	case SND_FORMAT_MULAW_8:
		file->SetSampleSize(sizeof(char));
		file->SetSampleFormat(B_MULAW_SAMPLES);
		break;
	case SND_FORMAT_LINEAR_8:
		file->SetSampleSize(sizeof(char));
		file->SetSampleFormat(B_LINEAR_SAMPLES);
		break;
	case SND_FORMAT_LINEAR_16:	
		file->SetSampleSize(sizeof(short));
		file->SetSampleFormat(B_LINEAR_SAMPLES);
		break;
	case SND_FORMAT_FLOAT:
		file->SetSampleSize(sizeof(float));
		file->SetSampleFormat(B_FLOAT_SAMPLES);
		break;
	case SND_FORMAT_DOUBLE:
		file->SetSampleSize(sizeof(double));
		file->SetSampleFormat(B_FLOAT_SAMPLES);
		break;
	case SND_FORMAT_UNSPECIFIED:
	case SND_FORMAT_LINEAR_24:
	case SND_FORMAT_LINEAR_32:
	case SND_FORMAT_INDIRECT:
	case SND_FORMAT_NESTED:
	case SND_FORMAT_DSP_CORE:
	case SND_FORMAT_DSP_DATA_8:
	case SND_FORMAT_DSP_DATA_16:
	case SND_FORMAT_DSP_DATA_24:
	case SND_FORMAT_DSP_DATA_32:
	case SND_FORMAT_DISPLAY:
	case SND_FORMAT_MULAW_SQUELCH:
	case SND_FORMAT_EMPHASIZED:
	case SND_FORMAT_COMPRESSED:
	case SND_FORMAT_COMPRESSED_EMPHASIZED:
	case SND_FORMAT_DSP_COMMANDS:
	case SND_FORMAT_DSP_COMMANDS_SAMPLES:
	case SND_FORMAT_ADPCM_G721:
	case SND_FORMAT_ADPCM_G722:
	case SND_FORMAT_ADPCM_G723_3:
	case SND_FORMAT_ADPCM_G723_5:
	case SND_FORMAT_ALAW_8:
	case SND_FORMAT_AES:
	case SND_FORMAT_DELTA_MULAW_8:
	default:
		file->SetSampleSize(1);
		file->SetSampleFormat(B_UNDEFINED_SAMPLES);
		break;
	}
	file->SetDataLocation(B_BENDIAN_TO_HOST_INT32(h.dataLocation));
	file->SetFrameCount(B_BENDIAN_TO_HOST_INT32(h.dataSize) / file->FrameSize());

	/* start with frame 0 */
	return (file->SeekToFrame(0));
}

status_t _write_unix_header(BSoundFile *file)
{
	_unix_sound_header h;

	h.magic = B_HOST_TO_BENDIAN_INT32(_UNIX_MAGIC);
	h.info[0] = 0;

	h.samplingRate = B_HOST_TO_BENDIAN_INT32(file->SamplingRate());
	h.channelCount = B_HOST_TO_BENDIAN_INT32(file->CountChannels());

	int32 size = file->SampleSize();
	int32 format = SND_FORMAT_UNSPECIFIED;

	switch (file->SampleFormat()) {
	case B_MULAW_SAMPLES:
	  if (size == 1)
		format = SND_FORMAT_MULAW_8;
	  break;
	case B_LINEAR_SAMPLES:
	  if (size == 1)
		format = SND_FORMAT_LINEAR_8;
	  else if (size == 2)
		format = SND_FORMAT_LINEAR_16;
	  else if (size == 3)
		format = SND_FORMAT_LINEAR_24;
	  else if (size == 4)
		format = SND_FORMAT_LINEAR_32;
	  break;
	case B_FLOAT_SAMPLES:
	  if (size == sizeof(float))
		format = SND_FORMAT_FLOAT;
	  else if (size == sizeof(double))
		format = SND_FORMAT_DOUBLE;
	  break;
	}

	h.dataFormat = B_HOST_TO_BENDIAN_INT32(format);
	h.dataLocation = B_HOST_TO_BENDIAN_INT32(sizeof(h));
	h.dataSize = B_HOST_TO_BENDIAN_INT32(file->CountFrames() * file->FrameSize());

	file->fSoundFile->Seek(0,SEEK_SET);
	status_t result = file->fSoundFile->Write(&h, sizeof(h));
	if (result > 0)
	  return (result == sizeof(h) ? B_NO_ERROR : B_ERROR);
	return result;
}


