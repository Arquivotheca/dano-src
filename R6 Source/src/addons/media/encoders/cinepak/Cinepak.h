#ifndef _CINEPAK_H_
#define _CINEPAK_H_

#include <MediaFormats.h>
#include "Encoder.h"

using namespace BPrivate;

class CinepakEncoder : public Encoder
{
public:
	CinepakEncoder();
	~CinepakEncoder();

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
	float			 fQuality;
	char 		 *fOutputBuffer;
	int32		 fOutputSize;      // size in bytes of fOutputBuffer
	void		*cinepakPtr;
	color_space	fColorspace;
};


#endif /* _CINEPAK_H_ */
