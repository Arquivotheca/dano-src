/*
	
	VideoConversions.cpp
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#include "VideoConversions.h"
#include "Tables.h"

#define TRACE(x...)

//-----------------------------------------------------------------

BTimecode
FrameNumberToTimecode(uint32 frameNumber, timecodetype type)
{
	BTimecode tc;
	uint32	scores_of_minutes, tens_of_minutes, ones_of_minutes, residual_frames;
	
	switch (tc.type = type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			tc.hour			=	frameNumber     / (60*60*30);
			residual_frames	=	frameNumber     % (60*60*30);
			tc.minute		=	residual_frames / (60*30);
			residual_frames	=	residual_frames % (60*30);
			tc.second		=	residual_frames / (30);
			tc.frame		=	residual_frames % (30);
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			tc.hour			=	frameNumber     / (60*60*25);
			residual_frames	=	frameNumber     % (60*60*25);
			tc.minute		=	residual_frames / (60*25);
			residual_frames	=	residual_frames % (60*25);
			tc.second		=	residual_frames / (25);
			tc.frame		=	residual_frames % (25);
			break;
		case B_24_FPS:
			tc.hour			=	frameNumber     / (60*60*24);
			residual_frames	=	frameNumber     % (60*60*24);
			tc.minute		=	residual_frames / (60*24);
			residual_frames	=	residual_frames % (60*24);
			tc.second		=	residual_frames / (24);
			tc.frame		=	residual_frames % (24);
			break;
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
			// Needs to be checked
			tc.hour			=	frameNumber     / (60*59*30 + 54*1*28 + 6*1*30);
			residual_frames	=	frameNumber     % (60*59*30 + 54*1*28 + 6*1*30);
			tens_of_minutes	=	residual_frames / (10*59*30 + 9 *1*28 + 1*1*30);
			residual_frames	=	residual_frames % (10*59*30 + 9 *1*28 + 1*1*30);
		
			//0, 10, 20, 30, 40, 50 minutes no drop
			if (residual_frames < 60*30)	
			{
				ones_of_minutes = 0;
				tc.second		=	residual_frames / (30);
				tc.frame		=	residual_frames % (30);	
			}
			//other minutes drop the first two frames
			else	
			{
				ones_of_minutes	= 	residual_frames / (59*30 + 1*28);
				residual_frames	=	frameNumber     % (59*30 + 1*28);
				if(residual_frames < 28)
				{
					tc.second 	= 	0;
					tc.frame	=	residual_frames + 2;
				}
				else
				{
					tc.second	=	(residual_frames + 2) / (30);
					tc.frame	=	frameNumber + 2 % (30);
				}
			}
			tc.minute = tens_of_minutes * 10 + ones_of_minutes;
			break;
		case B_VIDEO_DROP_PAL_M:
			// Needs to be checked
			tc.hour				=	frameNumber     / (60*59*30 + 27*1*26 + 33*1*30);
			residual_frames		=	frameNumber     % (60*59*30 + 27*1*26 + 33*1*30);
			scores_of_minutes	=	residual_frames / (20*59*30 + 9 *1*26 + 11*1*30);
			residual_frames		=	residual_frames % (20*59*30 + 9 *1*26 + 11*1*30);
		
			//0, 20, 40 minutes DON'T drop
			if (residual_frames < 60*30)	
			{
				ones_of_minutes	= 0;
				tc.second		=	residual_frames / (30);
				tc.frame		=	residual_frames % (30);	
			}
			else
			{
				//subtract four frames to compensate for the score minute
				ones_of_minutes	= 	(residual_frames - 4) / (59*30 + 26/2 + 30/2);
				//odd minutes DON'T drop
				if(ones_of_minutes % 2)
				{
					//subtract two frames to compensate for the score and even minutes
					residual_frames	=	(residual_frames - 2) % (59*30 + 26/2 + 30/2);
					tc.second		=	residual_frames / (30);
					tc.frame		=	residual_frames % (30);
				}
				//even minutes drop the first four frames of the first second
				else
				{
					//subtract four frames to compensate for the score minute
					residual_frames	=	(residual_frames - 4) 	% (59*30 + 26/2 + 30/2);
					tc.second		=	(residual_frames + 4) 	/ (30);
					tc.frame		=	(residual_frames + 4) 	% (30);
				}
			}
			tc.minute = scores_of_minutes * 20 + ones_of_minutes;
			break;
			
	}
	
	//printf("Frame #%06d Timecode %02d:%02d:%02d %02d\r",frameNumber,
	//		tc.hour,tc.minute,tc.second,tc.frame);
	return tc;
		
}

//-----------------------------------------------------------------

uint32
TimecodeToFrameNumber(BTimecode tc)
{
	switch (tc.type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			return (tc.hour*60*60*30 + tc.minute*60*30 + tc.second*30 + tc.frame);
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			return (tc.hour*60*60*25 + tc.minute*60*25 + tc.second*25 + tc.frame);
		case B_24_FPS:
			return (tc.hour*60*60*24 + tc.minute*60*24 + tc.second*24 + tc.frame);
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
			return (tc.hour*60*60*30 + tc.minute*60*30 + tc.second*30 + tc.frame);
		case B_VIDEO_DROP_PAL_M:
			return (tc.hour*60*60*30 + tc.minute*60*30 + tc.second*30 + tc.frame);
	}
	return 0; // if undefined type
}


//-----------------------------------------------------------------

status_t
VideoImageToBitmap(BVideoImage *videoImage, BBitmap *bitmap)
{ 
	int i, count;
	uint x, y;
	uint32 *u32src_ptr;
	uint32 *u32dst_ptr;
	uint32 p, p1, p2, p3;
	uint8 *u8dst_ptr;
	const color_map *colors;

	if (!videoImage->IsValid() || !bitmap->IsValid())
	{
		TRACE("invalid bitmap or videoimage\n");
		return B_ERROR;
	}
			
	uint32 videoX			=  (uint32) videoImage->ImageSize().x;
	uint32 videoY			= (uint32) videoImage->ImageSize().y;
	uint32 video_rowbytes	= videoImage->BytesPerRow();
	TRACE("Video:  %d x %d rowbytes = %d\n", videoX, videoY, video_rowbytes);

	uint32 bitmapX			= (uint32) (bitmap->Bounds().right - bitmap->Bounds().left + 1); 
	uint32 bitmapY			= (uint32) (bitmap->Bounds().bottom - bitmap->Bounds().top + 1);
	uint32 bitmap_rowbytes	= bitmap->BytesPerRow(); 
	TRACE("Bitmap: %d x %d rowbytes = %d\n", bitmapX, bitmapY, bitmap_rowbytes);
			
	if ( (bitmapX !=  videoX) || (bitmapY !=  videoY) )
	{
		TRACE("Image size mismatch\n");
		return B_ERROR;
	}
												 
	switch (bitmap->ColorSpace())
	{
		case B_RGB_32_BIT:
			TRACE("Target is RGB32\n");
			u32src_ptr = (uint32 *)videoImage->Buffer();
			u32dst_ptr = (uint32 *)bitmap->Bits();
			count = videoX * videoY;
			switch(videoImage->ColorSpace())
			{
				case B_RGB32:
					TRACE("Source is RGB32\n");
					for (y = 0; y < videoY; y++)
					{
						for (x = 0; x < videoX; x++)
						{
							*u32dst_ptr++ = *u32src_ptr++;
						}
						u32dst_ptr += (bitmap_rowbytes/4 - videoX);
						u32src_ptr += (video_rowbytes/4 - videoX);				
					}
					break;
				case B_RGB32_BIG:
					TRACE("Source is RGB32_BIG\n");
					for (y = 0; y < videoY; y++)
					{
						for (x = 0; x < videoX; x++)
						{
						p = *u32src_ptr++;
						*u32dst_ptr++ = p >> 24 |
										(p & 0x00ff0000) >> 8  |
										(p & 0x0000ff00) << 8  |
										p << 24;
						}
						u32dst_ptr += (bitmap_rowbytes/4 - videoX);
						u32src_ptr += (video_rowbytes/4 - videoX);				
					}
					break;
				case B_RGB24:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p1 = *u32src_ptr++;				
								p2 = *u32src_ptr++;				
								p3 = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p1 & 0xffffff00);
								*u32dst_ptr++	= 	(p2 & 0xffff0000) >> 8  |
													(p1 & 0x000000ff) << 24; 						
								*u32dst_ptr++	= 	(p3 & 0xff000000) >> 16 |
													(p2 & 0x0000ffff) << 16; 						
								*u32dst_ptr++	= 	(p3)              << 8;
							}
							switch (videoX%4)
							{
								case 0:
									continue;
								case 1:
									p1 = *u32src_ptr++;
									*u32dst_ptr++	= 	(p1 & 0xffffff00);
									break;
								case 2:
									p1 = *u32src_ptr++;
									p2 = *u32src_ptr++;
									*u32dst_ptr++	= 	(p1 & 0xffffff00);
									*u32dst_ptr++	= 	(p2 & 0xffff0000) >> 8  |
														(p1 & 0x000000ff) << 24;
									break;
								case 3:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									p3 = *u32src_ptr++;				
									*u32dst_ptr++	= 	(p1 & 0xffffff00);
									*u32dst_ptr++	= 	(p2 & 0xffff0000) >> 8  |
														(p1 & 0x000000ff) << 24; 						
									*u32dst_ptr++	= 	(p3 & 0xff000000) >> 16 |
														(p2 & 0x0000ffff) << 16; 						
								 	break;
								 default:
								 	break;					
							}
						}
					}
					else  /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p1 = *u32src_ptr++;				
								p2 = *u32src_ptr++;				
								p3 = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p1 & 0x00ffffff);
								*u32dst_ptr++	= 	(p1 & 0xff000000) >> 24 |
													(p2 & 0x0000ffff) << 8;
								*u32dst_ptr++	= 	(p2 & 0xffff0000) >> 16 |
													(p3 & 0x000000ff) << 16; 						
								*u32dst_ptr++	= 	(p3 >> 8);
							}
							switch (videoX%4)
							{
								case 0:
									continue;
								case 1:
									p1 = *u32src_ptr++;												
									*u32dst_ptr++	= 	(p1 & 0x00ffffff);
									break;
								case 2:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									*u32dst_ptr++	= 	(p1 & 0x00ffffff);
									*u32dst_ptr++	= 	(p1 & 0xff000000) >> 24 |
														(p2 & 0x0000ffff) << 8;
									break;
								case 3:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									p3 = *u32src_ptr++;				
									*u32dst_ptr++	= 	(p1 & 0x00ffffff);
									*u32dst_ptr++	= 	(p1 & 0xff000000) >> 24 |
														(p2 & 0x0000ffff) << 8;
									*u32dst_ptr++	= 	(p2 & 0xffff0000) >> 16 |
														(p3 & 0x000000ff) << 16; 						
									break;
								default:
									break;
							}
						}
						/* need to add code to finish the remaining 0-3 pixels */
					}
					break;
				case B_RGB24_BIG:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p1 = *u32src_ptr++;				
								p2 = *u32src_ptr++;				
								p3 = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p1 & 0x00ff0000) >> 8  |
													(p1 & 0x0000ff00) << 8  |
													(p1 & 0x000000ff) << 24; 
								*u32dst_ptr++	= 	(p2 & 0x0000ff00)       |
													(p2 & 0x000000ff) << 16 |
													(p1 & 0xff000000); 						
								*u32dst_ptr++	= 	(p3 & 0x000000ff) << 8  |
													(p2 & 0xff000000) >> 8  |
													(p2 & 0x00ff0000) << 8; 						
								*u32dst_ptr++	= 	(p3 & 0xff000000) >> 16 |
													(p3 & 0x00ff0000)       |
													(p3 & 0x0000ff00) << 16; 
							}
							switch (videoX%4)
							{
								case 0:
									continue;
								case 1:
									p1 = *u32src_ptr++;				
									*u32dst_ptr++	= 	(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8  |
														(p1 & 0x000000ff) << 24; 
									break;
								case 2:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8  |
														(p1 & 0x000000ff) << 24; 
									*u32dst_ptr++	= 	(p2 & 0x0000ff00)       |
														(p2 & 0x000000ff) << 16 |
														(p1 & 0xff000000); 						
									break;
								case 3:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									p3 = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8  |
														(p1 & 0x000000ff) << 24; 
									*u32dst_ptr++	= 	(p2 & 0x0000ff00)       |
														(p2 & 0x000000ff) << 16 |
														(p1 & 0xff000000); 						
									*u32dst_ptr++	= 	(p3 & 0x000000ff) << 8  |
														(p2 & 0xff000000) >> 8  |
														(p2 & 0x00ff0000) << 8; 						
									break;
								default:
									break;
							}						
						}
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p1 = *u32src_ptr++;				
								p2 = *u32src_ptr++;				
								p3 = *u32src_ptr++;				
								
								*u32dst_ptr++	=	(p1 & 0xff000000) >> 24 |
													(p1 & 0x00ff0000) >> 8  |
													(p1 & 0x0000ff00) << 8;
								*u32dst_ptr++	= 	(p2 & 0xff000000) >> 16 |
													(p2 & 0x00ff0000)       |
													(p1 & 0x000000ff);	
								*u32dst_ptr++	= 	(p3 & 0xff000000) >> 8  |
													(p2 & 0x000000ff) << 8  |
													(p2 & 0x0000ff00) >> 8;
								*u32dst_ptr++	= 	(p3 & 0x000000ff) << 16 |
													(p3 & 0x0000ff00)       |
													(p3 & 0x00ff0000) >> 16;
							}
							switch (videoX%4)
							{
								case 0:
									continue;
								case 1:
									p1 = *u32src_ptr++;				
									*u32dst_ptr++	=	(p1 & 0xff000000) >> 24 |
														(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8;
									break;
								case 2:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									*u32dst_ptr++	=	(p1 & 0xff000000) >> 24 |
														(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8;
									*u32dst_ptr++	= 	(p2 & 0xff000000) >> 16 |
														(p2 & 0x00ff0000)       |
														(p1 & 0x000000ff);	
									break;
								case 3:
									p1 = *u32src_ptr++;				
									p2 = *u32src_ptr++;				
									p3 = *u32src_ptr++;				
									
									*u32dst_ptr++	=	(p1 & 0xff000000) >> 24 |
														(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8;
									*u32dst_ptr++	= 	(p2 & 0xff000000) >> 16 |
														(p2 & 0x00ff0000)       |
														(p1 & 0x000000ff);	
									*u32dst_ptr++	= 	(p3 & 0xff000000) >> 8  |
														(p2 & 0x000000ff) << 8  |
														(p2 & 0x0000ff00) >> 8;
									*u32dst_ptr++	= 	(p3 & 0x000000ff) << 16 |
														(p3 & 0x0000ff00)       |
														(p3 & 0x00ff0000) >> 16;
									break;
								default:
									break;
							}						
						}
					}
					break;
				case B_RGB16:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x00f80000) >> 8  |
													(p & 0x00070000) << 5  |
													(p & 0xe0000000) >> 11 |
													(p & 0x1f000000) << 3; 
								*u32dst_ptr++	= 	(p & 0x000000f8) << 8  |
													(p & 0x00000007) << 21 |
													(p & 0x0000e000) << 5  |
													(p & 0x00001f00) << 19;
							}
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x00f80000) >> 8  |
														(p & 0x00070000) << 5  |
														(p & 0xe0000000) >> 11 |
														(p & 0x1f000000) << 3; 
									break;
								default:
									break;
							}	 						
						}
						/* need to add code to finish the remaining 0-1 pixels */
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x0000f800) << 8  |
													(p & 0x000007e0) << 5  |
													(p & 0x0000001f) << 3; 
								*u32dst_ptr++	= 	(p & 0xf8000000) >> 8  |
													(p & 0x07e00000) >> 11 |
													(p & 0x001f0000) >> 13;
							} 						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x0000f800) << 8  |
														(p & 0x000007e0) << 5  |
														(p & 0x0000001f) << 3; 
									break;
								default:
									break;
							}	 						
						}
					}
					break;
				case B_RGB16_BIG:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x0000f800)       |
													(p & 0x000007e0) << 13 |
													(p & 0x0000001f) << 27; 
								*u32dst_ptr++	= 	(p & 0xf8000000) >> 16 |
													(p & 0x07e00000) >> 3  |
													(p & 0x001f0000) << 11;
							}						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x0000f800)       |
														(p & 0x000007e0) << 13 |
														(p & 0x0000001f) << 27; 
									break;
								default:
									break;
							}	 						
						}
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x00f80000)       |
													(p & 0x00070000) >> 3  |
													(p & 0xe0000000) >> 19 |
													(p & 0x1f000000) >> 21; 
								*u32dst_ptr++	= 	(p & 0x000000f8) << 16 |
													(p & 0x00000007) << 13 |
													(p & 0x0000e000) >> 3  |
													(p & 0x00001f00) >> 5;
							} 						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x00f80000)       |
														(p & 0x00070000) >> 3  |
														(p & 0xe0000000) >> 19 |
														(p & 0x1f000000) >> 21; 
									break;
								default:
									break;
							}	 						
						}
					}
					break;
				case B_RGB15:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x007c0000) >> 7  |
													(p & 0x00030000) << 6  |
													(p & 0xe0000000) >> 10 |
													(p & 0x1f000000) << 3; 
								*u32dst_ptr++	= 	(p & 0x0000007c) << 9  |
													(p & 0x00000003) << 22 |
													(p & 0x0000e000) << 6  |
													(p & 0x00001f00) << 19;
							} 						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x007c0000) >> 7  |
														(p & 0x00030000) << 6  |
														(p & 0xe0000000) >> 10 |
														(p & 0x1f000000) << 3; 
									break;
								default:
									break;
							}	 						
						}
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x00007c00) << 9  |
													(p & 0x000003e0) << 6  |
													(p & 0x0000001f) << 3; 
								*u32dst_ptr++	= 	(p & 0x7c000000) >> 7  |
													(p & 0x03e00000) >> 10 |
													(p & 0x001f0000) >> 13;
							} 						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x00007c00) << 9  |
														(p & 0x000003e0) << 6  |
														(p & 0x0000001f) << 3; 
									break;
								default:
									break;
							}	 						
						}
					}
					break;
				case B_RGB15_BIG:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x00007c00) << 1  |
													(p & 0x000003e0) << 14 |
													(p & 0x0000001f) << 27; 
								*u32dst_ptr++	= 	(p & 0x7c000000) >> 15 |
													(p & 0x03e00000) >> 2  |
													(p & 0x001f0000) << 11;
							} 						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x00007c00) << 1  |
														(p & 0x000003e0) << 14 |
														(p & 0x0000001f) << 27; 
									break;
								default:
									break;
							}	 						
						}
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/2; x++)
							{
								p = *u32src_ptr++;				
								
								*u32dst_ptr++	= 	(p & 0x007c0000) << 1  |
													(p & 0x00030000) >> 2  |
													(p & 0xe0000000) >> 19 |
													(p & 0x1f000000) >> 21; 
								*u32dst_ptr++	= 	(p & 0x0000007c) << 17 |
													(p & 0x00000003) << 14 |
													(p & 0x0000e000) >> 2  |
													(p & 0x00001f00) >> 5;
							} 						
							switch (videoX%2)
							{
								case 0:
									continue;
								case 1:
									p = *u32src_ptr++;				
									
									*u32dst_ptr++	= 	(p & 0x007c0000) << 1  |
														(p & 0x00030000) >> 2  |
														(p & 0xe0000000) >> 19 |
														(p & 0x1f000000) >> 21; 
									break;
								default:
									break;
							}	 						
						}
					}
					break;
				case B_CMAP8:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p  = RGB8Map[(*u32src_ptr & 0xff000000) >> 24];				
								p1 = RGB8Map[(*u32src_ptr & 0x00ff0000) >> 16];				
								p2 = RGB8Map[(*u32src_ptr & 0x0000ff00) >> 8 ];				
								p3 = RGB8Map[(*u32src_ptr++ & 0x000000ff)    ];
												
								*u32dst_ptr++	= 	p >> 24 |
													(p & 0x00ff0000) >> 8  |
													(p & 0x0000ff00) << 8  |
													p << 24;
								*u32dst_ptr++	= 	p1 >> 24 |
													(p1 & 0x00ff0000) >> 8  |
													(p1 & 0x0000ff00) << 8  |
													p << 24;
								*u32dst_ptr++	= 	p2 >> 24 |
													(p2 & 0x00ff0000) >> 8  |
													(p2 & 0x0000ff00) << 8  |
													p2 << 24;
								*u32dst_ptr++	= 	p3 >> 24 |
													(p3 & 0x00ff0000) >> 8  |
													(p3 & 0x0000ff00) << 8  |
													p3 << 24;
							}
							switch(videoX%4)
							{
								case 0:
									continue;
								case 1:
									p  = RGB8Map[(*u32src_ptr++ & 0xff000000) >> 24];				
									*u32dst_ptr++	= 	p >> 24 |
														(p & 0x00ff0000) >> 8  |
														(p & 0x0000ff00) << 8  |
														p << 24;
									break;
								case 2:
									p  = RGB8Map[(*u32src_ptr & 0xff000000) >> 24];				
									p1 = RGB8Map[(*u32src_ptr++ & 0x00ff0000) >> 16];				
													
									*u32dst_ptr++	= 	p >> 24 |
														(p & 0x00ff0000) >> 8  |
														(p & 0x0000ff00) << 8  |
														p << 24;
									*u32dst_ptr++	= 	p1 >> 24 |
														(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8  |
														p << 24;
									break;
								case 3:
									p  = RGB8Map[(*u32src_ptr & 0xff000000) >> 24];				
									p1 = RGB8Map[(*u32src_ptr & 0x00ff0000) >> 16];				
									p2 = RGB8Map[(*u32src_ptr++ & 0x0000ff00) >> 8 ];				
									*u32dst_ptr++	= 	p >> 24 |
														(p & 0x00ff0000) >> 8  |
														(p & 0x0000ff00) << 8  |
														p << 24;
									*u32dst_ptr++	= 	p1 >> 24 |
														(p1 & 0x00ff0000) >> 8  |
														(p1 & 0x0000ff00) << 8  |
														p << 24;
									*u32dst_ptr++	= 	p2 >> 24 |
														(p2 & 0x00ff0000) >> 8  |
														(p2 & 0x0000ff00) << 8  |
														p2 << 24;
									break;
								default:
									break;
							}
						}
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								*u32dst_ptr++ = RGB8Map[(*u32src_ptr & 0x000000ff)        ];				
								*u32dst_ptr++ = RGB8Map[(*u32src_ptr & 0x0000ff00)   >> 8 ];				
								*u32dst_ptr++ = RGB8Map[(*u32src_ptr & 0x00ff0000)   >> 16];				
								*u32dst_ptr++ = RGB8Map[(*u32src_ptr++ & 0xff000000) >> 24];
							}
							switch(videoX%4)
							{
								case 0:
									continue;
								case 1:
									*u32dst_ptr++ = RGB8Map[(*u32src_ptr++ & 0x000000ff)        ];				
									break;
								case 2:
									*u32dst_ptr++ = RGB8Map[(*u32src_ptr & 0x000000ff)        ];				
									*u32dst_ptr++ = RGB8Map[(*u32src_ptr++ & 0x0000ff00)   >> 8 ];				
									break;
								case 3:
									*u32dst_ptr++ = RGB8Map[(*u32src_ptr & 0x000000ff)        ];				
									*u32dst_ptr++ = RGB8Map[(*u32src_ptr & 0x0000ff00)   >> 8 ];				
									*u32dst_ptr++ = RGB8Map[(*u32src_ptr++ & 0x00ff0000)   >> 16];				
									break;
								default:
									break;
							}
						}
					}
					break;
				case B_GRAY8:
					if (B_HOST_IS_BENDIAN)
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p  = (*u32src_ptr & 0xff000000) >> 24;
								p1 = (*u32src_ptr & 0x00ff0000) >> 16;				
								p2 = (*u32src_ptr & 0x0000ff00) >> 8;				
								p3 = *u32src_ptr++ & 0x000000ff;				
												
								*u32dst_ptr++	= 	p << 8  |
													p << 16 |
													p << 24;
								*u32dst_ptr++	= 	p1 << 8  |
													p1 << 16 |
													p1 << 24;
								*u32dst_ptr++	= 	p2 << 8  |
													p2 << 16 |
													p2 << 24;
								*u32dst_ptr++	= 	p3 << 8  |
													p3 << 16 |
													p3 << 24;
							}
							switch (videoX%4)
							{
								case 0:
									continue;
								case 1:
									p  = (*u32src_ptr++ & 0xff000000) >> 24;
									*u32dst_ptr++	= 	p << 8  |
														p << 16 |
														p << 24;
									break;
								case 2:
									p  = (*u32src_ptr & 0xff000000) >> 24;
									p1 = (*u32src_ptr++ & 0x00ff0000) >> 16;				
									*u32dst_ptr++	= 	p << 8  |
														p << 16 |
														p << 24;
									*u32dst_ptr++	= 	p1 << 8  |
														p1 << 16 |
														p1 << 24;
									break;
								case 3:
									p  = (*u32src_ptr & 0xff000000) >> 24;
									p1 = (*u32src_ptr & 0x00ff0000) >> 16;				
									p2 = (*u32src_ptr++ & 0x0000ff00) >> 8;				
									*u32dst_ptr++	= 	p << 8  |
														p << 16 |
														p << 24;
									*u32dst_ptr++	= 	p1 << 8  |
														p1 << 16 |
														p1 << 24;
									*u32dst_ptr++	= 	p2 << 8  |
														p2 << 16 |
														p2 << 24;
									break;
								default:
									break;
							}
						}
					}
					else /* Little Endian Host */
					{
						for (y = 0; y < videoY; y++)
						{
							for (x = 0; x < videoX/4; x++)
							{
								p  = *u32src_ptr & 0x000000ff;				
								p1 = (*u32src_ptr & 0x0000ff00) >> 8;				
								p2 = (*u32src_ptr & 0x00ff0000) >> 16;				
								p3 = (*u32src_ptr++ & 0xff000000) >> 24;
												
								*u32dst_ptr++	= 	p       |
													p << 8  |
													p << 16;
								*u32dst_ptr++	= 	p1       |
													p1 << 8  |
													p1 << 16;
								*u32dst_ptr++	= 	p2       |
													p2 << 8  |
													p2 << 16;
								*u32dst_ptr++	= 	p3       |
													p3 << 8  |
													p3 << 16;
							}
							switch (videoX%4)
							{
								case 0:
									continue;
								case 1:
									p  = *u32src_ptr++ & 0x000000ff;				
									*u32dst_ptr++	= 	p       |
														p << 8  |
														p << 16;
									break;
								case 2:
									p  = *u32src_ptr & 0x000000ff;				
									p1 = (*u32src_ptr++ & 0x0000ff00) >> 8;				
									*u32dst_ptr++	= 	p       |
														p << 8  |
														p << 16;
									*u32dst_ptr++	= 	p1       |
														p1 << 8  |
														p1 << 16;
									break;
								case 3:
									p  = *u32src_ptr & 0x000000ff;				
									p1 = (*u32src_ptr & 0x0000ff00) >> 8;				
									p2 = (*u32src_ptr++ & 0x00ff0000) >> 16;				
									*u32dst_ptr++	= 	p       |
														p << 8  |
														p << 16;
									*u32dst_ptr++	= 	p1       |
														p1 << 8  |
														p1 << 16;
									*u32dst_ptr++	= 	p2       |
														p2 << 8  |
														p2 << 16;
									break;
								default:
									break;
							}
						}
					}
					break;					
				case B_RGBA32:
				case B_RGBA32_BIG:
				case B_YUV422:
				case B_YUV411:
				case B_YUV420:
				default:
					return B_ERROR;
			}
			return B_NO_ERROR;
		case B_RGB_16_BIT:
			u32src_ptr = (uint32 *)videoImage->Buffer();
			u32dst_ptr = (uint32 *)bitmap->Bits();
			count = (uint32) (videoImage->ImageSize().x * videoImage->ImageSize().y);
			switch(videoImage->ColorSpace())
			{
				case B_RGB32:
				case B_RGB32_BIG:
				case B_RGBA32:
				case B_RGBA32_BIG:
				case B_RGB24:
				case B_RGB24_BIG:
				case B_RGB16:
				case B_RGB16_BIG:
				case B_RGB15:
				case B_RGB15_BIG:
				case B_CMAP8:
				case B_GRAY8:
				case B_YUV422:
				case B_YUV411:
				case B_YUV420:
				default:
					return B_ERROR;
			}
			return B_NO_ERROR;
		case B_COLOR_8_BIT:
   			u32src_ptr = (uint32 *)videoImage->Buffer();
			u8dst_ptr = (uint8 *)bitmap->Bits();
			count = (uint32) (videoImage->ImageSize().x * videoImage->ImageSize().y);
			colors = system_colors();
			switch(videoImage->ColorSpace())
			{
				case B_RGB32:
					if (B_HOST_IS_BENDIAN)
					{
						for (i = 0; i < count; i++)
						{
							p = *u32src_ptr++;
							*u8dst_ptr++ = colors->index_map[	(p & 0x0000f800) >> 1  |
																(p & 0x00f80000) >> 14 |
																(p & 0xf8000000) >> 27];
						}
					}
					else /* Little Endian Host */
					{
						for (i = 0; i < count; i++)
						{
							p = *u32src_ptr++;
							*u8dst_ptr++ = colors->index_map[	(p & 0x00f80000) >> 9 |
																(p & 0x0000f800) >> 6 |
																(p & 0x000000f8) >> 3];
						}
					}
					break;
				case B_RGB32_BIG:
					if (B_HOST_IS_BENDIAN)
					{
					}
					else /* Little Endian Host */
					{
					}
					break;
				case B_RGBA32:
				case B_RGBA32_BIG:
				case B_RGB24:
				case B_RGB24_BIG:
				case B_RGB16:
				case B_RGB16_BIG:
				case B_RGB15:
				case B_RGB15_BIG:
				case B_CMAP8:
				case B_GRAY8:
				case B_YUV422:
				case B_YUV411:
				case B_YUV420:
				default:
					return B_ERROR;
			}
			return B_NO_ERROR;
		case B_GRAYSCALE_8_BIT:
			u32src_ptr = (uint32 *)videoImage->Buffer();
			u8dst_ptr = (uint8 *)bitmap->Bits();
			count = (uint32) (videoImage->ImageSize().x * videoImage->ImageSize().y);
			switch(videoImage->ColorSpace())
			{
				case B_RGB32:
				case B_RGB32_BIG:
				case B_RGBA32:
				case B_RGBA32_BIG:
				case B_RGB24:
				case B_RGB24_BIG:
				case B_RGB16:
				case B_RGB16_BIG:
				case B_RGB15:
				case B_RGB15_BIG:
				case B_CMAP8:
				case B_GRAY8:
				case B_YUV422:
				case B_YUV411:
				case B_YUV420:
				default:
					return B_ERROR;
			}
			return B_NO_ERROR;
		case B_MONOCHROME_1_BIT:
			u32src_ptr = (uint32 *)videoImage->Buffer();
			u8dst_ptr = (uint8 *)bitmap->Bits();
			count = (uint32) (videoImage->ImageSize().x * videoImage->ImageSize().y);
			switch(videoImage->ColorSpace())
			{
				case B_RGB32:
				case B_RGB32_BIG:
				case B_RGBA32:
				case B_RGBA32_BIG:
				case B_RGB24:
				case B_RGB24_BIG:
				case B_RGB16:
				case B_RGB16_BIG:
				case B_RGB15:
				case B_RGB15_BIG:
				case B_CMAP8:
				case B_GRAY8:
				case B_YUV422:
				case B_YUV411:
				case B_YUV420:
				default:
					return B_ERROR;
			}
			return B_NO_ERROR;
		default:
			return B_ERROR;
	}
	return B_NO_ERROR;
}

