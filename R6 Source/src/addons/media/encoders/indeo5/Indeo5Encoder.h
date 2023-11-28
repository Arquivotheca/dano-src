#ifndef INDEO5ENCODER_H
#define INDEO5ENCODER_H

#include <SupportDefs.h>
#include <Encoder.h>

using namespace BPrivate;

/* indeo headers */
#undef MAX
#undef MIN
extern "C" {
	#include "datatype.h"
	#include "pia_main.h"
	#include "pia_enc.h"
	#include "pia_cin.h"
}

class Indeo5Encoder : public Encoder
{
	public:
		Indeo5Encoder();
		~Indeo5Encoder();

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
		status_t Init();
		status_t InitColor(color_space *cs);
		status_t InitFormat(media_file_format *mfi,
		                    media_format *in_format,
		                    media_format *out_format);

		class IndeoEnvironment {
			public:
				IndeoEnvironment();
				~IndeoEnvironment();
				ENVIRONMENT_INFO *GetEnvironment();
			private:
				bool encoder_ready;
				bool colorin_ready;
				ENVIRONMENT_INFO eienvironment;
		};
		
static	IndeoEnvironment indeoEnvironment;
		bool encoder_inst_ready;
		bool colorin_inst_ready;
		
		void *out_buffer;

		ENC_INST		EncInst;		/* Encoder Instance Structure */
		ENCODE_FRAME_INPUT_INFO EncInput;	/* Encoder Input Struct */
		ENCODE_FRAME_OUTPUT_INFO EncOutput;	/* Encoder Output Struct */
	
		CCIN_INST		CinInst;		/* Encoder Instance Structure */
		COLORIN_INPUT_INFO CinInput;	/* Encoder Input Struct */
		COLORIN_OUTPUT_INFO CinOutput;	/* Encoder Output Struct */
};

#endif

