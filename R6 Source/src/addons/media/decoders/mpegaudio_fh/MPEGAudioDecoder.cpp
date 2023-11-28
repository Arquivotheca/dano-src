// [em 27jan00]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>
#include <Extractor.h>
#include <Debug.h>

#include "MPEGAudioDecoder.h" 

// fraunhofer decoder
#include "mp3decifc.h"

#include <cstdio>
#include <cctype>

static media_encoded_audio_format::audio_encoding mpeg1_audio1;
static media_encoded_audio_format::audio_encoding mpeg1_audio2;
static media_encoded_audio_format::audio_encoding mpeg1_audio3;
static media_encoded_audio_format::audio_encoding mpeg1_audio4;
static media_encoded_audio_format::audio_encoding mpeg1_audio5;
static media_format mediaFormats[5];

static media_format_description	formatDescription;

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	formatDescription.family = B_MPEG_FORMAT_FAMILY;
	switch(*cookie) {
	case 0:
		formatDescription.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
		break;
	case 1:
		formatDescription.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_2;
		break;
	case 2:
		formatDescription.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_1;
		break;
	case 3:
		formatDescription.u.avi.codec = 0x65610055;  // DivX format
		formatDescription.family = B_AVI_FORMAT_FAMILY;
		break;
	case 4:
		formatDescription.u.quicktime.codec = '.mp3'; // 3ivX format
		formatDescription.u.quicktime.vendor = 0;
		formatDescription.family = B_QUICKTIME_FORMAT_FAMILY;
		break;
	default:
		return B_BAD_INDEX;
	}


	*otype = B_MEDIA_ENCODED_AUDIO;
	*odesc = &formatDescription;
	*ocount = 1;

	(*cookie)++;

	return B_OK;
}

void
register_decoder(const media_format ** out_formats, int32 * out_count)
{
	const media_format_description *desc;
	BMediaFormats formatObject;
	media_type type;
	int32 count;
	int32 cookie;
	status_t err;
	int i;

	if(out_formats == NULL || out_count == NULL)
		return;

	cookie = 0;
	for(i = 0; i < 5; i++)  {
		err = get_next_description(&cookie, &type, &desc, &count);
		if(err != B_OK) {
			break;
		}
		
		mediaFormats[i].type = type;
		memset(&mediaFormats[i].u, 0, sizeof(mediaFormats[i].u));

		err = formatObject.MakeFormatFor(desc, count, &mediaFormats[i]);
		if(err != B_OK) {
//			printf("MPEG-1 Audio Decoder: MakeFormatFor failed, %s\n", strerror(err));
			mediaFormats[i].u.encoded_audio.encoding = (media_encoded_audio_format::audio_encoding)0;
		}
	}
	
	mpeg1_audio1 = mediaFormats[2].u.encoded_audio.encoding;
	mpeg1_audio2 = mediaFormats[1].u.encoded_audio.encoding;
	mpeg1_audio3 = mediaFormats[0].u.encoded_audio.encoding;
	mpeg1_audio4 = mediaFormats[3].u.encoded_audio.encoding;
	mpeg1_audio5 = mediaFormats[4].u.encoded_audio.encoding;

	*out_formats = mediaFormats;
	*out_count = 5;
}

Decoder *instantiate_decoder()
{
	if (mpeg1_audio1 == 0 && mpeg1_audio2 == 0 && mpeg1_audio3 == 0
		 && mpeg1_audio4 == 0  && mpeg1_audio5 == 0) return 0;
	return new MPEGAudioDecoder;
}

MPEGAudioDecoder::MPEGAudioDecoder()
{
	fDecoderSem       = create_sem(1, "mpeg-audio-decoder");

	fCurFrame         = 0;
	fLayer            = 0;
	fNumChannels      = 0;
	fSampleRate       = 0;
	fBlocSize         = 4096;

	fDecoder          = 0;
	fDecoderBufferSize = DEFAULT_DECODER_BUFFER_SIZE;
	fDecoderBuffer = (uchar*)malloc(fDecoderBufferSize);

	_init_fh_decoder();
	
	fOutputBufferSize = DEFAULT_OUTPUT_BUFFER_SIZE;
	fOutputBuffer = (uchar*)malloc(fOutputBufferSize);
	fOutputBufferPos = 0;
	fOutputBufferUsed = 0;
	
	fProduceFloat      = false;
}

MPEGAudioDecoder::~MPEGAudioDecoder()
{
	if (fDecoderBuffer)
		free(fDecoderBuffer);
	fDecoderBuffer = NULL;

	if(fDecoder)
		fDecoder->Release();

	if (fOutputBuffer)
		free(fOutputBuffer);
	fOutputBuffer = NULL;

	delete_sem(fDecoderSem);
}

status_t MPEGAudioDecoder::GetCodecInfo(media_codec_info *mci) const
{
	if (fLayer == 0)
		strcpy(mci->pretty_name, "MPEG I Audio, Layers 1, 2, 2.5, 3");
	else
		sprintf(mci->pretty_name, "MPEG I Audio, Layer %d, %g kbps", fLayer, fBitRate/1000.0);
	strcpy(mci->short_name, "mp3");
	return B_OK;
}

status_t MPEGAudioDecoder::Sniff(const media_format *in_format,
								 const void *in_info, size_t in_size)
{
	status_t					err;

	if (in_format->type != B_MEDIA_ENCODED_AUDIO)
		return B_BAD_TYPE;

	if (in_format->u.encoded_audio.encoding == mpeg1_audio1)
		fLayer = 1;
	else if (in_format->u.encoded_audio.encoding == mpeg1_audio2)
		fLayer = 2;
	else if (in_format->u.encoded_audio.encoding == mpeg1_audio3)
		fLayer = 3;
	else if (in_format->u.encoded_audio.encoding == mpeg1_audio4)
		fLayer = 3;
	else if (in_format->u.encoded_audio.encoding == mpeg1_audio5)
		fLayer = 3;
	else
		return B_BAD_TYPE;

	fNumChannels = in_format->u.encoded_audio.output.channel_count;
	fSampleRate  = in_format->u.encoded_audio.output.frame_rate;
	if(in_format->u.encoded_audio.output.buffer_size > fBlocSize) {
		fBlocSize    = in_format->u.encoded_audio.output.buffer_size;
	}
	fBitRate	 = (int)in_format->u.encoded_audio.bit_rate;
	
	if (fBlocSize > fDecoderBufferSize) {
		int new_size = fDecoderBufferSize * 2; // Fraunhofer decoder requires 2^n-sized buffer
		uchar *new_buffer = (uchar *)realloc(fDecoderBuffer, new_size);		
		if (new_buffer == 0) {
			return B_NO_MEMORY;
		}
		fDecoderBuffer = new_buffer;
		fDecoderBufferSize = new_size;
		
		// create new decoder object
		_init_fh_decoder();
	}

	return B_OK;
}

status_t MPEGAudioDecoder::Format(media_format *inout_format)
{

	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	inout_format->type = B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = fSampleRate;
	inout_format->u.raw_audio.channel_count = fNumChannels;

#if !FIXED_POINT_DECODE
	if(inout_format->u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT) {
		fProduceFloat = true;
	}
	else
#endif
		inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;

	inout_format->u.raw_audio.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	inout_format->u.raw_audio.buffer_size = fBlocSize;

	return B_OK;
}


status_t MPEGAudioDecoder::Decode(
                  void* out_buffer,
                  int64* out_frameCount,
                  media_header* mh,
                  media_decode_info* mdi)
{
	int32	          copy_count, needed_count, amt;
	int             decode_count;
	status_t        err;
	media_header    header;
	
	// Fraunhofer return code
	SSC             ssc;

	uchar*          out_pos = (uchar*)out_buffer;
	int64           num_frames_incr = 0;
	bool	          last_input_buffer = false;

	// current chunk of encoded data from the track
	const uchar*    chunk = 0;
	size_t          chunk_size, chunk_pos;
	
	// minimum encoded data needed 
	size_t          min_decode_fill_amt = 2048;
	ASSERT(fDecodeBufferSize > min_decode_fill_amt);
	
	// ++++++
	const int       error_max = 16;
	int             error_count = 0;
	
	int             frame_size = (2 * fNumChannels * (fProduceFloat ? 2 : 1));
	
	*out_frameCount = 0;
	needed_count = fBlocSize;
	
	acquire_sem(fDecoderSem);

	//printf("\n### MP3 Decode()\n");
	
	while (needed_count > 0) {
		//printf("\nwe still need %d bytes of decoded data\n", needed_count);

		// first check if we have some decoded data that we can use
		amt = (fOutputBufferUsed - fOutputBufferPos);
		if (amt > 0) {
			if (amt > needed_count)
				copy_count = needed_count;
			else
				copy_count = amt;

			memcpy(out_pos, &fOutputBuffer[fOutputBufferPos], copy_count);
			//printf("copied %d bytes of previously decoded data\n", copy_count);
			
			fOutputBufferPos  += copy_count;
			needed_count      -= copy_count;
			out_pos           += copy_count;

			*out_frameCount   += copy_count / frame_size;	//	16 bit * channels
			num_frames_incr   += copy_count / frame_size;	//	16 bit * channels

			if (needed_count <= 0)   // are we all done?
				break;
		}

		// add data to the decoder's input buffer as needed
		//printf("%d bytes in input buffer%s\n", fDecoder->GetInputLeft(),
		//	last_input_buffer ? " (at last buffer)" : "");

		while((fDecoder->GetInputLeft() < min_decode_fill_amt) && !last_input_buffer) {
			// fetch a new chunk if the current one is dry
			if(!chunk || chunk_pos == chunk_size) {
				//printf("*** getting a chunk\n");
				chunk_size = 0;
				chunk_pos = 0;

				err = GetNextChunk((const void**)&chunk, &chunk_size, &header, mdi);
				if(err < B_OK) {
					if((err == B_LAST_BUFFER_ERROR) /*&& (*out_frameCount != 0)*/) {
						//printf("    --> last buffer\n");
						last_input_buffer = true;
						//break;
					}
					else {
						//printf("!!! GetNextChunk(): %s\n", strerror(err));
						release_sem(fDecoderSem);
						return err;
					}
				}
			}

			if(chunk_size > 0) {
				// add data to decoder buffer (as much as it wants)
				int fill_count = fDecoder->Fill(&chunk[chunk_pos], chunk_size-chunk_pos);
				//printf("filled %d\n", fill_count);
				chunk_pos += fill_count;
			}
		}

		//printf("chunk: at %ld of %ld%s\n", chunk_pos, chunk_size,
			//last_input_buffer ? " (at last buffer)" : "");
		
		// decode some data
		amt = fDecoder->GetInputLeft();
		if(amt == 0 && last_input_buffer && chunk_pos == chunk_size) {
			break;
		}
		
		if (amt >= min_decode_fill_amt || last_input_buffer) {

			// check if we need to make room in the output buffer
			amt = (fOutputBufferUsed - fOutputBufferPos);
			//printf("Moving %d bytes of decoded data\n", amt); 
			if (amt > 0)
				memmove(fOutputBuffer, &fOutputBuffer[fOutputBufferPos], amt);
			else
				amt = 0;
			fOutputBufferPos = 0;
			fOutputBufferUsed  = amt;

			// attempt to decode a frame
			if(fProduceFloat)
				ssc = fDecoder->DecodeFrame(
					(float*)&fOutputBuffer[fOutputBufferUsed],
					fOutputBufferSize-fOutputBufferUsed,
					&decode_count);
			else
				ssc = fDecoder->DecodeFrame(
					&fOutputBuffer[fOutputBufferUsed],
					fOutputBufferSize-fOutputBufferUsed,
					&decode_count);
			//printf("decoded %d bytes\n", decode_count);
				
			if(SSC_SUCCESS(ssc)) {
				// got it successfully
				fOutputBufferUsed += decode_count;
				error_count = 0;
			}
			else
			{
				if(last_input_buffer)
				{
					release_sem(fDecoderSem);
					return B_LAST_BUFFER_ERROR;
				}
			
				if(ssc == SSC_W_MPGA_SYNCNEEDDATA) {
					// decoder needs more input data
					size_t new_fill_amt = min_decode_fill_amt * 2;
					if(new_fill_amt >= fDecoderBufferSize)
						new_fill_amt = fDecoderBufferSize;
					if(new_fill_amt == min_decode_fill_amt) {
						// give up
						//printf("Fraunhofer too greedy, giving up\n");
						release_sem(fDecoderSem);
						return B_ERROR;
					}
					min_decode_fill_amt = new_fill_amt;				
				}
				else {
					printf("fraunhofer error: %s\n",
						mp3decGetErrorText(ssc));
					if(++error_count > error_max) { // bail after too many errors
						release_sem(fDecoderSem);
						return B_ERROR;
					}
				}
			}
		}
		
	}
	
	// no data fetched?
	if(needed_count == fBlocSize) {
		release_sem(fDecoderSem);
		return B_LAST_BUFFER_ERROR;
	}

	*mh = header;
	//	assuming 44.1 kHz
	mh->start_time = (int64)(1000000.0 * (float)fCurFrame / fSampleRate);
	fCurFrame += num_frames_incr;
	//printf("Returned %d frames\n", num_frames_incr);
	release_sem(fDecoderSem);
	return B_OK;
}

status_t MPEGAudioDecoder::Reset(int32 to_what,
								 int64 requiredFrame, int64 *inout_frame,
								 bigtime_t requiredTime, bigtime_t *inout_time)
{
	if (to_what == B_SEEK_BY_FRAME) {
		if (*inout_frame < 0)
			*inout_frame = 0;

		fCurFrame = *inout_frame;
	} else if (to_what == B_SEEK_BY_TIME) {
		if (*inout_time < 0)
			*inout_time = 0;
		
		fCurFrame = (int64)(((double)*inout_time / 1000000.0) * fSampleRate);
	} else {
		return B_BAD_VALUE;
	}

	fDecoder->Reset();

	fOutputBufferPos = 0;
	fOutputBufferUsed = 0;

	return B_OK;
}

status_t MPEGAudioDecoder::_init_fh_decoder() {
	if(fDecoder)
		fDecoder->Release();
		
	ASSERT(fDecoderBuffer);

#if FIXED_POINT_DECODE
	SSC ssc = mp3decCreateObjectExtBuf(
		fDecoderBuffer, fDecoderBufferSize,
		0, 0, 0,
		true,
		&fDecoder);
#else
	SSC ssc = mp3decCreateObjectExtBuf(
		fDecoderBuffer, fDecoderBufferSize,
		0, 0, 0,
		false,
		&fDecoder);
#endif		

	return SSC_SUCCESS(ssc) ? B_OK : B_ERROR;
}

// END -- MpegAudioDecoder.cpp --
