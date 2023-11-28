#ifndef _RGBCODEC_H_
#define _RGBCODEC_H_

#include "Decoder.h"

using namespace BPrivate;

#include "convert.h"

class RGBCodecDecoder : public Decoder
{
public:
	RGBCodecDecoder();
	~RGBCodecDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format,
	                  const void *in_info, size_t in_size);
	status_t	Format(media_format *inout_format);
	int32		CountFrames();
	status_t	Decode(void *out_buffer, int64 *frame_count, media_header *mh,
	                   media_decode_info *info);
	
private:
	media_raw_video_format	output_format;
	int   bitmap_depth;      /* 8, 16, 24, 32, etc */
	bool  bigendian;
	bool  upsidedown;
	int src_bytes_per_row;
	convertf *convert;
};


#endif /* _RGBCODEC_H_ */
