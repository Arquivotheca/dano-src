#ifndef _CINEPAK_H_
#define _CINEPAK_H_

using namespace BPrivate;

class CinepakDecoder : public Decoder
{
public:
	CinepakDecoder();
	~CinepakDecoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;
	status_t	Sniff(const media_format *in_format,
	        	      const void *info, size_t info_size);
	status_t	Format(media_format *inout_format);
	status_t	Decode(void *out_buffer, int64 *inout_frameCount,
					   media_header *mh, media_decode_info *info);
private:
	media_raw_video_format  output_format;
	void                    *dptr;
	int32					bitmap_depth;
};


#endif /* _CINEPAK_H_ */
