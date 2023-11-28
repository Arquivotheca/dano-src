//==============================================================================
#include "source.h261_encoder.h"
//==============================================================================
#include <stdio.h>
//==============================================================================
BPrivate::Encoder *
instantiate_nth_encoder(int32 index)
{
	if(index == 0)
	{
		return new H261Encoder();
	}
	else
	{
		return NULL;
	}
}
//==============================================================================
H261Encoder::H261Encoder()
{
}
//==============================================================================
H261Encoder::~H261Encoder()
{
}
//==============================================================================
status_t 
H261Encoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "H.261");
	strcpy(mci->short_name,  "h261-video");
	
	return B_NO_ERROR;
}
//==============================================================================
status_t
H261Encoder::SetFormat
(
	media_file_format *mfi,
  	media_format *in_fmt,
  	media_format *out_fmt
)
{
	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_fmt->require_flags = 0;

	if (in_fmt->type != B_MEDIA_RAW_VIDEO)
	{
		in_fmt->type = B_MEDIA_RAW_VIDEO;
		in_fmt->u.raw_video = media_raw_video_format::wildcard;
		in_fmt->u.raw_video.display.format = B_YCbCr422;
	}
	else if (in_fmt->u.raw_video.display.format != B_YCbCr422)
	{
		in_fmt->u.raw_video.display.format = B_YCbCr422;	
	}
	else	
	{
		//===========
		width  = in_fmt->u.raw_video.display.line_width;
		height = in_fmt->u.raw_video.display.line_count ;
		bpr    = in_fmt->u.raw_video.display.bytes_per_row;
		format = 77;
		if (in_fmt->u.raw_video.display.format == B_YCbCr422)
		{
			format = 422;	
		}
		else
		{
			format = 0;
			printf("NOT 422 : %d\n", in_fmt->u.raw_video.display.format);
			//in_fmt->u.raw_video.display.format = B_YCbCr422;
		}
		printf("encoding a %dx%d sequence\n", width, height);
		//===========
	}
		
	*out_fmt = *in_fmt;

	return B_OK;
}
//==============================================================================
status_t
H261Encoder::Encode(const void *in_buffer, int64 num_frames,
                   media_encode_info *info)
{
	printf("\n");
	info->flags = B_MEDIA_KEY_FRAME;
	
	
	printf("===\n");
	printf("num_frames = %d (%dx%d @ %d), %d\n", (int)num_frames, width, height, format, bpr);
	
	
	unsigned char *buffer = (unsigned char *)in_buffer;
	
	//pointer to the frame in the good format
	unsigned char *buffer_352_288_422;
	
	//buffer to write converted frame, base of centered sub frame (if necessary)
	static unsigned char *frame_352_288 = NULL;
	static unsigned char *frame_352_288_base = NULL;

	//check if need to make a conversion
	static bool first_frame_encoded = true;
	if (first_frame_encoded)
	{
		if ((width == 352) && (height == 288) && (bpr == 352*2))
		{
			//no conversion needed
		}
		else
		{
			//conversion needed
			frame_352_288 = new unsigned char[352*288*2];
			
			//we put gray in the unused background
			memset(frame_352_288, 128, 352*288*2);
			
			//calculation of the base for the centered sub frame
			{
				int base_x;
				base_x  = 352-width;	//number of missing pixels
				base_x /= 2;		 	//divide between sides
				base_x -= base_x%2;		//make sure it's even (because of 422)
				
				int base_y;
				base_y  = 288-height;	//number of missing pixels
				base_y /= 2;		 	//divide between sides
				base_y -= base_y%2;		//make sure it's even (because of 422)	

				frame_352_288_base = &frame_352_288[2*base_x + 352*2*base_y];
			}			
		}
		first_frame_encoded = false;
	}
	
	//make 352x288 without any stride
	if (frame_352_288 == NULL)
	{
		//no conversion needed
		buffer_352_288_422 = buffer;
	}
	else	
	{
		int i;
		bigtime_t t1,t2;
		
		t1 = real_time_clock_usecs();
		for (i=0;i<height;i++)
		{			
 			memcpy(&frame_352_288_base[i*352*2],&buffer[i*bpr],width*2);
		}
		t2 = real_time_clock_usecs();
		
		char operation[100];
		sprintf(operation, "expanding %dx%d to 352x288", width, height);
		printf("%40s = %7d usecs\n", operation, (int)(t2-t1));
		
		buffer_352_288_422 = frame_352_288;
	}

	//pass the buffer to the encoding control
	the_encoding_control.new_frame_352_288_422(buffer_352_288_422);
	printf("===\n");

	
	printf("\n");	
	//return WriteChunk(in_buffer, num_frames * frame_size, info);
	return B_OK;
}
//==============================================================================
