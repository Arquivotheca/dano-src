#ifndef _MSADPCM_DECODER_H
#define _MSADPCM_DECODER_H

#include <Decoder.h>
#include <msmedia.h>

using namespace BPrivate;

class MSADPCMDecoder : public Decoder {

public:
						MSADPCMDecoder();
	virtual				~MSADPCMDecoder();

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
	int32				offset;
	
	/* Received from the extractor */
	uint32				fBlocSize;
	char				*fOutputCache;
	uint32				fOutputCacheSize;
	PCMWaveFormat		fHeaderOut;
	ADPCMWaveFormat		*fHeader;
	media_header 		fMh;
};

#endif
