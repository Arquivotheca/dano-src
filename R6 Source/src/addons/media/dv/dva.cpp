/* XXX: pull various checks out of inject/clear loops? */
/* XXX: fix channel confusion */
/* XXX: locked/unlocked mode */

#include <stdio.h>

#ifndef DEBUG
	#define DEBUG 1
#endif

#include <media/MediaDefs.h>

#include <support/ByteOrder.h>
#include <support/Debug.h>

#include "dva.h"

#define DEBUG_PREFIX "DVAudioEncoder::"
#include "debug.h"

struct dif_block
{
	uchar			id0;
	uchar			id1;
	uchar			id2;
	uchar			data[77];
} _PACKED;

struct dif_sequence
{
	dif_block		block[150];
};

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
		(f[SEQUENCE_NTSC(n,c)].block[BLOCK_NTSC(n)].data + DATA_OFF_1CH_NTSC(n))

#define POINTER_2CH_NTSC(n, c, f) \
		(f[SEQUENCE_NTSC(n,c)].block[BLOCK_NTSC(n)].data + DATA_OFF_2CH_NTSC(n))

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
		(f[SEQUENCE_PAL(n,c)].block[BLOCK_PAL(n)].data + DATA_OFF_1CH_PAL(n))

#define POINTER_2CH_PAL(n, c, f) \
		(f[SEQUENCE_PAL(n,c)].block[BLOCK_PAL(n)].data + DATA_OFF_2CH_PAL(n))

static uint16
_16_to_12(uint16 sample16)
{
	if (sample16 >= 0xfe00)
		return (sample16 >> 0) - 0xf000;
	if (sample16 >= 0xfc00)
		return (sample16 >> 1) - 0x7100;
	if (sample16 >= 0xf800)
		return (sample16 >> 2) - 0x3200;
	if (sample16 >= 0xf000)
		return (sample16 >> 3) - 0x1300;
	if (sample16 >= 0xe000)
		return (sample16 >> 4) - 0x400;
	if (sample16 >= 0xc000)
		return (sample16 >> 5) + 0x300;
	if (sample16 >= 0x8000)
		return (sample16 >> 6) + 0x600;
	if (sample16 >= 0x4000)
		return (sample16 >> 6) + 0x600;
	else if (sample16 >= 0x2000)
		return (sample16 >> 5) + 0x500;
	else if (sample16 >= 0x1000)
		return (sample16 >> 4) + 0x400;
	else if (sample16 >= 0x800)
		return (sample16 >> 3) + 0x300;
	else if (sample16 >= 0x400)
		return (sample16 >> 2) + 0x200;
	else if (sample16 >= 0x200)
		return (sample16 >> 1) + 0x100;
	else
		return (sample16 >> 1) + 0x100;
}

static void
inject_audio_12(dif_sequence *frame, int16 *audio_buffer,
                 int32 start, int32 end, bool pal, int32 endianness)
{
	uint16 *a = (uint16 *)audio_buffer;

	for (int32 n=start;n<end;n++) {
		uchar *p = pal ? POINTER_2CH_PAL(n, 0, frame) :
				POINTER_2CH_NTSC(n, 0, frame);
		uint16 sample1, sample2;

		if (endianness == B_MEDIA_BIG_ENDIAN) {
			sample1 = _16_to_12(B_BENDIAN_TO_HOST_INT32(*(a++)));
			sample2 = _16_to_12(B_BENDIAN_TO_HOST_INT32(*(a++)));
		} else {
			sample1 = _16_to_12(B_LENDIAN_TO_HOST_INT32(*(a++)));
			sample2 = _16_to_12(B_LENDIAN_TO_HOST_INT32(*(a++)));
		}

		p[0] = sample1 / 0x10;
		p[1] = sample2 / 0x10;
		p[2] = (sample1 & 0x0f) * 0x10 + (sample2 & 0xf);

		if (pal)
			p += POINTER_2CH_PAL(0, 1, frame) - POINTER_2CH_PAL(0, 0, frame);
		else
			p += POINTER_2CH_NTSC(0, 1, frame) - POINTER_2CH_NTSC(0, 0, frame);

		if (endianness == B_MEDIA_BIG_ENDIAN) {
			sample1 = _16_to_12(B_BENDIAN_TO_HOST_INT32(*(a++)));
			sample2 = _16_to_12(B_BENDIAN_TO_HOST_INT32(*(a++)));
		} else {
			sample1 = _16_to_12(B_LENDIAN_TO_HOST_INT32(*(a++)));
			sample2 = _16_to_12(B_LENDIAN_TO_HOST_INT32(*(a++)));
		}

		p[0] = sample1 / 0x10;
		p[1] = sample2 / 0x10;
		p[2] = (sample1 & 0x0f) * 0x10 + (sample2 & 0xf);
	}
}

static void
clear_audio_12(dif_sequence *frame, int32 start, int32 end, bool pal)
{
	for (int32 n = start; n < end; n+=2) {
		uchar *p = pal ? POINTER_2CH_PAL(n, 0, frame) :
				POINTER_2CH_NTSC(n, 0, frame);
		p[0] = 0x80;
		p[1] = 0x80;
		p[2] = 0;

		if (pal)
			p += POINTER_2CH_PAL(0, 1, frame) - POINTER_2CH_PAL(0, 0, frame);
		else
			p += POINTER_2CH_NTSC(0, 1, frame) - POINTER_2CH_NTSC(0, 0, frame);
		p[0] = 0x80;
		p[1] = 0x80;
		p[2] = 0;
	}
}

static void
inject_audio_16(dif_sequence *frame, int16 *audio_buffer,
                 int32 start, int32 end, bool pal, int32 endianness)
{
	uint16 *a = (uint16 *)audio_buffer;
	for (int32 n = start; n < end; n++) {
		uchar *p = pal ? POINTER_1CH_PAL(n, 0, frame) :
				POINTER_1CH_NTSC(n, 0, frame);
		if (endianness == B_MEDIA_BIG_ENDIAN) {
			p[0] = *a & 0xff;
			p[1] = *a / 0x100;
		} else {
			p[0] = *a / 0x100;
			p[1] = *a & 0xff;
		}

		a++;

		if (pal)
			p += POINTER_1CH_PAL(0, 1, frame) - POINTER_1CH_PAL(0, 0, frame);
		else
			p += POINTER_1CH_NTSC(0, 1, frame) - POINTER_1CH_NTSC(0, 0, frame);
		if (endianness == B_MEDIA_BIG_ENDIAN) {
			p[0] = *a & 0xff;
			p[1] = *a / 0x100;
		} else {
			p[0] = *a / 0x100;
			p[1] = *a & 0xff;
		}
		a++;
	}
}

static void
clear_audio_16(dif_sequence *frame, int32 start, int32 end, bool pal)
{
	for (int32 n = start; n < end; n++) {
		uchar *p = pal ? POINTER_1CH_PAL(n, 0, frame) :
				POINTER_1CH_NTSC(n, 0, frame);
		p[0] = 0x80;
		p[1] = 0;

		if (pal)
			p += POINTER_1CH_PAL(0, 1, frame) - POINTER_1CH_PAL(0, 0, frame);
		else
			p += POINTER_1CH_NTSC(0, 1, frame) - POINTER_1CH_NTSC(0, 0, frame);
		p[0] = 0x80;
		p[1] = 0;
	}
}

DVAudioEncoder::DVAudioEncoder()
{
	fInitCheck = B_NO_INIT;
}

DVAudioEncoder::DVAudioEncoder(
		uchar bits, uint32 audiorate, uchar channels)
{
	fInitCheck = B_NO_INIT;
	SetParameters(bits, audiorate, channels);
}

DVAudioEncoder::~DVAudioEncoder()
{
	if ((fInitCheck == B_OK) && fFrame)
		CompletedFrame();
}

status_t
DVAudioEncoder::SetParameters(uchar bits, uint32 audiorate, uchar channels)
{
	ASSERT((fInitCheck < B_OK) || (fFrame == NULL));

	fInitCheck = B_ERROR;
	
	if ((bits != 12) && (bits != 16)) {
		printf("Bits/sample must be 12 or 16 (not %d)\n", bits);
		goto err1;
	}
	fBitsPerSample = bits;

	if ((audiorate != 32000) && (audiorate != 44100) && (audiorate != 48000)) {
		printf("Audio rate must be 32, 44.1, or 48 kHz (not %f)\n",
				audiorate / 1000.);
		goto err1;
	}
	fAudioRate = audiorate;

	if (channels != 2) {
		if (channels != 4) {
			printf("Channels must be 2 or 4 (not %d)\n", channels);
			goto err1;
		}
		if ((audiorate != 32000) || (bits != 12)) {
			printf("Quad channel only valid in 12 bit, 32 kHz mode\n");
			goto err1;
		}
	} else if (bits == 12) {
		printf("12 bits/sample only valid with 32 kHz, quad channel audio\n");
		goto err1;
	}
	fChannels = channels;

	fFrame = NULL;
	fFrameNumber = 0;
	
	fInitCheck = B_OK;
err1:
	return fInitCheck;
}

status_t
DVAudioEncoder::InitializeFrame(void *frame, bigtime_t t)
{
	status_t err;
	bigtime_t dt;

	if (fInitCheck < B_OK)
		return fInitCheck;

	if (!frame)
		return B_BAD_VALUE;

	fFrame = (dif_sequence *)frame;

	if (fFrameNumber) {
		dt = t - fFrameAudioStartTime;
		if (	(dt < -32 * 1000000LL / fAudioRate) ||
				(dt >  32 * 1000000LL / fAudioRate)) {
			PRINTF(20, ("Resyncing audio start time (%Ld - %Ld = %Ld)\n", \
					t, fFrameAudioStartTime, dt));
			fFrameAudioStartTime = t;
		}
	} else {
		dt = 0;
		fFrameAudioStartTime = t;
	}

	if ((fFrame[0].block[0].id0 >> 5) || (fFrame[0].block[0].id1 >> 3)) {
		printf("Not a DV frame\n");
		err = EINVAL;
		goto err1;
	}

	fPAL = (fFrame[0].block[0].data[0] & 0x80) ? true : false;

	fCurrentSamples = 0;

	switch (fAudioRate) {
		case 32000 :
			fMinimumSamples = fPAL ? 1264 : 1053;
			fMaximumSamples = fPAL ? 1296 : 1080;
			fTotalFrameSamples = fPAL ? 1280 : 1067;
			break;
		case 44100 :
			fMinimumSamples = fPAL ? 1742 : 1452;
			fMaximumSamples = fPAL ? 1786 : 1489;
			fTotalFrameSamples = fPAL ? 1764 : 1471;//.47;
			break;
		case 48000 :
			fMinimumSamples = fPAL ? 1896 : 1580;
			fMaximumSamples = fPAL ? 1944 : 1620;
			fTotalFrameSamples = fPAL ? 1920 : 1601;
			break;
		default :
			ASSERT(0);
	}

	fTotalFrameSamples += dt * fAudioRate / 1000000;
	if (fTotalFrameSamples < fMinimumSamples)
		fTotalFrameSamples = fMinimumSamples;
	else if (fTotalFrameSamples > fMaximumSamples)
		fTotalFrameSamples = fMaximumSamples;

	PRINTF(20, ("dt = %Ld, fTotalFrameSamples = %ld\n", \
			dt, fTotalFrameSamples));

	return B_OK;

err1:
	fFrame = NULL;
	return err;
}

void
DVAudioEncoder::CompletedFrame()
{
	ASSERT(fInitCheck == B_OK); ASSERT(fFrame);

	for (int32 seq=0;seq<(fPAL?12:10);seq++) {
		int32 bnum = (seq & 1) ? 0 : 3;	/* p99 table 31 */
		dif_block *b = fFrame[seq].block + (6+bnum*16);
		ASSERT(b->data[0] == 0x50);
		b->data[1] = 0x80;
		if (fCurrentSamples >= fMinimumSamples) {
			ASSERT(fCurrentSamples - fMinimumSamples <= 0x3f);
			b->data[1] += fCurrentSamples - fMinimumSamples;
		}
		b->data[2] = (fChannels == 2) ? 0x00 : 0x20;
		if (seq >= (fPAL?6:5)) b->data[2] |= 1;
		b->data[3] = fPAL ? 0xe0 : 0xc0;
		b->data[4] = 0xc0;
		switch (fBitsPerSample) {
			case 12 : b->data[4] |= 1; break;
			case 16 : b->data[4] |= 0; break;
			case 20 : b->data[4] |= 2; break;
			default : ASSERT(0); break;
		}
		switch (fAudioRate) {
			case 32000 : b->data[4] |= 0x10; break;
			case 44100 : b->data[4] |= 8; break;
			case 48000 : b->data[4] |= 0; break;
			default : ASSERT(0); break;
		}
	}

	ClearAudio(fCurrentSamples, fMaximumSamples);

	fFrameAudioStartTime = fFrameAudioStartTime +
			fTotalFrameSamples * 1000000 / fAudioRate;

	fFrameNumber++;

	fFrame = NULL;
}

int32
DVAudioEncoder::AddAudioToFrame(bigtime_t t, int16 *audio, int32 num_samples,
		int32 endianness)
{
	int32 to_inject, sample;
	int32 retval;

	ASSERT(fInitCheck == B_OK); ASSERT(fFrame); ASSERT(num_samples >= 0);

	sample = (t - fFrameAudioStartTime) * fAudioRate / 1000000;

	if (sample < 0) {
		int32 f_delta = -sample + 1; // Rounding occurs in the opposite
									 // direction for negative numbers
		if (f_delta > num_samples) {
			PRINTF(10, ("Audio buffer arrived too late (%Ld < %Ld)\n", \
					t, fFrameAudioStartTime));
			return -1;
		}

		sample = 0;
		audio += fChannels * f_delta;
		num_samples -= f_delta;
	}

	if (sample != fCurrentSamples)
		PRINTF(-1, ("%ld: sample = %ld, fCurrentSamples = %ld, dt = %Ld\n", \
				fFrameNumber, sample, fCurrentSamples, t - fFrameAudioStartTime));

	/* Adjust for rounding errors */
	if ((sample == fCurrentSamples + 1) || (sample == fCurrentSamples - 1))
		sample = fCurrentSamples;

//	ASSERT(sample >= fCurrentSamples);

	if (sample < fCurrentSamples)
		sample = fCurrentSamples;

	if (sample > fCurrentSamples)
		ClearAudio(fCurrentSamples, sample);
	fCurrentSamples = sample;

	to_inject = fTotalFrameSamples - fCurrentSamples;
	retval = 1;
	if (num_samples < to_inject) {
		to_inject = num_samples;
		retval = 0;
	}

	switch (fBitsPerSample) {
		case 12 :
			inject_audio_12(fFrame, audio, fCurrentSamples,
					fCurrentSamples + to_inject, fPAL, endianness);
			break;
		case 16 :
			inject_audio_16(fFrame, audio, fCurrentSamples,
					fCurrentSamples + to_inject, fPAL, endianness);
			break;
		default :
			printf("Invalid bits/sample (%d)\n", fBitsPerSample);
			ASSERT(0);
			break;
	}

	fCurrentSamples += to_inject;

	return retval;
}

void
DVAudioEncoder::ClearAudio(int32 start, int32 end)
{
	switch (fBitsPerSample) {
		case 12 :
			clear_audio_12(fFrame, start, end, fPAL);
			break;
		case 16 :
			clear_audio_16(fFrame, start, end, fPAL);
			break;
		default :
			ASSERT(0);
			break;
	}
}
