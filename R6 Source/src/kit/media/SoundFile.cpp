/* ++++++++++

   FILE:  SoundFile.cpp
   REVS:  $Revision: 1.33 $
   NAME:  r
   DATE:  Mon Jun 05 20:30:34 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <SoundFile.h>
#include <R3MediaDefs.h>
#include <AIFFSoundFile.h>
#include <UnixSoundFile.h>
#include <WAVSoundFile.h>

#include <stdio.h>
#include <byteorder.h>


/* ================
   Implementation of BSoundFile
   ================ */


BSoundFile::BSoundFile()

{
  fSoundFile = NULL;
  fCStatus = B_NO_INIT;
  _init_raw_stats();
}

BSoundFile::BSoundFile(const entry_ref* ref, uint32 open_mode)
{
  fSoundFile = NULL;
  fCStatus = B_NO_INIT;
  _init_raw_stats();
  fCStatus = SetTo(ref, open_mode);
}


BSoundFile::~BSoundFile()
{
  switch (fFileFormat) {
  case B_UNIX_FILE:
	_write_unix_header(this);
	break;
  case B_AIFF_FILE:
	//	_write_aiff_header(this);
	break;
  case B_WAVE_FILE:
	_write_wav_header(this);
	break;
  }
  delete fSoundFile;
//  if (fCompressionName)
//	free(fCompressionName);
}

status_t BSoundFile::InitCheck() const
{
  return fCStatus;
}

status_t BSoundFile::SetTo(const entry_ref *entry, uint32 open_mode)
{
  uint32 access_mode = open_mode & O_ACCMODE;

  /* only allow O_RDONLY and O_WRONLY */
  if (access_mode != O_RDONLY && access_mode != O_WRONLY)
	return B_BAD_VALUE;

  /* Writing AIFF not yet supported */
  if (access_mode == O_WRONLY && fFileFormat == B_AIFF_FILE)
	return B_BAD_VALUE;

  if (!fSoundFile)
	fSoundFile = new BFile();

  status_t err = fSoundFile->SetTo(entry, open_mode);
  if (err < B_OK)
	return err;

  if (access_mode == O_RDONLY)
	if (_is_unix(this)) 
	  err = _read_unix_header(this);
	else if (_is_aiff(this))  
	  err = _read_aiff_header(this);
	else if (_is_wav(this)) 
	  err = _read_wav_header(this);
	else {
	  _init_raw_stats();
	  struct stat st;
	  err = fSoundFile->GetStat(&st);
	  if (err != B_NO_ERROR)
		return err;
	  fFrameCount = st.st_size / FrameSize();
	}
  else switch (fFileFormat) {
  case B_UNIX_FILE:
	err = _write_unix_header(this);
	break;
  case B_AIFF_FILE:
	//	err = _write_aiff_header(this);
	break;
  case B_WAVE_FILE:
	err = _write_wav_header(this);
	break;
  }

  return err;
}

void BSoundFile::_init_raw_stats()
{
  fFileFormat = B_UNKNOWN_FILE;
  fSamplingRate = 44100;
  fChannelCount = 2;			/* stereo */
  fSampleSize = 2;				/* 16 bit */
  fByteOrder = B_HOST_IS_BENDIAN ? B_BIG_ENDIAN : B_LITTLE_ENDIAN;
  fSampleFormat = B_LINEAR_SAMPLES;
  fByteOffset = 0;
  fFrameCount = 0;
  fFrameIndex = 0;
  fIsCompressed = FALSE;
  fCompressionType = -1;
  fCompressionName = NULL;
}

int32	BSoundFile::SetCompressionType(int32 /*type*/)
{
	return 0;
}

char *BSoundFile::SetCompressionName(char */*name*/)
{
	return NULL;
}

bool BSoundFile::SetIsCompressed(bool /*tf*/) 
{
	return FALSE;
}

int32 BSoundFile::SetSamplingRate(int32 fps)
{
	fSamplingRate = fps;
	return (fSamplingRate);
}
	
int32 BSoundFile::SetFileFormat(int32 format) 
{
	fFileFormat = format;
	return (fFileFormat);
}

off_t BSoundFile::SetFrameCount(off_t count)
{
	fFrameCount = count;
	return (fFrameCount);
}

int32 BSoundFile::SetChannelCount(int32 spf)
{
	fChannelCount = spf;
	return (fChannelCount);
}

int32 BSoundFile::SetSampleSize(int32 bps)
{
	fSampleSize = bps;
	return (fSampleSize);
}

int32 BSoundFile::SetByteOrder(int32 bord)
{
	fByteOrder = bord;
	return (fByteOrder);
}

int32 BSoundFile::SetSampleFormat(int32 fmt)
{
	fSampleFormat = fmt;
	return (fSampleFormat);
}

off_t BSoundFile::SetDataLocation(off_t offset)
{
	fByteOffset = offset;
	return (fByteOffset);
}

/* The default implementation uses FrameSize() and fByteOffset
 * to form an offset into the sound file.
 */
off_t BSoundFile::SeekToFrame(off_t index)
{
	size_t bytePos;
	off_t result;

	if (fFrameCount < index)
	  index = fFrameCount;
	bytePos = (index * FrameSize()) + fByteOffset;
	result = fSoundFile->Seek(bytePos, SEEK_SET);

	if (result < 0) {
		return result;
	}

	fFrameIndex = index;
	return fFrameIndex;
}

off_t BSoundFile::FramesRemaining() const
{
	if ((fFrameCount >= 0) && (fFrameIndex >= 0)) 
		return (fFrameCount - fFrameIndex);
	return -1;
}

size_t BSoundFile::ReadFrames(char *Buffer, size_t count)
{
	size_t  bytesToRead;
	ssize_t bytesRead, framesRead;

	if (fFrameCount - fFrameIndex < count)
	  count = fFrameCount - fFrameIndex;
	bytesToRead = count * FrameSize();
	bytesRead = fSoundFile->Read(Buffer, bytesToRead);

	if (bytesRead < 0) 
	  return bytesRead;

	/* one-byte wave is naturally unsigned, the bastards */
	/* must now do the conversion yourself
	if ((fSampleSize == 1) && (fFileFormat == B_WAVE_FILE)) {
		bytesToRead = bytesRead;
		shiftPtr = Buffer;
		while (bytesToRead-- > 0)
		  *shiftPtr++ -= 128;
	}
	*/
	
	framesRead = bytesRead / FrameSize();
	fFrameIndex += framesRead;

	return framesRead;
}

size_t BSoundFile::WriteFrames(char *Buffer, size_t count)
{
	size_t  bytesToWrite;
	ssize_t bytesWritten, framesWritten;
	bytesToWrite = count * FrameSize();

	bytesWritten = fSoundFile->Write(Buffer, bytesToWrite);

	if (bytesWritten < 0) 
	  return bytesWritten;

	framesWritten = bytesWritten / FrameSize();
	fFrameIndex += framesWritten;
	if (fFrameIndex > fFrameCount)
	  fFrameCount = fFrameIndex;

	return framesWritten;
}

 bool BSoundFile::IsCompressed() const
{	return(fIsCompressed);	}

 int32 BSoundFile::CompressionType() const
{	return(fCompressionType);	}

 char *BSoundFile::CompressionName() const
{	return(fCompressionName);	}

 int32 BSoundFile::FileFormat() const
{	return(fFileFormat);	}

 int32 BSoundFile::SamplingRate() const
{	return(fSamplingRate);	}

 int32 BSoundFile::CountChannels() const
{	return(fChannelCount);	}

 int32 BSoundFile::SampleSize() const
{	return(fSampleSize);	}

 int32 BSoundFile::ByteOrder() const
{	return(fByteOrder);	}

 int32 BSoundFile::SampleFormat() const
{	return(fSampleFormat);	}

 int32 BSoundFile::FrameSize() const
{ 	return(fSampleSize * fChannelCount);	}

 off_t BSoundFile::CountFrames() const
{	return(fFrameCount);	}

 off_t BSoundFile::FrameIndex() const
{	return(fFrameIndex);	}

void BSoundFile::_ReservedSoundFile1() {}
void BSoundFile::_ReservedSoundFile2() {}
void BSoundFile::_ReservedSoundFile3() {}
