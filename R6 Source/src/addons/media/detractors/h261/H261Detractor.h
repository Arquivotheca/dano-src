#include "vic28.p64.h"

#include <Detractor.h>

class H261Detractor: public Detractor
{
	public:
		H261Detractor();
		virtual ~H261Detractor();

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
		//current frame
		//  -1 : in constructor
		//   0 : source has been set (in call to SetTo)
		int64 fCurrentFrame;
		
		
		BDataIO *fSource;
		
		//initialized in constructor (never change)
		int32 xRes;
		int32 yRes;
		float fps;
		
		/*file specific (START)*/
		unsigned char buff[16*1024];
		int buff_content;
		int start_bit;
		int file_decode_packet();
		/*file specific (END)*/
		
		FullP64Decoder p64_decoder;
		

};

