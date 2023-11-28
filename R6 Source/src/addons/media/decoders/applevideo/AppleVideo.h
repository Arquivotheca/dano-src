#ifndef _APPLEVIDEO_H_
#define _APPLEVIDEO_H_

using namespace BPrivate;

class AppleVideoDecoder : public Decoder
{
public:
	AppleVideoDecoder();
	~AppleVideoDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format, const void *in_info,
					  size_t in_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *inout_frameCount,
					   media_header *out_mf, media_decode_info *info);
	
private:
	media_raw_video_format	output_format;
};


#endif /* _AppleVideo_H_ */
