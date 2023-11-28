//==============================================================================
#ifndef __INCLUDE_SOURCE_H261_ENCODER_H
#define __INCLUDE_SOURCE_H261_ENCODER_H
//==============================================================================
#include <MediaFormats.h>
#include "Encoder.h"
#include "control.encoding.h"
//==============================================================================
using namespace BPrivate;
//==============================================================================
class H261Encoder : public Encoder
{
public:
				H261Encoder();
				~H261Encoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;

	status_t	SetFormat(media_file_format *mfi,
						  media_format *in_format,
						  media_format *out_format);

	status_t	Encode(const void *in_buffer, int64 num_frames,
				       media_encode_info *info);

private:
	int width;
	int height;
	int format;
	int bpr;		//bytes per row

	encoding_control the_encoding_control;	
};
//==============================================================================
#endif
//==============================================================================
