#ifndef _IMA4_DECODER_H
#define _IMA4_DECODER_H

#include <Decoder.h>
#include "ima4.h"

using namespace BPrivate;

typedef int16 		out_type;		/* supported : int16 or float */

class IMA4Decoder : public Decoder {
public:
						IMA4Decoder();
	virtual				~IMA4Decoder();

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
	int32				cached_count;
	out_type			*cached_buffer;
	out_type			*fOutputCache;
	
	// Received from the extractor
	float				fRate;
	uint32				fChannelCount;
	uint32				fBlocSize;
	// one packet per channel for QT; channels interleaved within packet for WAV
	// (in chunks of 8 samples)
	uint32				fPacketSize;
	// decoded samples per packet
	uint32				fDecodedPacketCount;
	media_header 		fMh;

	// packet header config
	uint32				fPacketHeaderSize;

	// block-interleave	config:
	// 1 for QT
	uint32				fChannelsPerPacket;
	// if fChannelsPerPacket > 1, this defines the number of samples encoded
	// consecutively for each channel to make up a chunk.
	uint32				fPacketChunkSamples;
	
	media_encoded_audio_format::audio_encoding	fEncoding;
	
	// decode packet(s) (one for each channel if QT, one for WAV)
	// (will decode a partial block if not enough data is available)
	status_t DecodePacketBlock(
		uint8* decoded,
		uint32* outSamplesWritten);

	// read a QT or WAV header; for WAV, expects state to be an array of
	// size fChannelCount
	status_t ReadPacketHeader(
		ima4_adpcm_state* state);
};

#endif
