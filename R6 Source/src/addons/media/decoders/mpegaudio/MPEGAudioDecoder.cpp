#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>
#include <Extractor.h>
#include <Path.h>
#include <FindDirectory.h>
#include <image.h>
#include "MPEGAudioDecoder.h"

int kBufferSize = 0x20000;



#include <stdio.h>
#include <ctype.h>

unsigned short layer1_bit_rate_11172[] = {
	0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0
};

unsigned short layer2_bit_rate_11172[] = {
	0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0
};

unsigned short layer3_bit_rate_11172[] = {
	0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};

unsigned short layer1_bit_rate_13818[] = {
	0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0
};

unsigned short layer2_bit_rate_13818[] = {   /* also for layer 3 */
	0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
};

static media_encoded_audio_format::audio_encoding mpeg1_audio1;
static media_encoded_audio_format::audio_encoding mpeg1_audio2;
static media_encoded_audio_format::audio_encoding mpeg1_audio3;
static media_format mediaFormats[3];

void register_decoder(const media_format ** out_formats, int32 * out_count)
{
	BMediaFormats bmf;
	media_format_description d;
	status_t err;

	memset(&d, 0, sizeof(d));
	d.family = B_MPEG_FORMAT_FAMILY;
	d.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_1;
	mediaFormats[0].type = B_MEDIA_ENCODED_AUDIO;
	mediaFormats[0].u.encoded_audio = media_encoded_audio_format::wildcard;
	err = bmf.MakeFormatFor(&d, 1, &mediaFormats[0]);
	if (err == B_OK)
		mpeg1_audio1 = mediaFormats[0].u.encoded_audio.encoding;

	d.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_2;
	mediaFormats[1].type = B_MEDIA_ENCODED_AUDIO;
	mediaFormats[1].u.encoded_audio = media_encoded_audio_format::wildcard;
	err = bmf.MakeFormatFor(&d, 1, &mediaFormats[1]);
	if (err == B_OK)
		mpeg1_audio2 = mediaFormats[1].u.encoded_audio.encoding;

	d.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
	mediaFormats[2].type = B_MEDIA_ENCODED_AUDIO;
	mediaFormats[2].u.encoded_audio = media_encoded_audio_format::wildcard;
	err = bmf.MakeFormatFor(&d, 1, &mediaFormats[2]);
	if (err == B_OK)
		mpeg1_audio3 = mediaFormats[2].u.encoded_audio.encoding;

	*out_formats = mediaFormats;
	*out_count = 3;
}


Decoder *instantiate_decoder()
{
	if ((mpeg1_audio1 == 0) && (mpeg1_audio2 == 0) && (mpeg1_audio3 == 0)) return 0;
	return new MPEGAudioDecoder;
}

MPEGAudioDecoder::MPEGAudioDecoder()
{
	fDecoderSem       = create_sem(1, "mpeg-audio-decoder");

	fMpegHeadInit     = false;
	fFrameBytes       = 0;

//	fResyncNeeded     = false;

	fCurFrame         = 0;
	fLayer            = 0;
	fNumChannels      = 0;
	fSampleRate       = 0;
	fBlocSize         = 4096;

	fEncodedData      = (uchar *)malloc(MAX_ENCODED_BUFFER_SIZE);
	fEncodedDataMax   = MAX_ENCODED_BUFFER_SIZE;
	fEncodedDataSize  = 0;
	fEncodedDataIndex = 0;

	fDecodedData      = (uchar *)malloc(MAX_DECODED_BUFFER_SIZE);
	fDecodedDataMax   = MAX_DECODED_BUFFER_SIZE;
	fDecodedDataSize  = 0;
	fDecodedDataIndex = 0;

	memset(&fMediaHeader, 0, sizeof(fMediaHeader));
	memset(&fMpegHead, 0, sizeof(fMpegHead));
	
	// the Xing MP3 decoder is not thread-safe, so we load
	// it as an add-on for each decoder
	BPath path;
	find_directory(B_BEOS_LIB_DIRECTORY, &path);
	path.Append("libmp3.so");
	fMP3Library = load_add_on(path.Path());

	if (get_image_symbol(fMP3Library, "audio_decode_init",
				B_SYMBOL_TYPE_TEXT, (void **)&audio_decode_init) < B_OK)
		debugger("unable to get audio_decode_init from libmp3.so!");
	if (get_image_symbol(fMP3Library, "audio_decode",
				B_SYMBOL_TYPE_TEXT, (void **)&audio_decode) < B_OK)
		debugger("unable to get audio_decode from libmp3.so!");
	if (get_image_symbol(fMP3Library, "head_info2",
				B_SYMBOL_TYPE_TEXT, (void **)&head_info2) < B_OK)
		debugger("unable to get head_info2 from libmp3.so!");
}

MPEGAudioDecoder::~MPEGAudioDecoder()
{
	if (fDecodedData)
		free(fDecodedData);
	fDecodedData = NULL;

	if (fEncodedData)
		free(fEncodedData);
	fEncodedData = NULL;

	memset(&fMediaHeader, 0, sizeof(fMediaHeader));
	delete_sem(fDecoderSem);

	unload_add_on(fMP3Library);
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
	else 
		return B_BAD_TYPE;

	fNumChannels = in_format->u.encoded_audio.output.channel_count;
	fSampleRate  = in_format->u.encoded_audio.output.frame_rate;
	fBlocSize    = in_format->u.encoded_audio.output.buffer_size;
	fBitRate	 = (int)in_format->u.encoded_audio.bit_rate;
	
	if (fBlocSize > fEncodedDataMax - 4096) {
		int new_size = fBlocSize + 4096;
		uchar *new_buffer = (uchar *)realloc(fEncodedData, new_size);		
		if (new_buffer == 0) {
			return B_NO_MEMORY;
		}
		fEncodedData = new_buffer;
		fEncodedDataMax = new_size;
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
	inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	inout_format->u.raw_audio.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	inout_format->u.raw_audio.buffer_size = fBlocSize;

	return B_OK;
}


static int
is_mpeg_audio_header(unsigned char *hdr, size_t len, bool knownBitRate, int32 recursionDepth = 0)
{
	if (len < 4) {
		return recursionDepth > 0;
	}

	int index, bit_rate, layer, sample_rate, id;
	bool padding;
	
	if (hdr[0] != 0xff || (hdr[1] & 0xf0) != 0xf0) {
		return 0;
	}
	/* set layer = 1, 2, or 3 */
	layer = 4 - ((hdr[1] & 0x6) >> 1);
	if (layer == 4) {
		return 0;
	}

	/* set id */
	id = (hdr[1] & 0x08) >> 3;

	/* check that the bit_rate index is in range */
	index = (hdr[2] & 0xf0) >> 4;
	if (index == 15) {
		return 0;
	}

	/* set the bit_rate */
	if (id == 1) {    /* 11172 format tables */
		if (layer == 1)
			bit_rate = layer1_bit_rate_11172[index];
		else if (layer == 2)
			bit_rate = layer2_bit_rate_11172[index];
		else /*if (layer == 3)*/
			bit_rate = layer3_bit_rate_11172[index];
	} else {
		if (layer == 1)
			bit_rate = layer1_bit_rate_13818[index];
		else /*if (layer == 2 || layer == 3)*/
			bit_rate = layer2_bit_rate_13818[index];
	}
	
	if (knownBitRate && bit_rate == 0) {
		return 0;
	}
	
	/* check that the sample rate index is in range */
	index = (hdr[2] & 0x0c) >> 2;

	/* calculate sample_rate */
	switch (index) {
	case 0:
		sample_rate = (id == 0) ? 22050 : 44100;
		break;
	case 1:
		sample_rate = (id == 0) ? 24000 : 48000;
		break;
	case 2:
		sample_rate = (id == 0) ? 16000 : 32000;
		break;
	default:
		return 0;
	}
	
	padding = ((hdr[2] & 0x2) == 2);
	
	/* check that the emphasis field is not illegal */
	if ((hdr[3] & 3) == 2) {
		return 0;
	}

	if (bit_rate == 0) {
		/* if we get here it is likely that this is indeed an mpeg audio header */
		return 1;
	}
	
	/* calculate the size of this frame */
	size_t frame_size;
	int pad_bytes;
	if (layer == 1) {
		pad_bytes = padding ? 4 : 0;
		frame_size = ((size_t)((12 * 1000 * bit_rate) / (float)sample_rate)) * 4 + pad_bytes;
	} else {
		pad_bytes = padding ? 1 : 0;
		if (layer == 3 && id == 0) {
			// special case for MPEG-2 layer 3
			frame_size = (size_t)((72 * 1000 * bit_rate) / (float)sample_rate) + pad_bytes;		
		} else {
			frame_size = (size_t)((144 * 1000 * bit_rate) / (float)sample_rate) + pad_bytes;				
		}
	}
	
//	printf("frame_size = %lu, bit_rate = %d, sample_rate = %d, padding = %d, layer = %d, len = %lu\n", frame_size, bit_rate, sample_rate, padding, layer, len);
	if (len < frame_size) {
		return recursionDepth > 0;
	} else if (recursionDepth > 8) {
		// this is the last frame we can look at with this data, and it matched size
		return 1;
	} else {
//		printf("recursing - depth %ld\n", recursionDepth);
		int r = is_mpeg_audio_header(hdr + frame_size, len - frame_size, knownBitRate, recursionDepth + 1);
		if (r == 0) {
//			printf("\tdata = %02x %02x - %02x %02x %02x %02x - %02x %02x\n",
//				hdr[frame_size-2], hdr[frame_size-1], hdr[frame_size],
//				hdr[frame_size+1], hdr[frame_size+2], hdr[frame_size+3],
//				hdr[frame_size+4], hdr[frame_size+5]);
		}
		return r;
	}
}



status_t MPEGAudioDecoder::Decode(void *out_buffer,
								  int64 *out_frameCount,
								  media_header *mh,
								  media_decode_info *mdi)
{
	int32	  i, copy_count, needed_count, decoded_count, amt;
	status_t  err;
	IN_OUT    inout;
	char     *optr = (char *)out_buffer;
	int64     num_frames_incr = 0;
	bool	  last_buffer = false;
	int32  debug_loop_count = 0;

	*out_frameCount = 0;
	needed_count = fBlocSize;
	
	acquire_sem(fDecoderSem);

	while (needed_count > 0) {
		if (debug_loop_count++ > fBlocSize)
			debugger("mpeg decoder is in an infinite loop\n");

//		printf("we still need %d bytes of decoded data\n", needed_count);
		// first check if we have some decoded data that we can use
		amt = (fDecodedDataSize - fDecodedDataIndex);
		if (amt > 0) {
			if (amt > needed_count)
				copy_count = needed_count;
			else
				copy_count = amt;

			memcpy(optr, &fDecodedData[fDecodedDataIndex], copy_count);
			//printf("copied %d data of previously decoded data\n", copy_count);
			
			fDecodedDataIndex += copy_count;
			needed_count      -= copy_count;
			optr              += copy_count;

			*out_frameCount   += copy_count / (2 * fNumChannels);	//	16 bit * channels
			num_frames_incr   += copy_count / (2 * fNumChannels);	//	16 bit * channels

			if (needed_count <= 0)   // are we all done?
				break;
		}		

		// check if we need to get more data and if so, go get it...
		amt = (fEncodedDataSize - fEncodedDataIndex);
		if (amt < 2048 && !last_buffer) {
			size_t tmp;
			const uchar *ptr;
			
            if (amt > 0)
				memmove(fEncodedData, &fEncodedData[fEncodedDataIndex], amt);
			else
				amt = 0;
			fEncodedDataIndex = 0;
			fEncodedDataSize  = amt;

			err = GetNextChunk((const void **)&ptr, &tmp, &fMediaHeader, mdi);
			if (err != B_OK) {
				if ((err == B_LAST_BUFFER_ERROR) && (*out_frameCount != 0))
					last_buffer = true;
				else {
					release_sem(fDecoderSem);
					return err;
				}
			}
			else {
				//printf("GetNextChunk read %d bytes at index %d\n", tmp, amt);
				
				memcpy(&fEncodedData[fEncodedDataSize], ptr, tmp);
				fEncodedDataSize += tmp;
			}
/* the resync code should no longer be needed because the extractor should not ever
 * give data that does not start at a valid header.  If the extractor's check gives,
 * a false positive, this check will too!
 */
//			if (fResyncNeeded) {
//				printf("resync needed !!!\n");
//				for(; fEncodedDataIndex < fEncodedDataSize; fEncodedDataIndex++)
//					if (is_mpeg_audio_header(&fEncodedData[fEncodedDataIndex],
//						fEncodedDataSize - fEncodedDataIndex, (fBitRate != 0)))
//					{
//						fResyncNeeded = false;
//						break;
//					}
//				printf("found (or not) @ %d\n", fEncodedDataIndex);
//
//				if (fResyncNeeded)
//					printf("We resync'ed but still are out of sync...\n");
//			}

			if (fMpegHeadInit == false) {
				int junk;   /* we don't care about the bitrate */
				
				// find a audio header so head_info2 doesn't puke
				if(!is_mpeg_audio_header(&fEncodedData[fEncodedDataIndex],
					fEncodedDataSize - fEncodedDataIndex, (fBitRate != 0)))
				{
					fEncodedDataIndex++;
					for(; fEncodedDataIndex < fEncodedDataSize; fEncodedDataIndex++) {
						if (is_mpeg_audio_header(&fEncodedData[fEncodedDataIndex],
							fEncodedDataSize - fEncodedDataIndex, (fBitRate != 0)))
						{
							break;
						}
					}
					// how much data do we need to have left?
					if(fEncodedDataIndex >= fEncodedDataSize) {
						printf("mpegaudio: can't find audio sync pattern\n");
						release_sem(fDecoderSem);
						return B_ERROR;
					}
					else {
						// shift the data down
						amt = fEncodedDataSize - fEncodedDataIndex;
						memmove(fEncodedData, &fEncodedData[fEncodedDataIndex], amt);
						fEncodedDataIndex = 0;
						fEncodedDataSize  = amt;
					}
				}

				fFrameBytes = head_info2(fEncodedData, fEncodedDataSize,
										 &fMpegHead, &junk);
				if (fFrameBytes == 0) {
					printf("error from head_info2() %d\n", fFrameBytes);
					release_sem(fDecoderSem);
					return B_ERROR;
				}
				fMpegHeadInit = true;

				// initialize the decoder (16 bit 44khz); see xingmp3/cup.c
				if (audio_decode_init(&fMpegHead, fFrameBytes, 0,0,0,24000) == 0) {
					printf("audio_decode_init failed!\n");
					release_sem(fDecoderSem);
					return B_ERROR;
				}
//fixme:		call audio_decode_info() to get real frame rate, channels
			}
		}

		// if we're here check if we have some encoded data to decode...
		amt = (fEncodedDataSize - fEncodedDataIndex);
		if (amt >= 2048 || last_buffer) {
			if(amt < 4)
				break;
			// check if we need to slide everything down
			amt = (fDecodedDataSize - fDecodedDataIndex);
			//printf("Moving %d bytes of decoded data\n", amt); 
			if (amt > 0 && amt < 256) {
				memmove(fDecodedData, &fDecodedData[fDecodedDataIndex], amt);
				fDecodedDataIndex = 0;
				fDecodedDataSize  = amt;
			} else {
				fDecodedDataIndex = 0;
				fDecodedDataSize  = 0;
			}
			if (is_mpeg_audio_header(&fEncodedData[fEncodedDataIndex], fEncodedDataSize - fEncodedDataIndex, (fBitRate != 0))) {
				inout = audio_decode(&fEncodedData[fEncodedDataIndex],
									 (short *)&fDecodedData[fDecodedDataSize]);
//				printf("decoded %d bytes into index %d, used %d bytes from encoded index %d\n", inout.out_bytes, fDecodedDataSize, inout.in_bytes, fEncodedDataIndex);
				fDecodedDataSize  += inout.out_bytes;
				fEncodedDataIndex += inout.in_bytes;
			}
			else {
//				printf("*************** Gack! we lost sync.... - in_bytes = %d @ %d/%d\n",inout.in_bytes, fEncodedDataIndex, fEncodedDataSize);

				inout.in_bytes = 0;
				for(; fEncodedDataIndex < fEncodedDataSize; fEncodedDataIndex++) {
					if (is_mpeg_audio_header(&fEncodedData[fEncodedDataIndex],
						fEncodedDataSize - fEncodedDataIndex, (fBitRate != 0)))
					{
//						printf("\t next header found at %d\n", fEncodedDataIndex);
						break;
					} else {
//						printf(" %02x", fEncodedData[fEncodedDataIndex]);					
					}
				}
//				printf("\n\t ended up at %d\n", fEncodedDataIndex);
				
			}
		}
	}

	*mh = fMediaHeader;
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

	fDecodedDataIndex = 0;
	fDecodedDataSize  = 0;

	fEncodedDataIndex = 0;
	fEncodedDataSize  = 0;

//	fResyncNeeded = true;

	return B_OK;
}
