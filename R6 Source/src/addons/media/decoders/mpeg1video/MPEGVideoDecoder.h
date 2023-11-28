#ifndef MPEGVideoDecoder_H
#define MPEGVideoDecoder_H

#include "Decoder.h"

using namespace BPrivate;

class video_data;

class MPEGVideoDecoder : public Decoder {
public:

	MPEGVideoDecoder();
	~MPEGVideoDecoder();
	virtual status_t	GetCodecInfo(media_codec_info *mci) const;
	virtual status_t	Sniff(const media_format *in_format,
	                          const void *in_info, size_t in_size);
	virtual status_t	Format(media_format *inout_format);
	virtual status_t	Decode(void *out_buffer, int64 *out_frameCount,
							   media_header *mh, media_decode_info *info);
	virtual status_t	Reset(int32 in_towhat, int64 in_requiredFrame,
								int64 *inout_frame, bigtime_t in_requiredTime,
								bigtime_t *inout_time); 

private:

	status_t InitializeDecoder(void);

	static int StaticStreamRead(void *decoder, void *data, int len);
	int StreamRead(void *data, size_t len);

	void WriteFrame(unsigned char *src[], unsigned char *buffer);

	video_data *fDecoderState;
	int fFrame;
	float fFrameRate;
	const void *fInputChunk;
	size_t fInputChunkLength;
	uint32 fInputChunkOffset;
	
	media_header fMH;  // kept so StreamRead() has it to call GetNextChunk
	media_decode_info *fDecodeInfo;

	media_raw_video_format  fOutputFormat;
	int32 fHeight;
	int32 fWidth;
	uint8 *decodeBuf,*decodeBufRef;
	bool fUseYUV;
};



#endif
