#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/OS.h>
#include <media/MediaFormats.h>
#include <support/ByteOrder.h>
#define DEBUG 1
#include <support/Debug.h>

#include "Decoder.h"
#include "MediaTrack.h"

#include "dvaudio.h"

#define TOUCH(x) ((void)(x))

const size_t output_buffer_size = 4*1024;

media_encoded_audio_format::audio_encoding my_encoding;
static media_format mediaFormat;

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	status_t 					err;
	media_format_description	formatDescription[1];
	const int					formatDescription_count =
		sizeof(formatDescription) / sizeof(media_format_description);

	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
	mediaFormat.u.encoded_audio = media_encoded_audio_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));

	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'dv_a';

	err = formatObject.MakeFormatFor(formatDescription, formatDescription_count, &mediaFormat);
	if(err != B_NO_ERROR) {
		printf("DVAudioDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	my_encoding = mediaFormat.u.encoded_audio.encoding;
	*out_format = &mediaFormat;
	*out_count = 1;
}

Decoder *instantiate_decoder(void)
{
	if(my_encoding == 0)
		return NULL;

	return new DVAudioDecoder();
}

DVAudioDecoder::DVAudioDecoder()
{
}

DVAudioDecoder::~DVAudioDecoder()
{
}

status_t
DVAudioDecoder::GetCodecInfo(media_codec_info *mci) const
{
    strcpy(mci->pretty_name, "DV Audio");
    strcpy(mci->short_name, "dvaudio");
    return B_OK;
}

//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
DVAudioDecoder::Sniff(const media_format *in_format,
                 const void *in_info, size_t in_size)
{
	TOUCH(in_info); TOUCH(in_size);

	if (in_format->type != B_MEDIA_ENCODED_AUDIO)
		return B_BAD_TYPE;

	if (in_format->u.encoded_audio.encoding != my_encoding)
		return B_BAD_TYPE;

	fFrameFrameRate = 48000.0;
	fFrameChannelCount = 2;
	
	return B_OK;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
DVAudioDecoder::Format(media_format *inout_format)
{
	media_raw_audio_format *raf = &inout_format->u.raw_audio;

	inout_format->type = B_MEDIA_RAW_AUDIO;
	
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	raf->frame_rate = fOutputFrameRate = fFrameFrameRate;   // bad!!!
	raf->channel_count = fOutputChannelCount = fFrameChannelCount;
	raf->format = media_raw_audio_format::B_AUDIO_SHORT;
	raf->byte_order = B_MEDIA_HOST_ENDIAN;
	raf->buffer_size = output_buffer_size;

	return B_OK;
}

/* NTSC definitions */

#define TRACK_NTSC(n,c)			((((n)/3+2*((n)%3))%5) + 5*(c))
#define SYNC_BLOCK_NTSC(n)		((2+3*((n)%3))+((n)%45)/15)
#define BYTE_POS_1CH_NTSC(n)	(10+2*((n)/45))
#define BYTE_POS_2CH_NTSC(n)	(10+3*((n)/45))

#define SEQUENCE_NTSC(n,c)		TRACK_NTSC(n,c)
#define BLOCK_NTSC(n)			(6 + (SYNC_BLOCK_NTSC(n) - 2) * 16)
#define DATA_OFF_1CH_NTSC(n)	(BYTE_POS_1CH_NTSC(n)-5)
#define DATA_OFF_2CH_NTSC(n)	(BYTE_POS_2CH_NTSC(n)-5)

#define POINTER_1CH_NTSC(n, c, f) \
		(f->sequence[SEQUENCE_NTSC(n,c)].block[BLOCK_NTSC(n)].data + DATA_OFF_1CH_NTSC(n))

#define POINTER_2CH_NTSC(n, c, f) \
		(f->sequence[SEQUENCE_NTSC(n,c)].block[BLOCK_NTSC(n)].data + DATA_OFF_2CH_NTSC(n))

/* PAL definitions */

#define TRACK_PAL(n,c)		((((n)/3+2*((n)%3))%6) + 6*(c))
#define SYNC_BLOCK_PAL(n)	((2+3*((n)%3))+((n)%54)/18)
#define BYTE_POS_1CH_PAL(n)	(10+2*((n)/54))
#define BYTE_POS_2CH_PAL(n)	(10+3*((n)/54))

#define SEQUENCE_PAL(n,c)	TRACK_PAL(n,c)
#define BLOCK_PAL(n)		(6 + (SYNC_BLOCK_PAL(n) - 2) * 16)
#define DATA_OFF_1CH_PAL(n)	(BYTE_POS_1CH_PAL(n)-5)
#define DATA_OFF_2CH_PAL(n)	(BYTE_POS_2CH_PAL(n)-5)

#define POINTER_1CH_PAL(n, c, f) \
		(f->sequence[SEQUENCE_PAL(n,c)].block[BLOCK_PAL(n)].data + DATA_OFF_1CH_PAL(n))

#define POINTER_2CH_PAL(n, c, f) \
		(f->sequence[SEQUENCE_PAL(n,c)].block[BLOCK_PAL(n)].data + DATA_OFF_2CH_PAL(n))

uint16
_12_to_16(uint16 sample12)
{
	if (sample12 >= 0xe00)
		return sample12 | 0xf000;
	else if (sample12 >= 0xd00)
		return (((sample12 + 257) << 1) | 0xe000) - 1;
	else if (sample12 >= 0xc00)
		return (((sample12 + 513) << 2) | 0xc000) - 1;
	else if (sample12 >= 0xb00)
		return (((sample12 + 769) << 3) | 0x8000) - 1;
	else if (sample12 >= 0xa00)
		return (((sample12 + 1025) << 4) | 0x8000) - 1;
	else if (sample12 >= 0x900)
		return (((sample12 + 1281) << 5) | 0x8000) - 1;
	else if (sample12 >= 0x800)
		return (((sample12 + 1537) << 6) | 0x8000) - 1;
	else if (sample12 >= 0x700)
		return (sample12 - 1536) << 6;
	else if (sample12 >= 0x600)
		return (sample12 - 1280) << 5;
	else if (sample12 >= 0x500)
		return (sample12 - 1024) << 4;
	else if (sample12 >= 0x400)
		return (sample12 - 768) << 3;
	else if (sample12 >= 0x300)
		return (sample12 - 512) << 2;
	else if (sample12 >= 0x200)
		return (sample12 - 256) << 1;
	else
		return sample12;
}

int32
extract_audio_12(dv_frame *frame, int16 *audio_buffer,
                 int32 start, int32 end, bool PAL)
{
	uint16 *a = (uint16 *)audio_buffer;

	for (int32 n=start;n<end;n++) {
		uchar *p;
		int16 sample1, sample2;

		p = PAL ? POINTER_2CH_PAL(n, 0, frame) : POINTER_2CH_NTSC(n, 0, frame);
		sample1 = p[0] * 0x10 + (p[2] >> 4);
		sample2 = p[1] * 0x10 + (p[2] & 0xf);

		if ((sample1 == 0x800) || (sample2 == 0x800)) {
//			printf("sample %ld is bad (%x %x)\n", n, sample1, sample2);
			continue;
		}

		*(a++) = _12_to_16(sample1);
		*(a++) = _12_to_16(sample2);
	}

	return (a - (uint16 *)audio_buffer) / 2;
}

static int32
extract_audio_16(dv_frame *frame, int16 *audio_buffer,
                 int32 start, int32 end, bool PAL)
{
	uint16 *a = (uint16 *)audio_buffer;

	for (int32 n = start; n < end; n++) {
		uchar *p;
		uint16 l, r;

		p = PAL ? POINTER_1CH_PAL(n, 0, frame) : POINTER_1CH_NTSC(n, 0, frame);
		l = p[0] * 0x100 + p[1];

//		p = PAL ? POINTER_1CH_PAL(n, 1, frame) : POINTER_1CH_NTSC(n, 1, frame);
		if (PAL)
			p += POINTER_1CH_PAL(0, 1, frame) - POINTER_1CH_PAL(0, 0, frame);
		else
			p += POINTER_1CH_NTSC(0, 1, frame) - POINTER_1CH_NTSC(0, 0, frame);
		r = p[0] * 0x100 + p[1];

		if ((l == 0x8000) || (r == 0x8000)) {
//			printf("sample %ld is bad (%x %x)\n", n, l, r);
			continue;
		}

		*(a++) = l;
		*(a++) = r;
	}

	return (a - (uint16 *)audio_buffer) / 2;
}

static status_t
extract_aaux_source(
		dv_frame *frame, media_format *fmt, int32 *spf, int32 *quant, bool *PAL)
{
	// some givens
	fmt->type = B_MEDIA_RAW_AUDIO;
	fmt->u.raw_audio = media_raw_audio_format::wildcard;
	fmt->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	fmt->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
		
	dif_block *b = &frame->sequence[0].block[6+3*16];
	if (b->data[0] != 0x50) {
		printf("AAUX block not found\n");
		return B_ERROR;
	}

	int base = 0;
	*PAL = ((b->data[3] & 0x20) != 0);
				
	// sample rate
	switch((b->data[4] >> 3) & 0x7) {
		case 0:
			fmt->u.raw_audio.frame_rate = 48000.0;
			base = *PAL ? 1896 : 1580;
			break;
		case 1:
			fmt->u.raw_audio.frame_rate = 44100.0;
			base = *PAL ? 1742 : 1452;
			break;
		case 2:
			fmt->u.raw_audio.frame_rate = 32000.0;
			base = *PAL ? 1264 : 1053;
			break;
		default:
			printf("sample rate is 0x%x???\n", b->data[4] & 0x38);
			return B_ERROR;
	}
//printf("sample rate: %g\n", fmt->u.raw_audio.frame_rate);
	*spf = base + (b->data[1] & 0x3f);

	// audio quantization (sample format)
	*quant = (b->data[4] & 0x7);

	// XXX: always 2 until mixer can handle 4 channel audio
	fmt->u.raw_audio.channel_count = 2;
//printf("channel_count: %d\n", fmt->u.raw_audio.channel_count);
	return B_OK;
}

status_t 
DVAudioDecoder::Reset(int32 in_towhat, int64 in_requiredFrame, int64 *inout_frame, bigtime_t in_requiredTime, bigtime_t *inout_time)
{
	TOUCH(in_towhat); TOUCH(in_requiredFrame); TOUCH(inout_frame);
	TOUCH(in_requiredTime); TOUCH(inout_time);

	fFrame = NULL;
	fFirstSample = 0;
	fLastSample = 0;
	return B_NO_ERROR;
}

//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 
status_t
DVAudioDecoder::Decode(void *output, int64 *frame_count, media_header *mh,
                  media_decode_info *info)
{
	status_t err = B_BAD_VALUE;
	
	int64 request_frames = *frame_count;
	int64 frames = 0;

	int16 *audio = (int16 *)output;

	//printf("request %Ld audio frames\n", request_frames);
	
	request_frames = output_buffer_size / sizeof(int16) / fOutputChannelCount;
	
	while (frames < request_frames) {
		if(fFrame == NULL) {
			const void *compressed_data;
			size_t size;
			err = GetNextChunk(&compressed_data, &size, mh, info);
			if(err != B_NO_ERROR)
				break;

			ASSERT(compressed_data != NULL);

			if ((size != 120000) && (size != 144000)) {
				printf("DVAudio: bad frame size: %ld\n", size);
				err = B_ERROR;
				break;
			}

			fFrame = (dv_frame *)compressed_data;

			media_format mf;
			int32 spf;
			err = extract_aaux_source(fFrame, &mf, &spf, &fQuant, &fPAL);
			if (err < B_OK) {
				//printf("extract AAUX source failed: 0x%x (%s)\n", err, strerror(err));
				fFrame = NULL;
				break;
			}
			fFrameChannelCount = mf.u.raw_audio.channel_count;
			fFrameFrameRate = mf.u.raw_audio.frame_rate;
			fFirstSample = 0;
			fLastSample = spf;
		}
		if(fFrameChannelCount != fOutputChannelCount ||
		   fFrameFrameRate != fOutputFrameRate) {
			const char *compression = "unknown";
			if(fQuant == LINEAR_16)
				compression = "16-bit linear";
			else if(fQuant == NONLINEAR_12)
				compression = "12-bit nonlinear";
			printf("DV audio format changed to %ld channel %s %g kHz\n",
				fFrameChannelCount, compression, fFrameFrameRate / 1000);
			err = B_MEDIA_BAD_FORMAT;
			break;
		}
		
		int32 last_sample = fLastSample;
		if(request_frames - frames < fLastSample - fFirstSample)
			last_sample = fFirstSample + request_frames - frames;
		int32 got_frames;
		//printf("extract audio %d to %d from %p\n", fFirstSample, last_sample, fFrame);
		switch(fQuant) {
		case LINEAR_16:
			got_frames = extract_audio_16(fFrame, audio, fFirstSample, last_sample, fPAL);
			break;
	
		case NONLINEAR_12:
			got_frames = extract_audio_12(fFrame, audio, fFirstSample, last_sample, fPAL);
			break;
			
		default:
			fFrame = NULL;
			printf("can only handle 16-bit linear or 12-bit nonlinear, quant=0x%lx\n", fQuant);
			got_frames = last_sample - fFirstSample;
			//printf("filling buffer size %d (from %d to %d)\n", got_frames,
			//       fFirstSample, last_sample);
			memset(audio, 0, got_frames * fFrameChannelCount * sizeof(int16));
		}
		audio += got_frames * fFrameChannelCount;
		frames += got_frames;
		//printf("got frames %d, total %Ld of %Ld\n", got_frames, frames, request_frames);
		
		if(last_sample < fLastSample) {
			fFirstSample = last_sample;
		}
		else {
			fFrame = 0;
		}
	}

	//printf("frame count %d\n", frames);
	*frame_count = (int64)frames;
	if(frames <= 0)
		return err;
	return B_NO_ERROR;
}
