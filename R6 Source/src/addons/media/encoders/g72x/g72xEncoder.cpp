/* ============================================================================ */
#include "g72xEncoder.h"
#include "g72xEncoderSpecific.h"
/* ============================================================================ */
#include <stdio.h>
/* ============================================================================ */

//debug output
#define PRINTF if (0) printf

//global variables
media_encoded_audio_format::audio_encoding wav_encoding;

/* ============================================================================ */
void
register_encoder(void)
{
	PRINTF("g72x : register_encoder\n");
	
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[1];
	BMediaFormats				formatObject;

	//init mediaFormat
	mediaFormat.type            = B_MEDIA_ENCODED_AUDIO;
	mediaFormat.u.encoded_audio = media_encoded_audio_format::wildcard;
	
	//init formatDescription
	memset(formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family      = B_WAV_FORMAT_FAMILY;
	formatDescription[0].u.wav.codec = G72X_WAVE_FORMAT;
	
	//init formatObject
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat);
	if(err != B_NO_ERROR)
	{
		PRINTF("playground: MakeFormatFor failed, %s\n", strerror(err));
	}
	
	//get encoding by side effect of MakeFormatFor
	wav_encoding = mediaFormat.u.encoded_audio.encoding;
}
/* ============================================================================ */
Encoder *
instantiate_encoder(void)
{
	PRINTF("g72x : instantiate_encoder\n");
	
	Encoder *return_value = NULL;
	
	if(wav_encoding != 0)
	{
		return_value = new g72xEncoder();
	}
	
	return return_value;
}
/* ============================================================================ */
g72xEncoder::g72xEncoder()
{
	PRINTF("g72x : g72xEncoder::g72xEncoder\n");
	
	// from SetFormat
	_channels      = 0;
	_sampling_rate = 0;
	
	// encoder state
	g72x_init_state(&_encoder_state);
	
	//bit output
	_bit_value 	= 0;
	_bit_number = 0;
}
/* ============================================================================ */
g72xEncoder::~g72xEncoder()
{
	PRINTF("g72x : g72xEncoder::~g72xEncoder\n");
}
/* ============================================================================ */
status_t
g72xEncoder::GetCodecInfo(media_codec_info *mci) const
{
	PRINTF("g72x : g72xEncoder::GetCodecInfo\n");

	strcpy(mci->pretty_name, G72X_PRETTY_NAME);
	strcpy(mci->short_name,  G72X_SHORT_NAME);

	return B_OK;
}
/* ============================================================================ */
status_t
g72xEncoder::SetFormat
(
	media_file_format *mfi,
	media_format *in_format,
    media_format *out_format
)
{
	PRINTF("g72x : g72xEncoder::SetFormat\n");

	//check parameters
	if (!mfi || !in_format || !out_format)
	{
		PRINTF("playground : Bad parameters (NULL pointer)\n");
		return B_BAD_VALUE;		
	}

	//check family (we support only waves)
	if (mfi->family != B_WAV_FORMAT_FAMILY)
	{
		PRINTF("g72x : We can't handle family different than WAV\n");
		return B_MEDIA_BAD_FORMAT;		
	}

	//stored what's needed for building the fmt chunk
	{
		_channels      = (int32) in_format->u.raw_audio.channel_count;
		_sampling_rate = (int32) in_format->u.raw_audio.frame_rate;
	}

	//modify input format (to be sure it's what's expected)
	{
		// we accept only RAW audio, ...
		if (in_format->type != B_MEDIA_RAW_AUDIO)
		{
			in_format->type = B_MEDIA_RAW_AUDIO;
			PRINTF("   in_format->type modified to B_MEDIA_RAW_AUDIO\n");
		}
		
		// ...formatted with short
		if (in_format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT)
		{
			in_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
			PRINTF("   in_format->u.raw_audio.format modified to media_raw_audio_format::B_AUDIO_SHORT\n");
		}
	}

	// build output format
	{
		// most of the output format is the same as the imput one
		{
			*out_format = *in_format;
		}
			
		// the output format is encoded
		{
			out_format->type = B_MEDIA_ENCODED_AUDIO;
			out_format->u.encoded_audio.encoding = wav_encoding;
		}		
	}

	return B_OK;
}
/* ============================================================================ */
void
g72xEncoder::AttachedToTrack()
{
	PRINTF("g72x : g72xEncoder::AttachedToTrack\n");

	// The WAV writer is unable to generate the "fmt " chunk for most WAVE
	// codecs. So, we generate it here and passes it as track information (the
	// WAV writer dumps this track information as the "fmt " chunk)

	//allocate some memory
	char *fmt_chunk = new char[14];
	
	//fill the "fields"
	*((int16 *)&fmt_chunk[ 0]) = G72X_WAVE_FORMAT;			//Format category
	*((int16 *)&fmt_chunk[ 2]) = _channels;					//Number of Channels 	
	*((int32 *)&fmt_chunk[ 4]) = _sampling_rate;			//Sampling rate 	
	*((int32 *)&fmt_chunk[ 8]) = 1;							//For buffer estimation (unused/optional ??) 	
	*((int32 *)&fmt_chunk[ 8])*= _channels*_sampling_rate;		// number of samples 	
	*((int32 *)&fmt_chunk[ 8])*= G72X_BITS_PER_SAMPLE; 			//number of bits
	*((int32 *)&fmt_chunk[ 8])/= 8; 							//number of bytes
	*((int16 *)&fmt_chunk[12]) = 1;							//Data block size 		(unused/optional ??)

	//"write" the fmt chunk
	AddTrackInfo
	(
		0,							// is unused in WAVWriter::AddTrackInfo
		(const char *)fmt_chunk,	// pointer to fmt chunk
		14				            // size of fmt chunk
	);
	
	//free the memory
	delete[] fmt_chunk;
}
/* ============================================================================ */
status_t
g72xEncoder::Encode(const void *in_buffer, int64 num_frames,
	        	       media_encode_info *info)
{
	PRINTF("g72x : g72xEncoder::Encode (num_frames = %d)\n", num_frames);
	
	int16 *short_buffer = (int16 *) in_buffer;
	int64  num_samples  = num_frames * _channels;	

	//calculate size of chunk outputed during this call
	uint32 chunk_size;
		{
			chunk_size  = _bit_number;
			chunk_size += G72X_BITS_PER_SAMPLE*num_samples;			
			chunk_size /= 8;
		}
						
	//allocate a buffer for it
	uint8 *out_chunk   = (uint8 *)malloc(chunk_size);	
	int32  out_chunk_i = 0;
		
	//encode samples
	{
		int64 i;
		for (i=0;i<num_samples;i++)
		{
			//generate bits
			uint32 bit_code = G72X_ENCODER_FUNCTION(short_buffer[i],
									    			AUDIO_ENCODING_LINEAR,
									    			&_encoder_state);
			//add to bit output
			_bit_value  += bit_code << _bit_number;
			_bit_number += G72X_BITS_PER_SAMPLE;
			
			//store in chunk if needed
			if (_bit_number >= 8)
			{
				out_chunk[out_chunk_i] = (uint8) _bit_value;
				out_chunk_i++;
				
				_bit_value >>= 8;
				_bit_number -= 8;
			}
		}		
	}
				
	//write chunk and free it
	{
		WriteChunk(out_chunk, chunk_size, info);
		free(out_chunk);		
	}
	
	return B_OK;
}
/* ============================================================================ */
status_t
g72xEncoder::Flush()
{
	PRINTF("g72x : g72xEncoder::Flush\n");

	if (_bit_number > 0)
	{
		PRINTF ("flushing %d bits\n", _bit_number);
		
		media_encode_info mei;
		
		uint8 content = (uint8) _bit_value;
		WriteChunk(&content, 1, &mei);
		
		_bit_value  = 0;
		_bit_number = 0;
	}

	return B_OK;
}
/* ============================================================================ */
