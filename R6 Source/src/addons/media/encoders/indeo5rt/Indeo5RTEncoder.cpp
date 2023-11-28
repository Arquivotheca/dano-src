
#include "Indeo5RTEncoder.h"

#include <stdio.h>
#include <malloc.h>

#define BUFFER_CHECK 0

typedef uint32 U32;

#include "convtabs.c"
#include "toyconvtab.c"

static void rgb32toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);
static void rgb24toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);
static void rgb16toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);
static void rgb15toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);
static void ycbcr411toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);
static void ycbcr422toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);
static void y8toyuv9(const uint8 *InputPixels, uint8 *Yin, uint8 *U, uint8 *V, int width, int height);

media_encoded_video_format::video_encoding my_encoding;

void register_encoder(void)
{
	//printf("Indeo Realtime Encoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[5];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'IV50';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'IV50';
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'iv50';
	formatDescription[3].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[3].u.quicktime.codec = 'IV50';
	formatDescription[3].u.quicktime.vendor = 0;
	formatDescription[4].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[4].u.quicktime.codec = 'iv50';
	formatDescription[4].u.quicktime.vendor = 0;

	err = formatObject.MakeFormatFor(formatDescription, 5, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		//printf("Indeo5rtEncoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	my_encoding = mediaFormat.u.encoded_video.encoding;
}


extern "C" void InitIndeoAlloc();
extern "C" void UninitIndeoAlloc();

// global object, gets constructed/deleted on addon load/unload
class IndeoHousekeeping {
public:
	IndeoHousekeeping()
	{
		InitIndeoAlloc();
	}

	~IndeoHousekeeping()
	{
		UninitIndeoAlloc();
	}
} _ih;

#if BUFFER_CHECK
#warning ### using populate/check_buffer ###
void
populate_buffer(void *buf, size_t len)
{
	memset(buf, 0xbe, len);
}
size_t
check_buffer(void *buf, size_t len)
{
	unsigned char *c = (unsigned char*)buf;

	size_t i;
	for(i = len; i; --i) {
		if(c[i-1] != (unsigned char)0xbe) break;
	}
	return i;
}
#endif

Encoder *instantiate_encoder(void)
{
	if(!check_mmx_match())
		return NULL;
	if(my_encoding == 0)
		return NULL;
	return new Indeo5RTEncoder();
}

#if 0
Encoder *instantiate_nth_encoder(int32 index)
{
printf("Indeo: instantiate_nth_encoder %d\n", index);
	if(!check_mmx_match())
		return NULL;
	if(index < 0 || index > 5)
		return NULL;
	if(my_encoding == 0) {
		printf("Indeo: instantiate_nth_encoder %d, my_encoding == 0\n", index);
		return NULL;
	}
	return new Indeo5RTEncoder();
}
#endif

Indeo5RTEncoder::Indeo5RTEncoder()
{
	out_buffer = NULL;
	Y = U = V = NULL;
	keyrate = 15;
	TargetBytes = 0;
	framerate = 60;
	ScalabilityOn = false;
	pRTEncCntx = NULL;
	quality = 100 - (int)(0.60*100);
	convert_time = 0;
	encode_time = 0;
	max_used_size = 0;
}

Indeo5RTEncoder::~Indeo5RTEncoder()
{
#if 0
	if(encode_time > 0) {
		printf("Indeo5RTEncoder: convert time: %Ld, encode time: %Ld\n",
		       convert_time, encode_time);
	}
#endif

	if(pRTEncCntx) {
		RTCompressEnd(pRTEncCntx);
	}
		
	if(out_buffer) {
#if BUFFER_CHECK
		size_t s = check_buffer(out_buffer, out_buffer_size);
		fprintf(stderr, "%u %u %d %u %d%%\n", s, max_used_size, s - max_used_size, out_buffer_size, s * 100 / out_buffer_size);
#endif
		free(out_buffer);
	}
	if(Y)
		free(Y);
	if(U)
		free(U);
	if(V)
		free(V);
}

status_t 
Indeo5RTEncoder::InitFormat(media_file_format *mfi,
                            media_format *in_format,
                            media_format *out_format)
{
	status_t err = B_NO_ERROR;
	
	in_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_format->require_flags = 0;
	out_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_format->require_flags = 0;

	if(in_format->type != B_MEDIA_RAW_VIDEO) {
		in_format->type = B_MEDIA_RAW_VIDEO;
		in_format->u.raw_video = media_raw_video_format::wildcard;
	}
	
	switch(in_format->u.raw_video.display.format) {
		case B_GRAY8:     convert = y8toyuv9;        break;
		case B_RGB15:     convert = rgb15toyuv9;     break;
		case B_RGBA15:    convert = rgb15toyuv9;     break;
		case B_RGB16:     convert = rgb16toyuv9;     break;
		case B_RGB24:     convert = rgb24toyuv9;     break;
		case B_RGB32:     convert = rgb32toyuv9;     break;
		case B_RGBA32:    convert = rgb32toyuv9;     break;
		case B_YCbCr422:  convert = ycbcr422toyuv9;  break;
		default:
			in_format->u.raw_video.display.format = B_YCbCr411;
			/* fall through */
		case B_YCbCr411:  convert = ycbcr411toyuv9;  break;
	}
	
	in_format->u.raw_video.display.line_width &= ~3;
	in_format->u.raw_video.display.line_count &= ~3;

	out_format->type = B_MEDIA_ENCODED_VIDEO;
	out_format->u.encoded_video = media_encoded_video_format::wildcard;
	out_format->u.encoded_video.encoding = my_encoding;
	out_format->u.encoded_video.output = in_format->u.raw_video;
	
	return err;
}

status_t
Indeo5RTEncoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Indeo5 Realtime Video");
	strcpy(mci->short_name, "iv50rt");
	return B_NO_ERROR;
}

status_t 
Indeo5RTEncoder::SetFormat(media_file_format *mfi,
                           media_format *in_format,
                           media_format *out_format)
{
	status_t err;

	if(pRTEncCntx != NULL) {
		return B_NOT_ALLOWED;
	}
	
	err = InitFormat(mfi, in_format, out_format);
	
	if(in_format->u.raw_video.display.line_width > MAX_X_RES) {
		in_format->u.raw_video.display.line_width = MAX_X_RES;
	}
	if(in_format->u.raw_video.display.line_count > MAX_Y_RES) {
		in_format->u.raw_video.display.line_count = MAX_Y_RES;
	}
	
	if(err != B_NO_ERROR)
		return err;

	width  = in_format->u.raw_video.display.line_width;
	height = in_format->u.raw_video.display.line_count;
	
	return B_NO_ERROR;
}

status_t 
Indeo5RTEncoder::StartEncoder()
{
	if(pRTEncCntx != NULL) {
		return B_NOT_ALLOWED;
	}

	pRTEncCntx = RTCompressBegin(height, width, keyrate,
	                             TargetBytes, ScalabilityOn);
	if(pRTEncCntx == NULL) {
		return B_NO_MEMORY;
	}
	out_buffer_size = width*height + 16*1024;
	out_buffer = (char*)malloc(out_buffer_size);

#if BUFFER_CHECK
	populate_buffer(out_buffer, out_buffer_size);
#endif

	Y = (uint8*)malloc((width)*(height));
	U = (uint8*)malloc((width)*(height)/16);
	V = (uint8*)malloc((width)*(height)/16);
	if(out_buffer == NULL || Y == NULL || U == NULL || V == NULL) {
		free(out_buffer);
		free(Y);
		free(U);
		free(V);
		RTCompressEnd(pRTEncCntx);
		pRTEncCntx = NULL;
		return B_NO_MEMORY;
	}
	//printf("outbuffer %p, Y %p, U %p, V %p\n", out_buffer, Y, U, V);
	last_write_failed = false;
	return B_NO_ERROR;
}


status_t 
Indeo5RTEncoder::Encode(const void *in_buffer, int64 num_frames,
                        media_encode_info *info)
{
	uint32 frametype = FrameTypeAuto;
	
	if(info->flags & B_MEDIA_KEY_FRAME || last_write_failed)
		frametype = PicTypeK;

	bool FasterEncode = (info->time_to_encode == 0);
	uint32 usedsize = 0;
	bigtime_t t1,t2,t3;
	status_t err;
	
	if(pRTEncCntx == NULL) {
		printf("Indeo5RTEncoder::Encode called before set format\n");
		return B_NO_INIT;
	}
	t1 = system_time();
	convert((const uint8*)in_buffer, Y, U, V, width, height);
	t2 = system_time();
	RTCompress(pRTEncCntx, Y, V, U, keyrate, framerate, &frametype,
	           quality, FasterEncode, (uint8*)out_buffer, &usedsize);
	t3 = system_time();

	if(BUFFER_CHECK && usedsize > max_used_size) max_used_size = usedsize;
	
	//printf("encode Q(%d), encode_time %Ld\n", FasterEncode, t3-t2);
	convert_time += t2-t1;
	encode_time += t3-t2;

	if(frametype == PicTypeK)
		info->flags |= B_MEDIA_KEY_FRAME;
	else
		info->flags &= ~B_MEDIA_KEY_FRAME;

	err = WriteChunk(out_buffer, usedsize, info);
	last_write_failed = (err != B_NO_ERROR);
#if 0
static int framenum = 0;
	framenum++;
	if(framenum % 100 == 0) {
		printf("Indeo5RTEncoder: convert time: %Ld, encode time: %Ld\n",
		       t2-t1, t3-t2);
		printf("Indeo5RTEncoder: total convert time: %Ld, encode time: %Ld\n",
		       convert_time, encode_time);
	}
#endif
	return err;
}

status_t 
Indeo5RTEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
	memset(parameters, 0, sizeof(encode_parameters));
	parameters->quality = (float)(100-quality)/100.0;
	return B_NO_ERROR;
}

status_t 
Indeo5RTEncoder::SetEncodeParameters(encode_parameters *parameters)
{
	quality = 100 - (int)(parameters->quality*100);
	if(quality > 100) quality = 100;
	if(quality < 0) quality = 0;
	return B_NO_ERROR;
}

#if 0
status_t 
Indeo5RTEncoder::GetQuality(float *fquality)
{
	*fquality = (float)(100-quality)/100.0;
	return B_NO_ERROR;
}

status_t 
Indeo5RTEncoder::SetQuality(float fquality)
{
	quality = 100 - (int)(fquality*100);
	if(quality > 100) quality = 100;
	if(quality < 0) quality = 0;
	return B_NO_ERROR;
}

#endif

/*
** Color space converters
*/

static void rgb32toyuv9(const uint8 *InputPixels,
                        uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	const int bytes_per_pixel = 4;
	int linesize = width*bytes_per_pixel;
	int block_offset = 4 - width*4;
	int line_offset = 3*width;
	for(int y = 0; y < height; y+=4) {
		for(int x = 0; x < width; x+=4) {
			uint32 R = 0;
			uint32 G = 0;
			uint32 B = 0;
			for(int ly = 0; ly < 4; ly++) {
				int input_offset = (y+ly)*width+x;
				const uint8 *input_pixel = (uint8*)(((uint32*)InputPixels) + input_offset);
				uint8 *Yout = (Yin + input_offset);

				uint32 uYVUval;
				uint32 rgb;
				for(int lx = 0; lx < 4; lx++) {
					uint8 Yval;
					uint32 rgb = *input_pixel++;
					Yval = BtoY[(uint8)rgb];
					B += (uint8)rgb;

					rgb = *input_pixel++;
					Yval += GtoY[(uint8)rgb];
					G += (uint8)rgb;

					rgb = *input_pixel++;
					Yval += RtoY[(uint8)rgb];
					R += (uint8)rgb;
					input_pixel++;

					*Yout++ = Yval;
				}
			}
			{
				uint32 uYVUval  = BtoYUV[B >> 4];
				uYVUval += GtoYUV[G >> 4];
				uYVUval += RtoYUV[R >> 4];

				*V++ = (uint8)uYVUval;
				uYVUval >>= 8;
				*U++ = (uint8)uYVUval;
			}
		}
	}
	return;
}

static void rgb24toyuv9(const uint8 *InputPixels,
                        uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	const int bytes_per_pixel = 4;
	int linesize = width*bytes_per_pixel;
	int block_offset = 4 - width*4;
	int line_offset = 3*width;
	for(int y = 0; y < height; y+=4) {
		for(int x = 0; x < width; x+=4) {
			uint32 R = 0;
			uint32 G = 0;
			uint32 B = 0;
			for(int ly = 0; ly < 4; ly++) {
				int input_offset = (y+ly)*width+x;
				const uint8 *input_pixel = (uint8*)(InputPixels + input_offset*3);
				uint8 *Yout = (Yin + input_offset);

				uint32 uYVUval;
				uint32 rgb;
				for(int lx = 0; lx < 4; lx++) {
					uint8 Yval;
					uint32 rgb = *input_pixel++;
					Yval = BtoY[(uint8)rgb];
					B += (uint8)rgb;

					rgb = *input_pixel++;
					Yval += GtoY[(uint8)rgb];
					G += (uint8)rgb;

					rgb = *input_pixel++;
					Yval += RtoY[(uint8)rgb];
					R += (uint8)rgb;

					*Yout++ = Yval;
				}
			}
			{
				uint32 uYVUval  = BtoYUV[B >> 4];
				uYVUval += GtoYUV[G >> 4];
				uYVUval += RtoYUV[R >> 4];

				*V++ = (uint8)uYVUval;
				uYVUval >>= 8;
				*U++ = (uint8)uYVUval;
			}
		}
	}
	return;
}

static void rgb16toyuv9(const uint8 *InputPixels,
                        uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	const int bytes_per_pixel = 4;
	int linesize = width*bytes_per_pixel;
	int block_offset = 4 - width*4;
	int line_offset = 3*width;
	for(int y = 0; y < height; y+=4) {
		for(int x = 0; x < width; x+=4) {
			uint32 R = 0;
			uint32 G = 0;
			uint32 B = 0;
			for(int ly = 0; ly < 4; ly++) {
				int input_offset = (y+ly)*width+x;
				const uint16 *input_pixel = ((uint16*)InputPixels) + input_offset;
				uint8 *Yout = (Yin + input_offset);

				uint32 uYVUval;
				uint32 rgb;
				for(int lx = 0; lx < 4; lx++) {
					uint8 Yval;
					uint32 rgb = *input_pixel++;
					Yval = B5toY[(uint8)rgb&0x1f];
					B += (uint8)rgb&0x1f;
					rgb >>= 6;
					Yval += G5toY[(uint8)rgb&0x1f];
					G += (uint8)rgb&0x1f;
					rgb >>= 5;
					Yval += R5toY[(uint8)rgb&0x1f];
					R += (uint8)rgb&0x1f;
					*Yout++ = Yval;
				}
			}
			{
				uint32 uYVUval  = B5toYUV[B >> 4];
				uYVUval += G5toYUV[G >> 4];
				uYVUval += R5toYUV[R >> 4];
				*V++ = (uint8)uYVUval;
				uYVUval >>= 8;
				*U++ = (uint8)uYVUval;
			}
		}
	}
}

static void rgb15toyuv9(const uint8 *InputPixels,
                        uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	const int bytes_per_pixel = 4;
	int linesize = width*bytes_per_pixel;
	int block_offset = 4 - width*4;
	int line_offset = 3*width;
	for(int y = 0; y < height; y+=4) {
		for(int x = 0; x < width; x+=4) {
			uint32 R = 0;
			uint32 G = 0;
			uint32 B = 0;
			for(int ly = 0; ly < 4; ly++) {
				int input_offset = (y+ly)*width+x;
				const uint16 *input_pixel = ((uint16*)InputPixels) + input_offset;
				uint8 *Yout = (Yin + input_offset);

				uint32 uYVUval;
				uint32 rgb;
				for(int lx = 0; lx < 4; lx++) {
					uint8 Yval;
					uint32 rgb = *input_pixel++;
					Yval = B5toY[(uint8)rgb&0x1f];
					B += (uint8)rgb&0x1f;
					rgb >>= 5;
					Yval += G5toY[(uint8)rgb&0x1f];
					G += (uint8)rgb&0x1f;
					rgb >>= 5;
					Yval += R5toY[(uint8)rgb&0x1f];
					R += (uint8)rgb&0x1f;
					*Yout++ = Yval;
				}
			}
			{
				uint32 uYVUval  = B5toYUV[B >> 4];
				uYVUval += G5toYUV[G >> 4];
				uYVUval += R5toYUV[R >> 4];
				*V++ = (uint8)uYVUval;
				uYVUval >>= 8;
				*U++ = (uint8)uYVUval;
			}
		}
	}
}

static void ycbcr411toyuv9(const uint8 *InputPixels,
                           uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	for(int y = 0; y < height; y+=4) {
		for(int x = 0; x < width; x+=8) {
			uint32 Cb0=0, Cr0=0;
			uint32 Cb4=0, Cr4=0;
			for(int ly = 0; ly < 4; ly++) {
				int input_offset = (y+ly)*width+x;
				const uint8 *input_pixel = InputPixels + input_offset * 12 / 8;
				uint8 *Yout = (Yin + input_offset);

				Cb0 += *input_pixel++;
				*Yout++ = *input_pixel++;
				Cr0 += *input_pixel++;
				*Yout++ = *input_pixel++;
				Cb4 += *input_pixel++;
				*Yout++ = *input_pixel++;
				Cr4 += *input_pixel++;
				*Yout++ = *input_pixel++;
				*Yout++ = *input_pixel++;
				*Yout++ = *input_pixel++;
				*Yout++ = *input_pixel++;
				*Yout++ = *input_pixel++;
			}
			*U++ = (uint8)(Cb0 >> 2);
			*U++ = (uint8)(Cb4 >> 2);
			*V++ = (uint8)(Cr0 >> 2);
			*V++ = (uint8)(Cr4 >> 2);
		}
	}
}

static void ycbcr422toyuv9(const uint8 *InputPixels,
                           uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	for(int y = 0; y < height; y+=4) {
		for(int x = 0; x < width; x+=4) {
			uint32 Cb0=0, Cr0=0;
			for(int ly = 0; ly < 4; ly++) {
				int input_offset = (y+ly)*width+x;
				const uint8 *input_pixel = InputPixels + input_offset * 2;
				uint8 *Yout = (Yin + input_offset);

				*Yout++ = *input_pixel++;
				Cb0 += *input_pixel++;
				*Yout++ = *input_pixel++;
				Cr0 += *input_pixel++;
				*Yout++ = *input_pixel++;
				Cb0 += *input_pixel++;
				*Yout++ = *input_pixel++;
				Cr0 += *input_pixel++;
			}
			*U++ = (uint8)(Cb0 >> 3);
			*V++ = (uint8)(Cr0 >> 3);
		}
	}
}

static void y8toyuv9(const uint8 *InputPixels,
                     uint8 *Yin, uint8 *U, uint8 *V, int width, int height)
{
	uint8 *Yout = Yin;
	const uint8 *srcp = InputPixels;
	const uint8 *endp = srcp+width*height;
	while(srcp < endp) {
		*Yout++ = *srcp++;
	}
	memset(U, 128, width*height/16);
	memset(V, 128, width*height/16);
}
