#ifndef _MPEG_AUDIO_DECODER_H
#define _MPEG_AUDIO_DECODER_H

#include <OS.h>
#include <Decoder.h>
#include "mhead.h"
#include "port.h"

using namespace BPrivate;

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
	int			  fBitRate;
	int           fLayer;
	int           fBlocSize;

	bool          fMpegHeadInit;
	MPEG_HEAD     fMpegHead;

	media_header  fMediaHeader;
	
	int           fFrameBytes;

//	bool          fResyncNeeded;

	uchar        *fEncodedData;
	int           fEncodedDataIndex;
	int           fEncodedDataSize;
	int           fEncodedDataMax;

	uchar        *fDecodedData;
	int           fDecodedDataIndex;
	int           fDecodedDataSize;
	int           fDecodedDataMax;
	
	// workaround for Xing MP3 decoder bug
	image_id	  fMP3Library;
	int			(*audio_decode_init)(MPEG_HEAD *h, int framebytes_arg,
					int reduction_code, int transform_code, int convert_code,
					int freq_limit);
	IN_OUT		(*audio_decode)(unsigned char *bs, short *pcm);
	int			(*head_info2)(unsigned char *buf, unsigned int n,
					MPEG_HEAD *h, int *br);
};

enum {
	MAX_ENCODED_BUFFER_SIZE = 16384,
	MAX_DECODED_BUFFER_SIZE = 16384
};

#endif
