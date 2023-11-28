
#ifndef _DETRACTOR_H
#define _DETRACTOR_H

#include <SupportDefs.h>

class media_file_format;
class media_codec_info;
class media_format;
class media_header;
struct entry_ref;
class BDataIO;

class Detractor
{
	public:
		Detractor();
		virtual ~Detractor();

	// this is the "mediafile-part", needed to support the
	// (read only) BMediaFile calls.
		virtual status_t SetTo(const entry_ref *ref);
		virtual status_t SetTo(BDataIO *source);

		virtual status_t InitCheck() const;

		virtual status_t GetFileFormatInfo(media_file_format *mfi) const;
		virtual const char* Copyright(void) const;
		virtual int32 CountTracks() const;

	// this is the "mediatrack-part"
		virtual status_t GetCodecInfo(int32 tracknum, media_codec_info *mci) const;
		virtual status_t EncodedFormat(int32 tracknum, media_format *out_format) const;
		virtual status_t DecodedFormat(int32 tracknum, media_format *inout_format);
		virtual int64    CountFrames(int32 tracknum) const;
		virtual bigtime_t Duration(int32 tracknum) const;
		virtual int64    CurrentFrame(int32 tracknum) const;
		virtual bigtime_t CurrentTime(int32 tracknum) const;
		virtual status_t ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount,
							   media_header *mh = NULL);
		virtual status_t SeekTo(int32 tracknum, int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 flags=0);
		virtual status_t FindKeyFrameForFrame(int32 tracknum, int64 *inout_frame, int32 flags=0) const;

		virtual	status_t ControlFile(int32 selector, void * io_data, size_t size);
		virtual	status_t ControlCodec(int32 track, int32 selector, void * io_data, size_t size);
};

#endif

