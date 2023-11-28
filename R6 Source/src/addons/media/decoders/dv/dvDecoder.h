#ifndef _DV_H_
#define _DV_H_

#include <MediaFile.h>
#include <MediaTrack.h>
#include "Extractor.h"
#include "Decoder.h"

using namespace BPrivate;

#include "dvcodec.h"

Decoder *instantiate_DV(void);
void register_DV(void);

class DVDecoder : public Decoder
{
public:
	DVDecoder();
	~DVDecoder();

	status_t	GetCodecInfo(media_codec_info *mfi) const;
	status_t	Sniff(const media_format *in_format, const void *in_info,
				      size_t in_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *inout_frameCount,
				       media_header *mh, media_decode_info *info);

private:
	media_raw_video_format	output_format;
	DVCodec		*fDVCodec;
};


#endif /* _DV_H_ */
