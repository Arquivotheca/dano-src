#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <MediaFormats.h>
#include <MediaTrack.h>

#include "Decoder.h"
#include "Extractor.h"
#include "Decoder.h"
#include "AppleVideo.h"

using namespace BPrivate;

media_encoded_video_format::video_encoding my_encoding;

static media_format mediaFormat;

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	//printf("AppleVideoDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[5];
	const int					formatDescription_count =
		sizeof(formatDescription) / sizeof(media_format_description);

	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'azpr';
	formatDescription[1].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[1].u.quicktime.codec = 'rpza';
	formatDescription[2].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[2].u.quicktime.codec = 'azpr';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'rpza';
	formatDescription[4].family = B_AVI_FORMAT_FAMILY;
	formatDescription[4].u.avi.codec = 'azpr';
	err = formatObject.MakeFormatFor(formatDescription, formatDescription_count, &mediaFormat);
	if(err != B_NO_ERROR) {
		//printf("Indeo5Decoder: AppleVideoDecoder failed, %s\n", strerror(err));
	}
	my_encoding = mediaFormat.u.encoded_video.encoding;
	*out_format = &mediaFormat;
	*out_count = 1;
}


Decoder *instantiate_decoder(void)
{
	if(my_encoding == 0)
		return NULL;
	return new AppleVideoDecoder();
}

AppleVideoDecoder::AppleVideoDecoder()
{
}

AppleVideoDecoder::~AppleVideoDecoder()
{
}

status_t
AppleVideoDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "AppleVideo Compression");
	strcpy(mci->short_name, "apple-video");
	return B_OK;
}


//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
AppleVideoDecoder::Sniff(const media_format *in_format,
                         const void *in_info, size_t in_size)
{
	status_t					err;
	const media_raw_video_format *rvf = &in_format->u.raw_video;

	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;
	if (in_format->u.encoded_video.encoding != my_encoding)
		return B_BAD_TYPE;

	output_format = in_format->u.encoded_video.output;
	output_format.display.format = B_RGB32;
	output_format.display.bytes_per_row = output_format.display.line_width * 4;
	PRINT(("rowBytes %ld\n", in_format->u.encoded_video.output.display.bytes_per_row));
	return B_OK;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
AppleVideoDecoder::Format(media_format *inout_format)
{
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_LINEAR_UPDATES
	                         | B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = B_MEDIA_RETAINED_DATA;

	inout_format->type = B_MEDIA_RAW_VIDEO;
	inout_format->u.raw_video = output_format;
	return B_OK;
}



//	QuickTime Macros
#define QuickTimeMinMaxCheck(x,y,min_x,min_y,max_x,max_y) {	\
  if (x > max_x) max_x=x; if (y > max_y) max_y=y;	\
  if (x < min_x) min_x=x; if (y < min_y) min_y=y;  }

inline void
AppleVideoC1(uint32 *ip0, uint32 *ip1, uint32 *ip2,
	uint32 *ip3, uint32 c, uint32 *max)
{
	if (ip0 >= max)
		return;
	ip0[0] = ip0[1] = ip0[2] = ip0[3] = c;
	if (ip1 >= max)
		return;
	ip1[0] = ip1[1] = ip1[2] = ip1[3] = c;
	if (ip2 >= max)
		return;
	ip2[0] = ip2[1] = ip2[2] = ip2[3] = c;
	if (ip3 >= max)
		return;
	ip3[0] = ip3[1] = ip3[2] = ip3[3] = c; 
}

inline void
AppleVideoC4(uint32 *ip, uint32 *c, uint32 mask)
{
	ip[0] = (c[((mask>>6)&0x03)]);
	ip[1] = (c[((mask>>4)&0x03)]);
	ip[2] = (c[((mask>>2)&0x03)]);
	ip[3] =(c[ (mask & 0x03)]);
}

inline void
AppleVideoColor16(uint32 *ip0, uint32 *ip1, uint32 *ip2,
	uint32 *ip3, uint32 *c, uint32 *max)
{
	if (ip0 >= max)
		return;
	ip0[0] = (uint32)(*c++); ip0[1] = (uint32)(*c++);
	ip0[2] = (uint32)(*c++); ip0[3] = (uint32)(*c++);
	if (ip1 >= max)
		return;
	ip1[0] = (uint32)(*c++); ip1[1] = (uint32)(*c++);
	ip1[2] = (uint32)(*c++); ip1[3] = (uint32)(*c++);
	if (ip2 >= max)
		return;
	ip2[0] = (uint32)(*c++); ip2[1] = (uint32)(*c++);
	ip2[2] = (uint32)(*c++); ip2[3] = (uint32)(*c++);
	if (ip3 >= max)
		return;
	ip3[0] = (uint32)(*c++); ip3[1] = (uint32)(*c++);
	ip3[2] = (uint32)(*c++); ip3[3] = (uint32)(*c  );
}

inline void
AppleVideoBlockInc(int32 &x, int32 &y, int32 imagex,
	uchar *&im0, uchar *&im1, uchar *&im2, uchar *&im3,
	int32 binc, int32 rinc)
{
	x += 4;
	im0 += binc;
	im1 += binc;
	im2 += binc;
	im3 += binc;
	if (x >= imagex) {
		rinc = rinc - 4 * (x - imagex);
			// adjust for row overshoot
		x=0;
		y += 4;
		im0 += rinc;
		im1 += rinc;
		im2 += rinc;
		im3 += rinc;
	}
}

#define AppleVideoRGBC1(ip,r,g,b) { \
 ip[0] = ip[3] = ip[6] = ip[9]  = r; \
 ip[1] = ip[4] = ip[7] = ip[10] = g; \
 ip[2] = ip[5] = ip[8] = ip[11] = b; }

#define AppleVideoRGBC4(ip,r,g,b,mask); { uint32 _idx; \
 _idx = (mask>>6)&0x03; ip[0] = r[_idx]; ip[1] = g[_idx]; ip[2] = b[_idx]; \
 _idx = (mask>>4)&0x03; ip[3] = r[_idx]; ip[4] = g[_idx]; ip[5] = b[_idx]; \
 _idx = (mask>>2)&0x03; ip[6] = r[_idx]; ip[7] = g[_idx]; ip[8] = b[_idx]; \
 _idx =  mask    &0x03; ip[9] = r[_idx]; ip[10] = g[_idx]; ip[11] = b[_idx]; }


#define AppleVideoRGBC16(ip,r,g,b) { \
 ip[0]= *r++; ip[1]= *g++; ip[2]= *b++; \
 ip[3]= *r++; ip[4]= *g++; ip[5]= *b++; \
 ip[6]= *r++; ip[7]= *g++; ip[8]= *b++; \
 ip[9]= *r++; ip[10]= *g++; ip[11]= *b++; }


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

#define ERROR

//-------------------------------------------------------------------
//	GetTrueColor
//-------------------------------------------------------------------
//
//	Convert RGB values into a 32-bit BGRA uint32
//

uint32 GetTrueColor(uint32 red, uint32 green, uint32 blue, uint32 bits)
{
	uint32 newColor = 0;
	
	switch(bits)
	{
		case 16:
			{
				red	  = (red   << 3) | (red   >> 2);
				green = (green << 3) | (green >> 2);
				blue  = (blue  << 3) | (blue  >> 2);
								
				uint32 alpha = 255;
				
				//	Create new color
				#if B_HOST_IS_LENDIAN
					newColor = ( ((alpha << 24) & kAlphaMask) | ((red << 16) & kRedMask) | ((green << 8) & kGreenMask) | (blue & kBlueMask) ); 					
				#else
					newColor = ( ((blue << 24) & kBlueMask) | ((green << 16) & kGreenMask) | ((red << 8) & kRedMask) | (alpha & kAlphaMask) ); 	
				#endif								
			}
			break;
						
		default:
			TRESPASS();
			break;
	
	}
		
	return newColor;
}

//-------------------------------------------------------------------
//	QuickTimeGetColor
//-------------------------------------------------------------------
//
//

static uint32
QuickTimeGetColor(uint32 color)
{
	register uint32 newColor, ra, ga, ba, ra5, ga5, ba5;
		
	ra5 = (color >> 10) & 0x1f;
	ga5 = (color >>  5) & 0x1f;
	ba5 =  color & 0x1f;
		
	//ra = qt_gamma_adj[ra5]; 
	//ga = qt_gamma_adj[ga5]; 
	//ba = qt_gamma_adj[ba5];

	ra = ra5; 
	ga = ga5; 
	ba = ba5;
			
	newColor = GetTrueColor(ra, ga, ba, 16);
			
	return(newColor);
}

//-------------------------------------------------------------------
//	QuickTimeGetRGBColor
//-------------------------------------------------------------------
//
//

static void
QuickTimeGetRGBColor( uchar *r, uchar *g, uchar *b, uint32 color)
{ 
	uint32 ra, ga, ba;
	
  	ra = (color >> 10) & 0x1f;	
  	ra = (ra << 3) | (ra >> 2);
  	ga = (color >>  5) & 0x1f;	
  	ga = (ga << 3) | (ga >> 2);
  	
  	ba =  color & 0x1f;		
  	ba = (ba << 3) | (ba >> 2);
  	
  	*r = ra; 
  	*g = ga; 
  	*b = ba;
}


//-------------------------------------------------------------------
//	QuickTimeGetAVColors
//-------------------------------------------------------------------
//
//

static void
QuickTimeGetAVColors(uint32 *c, uint32 cA, uint32 cB)
{
	uint32 clr,rA,gA,bA,rB,gB,bB,r0,g0,b0,r1,g1,b1;
	uint32 rA5,gA5,bA5,rB5,gB5,bB5;
	uint32 r05,g05,b05,r15,g15,b15;
	
	//	color 3
	rA5 = (cA >> 10) & 0x1f;
	gA5 = (cA >>  5) & 0x1f;
	bA5 =  cA & 0x1f;
	
	//	color 0
	rB5 = (cB >> 10) & 0x1f;
	gB5 = (cB >>  5) & 0x1f;
	bB5 =  cB & 0x1f;
	
	//	color 2
	r05 = (21 * rA5 + 11 * rB5) >> 5;
	g05 = (21 * gA5 + 11 * gB5) >> 5;
	b05 = (21 * bA5 + 11 * bB5) >> 5;
	
	//	color 1
	r15 = (11 * rA5 + 21 * rB5) >> 5;
	g15 = (11 * gA5 + 21 * gB5) >> 5;
	b15 = (11 * bA5 + 21 * bB5) >> 5;
	
	//	adj and scale to 16 bits
	/*rA = qt_gamma_adj[rA5]; 
	gA = qt_gamma_adj[gA5]; 
	bA = qt_gamma_adj[bA5];
	
	rB = qt_gamma_adj[rB5]; 
	gB = qt_gamma_adj[gB5]; 
	bB = qt_gamma_adj[bB5];
	
	r0 = qt_gamma_adj[r05]; 
	g0 = qt_gamma_adj[g05]; 
	b0 = qt_gamma_adj[b05];
	
	r1 = qt_gamma_adj[r15]; 
	g1 = qt_gamma_adj[g15]; 
	b1 = qt_gamma_adj[b15];
	
	rA = qt_gamma_adj[rA5]; 
	gA = qt_gamma_adj[gA5]; 
	bA = qt_gamma_adj[bA5];*/
	
	rA = rA5; 
	gA = gA5; 
	bA = bA5;

	rB = rB5; 
	gB = gB5; 
	bB = bB5;
	
	r0 = r05; 
	g0 = g05; 
	b0 = b05;
	
	r1 = r15; 
	g1 = g15; 
	b1 = b15;
	
	//	1st Color
	clr = GetTrueColor(rA, gA, bA, 16);
	c[3] = clr;
	
	// 2nd Color
	clr = GetTrueColor(rB, gB, bB, 16);	
	c[0] = clr;
	
	//	1st Av
	clr = GetTrueColor(r0, g0, b0, 16);
	c[2] = clr;
	
	//	2nd Av
	clr = GetTrueColor(r1, g1, b1, 16);
	c[1] = clr;
}


//-------------------------------------------------------------------
//	QuickTimeGetAVRGBColors
//-------------------------------------------------------------------
//
//

static void
QuickTimeGetAVRGBColors(uint32 *c, uchar *r, uchar *g, uchar *b, uint32 cA, uint32 cB)
{ 
	uint32 rA, gA, bA, rB, gB, bB, ra, ga, ba;
	
	//	Color 3
	rA 	 = (cA >> 10) & 0x1f;	
	r[3] = (rA << 3) | (rA >> 2);
	
	gA 	 = (cA >>  5) & 0x1f;	
	g[3] = (gA << 3) | (gA >> 2);
	
	bA 	 =  cA & 0x1f;		
	b[3] = (bA << 3) | (bA >> 2);
	
	//	Color 0
	rB   = (cB >> 10) & 0x1f;	
	r[0] = (rB << 3) | (rB >> 2);
	
	gB   = (cB >>  5) & 0x1f;	
	g[0] = (gB << 3) | (gB >> 2);
	
	bB 	 =  cB & 0x1f;		
	b[0] = (bB << 3) | (bB >> 2);
	
	//	Color 2
	ra 	 = (21*rA + 11*rB) >> 5;	
	r[2] = (ra << 3) | (ra >> 2);
	
	ga   = (21*gA + 11*gB) >> 5;	
	g[2] = (ga << 3) | (ga >> 2);
	
	ba   = (21*bA + 11*bB) >> 5;	
	b[2] = (ba << 3) | (ba >> 2);
	
	//	Color 1
	ra   = (11*rA + 21*rB) >> 5;	
	r[1] = (ra << 3) | (ra >> 2);
	
	ga   = (11*gA + 21*gB) >> 5;	
	g[1] = (ga << 3) | (ga >> 2);
	
	ba   = (11*bA + 21*bB) >> 5;	
	b[1] = (ba << 3) | (ba >> 2);
}


static bool
DecodeAppleVideo(uint32 width, uint32 height,
                 const void *srcBuffer, size_t bufSize, void *dstBuffer)
{ 
	uint32 	changed;
	int32 	x, y, length, row_inc, blk_inc; 
	int32 	minX, maxX, minY, maxY;
	uchar 	*im0, *im1, *im2, *im3;
	
	//	Setup pointers
	uchar *srcPtr 	= (uchar *)srcBuffer;
	uchar *dstPtr 	= (uchar *)dstBuffer; 	

	// skip 0xe1 byte
	srcPtr++;	
		
	// Read length
	length  = (*srcPtr++) << 16; 
	length |= (*srcPtr++) << 8; 
	length |= (*srcPtr++); 
	
	//	Check for corrupt frame.  Common with Apple Video codec
	if (length != bufSize)
	{ 
		ERROR("Corrupt frame... skipping...%x %x\n", bufSize, length);
		return false;
	}
	
	// read 4 bytes already 
	length -= 4;				
		
	maxX 	= 0;
	maxY 	= 0; 
	minX 	= width; 
	minY 	= height; 
	changed = 0;
	x 		= 0;
	y 		= 0;
	blk_inc = 4;
	
	uint32 *outBufferEnd = (uint32 *)(dstPtr + width * height * 4);
	row_inc = blk_inc * width;
	blk_inc *= 4;
	im1 = im0 = dstPtr;	
	im1 += row_inc; 
	im2 = im1;		
	im2 += row_inc;
	im3 = im2;		
	im3 += row_inc;
	
//	PRINT(("width %d, height %d, rowInc %d\n", width, height, row_inc));

	// skip 3 rows at a time
	row_inc *= 3; 
	

	while(length > 0)
	{ 
		uint32 code = *srcPtr++; 
		length--;
	
		//	Single 
		if ( (code >= 0xa0) && (code <= 0xbf) )			
		{
			uint32 color, skip;
			changed = 1;
			color = (*srcPtr++) << 8; 
			color |= *srcPtr++; 
			length -= 2;
			skip = (code - 0x9f);
			
			
			color = QuickTimeGetColor(color); 
			
			while(skip--)
			{
				
#if xDEBUG
				if (x % 16 == 0) {
					color = 0;
				}
#endif

				uint32 *ip0 = (uint32 *)im0; 
				uint32 *ip1 = (uint32 *)im1; 
				uint32 *ip2 = (uint32 *)im2; 
				uint32 *ip3 = (uint32 *)im3;
				AppleVideoC1(ip0,ip1,ip2,ip3,color,outBufferEnd);
								
				QuickTimeMinMaxCheck(x, y, minX, minY, maxX, maxY);
				AppleVideoBlockInc(x, y, width, im0, im1, im2, im3, blk_inc, row_inc);
			}
		}
		// Skip
		else if ( (code >= 0x80) && (code <= 0x9f) )		
		{ 
			uint32 skip = (code-0x7f);
			
			while(skip--) 
				AppleVideoBlockInc(x, y, width, im0, im1, im2, im3, blk_inc, row_inc);
		}
		//	Four or Sixteen color block
		else if ( (code < 0x80) || ((code >= 0xc0) && (code <= 0xdf)) )
		{ 
			uint32 cA,cB;
			changed = 1;
			
			// Get 1st two colors
			if (code >= 0xc0) 
			{ 
				cA = (*srcPtr++) << 8; 
				cA |= *srcPtr++; 
				length -= 2; 
			}
			else 
			{
				cA = (code << 8) | *srcPtr++; 
				length -= 1;
			}
			
			cB = (*srcPtr++) << 8; 
			cB |= *srcPtr++; 
			length -= 2;
		
			//	Sixteen color block
			if ( (code < 0x80) && ((cB & 0x8000)==0) )
			{
				uint32 i,d,*clr,c[16];
				
				clr = c;
				*clr++ = QuickTimeGetColor(cA);
				*clr++ = QuickTimeGetColor(cB);
				
				for(i = 2; i < 16; i++)
				{
					d = (*srcPtr++) << 8; 
					d |= *srcPtr++; 
					length -= 2;
					
					*clr++ = QuickTimeGetColor(d);
				}				
								
#if xDEBUG
					if (x % 16 == 0) {
						c[0] = 0;
						c[1] = 0;
						c[2] = 0;
						c[3] = 0;
						c[4] = 0;
						c[5] = 0;
						c[6] = 0;
						c[7] = 0;
						c[8] = 0;
						c[9] = 0;
						c[10] = 0;
						c[11] = 0;
						c[12] = 0;
						c[13] = 0;
						c[14] = 0;
						c[15] = 0;
					}
#endif

				clr = c;
				
				uint32 *ip0 = (uint32 *)im0; 
				uint32 *ip1 = (uint32 *)im1; 
				uint32 *ip2 = (uint32 *)im2; 
				uint32 *ip3 = (uint32 *)im3;
				AppleVideoColor16(ip0, ip1, ip2, ip3, clr, outBufferEnd);
								
				QuickTimeMinMaxCheck(x,y,minX,minY,maxX,maxY);
				AppleVideoBlockInc(x, y, width, im0, im1, im2, im3, blk_inc, row_inc);
			}
			// Four color block
			else					
			{ 
				uint32 m_cnt, msk0, msk1, msk2, msk3, c[4];
			
				if (code < 0x80) 
					m_cnt = 1; 
				else 
					m_cnt = code - 0xbf; 
			
				QuickTimeGetAVColors(c, cA, cB);
				while(m_cnt--)
				{
				
#if xDEBUG
					if (x % 16 == 0) {
						c[0] = 0;
						c[1] = 0;
						c[2] = 0;
						c[3] = 0;
					}
#endif

					msk0 = *srcPtr++; 
					msk1 = *srcPtr++;
					msk2 = *srcPtr++; 
					msk3 = *srcPtr++; 
					length -= 4;
					
					uint32 *ip0= (uint32 *)im0; 
					uint32 *ip1= (uint32 *)im1; 			
					uint32 *ip2= (uint32 *)im2; 
					uint32 *ip3= (uint32 *)im3;
		
					if (ip0 < outBufferEnd)
						AppleVideoC4(ip0, c, msk0);
					if (ip1 < outBufferEnd)
						AppleVideoC4(ip1, c, msk1);
					if (ip2 < outBufferEnd)
						AppleVideoC4(ip2, c, msk2);
					if (ip3 < outBufferEnd)
						AppleVideoC4(ip3, c, msk3);
					
					QuickTimeMinMaxCheck(x, y, minX, minY, maxX, maxY);
					AppleVideoBlockInc(x, y, width, im0, im1, im2, im3, blk_inc, row_inc);
				}  
			}
		}
		//	Unknown
		else
		{
			ERROR("Apple Video: Unknown %x\n", code);
			return false;
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
AppleVideoDecoder::Decode(void *output, int64 *inout_frameCount,
						  media_header *mf, media_decode_info *info)
{
	const void	*compressed_data;
	size_t 		size;
	status_t	err;

	err = GetNextChunk(&compressed_data, &size, mf, info);
	if (err != B_OK)
		return err;
	
	if (DecodeAppleVideo(output_format.display.line_width,
	                     output_format.display.line_count, compressed_data,
	                     size, output) == false)
		return B_ERROR;

	*inout_frameCount = 1;

	return B_OK;
}
