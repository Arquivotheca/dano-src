#include <File.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Entry.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include "DummyDetractor.h"


//#define DEBUG printf
#define DEBUG if (0) printf
#define SEEK(x) //printf x
#define SEQ(x) //printf x


extern "C" const char * mime_type_detractor = "audio/x-raw";

extern "C" Detractor* instantiate_detractor()
{
	return new DummyDetractor;
}


DummyDetractor::DummyDetractor()
{
	fCurrentFrame = -1;
}

DummyDetractor::~DummyDetractor()
{
}


status_t DummyDetractor::SetTo(const entry_ref *ref)
{
	if(strcmp(ref->name,"detractor.dat")==0)
	{
		printf("DummyDetractor::SetTo(entry_ref) OK\n");
		fCurrentFrame = 0;
		return B_OK;
	}
	printf("DummyDetractor::SetTo(entry_ref) FAIL\n");
	return B_ERROR;
}

status_t DummyDetractor::SetTo(BDataIO *source)
{
	printf("DummyDetractor::SetTo(BDataIO)\n");
	fCurrentFrame = 0;
	return B_OK;
}

status_t DummyDetractor::InitCheck() const
{
	printf("DummyDetractor::InitCheck\n");
	return (fCurrentFrame>=0);
}

status_t DummyDetractor::GetFileFormatInfo(media_file_format *mfi) const
{
	printf("DummyDetractor::GetFileFormatInfo\n");
    strcpy(mfi->mime_type,      "audio/x-noise");
    strcpy(mfi->pretty_name,    "Noise");
    strcpy(mfi->short_name,     "dat");
    strcpy(mfi->file_extension, "dat");

    mfi->family = B_ANY_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        media_file_format::B_IMPERFECTLY_SEEKABLE  |
                        media_file_format::B_PERFECTLY_SEEKABLE    |
                        media_file_format::B_KNOWS_RAW_AUDIO       |
                        media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}

const char* DummyDetractor::Copyright(void) const
{
	printf("DummyDetractor::Copyright\n");
	return "Copyright 2000 Be, Inc.";
}

int32 DummyDetractor::CountTracks() const
{
	printf("DummyDetractor::CountTracks\n");
	return 1;
}

status_t DummyDetractor::GetCodecInfo(int32 tracknum, media_codec_info *mci) const
{
	printf("DummyDetractor::GetCodecInfo\n");
	strcpy(mci->pretty_name, "Raw Audio");
	strcpy(mci->short_name, "raw-audio");
	return B_OK;
}

status_t DummyDetractor::EncodedFormat(int32 tracknum, media_format *out_format) const
{
	printf("DummyDetractor::EncodedFormat\n");
	out_format->type=B_MEDIA_RAW_AUDIO;
	out_format->u.raw_audio.frame_rate = 44100;
	out_format->u.raw_audio.channel_count = 2;
	out_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	out_format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	out_format->u.raw_audio.buffer_size = 1024;
	return B_OK;
}

status_t DummyDetractor::DecodedFormat(int32 tracknum, media_format *inout_format)
{
	printf("DummyDetractor::DecodedFormat\n");
	inout_format->type=B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = 44100;
	inout_format->u.raw_audio.channel_count = 2;
	inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	inout_format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	inout_format->u.raw_audio.buffer_size = 1024;
	return B_OK;
}

int64    DummyDetractor::CountFrames(int32 tracknum) const
{
	printf("DummyDetractor::CountFrames\n");

	return 4410000;
}

bigtime_t DummyDetractor::Duration(int32 tracknum) const
{
	printf("DummyDetractor::Duration\n");

	return 100000000.0;
}

int64    DummyDetractor::CurrentFrame(int32 tracknum) const
{
	printf("DummyDetractor::CurrentFrame\n");

	return fCurrentFrame;
}

bigtime_t DummyDetractor::CurrentTime(int32 tracknum) const
{
	printf("DummyDetractor::CurrentTime\n");
	return fCurrentFrame*1000000/44100;
}

status_t DummyDetractor::ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount, media_header *mh = NULL)
{
	printf("DummyDetractor::ReadFrames\n");

	int16 *out=(int16*)out_buffer;
	for(int i=0;i<512;i++)
		*out++ = mrand48();

	*out_frameCount = 256;
	fCurrentFrame+=256;
	return B_OK;
}
							   
status_t DummyDetractor::SeekToFrame(int32 tracknum, int64 *inout_frame, int32 flags=0)
{
	printf("DummyDetractor::SeekToFrame\n");
	fCurrentFrame = *inout_frame;
	return B_OK;
}

status_t DummyDetractor::FindKeyFrameForFrame(int32 tracknum, int64 *inout_frame, int32 flags=0) const
{
	printf("DummyDetractor::FindKeyFrameForFrame\n");
	return B_OK;
}

