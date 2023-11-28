#include <File.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Entry.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include "H261Detractor.h"

#include "mmx.yuv2rgb.h"

extern "C" const char * mime_type_detractor = "video/x-h261";

extern "C" Detractor* instantiate_detractor()
{
	return new H261Detractor;
}

H261Detractor::H261Detractor()
{
	printf("H261Detractor::H261Detractor\n");
	
	fCurrentFrame = -1;
	
	xRes = -1;
	yRes = -1;
	fps  = 30; 
	
	
	/*file specific (START)*/
	buff_content = 0;
	start_bit    = 0;
	/*file specific (END)*/	
}

H261Detractor::~H261Detractor()
{
	printf("H261Detractor::~H261Detractor v0.04\n");
}

status_t H261Detractor::SetTo(const entry_ref *ref)
{
	printf("H261Detractor::SetTo(const entry_ref *ref)\n");
	return SetTo(new BFile(ref, B_READ_ONLY));
}

status_t H261Detractor::SetTo(BDataIO *source)
{
	printf("H261Detractor::SetTo(BDataIO *source)\n");
	

	
	//make sure to start at the beginning of the file (data can have been already read by other sniffer)
	{
		BPositionIO *posIO=dynamic_cast<BPositionIO*>(source);
		if(posIO)
		{
			posIO->Seek(0,SEEK_SET);
		}
	}
	

	
	/*file specific (START)*/
	{
		//make sure the buffer is empty
		{
			buff_content = 0;
			start_bit    = 0;
		}
		
		//read the first 32 bits
		{
			int readed = source->Read(&buff[buff_content], 4);
			if (readed != 4)
			{
				printf("H261, file rejected (size < 4 bytes !)\n");
				return B_ERROR;
			}
			buff_content += readed;
		}
		
		//check the first 20 bits
		{
			if ((buff[0] != 0) || (buff[1] != 1)||((buff[2]& 0xf0)!= 0))
			{
				printf("H261, file rejected (bad first 20 bits)\n");
				return B_ERROR;
			} 
		}
		
		//extract the resolution		
		if ((buff[3] & 0x08) == 0x00)
		{
			xRes = 176;
			yRes = 144;
		}
		else
		{
			xRes = 352;
			yRes = 288;
		}
	}
	/*file specific (END)*/

	
	
	//source has been accepted
	printf("Accept input as h.261 (%dx%d) v0.02 (%p)\n", (int)xRes, (int)yRes, source);


	fCurrentFrame = 0;			
	fSource = source;
	
	return B_OK;
}

status_t H261Detractor::InitCheck() const
{
	printf("DummyDetractor::InitCheck\n");
	return (fCurrentFrame>=0);
}

status_t H261Detractor::GetFileFormatInfo(media_file_format *mfi) const
{
	printf("H261Detractor::GetFileFormatInfo\n");
    strcpy(mfi->mime_type,      "video/x-h261");
    strcpy(mfi->pretty_name,    "H.261");
    strcpy(mfi->short_name,     "h261");
    strcpy(mfi->file_extension, "h261");

    mfi->family = B_ANY_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        //media_file_format::B_IMPERFECTLY_SEEKABLE  |
                        //media_file_format::B_PERFECTLY_SEEKABLE    |
                        //media_file_format::B_KNOWS_RAW_AUDIO       |
                        //media_file_format::B_KNOWS_ENCODED_AUDIO;
                        media_file_format::B_KNOWS_RAW_VIDEO |
                        media_file_format::B_KNOWS_ENCODED_VIDEO;

	return B_OK;
}

const char* H261Detractor::Copyright(void) const
{
	printf("DummyDetractor::Copyright\n");
	return "Copyright 2000 Be, Inc.";
}

int32 H261Detractor::CountTracks() const
{
	printf("H261Detractor::CountTracks\n");
	return 1;
}

status_t H261Detractor::GetCodecInfo(int32 tracknum, media_codec_info *mci) const
{
	printf("H261Detractor::GetCodecInfo (track = %d)\n", (int)tracknum);
	strcpy(mci->pretty_name, "H.261 Video Detractor");
	strcpy(mci->short_name, "h261-video");
	
	return B_OK;
}

status_t H261Detractor::EncodedFormat(int32 tracknum, media_format *out_format) const
{
	printf("H261Detractor::EncodedFormat (track = %d)\n", (int)tracknum);


	out_format->type=B_MEDIA_ENCODED_VIDEO;
	
	out_format->u.encoded_video = media_encoded_video_format::wildcard;
	out_format->u.encoded_video.output.field_rate = fps;
	
	out_format->u.encoded_video.output.display = media_video_display_info::wildcard;
	out_format->u.encoded_video.output.display.format = B_RGB32;
	out_format->u.encoded_video.output.display.line_width = xRes;
	out_format->u.encoded_video.output.display.line_count = yRes;
		 
	return B_OK;
}

status_t H261Detractor::DecodedFormat(int32 tracknum, media_format *inout_format)
{
	printf("H261Detractor::DecodedFormat (track = %d)\n", (int)tracknum);
	if (tracknum != 0)
	{
		return B_ERROR;
	}
	
	inout_format->type=B_MEDIA_RAW_VIDEO;
	
	inout_format->u.raw_video = media_raw_video_format::wildcard;
	inout_format->u.raw_video.field_rate = fps;
	/*
	inout_format->u.raw_video.field_rate = fps;
	inout_format->u.raw_video.interlace = 1;
	inout_format->u.raw_video.first_active = 0;
	inout_format->u.raw_video.last_active = yRes-1;
	inout_format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	inout_format->u.raw_video.pixel_width_aspect = 1;
	inout_format->u.raw_video.pixel_height_aspect = 1;
	*/
	inout_format->u.raw_video.display = media_video_display_info::wildcard;
	inout_format->u.raw_video.display.format = B_RGB32;
	inout_format->u.raw_video.display.line_width = xRes;
	inout_format->u.raw_video.display.line_count = yRes;

	return B_OK;
}

int64    H261Detractor::CountFrames(int32 tracknum) const
{
	printf("DummyDetractor::CountFrames (track %d)\n", (int) tracknum);

	return 441000000;
}

bigtime_t H261Detractor::Duration(int32 tracknum) const
{
	printf("DummyDetractor::Duration (track %d)\n", (int) tracknum);

	return (bigtime_t)10000000000.0;
}

int64    H261Detractor::CurrentFrame(int32 tracknum) const
{
	printf("DummyDetractor::CurrentFrame(track %d)\n", (int) tracknum);

	return fCurrentFrame;
}

bigtime_t H261Detractor::CurrentTime(int32 tracknum) const
{
	printf("DummyDetractor::CurrentTime(track %d)\n", (int) tracknum);
	return (bigtime_t)(fCurrentFrame*1000000.0/fps);
}

status_t H261Detractor::ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount, media_header *mh = NULL)
{
	printf("H261Detractor::ReadFrames[%d](frame = %d) (track = %d)(mh = %p)\n",
	 	333,
	 	(int)fCurrentFrame,
	 	(int)tracknum,
	 	mh);

	/*file specific (START)*/
	int ret_val = 0;
	while (ret_val == 0)
	{
		ret_val = file_decode_packet();	
	}
	if (ret_val == 2)
	{
		return B_ERROR;
	}
	/*file specific (END)*/


	//output the frame
	p64_decoder.sync();
	yuv_to_rgb32_mmx((unsigned char *)out_buffer,
		p64_decoder.frame(), 
		&(p64_decoder.frame()[p64_decoder.width()*p64_decoder.height()]),
		&(p64_decoder.frame()[p64_decoder.width()*p64_decoder.height()+p64_decoder.width()*p64_decoder.height()/4]),
		p64_decoder.width(),
		p64_decoder.height(),
		p64_decoder.width()*4);
	
	
	
	if (mh)
	{
		mh->start_time = bigtime_t(fCurrentFrame * 1000000.0 /fps);
	}
	
	*out_frameCount = 1;
	fCurrentFrame++;
		
	return B_OK;
}
							   

status_t H261Detractor::SeekToFrame(int32 tracknum, int64 *inout_frame, int32 flags=0)
{
	printf("DummyDetractor::SeekToFrame (track %d)(frame %d)(flags %d)\n",
		(int) tracknum,
		(int)*inout_frame,
		(int)flags);
	fCurrentFrame = *inout_frame;
	return B_OK;
}


status_t H261Detractor::FindKeyFrameForFrame(int32 tracknum, int64 *inout_frame, int32 flags=0) const
{
	printf("DummyDetractor::FindKeyFrameForFrame (track %d)(frame %d)(flags %d)\n",
		(int) tracknum,
		(int)*inout_frame,
		(int)flags);
	return B_OK;
}


/*===================================*/
int H261Detractor::file_decode_packet()
{
	//decode a packet
	//return
	//	true: last packet of a picture
	//	false:
	
	
	//search GOB/packet end
	int size = -1;
	int next_gn = -1;
	{
		int i = 2;
		while (size == -1)
		{
			//we need access to
			//    i+1: to check the 16 bits
			//    i+2: in some case, to extract the next gn
			if (i+2 < buff_content)
			{
				if (buff[i] == 0)
				{
					int val;
		
					     if                                ( buff[i+1]         == 0x01 ) val =  0;
					else if (((buff[i-1] & 0x01) == 0 ) && ((buff[i+1] & 0xfe) == 0x02)) val =  1;
					else if (((buff[i-1] & 0x03) == 0 ) && ((buff[i+1] & 0xfc) == 0x04)) val =  2;
					else if (((buff[i-1] & 0x07) == 0 ) && ((buff[i+1] & 0xf8) == 0x08)) val =  3;
					else if (((buff[i-1] & 0x0f) == 0 ) && ((buff[i+1] & 0xf0) == 0x10)) val =  4;
					else if (((buff[i-1] & 0x1f) == 0 ) && ((buff[i+1] & 0xe0) == 0x20)) val =  5;
					else if (((buff[i-1] & 0x3f) == 0 ) && ((buff[i+1] & 0xc0) == 0x40)) val =  6;
					else if (((buff[i-1] & 0x7f) == 0 ) && ((buff[i+1] & 0x80) == 0x80)) val =  7;
					else                                                                 val = -1;
					
					if (val != -1)
					{
						size = 8*i-val - start_bit;
						
						//extract next gn
						next_gn = 0;
						{
							int i;
							for (i=0;i<4;i++)
							{
								next_gn *= 2;
								next_gn += ((buff[(start_bit+size+16+i)/8] << (start_bit+size+16+i)%8)%256)>> 7;
							}
						}

					}
				}
				i++;
			}
			else
			{
				//printf("reading\n");
				int readed = fSource->Read(&buff[buff_content], 512);
				buff_content += readed;
				if (readed != 512)
				{
					switch (readed)
					{
					case 0:
					case B_ERROR:
						return 2;
					default:
						size = 8*buff_content - start_bit;
						next_gn = 0;
					}
				}
			}		
		}
	}
	
	//printf("size = %5d (next_gn = %2d)\n", size, next_gn);
	
	//decode GOB/packet
	p64_decoder.decode
			(
				buff,
				(start_bit+size+7)/8,
				start_bit,
				(8 - (start_bit+size)%8)%8,
				0, //mba   (start with gob header)
				0, //gob   (start with gob header)
				0, //quant (start with gob header)
				0, //mvdh  (start with gob header)
				0  //mvdv  (start with gob header)			
			);

	
	//shifting the content of the buffer
	{
		int shift_amount = (start_bit + size) / 8;
		
		memmove(buff, &buff[shift_amount], buff_content-shift_amount);
		buff_content -= shift_amount;
		
		start_bit = (start_bit + size) % 8;
	}		
	
	//this is the last GOB/packet of this picture if the next 'GOB' is a picture header 	
	if (next_gn == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

