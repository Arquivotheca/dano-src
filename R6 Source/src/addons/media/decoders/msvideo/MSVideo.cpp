#include <stdio.h>
#include <stdlib.h>
#include <ByteOrder.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "RIFFTypes.h"
#include "MSVideo.h"

media_encoded_video_format::video_encoding msvideo_encoding;
media_format mediaFormat;

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	//printf("MSVideoDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[6];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 'wham';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'WHAM';
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'cram';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'CRAM';
	formatDescription[4].family = B_AVI_FORMAT_FAMILY;
	formatDescription[4].u.avi.codec = 'msvc';
	formatDescription[5].family = B_AVI_FORMAT_FAMILY;
	formatDescription[5].u.avi.codec = 'MSVC';
	err = formatObject.MakeFormatFor(formatDescription, 6, &mediaFormat);
	if(err != B_NO_ERROR) {
		//printf("MSVideoDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	msvideo_encoding = mediaFormat.u.encoded_video.encoding;

	*out_format = &mediaFormat;
	*out_count = 1;
};

Decoder *instantiate_decoder(void)
{
	if(msvideo_encoding == 0)
		return NULL;
	return new MSVideoDecoder();
}

MSVideoDecoder::MSVideoDecoder()
{
}

MSVideoDecoder::~MSVideoDecoder()
{
}


status_t
MSVideoDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "MS-Video Compression");
	strcpy(mci->short_name, "msvideo");
	return B_OK;
}


//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
MSVideoDecoder::Sniff(const media_format *in_format,
					  const void *in_info, size_t in_size)
{
	status_t					err;
	AVIVIDSHeader               *vids = (AVIVIDSHeader *)in_info;

	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;
	if (in_format->u.encoded_video.encoding != msvideo_encoding)
		return B_BAD_TYPE;

	/* we only accept 8 and 16 bit format data */
	if (vids && vids->BitCount != 8 && vids->BitCount != 16)
		return B_BAD_TYPE;

	fOutputFormat = in_format->u.encoded_video.output;

	if (vids)
		bitmap_depth = vids->BitCount;
	else
		bitmap_depth = 16;  /* XXXdbg just have to assume it's 16 bit... */

	fOutputFormat.display.format = B_RGB32;
	fOutputFormat.display.bytes_per_row = 4*fOutputFormat.display.line_width;
	if(bitmap_depth == 8) {
		if(in_size < 256*4)
			return B_BAD_TYPE;
		pal = (uint32*)((char*)in_info + in_size - 256 * 4);
	}
	else {
		pal = NULL;
	}
	return B_OK;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
MSVideoDecoder::Format(media_format *inout_format)
{
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = B_MEDIA_RETAINED_DATA;
	
	inout_format->type = B_MEDIA_RAW_VIDEO;
	inout_format->u.raw_video = fOutputFormat;
	return B_OK;
}




//	Microsoft Macros
#define MSBlockInclude(x, y, width) { x += 4; if ( x >= width) { x = 0; y -= 4; } }

#define MSVideoC1(ip,clr,rdec) { \
	*ip++ = clr; *ip++ = clr; *ip++ = clr; *ip = clr; ip -= rdec; \
	*ip++ = clr; *ip++ = clr; *ip++ = clr; *ip = clr; ip -= rdec; \
	*ip++ = clr; *ip++ = clr; *ip++ = clr; *ip = clr; ip -= rdec; \
	*ip++ = clr; *ip++ = clr; *ip++ = clr; *ip = clr; }

#define MSVideoC2(ip,flag,cA,cB,rdec) { \
	 *ip++ =(flag&0x01)?(cB):(cA); *ip++ =(flag&0x02)?(cB):(cA); \
	 *ip++ =(flag&0x04)?(cB):(cA); *ip   =(flag&0x08)?(cB):(cA); ip-=rdec; \
	 *ip++ =(flag&0x10)?(cB):(cA); *ip++ =(flag&0x20)?(cB):(cA); \
	 *ip++ =(flag&0x40)?(cB):(cA); *ip   =(flag&0x80)?(cB):(cA); }

#define MSVideoC4(ip,flag,cA0,cA1,cB0,cB1,rdec) { \
	 *ip++ =(flag&0x01)?(cB0):(cA0); *ip++ =(flag&0x02)?(cB0):(cA0); \
	 *ip++ =(flag&0x04)?(cB1):(cA1); *ip   =(flag&0x08)?(cB1):(cA1); ip-=rdec; \
	 *ip++ =(flag&0x10)?(cB0):(cA0); *ip++ =(flag&0x20)?(cB0):(cA0); \
	 *ip++ =(flag&0x40)?(cB1):(cA1); *ip   =(flag&0x80)?(cB1):(cA1); }

#define MSMinMaxCheck(x,y,min_x,max_x,min_y,max_y) { \
	if (x < min_x) min_x = x; if (y > max_y) max_y = y; \
	if (x > max_x) max_x = x; if (y < min_y) min_y = y; } 

#define MSGet16(data, dptr) { data = *dptr++; data |= (*dptr++) << 8; }

// Constants
#if B_HOST_IS_LENDIAN
	#define kBlueMask 	0x000000FF
	#define kGreenMask 	0x0000FF00
	#define kRedMask	0x00FF0000
	#define kAlphaMask	0xFF000000
#else
	#define kBlueMask 	0xFF000000
	#define kGreenMask 	0x00FF0000
	#define kRedMask	0x0000FF00
	#define kAlphaMask	0x000000FF
#endif


#define FUNCTION
#define LOOP

//-------------------------------------------------------------------
//	AVIGetColor
//-------------------------------------------------------------------
//
//	Convert value into a 32-bit BGRA uint32
//

uint32	AVIGetColor(uint32 srcColor)
{ 
	register uint32 alpha, newColor, ra, ga, ba, tr, tg, tb;
	
	ra = (srcColor >> 10) & 0x1f;
	ga = (srcColor >>  5) & 0x1f;
	ba =  srcColor & 0x1f;
	
	tr = (ra << 3) | (ra >> 2);
	tg = (ga << 3) | (ga >> 2);
	tb = (ba << 3) | (ba >> 2);
	
	alpha = 255;
	
	//	Create new BGRA uint32 color
	//newColor = ( (tb << 24) & kBlueMask) | ( (tg << 16) & kGreenMask) | ((tr << 8) & kRedMask) | (alpha & kAlphaMask); 

	//	Create new color
	#if B_HOST_IS_LENDIAN
		newColor = ( ((alpha << 24) & kAlphaMask) | ((tr << 16) & kRedMask) | ((tg << 8) & kGreenMask) | (tb & kBlueMask) ); 					
	#else
		newColor = ( ((tb << 24) & kBlueMask) | ((tg << 16) & kGreenMask) | ((tr << 8) & kRedMask) | (alpha & kAlphaMask) ); 	
	#endif

	return(newColor);
}

//-------------------------------------------------------------------
//	DecodeMSVideo8
//-------------------------------------------------------------------
//
//	Decode Microsoft Video 01 8-bit 'CRAM' and return BBitmap
//

static bool
DecodeMSVideo8(uint32 width, uint32 height, const void *buffer,
			   size_t bufSize, void *dstBuffer, uint32 *clut)
{ 
	FUNCTION("DecodeMSVideo8\n");
	
	uint32 	changed = 0;
	int32 	maxX 	= 0;
	int32	maxY 	= 0;	
	int32	minX 	= width;	
	int32	minY 	= height;	
	uint32	row_dec = width + 3;
	int32 	x 		= 0;
	int32	y 		= height - 1;
	
	uint32 	blockCount 	= ((width * height) >> 4) + 1;
	bool	exitFlag 	= false;
	
	//	Setup pointers
	uchar *bufPtr 	= (uchar *)buffer;
	uchar *bits 	= (uchar *)dstBuffer; 	
	
	while(!exitFlag)
	{
		uint32 code0 = *bufPtr++;	
		uint32 code1 = *bufPtr++;	
		blockCount--;
		
		if ( ((code1 == 0) && (code0 == 0) && !blockCount) || (y < 0)) 
			exitFlag = true;
		else
		{
			//	Skip
			if ((code1 >= 0x84) && (code1 <= 0x87)) 
			{ 
				uint32 skip = ((code1 - 0x84) << 8) + code0;
				blockCount -= (skip-1); 
				
				while(skip--) 
					MSBlockInclude(x, y, width);
			//	Single block encoded 
			} 
			else 
			{
				//	8 color quadrant encoding
				if (code1 >= 0x90) 
				{
					uint32 cA0, cA1, cB0, cB1;
					
					uint32 *i_ptr = (uint32 *)(bits + ((y * width + x)<<2));
					cB0 = clut[(uchar)*bufPtr++];  
					cA0 = clut[(uchar)*bufPtr++];
					cB1 = clut[(uchar)*bufPtr++];  
					cA1 = clut[(uchar)*bufPtr++];						
					
					MSVideoC4( i_ptr, code0, cA0, cA1, cB0, cB1, row_dec); 
					i_ptr -=row_dec;
					
					cB0 = clut[(uchar)*bufPtr++];  
					cA0 = clut[(uchar)*bufPtr++];
					cB1 = clut[(uchar)*bufPtr++];  
					cA1 = clut[(uchar)*bufPtr++];
					MSVideoC4(i_ptr, code1, cA0, cA1, cB0, cB1, row_dec);
				} 
				//	2 color encoding
				else if (code1 < 0x80) 
				{ 
					uint32 clr_A,clr_B;
					uint32 *i_ptr = (uint32 *)(bits + ((y * width + x)<<2));
					
					clr_B = clut[(uchar)*bufPtr++];   
					clr_A = clut[(uchar)*bufPtr++];
					
					MSVideoC2( i_ptr, code0, clr_A, clr_B, row_dec); 
					i_ptr -= row_dec;
					MSVideoC2(i_ptr, code1, clr_A, clr_B, row_dec);
				}
				//	1 color encoding
				else
				{
					uchar clr 	 = (uchar)code0;
					uint32 *i_ptr = (uint32 *)(bits + ((y * width + x)<<2));
					MSVideoC1(i_ptr,clut[clr],row_dec);
				}
	
				MSMinMaxCheck(x, y, minX, maxX, minY, maxY);
				changed = 1; 
				MSBlockInclude(x, y, width);
			}
		}
	}
			
	return true;
}



//-------------------------------------------------------------------
//	DecodeMSVideo16
//-------------------------------------------------------------------
//
//	Decode Microsoft Video 01 16-bit 'CRAM' and return BBitmap
//

static bool
DecodeMSVideo16(uint32 width, uint32 height, const void *buffer,
				size_t bufSize, void *dstBuffer)
{ 
	FUNCTION("DecodeMSVideo16\n");
	
	int32 bufferSize = bufSize;
	
	width 	= 4 * ((width  + 3) /4);
	height 	= 4 * ((height + 3) /4);

	uint32 code0, code1;
		
	int32 	maxX 	= 0;
	int32 	maxY 	= 0;	
	int32 	minX 	= width;	
	int32 	minY 	= height;
	int32	x 		= 0;
	int32 	y 		= height - 1;
	
	uint32 	rowDecrement = width + 3; 	
	
	bool changed  = false;
	bool exitFlag = false;
	
	uint32 blockCount = ((width * height) >> 4) + 1;
	
	//	Setup pointers
	uchar *srcPtr 	= (uchar *)buffer;
	uchar *dstPtr 	= (uchar *)dstBuffer; 	
		
	while(exitFlag == false)
	{
		code0 =  *srcPtr++;	
		code1 =  *srcPtr++;	
		blockCount--;
		
		if ( (code1==0) && (code0==0) && !blockCount) 
		{ 
			exitFlag = true; 
			continue; 
		}
		
		if (y < 0) 
		{
			exitFlag = true; 
			continue; 
		}
		
		//	Skip
		if ((code1 >= 0x84) && (code1 <= 0x87))
		{ 
			uint32 skip = ((code1 - 0x84) << 8) + code0;
			blockCount -= (skip-1); 
			
			while(skip--) 
				MSBlockInclude(x, y, width);
		}
		//	Don't skip
		else
		{ 
			uint32 *rowPtr = (uint32 *)(dstPtr + ((y * width + x) << 2) );
			
			//	2 or 8 color encoding
			if (code1 < 0x80)
			{ 
				uint32 cA,cB,cA0,cB0;
				
				MSGet16(cB, srcPtr); 
				MSGet16(cA, srcPtr);
				cB0 = AVIGetColor(cB);
				cA0 = AVIGetColor(cA);
				
				//	Eight color encoding
				if (cB & 0x8000)
				{ 
					uint32 cA1,cB1;
					MSGet16(cB, srcPtr); 
					MSGet16(cA, srcPtr);
					cB1 = AVIGetColor(cB);
					cA1 = AVIGetColor(cA);
					
					MSVideoC4(rowPtr, code0, cA0, cA1, cB0, cB1, rowDecrement); 
					rowPtr -= rowDecrement;
					
					MSGet16(cB, srcPtr); 
					MSGet16(cA, srcPtr);
					cB0 = AVIGetColor(cB);
					cA0 = AVIGetColor(cA);
					
					MSGet16(cB, srcPtr); 
					MSGet16(cA, srcPtr);
					cB1 = AVIGetColor(cB);
					cA1 = AVIGetColor(cA);
					MSVideoC4(rowPtr, code1, cA0, cA1, cB0, cB1, rowDecrement);
				} 
				//	Two color encoding
				else
				{
					MSVideoC2(rowPtr,code0,cA0,cB0, rowDecrement); 
					rowPtr -= rowDecrement;
					MSVideoC2(rowPtr, code1, cA0, cB0, rowDecrement);
				}
			}
			//	One color encoding
			else
			{ 
				uint32 cA 	 = (code1 << 8) | code0;
				uint32 color = AVIGetColor(cA);
				MSVideoC1(rowPtr, color, rowDecrement);
			}
			
			changed = true; 
			MSMinMaxCheck(x, y, minX, maxX, minY, maxY);
			MSBlockInclude(x, y, width);
		}
	}

	return true;
}




//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 
status_t
MSVideoDecoder::Decode(void *output, int64 *frame_count, media_header *mh,
                       media_decode_info *info)
{
	const void		*compressed_data;
	size_t 			size;
	bool 			ret;
	status_t		err;

	err = GetNextChunk(&compressed_data, &size, mh, info);
	if (err != B_OK)
		return err;

	if (bitmap_depth == 16) {
		ret = DecodeMSVideo16(fOutputFormat.display.line_width,
		                      fOutputFormat.display.line_count,
		                      compressed_data,size,output);
	} else if (bitmap_depth == 8) {
		ret = DecodeMSVideo8(fOutputFormat.display.line_width,
		                     fOutputFormat.display.line_count,
		                     compressed_data,size,output,pal);
	} else {
		ret = false;
	}

	if (ret == false)
		return B_ERROR;

	*frame_count = 1;

	return B_OK;
}
	
