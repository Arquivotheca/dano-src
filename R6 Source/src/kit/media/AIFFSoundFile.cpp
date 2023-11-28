/* ++++++++++

   FILE:  AIFFSoundFile.h
   REVS:  $Revision: 1.13 $
   NAME:  Arthur Decco
   DATE:  Mon Jun 05 18:55:07 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <AIFFSoundFile.h>
#include <byteorder.h>
#include <stdio.h>
#include <math.h>

status_t _read_common(BSoundFile *);
status_t _read_version(BSoundFile *);
status_t _read_data(BSoundFile *);
int32 _reckon_rate(int16 e, uint32 m0, uint32 m1);

bool _is_aiff(BSoundFile *file)
{
	_aiff_format_chunk form;

	file->fSoundFile->Seek(0, SEEK_SET);	
	if (file->fSoundFile->Read(&form, sizeof(form)) != sizeof(form))
	  return FALSE;

	if (B_BENDIAN_TO_HOST_INT32(form.magic) == _AIFF_MAGIC)
	  switch (B_BENDIAN_TO_HOST_INT32(form.file_type)) {
	  case _AIFF_TYPE:
		file->SetFileFormat(B_AIFF_FILE);
		file->SetIsCompressed(FALSE);
		return TRUE;

		// No compression support yet
		//	  case _AIFC_TYPE:
		//		file->SetFileFormat(B_AIFF_FILE);
		//		file->SetIsCompressed(TRUE);
		//		return TRUE;
	  }

	return FALSE;
}

status_t _read_aiff_header(BSoundFile *file)
{
	_aiff_chunk_header chunk_head;

	file->SetByteOrder(B_BIG_ENDIAN);
	file->SetSampleFormat(B_LINEAR_SAMPLES);

	while (file->fSoundFile->Read(&chunk_head, sizeof(chunk_head)) ==
		   sizeof(chunk_head)) {
		
		switch (B_BENDIAN_TO_HOST_INT32(chunk_head.magic)) {

		case _AIFF_COMMON:
			if (_read_common(file) < B_NO_ERROR)
			  return B_ERROR;
			break;
			
		case _AIFC_VERSION:
			if (_read_version(file) < B_NO_ERROR)
			  return B_ERROR;
			break;
			
		case _AIFF_DATA:
			if (_read_data(file) < B_NO_ERROR)
			  return B_ERROR;
			/* we aren't at the end of the data chunk--all
			 * we did was read the data header. So we drop through.
			 */

		default:
			int32 skip = B_BENDIAN_TO_HOST_INT32(chunk_head.zappo);
			if (file->fSoundFile->Seek(skip, SEEK_CUR) < B_NO_ERROR)
			  return B_ERROR;
			break;
		}
	}
	
	file->SeekToFrame(0);
	return B_NO_ERROR;
}  

status_t  _read_common(BSoundFile *file)
{
	_aiff_common_chunk com;

	if (file->fSoundFile->Read(&com.channel_count, 2) != 2)
	  return B_ERROR;
	if (file->fSoundFile->Read(&com.frame_count, 4) != 4)
	  return B_ERROR;
	if (file->fSoundFile->Read(&com.sample_size, 2) != 2)
	  return B_ERROR;
	if (file->fSoundFile->Read(&com.srate_exponent, 2) != 2)
	  return B_ERROR;
	if (file->fSoundFile->Read(&com.srate_mantissa_0, 4) != 4)
	  return B_ERROR;
	if (file->fSoundFile->Read(&com.srate_mantissa_1, 4) != 4)
	  return B_ERROR;

	file->SetSampleSize(B_BENDIAN_TO_HOST_INT16(com.sample_size) / 8);
	file->SetChannelCount(B_BENDIAN_TO_HOST_INT16(com.channel_count));
	file->SetFrameCount(B_BENDIAN_TO_HOST_INT32(com.frame_count));
	int32 rate = _reckon_rate(B_BENDIAN_TO_HOST_INT16(com.srate_exponent),
							  B_BENDIAN_TO_HOST_INT32(com.srate_mantissa_0),
							  B_BENDIAN_TO_HOST_INT32(com.srate_mantissa_1));
	file->SetSamplingRate(rate);

	return B_NO_ERROR;
}

#define UL31 ((uint32) 0x80000000L)
#define D31	((double) 2147483648.0)

static double ul2d(uint32 ul)
{
  double val;

  if (ul & UL31) 
	val = D31 + (ul & ~UL31);
  else
	val = ul;
  return val;
}

int32 _reckon_rate(int16 e, uint32 m0, uint32 m1)
{
  double srate;
  bool is_neg = (e < 0);
  double dm0, dm1;

  if (is_neg) e &= 0x7FFF;

  dm0 = ul2d(m0);
  dm1 = ul2d(m1);

  if (!is_neg && m0 == 0 && m1 == 0 && e == 0)
    return 0;

  srate = dm0 * pow(2.0, -31.0);
  srate += dm1 * pow(2.0, -63.0);
  srate *= pow(2.0, (double) (e - 16383));
  srate *= is_neg ? -1.0 : 1.0;
  return ((int32) floor(srate));
}

	
status_t _read_data(BSoundFile *file)
{
	_aiff_data_chunk data;
	
	int32 whence = file->fSoundFile->Seek(0,SEEK_CUR);
	if (file->fSoundFile->Read(&data, sizeof(data)) != sizeof(data))
	  return B_ERROR;

	int32 where = file->fSoundFile->Seek(0,SEEK_CUR);
	if (where < 0)
	  return B_ERROR;

	file->SetDataLocation(where);

	/* Rewind to the top of the chunk so we can zappo to the
	 * end of the data after we pop out.
	 */
	file->fSoundFile->Seek(whence, SEEK_SET);
	return B_NO_ERROR;
}

status_t _read_version(BSoundFile */*file*/)
{
	return B_NO_ERROR;
}
