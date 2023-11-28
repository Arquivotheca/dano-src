
#include <Detractor.h>

class DummyDetractor: public Detractor
{
	public:
		DummyDetractor();
		virtual ~DummyDetractor();

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
		virtual status_t SeekToFrame(int32 tracknum, int64 *inout_frame, int32 flags=0);
		virtual status_t FindKeyFrameForFrame(int32 tracknum, int64 *inout_frame, int32 flags=0) const;

	private:
		int64 fCurrentFrame;
};

