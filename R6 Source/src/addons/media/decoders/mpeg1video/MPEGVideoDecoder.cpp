#include <ByteOrder.h>
#include <MediaTrack.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>

#include <stdio.h>
#include <assert.h>

#include "MPEGVideoDecoder.h"
#include "MPEGyuv2rgb.h"
#include "global.h"
#include "ycbcr_mmx.h"

//#define DEBUG printf
#define DEBUG if (0) printf

static int YCbCr2RGB32(unsigned char *buffer,int width,unsigned char *ys,
	unsigned char *us,unsigned char *vs);
static int YCbCr2YCbCr(unsigned char *buffer,int width,unsigned char *ys,
	unsigned char *us,unsigned char *vs);

media_encoded_video_format::video_encoding mpeg1_encoding;
static media_format mediaFormat;



void register_decoder(const media_format ** out_format, int32 * out_count)
{
	DEBUG("MPEGVideoDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[1];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_MPEG_FORMAT_FAMILY;
	formatDescription[0].u.mpeg.id = B_MPEG_1_VIDEO;
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat);
	if(err != B_NO_ERROR) {
		DEBUG("MPEGVideoDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	mpeg1_encoding = mediaFormat.u.encoded_video.encoding;
	*out_format = &mediaFormat;
	*out_count = 1;
};

Decoder* instantiate_decoder() 
{
	if(mpeg1_encoding == 0)
		return NULL;
	return new MPEGVideoDecoder();
}


MPEGVideoDecoder::MPEGVideoDecoder()
	:	fDecoderState(NULL),
		fFrame(-1),
		fInputChunk(NULL),
		fInputChunkLength(0),
		fInputChunkOffset(0),
		fDecodeInfo(NULL),
		decodeBuf(NULL),
		decodeBufRef(NULL)
{
}

MPEGVideoDecoder::~MPEGVideoDecoder()
{
	if (fDecoderState) {
		free(decodeBufRef);
		Deinitialize_Sequence(fDecoderState);
		deleteMVideo(fDecoderState);
	}
}


status_t
MPEGVideoDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "MPEG-1 Compression");
	strcpy(mci->short_name, "mpeg1");
	return B_OK;
}


status_t MPEGVideoDecoder::Sniff(const media_format *mf, const void*, size_t)
{
	if (mf->type == B_MEDIA_ENCODED_VIDEO &&
		mf->u.encoded_video.encoding == mpeg1_encoding) {
		fOutputFormat = mf->u.encoded_video.output;
		return B_OK;
	}

	return B_BAD_TYPE;
}

status_t MPEGVideoDecoder::Format(media_format *inout_format)
{
	inout_format->type = B_MEDIA_RAW_VIDEO;
	inout_format->require_flags = 0;
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	if(inout_format->u.raw_video.display.format==B_YCbCr422)
	{
		fOutputFormat.display.format = B_YCbCr422;
		fUseYUV=true;
	}
	else
	{
		fOutputFormat.display.format = B_RGB32;
		fUseYUV=false;
	}
	// copy bytes_per_row from input
	fOutputFormat.display.bytes_per_row = inout_format->u.raw_video.display.bytes_per_row;
	inout_format->u.raw_video = fOutputFormat;

	return B_OK;
}

status_t MPEGVideoDecoder::InitializeDecoder(void)
{
	uint32 add,off;
	fDecoderState = newMVideo(this, StaticStreamRead);
	if (!fDecoderState)
		return ENOMEM;

	if (checkVheader(fDecoderState) != 0) {
		printf("Problem with mpeg video header!\n");
		goto err1;
	}

	/* need to call this once to move to the first picture */
	if (Get_Hdr(fDecoderState) != 1) {
		printf("Get_Hdr failed while initializing decoder\n");
		// Handle this somehow...
		goto err1;
	}

	Initialize_Sequence(fDecoderState);

	fWidth = fDecoderState->horizontal_size;
	fHeight = fDecoderState->vertical_size;
	fFrameRate= fDecoderState->frame_rate;	

	decodeBufRef = (uchar *)malloc(3 * fWidth * fHeight + 8);
	if (!decodeBufRef)
		goto err2;
	
	add =(uint32) decodeBufRef;
	off = (add + 7) / 8;
	off = off * 8 - add;
	
	decodeBuf = decodeBufRef + off;

	Decode_Picture(fDecoderState);

	return B_OK;

//err3:
//	free(decodeBuf);	
err2:
	Deinitialize_Sequence(fDecoderState);
err1:
	deleteMVideo(fDecoderState);
	fDecoderState = NULL;
	return B_ERROR;
}

status_t MPEGVideoDecoder::Decode(void *out_buffer, int64 *out_frameCount,
								  media_header *mh, media_decode_info *info)
{
	assert(out_buffer != 0);

//	DEBUG("MPEGVideoDecoder::Decode\n");

	//alignment of out_buffer on 64 bits;
	
	uint32 add,off;
	add = (uint32) out_buffer;
	off = (add + 7) / 8;
	off = off *8 - add;
	if(off != 0)
		printf("MPEGVideoDecoder::Decode out_buffer not aligned for MMX instructions \n");


	*out_frameCount = 1;
	fDecodeInfo = info;
	if (fDecoderState == NULL) {
		status_t err;
		err = InitializeDecoder();
		if (err < B_OK) {
			printf("Error initializing decoder\n");
			return err;
		}
	}

	// resync fFrame if it's too off-base

	if (fFrame < 0 ||
		fFrame+1 < fDecoderState->Ref_Framenum ||
		fFrame-1 > fDecoderState->Ref_Framenum)
	{
		fFrame = (fDecoderState->Ref_Framenum > fDecoderState->True_Framenum) ?
				fDecoderState->Ref_Framenum : fDecoderState->True_Framenum;;
	}

	if (fFrame == fDecoderState->Ref_Framenum)
		WriteFrame(fDecoderState->backward_reference_frame, (uchar *)decodeBuf);
	else if (fFrame == fDecoderState->True_Framenum)
		WriteFrame(fDecoderState->current_frame, (uchar *)decodeBuf);

	// can wait here for a long time
	if (Get_Hdr(fDecoderState) != 1) {
		printf("Couldn't get video header\n");
		return B_ERROR;
	}

	Decode_Picture(fDecoderState);
	if (fFrame == fDecoderState->True_Framenum)
		WriteFrame(fDecoderState->auxframe, (uchar *)decodeBuf);

	fFrame++;

	int32 bytesperrow = fOutputFormat.display.bytes_per_row;
	if(!fUseYUV)
	{
		/*
		uint8 *dptr = (uint8*)out_buffer ;
		uint8 *yptr = (uint8*) &decodeBuf[0];
		uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) ];
		uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) ];
		
		
		
		yuv2rgb(dptr,yptr,uptr,vptr,fWidth,fHeight,4*fWidth);
		*/
		
		for (int32 y = 0; y < fHeight; y++) {
			uint8 *dptr = (uint8*)out_buffer + (y * bytesperrow);
			uint8 *yptr = (uint8*) &decodeBuf[y * fWidth];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) + ((y >> 1) *
				(fWidth >> 1))];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) +
				((y >> 1) * (fWidth >> 1))];
		
			YCbCr2RGB32(dptr, fWidth, yptr, uptr, vptr);
		}
	}
	else
	{
		
		/*
		for (int32 y = 0; y < fHeight; y++) {
			uint8 *dptr = (uint8*)out_buffer + (y * bytesperrow);
			uint8 *yptr = (uint8*) &decodeBuf[y * fWidth];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) + ((y >> 1) *
				(fWidth >> 1))];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) +
				((y >> 1) * (fWidth >> 1))];
			//if(fUseYUV)
			YCbCr2YCbCr(dptr, fWidth, yptr, uptr, vptr);
		}
		*/
		uint8 *dptr = (uint8*)out_buffer ;
		uint8 *yptr = (uint8*) &decodeBuf[0];
		uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) ];
		uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2)];
		
		
		ycbcr_mmx_2(dptr,yptr,uptr,vptr,fHeight,fWidth, bytesperrow-fWidth*2);
		
		
	}
	

	*mh = fMH;
	mh->u.raw_video.field_sequence = fDecoderState->picture_num;
	mh->start_time = (bigtime_t)(1e6 * mh->u.raw_video.field_sequence / fDecoderState->frame_rate);

	fDecoderState->picture_num++;

	return B_OK;
}

status_t MPEGVideoDecoder::Reset(int32 in_towhat, int64 in_requiredFrame,
		int64 *inout_frame, bigtime_t in_requiredTime, bigtime_t *inout_time)
{
	printf("MPEGVideoDecoder::Reset\n");
	if (fDecoderState) {
		fInputChunkOffset = fInputChunkLength + 1;
		Initialize_Buffer(fDecoderState);

		fDecoderState->picture_num = in_requiredFrame;
		fDecoderState->timebase.baseframe = fDecoderState->picture_num;
		fDecoderState->timebase.t = (int64)(1e6 * fDecoderState->timebase.baseframe / fDecoderState->frame_rate);
	
		Get_Hdr(fDecoderState);
		Decode_Picture(fDecoderState);
		
	
	}

	fFrame = -1;

	return B_OK;
}

int MPEGVideoDecoder::StaticStreamRead(void *decoder, void *data, int len)
{
	return ((MPEGVideoDecoder*) decoder)->StreamRead(data, len);
}

int MPEGVideoDecoder::StreamRead(void *data, size_t len)
{
//	DEBUG("MPEGVideoDecoder::StreamRead %x\n", len);

	status_t err;
	char *bufptr = (char*) data;
	size_t copied = 0;
	while (copied < len) {
		if (fInputChunkOffset >= fInputChunkLength) {
			err = GetNextChunk(&fInputChunk, &fInputChunkLength, &fMH, fDecodeInfo);
			if (err < B_OK) {
				printf("StreamRead failed.\n");
				return err;
			}
				
			fInputChunkOffset = 0;
		}

		int sizeToCopy =
				((len - copied) > (fInputChunkLength - fInputChunkOffset)) ?
				(fInputChunkLength - fInputChunkOffset) : (len - copied);
		memcpy(bufptr, (uint8*)fInputChunk+fInputChunkOffset, sizeToCopy);
		fInputChunkOffset += sizeToCopy;
		copied += sizeToCopy;
		bufptr += sizeToCopy;
	}
	
	return len;
}


void MPEGVideoDecoder::WriteFrame(unsigned char *src[], unsigned char *buffer)
{
	int i;
	int incr = fDecoderState->Coded_Width;
	int w = fDecoderState->Coded_Width >> 1;
	int h = fDecoderState->vertical_size >> 1;
	int w2 = fDecoderState->horizontal_size >> 1;

	assert(buffer);
	
	// write the first frame's y, the u, and v, and then the 2nd frame's y
	for (i = 0; i < fDecoderState->vertical_size; i++) {
		unsigned char *py = src[0] + incr*(i+0);
		memcpy(buffer,py,fDecoderState->horizontal_size);
		buffer+=fDecoderState->horizontal_size;
	}

	if (fDecoderState->horizontal_size == fDecoderState->Coded_Width) {
		memcpy(buffer,src[1],w*h);
		buffer+=(w*h);
		memcpy(buffer,src[2],w*h);
		buffer+=(w*h);
	} else {
		unsigned char *py = src[1];
		for (i = 0; i < h; i++) {
			memcpy(buffer,py,w2);
			buffer += w2;
			py += w;
		}

		py = src[2];
		for (i = 0; i < h; i++) {
			memcpy(buffer, py, w2);
			buffer += w2;
			py += w;
		}
	}
}

#define SHIFT_BITS 8

extern int32 LUT_1_164[0x100];
extern int32 LUT_1_596[0x100];
extern int32 LUT_0_813[0x100];
extern int32 LUT_0_392[0x100];
extern int32 LUT_2_017[0x100];

uchar fixed32toclipped8(int32 fixed)
{
	if (fixed <= 0)
		return 0;
	else if (fixed >= (255 << SHIFT_BITS))
		return 255;
	else
		return (fixed + (1 << (SHIFT_BITS - 1))) >> SHIFT_BITS;
}

inline uint32 ycbcr_to_rgb(uchar y, uchar cb, uchar cr)
{
	int32 Y;
	uchar red, green, blue;

/*
	red = clip8(1.164 * (y - 16) + 1.596 * (cr - 128));
	green = clip8(1.164 * (y - 16) - 0.813 * (cr - 128) - 0.392 * (cb - 128));
	blue = clip8(1.164 * (y - 16) + 2.017 * (cb - 128));
*/

	Y = LUT_1_164[y];

	red =	fixed32toclipped8(Y + LUT_1_596[cr]);
	green =	fixed32toclipped8(Y - LUT_0_813[cr] - LUT_0_392[cb]);
	blue =	fixed32toclipped8(Y + LUT_2_017[cb]);

#if B_HOST_IS_LENDIAN
	return (blue | (green << 8) | (red << 16));
#else
	return ((red << 8) | (green << 16) | (blue << 24));
#endif
}

static int YCbCr2RGB32(unsigned char *buffer,int width,unsigned char *ys,
	unsigned char *us,unsigned char *vs)
{
	assert(buffer != 0);
	assert(ys != 0);
	assert(us != 0);
	assert(vs != 0);

	unsigned char *py=ys;
	unsigned char *pu=us;
	unsigned char *pv=vs;
	unsigned int *p2 = (unsigned int*)buffer;

	for (;width>1; width-=2) {
		uchar y,cb,cr;
		
		cb = *pu++;
		cr = *pv++;
		y = *py++;
		*p2++ = ycbcr_to_rgb(y, cb, cr);
		y = *py++;
		*p2++ = ycbcr_to_rgb(y, cb, cr);
	}

	return 0;
}

static int YCbCr2YCbCr(unsigned char *buffer,int width,unsigned char *ys,
	unsigned char *us,unsigned char *vs)
{
	assert(buffer != 0);
	assert(ys != 0);
	assert(us != 0);
	assert(vs != 0);

	unsigned char *py=ys;
	unsigned char *pu=us;
	unsigned char *pv=vs;
	unsigned char *p2 = buffer;

	for (;width>1; width-=2) {
		uchar y,cb,cr;
		
		cb = *pu++;
		cr = *pv++;
		y = *py++;
		*p2++ = y;
		*p2++ = cb;
		y = *py++;
		*p2++ = y;
		*p2++ = cr;
	}

	return 0;
}
