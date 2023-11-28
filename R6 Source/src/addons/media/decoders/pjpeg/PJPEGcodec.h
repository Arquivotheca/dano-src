#ifndef _PJPEG_H_
#define _PJPEG_H_

#include "Decoder.h"
#include "jpeg_interface.h"

using namespace BPrivate;

class PJPEGDecoder : public Decoder
{
public:
	PJPEGDecoder();
	~PJPEGDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format,
					  const void *in_info, size_t in_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *inout_framecout,
					   media_header *mh, media_decode_info *info);
	status_t	Reset(int32 in_towhat,
					  int64 in_requiredFrame, int64 *inout_frame,
					  bigtime_t in_requiredTime,bigtime_t *inout_time);
	
private:
	media_raw_video_format  fOutputFormat;
	jpeg_stream             fStream;
	bool                    fMJPEG;
	bool					fTopFieldF1;
	bool					fCombineFields;
	const void				*fInputBuffer;
	size_t					fInputBufferSize;
	bigtime_t				fInputBufferTime;
	int                     fField;
	media_header			fMh;
};


#endif /* _PJPEG_H_ */
