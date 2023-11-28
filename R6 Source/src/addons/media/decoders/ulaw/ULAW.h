#ifndef _ULAW_DECODER_H
#define _ULAW_DECODER_H

#include <Decoder.h>

using namespace BPrivate;

class ULAWDecoder : public Decoder {

public:
						ULAWDecoder();
	virtual				~ULAWDecoder();

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
	status_t			InitTable(int32 method);

	const char			*in_buffer;
	size_t				count;
	int32				offset;
	
	/* Received from the extractor */
	uint32				fBlocSize;
	float				fRate;
	uint16				fChannelCount;
	int16				fTable[256];
	media_header 		fMh;
};

#endif
