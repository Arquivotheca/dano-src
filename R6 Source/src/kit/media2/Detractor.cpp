
#include "Detractor.h"

#include <stdio.h>

namespace B {
namespace Private {

using namespace Media2;

Detractor::Detractor()
{
//	printf("Detractor ctor\n");
}

Detractor::~Detractor()
{
//	printf("Detractor dtor\n");
}

status_t Detractor::SetSource(const entry_ref *)
{
	printf("Detractor::SetSource(entry_ref)\n");

	return B_OK;
}

status_t Detractor::SetSource(IByteInput::arg, IByteSeekable::arg)
{
	printf("Detractor::SetSource(IByteInput, IByteSeekable)\n");

	return B_OK;
}

status_t Detractor::InitCheck() const
{
	printf("Detractor::InitCheck\n");

	return B_OK;
}

status_t Detractor::GetFileFormatInfo(media_file_format *) const
{
	printf("Detractor::GetFileFormatInfo\n");

	return B_OK;
}

const char* Detractor::Copyright() const
{
	printf("Detractor::Copyright\n");

	return "";
}

int32 Detractor::CountTracks() const
{
	printf("Detractor::CountTracks\n");

	return 2;
}

status_t Detractor::GetCodecInfo(int32, media_codec_info *) const
{
	printf("Detractor::GetCodecInfo\n");

	return B_OK;
}

status_t Detractor::EncodedFormat(int32, media_format *) const
{
	printf("Detractor::EncodedFormat\n");

	return B_OK;
}

status_t Detractor::DecodedFormat(int32, media_format *)
{
	printf("Detractor::DecodedFormat\n");

	return B_OK;
}

int64    Detractor::CountFrames(int32) const
{
	printf("Detractor::CountFrames\n");

	return 10000;
}

bigtime_t Detractor::Duration(int32) const
{
	printf("Detractor::Duration\n");

	return 10000LL;
}

int64    Detractor::CurrentFrame(int32) const
{
	printf("Detractor::CurrentFrame\n");

	return 0;
}

bigtime_t Detractor::CurrentTime(int32) const
{
	printf("Detractor::CurrentTime\n");
	return 0;
}

status_t Detractor::ReadFrames(int32, void *, int64 *, media_header *)
{
	printf("Detractor::ReadFrames\n");

	return B_OK;
}
							   
status_t Detractor::SeekTo(int32, int32, bigtime_t *, int64 *, int32)
{
	printf("Detractor::SeekToFrame\n");
	return B_OK;
}

status_t Detractor::FindKeyFrameForFrame(int32, int64 *, int32) const
{
	printf("Detractor::FindKeyFrameForFrame\n");
	return B_OK;
}

status_t Detractor::ControlFile(int32, void *, size_t)
{
	return EBADF;
}

status_t Detractor::ControlCodec(int32, int32, void *, size_t)
{
	return EBADF;
}

} } // B::Private
