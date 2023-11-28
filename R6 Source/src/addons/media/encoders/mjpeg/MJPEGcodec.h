#ifndef _MJPEG_H_
#define _MJPEG_H_

#include <MediaFormats.h>
#include "Encoder.h"
#include "jpeg_interface.h"

using namespace BPrivate;

class MJPEGEncoder : public Encoder
{
public:
	MJPEGEncoder();
	~MJPEGEncoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;

	status_t	SetFormat(media_file_format *mfi,
						  media_format *in_format,
						  media_format *out_format);

	status_t	StartEncoder();
	status_t	Encode(const void *in_buffer, int64 num_frames,
	        	       media_encode_info *info);
	
	status_t	GetEncodeParameters(encode_parameters *parameters) const;
	status_t	SetEncodeParameters(encode_parameters *parameters);

private:
	int          fWidth, fHeight;
	int			 fQuality;
	char 		 *fOutputBuffer;
	char 		 *fBuffer1,*fBuffer2;
	int32		 fOutputSize;      // size in bytes of fOutputBuffer
	jpeg_stream  fStream;
};


#endif /* _MJPEG_H_ */
