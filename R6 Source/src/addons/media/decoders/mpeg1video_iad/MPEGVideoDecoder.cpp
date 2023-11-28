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
static media_format_description	formatDescription[1];

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	switch(*cookie) {
	case 0:
		*otype = B_MEDIA_ENCODED_VIDEO;

		formatDescription[0].family = B_MPEG_FORMAT_FAMILY;
		formatDescription[0].u.mpeg.id = B_MPEG_1_VIDEO;

		*odesc = formatDescription;
		*ocount = 1;
		break;
	default:
		return B_BAD_INDEX;
	}

	(*cookie)++;

	return B_OK;
}

void
register_decoder(const media_format ** out_format, int32 * out_count)
{
	const media_format_description *desc;
	BMediaFormats formatObject;
	media_type type;
	status_t err;
	int32 count;
	int32 cookie;

	if(out_format == NULL || out_count == NULL)
		return;

	*out_format = NULL;
	*out_count = 0;

	cookie = 0;
	err = get_next_description(&cookie, &type, &desc, &count);
	if(err != B_OK) {
		return;
	}
	
	mediaFormat.type = type;
	memset(&mediaFormat.u, 0, sizeof(mediaFormat.u));

	err = formatObject.MakeFormatFor(desc, count, &mediaFormat);
	if(err != B_OK) {
		//printf("MPEG1 Video Decoder: MakeFormatFor failed, %s\n", strerror(err));
		return;
	}
	mpeg1_encoding = mediaFormat.u.encoded_video.encoding;

	*out_format = &mediaFormat;
	*out_count = 1;
}

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
	printf("MPEGVideoDecoder::~MPEGVideoDecoder\n");
	if (fDecoderState) {
		printf("begin deleting \n");
		printf("decodeBufRef%p\n",decodeBufRef);
		printf("decodeBuf%p\n",decodeBuf);
		free(decodeBufRef);
		printf("free(decodeBufRef)\n");
		
		Deinitialize_Sequence(fDecoderState);
		printf("Deinitialize_Sequence(fDecoderState)\n");
		
		
		deleteMVideo(fDecoderState);
		printf("deleteMVideo(fDecoderState)\n");
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
	
	printf("MPEGVideoDecoder::InitializeDecoder\n");
	
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
	
	
	//fWidth = fDecoderState->Coded_Width;
	//fHeight = fDecoderState->Coded_Height;
	
	fFrameRate= fDecoderState->frame_rate;	


	//printf("%ld %ld\n",fHeight,fWidth);
	decodeBufRef = (uchar *)malloc(3 * fWidth * fHeight + 8);
	if (!decodeBufRef)
	{
		printf("decodeBufRef error\n");
		goto err2;
	}
	//printf("decodeBufRef%p\n",decodeBufRef);
	
	add =(uint32) decodeBufRef;
	off = (add + 7) / 8;
	off = off * 8 - add;
	
	decodeBuf = decodeBufRef + off;
	//printf("decodeBuf%p\n",decodeBuf);

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

//printf("MPEGVideoDecoder::Decode\n");

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

	//printf("Decode_Picture(fDecoderState)\n");
	Decode_Picture(fDecoderState);
	if (fFrame == fDecoderState->True_Framenum)
		WriteFrame(fDecoderState->auxframe, (uchar *)decodeBuf);

	fFrame++;

	int32 bytesperrow = fOutputFormat.display.bytes_per_row;
	if(!fUseYUV)
	{
		/*
		for (int32 y = 0; y < fWidth; y++) {
			uint8 *dptr = (uint8*)out_buffer + (y * fHeight * (fUseYUV?2:4));
			uint8 *yptr = (uint8*) &decodeBuf[y * fHeight];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) + ((y >> 1) *
				(fHeight >> 1))];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) +
				((y >> 1) * (fHeight >> 1))];
			if(fUseYUV)
				YCbCr2RGB32(dptr, fHeight, yptr, uptr, vptr);
		}
		*/
		
		uint8 *dptr = (uint8*)out_buffer ;
		uint8 *yptr = (uint8*) &decodeBuf[0];
		uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) ];
		uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) ];
		
		
		#if ROTATE_DISPLAY
		yuv2rgb(dptr,yptr,uptr,vptr,fHeight,fWidth,4*fHeight);		// vertical direction is not padded
		#else
		yuv2rgb(dptr,yptr,uptr,vptr,fWidth,fHeight,bytesperrow);	// horizontal might be
		#endif
		
	}
	else
	{
		/*
		for (int32 y = 0; y < fHeight; y++) {
			uint8 *dptr = (uint8*)out_buffer + (y * fWidth * (fUseYUV?2:4));
			uint8 *yptr = (uint8*) &decodeBuf[y * fWidth];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) + ((y >> 1) *
				(fWidth >> 1))];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) +
				((y >> 1) * (fWidth >> 1))];
			if(fUseYUV)
				YCbCr2YCbCr(dptr, fWidth, yptr, uptr, vptr);
		}
		*/
		
		
		
		
		//printf("begin conversion\n");
		#if ROTATE_DISPLAY
		if((fHeight%16)==0)
		{
			uint8 *dptr = (uint8*)out_buffer ;
			uint8 *yptr = (uint8*) &decodeBuf[0];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) ];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2)];
			ycbcr_mmx_2(dptr,yptr,uptr,vptr,fWidth,fHeight, 0);		// no padding when rotated
		}
		else
		{
			for (int32 y = 0; y < fWidth; y++) {
			uint8 *dptr = (uint8*)out_buffer + (y * fHeight * 2);
			uint8 *yptr = (uint8*) &decodeBuf[y * fHeight];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) + ((y >> 1) *
				(fHeight >> 1))];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) +
				((y >> 1) * (fHeight >> 1))];
			YCbCr2YCbCr(dptr, fHeight, yptr, uptr, vptr);
			}
		}
		//printf("end conversion\n");
		#else
		if((fWidth%16)==0)
		{
			uint8 *dptr = (uint8*)out_buffer ;
			uint8 *yptr = (uint8*) &decodeBuf[0];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) ];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2)];
			ycbcr_mmx_2(dptr,yptr,uptr,vptr,fHeight,fWidth,bytesperrow-fWidth*2);
		}
		else
		{
			for (int32 y = 0; y < fHeight; y++) {
			uint8 *dptr = (uint8*)out_buffer + (y * bytesperrow);
			uint8 *yptr = (uint8*) &decodeBuf[y * fWidth];
			uint8 *uptr = (uint8*) &decodeBuf[(fWidth * fHeight) + ((y >> 1) *
				(fWidth >> 1))];
			uint8 *vptr = (uint8*) &decodeBuf[((fWidth * fHeight * 5) >> 2) +
				((y >> 1) * (fWidth >> 1))];
				YCbCr2YCbCr(dptr, fWidth, yptr, uptr, vptr);
			}
		}
		#endif
	
	}
	

	*mh = fMH;
	mh->u.raw_video.field_sequence = fDecoderState->picture_num;
	mh->start_time = (bigtime_t)(1e6 * mh->u.raw_video.field_sequence / fDecoderState->frame_rate);

	fDecoderState->picture_num++;
	//printf("end Decode\n");
	return B_OK;
}

status_t MPEGVideoDecoder::Reset(int32 in_towhat, int64 in_requiredFrame,
		int64 *inout_frame, bigtime_t in_requiredTime, bigtime_t *inout_time)
{
	//printf("MPEGVideoDecoder::Reset\n");
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
//printf("MPEGVideoDecoder::StreamRead %x\n", len);

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
	#if ROTATE_DISPLAY
	int i;
	int incr1 = fDecoderState->Coded_Height;
	int incr2 = fDecoderState->Coded_Height >> 1;
	int h1 = fDecoderState->horizontal_size ;
	int h2 = fDecoderState->horizontal_size >> 1;
	int w1 = fDecoderState->vertical_size;
	int w2 = fDecoderState->vertical_size >> 1;

	unsigned char * pb = buffer;

	int delta = incr1 - w1;

	for (i = 0; i < h1; i++) {
		unsigned char *py = src[0] + incr1*(i+0) + delta;
		memcpy(buffer,py,w1);
		buffer+=w1;
	}
	
	delta = delta / 2;
	
	for (i = 0; i < h2; i++) {
		unsigned char *py = src[1] + incr2*(i+0) + delta;
		memcpy(buffer,py,w2);
		buffer+=w2;
	}
	
	for (i = 0; i < h2; i++) {
		unsigned char *py = src[2] + incr2*(i+0) + delta;
		memcpy(buffer,py,w2);
		buffer+=w2;
	}
	/*
	
	for(i=0;i<h1*w1;i++)
	{	
		*pb = 0;
		pb ++;
	}
	
	for(i=0;i<h2*w2;i++)
	{
		*pb =150;
		pb ++;
	}
	
	for(i=0;i<h2*w2;i++)
	{
		*pb =150;
		pb ++;
	}
	
	*/
	
	
	

	#else
	int i;
	int incr = fDecoderState->Coded_Width;
	int w = fDecoderState->Coded_Width >> 1;
	int h = fDecoderState->vertical_size ;
	int h2 = fDecoderState->vertical_size >> 1;
	int w1 = fDecoderState->horizontal_size;
	int w2 = fDecoderState->horizontal_size >> 1;
	
	for (i = 0; i < h; i++) {
		unsigned char *py = src[0] + incr*(i+0);
		memcpy(buffer,py,w1);
		buffer+=w1;
	}

	if (w1 == incr) {
		memcpy(buffer,src[1],w*h2);
		buffer+=(w*h2);
		memcpy(buffer,src[2],w*h2);
		buffer+=(w*h2);
	} else {
		unsigned char *py = src[1];
		for (i = 0; i < h2; i++) {
			memcpy(buffer,py,w2);
			buffer += w2;
			py += w;
		}

		py = src[2];
		for (i = 0; i < h2; i++) {
			memcpy(buffer, py, w2);
			buffer += w2;
			py += w;
		}
	}
	#endif
	assert(buffer);
	
	
	//printf("MPEGVideoDecoder::WriteFrame \n");
	// write the first frame's y, the u, and v, and then the 2nd frame's y
	
	//printf("MPEGVideoDecoder::WriteFrame %ld %ld\n",fDecoderState->Coded_Width,fDecoderState->horizontal_size);
	//printf("MPEGVideoDecoder::WriteFrame %ld %ld\n",fDecoderState->Coded_Height,fDecoderState->vertical_size);
	
	
	
	
	/*
	int size =  fDecoderState->Coded_Width*fDecoderState->Coded_Height;
	unsigned char *py = src[0];
	memcpy(buffer,py,size);
	
	buffer += size;
	py = src[1];
	memcpy(buffer,py,size/4);
	
	buffer += size/4;
	py = src[2];
	memcpy(buffer,py,size/4);
	*/
	
	
	
	//printf("MPEGVideoDecoder::WriteFrame end\n");
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
