/* ++++++++++

   FILE:  WAVSoundFile.h
   REVS:  $Revision: 1.12 $
   NAME:  r
   DATE:  Mon Jun 05 18:55:07 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <WAVSoundFile.h>
#include <byteorder.h>
#include <stdio.h>

status_t _read_fmt(BSoundFile *file);

bool _is_wav(BSoundFile *file)
{
	int32 bytesRead;
	_wav_sound_header h;

	file->fSoundFile->Seek(0,SEEK_SET);
	bytesRead = file->fSoundFile->Read(&h, sizeof(h));

	if (bytesRead != sizeof(h))
	  return FALSE;
	
	if (B_BENDIAN_TO_HOST_INT32(h.riff_magic) == _RIFF_MAGIC &&
		B_BENDIAN_TO_HOST_INT32(h.wav_magic) == _WAVE_MAGIC) {
		file->SetFileFormat(B_WAVE_FILE);
		return TRUE;
	}

	return FALSE;
}

status_t _read_wav_header(BSoundFile *file)
{
	_riff_chunk chunk_head;

	/* Wave spec: fmt comes before data--both are mandatory. */
	bool got_fmt = FALSE;
	bool got_data = FALSE;

	file->SetByteOrder(B_LITTLE_ENDIAN);
	file->SetSampleFormat(B_LINEAR_SAMPLES);

	while (file->fSoundFile->Read(&chunk_head, sizeof(chunk_head)) == 
		   sizeof(chunk_head)) {
		
		int32 chunk_size = B_LENDIAN_TO_HOST_INT32(chunk_head.size);
		int32 chunk_start = file->fSoundFile->Seek(0, SEEK_CUR);

		switch (B_BENDIAN_TO_HOST_INT32(chunk_head.magic)) {

		case _RIFF_FMT:
			if (_read_fmt(file) < B_NO_ERROR)
			  return B_ERROR;
			got_fmt = TRUE;
			break;
			
		case _RIFF_DATA:
			if (!got_fmt || !file->FrameSize())
			  return B_ERROR;
			file->SetFrameCount(chunk_size / file->FrameSize());
			file->SetDataLocation(chunk_start);
			got_data = TRUE;
			break;
		}
		if (got_data)
		  break;

		struct stat st;
		if (file->fSoundFile->GetStat(&st) < B_NO_ERROR)
		  return B_ERROR;
		if (chunk_start + chunk_size > st.st_size)
		  return B_ERROR;
		if (file->fSoundFile->Seek(chunk_start + chunk_size, SEEK_SET)
			< B_NO_ERROR)
		  return B_ERROR;
	}

	if (!got_fmt || !got_data)
	  return B_ERROR;
	else
	  return (file->SeekToFrame(0));
}  

status_t _read_fmt(BSoundFile *file)
{
	_wav_format_chunk wfc;

	if (file->fSoundFile->Read(&wfc, sizeof(wfc)) != sizeof(wfc))
	  return B_ERROR;

	file->SetSamplingRate(B_LENDIAN_TO_HOST_INT32(wfc.sampling_rate));
	file->SetChannelCount(B_LENDIAN_TO_HOST_INT16(wfc.channel_count));
	file->SetByteOrder(B_LITTLE_ENDIAN);

	if (B_LENDIAN_TO_HOST_INT16(wfc.format) == 1)
	  file->SetSampleFormat(B_LINEAR_SAMPLES);
	else
	  file->SetSampleFormat(B_UNDEFINED_SAMPLES);

	int16 s = B_LENDIAN_TO_HOST_INT16(wfc.sample_size) / 8;
	file->SetSampleSize(s);
	return B_NO_ERROR;
}

status_t _write_wav_header(BSoundFile *file)
{
	_wav_sound_header riff_head;
	_riff_chunk fmt_chunk_head;
	_wav_format_chunk fmt_chunk;
	_riff_chunk data_chunk_head;

	riff_head.riff_magic = B_HOST_TO_BENDIAN_INT32(_RIFF_MAGIC);
	riff_head.wav_magic = B_HOST_TO_BENDIAN_INT32(_WAVE_MAGIC);
	fmt_chunk_head.magic = B_HOST_TO_BENDIAN_INT32(_RIFF_FMT);
	fmt_chunk_head.size = B_HOST_TO_LENDIAN_INT32(sizeof(fmt_chunk));
	fmt_chunk.format = B_HOST_TO_LENDIAN_INT16(WAVE_FORMAT_PCM);
	fmt_chunk.channel_count = B_HOST_TO_LENDIAN_INT16(file->CountChannels());
	fmt_chunk.sampling_rate = B_HOST_TO_LENDIAN_INT32(file->SamplingRate());
	fmt_chunk.average_rate = B_HOST_TO_LENDIAN_INT32(file->SamplingRate()
													 * file->FrameSize());
	fmt_chunk.alignment = B_HOST_TO_LENDIAN_INT16(file->FrameSize());
	fmt_chunk.sample_size = B_HOST_TO_LENDIAN_INT16(8 * file->SampleSize());
	data_chunk_head.magic = B_HOST_TO_BENDIAN_INT32(_RIFF_DATA);
	int32 len = file->CountFrames() * file->FrameSize();
	data_chunk_head.size = B_HOST_TO_LENDIAN_INT32(len);
	riff_head.chunk_size = B_HOST_TO_LENDIAN_INT32(len
												   + sizeof(riff_head.wav_magic)
												   + 2 * sizeof(_riff_chunk)
												   + sizeof(fmt_chunk));
	file->fSoundFile->Seek(0,SEEK_SET);
	status_t result = file->fSoundFile->Write(&riff_head, sizeof(riff_head));
	if (result == sizeof(riff_head)) {
	  file->fSoundFile->Write(&fmt_chunk_head, sizeof(fmt_chunk_head));
	  file->fSoundFile->Write(&fmt_chunk, sizeof(fmt_chunk));
	  result = file->fSoundFile->Write(&data_chunk_head, sizeof(data_chunk_head));
	}
	if (result > 0)
	  return (result == sizeof(data_chunk_head) ? B_NO_ERROR : B_ERROR);
	return result;
}  

