#ifndef _RAW_ENCODER_H
#define _RAW_ENCODER_H

#include <media2/MediaFormats.h>
#include "Encoder.h"

namespace B {
namespace Media2 {

class RawEncoder : public B::Private::Encoder {

public:
				RawEncoder(bool audio);
				~RawEncoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;

	status_t	SetFormat(media_file_format *mfi,
						  media_format *in_format,
						  media_format *out_format);

	status_t	Encode(const void *in_buffer, int64 num_frames,
				       media_encode_info *info);

private:
	int frame_size;
	bool audio;
};


} } // B::Media2
#endif
