#include <stdio.h>
#include <stdlib.h>
#include <MediaFormats.h>
#include <memory.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"
#include "MSRLE.h"
#include "RIFFTypes.h"

media_encoded_video_format::video_encoding msrle_encoding;
static media_format mediaFormat;

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	//printf("MSRLEDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[2];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 'rle8';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 0x01000000;
	err = formatObject.MakeFormatFor(formatDescription, 2, &mediaFormat);
	if(err != B_NO_ERROR) {
		//printf("MSRLEDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	msrle_encoding = mediaFormat.u.encoded_video.encoding;

	*out_format = &mediaFormat;
	*out_count = 1;
};

Decoder *instantiate_decoder(void)
{
	if(msrle_encoding == 0)
		return 0;
	return new MSRLEDecoder();
}

MSRLEDecoder::MSRLEDecoder()
{
}

MSRLEDecoder::~MSRLEDecoder()
{
}

status_t
MSRLEDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "MS RLE Compression");
	strcpy(mci->short_name, "msrle");
	return B_OK;
}

//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
MSRLEDecoder::Sniff(const media_format *in_format,
					const void *in_info, size_t in_size) 
{
	//printf("MSRLEDecoder:sniff\n");
	status_t					err;

	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;
	if(in_format->u.encoded_video.encoding != msrle_encoding)
		return B_BAD_TYPE;
	
	AVIVIDSHeader               *vids = (AVIVIDSHeader *)in_info;
	if(in_size < sizeof(AVIVIDSHeader) + 256*4)
		return B_BAD_TYPE;
	fClut = (uint32*)(vids+1);

	fOutputFormat = in_format->u.encoded_video.output;
	fOutputFormat.display.format = B_RGB32;
	fOutputFormat.display.bytes_per_row = 4*fOutputFormat.display.line_width;
	return B_OK;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
MSRLEDecoder::Format(media_format *inout_format)
{
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;
	
	inout_format->type = B_MEDIA_RAW_VIDEO;
	inout_format->u.raw_video = fOutputFormat;
	return B_OK;
}



#define LOOP


//-------------------------------------------------------------------
//	DecodeMicrosoftRLE
//-------------------------------------------------------------------
//
//	Decode Microsoft RLE and return BBitmap
//

static bool
DecodeMicrosoftRLE(uint32 width, uint32 height,
                   const void *buffer, size_t bufSize,
                   void *dstBuffer, uint32 *clut)
{ 
	uint32 opcode, mod;
	int32 x, y, minX, maxX, minY, maxY;
	
	int32 dataSize = bufSize;
	
	maxX 	= 0;
	maxY 	= 0; 
	minX 	= width; 
	minY 	= height;
	x 		= 0;  
	y 		= height - 1;
		
	//	Setup pointers
	uchar *bufPtr = (uchar *)buffer;
	uchar *bits = (uchar *)dstBuffer; 	

	while( (y >= 0) && (dataSize > 0) )
	{
		mod 	 = *bufPtr++;
		opcode 	 = *bufPtr++;  
		dataSize -= 2;
	
		LOOP("MOD %x OPCODE %x <%d,%d>\n", mod, opcode, x, y);
		
		//	End of line
		if (mod == 0x00)				
		{
			if (opcode==0x00)
			{
				while( x > width) 
				{ 
					x -= width; 
					y--; 
				}
				
				x = 0; 
				y--;
				
				LOOP("EOL <%d,%d>\n",x,y);
			}
			//	End of image
			else if (opcode==0x01)
			{
				y = -1;
				
				LOOP("EOI <%d,%d>\n",x,y);
			}
			//	Skip
			else if (opcode==0x02)
			{
				uint32 yskip,xskip;
				xskip = *bufPtr++; 
				yskip = *bufPtr++;  dataSize-=2;
				x += xskip;
				y -= yskip;
				
				LOOP("SKIP <%d,%d>\n",x,y);
			}
			//	Absolute
			else					
			{
				int cnt = opcode;
	
				dataSize-=cnt;
				while(x >= width) 
				{ 
					x -= width; 
					y--; 
				}
				
				if (y > maxY) 
					maxY = y; 
				
				if (x < minX) 
					minX = x;
					
				uint32 *iptr = (uint32 *)(bits + ((y * width + x)<<2) );
					
				while(cnt--) 
				{ 
					if (x >= width)  
					{ 
						maxX = width; 
						minX = 0;
						x -= width; 
						y--; 
						iptr = (uint32 *)(bits + ((y * width + x)<<2)); 
					}
					
					*iptr++ = clut[(uchar)(*bufPtr++)];
					x++;
				}
	
				LOOP("Absolute <%d,%d>\n",x,y);
	
				//	Pad to int16
				if (opcode & 0x01) 
				{ 
					bufPtr++; 
					dataSize--; 
				}
				
				if (y < minY) 
					minY = y; 
				
				if (x > maxX) 
					maxX = x;
			}
		}
		//	Encoded
		else					
		{
			int color,cnt;
			
			while(x >= width) 
			{ 
				x -= width; 
				y--; 
			}
			
			if (y > maxY) 
				maxY = y; 
			
			if (x < minX) 
				minX = x;
				
			cnt   = mod;
			color = opcode;
			
			
			uint32 *iptr = (uint32 *)(bits + ((y * width + x)<<2) );
			uint32 clr = clut[color];
		
			while(cnt--) 
			{ 
				if (x >= width)  
				{ 
					maxX = width; 
					minX = 0;
					x -= width; 
					y--; 
					iptr = (uint32 *)(bits + ((y * width + x)<<2)); 
				}
				*iptr++ = clr; 
				x++;
			}
			
			if (y < minY) 
				minY = y; 
			
			if (x > maxX) 
				maxX = x;
				
			LOOP("Encoded <%d,%d>\n",x,y);
		}
	}
	
	#ifdef DEBUG
	{
		LOOP("dataSize %d\n  ",dataSize);
		while(dataSize)  
		{ 
			int d = *bufPtr++;  
			LOOP("<%02x> ",d); 
			dataSize--; 
		}
		LOOP("\n");
	}
	#endif
	
	return true;
}



//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 
status_t
MSRLEDecoder::Decode(void *output, int64 *frame_count, media_header *mh,
                     media_decode_info *info)
{
	const void	*compressed_data;
	size_t 		size;
	bool 		ret;
	status_t	err;

	err = GetNextChunk(&compressed_data, &size, mh, info);
	if (err != B_OK)
		return err;

	ret = DecodeMicrosoftRLE(fOutputFormat.display.line_width,
	                         fOutputFormat.display.line_count,
	                         compressed_data, size, output, fClut);
	if (ret == false)
		return B_ERROR;

	*frame_count = 1;
	
	return B_OK;
}
