/* ============================================================================ */
#ifndef _G72X_ENCODER_H
#define _G72X_ENCODER_H
/* ============================================================================ */
#include <Encoder.h>
/* ============================================================================ */
#include "g72x.h"
/* ============================================================================ */
using namespace BPrivate;
/* ============================================================================ */
class g72xEncoder : public Encoder
{
public:
				g72xEncoder();
				~g72xEncoder();

	status_t	GetCodecInfo(media_codec_info *mci) const; 	//mandatory

	status_t	SetFormat(media_file_format *mfi,			//mandatory
						  media_format *in_format,
						  media_format *out_format);

	status_t	Encode(const void *in_buffer, int64 num_frames, //mandatory
	        	       media_encode_info *info);
	        	       
	void		AttachedToTrack();		//must exist: for the "fmt " chunk
	status_t	Flush();				//must exist: to dump bit output

private:
	// from SetFormat
	int32		_channels;
	int32		_sampling_rate;
	
	// encoder state 
	g72x_state	_encoder_state;
	
	// bit output
	uint32		_bit_value;
	uint32 		_bit_number;
};
/* ============================================================================ */
#endif
/* ============================================================================ */
