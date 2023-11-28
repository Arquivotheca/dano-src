#ifndef INDEO5RTENCODER_H
#define INDEO5RTENCODER_H

#include <SupportDefs.h>
#include <Encoder.h>

using namespace BPrivate;

/* indeo headers */
#undef MAX
#undef MIN
extern "C" {
	#include "datatype.h"
}
#include "rtinclude/const.h"
#include "rtenc.h"

bool check_mmx_match();	/* platform matches this version */
typedef void (convertf)(const uint8 *InputPixels,
                        uint8 *Yin, uint8 *U, uint8 *V, int width, int height);

class Indeo5RTEncoder : public Encoder
{
	public:
		Indeo5RTEncoder();
		~Indeo5RTEncoder();

		status_t	GetCodecInfo(media_codec_info *mci) const;

		status_t	SetFormat(media_file_format *mfi,
		                      media_format *in_format,
		                      media_format *out_format);
	
		status_t	StartEncoder();

		status_t	Encode(const void *in_buffer, int64 num_frames,
		                   media_encode_info *info);
		
		status_t	GetEncodeParameters(encode_parameters *parameters) const;
		status_t	SetEncodeParameters(encode_parameters *parameters);
		// frame rate
		
	private:
		status_t InitFormat(media_file_format *mfi,
		                    media_format *in_format,
		                    media_format *out_format);
	
		pRTCntx pRTEncCntx;
		int16 keyrate;
		int32 TargetBytes;
		bool ScalabilityOn;
		int16 framerate;
		int16 quality; // 100-0

		bool encoder_ready;
		bool last_write_failed;
		convertf *convert;
		bool is32;
		
		int width, height;
		uint8 *Y;
		uint8 *U;
		uint8 *V;
		
		bigtime_t convert_time;
		bigtime_t encode_time;

		char *out_buffer;
		size_t out_buffer_size;
		size_t max_used_size;
};

#endif
