// [em 25jan00] fraunhofer decoder wrapper
#ifndef _MPEG_AUDIO_DECODER_H
#define _MPEG_AUDIO_DECODER_H

#include <OS.h>
#include <Decoder.h>

using namespace BPrivate;

// the Fraunhofer decoder
class IMpgaDecoder;

class MPEGAudioDecoder : public Decoder {
public:


	MPEGAudioDecoder();
	virtual ~MPEGAudioDecoder();
	
	virtual status_t	GetCodecInfo(media_codec_info *mci) const;
	virtual status_t	Sniff(const media_format *in_format,
							  const void *in_info, size_t in_size);
	virtual status_t	Format(media_format *inout_format);
	virtual status_t	Decode(void *out_buffer, int64 *out_frameCount,
							   media_header *mh, media_decode_info *mdi);
	virtual status_t	Reset(int32 in_towhat,
							  int64 in_requiredFrame, int64 *inout_frame,
							  bigtime_t in_requiredTime, bigtime_t *inout_time);

private:
	sem_id        fDecoderSem;

	int64         fCurFrame;
	int           fNumChannels;
	float         fSampleRate;
	int           fBitRate;
	int           fLayer;
	int           fBlocSize;

	IMpgaDecoder* fDecoder;
	
	bool					fProduceFloat;
	
	// this buffer is effectively internal to the decoder
	// +++++rtalloc!
	uchar*        fDecoderBuffer;
	int           fDecoderBufferSize;
	
	// outbound-data buffer
	// +++++rtalloc!
	uchar*        fOutputBuffer;
	int           fOutputBufferSize;
	int           fOutputBufferPos;
	int           fOutputBufferUsed;

	status_t _init_fh_decoder();
};

enum {
	DEFAULT_DECODER_BUFFER_SIZE = 16384,
	DEFAULT_OUTPUT_BUFFER_SIZE = 16384
};

#endif
