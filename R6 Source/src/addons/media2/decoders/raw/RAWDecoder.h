#ifndef _RAW_DECODER_H
#define _RAW_DECODER_H

#include "Decoder.h"

namespace B {
namespace Media2 {

class RAWDecoder : public B::Private::Decoder {

public:
						RAWDecoder();
	virtual				~RAWDecoder();

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
	int32				CopyBuffer(const char *from, char *to, int32 size);

	const char			*in_buffer;
	size_t				size;
	off_t				offset;
	/* Received from the extractor */
	uint32				fSampleFormat;
	uint32				fBlocSize;
	uint32				fSamplePerBloc;
	uint32				fSampleSize;
	float				fRate;
	uint16				fChannelCount;
	bool				fSwapByte;
	media_header 		fMh;
	uint32				fChannelMask;
	int16				fValidBits;
	uint16				fMatrixMask;
};

} } // B::Media2
#endif
