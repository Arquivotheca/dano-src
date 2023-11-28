#ifndef _IMA4_ENCODER_H
#define _IMA4_ENCODER_H

#include <Encoder.h>
#include "ima4_encode.h"

using namespace BPrivate;


class IMA4Encoder : public Encoder {
public:
				IMA4Encoder();
				~IMA4Encoder();

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
	status_t	EncodeBuffer(const char *src,
	                         media_encode_info *info);

	
	struct meta_data {
	    uint16				format_tag;
	    uint16				channel_count;
	    uint32				samples_per_sec;
	    uint32				avg_bytes_per_sec;
	    uint16				block_align;
	    uint16				bits_per_sample;
		uint16				sample_size;
		uint16				cb_size;
		uint16				samples_per_block;
	};
	
	enum {
		MAX_CHANNEL = 4
	};

	media_encode_info *fLastEncodeInfo;
	
	char					*fInBuffer;
	char					*fOutBuffer;
	size_t					fOutBufferSize;
	int32					fInLength;
	int32					fInLengthMax;
	int16					*fConvertBuffer;
	int32					fConvertBufferSize;
	int32					fPrevValue[MAX_CHANNEL];
	int32					fPrevIndex[MAX_CHANNEL];
	uint8					fChannelCount;
	uint8					fFormat;
	uint8					fBytesPerSample;
	uint32					fFrameSamples;
	uint32					fInputFrameSize;
	float					fFrameRate;
	media_format_family		fFamily;
};



#endif
