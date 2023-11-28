#ifndef _MSADPCM_ENCODER_H
#define _MSADPCM_ENCODER_H

#include <MediaFormats.h>
#include <Encoder.h>
#include "msadpcm_encode.h"
#include "RIFFTypes.h"

using namespace BPrivate;

enum {
	COEFF_COUNT	= 7
};

class MSADPCMEncoder : public Encoder {
public:
	struct header {
		char					*in_buffer;
		char					*out_buffer;
		size_t					out_buffer_size;
		int32					in_length;
		int32					in_length_max;
		int32					frame_size;
		media_format_family		family;
		PCMWaveFormat			in_fmt;
		ADPCMWaveFormat			out_fmt;
		ADPCMCoefSet			_out_fmt_extension[COEFF_COUNT-1];
	};

				MSADPCMEncoder();
				~MSADPCMEncoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;

	status_t	SetFormat(media_file_format *mfi,
						  media_format *in_format,
						  media_format *out_format);
	status_t	StartEncoder();

	void		AttachedToTrack();
	
	status_t	Encode(const void *in_buffer, int64 num_frames,
	        	       media_encode_info *info);

	status_t	Flush();

private:
	status_t	InitFormats(media_file_format *mfi,
							media_format *in_fmt, media_format *out_fmt);
	status_t	EncodeBuffer(const char *src, int32 src_length);

	header		fHeader;
	AVIAUDSHeader	fMetaData;
	media_encode_info *fLastEncodeInfo;
};



#endif
