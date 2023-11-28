#ifndef _MSVIDEO_H_
#define _MSVIDEO_H_

#include "Decoder.h"

using namespace BPrivate;


class MSVideoDecoder : public Decoder
{
public:
	MSVideoDecoder();
	~MSVideoDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format,
	                  const void *in_info, size_t in_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *inout_frameCount,
					   media_header *mh, media_decode_info *info);

private:
	media_raw_video_format  fOutputFormat;
	int                     bitmap_depth;
	uint32					*pal;
};


#endif /* _MSVIDEO_H_ */
