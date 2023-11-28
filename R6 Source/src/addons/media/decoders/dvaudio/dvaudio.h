#ifndef DVAUDIO_H
#define DVAUDIO_H

#include <MediaFile.h>
#include <MediaTrack.h>
#include "Extractor.h"
#include "Decoder.h"

using namespace BPrivate;

struct dif_block
{
	uchar			id0;
	uchar			id1;
	uchar			id2;
	uchar			data[77];
} _PACKED;

struct dif_sequence
{
	dif_block		block[150];
};

struct dv_frame
{
	dif_sequence	sequence[1];	// 10 for NTSC, 12 for PAL
};

// audio modes
enum {
	LINEAR_16		= 0,
	NONLINEAR_12	= 1,
	LINEAR_20		= 2
};

class DVAudioDecoder : public Decoder
{
public:
	DVAudioDecoder();
	~DVAudioDecoder();

	status_t	GetCodecInfo(media_codec_info *mfi) const;
	status_t	Sniff(const media_format *in_format, const void *in_info,
				      size_t in_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *inout_frameCount,
				       media_header *mh, media_decode_info *info);
	status_t	Reset(int32 in_towhat, int64 in_requiredFrame,
	        	      int64 *inout_frame, bigtime_t in_requiredTime,
	        	      bigtime_t *inout_time);
private:
	dv_frame	*fFrame;
	bool		fPAL;
	int32		fFirstSample;
	int32		fLastSample;
	int32		fQuant;
	int32		fFrameChannelCount;
	float		fFrameFrameRate;
	
	int32		fOutputChannelCount;
	float		fOutputFrameRate;
};


#endif // DVAUDIO_H
