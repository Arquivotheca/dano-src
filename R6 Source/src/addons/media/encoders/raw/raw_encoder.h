#ifndef _RAW_ENCODER_H
#define _RAW_ENCODER_H

#include <MediaFormats.h>
#include "Encoder.h"

using namespace BPrivate;

class RawEncoder : public Encoder {

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



#endif
