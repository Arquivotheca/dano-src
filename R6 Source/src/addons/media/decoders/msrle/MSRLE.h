#ifndef _MSRLE_H_
#define _MSRLE_H_

#include "Decoder.h"

using namespace BPrivate;


class MSRLEDecoder : public Decoder
{
public:
	MSRLEDecoder();
	~MSRLEDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format,
	                  const void *in_info, size_t in_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *frame_count, media_header *mh,
	                   media_decode_info *info);

private:
	media_raw_video_format  fOutputFormat;
	uint32					*fClut;
};


#endif /* _MSRLE_H_ */
