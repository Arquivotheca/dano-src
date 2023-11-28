#ifndef _MP3_ENCODER_H
#define _MP3_ENCODER_H

#include <Encoder.h>
#include <LocalControllable.h>
#include <unistd.h>
#include <stdio.h>

class BParameterWeb;
namespace BPrivate {
	class mp3_config;
};

using namespace BPrivate;

class MP3Encoder : public Encoder, private BLocalControllable {
public:

				MP3Encoder();
				~MP3Encoder();

	status_t	GetCodecInfo(media_codec_info *mci) const;

	status_t    StartEncoder();
	
	status_t	SetFormat(media_file_format *mfi,
						  media_format *in_format,
						  media_format *out_format);
	void		AttachedToTrack();
	
	status_t	Encode(const void *in_buffer, int64 num_frames,
					   media_encode_info *info);
	status_t	Flush();

	// Encoder parameter hooks
	BParameterWeb	*Web();
	status_t 		GetParameterValue(int32 id, void *valu, size_t *size);
	status_t		SetParameterValue(int32 id, const void *valu, size_t size);
	BView			*GetParameterView();
	virtual	status_t	CommitHeader();

private:


	bool                        m_is_wav_file;
	bool                        m_is_avi_file;
	media_raw_audio_format		m_format;
	char *						m_buffer;		//	FIFO for input data in int16 fmt
	size_t						m_bufAvail;		//	how many bytes are in buffer
	size_t						m_chunkSize;	//	total FIFO size in bytes (1152 frames)
	void *						m_cookie;		//	encoder state
	bool						m_flushed;		//	safeguard against multiple Flush()
	mp3_config *				m_config;
	BParameterWeb *				m_web;

	inline size_t xform(int cnt) {	//	how much will some data fill of the buffer?
		return cnt*2/(m_format.format&0xf);
	}
	inline void convert(const char * & buf, size_t & cnt) {
		int16 * const dest_ptr = (int16 *)(m_buffer+m_bufAvail);
		int dest_bytes = m_chunkSize-m_bufAvail;	//	how many dest bytes?
		int togo = dest_bytes;
		if (m_format.format == 0x2) {
			//	just copy to fill buffer
			if (togo > cnt) togo = cnt;
			memcpy(m_buffer+m_bufAvail, buf, togo);
			buf += togo;
			cnt -= togo;
			m_bufAvail += togo;
		}
		else {
			togo /= 2;	//	samples, not bytes, to put in buffer
			if (togo > cnt/(m_format.format & 0xf)) {
				togo = cnt/(m_format.format & 0xf);
			}
			int16 * out = (int16 *)(m_buffer+m_bufAvail);
			switch (m_format.format) {
			//	need to convert from input to buffer
			case 0x1:
				for (int i=0; i<togo; ++i) {
					out[i] = buf[i] * 257;	//	closest we can come without fractional bit arithmetic
				}
				break;
			case 0x4:
				for (int i=0; i<togo; ++i) {
					out[i] = ((int32 *)buf)[i]>>16;
				}
				break;
			case 0x11:
				for (int i=0; i<togo; ++i) {
					out[i] = ((int8)(((uchar *)buf)[i]^0x80)) * 257;
				}
				break;
			case 0x24:
				for (int i=0; i<togo; ++i) {
					int s = (int)(((float *)buf)[i] * 32767.0);
					out[i] = (s > 32767) ? 32767 : (s < -32767) ? -32767 : s;
				}
				break;
			default:	//	this should not happen because of argument checking before here
				fprintf(stderr, "MP3Encoder: unknown sample format 0x%x\n", m_format.format);
				abort();
			}
			buf += togo*(m_format.format&0xf);
			m_bufAvail += togo*2;
			cnt -= togo*(m_format.format&0xf);
		}
		if (m_format.byte_order != B_MEDIA_HOST_ENDIAN) {
			swap_data(B_INT16_TYPE, dest_ptr, dest_bytes, B_SWAP_ALWAYS);
		}
	}

	status_t	EncodeBuffer(void * src, int32 src_length,
							 media_encode_info *mei);

	// BLocalControllable hooks
	status_t GetParameterValue(
			int32 id,
			bigtime_t * last_change,
			void * value,
			size_t * ioSize);

	void SetParameterValue(
			int32 id,
			bigtime_t when,
			const void * value,
			size_t size);
	
	BParameterWeb* InitParameterWeb(mp3_config* config);
};



#endif
