#ifndef _CCITT_DECODER_H
#define _CCITT_DECODER_H

#include <Decoder.h>
#include "g72x.h"

using namespace BPrivate;

class CCITTDecoder : public Decoder {

public:
						CCITTDecoder();
	virtual				~CCITTDecoder();

	virtual status_t	GetCodecInfo(media_codec_info *mci) const;
	virtual status_t	Sniff(const media_format *in_format,
	                          const void *in_info, size_t in_size);
	virtual status_t	Format(media_format *inout_format);
	virtual status_t	Decode(void *out_buffer, int64 *out_frameCount,
							   media_header *mh, media_decode_info *info);
	virtual status_t	Reset(int32 in_towhat,
							  int64 in_requiredFrame, int64 *inout_frame,
							  bigtime_t in_requiredTime, bigtime_t *inout_time);

private:
	const char			*in_buffer;
	size_t				count;
	uint32				value;
	uint32				in_bits;
	g72x_state			fState;
	uint32				fSampleFormat;
	
	/* Received from the extractor */
	uint32				fBlocSize;
	uint32				fChannelCount;
	float				fRate;
	media_header 		fMh;
};

#endif
