#ifndef INDEO5_H
#define INDEO5_H

#include <SupportDefs.h>
#include "Decoder.h"

using namespace BPrivate;

/* indeo headers */
#undef MAX
#undef MIN
extern "C" {
	#include "datatype.h"
	#include "pia_main.h"
	#include "pia_dec.h"
	#include "pia_cout.h"
}


class Indeo5Decoder : public Decoder
{
	public:
		Indeo5Decoder();
		~Indeo5Decoder();
	
		status_t	GetCodecInfo(media_codec_info *mci) const;
		status_t	Sniff(const media_format *in_format,
		                  const void *in_info, size_t in_size);
		status_t	Format(media_format *inout_format);
		status_t	Decode(void *out_buffer, int64 *frame_count,
						   media_header *mh, media_decode_info *info);

	private:
		status_t Init();
		status_t InitColor(media_raw_video_format *requested_format);

		class IndeoEnvironment {
			public:
				IndeoEnvironment();
				~IndeoEnvironment();
				ENVIRONMENT_INFO *GetEnvironment();
			private:
				bool decoder_ready;
				bool colorout_ready;
				ENVIRONMENT_INFO eienvironment;
		};
		
static	IndeoEnvironment indeoEnvironment;
	
		media_raw_video_format	output_format;

		bool decoder_inst_ready;
		bool colorout_inst_ready;
		bool palettevalid;

		DEC_INST		DecInst;		/* Decoder Instance Structure */
		DECODE_FRAME_INPUT_INFO DecInput;	/* Decoder Input Struct */
		DECODE_FRAME_OUTPUT_INFO DecOutput;	/* Decoder Output Struct */
		
		CCOUT_INST		CoutInst;		/* Decoder Instance Structure */
		COLOROUT_FRAME_INPUT_INFO CoutInput;	/* Decoder Input Struct */
		COLOROUT_FRAME_OUTPUT_INFO CoutOutput;	/* Decoder Output Struct */
};

#endif
