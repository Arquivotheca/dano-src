#ifndef _YUVCODEC_H_
#define _YUVCODEC_H_

#include "Decoder.h"

using namespace BPrivate;

#include "convert.h"

class YUVCodecDecoder : public Decoder
{
public:
	YUVCodecDecoder();
	~YUVCodecDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format,
	                  const void *in_info, size_t in_size);
	status_t	Format(media_format *inout_format);
	int32		CountFrames();
	status_t	Decode(void *out_buffer, int64 *frame_count, media_header *mh,
	                   media_decode_info *info);
	
private:
	media_raw_video_format	output_format;
	uint32 fourcc;
	int src_bytes_per_row;
	convertf *convert;
};


#endif /* _YUVCODEC_H_ */
