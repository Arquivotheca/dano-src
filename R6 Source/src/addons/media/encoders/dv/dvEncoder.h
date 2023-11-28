#ifndef _PJPEG_H_
#define _PJPEG_H_

#include <MediaFormats.h>
#include "Encoder.h"

using namespace BPrivate;

extern "C" {
	#include <windows.h>
}
#include "dvcodec.h"
 
class DVEncoder : public Encoder
{
public:
	DVEncoder();
	~DVEncoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;

	status_t	SetFormat(media_file_format *mfi,
	        	          media_format *in_format,
	        	          media_format *out_format);

	status_t	StartEncoder();
	status_t	Encode(const void *in_buffer, int64 num_frames,
	        	       media_encode_info *info);
	
private:
	int			fWidth, fHeight;
	color_space	fSpace;
	char		*fOutputBuffer;
	int32		fOutputSize;      // size in bytes of fOutputBuffer
	int32		numframes;
	bigtime_t	tt;

	DVCodec		fDVCodec;
};


#endif /* _PJPEG_H_ */
