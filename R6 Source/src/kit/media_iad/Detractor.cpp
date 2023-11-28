

#include <Detractor.h>

#include <stdio.h>

Detractor::Detractor()
{
//	printf("Detractor ctor\n");
}

Detractor::~Detractor()
{
//	printf("Detractor dtor\n");
}

status_t Detractor::SetTo(const entry_ref *ref)
{
	printf("Detractor::SetTo(entry_ref)\n");

	return B_OK;
}

status_t Detractor::SetTo(BDataIO *source)
{
	printf("Detractor::SetTo(BDataIO)\n");

	return B_OK;
}

status_t Detractor::InitCheck() const
{
	printf("Detractor::InitCheck\n");

	return B_OK;
}

status_t Detractor::GetFileFormatInfo(media_file_format *mfi) const
{
	printf("Detractor::GetFileFormatInfo\n");

	return B_OK;
}

const char* Detractor::Copyright(void) const
{
	printf("Detractor::Copyright\n");

	return "";
}

int32 Detractor::CountTracks() const
{
	printf("Detractor::CountTracks\n");

	return 2;
}

status_t Detractor::GetCodecInfo(int32 tracknum, media_codec_info *mci) const
{
	printf("Detractor::GetCodecInfo\n");

	return B_OK;
}

status_t Detractor::EncodedFormat(int32 tracknum, media_format *out_format) const
{
	printf("Detractor::EncodedFormat\n");

	return B_OK;
}

status_t Detractor::DecodedFormat(int32 tracknum, media_format *inout_format)
{
	printf("Detractor::DecodedFormat\n");

	return B_OK;
}

int64    Detractor::CountFrames(int32 tracknum) const
{
	printf("Detractor::CountFrames\n");

	return 10000;
}

bigtime_t Detractor::Duration(int32 tracknum) const
{
	printf("Detractor::Duration\n");

	return 10000.0;
}

int64    Detractor::CurrentFrame(int32 tracknum) const
{
	printf("Detractor::CurrentFrame\n");

	return 0;
}

bigtime_t Detractor::CurrentTime(int32 tracknum) const
{
	printf("Detractor::CurrentTime\n");
	return 0;
}

status_t Detractor::ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount, media_header *mh = NULL)
{
	printf("Detractor::ReadFrames\n");

	return B_OK;
}
							   
status_t Detractor::SeekTo(int32 tracknum, int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 flags=0)
{
	printf("Detractor::SeekToFrame\n");
	return B_OK;
}

status_t Detractor::FindKeyFrameForFrame(int32 tracknum, int64 *inout_frame, int32 flags=0) const
{
	printf("Detractor::FindKeyFrameForFrame\n");
	return B_OK;
}

status_t Detractor::ControlFile(int32 selector, void * data, size_t size)
{
	return EBADF;
}

status_t Detractor::ControlCodec(int32 track, int32 selector, void * data, size_t size)
{
	return EBADF;
}

